/** @file

  Copyright (c) 2025, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Guid/EventGroup.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/FlashLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>

#define UEFI_EXTRA_RUNTIME_VARIABLE_OFFSET  L"UefiExtraOffset"
#define UEFI_EXTRA_RUNTIME_VARIABLE_SIZE    L"UefiExtraSize"
#define UEFI_EXTRA_RUNTIME_VARIABLE_VALUE   L"UefiExtraValue"
#define UEFI_EXTRA_RUNTIME_VARIABLE_ERASE   L"UefiExtraErase"
#define UEFI_EXTRA_FLASH_START              PcdGet32 (PcdUefiExtraFdSize) + PcdGet64 (PcdUefiExtraFdBaseAddress)
#define UEFI_EXTRA_FLASH_SIZE               PcdGet32 (PcdUefiExtraFlashSize)
#define UEFI_EXTRA_FLASH_BLOCK_SIZE         PcdGet32 (PcdFvBlockSize)
#define UEFI_EXTRA_ERASE_PATTERN            0xCAFEBEEFCAFEBEEF

static EFI_GET_NEXT_VARIABLE_NAME  mOriginGetNextVariableName;
static EFI_GET_VARIABLE            mOriginGetVariable;
static EFI_SET_VARIABLE            mOriginSetVariable;
static EFI_EVENT                   mVirtualAddressChangeEvent = NULL;
static UINTN                       mUefiExtraOffset           = 0x00;
static UINTN                       mUefiExtraSize             = 0x00;
static UINTN                       mCurrentIndex;
static BOOLEAN                     mVariableEnumerationDone = FALSE;

static CHAR16  *mUefiExtraVariables[] = {
  UEFI_EXTRA_RUNTIME_VARIABLE_OFFSET,
  UEFI_EXTRA_RUNTIME_VARIABLE_SIZE,
  UEFI_EXTRA_RUNTIME_VARIABLE_VALUE,
  UEFI_EXTRA_RUNTIME_VARIABLE_ERASE
};

/**
  Handles GetNextVariableName runtime service calls.

  @param  VariableNameSize    Size of the variable name.
  @param  VariableName        Pointer to variable name which is a 8-byte NVP signature.
  @param  VendorGuid          Variable Vendor Guid.

  @retval EFI_INVALID_PARAMETER      Invalid parameter.
  @retval EFI_SUCCESS                Find the specified variable.
  @retval EFI_NOT_FOUND              Not found.
  @retval EFI_BUFFER_TO_SMALL        DataSize is too small for the result.

**/
EFI_STATUS
UefiExtraRuntimeGetNextVariableName (
  IN OUT  UINTN     *VariableNameSize,
  IN OUT  CHAR16    *VariableName,
  IN OUT  EFI_GUID  *VendorGuid
  )
{
  UINTN       MaxLen;
  EFI_STATUS  Status;

  if ((VariableNameSize == NULL) || (VariableName == NULL) || (VendorGuid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Calculate the possible maximum length of name string, including the Null terminator.
  //
  MaxLen = *VariableNameSize / sizeof (CHAR16);
  if ((MaxLen == 0) || (StrnLenS (VariableName, MaxLen) == MaxLen)) {
    //
    // Null-terminator is not found in the first VariableNameSize bytes of the input VariableName buffer,
    // follow spec to return EFI_INVALID_PARAMETER.
    //
    return EFI_INVALID_PARAMETER;
  }

  if ((VariableName[0] == 0) && !mVariableEnumerationDone) {
    //
    // Start to provide custom variable
    //
    mCurrentIndex = 0;
  }

  if (mVariableEnumerationDone) {
    goto Exit;
  }

  if (mCurrentIndex == (sizeof (mUefiExtraVariables) / sizeof (mUefiExtraVariables[0]))) {
    //
    // No more variable, restart the search for standard variables
    //
    VariableName[0]          = 0;
    mVariableEnumerationDone = TRUE;
    goto Exit;
  }

  if (*VariableNameSize < StrSize (mUefiExtraVariables[mCurrentIndex])) {
    // Input size is too small need a bigger buffer
    *VariableNameSize = StrSize (mUefiExtraVariables[mCurrentIndex]);
    return EFI_BUFFER_TOO_SMALL;
  }

  *VariableNameSize = StrSize (mUefiExtraVariables[mCurrentIndex]);
  StrCpyS (VariableName, *VariableNameSize, mUefiExtraVariables[mCurrentIndex]);
  CopyGuid (VendorGuid, &gAmpereUefiExtraRuntimeGuid);
  mCurrentIndex++;

  return EFI_SUCCESS;

Exit:
  Status = mOriginGetNextVariableName (VariableNameSize, VariableName, VendorGuid);

  if (Status == EFI_NOT_FOUND) {
    mVariableEnumerationDone = FALSE;
  }

  return Status;
}

/**
  Handles GetVariable runtime service calls.

  This function will filter action get variable with VendorGuid is
  gAmpereUefiExtraRuntimeGuid and redirect to firmware update MM communicate.
  Otherwise, call UEFI Runtime Service GetVariable().

  @param  VariableName The name of the vendor's variable, it's a Null-Terminated Unicode String
  @param  VendorGuid   A unique identifier for the vendor.
  @param  Attributes   Pointer to memory location to return the attributes of variable. If the pointer
                       is NULL, the parameter would be ignored.
  @param  DataSize     As input, pointer to the maximum size of return Data-Buffer.
                       As output, pointer to the actual size of the returned Data-Buffer.
  @param  Data         Pointer to return Data-Buffer.

  @retval EFI_SUCCESS               The function completed successfully.
  @retval EFI_NOT_FOUND             The variable was not found.
  @retval EFI_BUFFER_TOO_SMALL      The DataSize is too small for the result.
  @retval EFI_INVALID_PARAMETER     VariableName is NULL.
  @retval EFI_INVALID_PARAMETER     VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER     DataSize is NULL.
  @retval EFI_INVALID_PARAMETER     The DataSize is not too small and Data is NULL.
  @retval EFI_DEVICE_ERROR          The variable could not be retrieved due to a hardware error.
  @retval EFI_SECURITY_VIOLATION    The variable could not be retrieved due to an authentication failure.
  @retval EFI_UNSUPPORTED           After ExitBootServices() has been called, this return code may be returned
                                    if no variable storage is supported. The platform should describe this
                                    runtime service as unsupported at runtime via an EFI_RT_PROPERTIES_TABLE
                                    configuration table.
**/
EFI_STATUS
UefiExtraRuntimeGetVariable (
  IN     CHAR16    *VariableName,
  IN     EFI_GUID  *VendorGuid,
  OUT    UINT32    *Attributes,
  IN OUT UINTN     *DataSize,
  OUT    VOID      *Data
  )
{
  EFI_STATUS  Status;
  UINTN       Value;

  if ((VendorGuid == NULL) || (!CompareGuid (VendorGuid, &gAmpereUefiExtraRuntimeGuid))) {
    return mOriginGetVariable (VariableName, VendorGuid, Attributes, DataSize, Data);
  }

  if ((VariableName == NULL) || (DataSize == NULL) ||
      ((StrCmp (VariableName, UEFI_EXTRA_RUNTIME_VARIABLE_OFFSET) != 0) &&
       (StrCmp (VariableName, UEFI_EXTRA_RUNTIME_VARIABLE_SIZE) != 0) &&
       (StrCmp (VariableName, UEFI_EXTRA_RUNTIME_VARIABLE_ERASE) != 0) &&
       (StrCmp (VariableName, UEFI_EXTRA_RUNTIME_VARIABLE_VALUE) != 0)))
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Fill in attributes if not null.
  //
  if (Attributes != NULL) {
    *Attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;
  }

  if (StrCmp (VariableName, UEFI_EXTRA_RUNTIME_VARIABLE_VALUE) == 0) {
    if ((mUefiExtraSize == 0) ||
        (mUefiExtraOffset + mUefiExtraSize > UEFI_EXTRA_FLASH_SIZE) ||
        (mUefiExtraSize > UEFI_EXTRA_FLASH_BLOCK_SIZE))
    {
      return EFI_INVALID_PARAMETER;
    }

    if (*DataSize < mUefiExtraSize) {
      *DataSize = mUefiExtraSize;
      return EFI_BUFFER_TOO_SMALL;
    }

    if (Data == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    Status = FlashReadCommand (
               UEFI_EXTRA_FLASH_START + mUefiExtraOffset,
               Data,
               *DataSize
               );
    return Status;
  }

  if (*DataSize < sizeof (Value)) {
    *DataSize = sizeof (Value);
    return EFI_BUFFER_TOO_SMALL;
  }

  if (StrCmp (VariableName, UEFI_EXTRA_RUNTIME_VARIABLE_OFFSET) == 0) {
    Value = mUefiExtraOffset;
  }

  if (StrCmp (VariableName, UEFI_EXTRA_RUNTIME_VARIABLE_SIZE) == 0) {
    Value = mUefiExtraSize;
  }

  CopyMem ((VOID *)Data, (VOID *)&Value, sizeof (Value));

  return EFI_SUCCESS;
}

/**
  Handles SetVariable runtime service calls.

  This function will filter action set variable with VendorGuid is
  gAmpereUefiExtraRuntimeGuid and redirect to firmware update MM communicate.
  Otherwise, call UEFI Runtime Service SetVariable().

  @param  VariableName The name of the vendor's variable, as a
                       Null-Terminated Unicode String
  @param  VendorGuid   A unique identifier for the vendor.
  @param  Attributes   Pointer to memory location to return the attributes of variable. If the pointer
                       is NULL, the parameter would be ignored.
  @param  DataSize     The size in bytes of Data-Buffer.
  @param  Data         Pointer to the content of the variable.

  @retval EFI_SUCCESS                     The function completed successfully.
  @retval EFI_NOT_FOUND                   The variable was not found.
  @retval EFI_BUFFER_TOO_SMALL            The DataSize is too small for the result.
  @retval EFI_INVALID_PARAMETER           VariableName is NULL.
  @retval EFI_INVALID_PARAMETER           VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER           DataSize is NULL.
  @retval EFI_INVALID_PARAMETER           The DataSize is not too small and Data is NULL.
  @retval EFI_DEVICE_ERROR                The variable could not be retrieved due to a hardware error.
  @retval EFI_SECURITY_VIOLATION          The variable could not be retrieved due to an authentication failure.
  @retval EFI_UNSUPPORTED                 After ExitBootServices() has been called, this return code may be returned
                                          if no variable storage is supported. The platform should describe this
                                          runtime service as unsupported at runtime via an EFI_RT_PROPERTIES_TABLE
                                          configuration table.

**/
EFI_STATUS
UefiExtraRuntimeSetVariable (
  IN  CHAR16    *VariableName,
  IN  EFI_GUID  *VendorGuid,
  IN  UINT32    Attributes,
  IN  UINTN     DataSize,
  IN  VOID      *Data
  )
{
  EFI_STATUS  Status;
  UINTN       ErasePattern;

  ErasePattern = UEFI_EXTRA_ERASE_PATTERN;

  Status = EFI_INVALID_PARAMETER;

  if ((VendorGuid == NULL) || (!CompareGuid (VendorGuid, &gAmpereUefiExtraRuntimeGuid))) {
    return mOriginSetVariable (VariableName, VendorGuid, Attributes, DataSize, Data);
  }

  if ((VariableName == NULL) ||
      ((StrCmp (VariableName, UEFI_EXTRA_RUNTIME_VARIABLE_OFFSET) != 0) &&
       (StrCmp (VariableName, UEFI_EXTRA_RUNTIME_VARIABLE_SIZE) != 0) &&
       (StrCmp (VariableName, UEFI_EXTRA_RUNTIME_VARIABLE_ERASE) != 0) &&
       (StrCmp (VariableName, UEFI_EXTRA_RUNTIME_VARIABLE_VALUE) != 0)))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (StrCmp (VariableName, UEFI_EXTRA_RUNTIME_VARIABLE_OFFSET) == 0) {
    mUefiExtraOffset = *((UINTN *)Data);
    return EFI_SUCCESS;
  }

  if (StrCmp (VariableName, UEFI_EXTRA_RUNTIME_VARIABLE_SIZE) == 0) {
    mUefiExtraSize = *((UINTN *)Data);
    return EFI_SUCCESS;
  }

  if (StrCmp (VariableName, UEFI_EXTRA_RUNTIME_VARIABLE_ERASE) == 0) {
    if ((mUefiExtraOffset + UEFI_EXTRA_FLASH_BLOCK_SIZE > UEFI_EXTRA_FLASH_SIZE)) {
      return EFI_INVALID_PARAMETER;
    }

    if ((mUefiExtraOffset % UEFI_EXTRA_FLASH_BLOCK_SIZE) != 0) {
      return EFI_INVALID_PARAMETER;
    }

    if ((DataSize != sizeof (ErasePattern)) &&
        (*((UINTN *)Data) != ErasePattern))
    {
      return EFI_INVALID_PARAMETER;
    }

    Status = FlashEraseCommand (
               UEFI_EXTRA_FLASH_START + mUefiExtraOffset,
               UEFI_EXTRA_FLASH_BLOCK_SIZE
               );
    goto Exit;
  }

  if ((DataSize > UEFI_EXTRA_FLASH_BLOCK_SIZE) ||
      (DataSize + mUefiExtraOffset > UEFI_EXTRA_FLASH_SIZE))
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = FlashWriteCommand (
             UEFI_EXTRA_FLASH_START + mUefiExtraOffset,
             Data,
             DataSize
             );

Exit:
  mUefiExtraSize   = 0;
  mUefiExtraOffset = 0;

  return Status;
}

/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.
  It convers pointer to new virtual address.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context.
**/
static
VOID
EFIAPI
UefiExtraRuntimeVirtualAddressChangeEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EfiConvertPointer (0x0, (VOID **)&mOriginGetNextVariableName);
  EfiConvertPointer (0x0, (VOID **)&mOriginGetVariable);
  EfiConvertPointer (0x0, (VOID **)&mOriginSetVariable);
}

/**
  Override a pointer in the EFI System Table.

  @param[in,out]  AddressToUpdate     Pointer to Runtime Service function is
                                      being to override
  @param[in]      NewPointer          Pointer to the new Runtime Service function
  @param[in,out]  OriginalPointer     Pointer to the backup version of Runtime
                                      Service function
**/
static
EFI_STATUS
UefiExtraRuntimeOverridePointer (
  IN OUT VOID  **AddressToUpdate,
  IN VOID      *NewPointer,
  OUT VOID     **OriginalPointer OPTIONAL
  )
{
  EFI_STATUS  Status;
  EFI_TPL     Tpl;

  ASSERT (*AddressToUpdate != NewPointer);

  Tpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  //
  // Save the current value if needed and update the pointer.
  //
  if (OriginalPointer != NULL) {
    *OriginalPointer = *AddressToUpdate;
  }

  *AddressToUpdate = NewPointer;

  //
  // Update the CRC32 in the EFI System Table header.
  //
  gST->Hdr.CRC32 = 0;
  Status         = gBS->CalculateCrc32 (&gST->Hdr, gST->Hdr.HeaderSize, &gST->Hdr.CRC32);
  ASSERT_EFI_ERROR (Status);

  gBS->RestoreTPL (Tpl);
  return Status;
}

/**
  Cleans up changes made by this module and release resources.

**/
static
VOID
UefiExtraRuntimeCleanup (
  VOID
  )
{
  EFI_STATUS  Status;

  //
  // Close the End of DXE event.
  //
  if (mVirtualAddressChangeEvent != NULL) {
    Status = gBS->CloseEvent (mVirtualAddressChangeEvent);
    ASSERT_EFI_ERROR (Status);
    mVirtualAddressChangeEvent = NULL;
  }

  if (gRT->SetVariable == UefiExtraRuntimeSetVariable) {
    Status = UefiExtraRuntimeOverridePointer (
               (VOID **)&gRT->SetVariable,
               (VOID *)mOriginSetVariable,
               NULL
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (gRT->GetVariable == UefiExtraRuntimeGetVariable) {
    Status = UefiExtraRuntimeOverridePointer (
               (VOID **)&gRT->GetVariable,
               (VOID *)mOriginGetVariable,
               NULL
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (gRT->GetNextVariableName == UefiExtraRuntimeGetNextVariableName) {
    Status = UefiExtraRuntimeOverridePointer (
               (VOID **)&gRT->GetNextVariableName,
               (VOID *)mOriginGetNextVariableName,
               NULL
               );
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  The driver entry point for the UefiExtra Runtime Access.

  @param[in]  ImageHandle   The firmware allocated handle for the EFI image.
  @param[in]  SystemTable   A pointer to the EFI System Table.

  @retval     EFI_SUCCESS          The function completes successfully
**/
EFI_STATUS
EFIAPI
UefiExtraRuntimeDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  if ((UEFI_EXTRA_FLASH_SIZE == 0) ||
     ((UEFI_EXTRA_FLASH_SIZE % UEFI_EXTRA_FLASH_BLOCK_SIZE) != 0))
  {
    DEBUG ((DEBUG_ERROR, "Uefi Extra flash size is invalid\n"));
    return EFI_UNSUPPORTED;
  }

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  UefiExtraRuntimeVirtualAddressChangeEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mVirtualAddressChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Install hooks.
  //
  Status = UefiExtraRuntimeOverridePointer (
             (VOID **)&gRT->GetNextVariableName,
             (VOID *)UefiExtraRuntimeGetNextVariableName,
             (VOID **)&mOriginGetNextVariableName
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OverridePointer(GetNextVariableName) failed : %r\n", Status));
    goto Exit;
  }

  Status = UefiExtraRuntimeOverridePointer (
             (VOID **)&gRT->GetVariable,
             (VOID *)UefiExtraRuntimeGetVariable,
             (VOID **)&mOriginGetVariable
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OverrideRuntimeService(GetVariable) failed : %r\n", Status));
    goto Exit;
  }

  Status = UefiExtraRuntimeOverridePointer (
             (VOID **)&gRT->SetVariable,
             (VOID *)UefiExtraRuntimeSetVariable,
             (VOID **)&mOriginSetVariable
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OverrideRuntimeService(SetVariable) failed : %r\n", Status));
    goto Exit;
  }

Exit:
  if (EFI_ERROR (Status)) {
    UefiExtraRuntimeCleanup ();
  }

  return Status;
}
