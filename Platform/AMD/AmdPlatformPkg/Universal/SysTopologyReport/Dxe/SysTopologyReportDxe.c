/** @file
  System Topology Report driver that reports the hardware information
  of the components on the platforms.

  Copyright (C) 2025 - 2026, Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <IndustryStandard/Nvme.h>
#include <IndustryStandard/Atapi.h>
#include <Protocol/NvmExpressPassthru.h>
#include <Protocol/DiskInfo.h>
#include <Protocol/BlockIo.h>
#include <Protocol/PciIo.h>
#include <Protocol/Smbios.h>
#include <IndustryStandard/SmBios.h>
#include <Library/BaseLib.h>
#include <SysTopologyReportDxe.h>

extern  EFI_BOOT_SERVICES  *gBS;

EFI_SYS_TOPOLOGY_PROTOCOL  SysTopologyProtocol;

STATIC CHAR8  *ErrorCorrectionTypes[] = {
  "Unknown",          // 0x00
  "Other",            // 0x01
  "Unknown",          // 0x02
  "None",             // 0x03
  "Parity",           // 0x04
  "SingleBitECC",     // 0x05
  "MultiBitECC",      // 0x06
  "CRC"               // 0x07
};

/**
  Create and initialize a system topology report context.

  @param[in]  This      Pointer to the EFI_SYS_TOPOLOGY_PROTOCOL instance.
  @param[out] ReportPtr Pointer to the created SYS_TOPOLOGY_REPORT_CONTEXT.

  @retval EFI_SUCCESS           The report was created successfully.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
**/
EFI_STATUS
EFIAPI
CreateReport (
  IN CONST EFI_SYS_TOPOLOGY_PROTOCOL  *This,
  SYS_TOPOLOGY_REPORT_CONTEXT         **ReportPtr
  )
{
  EFI_STATUS                   Status;
  SYS_TOPOLOGY_HEADER          *ReportHeader;
  SYS_TOPOLOGY_REPORT_CONTEXT  *Report;

  DEBUG ((EFI_D_INFO, "Before AllocatePool\n"));
  Status = gBS->AllocatePool (EfiBootServicesData, sizeof (SYS_TOPOLOGY_REPORT_CONTEXT), (VOID **)ReportPtr);
  DEBUG ((EFI_D_INFO, "Context AllocatePool Status = %r\n", Status));
  Report = *ReportPtr;

  DEBUG ((EFI_D_INFO, "Report Address 0x%x\n", Status));

  // Allocate default size report and initialize header
  Status = gBS->AllocatePool (EfiBootServicesData, DEFAULT_REPORT_SIZE, (VOID **)&Report->ReportHeader);
  DEBUG ((EFI_D_INFO, "Report header AllocatePool Status = %r\n", Status));
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_INFO, "Failed to allocate memory for USTR structure - Status = %r\n", Status));
    gBS->FreePool (*ReportPtr);
    *ReportPtr = NULL;
    return Status;
  }

  ReportHeader = Report->ReportHeader;

  ZeroMem (ReportHeader, DEFAULT_REPORT_SIZE);
  ReportHeader->Signature  = SYS_TOPOLOGY_SIGNATURE;
  ReportHeader->Version    = 1;
  ReportHeader->ReportSize = sizeof (SYS_TOPOLOGY_HEADER);
  Report->MemoryAllocated  = DEFAULT_REPORT_SIZE;

  return EFI_SUCCESS;
}

