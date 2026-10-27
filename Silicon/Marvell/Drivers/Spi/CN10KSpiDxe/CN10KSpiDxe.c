/** @file

  Marvell CN10K xSPI DXE driver.

  Connects the Cadence xSPI controller to the Marvell SPI master and SPI flash
  protocols. The controller registers and its direct-access window are mapped
  as uncached runtime memory so that the flash remains accessible after
  ExitBootServices().

  Copyright (c) 2025-2026 Marvell

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CN10KSpiDxe.h"

#include <IndustryStandard/CadenceXspi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CadenceXspiLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NorFlashInfoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeLib.h>

#include <Guid/EventGroup.h>

#define CN10K_SPI_REGBASE ((UINTN)(FixedPcdGet64 (PcdMPIControllerBase64) + \
                                   FixedPcdGet64 (PcdSpiNorBusControllerOffs64) * \
                                   FixedPcdGet32 (PcdSpiNorBusControllerNum)))

STATIC CN10K_SPI_CONFIG  mSpiConfig = {
  .MpiRegBase            = CN10K_SPI_REGBASE,
  .SpiDirectBase         = CN10K_SPI_REGBASE + SPI_DIRECT_ACC_BASE,
  .ChipSelect            = FixedPcdGet32 (PcdSpiNorChipSelect),
  .ClkFrequency          = FixedPcdGet32 (PcdSpiNorClkFrequency),
  .BusControllerNum      = FixedPcdGet32 (PcdSpiNorBusControllerNum),
  .FlashSize             = FixedPcdGet32 (PcdFlashDeviceSize),
};

STATIC MARVELL_SPI_MASTER_PROTOCOL  mSpiMasterProtocol;
STATIC MARVELL_SPI_FLASH_PROTOCOL   mSpiFlashProtocol;
STATIC EFI_EVENT                    mSpiVirtualAddrChangeEvent;

/**
  Program the xSPI serial clock divider.

  The controller reference clock is assumed to be 800 MHz. The first divider in
  ClkDivList whose resulting frequency does not exceed the configured
  ClkFrequency is selected; if no entry satisfies the limit, the slowest
  divider (128) is used. To change the divider safely, the clock is momentarily
  disabled before the register is rewritten and re-enabled.
**/
STATIC
VOID
SpiSetupClock (
  VOID
  )
{
  STATIC CONST UINT8  ClkDivList[] = {
    4, 6, 8, 10, 12, 16, 18, 20, 24, 32, 40, 50, 64, 128, 0xFF
  };
  UINT8               Index;
  UINT32              ClkVal;
  UINT32              ClkReg;

  Index = 0;
  while (ClkDivList[Index] != 0xFF) {
    ClkVal = 800000000 / ClkDivList[Index];
    if (ClkVal <= mSpiConfig.ClkFrequency) {
      break;
    }

    Index++;
  }

  if (ClkDivList[Index] == 0xFF) {
    Index = 0xD;
  }

  ClkReg = XSPI_READ32 (mSpiConfig.MpiRegBase + SPI_CLK_CTRL_OFFSET);
  if (((ClkReg >> 1) & 0xF) != Index) {
    ClkReg &= ~BIT0;
    XSPI_WRITE32 (mSpiConfig.MpiRegBase + SPI_CLK_CTRL_OFFSET, ClkReg);
    ClkReg &= ~0x1E;
    ClkReg |= ((Index << 1) | BIT0);
    XSPI_WRITE32 (mSpiConfig.MpiRegBase + SPI_CLK_CTRL_OFFSET, ClkReg);
  }
}

