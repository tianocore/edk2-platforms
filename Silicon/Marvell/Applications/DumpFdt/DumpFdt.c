/** @file

  SPDX-License-Identifier: BSD-2-Clause-Patent
  https://spdx.org/licenses

  Copyright (C) 2023 Marvell

  Copyright (c) 2015, ARM Ltd. All rights reserved.<BR>

**/
#include <Uefi.h>
#include <Pi/PiBootMode.h>
#include <Pi/PiHob.h>
#include <Library/HobLib.h>
#include <Library/HiiLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Guid/FdtHob.h>
#include <Guid/Fdt.h>
#include <Protocol/Shell.h>
#include <Library/ShellLib.h>
#include <Library/ShellCommandLib.h>
#include <libfdt.h>

#define CMD_NAME_STRING       L"dumpfdt"

STATIC EFI_HII_HANDLE gShellDumpFdtHiiHandle = NULL;
STATIC VOID       *mFdtBlobBase = NULL;
STATIC CONST CHAR16 gShellDumpFdtFileName[] = L"ShellCommands";

#define ALIGN(x, a)     (((x) + ((a) - 1)) & ~((a) - 1))
#define PALIGN(p, a)    ((void *)(ALIGN ((unsigned long)(p), (a))))
#define GET_CELL(p)     (p += 4, *((const uint32_t *)(p-4)))

STATIC
UINTN
IsPrintableString (
  IN CONST VOID* data,
  IN UINTN len
  )
{
  CONST CHAR8 *s = data;
  CONST CHAR8 *ss;

  // Zero length is not
  if (len == 0) {
    return 0;
  }

  // Must terminate with zero
  if (s[len - 1] != '\0') {
    return 0;
  }

  ss = s;
  while (*s/* && isprint (*s)*/) {
    s++;
  }

  // Not zero, or not done yet
  if (*s != '\0' || (s + 1 - ss) < len) {
    return 0;
  }

  return 1;
}

STATIC
VOID
PrintData (
  IN CONST CHAR8* data,
  IN UINTN len
  )
{
  UINTN i;
  CONST CHAR8 *p = data;

  // No data, don't print
  if (len == 0)
    return;

  if (IsPrintableString (data, len)) {
    Print (L" = \"%a\"", (const char *)data);
  } else if ((len % 4) == 0) {
    Print (L" = <");
    for (i = 0; i < len; i += 4) {
      Print (L"0x%08x%a", fdt32_to_cpu (GET_CELL (p)), i < (len - 4) ? " " : "");
    }
    Print (L">");
  } else {
    Print (L" = [");
    for (i = 0; i < len; i++)
      Print (L"%02x%a", *p++, i < len - 1 ? " " : "");
    Print (L"]");
  }
}

STATIC
VOID
DumpFdt (
  IN VOID*                FdtBlob
  )
{
  struct fdt_header *bph;
  UINT32 off_dt;
  UINT32 off_str;
  CONST CHAR8* p_struct;
  CONST CHAR8* p_strings;
  CONST CHAR8* p;
  CONST CHAR8* s;
  CONST CHAR8* t;
  UINT32 tag;
  UINTN sz;
  UINTN depth;
  UINTN shift;
  UINT32 version;

  {
    // Can 'memreserve' be printed by below code?
    INTN num = fdt_num_mem_rsv (FdtBlob);
    INTN i, err;
    UINT64 addr = 0, size = 0;

    for (i = 0; i < num; i++) {
      err = fdt_get_mem_rsv (FdtBlob, i, &addr, &size);
      if (err) {
        DEBUG ((DEBUG_ERROR, "Error (%d) : Cannot get memreserve section (%d)\n", err, i));
      }
      else {
        Print (L"/memreserve/ \t0x%lx \t0x%lx;\n", addr, size);
      }
    }
  }

  depth = 0;
  shift = 4;

  bph = FdtBlob;
  off_dt = fdt32_to_cpu (bph->off_dt_struct);
  off_str = fdt32_to_cpu (bph->off_dt_strings);
  p_struct = (CONST CHAR8*)FdtBlob + off_dt;
  p_strings = (CONST CHAR8*)FdtBlob + off_str;
  version = fdt32_to_cpu (bph->version);

  p = p_struct;
  while ((tag = fdt32_to_cpu (GET_CELL (p))) != FDT_END) {
    if (tag == FDT_BEGIN_NODE) {
      s = p;
      p = PALIGN (p + AsciiStrLen (s) + 1, 4);

      if (*s == '\0')
              s = "/";

      Print (L"%*s%a {\n", depth * shift, L" ", s);

      depth++;
      continue;
    }

    if (tag == FDT_END_NODE) {
      depth--;

      Print (L"%*s};\n", depth * shift, L" ");
      continue;
    }

    if (tag == FDT_NOP) {
      /* Print (L"%*s// [NOP]\n", depth * shift, L" "); */
      continue;
    }

    if (tag != FDT_PROP) {
      Print (L"%*s ** Unknown tag 0x%08x\n", depth * shift, L" ", tag);
      break;
    }
    sz = fdt32_to_cpu (GET_CELL (p));
    s = p_strings + fdt32_to_cpu (GET_CELL (p));
    if (version < 16 && sz >= 8)
            p = PALIGN (p, 8);
    t = p;

    p = PALIGN (p + sz, 4);

    Print (L"%*s%a", depth * shift, L" ", s);
    PrintData (t, sz);
    Print (L";\n");
  }
}