/**
  Free memory allocated for the system topology report context.

  @param[in] Report Pointer to the SYS_TOPOLOGY_REPORT_CONTEXT to free.

  @retval EFI_SUCCESS           The memory was freed successfully.
  @retval EFI_INVALID_PARAMETER The input parameter is invalid.
**/
EFI_STATUS
FreeReportPrivate (
  IN SYS_TOPOLOGY_REPORT_CONTEXT  *Report
  )
{
  if ((Report == NULL) || (Report->ReportHeader == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Free only the ReportHeader buffer; the caller owns the context itself.
  FreePool (Report->ReportHeader);
  Report->ReportHeader = NULL;

  return EFI_SUCCESS;
}

/**
  Free memory allocated for the system topology report context via protocol.

  @param[in] This      Pointer to the EFI_SYS_TOPOLOGY_PROTOCOL instance.
  @param[in] ReportPtr Pointer to the SYS_TOPOLOGY_REPORT_CONTEXT to free.

  @retval EFI_SUCCESS           The memory was freed successfully.
  @retval EFI_INVALID_PARAMETER The input parameter is invalid.
**/
EFI_STATUS
EFIAPI
FreeReport (
  EFI_SYS_TOPOLOGY_PROTOCOL       *This,
  IN SYS_TOPOLOGY_REPORT_CONTEXT  **ReportPtr
  )
{
  SYS_TOPOLOGY_REPORT_CONTEXT  *Report;

  Report = *ReportPtr;

  if ((Report == NULL) || (Report->ReportHeader == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Free memory allocated for Report
  FreePool (Report->ReportHeader);
  Report->ReportHeader = NULL;
  FreePool (Report);
  *ReportPtr = NULL;

  return EFI_SUCCESS;
}

/**
  Insert an entry into the system topology report.

  @param[in] Report Pointer to the SYS_TOPOLOGY_REPORT_CONTEXT.
  @param[in] Entry  Pointer to the SYS_TOPOLOGY_ENTRY_HEADER to insert.

  @retval EFI_SUCCESS           The entry was inserted successfully.
  @retval EFI_INVALID_PARAMETER The input parameter is invalid.
**/
EFI_STATUS
InsertEntry (
  SYS_TOPOLOGY_REPORT_CONTEXT  *Report,
  SYS_TOPOLOGY_ENTRY_HEADER    *Entry
  )
{
  EFI_STATUS           Status;
  SYS_TOPOLOGY_HEADER  *ReportHeader;
  UINT8                *ReportEnd;
  SYS_TOPOLOGY_HEADER  *NewReportHeader;
  UINT32               NewReportSize;

  Status = EFI_SUCCESS;
  if ((Report == NULL) || (Entry == NULL) || (Report->ReportHeader == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ReportHeader = Report->ReportHeader;
  DEBUG ((EFI_D_INFO, "Report->MemoryAllocated = 0x%x | Entry->Length + ReportHeader->ReportSize = 0x%x\n", Report->MemoryAllocated, Entry->Length + ReportHeader->ReportSize));
  // Check if we have enough space
  if (Report->MemoryAllocated <= Entry->Length + ReportHeader->ReportSize) {
    DEBUG ((EFI_D_INFO, "Resizing Record\n"));
    // Resize report
    NewReportSize = Report->MemoryAllocated*2;
    if (NewReportSize > MAX_REPORT_SIZE) {
      NewReportSize = MAX_REPORT_SIZE;
    }

    Status = gBS->AllocatePool (EfiBootServicesData, NewReportSize, (VOID **)&NewReportHeader);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_INFO, "Failed to allocate memory for USTR structure - Status = %r\n", Status));
      return Status;
    }

    CopyMem ((UINT8 *)NewReportHeader, (UINT8 *)ReportHeader, Report->MemoryAllocated);
    FreeReportPrivate (Report);

    // Update header pointer after resize
    Report->ReportHeader    = NewReportHeader;
    Report->MemoryAllocated = NewReportSize;
    ReportHeader            = Report->ReportHeader;
  }

  // Get end of report
  ReportEnd = (UINT8 *)ReportHeader + ReportHeader->ReportSize;
  CopyMem (ReportEnd, Entry, Entry->Length);

  // Update Header
  ReportHeader->ReportSize += Entry->Length;
  ReportHeader->EntryCount++;

  return Status;
}

/**
  Enable memory access for a PCI device by setting the memory space enable bit in the command register.

  @param[in] PciIo Pointer to the EFI_PCI_IO_PROTOCOL instance for the target PCI device.

  @retval EFI_SUCCESS           Memory access was enabled successfully.
  @retval Others                Error returned by PCI read/write operations.
**/
EFI_STATUS
EnableMemoryAccess (
  EFI_PCI_IO_PROTOCOL  *PciIo
  )
{
  EFI_STATUS  Status;
  UINT16      Command;

  Command = 0;
  // Write 1 to BIT1 of command register to enable memory space rw
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint16,
                        4,
                        1,
                        &Command
                        );
  if (!EFI_ERROR (Status)) {
    DEBUG ((EFI_D_INFO, "Command register = 0x%04x\n", Command));
    Command |= BIT1;

    Status = PciIo->Pci.Write (
                          PciIo,
                          EfiPciIoWidthUint16,
                          4,
                          1,
                          &Command
                          );
    DEBUG ((EFI_D_INFO, "EnableMemoryAccess: Pci.Write Status = %r\n", Status));
  }

  return Status;
}

/**
  Disable memory access for a PCI device by clearing the memory space enable bit in the command register.

  @param[in] PciIo Pointer to the EFI_PCI_IO_PROTOCOL instance for the target PCI device.

  @retval EFI_SUCCESS           Memory access was disabled successfully.
  @retval Others                Error returned by PCI read/write operations.
**/
EFI_STATUS
DisableMemoryAccess (
  EFI_PCI_IO_PROTOCOL  *PciIo
  )
{
  EFI_STATUS  Status;
  UINT16      Command;

  Command = 0;
  // Write 1 to BIT1 of command register to enable memory space rw
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint16,
                        4,
                        1,
                        &Command
                        );
  if (!EFI_ERROR (Status)) {
    DEBUG ((EFI_D_INFO, "Command register = 0x%04x\n", Command));
    Command &= (UINT16)(~BIT1 & 0xffff);
    DEBUG ((EFI_D_INFO, "Writing to Command register = 0x%04x\n", Command));
    Status = PciIo->Pci.Write (
                          PciIo,
                          EfiPciIoWidthUint16,
                          4,
                          1,
                          &Command
                          );
    DEBUG ((EFI_D_INFO, "DisableMemoryAccess: Pci.Write Status = %r\n", Status));
  }

  return Status;
}

/**
  Find the offset of a PCI capability structure with the specified CapabilityId.

  @param[in] PciIo         Pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param[in] CapabilityId  The capability ID to search for.

  @retval Offset of the capability structure if found, otherwise 0.
**/
UINT8
FindPciCapabilityStructure (
  EFI_PCI_IO_PROTOCOL  *PciIo,
  UINT8                CapabilityId
  )
{
  UINT8              RegisterOffset;
  UINT8              CapPtrOffset;
  PCI_COMMON_HEADER  PciConfig;
  EFI_STATUS         Status;
  PCI_CAPABILITY_ID  CapHdr;
  UINT8              HeaderType;

  RegisterOffset = 0;
  CapPtrOffset   = 0;
  ZeroMem (&PciConfig, sizeof (PCI_COMMON_HEADER));

  // Read PCI config space to get first capability structure offset
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        0,
                        sizeof (PCI_COMMON_HEADER),
                        &PciConfig
                        );

  DEBUG ((EFI_D_INFO, "FindPciCapabilityStructure: Config space read status = %r\n", Status));
  if (EFI_ERROR (Status)) {
    return 0;
  }

  // Ignore the multifunction bit in header type
  HeaderType = PciConfig.HeaderType & 0x7F;
  DEBUG ((EFI_D_INFO, "FindPciCapabilityStructure: PciConfig.HeaderType = 0x%x\n", HeaderType));
  if ((HeaderType != 0) && (HeaderType != 1)) {
    return 0;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        PCIE_CAP_POINTER,
                        1,
                        &CapPtrOffset
                        );

  DEBUG ((EFI_D_INFO, "FindPciCapabilityStructure: Cap ID pointer read status = %r\n", Status));
  if (EFI_ERROR (Status) || (CapPtrOffset == 0)) {
    return 0;
  }

  // Loop through capability structures looking for CapabilityId
  do {
    Status = PciIo->Pci.Read (
                          PciIo,
                          EfiPciIoWidthUint8,
                          CapPtrOffset,
                          2,
                          &CapHdr
                          );

    DEBUG ((EFI_D_INFO, "  CapId = 0x%x - NextId = 0x%x\n", CapHdr.CapabilityId, CapHdr.NextId));
    if (CapHdr.CapabilityId == CapabilityId) {
      DEBUG ((EFI_D_INFO, "  Found 0x%x ID Structure\n", CapabilityId));
      RegisterOffset = CapPtrOffset;
      break;
    }

    CapPtrOffset = CapHdr.NextId;
  } while (CapHdr.NextId != 0);

  return RegisterOffset;
}

/**
  Find the offset of a PCI extended capability structure with the specified CapabilityId.

  @param[in] PciIo         Pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param[in] CapabilityId  The extended capability ID to search for.

  @retval Offset of the extended capability structure if found, otherwise 0.
**/
UINT16
FindPciExtendedCapabilityStructure (
  EFI_PCI_IO_PROTOCOL  *PciIo,
  UINT16               CapabilityId
  )
{
  UINT16                 RegisterOffset;
  UINT16                 ExtCapPtrOffset;
  PCI_COMMON_HEADER      PciConfig;
  EFI_STATUS             Status;
  PCI_EXT_CAPABILITY_ID  CapHdr;
  UINT8                  HeaderType;

  RegisterOffset  = 0;
  ExtCapPtrOffset = PCIE_EXT_CAP_POINTER;
  ZeroMem (&PciConfig, sizeof (PCI_COMMON_HEADER));

  // Read PCI config space to get header type
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        0,
                        sizeof (PCI_COMMON_HEADER),
                        &PciConfig
                        );

  DEBUG ((EFI_D_INFO, "FindPciExtendedCapabilityStructure: Config space read status = %r\n", Status));
  if (EFI_ERROR (Status)) {
    return 0;
  }

  // Ignore the multifunction bit in header type
  HeaderType = PciConfig.HeaderType & 0x7F;
  DEBUG ((EFI_D_INFO, "FindPciExtendedCapabilityStructure: PciConfig.HeaderType = 0x%x\n", HeaderType));
  if ((HeaderType != 0) && (HeaderType != 1)) {
    return 0;
  }

  // Loop through extended capability structures looking for CapabilityId
  do {
    Status = PciIo->Pci.Read (
                          PciIo,
                          EfiPciIoWidthUint32,
                          ExtCapPtrOffset,
                          1,
                          &CapHdr
                          );

    DEBUG ((EFI_D_INFO, "  ExtCapId = 0x%x - NextId = 0x%x\n", CapHdr.ExtCapabilityId, CapHdr.NextId));
    if (CapHdr.ExtCapabilityId == CapabilityId) {
      DEBUG ((EFI_D_INFO, "  Found 0x%x ID Structure\n", CapabilityId));
      RegisterOffset = ExtCapPtrOffset;
      break;
    }

    if (ExtCapPtrOffset == (CapHdr.NextId & 0xFFFF)) {
      // Stuck in loop, invalid ext cap space
      break;
    }

    ExtCapPtrOffset = (UINT16)(CapHdr.NextId & 0xFFFF);
    DEBUG ((EFI_D_INFO, "ExtCapPtrOffset = 0x%x\n", ExtCapPtrOffset));
  } while (CapHdr.NextId != 0);

  return RegisterOffset;
}

