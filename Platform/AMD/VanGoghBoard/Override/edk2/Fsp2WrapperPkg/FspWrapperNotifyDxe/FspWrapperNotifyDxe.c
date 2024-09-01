/** @file
  This driver will register two callbacks to call fsp's notifies.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2014 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Protocol/PciEnumerationComplete.h>

#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>
#include <Library/FspWrapperApiLib.h>
#include <Library/FspWrapperPlatformLib.h>
#include <Library/PerformanceLib.h>
#include <Library/HobLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/Timer.h>
#include <Protocol/PciIo.h>
#include <FspsUpd.h>
#include <FspMemoryRegionHob.h>
#include <FspExportedInterfaceHob.h>
#include <FspStatusCode.h>
#include <Library/MemoryAllocationLib.h>

#define   FSP_API_NOTIFY_PHASE_AFTER_PCI_ENUMERATION  BIT16
extern EFI_GUID  gFspsUpdDataPointerAddressGuid;
extern EFI_GUID  gFspReservedMemoryResourceHobGuid;
extern EFI_GUID  gEfiEventExitBootServicesGuid;
extern EFI_GUID  gEfiEventVirtualAddressChangeGuid;
extern EFI_GUID  gEfiPciIoProtocolGuid;

FSPS_UPD *volatile                   FspsUpd;
FSPS_UPD *volatile                   FspsUpdInRt;
volatile FSP_EXPORTED_INTERFACE_HOB  *ExportedInterfaceHob;
typedef
EFI_STATUS
(EFIAPI *ADD_PERFORMANCE_RECORDS)(
  IN CONST VOID *HobStart
  );

struct _ADD_PERFORMANCE_RECORD_PROTOCOL {
  ADD_PERFORMANCE_RECORDS    AddPerformanceRecords;
};

typedef struct _ADD_PERFORMANCE_RECORD_PROTOCOL ADD_PERFORMANCE_RECORD_PROTOCOL;

extern EFI_GUID  gAddPerfRecordProtocolGuid;
extern EFI_GUID  gFspHobGuid;
extern EFI_GUID  gFspApiPerformanceGuid;

static EFI_EVENT  mExitBootServicesEvent     = NULL;
static EFI_EVENT  mVirtualAddressChangeEvent = NULL;

/**
  Relocate this image under 4G memory.

  @param  ImageHandle  Handle of driver image.
  @param  SystemTable  Pointer to system table.

  @retval EFI_SUCCESS  Image successfully relocated.
  @retval EFI_ABORTED  Failed to relocate image.

**/
EFI_STATUS
RelocateImageUnder4GIfNeeded (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

/**
 * For some reason, the FSP MAY enable the interrupt after processing SMM,
 * which is not ideal because this MAY cause timer interrupt being fired during FSP.
 *
 * A workaround is to disable timer shortly, and re-enable it after FSP call.
**/

STATIC EFI_TIMER_ARCH_PROTOCOL  *gTimer        = NULL;
STATIC UINT64                   mTimerInterval = 0;

VOID
EFIAPI
DisableTimer (
  VOID
  )
{
  EFI_STATUS  Status = gTimer->GetTimerPeriod (gTimer, &mTimerInterval);

  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "FSP TimerWorkaround begin: Timer interval val %llx\n", mTimerInterval));
  }

  Status = gTimer->SetTimerPeriod (gTimer, 0);
  ASSERT_EFI_ERROR (Status);
}

VOID
EFIAPI
EnableTimer (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "FSP TimerWorkaround end: Timer interval val %llx\n", mTimerInterval));
  EFI_STATUS  Status = EFI_SUCCESS;

  if (mTimerInterval != 0) {
    Status = gTimer->SetTimerPeriod (gTimer, mTimerInterval);
  }

  ASSERT_EFI_ERROR (Status);
}

