/** @file
  CRC32 Recovery Lib in PEI phase.

  Copyright (C) 2020 - 2025, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/ExtractGuidedSectionLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PcdLib.h>

EFI_PEI_PPI_DESCRIPTOR  mPpiListRecoveryBootMode = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiBootInRecoveryModePpiGuid,
  NULL
};

/**
  GetInfo gets raw data size and attribute of the input guided section.
  It first checks whether the input guid section is supported.
  If not, EFI_INVALID_PARAMETER will return.

  @param[in]  InputSection       Buffer containing the input GUIDed section to be processed.
  @param[out] OutputBufferSize   The size of OutputBuffer.
  @param[out] ScratchBufferSize  The size of ScratchBuffer.
  @param[out] SectionAttribute   The attribute of the input guided section.

  @retval EFI_SUCCESS            The size of destination buffer, the size of scratch buffer and
                                 the attribute of the input section are successfully retrieved.
  @retval EFI_INVALID_PARAMETER  The GUID in InputSection does not match this instance guid.

**/
EFI_STATUS
EFIAPI
Crc32GuidedSectionGetInfo (
  IN  CONST VOID  *InputSection,
  OUT UINT32      *OutputBufferSize,
  OUT UINT32      *ScratchBufferSize,
  OUT UINT16      *SectionAttribute
  );

/**
  Extraction handler tries to extract raw data from the input guided section.
  It also does authentication check for 32bit CRC value in the input guided section.
  It first checks whether the input guid section is supported.
  If not, EFI_INVALID_PARAMETER will return.

  @param[in]  InputSection          Buffer containing the input GUIDed section to be processed.
  @param[out] OutputBuffer          Buffer to contain the output raw data allocated by the caller.
  @param[in]  ScratchBuffer         A pointer to a caller-allocated buffer for function internal use.
  @param[out] AuthenticationStatus  A pointer to a caller-allocated UINT32 that indicates the
                                    authentication status of the output buffer.

  @retval EFI_SUCCESS            Section Data and Auth Status is extracted successfully.
  @retval EFI_INVALID_PARAMETER  The GUID in InputSection does not match this instance guid.

**/
EFI_STATUS
EFIAPI
Crc32GuidedSectionHandler (
  IN CONST  VOID    *InputSection,
  OUT       VOID    **OutputBuffer,
  IN        VOID    *ScratchBuffer         OPTIONAL,
  OUT       UINT32  *AuthenticationStatus
  );

/**
  This is a HOOK point before CRC32 Guided Section Handler called in EDK2.
  If platform detect checks fail, will install recovery PPI and set the boot mode
  as BOOT_IN_RECOVERY_MODE, to indicate platform needs to do recovery.

  @param[in]  InputSection          Buffer containing the input GUIDed section to be processed.
  @param[out] OutputBuffer          Buffer to contain the output raw data allocated by the caller.
  @param[in]  ScratchBuffer         A pointer to a caller-allocated buffer for function internal use.
  @param[out] AuthenticationStatus  A pointer to a caller-allocated UINT32 that indicates the
                                    authentication status of the output buffer.

  @retval EFI_SUCCESS            Section Data and Auth Status is extracted successfully.
  @retval EFI_INVALID_PARAMETER  The GUID in InputSection does not match this instance guid.

**/
EFI_STATUS
EFIAPI
Crc32RecoveryHandler (
  IN CONST  VOID    *InputSection,
  OUT       VOID    **OutputBuffer,
  IN        VOID    *ScratchBuffer         OPTIONAL,
  OUT       UINT32  *AuthenticationStatus
  )
{
  EFI_STATUS  Status;
  EFI_STATUS  Status1;
  UINTN       Size;

  Status = Crc32GuidedSectionHandler (InputSection, OutputBuffer, ScratchBuffer, AuthenticationStatus);
  if (EFI_ERROR (Status)) {
    // CRC32 error, set recovery mode.
    DEBUG ((DEBUG_INFO, "CRC32 checksum error, set bootmode to BOOT_IN_RECOVERY_MODE\n"));
    Status1 = PeiServicesInstallPpi (&mPpiListRecoveryBootMode);
    ASSERT_EFI_ERROR (Status1);
    Status1 = PeiServicesSetBootMode (BOOT_IN_RECOVERY_MODE);
    ASSERT_EFI_ERROR (Status1);

    // Disable TPM
    Size    = sizeof (gEfiTpmDeviceInstanceNoneGuid);
    Status1 = PcdSetPtrS (PcdTpmInstanceGuid, &Size, &gEfiTpmDeviceInstanceNoneGuid);
  }

  DEBUG ((DEBUG_INFO, "%a exit status = %r\n", __func__, Status));
  return Status;
}

/**
  Notification service to be called when gEfiCrc32GuidedSectionExtractionGuid is installed.

  @param[in]  PeiServices             Indirect reference to the PEI Services Table.
  @param[in]  NotifyDescriptor        Address of the notification descriptor data structure.
                                      Type EFI_PEI_NOTIFY_DESCRIPTOR is defined above.
  @param[in]  Ppi                     Address of the PPI that was installed.

  @retval   EFI_STATUS                This function will install a PPI to PPI database. The status
                                      code will be the code for (*PeiServices)->InstallPpi.
**/
EFI_STATUS
EFIAPI
NotifyOnCrc32GuidedSectionCallBack (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  DEBUG ((DEBUG_INFO, "%a: \n", __func__));
  //
  // Chain hook Crc32GuidedSectionHandler() to handle CRC32 error
  //
  return ExtractGuidedSectionRegisterHandlers (
           &gEfiCrc32GuidedSectionExtractionGuid,
           Crc32GuidedSectionGetInfo,
           Crc32RecoveryHandler
           );
}

EFI_PEI_NOTIFY_DESCRIPTOR  mNotifyOnCrc32GuidedSectionList = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiCrc32GuidedSectionExtractionGuid,
  NotifyOnCrc32GuidedSectionCallBack
};

/**
  Register the handler to chain Crc32GuidedSectionHandler to handle CRC32 checksum error.

  @param[in]  FileHandle   The handle of FFS header the loaded driver.
  @param[in]  PeiServices  The pointer to the PEI services.

  @retval  EFI_SUCCESS           Register successfully.
  @retval  EFI_OUT_OF_RESOURCES  Not enough memory to register this handler.

**/
EFI_STATUS
EFIAPI
PeiCrc32RecoveryLibConstructor (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS  Status;

  Status = (*PeiServices)->NotifyPpi (PeiServices, &mNotifyOnCrc32GuidedSectionList);
  return Status;
}