/**
  Dump PCIe record information for debugging.

  @param[in] Entry Pointer to the SYS_TOPOLOGY_PCIE structure.
**/
VOID
DumpPcieRecord (
  SYS_TOPOLOGY_PCIE  *Entry
  )
{
  if (Entry == NULL) {
    return;
  }

  DEBUG ((EFI_D_INFO, "Type              = 0x%x\n", Entry->Type));
  DEBUG ((EFI_D_INFO, "Version           = 0x%x\n", Entry->Version));
  DEBUG ((EFI_D_INFO, "Length            = 0x%x\n", Entry->Length));
  DEBUG ((EFI_D_INFO, "Class             = 0x%x\n", Entry->Class));
  DEBUG ((EFI_D_INFO, "Subclass          = 0x%x\n", Entry->Subclass));
  DEBUG ((EFI_D_INFO, "ProgIf            = 0x%x\n", Entry->ProgIf));
  DEBUG ((EFI_D_INFO, "DeviceType        = 0x%x\n", Entry->DeviceType));
  DEBUG ((EFI_D_INFO, "Vid               = 0x%x\n", Entry->Vid));
  DEBUG ((EFI_D_INFO, "Did               = 0x%x\n", Entry->Did));
  DEBUG ((EFI_D_INFO, "Status            = 0x%x\n", Entry->Status));
  DEBUG ((EFI_D_INFO, "Segment           = 0x%x\n", Entry->Segment));
  DEBUG ((EFI_D_INFO, "Bus               = 0x%x\n", Entry->Bus));
  DEBUG ((EFI_D_INFO, "Device            = 0x%x\n", Entry->Device));
  DEBUG ((EFI_D_INFO, "Function          = 0x%x\n", Entry->Function));
  DEBUG ((EFI_D_INFO, "PcieGen           = 0x%x\n", Entry->PcieGen));
  DEBUG ((EFI_D_INFO, "MaxPcieGen        = 0x%x\n", Entry->MaxPcieGen));
  DEBUG ((EFI_D_INFO, "LanesUsed         = 0x%x\n", Entry->LanesUsed));
  DEBUG ((EFI_D_INFO, "MaxLanes          = 0x%x\n", Entry->MaxLanes));
  DEBUG ((EFI_D_INFO, "SerialNumberUpper = 0x%x\n", Entry->SerialNumberUpper));
  DEBUG ((EFI_D_INFO, "SerialNumberLower = 0x%x\n", Entry->SerialNumberLower));
}

/**
  Collect PCIe device information and add it to the system topology report.

  @param[in] This   Pointer to the EFI_SYS_TOPOLOGY_PROTOCOL instance.
  @param[in] Report Pointer to the system topology report context.

  @retval EFI_SUCCESS           PCIe devices were collected successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
EFI_STATUS
EFIAPI
CollectPcieDevices (
  EFI_SYS_TOPOLOGY_PROTOCOL    *This,
  SYS_TOPOLOGY_REPORT_CONTEXT  *Report
  )
{
  EFI_STATUS                        Status;
  UINTN                             i;
  UINTN                             HandleCount;
  EFI_HANDLE                        *HandleBuffer;
  EFI_PCI_IO_PROTOCOL               *PciIo;
  SYS_TOPOLOGY_PCIE                 PciEntry;
  PCI_COMMON_HEADER                 PciConfig;
  UINTN                             Segment, Bus, Device, Function;
  UINT8                             CapPtr;
  UINT32                            CapReg;
  UINT16                            ExtCapPtr;
  PCI_EXT_CAPABILITY_SERIAL_NUMBER  SerialNumber;

  ZeroMem (&PciEntry, sizeof (SYS_TOPOLOGY_PCIE));
  ZeroMem (&PciConfig, sizeof (PCI_COMMON_HEADER));
  CapReg = 0;
  ZeroMem (&SerialNumber, sizeof (PCI_EXT_CAPABILITY_SERIAL_NUMBER));

  DEBUG ((EFI_D_INFO, "CollectPcieDevices: Collecting PCIe Devices\n"));

  // Get all handles that have PCI IO protocol on them
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );

  DEBUG ((EFI_D_INFO, "PciIo Protocol Handle Count = %d\n", HandleCount));

  if (!EFI_ERROR (Status)) {
    // Use each PCI IO protocol to gather information about PCIe devices
    for (i = 0; i < HandleCount; i++) {
      Status = gBS->HandleProtocol (
                      HandleBuffer[i],
                      &gEfiPciIoProtocolGuid,
                      (VOID **)&PciIo
                      );

      if (EFI_ERROR (Status)) {
        continue;
      }

      // Read PCI Config space
      Status = PciIo->Pci.Read (
                            PciIo,
                            EfiPciIoWidthUint8,
                            0,
                            sizeof (PCI_COMMON_HEADER),
                            &PciConfig
                            );

      if (!EFI_ERROR (Status)) {
        // Ignore Bridges, Base System Peripherals & Storage Devices
        if ((PciConfig.ClassCode[2] != 0x6) && (PciConfig.ClassCode[2] != 0x8) && (PciConfig.ClassCode[2] != 0x1)) {
          ZeroMem (&PciEntry, sizeof (SYS_TOPOLOGY_PCIE));

          DEBUG ((EFI_D_INFO, "Adding Vendor ID = 0x%04x - Device ID = 0x%04x\n", PciConfig.VendorId, PciConfig.DeviceId));
          PciEntry.Type       = 0;
          PciEntry.Version    = 2;
          PciEntry.Length     = sizeof (SYS_TOPOLOGY_PCIE);
          PciEntry.Class      = PciConfig.ClassCode[2];
          PciEntry.Subclass   = PciConfig.ClassCode[1];
          PciEntry.ProgIf     = PciConfig.ClassCode[0];
          PciEntry.Vid        = PciConfig.VendorId;
          PciEntry.Did        = PciConfig.DeviceId;
          PciEntry.Status     = PciConfig.Status;
          PciEntry.DeviceType = (PciConfig.HeaderType & BIT7) >> 7;

          CapPtr = FindPciCapabilityStructure (PciIo, PCIE_CAP_ID);
          if (CapPtr != 0) {
            // Read PCIe link Cap
            Status = PciIo->Pci.Read (
                                  PciIo,
                                  EfiPciIoWidthUint32,
                                  CapPtr + PCIE_LINK_CAP_OFFSET,
                                  1,
                                  &CapReg
                                  );
            if (!EFI_ERROR (Status)) {
              PciEntry.MaxPcieGen = (UINT8)(CapReg & 0xF);
              PciEntry.MaxLanes   = (UINT8)((CapReg & 0xF0) >> 4);
              DEBUG ((EFI_D_INFO, "MaxPcieGen = 0x%x - MaxLanes = 0x%x\n", PciEntry.MaxPcieGen, PciEntry.MaxLanes));
            }

            // Read PCIe link control & status
            Status = PciIo->Pci.Read (
                                  PciIo,
                                  EfiPciIoWidthUint32,
                                  CapPtr + PCIE_LINK_CTRL_OFFSET,
                                  1,
                                  &CapReg
                                  );
            if (!EFI_ERROR (Status)) {
              PciEntry.LanesUsed = (UINT8)((CapReg & 0xF00000) >> 20);
              PciEntry.PcieGen   = (UINT8)((CapReg & 0xF0000) >> 16);
              DEBUG ((EFI_D_INFO, "PcieGen = 0x%x - LanesUsed = 0x%x\n", PciEntry.PcieGen, PciEntry.LanesUsed));
            }
          }

          // Search extended capability space for serial number ID (0x3)
          ExtCapPtr = FindPciExtendedCapabilityStructure (PciIo, PCIE_EXT_CAP_SERIAL);
          if (ExtCapPtr != 0) {
            PciIo->Pci.Read (
                         PciIo,
                         EfiPciIoWidthUint8,
                         ExtCapPtr,
                         sizeof (PCI_EXT_CAPABILITY_SERIAL_NUMBER),
                         &SerialNumber
                         );

            DEBUG ((EFI_D_INFO, "Serial Number = %x%x\n", SerialNumber.Upper, SerialNumber.Lower));
            PciEntry.SerialNumberUpper = SerialNumber.Upper;
            PciEntry.SerialNumberLower = SerialNumber.Lower;
          }

          Status = PciIo->GetLocation (
                            PciIo,
                            &Segment,
                            &Bus,
                            &Device,
                            &Function
                            );
          if (!EFI_ERROR (Status)) {
            PciEntry.Segment  = (UINT8)(Segment & 0xff);
            PciEntry.Bus      = (UINT8)(Bus & 0xff);
            PciEntry.Device   = (UINT8)(Device & 0xff);
            PciEntry.Function = (UINT8)(Function & 0xff);
          }

          DumpPcieRecord (&PciEntry);
          InsertEntry (Report, (SYS_TOPOLOGY_ENTRY_HEADER *)&PciEntry);
        }
      }
    }
  }

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
    HandleBuffer = NULL;
  }

  return Status;
}

/**
  Get NVMe controller identify data.

  @param[in]  NvmePassThru    Pointer to the NVMe pass thru protocol.
  @param[out] ControllerData  Pointer to the NVME_ADMIN_CONTROLLER_DATA structure.

  @retval EFI_SUCCESS           Data retrieved successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
EFI_STATUS
GetNvmeIdentifyControllerData (
  IN  EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL  *NvmePassThru,
  OUT NVME_ADMIN_CONTROLLER_DATA          *ControllerData
  )
{
  EFI_STATUS                                Status;
  EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET  Packet;
  EFI_NVM_EXPRESS_COMMAND                   NvmeCmd;
  EFI_NVM_EXPRESS_COMPLETION                NvmeCompletion;

  ZeroMem (&Packet, sizeof (Packet));
  ZeroMem (&NvmeCmd, sizeof (NvmeCmd));
  ZeroMem (&NvmeCompletion, sizeof (NvmeCompletion));

  if ((NvmePassThru == NULL) || (ControllerData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  NvmeCmd.Cdw0.Opcode = IdentifyOpcode;
  NvmeCmd.Cdw10       = IdentifyControllerCns;
  NvmeCmd.Flags       = CDW10_VALID;

  Packet.NvmeCmd        = &NvmeCmd;
  Packet.NvmeCompletion = &NvmeCompletion;
  Packet.CommandTimeout = 5000000;
  Packet.TransferBuffer = ControllerData;
  Packet.TransferLength = sizeof (NVME_ADMIN_CONTROLLER_DATA);

  Status = NvmePassThru->PassThru (
                           NvmePassThru,
                           0,
                           &Packet,
                           NULL
                           );
  return Status;
}

/**
  Create a storage entry context.

  @param[in] Entry Pointer to the SYS_TOPOLOGY_STORAGE_CONTEXT structure.

  @retval EFI_SUCCESS           Entry created successfully.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
**/
EFI_STATUS
CreateStorageEntry (
  IN SYS_TOPOLOGY_STORAGE_CONTEXT  *Entry
  )
{
  EFI_STATUS            Status;
  SYS_TOPOLOGY_STORAGE  *StorageEntry;

  // Allocate default size report and initialize header
  Status = gBS->AllocatePool (EfiBootServicesData, DEFAULT_ENTRY_SIZE, (VOID **)&Entry->Storage);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_INFO, "Failed to allocate memory for Storage Entry - Status = %r\n", Status));
    return Status;
  }

  StorageEntry = Entry->Storage;

  ZeroMem (StorageEntry, DEFAULT_ENTRY_SIZE);
  StorageEntry->Type    = 1;
  StorageEntry->Version = 1;
  StorageEntry->Length  = sizeof (SYS_TOPOLOGY_STORAGE);

  Entry->MemoryAllocated = DEFAULT_ENTRY_SIZE;

  return EFI_SUCCESS;
}