/**
  PciEnumerationComplete Protocol notification event handler.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.
**/
VOID
EFIAPI
OnPciEnumerationComplete (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  NOTIFY_PHASE_PARAMS  NotifyPhaseParams;
  EFI_STATUS           Status;
  VOID                 *Interface;

  //
  // Try to locate it because gEfiPciEnumerationCompleteProtocolGuid will trigger it once when registration.
  // Just return if it is not found.
  //
  Status = gBS->LocateProtocol (
                  &gEfiPciEnumerationCompleteProtocolGuid,
                  NULL,
                  &Interface
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  NotifyPhaseParams.Phase = EnumInitPhaseAfterPciEnumeration;
  EFI_HANDLE  *Handles      = NULL;
  VOID        *Protocol     = NULL;
  UINTN       ProtocolCount = 0;

  gBS->LocateHandleBuffer (ByProtocol, &gEfiPciIoProtocolGuid, NULL, &ProtocolCount, &Handles);
  EFI_PCI_IO_PROTOCOL  **Protocols = AllocateZeroPool (sizeof (VOID *)*ProtocolCount);

  for (UINT64 i = 0; i < ProtocolCount; i++) {
    DEBUG ((DEBUG_INFO, "FSP-S Wrapper: Getting PCI Protocol %d/%d\n", i, ProtocolCount));
    gBS->HandleProtocol (Handles[i], &gEfiPciIoProtocolGuid, &Protocol);
    Protocols[i] = Protocol;
  }

  DEBUG ((DEBUG_ERROR, " ExportedInterfaceHob:%011p\n", ExportedInterfaceHob));
  // gBS->LocateProtocol(&gEfiPciIoProtocolGuid,NULL,&Protocol);
  ExportedInterfaceHob->EfiPciIoProtocol      = Protocols;
  ExportedInterfaceHob->EfiPciIoProtocolCount = ProtocolCount;
  PERF_START_EX (&gFspApiPerformanceGuid, "EventRec", NULL, 0, FSP_STATUS_CODE_POST_PCIE_ENUM_NOTIFICATION | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_ENTRY);
  DisableTimer ();
  Status = CallFspNotifyPhase (&NotifyPhaseParams);
  EnableTimer ();
  PERF_END_EX (&gFspApiPerformanceGuid, "EventRec", NULL, 0, FSP_STATUS_CODE_POST_PCIE_ENUM_NOTIFICATION | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);

  //
  // Reset the system if FSP API returned FSP_STATUS_RESET_REQUIRED status
  //
  if ((Status >= FSP_STATUS_RESET_REQUIRED_COLD) && (Status <= FSP_STATUS_RESET_REQUIRED_8)) {
    DEBUG ((DEBUG_INFO, "FSP NotifyPhase AfterPciEnumeration requested reset 0x%x\n", Status));
    CallFspWrapperResetSystem (Status);
  }

  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "FSP NotifyPhase AfterPciEnumeration failed, status: 0x%x\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "FSP NotifyPhase AfterPciEnumeration Success.\n"));
  }
}

