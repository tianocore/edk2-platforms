/** @file
  This driver init default Secure Boot variables.

  Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2018 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2021, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2021, Semihalf All rights reserved.<BR>
  Copyright (c) 2021, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (C) 2023 - 2026 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Guid/ImageAuthentication.h>

CHAR16  mPkDefaultVariableName[]  = EFI_PK_DEFAULT_VARIABLE_NAME;
CHAR16  mKekDefaultVariableName[] = EFI_KEK_DEFAULT_VARIABLE_NAME;
CHAR16  mDbDefaultVariableName[]  = EFI_DB_DEFAULT_VARIABLE_NAME;
CHAR16  mDbxDefaultVariableName[] = EFI_DBX_DEFAULT_VARIABLE_NAME;

//
// Microsoft signature owner GUID. The WS25 HLK Secure Boot tests require that
// Microsoft-provided KEK and db certificates are enrolled with this
// SignatureOwner GUID (77FA9ABD-0359-4D32-BD60-28F4E78F784B); a non-Microsoft
// owner (e.g. gEfiGlobalVariableGuid) is rejected as "Non-Microsoft
// signatureOwner". Matches gMicrosoftVendorGuid used by SecureBootInitDxe.
//
EFI_GUID  mMicrosoftVendorGuid = {
  0x77FA9ABD, 0x0359, 0x4D32, { 0xBD, 0x60, 0x28, 0xF4, 0xE7, 0x8F, 0x78, 0x4B }
};

/**
  Set PKDefault Variable.

  @param[in] X509Data              X509 Certificate data.
  @param[in] X509DataSize          X509 Certificate data size.

  @retval   EFI_SUCCESS            PKDefault is set successfully.

**/
EFI_STATUS
SetPkDefault (
  IN UINT8  *X509Data,
  IN UINTN  X509DataSize
  )
{
  EFI_STATUS          Status;
  UINT32              Attr;
  UINTN               DataSize;
  EFI_SIGNATURE_LIST  *PkCert;
  EFI_SIGNATURE_DATA  *PkCertData;

  PkCert = NULL;

  //
  // Allocate space for PK certificate list and initialize it.
  // Create PK database entry with SignatureHeaderSize equals 0.
  //
  PkCert = (EFI_SIGNATURE_LIST *)AllocateZeroPool (
                                   sizeof (EFI_SIGNATURE_LIST) + sizeof (EFI_SIGNATURE_DATA) - 1
                                   + X509DataSize
                                   );
  if (PkCert == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "%a: Cannot initialize PKDefault: %r\n", __func__, Status));
    goto ON_EXIT;
  }

  PkCert->SignatureListSize = (UINT32)(sizeof (EFI_SIGNATURE_LIST)
                                       + sizeof (EFI_SIGNATURE_DATA) - 1
                                       + X509DataSize);
  PkCert->SignatureSize       = (UINT32)(sizeof (EFI_SIGNATURE_DATA) - 1 + X509DataSize);
  PkCert->SignatureHeaderSize = 0;
  CopyGuid (&PkCert->SignatureType, &gEfiCertX509Guid);
  PkCertData = (EFI_SIGNATURE_DATA *)((UINTN)PkCert
                                      + sizeof (EFI_SIGNATURE_LIST)
                                      + PkCert->SignatureHeaderSize);
  CopyGuid (&PkCertData->SignatureOwner, &gEfiGlobalVariableGuid);
  //
  // Fill the PK database with PKpub data from X509 certificate file.
  //
  CopyMem (&(PkCertData->SignatureData[0]), X509Data, X509DataSize);

  Attr     = EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS;
  DataSize = PkCert->SignatureListSize;

  Status = gRT->SetVariable (
                  mPkDefaultVariableName,
                  &gEfiGlobalVariableGuid,
                  Attr,
                  DataSize,
                  PkCert
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Cannot initialize PKDefault: %r\n", __func__, Status));
    goto ON_EXIT;
  }

ON_EXIT:

  if (PkCert != NULL) {
    FreePool (PkCert);
  }

  return Status;
}