/**
  Free memory allocated for an entry context.

  @param[in] Entry Pointer to the SYS_TOPOLOGY_ENTRY_CONTEXT structure.

  @retval EFI_SUCCESS           Memory freed successfully.
  @retval EFI_INVALID_PARAMETER The input parameter is invalid.
**/
EFI_STATUS
FreeEntry (
  IN SYS_TOPOLOGY_ENTRY_CONTEXT  *Entry
  )
{
  if ((Entry == NULL) || (Entry->Header == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Free memory allocated for Report
  FreePool (Entry->Header);
  Entry->Header = NULL;

  return EFI_SUCCESS;
}

/**
  Add a string to the entry context.

  @param[in] Entry   Pointer to the SYS_TOPOLOGY_ENTRY_CONTEXT structure.
  @param[in] Offset  Pointer to the offset field.
  @param[in] String  Pointer to the string to add.
  @param[in] Length  Length of the string.

  @retval EFI_SUCCESS           String added successfully.
  @retval EFI_INVALID_PARAMETER The input parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Not enough memory.
**/
EFI_STATUS
AddString (
  IN SYS_TOPOLOGY_ENTRY_CONTEXT  *Entry,
  IN UINT16                      *Offset,
  IN CHAR8                       *String,
  IN UINT16                      Length
  )
{
  SYS_TOPOLOGY_ENTRY_HEADER  *Header;

  if ((Entry == NULL) || (Offset == NULL) || (String == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Header = Entry->Header;

  if (Entry->MemoryAllocated < (UINT32)(Header->Length + Length + 1)) {
    return EFI_OUT_OF_RESOURCES;
  }

  *Offset = Header->Length;
  CopyMem ((UINT8 *)Header + (UINTN)*Offset, String, Length);
  Header->Length += Length + 1; // Empty space in entries are zeroed out. Force null terminator

  return EFI_SUCCESS;
}

/**
  Collect NVMe drive information and add it to the report.

  @param[in] Report Pointer to the system topology report context.

  @retval EFI_SUCCESS           NVMe drives collected successfully.
**/
EFI_STATUS
CollectNvmeDrives (
  SYS_TOPOLOGY_REPORT_CONTEXT  *Report
  )
{
  EFI_STATUS                          Status;
  UINTN                               i;
  UINTN                               HandleCount;
  EFI_HANDLE                          *HandleBuffer;
  EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL  *NvmePassThru;
  NVME_ADMIN_CONTROLLER_DATA          ControllerData;
  SYS_TOPOLOGY_STORAGE_CONTEXT        Entry;
  SYS_TOPOLOGY_STORAGE                *Storage;

  ZeroMem (&ControllerData, sizeof (NVME_ADMIN_CONTROLLER_DATA));

  DEBUG ((EFI_D_INFO, "CollectNvmeDrives: Collecting NVMe drives\n"));

  // Get all handles that have PCI IO protocol on them
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiNvmExpressPassThruProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );

  DEBUG ((EFI_D_INFO, "LocateHandleBuffer(gEfiNvmExpressPassThruProtocolGuid) Status = %r\n", Status));

  if (!EFI_ERROR (Status)) {
    DEBUG ((EFI_D_INFO, "%d handles with gEfiNvmExpressPassThruProtocolGuid installed\n", HandleCount));

    for (i = 0; i < HandleCount; i++) {
      Status = gBS->HandleProtocol (
                      HandleBuffer[i],
                      &gEfiNvmExpressPassThruProtocolGuid,
                      (VOID **)&NvmePassThru
                      );

      DEBUG ((EFI_D_INFO, "HandleProtocol(gEfiNvmExpressPassThruProtocolGuid) Status = %r\n", Status));
      if (EFI_ERROR (Status)) {
        continue;
      }

      Status = GetNvmeIdentifyControllerData (NvmePassThru, &ControllerData);
      DEBUG ((EFI_D_INFO, "GetNvmeIdentifyControllerData() Status = %r\n", Status));
      if (!EFI_ERROR (Status)) {
        CreateStorageEntry (&Entry);
        Storage = Entry.Storage;

        Storage->MediaType = NVME_STORAGE;

        Status = AddString (
                   (SYS_TOPOLOGY_ENTRY_CONTEXT *)&Entry,
                   &(Storage->ModelName),
                   (CHAR8 *)ControllerData.Mn,
                   40
                   );

        Status = AddString (
                   (SYS_TOPOLOGY_ENTRY_CONTEXT *)&Entry,
                   &(Storage->SerialNumber),
                   (CHAR8 *)ControllerData.Sn,
                   20
                   );

        Status = AddString (
                   (SYS_TOPOLOGY_ENTRY_CONTEXT *)&Entry,
                   &(Storage->FirmwareVersion),
                   (CHAR8 *)ControllerData.Fr,
                   8
                   );

        CopyMem ((UINT8 *)&Storage->CapacityBytes, ControllerData.Tnvmcap, 8);

        DEBUG ((EFI_D_INFO, "SerialNo = %a\n", (CHAR8 *)Storage + (UINTN)Storage->SerialNumber));
        DEBUG ((EFI_D_INFO, "FirmwareVer = %a\n", (CHAR8 *)Storage + (UINTN)Storage->FirmwareVersion));
        DEBUG ((EFI_D_INFO, "ModelName = %a\n", (CHAR8 *)Storage + (UINTN)Storage->ModelName));

        DEBUG ((EFI_D_INFO, "CapacityBytes = 0x%x%x\n", *((UINT32 *)&Storage->CapacityBytes+1), Storage->CapacityBytes));

        InsertEntry (Report, (SYS_TOPOLOGY_ENTRY_HEADER *)Storage);
        FreeEntry ((SYS_TOPOLOGY_ENTRY_CONTEXT *)&Entry);
      }
    }
  }

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
    HandleBuffer = NULL;
  }

  return EFI_SUCCESS;
}

/**
  Check if the DiskInfo protocol instance is for a SATA device.

  @param[in] DiskInfo Pointer to the EFI_DISK_INFO_PROTOCOL instance.

  @retval TRUE  If the device is SATA.
  @retval FALSE Otherwise.
**/
BOOLEAN
IsSataDiskInfo (
  EFI_DISK_INFO_PROTOCOL  *DiskInfo
  )
{
  DEBUG ((EFI_D_INFO, "DiskInfo->Interface = %g\n", &(DiskInfo->Interface)));

  if (0 == CompareMem (&(DiskInfo->Interface), &gEfiDiskInfoIdeInterfaceGuid, sizeof (EFI_GUID))) {
    DEBUG ((EFI_D_INFO, "SATA drive found\n"));
    return TRUE;
  }

  if (0 == CompareMem (&(DiskInfo->Interface), &gEfiDiskInfoAhciInterfaceGuid, sizeof (EFI_GUID))) {
    DEBUG ((EFI_D_INFO, "SATA drive found\n"));
    return TRUE;
  }

  DEBUG ((EFI_D_INFO, "SATA drive not found\n"));

  return FALSE;
}

/**
  Fix SATA string format by swapping adjacent characters.

  @param[in,out] SataStr Pointer to the SATA string.
  @param[in]     StrLen  Length of the string.
**/
VOID
FixSataStr (
  CHAR8  *SataStr,
  UINT8  StrLen
  )
{
  UINT8  i;
  CHAR8  TmpChar;

  if ((StrLen % 2) != 0) {
    return;
  }

  for (i = 0; i < StrLen; i += 2) {
    TmpChar      = SataStr[i];
    SataStr[i]   = SataStr[i+1];
    SataStr[i+1] = TmpChar;
  }
}

/**
  Collect SATA drive information and add it to the report.

  @param[in] Report Pointer to the system topology report context.

  @retval EFI_SUCCESS           SATA drives collected successfully.
**/
EFI_STATUS
CollectSataDrives (
  SYS_TOPOLOGY_REPORT_CONTEXT  *Report
  )
{
  EFI_STATUS                    Status;
  UINTN                         HandleCount;
  EFI_HANDLE                    *HandleBuffer;
  EFI_DISK_INFO_PROTOCOL        *DiskInfo;
  EFI_BLOCK_IO_PROTOCOL         *BlockIo;
  ATA_IDENTIFY_DATA             IdentifyData;
  UINT32                        IdentifyDataSize;
  UINT8                         i;
  SYS_TOPOLOGY_STORAGE_CONTEXT  Entry;
  SYS_TOPOLOGY_STORAGE          *Storage;

  ZeroMem (&IdentifyData, sizeof (ATA_IDENTIFY_DATA));

  DEBUG ((EFI_D_INFO, "CollectSataDrives: Collecting SATA drives\n"));

  // Get all handles that have DiskInfo protocol on them
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiDiskInfoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );

  DEBUG ((EFI_D_INFO, "LocateHandleBuffer(gEfiDiskInfoProtocolGuid) Status = %r\n", Status));
  if (!EFI_ERROR (Status)) {
    for (i = 0; i < HandleCount; i++) {
      Status = gBS->HandleProtocol (
                      HandleBuffer[i],
                      &gEfiDiskInfoProtocolGuid,
                      (VOID **)&DiskInfo
                      );
      DEBUG ((EFI_D_INFO, "HandleProtocol(gEfiDiskInfoProtocolGuid) = %r\n", Status));
      if (EFI_ERROR (Status) || !IsSataDiskInfo (DiskInfo)) {
        continue;
      }

      IdentifyDataSize = sizeof (ATA_IDENTIFY_DATA);
      Status           = DiskInfo->Identify (DiskInfo, &IdentifyData, &IdentifyDataSize);
      DEBUG ((EFI_D_INFO, "DiskInfo->Identify() = %r\n", Status));
      if (EFI_ERROR (Status)) {
        continue;
      }

      CreateStorageEntry (&Entry);
      Storage = Entry.Storage;

      Storage->MediaType = SATA_STORAGE;

      FixSataStr (IdentifyData.ModelName, 40);
      FixSataStr (IdentifyData.SerialNo, 20);
      FixSataStr (IdentifyData.FirmwareVer, 8);

      Status = AddString (
                 (SYS_TOPOLOGY_ENTRY_CONTEXT *)&Entry,
                 &(Storage->ModelName),
                 IdentifyData.ModelName,
                 40
                 );

      Status = AddString (
                 (SYS_TOPOLOGY_ENTRY_CONTEXT *)&Entry,
                 &(Storage->SerialNumber),
                 IdentifyData.SerialNo,
                 20
                 );

      Status = AddString (
                 (SYS_TOPOLOGY_ENTRY_CONTEXT *)&Entry,
                 &(Storage->FirmwareVersion),
                 IdentifyData.FirmwareVer,
                 8
                 );

      DEBUG ((EFI_D_INFO, "SerialNo = %a\n", (CHAR8 *)Storage + (UINTN)Storage->SerialNumber));
      DEBUG ((EFI_D_INFO, "FirmwareVer = %a\n", (CHAR8 *)Storage + (UINTN)Storage->FirmwareVersion));
      DEBUG ((EFI_D_INFO, "ModelName = %a\n", (CHAR8 *)Storage + (UINTN)Storage->ModelName));

      Status = gBS->HandleProtocol (
                      HandleBuffer[i],
                      &gEfiBlockIoProtocolGuid,
                      (VOID **)&BlockIo
                      );

      if (!EFI_ERROR (Status)) {
        Storage->BlockSizeBytes = BlockIo->Media->BlockSize;
        Storage->CapacityBytes  = BlockIo->Media->BlockSize * (BlockIo->Media->LastBlock + 1);
        DEBUG ((EFI_D_INFO, "CapacityBytes = 0x%x%x\nBlockSizeBytes = 0x%x\n", *((UINT32 *)&Storage->CapacityBytes+1), Storage->CapacityBytes, Storage->BlockSizeBytes));
      }

      InsertEntry (Report, (SYS_TOPOLOGY_ENTRY_HEADER *)Storage);
      FreeEntry ((SYS_TOPOLOGY_ENTRY_CONTEXT *)&Entry);
    }
  }

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
    HandleBuffer = NULL;
  }

  return EFI_SUCCESS;
}