STATIC
VOID
ReportFspMemoryUsage (
  VOID
  )
{
  FSP_MEMORY_REGION_HOB  *MemoryRegionHob = NULL;
  EFI_STATUS             Status;

  DEBUG ((DEBUG_ERROR, " ExportedInterfaceHob:%011p\n", ExportedInterfaceHob));
  DEBUG ((
    DEBUG_INFO,
    "FSP Memory Map Size:%llx,Memory Descriptor Size:%llx:\n",
    ExportedInterfaceHob->FinalMemoryMapSize,
    ExportedInterfaceHob->FinalMemoryDescriptorSize
    ));
  DEBUG ((DEBUG_INFO, "FSP Memory usage:\n"));
  UINTN  MemoryDescriptorEntries = ExportedInterfaceHob->FinalMemoryMapSize / \
                                   ExportedInterfaceHob->FinalMemoryDescriptorSize;
  EFI_MEMORY_DESCRIPTOR  *FspMemoryDescriptor = ExportedInterfaceHob->FinalMemoryMap;
  // Now we find the FSP memory HOB, "Free" it, and "Allocate" the memory as its layout in FSP.
  VOID  *FspHob = GetFirstGuidHob (&gFspReservedMemoryResourceHobGuid);

  if (FspHob != NULL) {
    MemoryRegionHob = GET_GUID_HOB_DATA (FspHob);
  }

  if (!MemoryRegionHob) {
    DEBUG ((DEBUG_ERROR, "Cannot find FSP HOB!\n"));
    ASSERT ((FALSE));
    return;
  }

  DEBUG ((
    DEBUG_INFO,
    "FSP memory region:0x%08p~0x%08p\n",
    MemoryRegionHob->BeginAddress, \
    MemoryRegionHob->BeginAddress+MemoryRegionHob->Length
    ));
  // Free previously reserved explicitly for EDK memory recycle.
  EFI_PHYSICAL_ADDRESS  ReservedMemoryAddress = MemoryRegionHob->BeginAddress+MemoryRegionHob->Length-(10<<EFI_PAGE_SHIFT);

  // Atomic code begins here
  gBS->RaiseTPL (TPL_NOTIFY);
  DEBUG ((DEBUG_INFO, "Address    Pages      Type\n"));
  // Reverse iteration due to EDK's memory allocation method.
  FspMemoryDescriptor = (EFI_MEMORY_DESCRIPTOR *)((UINTN)FspMemoryDescriptor+ExportedInterfaceHob->FinalMemoryDescriptorSize*(MemoryDescriptorEntries-1));
  for (UINTN i = 0; i < MemoryDescriptorEntries; i++) {
    DEBUG ((
      DEBUG_INFO,
      "0x%08p 0x%08p %x\n",
      FspMemoryDescriptor->PhysicalStart, \
      FspMemoryDescriptor->NumberOfPages,
      FspMemoryDescriptor->Type
      ));
    if (FspMemoryDescriptor->PhysicalStart == ReservedMemoryAddress) {
      gBS->FreePages (ReservedMemoryAddress, FspMemoryDescriptor->NumberOfPages);
      FspMemoryDescriptor = (EFI_MEMORY_DESCRIPTOR *)((UINTN)FspMemoryDescriptor-ExportedInterfaceHob->FinalMemoryDescriptorSize);
      continue;
    }

    if (FspMemoryDescriptor->Type == EfiMemoryMappedIO) {
      EFI_GCD_MEMORY_SPACE_DESCRIPTOR  GcdMemorySpaceDescriptor;
      Status = gDS->GetMemorySpaceDescriptor (FspMemoryDescriptor->PhysicalStart, &GcdMemorySpaceDescriptor);
      if (!EFI_ERROR (Status)) {
        if (GcdMemorySpaceDescriptor.GcdMemoryType == EfiGcdMemoryTypeNonExistent) {
          Status = gDS->AddMemorySpace (
                          EfiGcdMemoryTypeMemoryMappedIo,
                          FspMemoryDescriptor->PhysicalStart,
                          FspMemoryDescriptor->NumberOfPages<<EFI_PAGE_SHIFT,
                          EFI_MEMORY_RUNTIME | EFI_MEMORY_UC
                          );
          if (!EFI_ERROR (Status)) {
            Status = gDS->AllocateMemorySpace (
                            EfiGcdAllocateAddress,
                            EfiGcdMemoryTypeMemoryMappedIo,
                            12,
                            FspMemoryDescriptor->NumberOfPages<<EFI_PAGE_SHIFT,
                            &FspMemoryDescriptor->PhysicalStart,
                            gImageHandle,
                            NULL
                            );
            if (!EFI_ERROR (Status)) {
              Status = gDS->GetMemorySpaceDescriptor (FspMemoryDescriptor->PhysicalStart, &GcdMemorySpaceDescriptor);
            }
          }
        }
      }

      // Attempt to set runtime attribute
      if (!EFI_ERROR (Status)) {
        if (GcdMemorySpaceDescriptor.GcdMemoryType == EfiGcdMemoryTypeMemoryMappedIo) {
          UINT64  Attributes = GcdMemorySpaceDescriptor.Attributes | EFI_MEMORY_RUNTIME | EFI_MEMORY_UC;
          Status = gDS->SetMemorySpaceAttributes (
                          FspMemoryDescriptor->PhysicalStart,
                          FspMemoryDescriptor->NumberOfPages<<EFI_PAGE_SHIFT,
                          Attributes
                          );
        }
      }

      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "MMIO Region 0x%08p~0x%08p cannot be reserved as RT.\n", \
          FspMemoryDescriptor->PhysicalStart, \
          (FspMemoryDescriptor->PhysicalStart+(FspMemoryDescriptor->NumberOfPages<<EFI_PAGE_SHIFT))
          ));
        ASSERT (FALSE);
      }

      DEBUG ((
        DEBUG_ERROR,
        "MMIO Region 0x%08p~0x%08p is reserved as RT.\n", \
        FspMemoryDescriptor->PhysicalStart, \
        (FspMemoryDescriptor->PhysicalStart+(FspMemoryDescriptor->NumberOfPages<<EFI_PAGE_SHIFT))
        ));
    } else {
      if (  (FspMemoryDescriptor->PhysicalStart >= MemoryRegionHob->BeginAddress)
         && ((FspMemoryDescriptor->PhysicalStart+(FspMemoryDescriptor->NumberOfPages<<EFI_PAGE_SHIFT)) <= (MemoryRegionHob->BeginAddress+MemoryRegionHob->Length)))
      {
        Status = gBS->FreePages (FspMemoryDescriptor->PhysicalStart, FspMemoryDescriptor->NumberOfPages);
        ASSERT (Status == EFI_SUCCESS);
        if (FspMemoryDescriptor->Type != EfiConventionalMemory) {
          Status = gBS->AllocatePages (AllocateAddress, FspMemoryDescriptor->Type, FspMemoryDescriptor->NumberOfPages, &FspMemoryDescriptor->PhysicalStart);
          ASSERT (Status == EFI_SUCCESS);
        } else {
          DEBUG ((
            DEBUG_ERROR,
            "Address 0x%08p~0x%08p is free\n", \
            FspMemoryDescriptor->PhysicalStart, \
            (FspMemoryDescriptor->PhysicalStart+(FspMemoryDescriptor->NumberOfPages<<EFI_PAGE_SHIFT))
            ));
        }
      } else {
        DEBUG ((
          DEBUG_ERROR,
          "Address 0x%08p~0x%08p out of range\n", \
          FspMemoryDescriptor->PhysicalStart, \
          (FspMemoryDescriptor->PhysicalStart+(FspMemoryDescriptor->NumberOfPages<<EFI_PAGE_SHIFT))
          ));
      }
    }

    FspMemoryDescriptor = (EFI_MEMORY_DESCRIPTOR *)((UINTN)FspMemoryDescriptor-ExportedInterfaceHob->FinalMemoryDescriptorSize);
  }

  // Atomic code ends here
  gBS->RestoreTPL (TPL_CALLBACK);
}

