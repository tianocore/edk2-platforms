/** @file
  Library to create SystemGuard NV Index consumed by OS.

  Copyright (C) 2022 - 2024, Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (C) Microsoft Corporation.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <IndustryStandard/Tpm20.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/Tpm2CommandLib.h>
#include <TpmSgNvIndexLib.h>

//
// Definition from Mircosoft "System Guard Secure Launch and SMM Protection"
// document, section "System requirements for System Guard", item "TPM NV Index"
//
STATIC CONST TPM2B_DIGEST  mSgAuthPolicyDigestBuffer = {
  SHA256_DIGEST_SIZE,
  {
    0xcb,            0x45,  0xc8, 0x1f, 0xf3, 0x4b, 0xcf, 0x0a,
    0xfb,            0x9e,  0x1a, 0x80, 0x29, 0xfa, 0x23, 0x1c,
    0x87,            0x27,  0x30, 0x3c, 0x09, 0x22, 0xdc, 0xce,
    0x68,            0x4b,  0xe3, 0xdb, 0x81, 0x7c, 0x20, 0xe1
  }
};

/**
  Executes DefineSpace for SystemGuard NV Index used by the OS, with
  requirements in Mircosoft "System Guard Secure Launch and SMM Protection",
  section "System requirements for System Guard", item "TPM NV Index"

  @retval     EFI_SUCCESS         NV Index was defined.
  @retval     EFI_ALREADY_STARTED NV Index has been defined already
  @retval     EFI_DEVICE_ERROR    Tpm2NvReadPublic() returned an unexpected error.
  @retval     Others              Tpm2NvDefineSpace() returned an error.
**/
EFI_STATUS
EFIAPI
DefineSgTpmNvIndexforOs (
  VOID
  )
{
  EFI_STATUS       Status;
  TPM2B_AUTH       NullAuth;
  TPM2B_NV_PUBLIC  NvData;
  TPM2B_NAME       PubName;

  DEBUG ((DEBUG_INFO, " %a() Entry...\n", __func__));

  //
  // First, we need to read whatever's there.
  //
  Status = Tpm2NvReadPublic (SG_NV_INDEX_HANDLE, &NvData, &PubName);
  // If an unexpected code is returned, we cannot handle that here.
  if ((Status != EFI_SUCCESS) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to read the index! %r\n", __func__, Status));
    Status = EFI_DEVICE_ERROR;
    goto Done;
  }

  if (Status == EFI_SUCCESS) {
    // Already defined, do nothing
    Status = EFI_ALREADY_STARTED;
    goto Done;
  }

  //
  // Initialize the auth.
  // For a NULL auth, all that matters is the size is 0.
  // NOTE: This assumes the platform hierarchy is unlocked and has NULL auth.
  //
  ZeroMem (&NullAuth, sizeof (NullAuth));

  //
  // Initialize the NV Index attribute data.
  //
  ZeroMem (&NvData, sizeof (NvData));
  NvData.nvPublic.nvIndex = SG_NV_INDEX_HANDLE;
  NvData.size            += sizeof (TPMI_RH_NV_INDEX);
  NvData.nvPublic.nameAlg = TPM_ALG_SHA256;                 // SHA-256 should be fine for name generation.
  NvData.size            += sizeof (TPMI_ALG_HASH);

  // 0x420F0404 - Attributes
  NvData.nvPublic.attributes.TPMA_NV_POLICYWRITE    = 1;    // BIT2
  NvData.nvPublic.attributes.TPMA_NV_POLICY_DELETE  = 1;    // BIT10
  NvData.nvPublic.attributes.TPMA_NV_PPREAD         = 1;    // BIT16
  NvData.nvPublic.attributes.TPMA_NV_OWNERREAD      = 1;    // BIT17
  NvData.nvPublic.attributes.TPMA_NV_AUTHREAD       = 1;    // BIT18
  NvData.nvPublic.attributes.TPMA_NV_POLICYREAD     = 1;    // BIT19
  NvData.nvPublic.attributes.TPMA_NV_NO_DA          = 1;    // BIT25
  NvData.nvPublic.attributes.TPMA_NV_PLATFORMCREATE = 1;    // BIT30
  NvData.size                                      += sizeof (TPMA_NV);

  // NOTE: This will update the NvData.nvPublic.authPolicy.size to the correct value.
  CopyMem (&NvData.nvPublic.authPolicy, &mSgAuthPolicyDigestBuffer, sizeof (TPM2B_DIGEST));
  NvData.size             += sizeof (UINT16) + NvData.nvPublic.authPolicy.size;
  NvData.nvPublic.dataSize = SG_NV_INDEX_SIZE;
  NvData.size             += sizeof (UINT16);

  //
  // Attempt to create the NV Index.
  //
  Status = Tpm2NvDefineSpace (
             TPM_RH_PLATFORM,                     // AuthHandle
             NULL,                                // AuthSession
             &NullAuth,                           // Auth
             &NvData
             );                                   // NvPublic
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Tpm2NvDefineSpace() = %r\n", __func__, Status));
  } else {
    DEBUG ((DEBUG_INFO, "%a - Tpm2NvDefineSpace() = %r\n", __func__, Status));
  }

Done:
  DEBUG ((DEBUG_INFO, "%a Exit - %r\n", __func__, Status));
  return Status;
} // DefineSgTpmNVIndex()