/**
  Replace spaces in the string with underscores.
  @param[in]  SourceString  Original string.
  @param[out] DestString    Target string buffer.
  @param[in]  DestSize      Size of the target buffer.
**/
VOID
ReplaceSpacesWithUnderscores (
  IN  CHAR8  *SourceString,
  OUT CHAR8  *DestString,
  IN  UINTN  DestSize
  )
{
  UINTN  Index;

  if ((SourceString == NULL) || (DestString == NULL) || (DestSize == 0)) {
    return;
  }

  for (Index = 0; (Index < DestSize - 1) && (SourceString[Index] != 0); Index++) {
    if (SourceString[Index] == ' ') {
      DestString[Index] = '_';
    } else {
      DestString[Index] = SourceString[Index];
    }
  }

  DestString[Index] = 0;
}

/**
  Retrieve a string from the SMBIOS table and handle default values.
  @param[in] StringTable    Pointer to the SMBIOS string table.
  @param[in] StringNumber   The string number to retrieve.
  @param[in] DefaultString  The default value to use when the string does not exist or is empty.
  @return                   Pointer to the string.
**/
CHAR8 *
GetSmbiosStringWithDefault (
  IN CHAR8  *StringTable,
  IN UINT8  StringNumber,
  IN CHAR8  *DefaultString
  )
{
  UINT8  Index;
  CHAR8  *StringPtr;

  if ((StringTable == NULL) || (StringNumber == 0)) {
    return DefaultString;
  }

  StringPtr = StringTable;
  for (Index = 1; Index < StringNumber; Index++) {
    if (*StringPtr == 0) {
      return DefaultString;
    }

    StringPtr += AsciiStrLen (StringPtr) + 1;
  }

  if (*StringPtr == 0) {
    return DefaultString;
  }

  return StringPtr;
}