STATIC
SHELL_STATUS
EFIAPI
ShellCommandRunDumpFdt (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  SHELL_STATUS  ShellStatus;
  EFI_STATUS    Status;

  ShellStatus  = SHELL_SUCCESS;

  Status = ShellInitialize ();
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return SHELL_ABORTED;
  }

  DumpFdt (mFdtBlobBase);

  return ShellStatus;
}

/**
  Return the file name of the help text file if not using HII.

  @return The string pointer to the file name.
**/
STATIC
CONST CHAR16*
EFIAPI
ShellCommandGetManFileNameDumpFdt (
  VOID
  )
{
  return gShellDumpFdtFileName;
}

/**
  Install the FDT specified by its device path in text form.

  @retval  EFI_SUCCESS            The FDT was installed.
  @retval  EFI_NOT_FOUND          Failed to locate a protocol or a file.
  @retval  EFI_INVALID_PARAMETER  Invalid device path.
  @retval  EFI_UNSUPPORTED        Device path not supported.
  @retval  EFI_OUT_OF_RESOURCES   An allocation failed.
**/
STATIC
EFI_STATUS
InstallFdt (
    VOID
)
{
  EFI_STATUS                          Status;
  VOID                                *HobList;
  EFI_HOB_GUID_TYPE                   *GuidHob;

  //
  // Get the HOB list.  If it is not present, then ASSERT.
  //
  HobList = GetHobList ();
  ASSERT (HobList != NULL);

  //
  // Search for FDT GUID HOB.  If it is not present, then
  // there's nothing we can do. It may not exist on the update path.
  //
  GuidHob = GetNextGuidHob (&gFdtHobGuid, HobList);
  if (GuidHob != NULL) {
    mFdtBlobBase = (VOID *)*(UINT64 *)(GET_GUID_HOB_DATA (GuidHob));

    //
    // Ensure that the FDT header is valid and that the Size of the Device Tree
    // is smaller than the size of the read file
    //
    if (fdt_check_header (mFdtBlobBase)) {
        DEBUG ((DEBUG_ERROR, "InstallFdt() - FDT blob seems to be corrupt\n"));
        mFdtBlobBase = NULL;
        Status = EFI_LOAD_ERROR;
        goto Error;
    }
  } else {
    Status = EFI_NOT_FOUND;
    goto Error;
  }

  Status = EFI_SUCCESS;

Error:
  return Status;
}

EFI_STATUS
EFIAPI
ShellDumpFdtCommandConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS Status;

  Status = InstallFdt();
  if (EFI_ERROR(Status)) {
    Print (L"%s: Error while installing FDT\n", CMD_NAME_STRING);
    return Status;
  }

  gShellDumpFdtHiiHandle = NULL;

  gShellDumpFdtHiiHandle = HiiAddPackages (
                                  &gShellDumpFdtHiiGuid,
                                  gImageHandle,
                                  UefiShellDumpFdtLibStrings,
                                  NULL
                                );

  if (gShellDumpFdtHiiHandle == NULL) {
    Print (L"%s: Cannot add Hii package\n", CMD_NAME_STRING);
    return EFI_DEVICE_ERROR;
  }

  Status = ShellCommandRegisterCommandName (
                           CMD_NAME_STRING,
                           ShellCommandRunDumpFdt,
                           ShellCommandGetManFileNameDumpFdt,
                           0,
                           CMD_NAME_STRING,
                           TRUE,
                           gShellDumpFdtHiiHandle,
                           STRING_TOKEN (STR_GET_HELP_DUMPFDT)
                         );

  if (EFI_ERROR(Status)) {
    Print (L"%s: Error while registering command\n", CMD_NAME_STRING);
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
ShellDumpFdtCommandDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{

  if (gShellDumpFdtHiiHandle != NULL) {
    HiiRemovePackages (gShellDumpFdtHiiHandle);
  }

  return EFI_SUCCESS;
}