/**
  Set KDKDefault Variable.

  @param[in] X509Data              X509 Certificate data.
  @param[in] X509DataSize          X509 Certificate data size.

  @retval   EFI_SUCCESS            KEKDefault is set successfully.

**/
EFI_STATUS
SetKekDefault (
  IN UINT8  *X509Data,
  IN UINTN  X509DataSize
  )
{
  EFI_STATUS          Status;
  EFI_SIGNATURE_DATA  *KEKSigData;
  EFI_SIGNATURE_LIST  *KekSigList;
  UINTN               DataSize;
  UINTN               KekSigListSize;
  UINT32              Attr;

  KekSigList     = NULL;
  KekSigListSize = 0;
  DataSize       = 0;
  KEKSigData     = NULL;

  KekSigListSize = sizeof (EFI_SIGNATURE_LIST) + sizeof (EFI_SIGNATURE_DATA) - 1 + X509DataSize;
  KekSigList     = (EFI_SIGNATURE_LIST *)AllocateZeroPool (KekSigListSize);
  if (KekSigList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "%a: Cannot initialize KEKDefault: %r\n", __func__, Status));
    goto ON_EXIT;
  }

  //
  // Fill Certificate Database parameters.
  //
  KekSigList->SignatureListSize   = (UINT32)KekSigListSize;
  KekSigList->SignatureHeaderSize = 0;
  KekSigList->SignatureSize       = (UINT32)(sizeof (EFI_SIGNATURE_DATA) - 1 + X509DataSize);
  CopyGuid (&KekSigList->SignatureType, &gEfiCertX509Guid);

  KEKSigData = (EFI_SIGNATURE_DATA *)((UINT8 *)KekSigList + sizeof (EFI_SIGNATURE_LIST));
  CopyGuid (&KEKSigData->SignatureOwner, &mMicrosoftVendorGuid);
  CopyMem (KEKSigData->SignatureData, X509Data, X509DataSize);

  //
  // Check if KEK been already existed.
  // If true, use EFI_VARIABLE_APPEND_WRITE attribute to append the
  // new kek to original variable
  //
  Attr = EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS;

  Status = gRT->GetVariable (
                  mKekDefaultVariableName,
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &DataSize,
                  NULL
                  );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Attr |= EFI_VARIABLE_APPEND_WRITE;
  } else if (Status != EFI_NOT_FOUND) {
    DEBUG ((DEBUG_ERROR, "%a: Cannot get the value of KEK: %r\n", __func__, Status));
    goto ON_EXIT;
  }

  Status = gRT->SetVariable (
                  mKekDefaultVariableName,
                  &gEfiGlobalVariableGuid,
                  Attr,
                  KekSigListSize,
                  KekSigList
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Cannot initialize KEKDefault: %r\n", __func__, Status));
    goto ON_EXIT;
  }

ON_EXIT:

  if (KekSigList != NULL) {
    FreePool (KekSigList);
  }

  return Status;
}

/**
  Checks if the file content complies with EFI_VARIABLE_AUTHENTICATION_2 format

  @param[in] Data                  Data.
  @param[in] DataSize              Data size.

  @retval    TRUE                  The content is EFI_VARIABLE_AUTHENTICATION_2 format.
  @retval    FALSE                 The content is NOT a EFI_VARIABLE_AUTHENTICATION_2 format.

**/
BOOLEAN
IsAuthentication2Format (
  IN UINT8  *Data,
  IN UINTN  DataSize
  )
{
  EFI_VARIABLE_AUTHENTICATION_2  *Auth2;
  EFI_VARIABLE_AUTHENTICATION_2  Auth2Buffer;
  BOOLEAN                        IsAuth2Format;

  IsAuth2Format = FALSE;

  //
  // The data must be at least large enough to contain the
  // EFI_VARIABLE_AUTHENTICATION_2 header before its fields can be inspected.
  // A real signed dbx/db update (e.g. Microsoft DbxUpdate.bin) is always much
  // larger than the header; using "!=" here wrongly rejected every such update
  // and caused the whole signed blob to be stored as a bogus X509 entry,
  // producing a malformed dbxDefault that crashes the HLK Secure Boot parser.
  //
  if (DataSize < sizeof (EFI_VARIABLE_AUTHENTICATION_2)) {
    goto ON_EXIT;
  }

  CopyMem (&Auth2Buffer, Data, sizeof (EFI_VARIABLE_AUTHENTICATION_2));
  Auth2 = &Auth2Buffer;

  if (Auth2->AuthInfo.Hdr.wCertificateType != WIN_CERT_TYPE_EFI_GUID) {
    goto ON_EXIT;
  }

  if (CompareGuid (&gEfiCertPkcs7Guid, &Auth2->AuthInfo.CertType)) {
    IsAuth2Format = TRUE;
  }

ON_EXIT:

  return IsAuth2Format;
}