/**
  Collect memory device information from the SMBIOS table and add it to the report.
  @param[in] Report  Pointer to the topology report context.
  @retval EFI_SUCCESS           Memory devices were collected successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
EFI_STATUS
CollectSmbiosMemoryDevices (
  SYS_TOPOLOGY_REPORT_CONTEXT  *Report
  )
{
  EFI_STATUS               Status;
  EFI_SMBIOS_PROTOCOL      *SmbiosProtocol;
  EFI_SMBIOS_HANDLE        SmbiosHandle;
  EFI_SMBIOS_TABLE_HEADER  *Record;
  SMBIOS_TABLE_TYPE17      *MemoryDevice;
  SMBIOS_TABLE_TYPE16      *PhysicalMemoryArray;
  EFI_SMBIOS_HANDLE        Type16Handle;
  CHAR8                    *StringPtr;
  CHAR8                    *SourceStr;
  SYS_TOPOLOGY_MEMORY      *Memory;
  CHAR8                    UnknownLocator[16];
  CHAR8                    UnknownBank[13];
  CHAR8                    UnknownMfr[12];
  CHAR8                    UnknownSn[11];
  CHAR8                    UnknownPn[11];
  CHAR8                    UnknownFw[11];
  CHAR8                    UnknownRcd[12];
  CHAR8                    UnknownPmic[14];

  PhysicalMemoryArray = NULL;
  Type16Handle        = SMBIOS_HANDLE_PI_RESERVED;
  AsciiStrCpyS (UnknownLocator, sizeof (UnknownLocator), "Unknown_Locator");
  AsciiStrCpyS (UnknownBank, sizeof (UnknownBank), "Unknown_Bank");
  AsciiStrCpyS (UnknownMfr, sizeof (UnknownMfr), "Unknown_Mfr");
  AsciiStrCpyS (UnknownSn, sizeof (UnknownSn), "Unknown_SN");
  AsciiStrCpyS (UnknownPn, sizeof (UnknownPn), "Unknown_PN");
  AsciiStrCpyS (UnknownFw, sizeof (UnknownFw), "Unknown_FW");
  AsciiStrCpyS (UnknownRcd, sizeof (UnknownRcd), "Unknown_RCD");
  AsciiStrCpyS (UnknownPmic, sizeof (UnknownPmic), "Unknown_PMIC0");

  DEBUG ((DEBUG_INFO, "CollectSmbiosMemoryDevices Start\n"));
  Status = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid, NULL, (VOID **)&SmbiosProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate SMBIOS protocol - %r\n", Status));
    return Status;
  }

  while (TRUE) {
    Status = SmbiosProtocol->GetNext (SmbiosProtocol, &Type16Handle, NULL, &Record, NULL);
    if (EFI_ERROR (Status)) {
      break;
    }

    if (Record->Type == EFI_SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY) {
      PhysicalMemoryArray = (SMBIOS_TABLE_TYPE16 *)Record;
      DEBUG (
        (DEBUG_INFO, "Found SMBIOS Type 16 - Memory Array with Error Correction type: %d\n",
         PhysicalMemoryArray->MemoryErrorCorrection)
        );
      break;
    }
  }

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  while (TRUE) {
    Status = SmbiosProtocol->GetNext (SmbiosProtocol, &SmbiosHandle, NULL, &Record, NULL);
    if (EFI_ERROR (Status)) {
      break;
    }

    if (Record->Type != EFI_SMBIOS_TYPE_MEMORY_DEVICE) {
      continue;
    }

    MemoryDevice = (SMBIOS_TABLE_TYPE17 *)Record;
    if (MemoryDevice->Size == 0) {
      continue;
    }

    Status = gBS->AllocatePool (
                    EfiBootServicesData,
                    sizeof (SYS_TOPOLOGY_MEMORY),
                    (VOID **)&Memory
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to allocate memory for Memory Entry - %r\n", Status));
      continue;
    }

    ZeroMem (Memory, sizeof (SYS_TOPOLOGY_MEMORY));
    Memory->Header.Type    = 2; // Memory Device Type
    Memory->Header.Version = 1;
    Memory->Header.Length  = sizeof (SYS_TOPOLOGY_MEMORY);
    StringPtr              = (CHAR8 *)MemoryDevice + MemoryDevice->Hdr.Length;
    Memory->Size           = MemoryDevice->ExtendedSize;
    Memory->Speed          = MemoryDevice->Speed;
    SourceStr              = GetSmbiosStringWithDefault (StringPtr, MemoryDevice->DeviceLocator, UnknownLocator);
    CHAR8  *FormattedDeviceLocator = AllocatePool (AsciiStrSize (SourceStr));
    if (FormattedDeviceLocator != NULL) {
      ReplaceSpacesWithUnderscores (SourceStr, FormattedDeviceLocator, AsciiStrSize (SourceStr));
      Memory->DeviceLocator = AllocateCopyPool (AsciiStrSize (FormattedDeviceLocator), FormattedDeviceLocator);
      FreePool (FormattedDeviceLocator);
    } else {
      Memory->DeviceLocator = AllocateCopyPool (AsciiStrSize (SourceStr), SourceStr);
    }

    SourceStr = GetSmbiosStringWithDefault (StringPtr, MemoryDevice->BankLocator, UnknownBank);
    CHAR8  *FormattedBankLocator = AllocatePool (AsciiStrSize (SourceStr));
    if (FormattedBankLocator != NULL) {
      ReplaceSpacesWithUnderscores (SourceStr, FormattedBankLocator, AsciiStrSize (SourceStr));
      Memory->BankLocator = AllocateCopyPool (AsciiStrSize (FormattedBankLocator), FormattedBankLocator);
      FreePool (FormattedBankLocator);
    } else {
      Memory->BankLocator = AllocateCopyPool (AsciiStrSize (SourceStr), SourceStr);
    }

    SourceStr = GetSmbiosStringWithDefault (StringPtr, MemoryDevice->Manufacturer, UnknownMfr);
    CHAR8  *FormattedManufacturer = AllocatePool (AsciiStrSize (SourceStr));
    if (FormattedManufacturer != NULL) {
      ReplaceSpacesWithUnderscores (SourceStr, FormattedManufacturer, AsciiStrSize (SourceStr));
      Memory->Manufacturer = AllocateCopyPool (AsciiStrSize (FormattedManufacturer), FormattedManufacturer);
      FreePool (FormattedManufacturer);
    } else {
      Memory->Manufacturer = AllocateCopyPool (AsciiStrSize (SourceStr), SourceStr);
    }

    // SerialNumber
    SourceStr            = GetSmbiosStringWithDefault (StringPtr, MemoryDevice->SerialNumber, UnknownSn);
    Memory->SerialNumber = AllocateCopyPool (AsciiStrSize (SourceStr), SourceStr);
    // PartNumber
    SourceStr          = GetSmbiosStringWithDefault (StringPtr, MemoryDevice->PartNumber, UnknownPn);
    Memory->PartNumber = AllocateCopyPool (AsciiStrSize (SourceStr), SourceStr);
    // BaseModuleType
    Memory->BaseModuleType = AllocateCopyPool (AsciiStrSize ("RDIMM"), "RDIMM");
    // MemoryType
    Memory->MemoryType = AllocateCopyPool (AsciiStrSize ("DRAM"), "DRAM");
    // MemoryDeviceType
    Memory->MemoryDeviceType = AllocateCopyPool (AsciiStrSize ("DDR5"), "DDR5");
    // FirmwareVersion
    SourceStr               = GetSmbiosStringWithDefault (StringPtr, MemoryDevice->FirmwareVersion, UnknownFw);
    Memory->FirmwareVersion = AllocateCopyPool (AsciiStrSize (SourceStr), SourceStr);
    // DataWidth
    Memory->DataWidth = MemoryDevice->DataWidth;
    // BusWidth
    Memory->BusWidth = MemoryDevice->TotalWidth;
    // RankCount
    Memory->RankCount = MemoryDevice->Attributes;
    // ErrorCorrectionType
    if (PhysicalMemoryArray != NULL) {
      if (PhysicalMemoryArray->MemoryErrorCorrection < ARRAY_SIZE (ErrorCorrectionTypes)) {
        Memory->ErrorCorrectionType = AllocateCopyPool (
                                        AsciiStrSize (ErrorCorrectionTypes[PhysicalMemoryArray->MemoryErrorCorrection]),
                                        ErrorCorrectionTypes[PhysicalMemoryArray->MemoryErrorCorrection]
                                        );
      } else {
        Memory->ErrorCorrectionType = AllocateCopyPool (AsciiStrSize ("Unknown"), "Unknown");
      }
    } else {
      Memory->ErrorCorrectionType = AllocateCopyPool (AsciiStrSize ("Unknown"), "Unknown");
    }

    // PMIC0 ManufacturerID
    SourceStr                   = (MemoryDevice->Pmic0ManufacturerID <= 0xFF) ? GetSmbiosStringWithDefault (StringPtr, (UINT8)MemoryDevice->Pmic0ManufacturerID, UnknownPmic) : UnknownPmic;
    Memory->Pmic0ManufacturerId = AllocateCopyPool (AsciiStrSize (SourceStr), SourceStr);

    // RCD ManufacturerID
    SourceStr                 = (MemoryDevice->RcdManufacturerID <= 255) ? GetSmbiosStringWithDefault (StringPtr, (UINT8)MemoryDevice->RcdManufacturerID, UnknownRcd) : UnknownRcd;
    Memory->RcdManufacturerId = AllocateCopyPool (AsciiStrSize (SourceStr), SourceStr);

    /*
    DEBUG ((DEBUG_INFO, "DebugInfo CollectSmbiosMemoryDevices START\n"));
    DEBUG ((DEBUG_INFO, "DebugInfo Memory Device: Size=%u KB, Speed=%u MHz\n", Memory->Size, Memory->Speed));
    DEBUG ((DEBUG_INFO, "DeviceLocator: %a\n", Memory->DeviceLocator));
    DEBUG ((DEBUG_INFO, "BankLocator: %a\n", Memory->BankLocator));
    DEBUG ((DEBUG_INFO, "Manufacturer: %a\n", Memory->Manufacturer));
    DEBUG ((DEBUG_INFO, "SerialNumber: %a\n", Memory->SerialNumber));
    DEBUG ((DEBUG_INFO, "PartNumber: %a\n", Memory->PartNumber));
    DEBUG ((DEBUG_INFO, "BaseModuleType: %a\n", Memory->BaseModuleType));
    DEBUG ((DEBUG_INFO, "MemoryType: %a\n", Memory->MemoryType));
    DEBUG ((DEBUG_INFO, "FirmwareVersion: %a\n", Memory->FirmwareVersion));
    DEBUG ((DEBUG_INFO, "FirmwareVersion type17: %a\n", MemoryDevice->FirmwareVersion));
    DEBUG ((DEBUG_INFO, "DataWidth: %a\n", Memory->DataWidth));
    DEBUG ((DEBUG_INFO, "RankCount=%u \n", Memory->RankCount));
    DEBUG ((DEBUG_INFO, "ErrorCorrectionType: %a\n", Memory->ErrorCorrectionType));
    DEBUG ((DEBUG_INFO, "PMIC0 ManufacturerID: %a\n", Memory->Pmic0ManufacturerId));
    DEBUG ((DEBUG_INFO, "RCD ManufacturerID: %a\n", Memory->RcdManufacturerId));
    DEBUG ((DEBUG_INFO, "DebugInfo CollectSmbiosMemoryDevices END\n"));
    */

    InsertEntry (Report, (SYS_TOPOLOGY_ENTRY_HEADER *)Memory);
    FreePool (Memory);
  }

  return EFI_SUCCESS;
}