/**
  Notification function of EVT_GROUP_READY_TO_BOOT event group.

  This is a notification function registered on EVT_GROUP_READY_TO_BOOT event group.
  When the Boot Manager is about to load and execute a boot option, it reclaims variable
  storage if free size is below the threshold.

  @param[in] Event        Event whose notification function is being invoked.
  @param[in] Context      Pointer to the notification function's context.

**/
VOID
EFIAPI
OnReadyToBoot (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  NOTIFY_PHASE_PARAMS  NotifyPhaseParams;
  EFI_STATUS           Status;

  NotifyPhaseParams.Phase = EnumInitPhaseReadyToBoot;
  PERF_START_EX (&gFspApiPerformanceGuid, "EventRec", NULL, 0, FSP_STATUS_CODE_READY_TO_BOOT_NOTIFICATION | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_ENTRY);
  DisableTimer ();
  Status = CallFspNotifyPhase (&NotifyPhaseParams);
  EnableTimer ();
  PERF_END_EX (&gFspApiPerformanceGuid, "EventRec", NULL, 0, FSP_STATUS_CODE_READY_TO_BOOT_NOTIFICATION | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);

  //
  // Reset the system if FSP API returned FSP_STATUS_RESET_REQUIRED status
  //
  if ((Status >= FSP_STATUS_RESET_REQUIRED_COLD) && (Status <= FSP_STATUS_RESET_REQUIRED_8)) {
    DEBUG ((DEBUG_INFO, "FSP NotifyPhase ReadyToBoot requested reset 0x%x\n", Status));
    CallFspWrapperResetSystem (Status);
  }

  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "FSP NotifyPhase ReadyToBoot failed, status: 0x%x\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "FSP NotifyPhase ReadyToBoot Success.\n"));
    // Now we install ACPI Tables.
    EFI_ACPI_TABLE_PROTOCOL  *AcpiTableProtocol = NULL;
    VOID                     *FspsUpdHob        = GetFirstGuidHob (&gFspsUpdDataPointerAddressGuid);
    if ( FspsUpdHob != NULL ) {
      FspsUpd = ((FSPS_UPD *)(UINTN)(*(UINT32 *)GET_GUID_HOB_DATA (FspsUpdHob)));
      Status  = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiTableProtocol);
      if (!EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "%a:FSP-S UPD Ptr:%x\n", __func__, FspsUpd));
        UINTN  TableKey = 0;
        if (ExportedInterfaceHob->AcpiTpm2Table != 0) {
          DEBUG ((DEBUG_INFO, "TPM2 Table: %x\n", ExportedInterfaceHob->AcpiTpm2Table));
          Status |= AcpiTableProtocol->InstallAcpiTable (
                                         AcpiTableProtocol,
                                         (VOID *)(UINTN)(ExportedInterfaceHob->AcpiTpm2Table),
                                         ((EFI_ACPI_SDT_HEADER *)(UINTN)(ExportedInterfaceHob->AcpiTpm2Table))->Length,
                                         &TableKey
                                         );
        }

        if (ExportedInterfaceHob->AcpiCratTable != 0) {
          DEBUG ((DEBUG_INFO, "CRAT Table: %x\n", ExportedInterfaceHob->AcpiCratTable));
          Status |= AcpiTableProtocol->InstallAcpiTable (
                                         AcpiTableProtocol,
                                         (VOID *)(UINTN)(ExportedInterfaceHob->AcpiCratTable),
                                         ((EFI_ACPI_SDT_HEADER *)(UINTN)(ExportedInterfaceHob->AcpiCratTable))->Length,
                                         &TableKey
                                         );
        }

        if (ExportedInterfaceHob->AcpiCditTable != 0) {
          DEBUG ((DEBUG_INFO, "CDIT Table: %x\n", ExportedInterfaceHob->AcpiCditTable));
          Status |= AcpiTableProtocol->InstallAcpiTable (
                                         AcpiTableProtocol,
                                         (VOID *)(UINTN)(ExportedInterfaceHob->AcpiCditTable),
                                         ((EFI_ACPI_SDT_HEADER *)(UINTN)(ExportedInterfaceHob->AcpiCditTable))->Length,
                                         &TableKey
                                         );
        }

        if (ExportedInterfaceHob->AcpiIvrsTable != 0) {
          DEBUG ((DEBUG_INFO, "IVRS Table: %x\n", ExportedInterfaceHob->AcpiIvrsTable));
          Status |= AcpiTableProtocol->InstallAcpiTable (
                                         AcpiTableProtocol,
                                         (VOID *)(UINTN)(ExportedInterfaceHob->AcpiIvrsTable),
                                         ((EFI_ACPI_SDT_HEADER *)(UINTN)(ExportedInterfaceHob->AcpiIvrsTable))->Length,
                                         &TableKey
                                         );
        }

        for (int i = 0; i < MAX_ACPI_SSDT_TABLE_COUNT; i++) {
          if (ExportedInterfaceHob->AcpiSsdtTables[i] != 0) {
            DEBUG ((DEBUG_INFO, "SSDT Table #%d: %x\n", i, ExportedInterfaceHob->AcpiSsdtTables[i]));
            Status |= AcpiTableProtocol->InstallAcpiTable (
                                           AcpiTableProtocol,
                                           (VOID *)(UINTN)(ExportedInterfaceHob->AcpiSsdtTables[i]),
                                           ((EFI_ACPI_SDT_HEADER *)(UINTN)(ExportedInterfaceHob->AcpiSsdtTables[i]))->Length,
                                           &TableKey
                                           );
          }
        }
      }
    }

    ReportFspMemoryUsage ();
  }

  gBS->CloseEvent (Event);
}