/**
  Add a memory region to the GCD and mark it as uncached runtime memory.

  Existing MMIO descriptors are tolerated: an EFI_ACCESS_DENIED from
  AddMemorySpace() is ignored, and an EFI_UNSUPPORTED from
  SetMemorySpaceAttributes() is accepted when the region is already described
  as MMIO in the GCD.

  @param[in]  Base  Base address of the region, in bytes.
  @param[in]  Size  Size of the region, in bytes.

  @retval  EFI_SUCCESS  The region is mapped with the required attributes.
  @retval  Others       The region could not be added or configured.
**/
STATIC
EFI_STATUS
SpiMapRegion (
  IN UINTN  Base,
  IN UINTN  Size
  )
{
  EFI_STATUS  Status;

  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  Descriptor;

  Status = gDS->AddMemorySpace (
                  EfiGcdMemoryTypeMemoryMappedIo,
                  Base,
                  Size,
                  EFI_MEMORY_UC | EFI_MEMORY_RUNTIME
                  );
  if (EFI_ERROR (Status) && (Status != EFI_ACCESS_DENIED)) {
    return Status;
  }

  Status = gDS->SetMemorySpaceAttributes (
                  Base,
                  Size,
                  EFI_MEMORY_UC | EFI_MEMORY_RUNTIME
                  );
  if ((Status == EFI_UNSUPPORTED) &&
      (gDS->GetMemorySpaceDescriptor (Base, &Descriptor) == EFI_SUCCESS) &&
      (Descriptor.GcdMemoryType == EfiGcdMemoryTypeMemoryMappedIo))
  {
    Status = EFI_SUCCESS;
  }

  return Status;
}

