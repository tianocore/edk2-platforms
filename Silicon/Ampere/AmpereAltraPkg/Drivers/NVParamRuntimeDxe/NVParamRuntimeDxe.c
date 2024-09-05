/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Guid/EventGroup.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/NVParamLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <NVParamDef.h>

#define NVPARAM_RUNTIME_VARIABLE_OFFSET  L"NVParamOffset"
#define NVPARAM_RUNTIME_VARIABLE_VALUE   L"NVParamValue"
#define INVALID_NVPARAM_VALUE            0xFFFFFFFF

static EFI_GET_NEXT_VARIABLE_NAME  mOriginGetNextVariableName;
static EFI_GET_VARIABLE            mOriginGetVariable;
static EFI_SET_VARIABLE            mOriginSetVariable;
static EFI_EVENT                   mVirtualAddressChangeEvent = NULL;
static UINTN                       mOffset                    = INVALID_NVPARAM_VALUE;
static UINTN                       mCurrentNVParam;
static BOOLEAN                     mNVParamEnumerationDone = FALSE;

static CHAR16  *mNVParamVariables[] = {
  NVPARAM_RUNTIME_VARIABLE_OFFSET,
  NVPARAM_RUNTIME_VARIABLE_VALUE
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
NVParamRuntimeGetNextVariableName (
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

  if ((VariableName[0] == 0) && !mNVParamEnumerationDone) {
    //
    // Start to provide custom variable
    //
    mCurrentNVParam = 0;
  }

  if (mNVParamEnumerationDone) {
    goto Exit;
  }

  if (mCurrentNVParam == (sizeof (mNVParamVariables) / sizeof (mNVParamVariables[0]))) {
    //
    // No more variable, restart the search for standard variables
    //
    VariableName[0]         = 0;
    mNVParamEnumerationDone = TRUE;
    goto Exit;
  }

  if (*VariableNameSize < StrSize (mNVParamVariables[mCurrentNVParam])) {
    // Input size is too small need a bigger buffer
    *VariableNameSize = StrSize (mNVParamVariables[mCurrentNVParam]);
    return EFI_BUFFER_TOO_SMALL;
  }

  *VariableNameSize = StrSize (mNVParamVariables[mCurrentNVParam]);
  StrCpyS (VariableName, *VariableNameSize, mNVParamVariables[mCurrentNVParam]);
  CopyGuid (VendorGuid, &gAmpereNVParamRuntimeGuid);
  mCurrentNVParam++;

  return EFI_SUCCESS;

Exit:
  Status = mOriginGetNextVariableName (VariableNameSize, VariableName, VendorGuid);

  if (Status == EFI_NOT_FOUND) {
    mNVParamEnumerationDone = FALSE;
  }

  return Status;
}

/**
  Handles GetVariable runtime service calls.

  This function will filter action get variable with VendorGuid is
  gAmpereNVParamRuntimeGuid and redirect to firmware update MM communicate.
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
NVParamRuntimeGetVariable (
  IN     CHAR16    *VariableName,
  IN     EFI_GUID  *VendorGuid,
  OUT    UINT32    *Attributes,
  IN OUT UINTN     *DataSize,
  OUT    VOID      *Data
  )
{
  EFI_STATUS  Status;
  UINT32      Value;
  UINTN       Size;

  if ((VendorGuid == NULL) || (!CompareGuid (VendorGuid, &gAmpereNVParamRuntimeGuid))) {
    return mOriginGetVariable (VariableName, VendorGuid, Attributes, DataSize, Data);
  }

  if ((VariableName == NULL) || (DataSize == NULL) ||
      ((StrCmp (VariableName, NVPARAM_RUNTIME_VARIABLE_OFFSET) != 0) &&
       (StrCmp (VariableName, NVPARAM_RUNTIME_VARIABLE_VALUE) != 0)))
  {
    return EFI_INVALID_PARAMETER;
  }

  Size = sizeof (Value);
  if (*DataSize < Size) {
    *DataSize = Size;
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Fill in attributes if not null.
  //
  if (Attributes != NULL) {
    *Attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;
  }

  if (StrCmp (VariableName, NVPARAM_RUNTIME_VARIABLE_OFFSET) == 0) {
    Value = mOffset;
    goto Exit;
  }

  if (mOffset == INVALID_NVPARAM_VALUE) {
    return EFI_INVALID_PARAMETER;
  }

  Status = NVParamGet (
             (UINT32)mOffset,
             NV_PERM_ATF | NV_PERM_BIOS |
             NV_PERM_MANU | NV_PERM_BMC,
             &Value
             );

  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    return Status;
  }

  if (Status == EFI_NOT_FOUND) {
    Value = INVALID_NVPARAM_VALUE;
  }

Exit:
  CopyMem ((VOID *)Data, (VOID *)&Value, sizeof (Value));

  return EFI_SUCCESS;
}

/**
  Handles SetVariable runtime service calls.

  This function will filter action set variable with VendorGuid is
  gAmpereNVParamRuntimeGuid and redirect to firmware update MM communicate.
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
NVParamRuntimeSetVariable (
  IN  CHAR16    *VariableName,
  IN  EFI_GUID  *VendorGuid,
  IN  UINT32    Attributes,
  IN  UINTN     DataSize,
  IN  VOID      *Data
  )
{
  EFI_STATUS  Status;

  Status = EFI_INVALID_PARAMETER;

  if ((VendorGuid == NULL) || (!CompareGuid (VendorGuid, &gAmpereNVParamRuntimeGuid))) {
    return mOriginSetVariable (VariableName, VendorGuid, Attributes, DataSize, Data);
  }

  if ((VariableName == NULL) ||
      ((StrCmp (VariableName, NVPARAM_RUNTIME_VARIABLE_OFFSET) != 0) &&
       (StrCmp (VariableName, NVPARAM_RUNTIME_VARIABLE_VALUE) != 0)))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((DataSize == 0) || (DataSize != sizeof (UINT32))) {
    return EFI_INVALID_PARAMETER;
  }

  if (StrCmp (VariableName, NVPARAM_RUNTIME_VARIABLE_OFFSET) == 0) {
    mOffset = *((UINT32 *)Data);
    return EFI_SUCCESS;
  }

  if (StrCmp (VariableName, NVPARAM_RUNTIME_VARIABLE_VALUE) == 0) {
    if (mOffset == INVALID_NVPARAM_VALUE) {
      return EFI_INVALID_PARAMETER;
    }

    Status = NVParamSet (
               (UINT32)mOffset,
               NV_PERM_ATF | NV_PERM_BIOS |
               NV_PERM_MANU |NV_PERM_BMC,
               NV_PERM_BIOS | NV_PERM_MANU,
               *((UINT32 *)Data)
               );

    if (!EFI_ERROR (Status)) {
      mOffset = INVALID_NVPARAM_VALUE;
    }
  }

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
NVParamRuntimeVirtualAddressChangeEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EfiConvertPointer (0x0, (VOID **)&mOriginGetNextVariableName);
  EfiConvertPointer (0x0, (VOID **)&mOriginGetVariable);
  EfiConvertPointer (0x0, (VOID **)&mOriginSetVariable);

  EfiConvertPointer (0x0, (VOID **)&mOffset);
  EfiConvertPointer (0x0, (VOID **)&mCurrentNVParam);
  EfiConvertPointer (0x0, (VOID **)&mNVParamEnumerationDone);
}

/**
  Override a pointer in the EFI System Table.

**/
static
EFI_STATUS
NVParamRuntimeOverridePointer (
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
NVParamRuntimeCleanup (
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

  if (gRT->SetVariable == NVParamRuntimeSetVariable) {
    Status = NVParamRuntimeOverridePointer (
               (VOID **)&gRT->SetVariable,
               (VOID *)mOriginSetVariable,
               NULL
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (gRT->GetVariable == NVParamRuntimeGetVariable) {
    Status = NVParamRuntimeOverridePointer (
               (VOID **)&gRT->GetVariable,
               (VOID *)mOriginGetVariable,
               NULL
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (gRT->GetNextVariableName == NVParamRuntimeGetNextVariableName) {
    Status = NVParamRuntimeOverridePointer (
               (VOID **)&gRT->GetNextVariableName,
               (VOID *)mOriginGetNextVariableName,
               NULL
               );
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  The driver entry point for the NVPARAM Runtime Access.

  @param[in]  ImageHandle   The firmware allocated handle for the EFI image.
  @param[in]  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor successfully .
**/
EFI_STATUS
EFIAPI
NVParamRuntimeDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  NVParamRuntimeVirtualAddressChangeEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mVirtualAddressChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Install hooks.
  //
  Status = NVParamRuntimeOverridePointer (
             (VOID **)&gRT->GetNextVariableName,
             (VOID *)NVParamRuntimeGetNextVariableName,
             (VOID **)&mOriginGetNextVariableName
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OverridePointer(GetNextVariableName) failed : %r\n", Status));
    goto Exit;
  }

  Status = NVParamRuntimeOverridePointer (
             (VOID **)&gRT->GetVariable,
             (VOID *)NVParamRuntimeGetVariable,
             (VOID **)&mOriginGetVariable
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OverrideRuntimeService(GetVariable) failed : %r\n", Status));
    goto Exit;
  }

  Status = NVParamRuntimeOverridePointer (
             (VOID **)&gRT->SetVariable,
             (VOID *)NVParamRuntimeSetVariable,
             (VOID **)&mOriginSetVariable
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OverrideRuntimeService(SetVariable) failed : %r\n", Status));
    goto Exit;
  }

Exit:
  if (EFI_ERROR (Status)) {
    NVParamRuntimeCleanup ();
  }

  return Status;
}