VOID *
EFIAPI
ConvertPointer (
  VOID  *In
  )
{
  if (gRT->ConvertPointer (0, &In) == EFI_SUCCESS) {
    return In;
  }

  return NULL;
}

VOID
EFIAPI
OnVirtualAddressChange (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  typedef VOID (EFIAPI *FSP_VIRTUAL_ADDRESS_CHANGE_CALLBACK)(FSPS_UPD *NewUpdAddress);
  FSP_VIRTUAL_ADDRESS_CHANGE_CALLBACK  VirtualAddressChangeCallback;

  // VOID *VirtualAddressChangeCallbackAddress;
  // First, we convert the FSP UPD Address.
  Status = gRT->ConvertPointer (0, (VOID **)&FspsUpdInRt);
  ASSERT (Status == EFI_SUCCESS);
  FspsUpd                              = (FSPS_UPD *)FspsUpdInRt;
  ExportedInterfaceHob->ConvertPointer = ConvertPointer;
  VirtualAddressChangeCallback         = ExportedInterfaceHob->VirtualAddressChangeCallback;
  VirtualAddressChangeCallback (FspsUpdInRt);
  return;
}

/**
  This stage is notified just before the firmware/Preboot environment transfers
  management of all system resources to the OS or next level execution environment.

  @param  Event         Event whose notification function is being invoked.
  @param  Context       Pointer to the notification function's context, which is
                        always zero in current implementation.

**/
VOID
EFIAPI
OnEndOfFirmware (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  NOTIFY_PHASE_PARAMS              NotifyPhaseParams;
  EFI_STATUS                       Status;
  ADD_PERFORMANCE_RECORD_PROTOCOL  *AddPerfRecordInterface;
  EFI_PEI_HOB_POINTERS             Hob;
  VOID                             **FspHobListPtr;

  gBS->CloseEvent (Event);
  // The FSP UPD is meant to be used in UEFI RT mode.
  // For this reason, we MUST copy the UPD to RT Memory region.
  DEBUG ((DEBUG_ERROR, "Copy :%p<->%p\n", FspsUpd, FspsUpdInRt));
  CopyMem (FspsUpdInRt, FspsUpd, sizeof (FSPS_UPD));
  NotifyPhaseParams.Phase = EnumInitPhaseEndOfFirmware;
  PERF_START_EX (&gFspApiPerformanceGuid, "EventRec", NULL, 0, FSP_STATUS_CODE_END_OF_FIRMWARE_NOTIFICATION | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_ENTRY);
  DisableTimer ();
  Status = CallFspNotifyPhase (&NotifyPhaseParams);
  EnableTimer ();
  PERF_END_EX (&gFspApiPerformanceGuid, "EventRec", NULL, 0, FSP_STATUS_CODE_END_OF_FIRMWARE_NOTIFICATION | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);

  //
  // Reset the system if FSP API returned FSP_STATUS_RESET_REQUIRED status
  //
  if ((Status >= FSP_STATUS_RESET_REQUIRED_COLD) && (Status <= FSP_STATUS_RESET_REQUIRED_8)) {
    DEBUG ((DEBUG_INFO, "FSP NotifyPhase EndOfFirmware requested reset 0x%x\n", Status));
    CallFspWrapperResetSystem (Status);
  }

  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "FSP NotifyPhase EndOfFirmware failed, status: 0x%x\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "FSP NotifyPhase EndOfFirmware Success.\n"));
  }

  // Add the FSP interface here.
  ExportedInterfaceHob->ConvertPointer = ConvertPointer;
  Status                               = gBS->LocateProtocol (
                                                &gAddPerfRecordProtocolGuid,
                                                NULL,
                                                (VOID **)&AddPerfRecordInterface
                                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "gAddPerfRecordProtocolGuid - Locate protocol failed\n"));
    return;
  } else {
    Hob.Raw = GetFirstGuidHob (&gFspHobGuid);
    if (Hob.Raw != NULL) {
      FspHobListPtr = GET_GUID_HOB_DATA (Hob.Raw);
      AddPerfRecordInterface->AddPerformanceRecords ((VOID *)(UINTN)(((UINT32)(UINTN)*FspHobListPtr) & 0xFFFFFFFF));
    }
  }
}