/**
  Map the controller register block and the direct-access window.

  The register region covers everything up to and including the ownership lock
  register; the second region covers the direct-access (SDMA) window used by
  the library for data transfers.

  @retval  EFI_SUCCESS  Both regions were mapped.
  @retval  Others       Mapping either region failed.
**/
STATIC
EFI_STATUS
SpiMapController (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = SpiMapRegion (
             mSpiConfig.MpiRegBase,
             EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (SPI_LOCK_OFFSET))
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SpiMapRegion (
             mSpiConfig.SpiDirectBase,
             EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (SPI_DIRECT_ACC_SIZE))
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Acquire exclusive ownership of the flash controller.

  A non-zero lock register means another agent (for example a secure monitor)
  currently owns the controller. If the register is clear it is claimed for
  UEFI by writing SPI_UEFI_OWN.

  @param[in]  Context  Unused. Present to match CADENCE_XSPI_BUS_LOCK.

  @retval  EFI_SUCCESS       Ownership was acquired.
  @retval  EFI_ACCESS_DENIED  The controller is owned by another agent.
**/
STATIC
EFI_STATUS
SpiLockCtrl (
  IN VOID  *Context
  )
{
  UINT32          Tmp;

  Tmp = XSPI_READ32 (mSpiConfig.MpiRegBase + SPI_LOCK_OFFSET);
  if (Tmp != 0) {
    return EFI_ACCESS_DENIED;
  }

  XSPI_WRITE32 (mSpiConfig.MpiRegBase + SPI_LOCK_OFFSET, SPI_UEFI_OWN);
  return EFI_SUCCESS;
}

/**
  Release ownership of the flash controller.

  Writes SPI_NOT_CLAIMED to the lock register so another agent may take over.

  @param[in]  Context  Unused. Present to match CADENCE_XSPI_BUS_UNLOCK.
**/
STATIC
VOID
SpiUnlockCtrl (
  IN VOID  *Context
  )
{
  XSPI_WRITE32 (mSpiConfig.MpiRegBase + SPI_LOCK_OFFSET, SPI_NOT_CLAIMED);
}

//
// Serialize access to the shared controller with the ownership lock register.
//
STATIC CONST CADENCE_XSPI_BUS_OPS  mSpiBusOps = {
  SpiLockCtrl,
  SpiUnlockCtrl,
  NULL
};

/**
  Identify the attached NOR flash device.

  Reads the JEDEC ID through the Cadence library and looks up the matching
  NOR_FLASH_INFO, which is stored in SpiDev->Info.

  @param[in]  SpiDev        SPI device instance to update.
  @param[in]  UseInRuntime  TRUE if the flash information is allocated for use
                            after ExitBootServices().

  @retval  EFI_SUCCESS       The device was identified.
  @retval  Others            The ID read or the information lookup failed.
**/
STATIC
EFI_STATUS
EFIAPI
CN10KSpiFlashReadId (
  IN     SPI_DEVICE  *SpiDev,
  IN     BOOLEAN     UseInRuntime
  )
{
  EFI_STATUS  Status;
  UINT8       Id[NOR_FLASH_MAX_ID_LEN];

  ZeroMem (Id, sizeof (Id));

  Status = CadenceXspiFlashReadId (Id);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: RDID failed: %r\n", __FUNCTION__, Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "%a: JEDEC %02x %02x %02x\n", __FUNCTION__, Id[0], Id[1], Id[2]));

  Status = NorFlashGetInfo (Id, &SpiDev->Info, UseInRuntime);
  DEBUG ((DEBUG_INFO, "%a: NorFlashGetInfo: %r\n", __FUNCTION__, Status));

  return Status;
}

/**
  Initialize a flash device for use through the SPI flash protocol.

  The driver always uses 3-byte (24-bit) flash addresses.

  @param[in]  This    The SPI flash protocol instance.
  @param[in]  SpiDev  SPI device to initialize.

  @retval  EFI_SUCCESS  The device is ready.
**/
STATIC
EFI_STATUS
EFIAPI
CN10KSpiFlashInit (
  IN MARVELL_SPI_FLASH_PROTOCOL  *This,
  IN SPI_DEVICE                  *SpiDev
  )
{
  SpiDev->AddrSize = 3;
  return EFI_SUCCESS;
}

/**
  Read data from the NOR flash.

  @param[in]   SpiDev         SPI device to read from.
  @param[in]   Address        Flash address to start reading at.
  @param[in]   DataByteCount  Number of bytes to read.
  @param[out]  Buffer         Destination buffer.

  @retval  EFI_SUCCESS  The data was read.
  @retval  Others       The read failed.
**/
STATIC
EFI_STATUS
EFIAPI
CN10KSpiFlashRead (
  IN SPI_DEVICE  *SpiDev,
  IN UINT32      Address,
  IN UINTN       DataByteCount,
  OUT VOID       *Buffer
  )
{
  return CadenceXspiFlashRead (Address, DataByteCount, Buffer);
}

/**
  Write data to the NOR flash.

  The region must already be erased. The library splits the transfer on page
  boundaries and waits for each page to be programmed.

  @param[in]  SpiDev         SPI device to write to.
  @param[in]  Address        Flash address to start writing at.
  @param[in]  DataByteCount  Number of bytes to write.
  @param[in]  Buffer         Source buffer.

  @retval  EFI_SUCCESS  The data was written.
  @retval  Others       The write failed.
**/
STATIC
EFI_STATUS
EFIAPI
CN10KSpiFlashWrite (
  IN SPI_DEVICE  *SpiDev,
  IN UINT32      Address,
  IN UINTN       DataByteCount,
  IN VOID        *Buffer
  )
{
  return CadenceXspiFlashWrite (Address, DataByteCount, Buffer);
}

/**
  Erase a region of the NOR flash.

  The Cadence library erases in 4 KB blocks and waits for each block to
  complete.

  @param[in]  SpiDev         SPI device to erase.
  @param[in]  Address        Flash address to start erasing at.
  @param[in]  DataByteCount  Number of bytes to erase.

  @retval  EFI_SUCCESS  The region was erased.
  @retval  Others       The erase failed.
**/
STATIC
EFI_STATUS
EFIAPI
CN10KSpiFlashErase (
  IN SPI_DEVICE  *SpiDev,
  IN UINTN       Address,
  IN UINTN       DataByteCount
  )
{
  return CadenceXspiFlashErase ((UINT32)Address, DataByteCount);
}

/**
  Update a flash region using read-modify-erase-write.

  The region is processed one sector at a time: the current sector is read,
  the new bytes are merged into the read buffer, and the sector is erased and
  written back. This preserves bytes in the first and last sectors that are
  outside the requested range.

  If Progress is not NULL it is called with StartPercentage before the first
  sector and then advanced after each sector, always ending at EndPercentage
  on success.

  @param[in]  SpiDev           SPI device to update.
  @param[in]  Address          Flash address to start updating at.
  @param[in]  DataByteCount    Number of bytes to update.
  @param[in]  Buffer           Source data.
  @param[in]  Progress         Optional progress callback.
  @param[in]  StartPercentage  Progress value reported before updating.
  @param[in]  EndPercentage    Progress value reported when updating completes.

  @retval  EFI_SUCCESS            The region was updated.
  @retval  EFI_INVALID_PARAMETER  Buffer is NULL, DataByteCount is zero, or the
                                  sector size is zero.
  @retval  EFI_OUT_OF_RESOURCES   The sector buffer could not be allocated.
  @retval  Others                 A read, erase, or write failed.
**/
STATIC
EFI_STATUS
EFIAPI
CN10KSpiFlashUpdateWithProgress (
  IN SPI_DEVICE                                     *SpiDev,
  IN UINT32                                         Address,
  IN UINTN                                          DataByteCount,
  IN UINT8                                          *Buffer,
  IN EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS  Progress,
  IN UINTN                                          StartPercentage,
  IN UINTN                                          EndPercentage
  )
{
  EFI_STATUS  Status;
  UINTN       Chunk;
  UINTN       ProgressPercentage;
  UINTN       ProgressStep;
  UINTN       SectorSize;
  UINT8       *TmpBuf;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (DataByteCount == 0) {
    return EFI_INVALID_PARAMETER;
  }

  SectorSize = SpiDev->Info->SectorSize;
  if (SectorSize == 0) {
    return EFI_INVALID_PARAMETER;
  }

  TmpBuf = AllocatePool (SectorSize);
  if (TmpBuf == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (Progress != NULL) {
    Progress (StartPercentage);
  }

  ProgressPercentage = StartPercentage;
  ProgressStep       = (UINTN)((UINT64)(EndPercentage - StartPercentage) *
                               SectorSize / DataByteCount);
  Status             = EFI_SUCCESS;

  while (DataByteCount != 0) {
    Chunk = MIN (DataByteCount, SectorSize);

    Status = CadenceXspiFlashRead (Address, SectorSize, TmpBuf);
    if (EFI_ERROR (Status)) {
      break;
    }

    CopyMem (TmpBuf, Buffer, Chunk);

    Status = CadenceXspiFlashErase (Address, SectorSize);
    if (EFI_ERROR (Status)) {
      break;
    }

    Status = CadenceXspiFlashWrite (Address, SectorSize, TmpBuf);
    if (EFI_ERROR (Status)) {
      break;
    }

    Address       += SectorSize;
    Buffer        += Chunk;
    DataByteCount -= Chunk;

    if (Progress != NULL) {
      ProgressPercentage = MIN (
                             ProgressPercentage + ProgressStep,
                             EndPercentage
                             );
      Progress (ProgressPercentage);
    }
  }

  FreePool (TmpBuf);

  if (!EFI_ERROR (Status) && (Progress != NULL) &&
      (ProgressPercentage != EndPercentage))
  {
    Progress (EndPercentage);
  }

  return Status;
}

/**
  Update a flash region without progress reporting.

  Wrapper around CN10KSpiFlashUpdateWithProgress() that reports no progress.

  @param[in]  SpiDev         SPI device to update.
  @param[in]  Address        Flash address to start updating at.
  @param[in]  DataByteCount  Number of bytes to update.
  @param[in]  Buffer         Source data.

  @retval  EFI_SUCCESS            The region was updated.
  @retval  EFI_INVALID_PARAMETER  Buffer is NULL or DataByteCount is zero.
  @retval  Others                 The update failed.
**/
STATIC
EFI_STATUS
EFIAPI
CN10KSpiFlashUpdate (
  IN SPI_DEVICE  *SpiDev,
  IN UINT32      Address,
  IN UINTN       DataByteCount,
  IN UINT8       *Buffer
  )
{
  return CN10KSpiFlashUpdateWithProgress (
           SpiDev,
           Address,
           DataByteCount,
           Buffer,
           NULL,
           0,
           100
           );
}

/**
  Initialize the SPI master.

  The controller is configured once in the driver entry point, so there is
  nothing left to do here.

  @param[in]  This  The SPI master protocol instance.

  @retval  EFI_SUCCESS  Always returned.
**/
STATIC
EFI_STATUS
EFIAPI
CN10KSpiMasterInit (
  IN MARVELL_SPI_MASTER_PROTOCOL  *This
  )
{
  return EFI_SUCCESS;
}

/**
  Populate a SPI device with the controller parameters.

  The chip select and SPI mode come from the platform PCDs rather than from the
  Cs and Mode arguments, which are ignored. The returned device uses SPI mode 0
  and 3-byte addressing.

  @param[in]  This    The SPI master protocol instance.
  @param[in]  SpiDev  SPI device to initialize.
  @param[in]  Cs      Chip select requested by the caller. Ignored.
  @param[in]  Mode    SPI mode requested by the caller. Ignored.

  @return  The initialized SpiDev pointer.
**/
STATIC
SPI_DEVICE *
EFIAPI
CN10KSpiSetupDevice (
  IN MARVELL_SPI_MASTER_PROTOCOL  *This,
  IN SPI_DEVICE                   *SpiDev,
  IN UINTN                        Cs,
  IN SPI_MODE                     Mode
  )
{
  SpiDev->HostRegisterBaseAddress = mSpiConfig.MpiRegBase;
  SpiDev->CoreClock               = 800000000;
  SpiDev->MaxFreq                 = mSpiConfig.ClkFrequency;
  SpiDev->Cs                      = mSpiConfig.ChipSelect;
  SpiDev->Mode                    = SPI_MODE0;
  SpiDev->AddrSize                = 3;

  return SpiDev;
}

/**
  Release a SPI device.

  The device is owned by the caller, so no resources are freed here.

  @param[in]  SpiDev  SPI device to release.

  @retval  EFI_SUCCESS  Always returned.
**/
STATIC
EFI_STATUS
EFIAPI
CN10KSpiFreeDevice (
  IN SPI_DEVICE  *SpiDev
  )
{
  return EFI_SUCCESS;
}

/**
  Perform a generic full-duplex SPI transfer.

  Not implemented for the Cadence xSPI controller; use the SPI flash protocol
  operations instead.

  @param[in]  This           The SPI master protocol instance.
  @param[in]  SpiDev         SPI device to transfer with.
  @param[in]  DataByteCount  Number of bytes to transfer.
  @param[in]  DataOut        Data to send.
  @param[in]  DataIn         Buffer for received data.
  @param[in]  Flag           Transfer flags.

  @retval  EFI_UNSUPPORTED  Always returned.
**/
STATIC
EFI_STATUS
EFIAPI
CN10KSpiTransfer (
  IN MARVELL_SPI_MASTER_PROTOCOL  *This,
  IN SPI_DEVICE                   *SpiDev,
  IN UINTN                        DataByteCount,
  IN VOID                         *DataOut,
  IN VOID                         *DataIn,
  IN UINTN                        Flag
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Send a raw command followed by a data phase.

  Not implemented for the Cadence xSPI controller; use the SPI flash protocol
  operations instead.

  @param[in]   This      The SPI master protocol instance.
  @param[in]   SpiDev    SPI device to communicate with.
  @param[in]   Cmd       Command bytes to send.
  @param[in]   CmdSize   Number of command bytes.
  @param[in]   DataOut   Data to send after the command.
  @param[out]  DataIn    Buffer for data received after the command.
  @param[in]   DataSize  Number of data bytes.

  @retval  EFI_UNSUPPORTED  Always returned.
**/
STATIC
EFI_STATUS
EFIAPI
CN10KSpiReadWrite (
  IN  MARVELL_SPI_MASTER_PROTOCOL  *This,
  IN  SPI_DEVICE                   *SpiDev,
  IN  UINT8                        *Cmd,
  IN  UINTN                        CmdSize,
  IN  UINT8                        *DataOut,
  OUT UINT8                        *DataIn,
  IN  UINTN                        DataSize
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Prepare the controller for use at runtime.

  Re-maps the controller registers and direct-access window so they are
  described as runtime memory before ExitBootServices().

  @param[in]  SpiDev  SPI device whose controller is configured.

  @retval  EFI_SUCCESS  The controller is ready for runtime access.
  @retval  Others       Mapping the controller failed.
**/
STATIC
EFI_STATUS
EFIAPI
CN10KSpiConfigRuntime (
  IN SPI_DEVICE  *SpiDev
  )
{
  return SpiMapController ();
}

/**
  Handle the virtual address change event.

  Converts the controller base addresses kept by this driver and by the Cadence
  library to their virtual equivalents.

  @param[in]  Event    The virtual address change event.
  @param[in]  Context  Unused.
**/
STATIC
VOID
EFIAPI
SpiVirtualNotifyEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EfiConvertPointer (0x0, (VOID **)&mSpiConfig.MpiRegBase);
  EfiConvertPointer (0x0, (VOID **)&mSpiConfig.SpiDirectBase);

  CadenceXspiVirtualNotify ();
}

/**
  CN10K xSPI driver entry point.

  Maps the controller, programs the serial clock, initializes the Cadence
  library, installs the Marvell SPI master and SPI flash protocols, and
  registers the virtual address change handler.

  @param[in]  ImageHandle  The EFI image handle.
  @param[in]  SystemTable  A pointer to the system table.

  @retval  EFI_SUCCESS  The driver initialized successfully.
  @retval  Others       Controller setup, library init, or protocol
                        installation failed.
**/
EFI_STATUS
EFIAPI
CN10KSpiEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  CADENCE_XSPI_CONFIG  XspiConfig;

  DEBUG ((
    DEBUG_INFO,
    "%a: base 0x%lx cs %d clk %d bus %d\n",
    __FUNCTION__,
    mSpiConfig.MpiRegBase,
    mSpiConfig.ChipSelect,
    mSpiConfig.ClkFrequency,
    mSpiConfig.BusControllerNum
    ));

  Status = SpiMapController ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to map SPI controller: %r\n", __FUNCTION__, Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "%a: controller mapped\n", __FUNCTION__));

  SpiSetupClock ();

  XspiConfig.MpiBase    = mSpiConfig.MpiRegBase;
  XspiConfig.DirectBase = mSpiConfig.SpiDirectBase;
  XspiConfig.ChipSelect = mSpiConfig.ChipSelect;
  XspiConfig.BusOps     = mSpiBusOps;

  Status = CadenceXspiInit (&XspiConfig);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to init xSPI library: %r\n", __FUNCTION__, Status));
    return Status;
  }

  mSpiMasterProtocol.Init          = CN10KSpiMasterInit;
  mSpiMasterProtocol.ReadWrite     = CN10KSpiReadWrite;
  mSpiMasterProtocol.Transfer      = CN10KSpiTransfer;
  mSpiMasterProtocol.SetupDevice   = CN10KSpiSetupDevice;
  mSpiMasterProtocol.FreeDevice    = CN10KSpiFreeDevice;
  mSpiMasterProtocol.ConfigRuntime = CN10KSpiConfigRuntime;

  mSpiFlashProtocol.Init               = CN10KSpiFlashInit;
  mSpiFlashProtocol.ReadId             = CN10KSpiFlashReadId;
  mSpiFlashProtocol.Read               = CN10KSpiFlashRead;
  mSpiFlashProtocol.Write              = CN10KSpiFlashWrite;
  mSpiFlashProtocol.Erase              = CN10KSpiFlashErase;
  mSpiFlashProtocol.Update             = CN10KSpiFlashUpdate;
  mSpiFlashProtocol.UpdateWithProgress = CN10KSpiFlashUpdateWithProgress;

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gMarvellSpiMasterProtocolGuid,
                  &mSpiMasterProtocol,
                  &gMarvellSpiFlashProtocolGuid,
                  &mSpiFlashProtocol,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: install protocols failed: %r\n", __FUNCTION__, Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "%a: SPI protocols installed\n", __FUNCTION__));

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  SpiVirtualNotifyEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mSpiVirtualAddrChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