/**
  Set signature database with the data of EFI_VARIABLE_AUTHENTICATION_2 format.

  @param[in] AuthData              AUTHENTICATION_2 data.
  @param[in] AuthDataSize          AUTHENTICATION_2 data size.
  @param[in] VariableName          Variable name of signature database, must be
                                   EFI_DB_DEFAULT_VARIABLE_NAME or EFI_DBX_DEFAULT_VARIABLE_NAME or EFI_DBT_DEFAULT_VARIABLE_NAME.

  @retval   EFI_SUCCESS            New signature is set successfully.
  @retval   EFI_INVALID_PARAMETER  The parameter is invalid.
  @retval   EFI_UNSUPPORTED        Unsupported command.
  @retval   EFI_OUT_OF_RESOURCES   Could not allocate needed resources.

**/
EFI_STATUS
SetAuthentication2ToSigDb (
  IN UINT8   *AuthData,
  IN UINTN   AuthDataSize,
  IN CHAR16  *VariableName
  )
{
  EFI_STATUS                     Status;
  UINTN                          DataSize;
  UINT32                         Attr;
  UINT8                          *Data;
  EFI_VARIABLE_AUTHENTICATION_2  *Auth2;
  EFI_VARIABLE_AUTHENTICATION_2  Auth2Buffer;

  Attr = EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS;

  //
  // Check if SigDB variable has been already existed.
  // If true, use EFI_VARIABLE_APPEND_WRITE attribute to append the
  // new signature data to original variable
  //
  DataSize = 0;
  Status   = gRT->GetVariable (
                    VariableName,
                    &gEfiGlobalVariableGuid,
                    NULL,
                    &DataSize,
                    NULL
                    );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Attr |= EFI_VARIABLE_APPEND_WRITE;
  } else if (Status != EFI_NOT_FOUND) {
    DEBUG ((DEBUG_ERROR, "%a: Cannot get the value of signature database: %r\n", __func__, Status));
    return Status;
  }

  //
  // Ignore AUTHENTICATION_2 region. Only the actual certificate is needed.
  //
  CopyMem (&Auth2Buffer, AuthData, sizeof (EFI_VARIABLE_AUTHENTICATION_2));
  Auth2    = &Auth2Buffer;
  DataSize = AuthDataSize - Auth2->AuthInfo.Hdr.dwLength - sizeof (EFI_TIME);
  Data     = AuthData + (AuthDataSize - DataSize);

  Status = gRT->SetVariable (
                  VariableName,
                  &gEfiGlobalVariableGuid,
                  Attr,
                  DataSize,
                  Data
                  );

  DEBUG ((DEBUG_INFO, "Set AUTH_2 data to Var:%s Status: %x\n", VariableName, Status));
  return Status;
}

/**

  Set signature database with the data of X509 format.

  @param[in] X509Data              X509 Certificate data.
  @param[in] X509DataSize          X509 Certificate data size.
  @param[in] VariableName          Variable name of signature database, must be
                                   EFI_DB_DEFAULT_VARIABLE_NAME or EFI_DBX_DEFAULT_VARIABLE_NAME or EFI_DBT_DEFAULT_VARIABLE_NAME.
  @param[in] SignatureOwnerGuid    Guid of the signature owner.

  @retval   EFI_SUCCESS            New X509 is enrolled successfully.
  @retval   EFI_OUT_OF_RESOURCES   Could not allocate needed resources.

**/
EFI_STATUS
SetX509ToSigDb (
  IN UINT8     *X509Data,
  IN UINTN     X509DataSize,
  IN CHAR16    *VariableName,
  IN EFI_GUID  *SignatureOwnerGuid
  )
{
  EFI_STATUS          Status;
  EFI_SIGNATURE_LIST  *SigDBCert;
  EFI_SIGNATURE_DATA  *SigDBCertData;
  VOID                *Data;
  UINTN               DataSize;
  UINTN               SigDBSize;
  UINT32              Attr;

  SigDBSize     = 0;
  DataSize      = 0;
  SigDBCert     = NULL;
  SigDBCertData = NULL;
  Data          = NULL;

  SigDBSize = sizeof (EFI_SIGNATURE_LIST) + sizeof (EFI_SIGNATURE_DATA) - 1 + X509DataSize;

  Data = AllocateZeroPool (SigDBSize);
  if (Data == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "%a: Cannot allocate memory: %r\n", __func__, Status));
    goto ON_EXIT;
  }

  //
  // Fill Certificate Database parameters.
  //
  SigDBCert                      = (EFI_SIGNATURE_LIST *)Data;
  SigDBCert->SignatureListSize   = (UINT32)SigDBSize;
  SigDBCert->SignatureHeaderSize = 0;
  SigDBCert->SignatureSize       = (UINT32)(sizeof (EFI_SIGNATURE_DATA) - 1 + X509DataSize);
  CopyGuid (&SigDBCert->SignatureType, &gEfiCertX509Guid);

  SigDBCertData = (EFI_SIGNATURE_DATA *)((UINT8 *)SigDBCert + sizeof (EFI_SIGNATURE_LIST));
  CopyGuid (&SigDBCertData->SignatureOwner, SignatureOwnerGuid);
  CopyMem ((UINT8 *)(SigDBCertData->SignatureData), X509Data, X509DataSize);

  //
  // Check if signature database entry has been already existed.
  // If true, use EFI_VARIABLE_APPEND_WRITE attribute to append the
  // new signature data to original variable
  //
  Attr = EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS;

  Status = gRT->GetVariable (
                  VariableName,
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &DataSize,
                  NULL
                  );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Attr |= EFI_VARIABLE_APPEND_WRITE;
  } else if (Status != EFI_NOT_FOUND) {
    goto ON_EXIT;
  }

  Status = gRT->SetVariable (
                  VariableName,
                  &gEfiGlobalVariableGuid,
                  Attr,
                  SigDBSize,
                  Data
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Cannot set signature database: %r\n", __func__, Status));
    goto ON_EXIT;
  }

