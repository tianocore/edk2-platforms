/** @file

  Implementation of SpiHcPlatformLibrary for SMM
  
  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MmServicesTableLib.h>
#include <Protocol/SpiSmmHc.h>
#include <Protocol/AmdSpiSmmHcState.h>
#include <Protocol/MmReadyToLock.h>
#include <FchRegistersCommon.h>
#include <Library/PlatformPspRomArmorWhitelistLib.h>
#include <Library/PciSegmentLib.h>
#include <Library/SpiHcPlatformLib.h>
#include <IndustryStandard/SpiNorFlashJedecSfdp.h>
#include <FchRegistersCommon.h>
#include "AmdSpiHcInternal.h"
#include "AmdSpiHcSmmState.h"

#define SPI_HC_MAXIMUM_TRANSFER_BYTES  64

EFI_HANDLE  mSpiStateHandle = 0;

// Global variables to manage the platform-dependent SPI host controller when in DXE or SMM
BOOLEAN                   mPspMailboxSpiMode;
SPI_COMMUNICATION_BUFFER  mSpiCommunicationBuffer;
VOID                      *mRegistration;
EFI_PHYSICAL_ADDRESS      mHcAddress;

// SMM specific global variables to manage the platform-dependent SPI host controller
SMM_EFI_SPI_HC_STATE_PROTOCOL  mStateProtocol;
BOOLEAN                        mSmmAlreadySavedState;
VOID                           *mState;
UINT32                         mStateSize;
UINT32                         mStateRecordCount;

/**
  SPI host controller event notify callback to lock down the SPI chipset

  @param
  @param

  @retval
**/
EFI_STATUS
EFIAPI
AmdSpiHcEventNotify (
  IN CONST  EFI_GUID    *Protocol,
  IN        VOID        *Interface,
  IN        EFI_HANDLE  Handle
  )
{
  EFI_STATUS                     Status;
  EFI_SPI_HC_PROTOCOL            *SpiHc;
  SPI_WHITE_LIST                 *SpiWhitelist;
  SMM_EFI_SPI_HC_STATE_PROTOCOL  *SpiStateProtocol;

  // There can only be one SPI host controller driver in SMM
  Status = gMmst->MmLocateProtocol (
                    &gEfiSpiSmmHcProtocolGuid,
                    NULL,
                    (VOID **)&SpiHc
                    );

  // Call PSP MailBox to change to PSP SPI mode
  Status = gMmst->MmLocateProtocol (
                    &gAmdSpiHcStateProtocolGuid,
                    NULL,
                    (VOID **)&SpiStateProtocol
                    );

  SpiStateProtocol->Lock (SpiStateProtocol);
  Status = PspEnterSmmOnlyMode (&mSpiCommunicationBuffer);

  if (!EFI_ERROR (Status)) {
    mPspMailboxSpiMode = TRUE;
  } else {
    return EFI_DEVICE_ERROR;
  }

  if (FeaturePcdGet (PcdRomArmorWhitelistEnable)) {
    // Retrieve allocated Whitelist table
    Status = GetPspRomArmorWhitelist (&SpiWhitelist);
    if (EFI_ERROR (Status)) {
      if (SpiWhitelist != NULL) {
        FreePool (SpiWhitelist);
      }

      return Status;
    }

    // Send Whitelist to PSP
    Status = PspEnforceWhitelist (SpiWhitelist);
    if (SpiWhitelist != NULL) {
      FreePool (SpiWhitelist);
    }
  }

  return Status;
}

/**
  This function reports the details of the SPI Host Controller to the SpiHc driver.

  @param[out]     Attributes              The suported attributes of the SPI host controller
  @param[out]     FrameSizeSupportMask    The suported FrameSizeSupportMask of the SPI host controller
  @param[out]     MaximumTransferBytes    The suported MaximumTransferBytes of the SPI host controller

  @retval EFI_SUCCESS             SPI_HOST_CONTROLLER_INSTANCE was allocated properly
  @retval EFI_OUT_OF_RESOURCES    The SPI_HOST_CONTROLLER_INSTANCE could not be allocated
*/
EFI_STATUS
EFIAPI
GetPlatformSpiHcDetails (
  OUT     UINT32  *Attributes,
  OUT     UINT32  *FrameSizeSupportMask,
  OUT     UINT32  *MaximumTransferBytes
  )
{
  EFI_STATUS  Status;

  // Fill in the SPI Host Controller Protocol
  *Attributes = HC_SUPPORTS_WRITE_THEN_READ_OPERATIONS |
                HC_SUPPORTS_READ_ONLY_OPERATIONS |
                HC_SUPPORTS_WRITE_ONLY_OPERATIONS;
  *FrameSizeSupportMask = FCH_SPI_FRAME_SIZE_SUPPORT_MASK;
  *MaximumTransferBytes = SPI_HC_MAXIMUM_TRANSFER_BYTES;

  // Set platform specific global variables
  mPspMailboxSpiMode = FALSE; // not supported in SMM
  mHcAddress         = (
                        PciSegmentRead32 (
                          PCI_SEGMENT_LIB_ADDRESS (0x00, FCH_LPC_BUS, FCH_LPC_DEV, FCH_LPC_FUNC, FCH_LPC_REGA0)
                          )
                        ) & 0xFFFFFF00;

  // Allocate Host Controller save state space
  Status = AllocateState ();
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Fill in the SPI HC Save State Protocol
  mStateProtocol.SaveState         = SaveState;
  mStateProtocol.RestoreState      = RestoreState;
  mStateProtocol.Lock              = FchSpiLockSpiHostControllerRegisters;
  mStateProtocol.Unlock            = FchSpiUnlockSpiHostControllerRegisters;
  mStateProtocol.BlockOpcode       = FchSpiBlockOpcode;
  mStateProtocol.UnblockOpcode     = FchSpiUnblockOpcode;
  mStateProtocol.UnblockAllOpcodes = FchSpiUnblockAllOpcodes;

  Status = gMmst->MmInstallProtocolInterface (
                    &mSpiStateHandle,
                    &gAmdSpiHcStateProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mStateProtocol
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_VERBOSE, "Error installing gAmdSpiHcStateProtocolGuid\n"));
  }

  Status = gMmst->MmRegisterProtocolNotify (
                    &gEfiMmReadyToLockProtocolGuid,
                    AmdSpiHcEventNotify,
                    &mRegistration
                    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_VERBOSE, "Error registering gEfiMmReadyToLockProtocolGuid\n"));
  }

  return Status;
}
