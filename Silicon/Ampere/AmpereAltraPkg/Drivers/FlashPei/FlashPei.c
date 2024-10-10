/** @file

  Copyright (c) 2020 - 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Uefi.h>

#include <Library/ArmSmcLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FlashLib.h>
#include <Library/IpmiCommandLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NVParamLib.h>
#include <Library/PcdLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/ResetSystemLib.h>
#include <IndustryStandard/Ipmi.h>

/**
  Check if the IPMI clear cmos flag is set or not.

  @retval TRUE    The clear cmos is set.
  @retval FALSE   The clear cmos is not set.

**/
STATIC
BOOLEAN
IsIpmiClearCmosSet (
  VOID
  )
{
  EFI_STATUS                              Status;
  IPMI_GET_BOOT_OPTIONS_REQUEST           BootOptionsRequest;
  IPMI_GET_BOOT_OPTIONS_RESPONSE          *BootOptionsResponse;
  IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_5  *BootOptionsParameterData;
  IPMI_SET_BOOT_OPTIONS_REQUEST           *SetBootOptionsRequest;
  IPMI_SET_BOOT_OPTIONS_RESPONSE          SetBootOptionsResponse;
  BOOLEAN                                 IsClearCmosSet;

  ZeroMem (&BootOptionsRequest, sizeof (IPMI_GET_BOOT_OPTIONS_REQUEST));
  ZeroMem (&SetBootOptionsResponse, sizeof (IPMI_SET_BOOT_OPTIONS_RESPONSE));

  IsClearCmosSet = FALSE;

  BootOptionsResponse = AllocateZeroPool (sizeof (BootOptionsRequest) + sizeof (BootOptionsParameterData));
  if (BootOptionsResponse == NULL) {
    return FALSE;
  }

  BootOptionsRequest.ParameterSelector.Bits.ParameterSelector = IPMI_BOOT_OPTIONS_PARAMETER_BOOT_FLAGS;

  Status = IpmiGetSystemBootOptions (&BootOptionsRequest, BootOptionsResponse);
  if (EFI_ERROR (Status)) {
    FreePool (BootOptionsResponse);
    return FALSE;
  }

  BootOptionsParameterData = (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_5 *)BootOptionsResponse->ParameterData;
  if (BootOptionsParameterData->Data2.Bits.CmosClear != 0) {
    IsClearCmosSet = TRUE;
  }

  SetBootOptionsRequest = AllocateZeroPool (sizeof (SetBootOptionsRequest) + sizeof (BootOptionsParameterData));
  if (EFI_ERROR (Status)) {
    FreePool (BootOptionsResponse);
    return FALSE;
  }

  SetBootOptionsRequest->ParameterValid.Bits.ParameterSelector    = IPMI_BOOT_OPTIONS_PARAMETER_BOOT_FLAGS;
  SetBootOptionsRequest->ParameterValid.Bits.MarkParameterInvalid = 0x0;

  BootOptionsParameterData->Data2.Bits.CmosClear = 0;
  CopyMem (&SetBootOptionsRequest->ParameterData, BootOptionsParameterData, sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_5));

  Status = IpmiSetSystemBootOptions (SetBootOptionsRequest, &SetBootOptionsResponse);
  if (EFI_ERROR (Status)) {
    IsClearCmosSet = FALSE;
  }

  FreePool (BootOptionsResponse);
  FreePool (SetBootOptionsRequest);

  return IsClearCmosSet;
}

/**
  Entry point function for the PEIM

  @param FileHandle      Handle of the file being invoked.
  @param PeiServices     Describes the list of possible PEI Services.

  @return EFI_SUCCESS    If we installed our PPI

**/
EFI_STATUS
EFIAPI
FlashPeiEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  CHAR8       BuildUuid[PcdGetSize (PcdPlatformConfigUuid)];
  CHAR8       StoredUuid[PcdGetSize (PcdPlatformConfigUuid)];
  EFI_STATUS  Status;
  UINTN       FWNvRamStartOffset;
  UINT32      FWNvRamSize;
  UINTN       NvRamAddress;
  UINT32      NvRamSize;
  BOOLEAN     ClearUserConfig;

  CopyMem ((VOID *)BuildUuid, PcdGetPtr (PcdPlatformConfigUuid), sizeof (BuildUuid));

  NvRamAddress = PcdGet64 (PcdFlashNvStorageVariableBase64);
  NvRamSize    = FixedPcdGet32 (PcdFlashNvStorageVariableSize) +
                 FixedPcdGet32 (PcdFlashNvStorageFtwWorkingSize) +
                 FixedPcdGet32 (PcdFlashNvStorageFtwSpareSize);

  DEBUG ((
    DEBUG_INFO,
    "%a: Using NV store FV in-memory copy at 0x%lx with size 0x%x\n",
    __func__,
    NvRamAddress,
    NvRamSize
    ));

  Status = FlashGetNvRamInfo (&FWNvRamStartOffset, &FWNvRamSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get Flash NVRAM info %r\n", __func__, Status));
    return Status;
  }

  if (FWNvRamSize < (NvRamSize * 2 + sizeof (BuildUuid))) {
    //
    // NVRAM size provided by FW is not enough
    //
    return EFI_INVALID_PARAMETER;
  }

  //
  // We stored BUILD UUID build at the offset NVRAM_SIZE * 2
  //
  Status = FlashReadCommand (
             FWNvRamStartOffset + NvRamSize * 2,
             (UINT8 *)StoredUuid,
             sizeof (StoredUuid)
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ClearUserConfig = IsIpmiClearCmosSet ();

  if (CompareMem ((VOID *)StoredUuid, (VOID *)BuildUuid, sizeof (BuildUuid)) != 0) {
    DEBUG ((DEBUG_INFO, "BUILD UUID Changed, Update Storage with NVRAM FV\n"));
    ClearUserConfig = TRUE;
  }

  if (ClearUserConfig) {
    Status = FlashEraseCommand (FWNvRamStartOffset, NvRamSize * 2 + sizeof (BuildUuid));
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = FlashWriteCommand (
               FWNvRamStartOffset,
               (UINT8 *)NvRamAddress,
               NvRamSize
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Write new BUILD UUID to the Flash
    //
    Status = FlashEraseCommand (FWNvRamStartOffset + (NvRamSize * 2), sizeof (BuildUuid));
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = FlashWriteCommand (
               FWNvRamStartOffset + NvRamSize * 2,
               (UINT8 *)BuildUuid,
               sizeof (BuildUuid)
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = NVParamClrAll ();
    if (!EFI_ERROR (Status)) {
      //
      // Trigger reset to use default NVPARAM
      //
      ResetCold ();
    }
  } else {
    DEBUG ((DEBUG_INFO, "Identical UUID, copy stored NVRAM to RAM\n"));

    Status = FlashReadCommand (
               FWNvRamStartOffset,
               (UINT8 *)NvRamAddress,
               NvRamSize
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}