ON_EXIT:

  if (Data != NULL) {
    FreePool (Data);
  }

  return Status;
}

/**

  Set signature database.

  @param[in] Data                  Data.
  @param[in] DataSize              Data size.
  @param[in] VariableName          Variable name of signature database, must be
                                   EFI_DB_DEFAULT_VARIABLE_NAME or EFI_DBX_DEFAULT_VARIABLE_NAME or EFI_DBT_DEFAULT_VARIABLE_NAME.
  @param[in] SignatureOwnerGuid    Guid of the signature owner.

  @retval   EFI_SUCCESS            Signature is set successfully.
  @retval   EFI_OUT_OF_RESOURCES   Could not allocate needed resources.

**/
EFI_STATUS
SetSignatureDatabase (
  IN UINT8     *Data,
  IN UINTN     DataSize,
  IN CHAR16    *VariableName,
  IN EFI_GUID  *SignatureOwnerGuid
  )
{
  if (IsAuthentication2Format (Data, DataSize)) {
    return SetAuthentication2ToSigDb (Data, DataSize, VariableName);
  } else {
    return SetX509ToSigDb (Data, DataSize, VariableName, SignatureOwnerGuid);
  }
}

/** Initializes PKDefault variable with data from FFS section.

  @retval  EFI_SUCCESS           Variable was initialized successfully.
  @retval  EFI_UNSUPPORTED       Variable already exists.
**/
EFI_STATUS
InitPkDefault (
  IN VOID
  )
{
  EFI_STATUS  Status;
  UINT8       *Data;
  UINTN       DataSize;

  //
  // Check if variable exists, if so do not change it
  //
  Status = GetVariable2 (mPkDefaultVariableName, &gEfiGlobalVariableGuid, (VOID **)&Data, &DataSize);
  if (Status == EFI_SUCCESS) {
    DEBUG ((DEBUG_INFO, "Variable %s exists. Old value is preserved\n", mPkDefaultVariableName));
    FreePool (Data);
    return EFI_UNSUPPORTED;
  }

  //
  // Variable does not exist, can be initialized
  //
  DEBUG ((DEBUG_INFO, "Variable %s does not exist.\n", mPkDefaultVariableName));

  //
  // Enroll default PK.
  //
  Status = GetSectionFromFv (
             &gDefaultPKFileGuid,
             EFI_SECTION_RAW,
             0,
             (VOID **)&Data,
             &DataSize
             );
  if (!EFI_ERROR (Status)) {
    SetPkDefault (Data, DataSize);
  }

  return EFI_SUCCESS;
}

/** Initializes KEKDefault variable with data from FFS section.

  @retval  EFI_SUCCESS           Variable was initialized successfully.
  @retval  EFI_UNSUPPORTED       Variable already exists.
**/
EFI_STATUS
InitKekDefault (
  IN VOID
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  UINT8       *Data;
  UINTN       DataSize;

  //
  // Check if variable exists, if so do not change it
  //
  Status = GetVariable2 (mKekDefaultVariableName, &gEfiGlobalVariableGuid, (VOID **)&Data, &DataSize);
  if (Status == EFI_SUCCESS) {
    DEBUG ((DEBUG_INFO, "Variable %s exists. Old value is preserved\n", mKekDefaultVariableName));
    FreePool (Data);
    return EFI_UNSUPPORTED;
  }

  Index = 0;
  do {
    Status = GetSectionFromFv (
               &gDefaultKEKFileGuid,
               EFI_SECTION_RAW,
               Index,
               (VOID **)&Data,
               &DataSize
               );
    if (!EFI_ERROR (Status)) {
      SetKekDefault (Data, DataSize);
      Index++;
    }
  } while (Status == EFI_SUCCESS);

  return EFI_SUCCESS;
}

