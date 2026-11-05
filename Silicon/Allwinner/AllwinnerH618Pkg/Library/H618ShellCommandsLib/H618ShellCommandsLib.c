/** @file
  Allwinner H618 UEFI Shell commands.

  Copyright (c) 2026, Khai Do <dovankhai97@gmail.com>. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <AllwinnerH618.h>

#define SID_REG_PRCTL             (AW_H618_SID_BASE + AW_H618_SID_PRCTL_OFFSET)
#define SID_REG_RDKEY             (AW_H618_SID_BASE + AW_H618_SID_RDKEY_OFFSET)
#define SID_OP_READ_LOCK          0xAC02U
#define SID_PRCTL_OP_LOCK_BIT     0x0002U
#define SID_POLL_TIMEOUT          10000U
#define SID_DEFAULT_DUMP_BYTES    0x10U

STATIC CONST CHAR16  mH618ShellFileName[] = L"H618ShellCommands";
STATIC EFI_HII_HANDLE  mH618ShellHiiHandle = NULL;

STATIC CONST EFI_GUID  mH618ShellHiiGuid = {
  0x8b2e9a74, 0x3c51, 0x4f18, { 0x9a, 0x42, 0x6e, 0x1d, 0x8b, 0x3c, 0x5a, 0x91 }
};

/**
  Return the man filename for H618 shell commands.

  @retval !NULL  The unicode string of the MAN filename.
**/
STATIC
CONST CHAR16 *
EFIAPI
ShellCommandGetManFileNameH618 (
  VOID
  )
{
  return mH618ShellFileName;
}

/**
  Dump Allwinner H618 Security ID (SID) eFuse words with boundary checks.
  Usage: siddump [Offset] [Bytes]

  @param[in] ImageHandle  The image handle.
  @param[in] SystemTable  The system table pointer.

  @retval SHELL_SUCCESS            The command completed successfully.
  @retval SHELL_INVALID_PARAMETER  Offset or byte count is out of bounds or unaligned.
**/
STATIC
SHELL_STATUS
EFIAPI
ShellCommandRunSidDump (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  LIST_ENTRY    *Package;
  CHAR16        *ProblemParam;
  CONST CHAR16  *ArgStr;
  UINT64        ParsedVal;
  UINT32        StartOffset;
  UINT32        ByteCount;
  UINT32        Offset;
  UINT32        Timeout;
  UINT32        WordVal;

  StartOffset = 0;
  ByteCount   = SID_DEFAULT_DUMP_BYTES;

  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);

  Status = ShellCommandLineParseEx (EmptyParamList, &Package, &ProblemParam, TRUE, TRUE);
  if (EFI_ERROR (Status)) {
    if (ProblemParam != NULL) {
      FreePool (ProblemParam);
    }

    return SHELL_INVALID_PARAMETER;
  }

  if (ShellCommandLineGetCount (Package) > 3) {
    Print (L"Usage: siddump [Offset] [Bytes]\r\n");
    ShellCommandLineFreeVarList (Package);
    return SHELL_INVALID_PARAMETER;
  }

  ArgStr = ShellCommandLineGetRawValue (Package, 1);
  if (ArgStr != NULL) {
    Status = ShellConvertStringToUint64 (ArgStr, &ParsedVal, FALSE, FALSE);
    if (EFI_ERROR (Status) || (ParsedVal >= AW_H618_SID_EFUSE_SIZE) || ((ParsedVal & 0x3ULL) != 0)) {
      Print (
        L"Error: SID Offset must be 4-byte aligned and < 0x%x\r\n",
        AW_H618_SID_EFUSE_SIZE
        );
      ShellCommandLineFreeVarList (Package);
      return SHELL_INVALID_PARAMETER;
    }

    StartOffset = (UINT32)ParsedVal;
  }

  ArgStr = ShellCommandLineGetRawValue (Package, 2);
  if (ArgStr != NULL) {
    Status = ShellConvertStringToUint64 (ArgStr, &ParsedVal, FALSE, FALSE);
    if (EFI_ERROR (Status) ||
        (ParsedVal < sizeof (UINT32)) ||
        ((ParsedVal & 0x3ULL) != 0) ||
        (ParsedVal > (UINT64)(AW_H618_SID_EFUSE_SIZE - StartOffset)))
    {
      Print (
        L"Error: SID Bytes must be a multiple of 4 within [0x4..0x%x]\r\n",
        AW_H618_SID_EFUSE_SIZE - StartOffset
        );
      ShellCommandLineFreeVarList (Package);
      return SHELL_INVALID_PARAMETER;
    }

    ByteCount = (UINT32)ParsedVal;
  } else if (StartOffset > (AW_H618_SID_EFUSE_SIZE - ByteCount)) {
    ByteCount = AW_H618_SID_EFUSE_SIZE - StartOffset;
  }

  ShellCommandLineFreeVarList (Package);

  Print (
    L"Allwinner H618 SID Controller (Base: 0x%08x, Offset: 0x%03x, Size: 0x%03x)\r\n",
    (UINT32)AW_H618_SID_BASE,
    StartOffset,
    ByteCount
    );
  Print (L"------------------------------------------------------------------------\r\n");

  for (Offset = StartOffset; Offset < (StartOffset + ByteCount); Offset += sizeof (UINT32)) {
    MmioWrite32 (SID_REG_PRCTL, (Offset << 16) | SID_OP_READ_LOCK);
    for (Timeout = 0; Timeout < SID_POLL_TIMEOUT; Timeout++) {
      if ((MmioRead32 (SID_REG_PRCTL) & SID_PRCTL_OP_LOCK_BIT) == 0) {
        break;
      }
    }

    WordVal = MmioRead32 (SID_REG_RDKEY);
    MmioWrite32 (SID_REG_PRCTL, 0x0);
    Print (L"  SID[0x%03x] = 0x%08x\r\n", Offset, WordVal);
  }

  return SHELL_SUCCESS;
}

/**
  Constructor for Allwinner H618 custom Shell commands library.

  @param[in] ImageHandle  The image handle of the process.
  @param[in] SystemTable  The EFI System Table pointer.

  @retval EFI_SUCCESS     The shell commands were registered successfully.
**/
EFI_STATUS
EFIAPI
H618ShellCommandsLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  mH618ShellHiiHandle = HiiAddPackages (
                          &mH618ShellHiiGuid,
                          gImageHandle,
                          H618ShellCommandsLibStrings,
                          NULL
                          );
  if (mH618ShellHiiHandle == NULL) {
    return EFI_DEVICE_ERROR;
  }

  ShellCommandRegisterCommandName (
    L"siddump",
    ShellCommandRunSidDump,
    ShellCommandGetManFileNameH618,
    0,
    L"H618",
    TRUE,
    mH618ShellHiiHandle,
    STRING_TOKEN (STR_GET_HELP_SIDDUMP)
    );

  return EFI_SUCCESS;
}

/**
  Destructor for Allwinner H618 custom Shell commands library.

  @param[in] ImageHandle  The image handle of the process.
  @param[in] SystemTable  The EFI System Table pointer.

  @retval EFI_SUCCESS     Resources freed successfully.
**/
EFI_STATUS
EFIAPI
H618ShellCommandsLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (mH618ShellHiiHandle != NULL) {
    HiiRemovePackages (mH618ShellHiiHandle);
    mH618ShellHiiHandle = NULL;
  }

  return EFI_SUCCESS;
}