STATIC
VOID *
GetFspHobList (
  VOID
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;

  GuidHob = GetFirstGuidHob (&gFspHobGuid);
  if (GuidHob != NULL) {
    return *(VOID **)GET_GUID_HOB_DATA (GuidHob);
  } else {
    return NULL;
  }
}

/**
  Main entry for the FSP DXE module.

  This routine registers two callbacks to call fsp's notifies.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
FspWrapperNotifyDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   ReadyToBootEvent;
  VOID        *Registration;
  EFI_EVENT   ProtocolNotifyEvent;
  UINT32      FspApiMask;

  if (!PcdGet8 (PcdFspModeSelection)) {
    // Dispatch Mode
    return EFI_SUCCESS;
  }

  //
  // Load this driver's image to memory
  //
  Status = RelocateImageUnder4GIfNeeded (ImageHandle, SystemTable);
  if (EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  FspApiMask = PcdGet32 (PcdSkipFspApi);
  if ((FspApiMask & FSP_API_NOTIFY_PHASE_AFTER_PCI_ENUMERATION) == 0) {
    ProtocolNotifyEvent = EfiCreateProtocolNotifyEvent (
                            &gEfiPciEnumerationCompleteProtocolGuid,
                            TPL_CALLBACK,
                            OnPciEnumerationComplete,
                            NULL,
                            &Registration
                            );
    ASSERT (ProtocolNotifyEvent != NULL);
  }

  Status = EfiCreateEventReadyToBootEx (
             TPL_CALLBACK,
             OnReadyToBoot,
             NULL,
             &ReadyToBootEvent
             );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  OnEndOfFirmware,
                  NULL,
                  &gEfiEventExitBootServicesGuid,
                  &mExitBootServicesEvent
                  );

  gBS->CreateEventEx (
         EVT_NOTIFY_SIGNAL,
         TPL_NOTIFY,
         OnVirtualAddressChange,
         NULL,
         &gEfiEventVirtualAddressChangeGuid,
         &mVirtualAddressChangeEvent
         );
  ASSERT_EFI_ERROR (Status);
  // The FSP UPD is meant to be used in UEFI RT mode.
  // For this reason, we MUST copy the UPD to RT Memory region.
  Status = gBS->AllocatePool (EfiRuntimeServicesData, sizeof (FSPS_UPD), (VOID **)&FspsUpdInRt);
  ASSERT ((Status == EFI_SUCCESS));
  Status = gBS->LocateProtocol (&gEfiTimerArchProtocolGuid, NULL, (VOID **)&gTimer);
  ASSERT ((Status == EFI_SUCCESS));
  VOID  *ExportedInterfaceRawHob = GetNextGuidHob (&gFspExportedInterfaceHobGuid, (VOID *)((UINTN)GetFspHobList ()&0xFFFFFFFF));

  DEBUG ((DEBUG_ERROR, " ExportedInterfaceRawHob:%011p\n", ExportedInterfaceRawHob));
  if ( ExportedInterfaceRawHob != NULL) {
    ExportedInterfaceHob = GET_GUID_HOB_DATA (ExportedInterfaceRawHob);
    DEBUG ((DEBUG_ERROR, " ExportedInterfaceHob:%011p\n", ExportedInterfaceHob));
    ExportedInterfaceHob = ExportedInterfaceHob->ExportedInterfaceHobAddressAfterNotifyPhase;
    DEBUG ((DEBUG_ERROR, "New ExportedInterfaceHob:%011p\n", ExportedInterfaceHob));
  }

  return EFI_SUCCESS;
}

VOID
EFIAPI
CallFspWrapperResetSystem (
  IN EFI_STATUS  FspStatusResetType
  )
{
  //
  // Perform reset according to the type.
  //

  CpuDeadLoop ();
}