/** Initializes dbDefault variable with data from FFS section.

  @retval  EFI_SUCCESS           Variable was initialized successfully.
  @retval  EFI_UNSUPPORTED       Variable already exists.
**/
EFI_STATUS
InitDbDefault (
  IN VOID
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  UINT8       *Data;
  UINTN       DataSize;

  Status = GetVariable2 (mDbDefaultVariableName, &gEfiGlobalVariableGuid, (VOID **)&Data, &DataSize);
  if (Status == EFI_SUCCESS) {
    DEBUG ((DEBUG_INFO, "Variable %s exists. Old value is preserved\n", mDbDefaultVariableName));
    FreePool (Data);
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_INFO, "Variable %s does not exist.\n", mDbDefaultVariableName));

  Index = 0;
  do {
    Status = GetSectionFromFv (
               &gDefaultdbFileGuid,
               EFI_SECTION_RAW,
               Index,
               (VOID **)&Data,
               &DataSize
               );
    if (!EFI_ERROR (Status)) {
      SetSignatureDatabase (Data, DataSize, mDbDefaultVariableName, &mMicrosoftVendorGuid);
      Index++;
    }
  } while (Status == EFI_SUCCESS);

  return EFI_SUCCESS;
}

/** Initializes dbxDefault variable with data from FFS section.

  @retval  EFI_SUCCESS           Variable was initialized successfully.
  @retval  EFI_UNSUPPORTED       Variable already exists.
**/
EFI_STATUS
InitDbxDefault (
  IN VOID
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  UINT8       *Data;
  UINTN       DataSize;

  //
  // Check if variable exists, if so do not change it
  //
  Status = GetVariable2 (mDbxDefaultVariableName, &gEfiGlobalVariableGuid, (VOID **)&Data, &DataSize);
  if (Status == EFI_SUCCESS) {
    DEBUG ((DEBUG_INFO, "Variable %s exists. Old value is preserved\n", mDbxDefaultVariableName));
    FreePool (Data);
    return EFI_UNSUPPORTED;
  }

  //
  // Variable does not exist, can be initialized
  //
  DEBUG ((DEBUG_INFO, "Variable %s does not exist.\n", mDbxDefaultVariableName));

  Index = 0;
  do {
    Status = GetSectionFromFv (
               &gDefaultdbxFileGuid,
               EFI_SECTION_RAW,
               Index,
               (VOID **)&Data,
               &DataSize
               );
    if (!EFI_ERROR (Status)) {
      SetSignatureDatabase (Data, DataSize, mDbxDefaultVariableName, &gEfiGlobalVariableGuid);
      Index++;
    }
  } while (Status == EFI_SUCCESS);

  return EFI_SUCCESS;
}

/**
  Initializes default SecureBoot certificates with data from FFS section.

  @param[in] ImageHandle          The firmware allocated handle for the EFI image.
  @param[in] SystemTable           A pointer to the EFI System Table.

  @retval  EFI_SUCCESS           Variable was initialized successfully.
**/
EFI_STATUS
EFIAPI
SecureBootDefaultKeysInitEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = InitPkDefault ();
  if (EFI_ERROR (Status) && (Status != EFI_UNSUPPORTED)) {
    DEBUG ((DEBUG_ERROR, "%a: Cannot initialize PKDefault: %r\n", __func__, Status));
    return Status;
  }

  Status = InitKekDefault ();
  if (EFI_ERROR (Status) && (Status != EFI_UNSUPPORTED)) {
    DEBUG ((DEBUG_ERROR, "%a: Cannot initialize KEKDefault: %r\n", __func__, Status));
    return Status;
  }

  Status = InitDbDefault ();
  if (EFI_ERROR (Status) && (Status != EFI_UNSUPPORTED)) {
    DEBUG ((DEBUG_ERROR, "%a: Cannot initialize dbDefault: %r\n", __func__, Status));
    return Status;
  }

  Status = InitDbxDefault ();
  if (EFI_ERROR (Status) && (Status != EFI_UNSUPPORTED)) {
    DEBUG ((DEBUG_ERROR, "%a: Cannot initialize dbxDefault: %r\n", __func__, Status));
    return Status;
  }

  return EFI_SUCCESS;
}