/**
  Collect BIOS version information from SMBIOS and add it to the system topology report.

  @param[in]  Report  Pointer to the system topology report context.

  @retval EFI_SUCCESS           The BIOS version was collected successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_NOT_FOUND         BIOS information was not found.
**/
EFI_STATUS
CollectSmbiosBiosVersion (
  SYS_TOPOLOGY_REPORT_CONTEXT  *Report
  )
{
  EFI_STATUS               Status;
  EFI_SMBIOS_PROTOCOL      *SmbiosProtocol;
  EFI_SMBIOS_HANDLE        SmbiosHandle;
  EFI_SMBIOS_TABLE_HEADER  *Record;
  SMBIOS_TABLE_TYPE0       *BiosInfo;
  SYS_TOPOLOGY_BIOS        *BiosEntry;
  CHAR8                    UnknownBIOSVersion[20];

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  BiosInfo     = NULL;
  AsciiStrCpyS (UnknownBIOSVersion, sizeof (UnknownBIOSVersion), "Unknown_BIOSVersion");

  Status = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid, NULL, (VOID **)&SmbiosProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "CollectSmbiosBiosVersion: Failed to locate SMBIOS protocol - %r\n", Status));
    return Status;
  }

  //
  // Loop through the SMBIOS records until a Type 0 (BIOS Information) record is found.
  //
  while (TRUE) {
    Status = SmbiosProtocol->GetNext (SmbiosProtocol, &SmbiosHandle, NULL, &Record, NULL);
    if (EFI_ERROR (Status)) {
      break;
    }

    if (Record->Type == EFI_SMBIOS_TYPE_BIOS_INFORMATION) {
      BiosInfo = (SMBIOS_TABLE_TYPE0 *)Record;
      break;
    }
  }

  if (BiosInfo == NULL) {
    DEBUG ((DEBUG_ERROR, "CollectSmbiosBiosVersion: BIOS Information record not found\n"));
    return EFI_NOT_FOUND;
  }

  Status = gBS->AllocatePool (EfiBootServicesData, sizeof (SYS_TOPOLOGY_BIOS), (VOID **)&BiosEntry);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "CollectSmbiosBiosVersion: Failed to allocate memory for BIOS entry - %r\n", Status));
    return Status;
  }

  ZeroMem (BiosEntry, sizeof (SYS_TOPOLOGY_BIOS));
  BiosEntry->Header.Type    = 3;
  BiosEntry->Header.Version = 1;
  BiosEntry->Header.Length  = sizeof (SYS_TOPOLOGY_BIOS);
  CHAR8  *BIOSStringTable = (CHAR8 *)BiosInfo + BiosInfo->Hdr.Length;

  CHAR8  *BiosVersionStr = GetSmbiosStringWithDefault (BIOSStringTable, BiosInfo->BiosVersion, UnknownBIOSVersion);

  // DEBUG ((DEBUG_INFO, "BiosInfo->BiosVersion = %a\n", BiosVersionStr));

  BiosEntry->BiosVersion = AllocateCopyPool (AsciiStrSize (BiosVersionStr), BiosVersionStr);
  // DEBUG ((DEBUG_INFO, "BiosEntry->BiosVersion = %a\n", BiosEntry->BiosVersion));

  InsertEntry (Report, (SYS_TOPOLOGY_ENTRY_HEADER *)BiosEntry);

  //
  // Cleanup the allocated BIOS entry (its data has been copied to the report).
  //
  FreePool (BiosEntry);

  return EFI_SUCCESS;
}

/**
  Collect storage device information (NVMe and SATA) and add it to the system topology report.

  @param[in] This   Pointer to the EFI_SYS_TOPOLOGY_PROTOCOL instance.
  @param[in] Report Pointer to the system topology report context.

  @retval EFI_SUCCESS Storage devices were collected successfully.
**/
EFI_STATUS
EFIAPI
CollectStorageDevices (
  EFI_SYS_TOPOLOGY_PROTOCOL    *This,
  SYS_TOPOLOGY_REPORT_CONTEXT  *Report
  )
{
  CollectNvmeDrives (Report);
  CollectSataDrives (Report);
  return EFI_SUCCESS;
}

/**
  Collect CXL device information and add it to the system topology report.

  @param[in] This   Pointer to the EFI_SYS_TOPOLOGY_PROTOCOL instance.
  @param[in] Report Pointer to the system topology report context.

  @retval EFI_SUCCESS CXL devices were collected successfully.
**/
EFI_STATUS
EFIAPI
CollectCxlDevices (
  EFI_SYS_TOPOLOGY_PROTOCOL    *This,
  SYS_TOPOLOGY_REPORT_CONTEXT  *Report
  )
{
  return EFI_SUCCESS;
}

/**
  Collect memory device information and add it to the system topology report.

  @param[in] This   Pointer to the EFI_SYS_TOPOLOGY_PROTOCOL instance.
  @param[in] Report Pointer to the system topology report context.

  @retval EFI_SUCCESS Memory devices were collected successfully.
**/
EFI_STATUS
EFIAPI
CollectMemoryDevices (
  EFI_SYS_TOPOLOGY_PROTOCOL    *This,
  SYS_TOPOLOGY_REPORT_CONTEXT  *Report
  )
{
  CollectSmbiosMemoryDevices (Report);
  return EFI_SUCCESS;
}

/**
  Collect BIOS version information from SMBIOS and add it to the system topology report.

  @param[in]  This    Pointer to the EFI_SYS_TOPOLOGY_PROTOCOL instance.
  @param[in]  Report  Pointer to the system topology report context.

  @retval EFI_SUCCESS           The BIOS version was collected successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_NOT_FOUND         BIOS information was not found.
**/
EFI_STATUS
EFIAPI
CollectBiosVersion (
  EFI_SYS_TOPOLOGY_PROTOCOL    *This,
  SYS_TOPOLOGY_REPORT_CONTEXT  *Report
  )
{
  CollectSmbiosBiosVersion (Report);
  return EFI_SUCCESS;
}

/**
  Initializes the System Topology Report protocol and installs it on the platform.

  @param[in]  ImageHandle  EFI Image Handle for the DXE driver.
  @param[in]  SystemTable  Pointer to the EFI system table.

  @retval EFI_SUCCESS Module initialized successfully.
**/
EFI_STATUS
EFIAPI
SysTopologyReportInit (
  IN       EFI_HANDLE        ImageHandle,
  IN       EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  SysTopologyProtocol.CreateReport          = CreateReport;
  SysTopologyProtocol.FreeReport            = FreeReport;
  SysTopologyProtocol.CollectPcieDevices    = CollectPcieDevices;
  SysTopologyProtocol.CollectStorageDevices = CollectStorageDevices;
  SysTopologyProtocol.CollectCxlDevices     = CollectCxlDevices;
  SysTopologyProtocol.CollectMemoryDevices  = CollectMemoryDevices;
  SysTopologyProtocol.CollectBiosVersion    = CollectBiosVersion;

  DEBUG ((EFI_D_INFO, "Installing SysTopologyProtocol\n"));

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiSysTopologyProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &SysTopologyProtocol
                  );

  DEBUG ((EFI_D_INFO, "SysTopologyProtocol Install Status = %r\n", Status));

  return Status;
}
