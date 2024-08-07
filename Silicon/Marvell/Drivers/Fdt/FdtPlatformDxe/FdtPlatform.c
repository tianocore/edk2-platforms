/** @file

  SPDX-License-Identifier: BSD-2-Clause-Patent
  https://spdx.org/licenses

  Copyright (C) 2023 Marvell

  Copyright (c) 2015, ARM Ltd. All rights reserved.<BR>

**/

#include <Uefi.h>
#include <Library/PcdLib.h>
#include <Library/BdsLib.h>
#include <Pi/PiBootMode.h>
#include <Pi/PiHob.h>
#include <Library/HobLib.h>
#include <Library/HiiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Guid/FdtHob.h>
#include <libfdt.h>

//
// Internal variables
//

VOID       *mFdtBlobBase;

EFI_STATUS
DeleteFdtNode (
  IN  VOID    *FdtAddr,
  CONST CHAR8 *NodePath,
  CONST CHAR8 *Compatible
)
{
  INTN        Offset = -1;
  INTN        Return;

  if ((NodePath != NULL) && (Compatible != NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (NodePath != NULL) {
    Offset = fdt_path_offset (FdtAddr, NodePath);

    DEBUG ((DEBUG_INFO, "Offset: %d\n", Offset));

    if (Offset < 0) {
      DEBUG ((DEBUG_ERROR, "Error getting the device node %a offset: %a\n",
              NodePath, fdt_strerror (Offset)));
      return EFI_NOT_FOUND;
    }
  }

  if (Compatible != NULL) {
    Offset = fdt_node_offset_by_compatible (FdtAddr, -1, Compatible);

    DEBUG ((DEBUG_INFO, "Offset: %d\n", Offset));

    if (Offset < 0) {
      DEBUG ((DEBUG_ERROR, "Error getting the device node for %a offset: %a\n",
              Compatible, fdt_strerror (Offset)));
      return EFI_NOT_FOUND;
    }
  }

  if (Offset >= 0) {
    Return = fdt_del_node (FdtAddr, Offset);

    DEBUG ((DEBUG_INFO, "Return: %d\n", Return));

    if (Return < 0) {
      DEBUG ((DEBUG_ERROR, "Error deleting the device node %a: %a\n",
              NodePath, fdt_strerror (Return)));
      return EFI_NOT_FOUND;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
DeleteRtcNode (
  IN  VOID    *FdtAddr
  )
{
  INT32 Offset, NameLen, Return;
  BOOLEAN Found;
  CONST CHAR8 *Name;

  Found = FALSE;
  for (Offset = fdt_next_node(FdtAddr, 0, NULL);
    Offset >= 0;
    Offset = fdt_next_node(FdtAddr, Offset, NULL)) {

    Name = fdt_get_name(FdtAddr, Offset, &NameLen);
    if (!Name) {
      continue;
    }

    if ((Name[0] == 'r') && (Name[1] == 't') && (Name[2] == 'c')) {
      Found = TRUE;
      break;
    }
  }

  if (Found == TRUE) {
    Return = fdt_del_node (FdtAddr, Offset);

    if (Return < 0) {
      DEBUG ((DEBUG_ERROR, "Error deleting the device node %a\n", Name));
      return EFI_NOT_FOUND;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
FdtFixup(
  IN VOID *FdtAddr
  )
{
  EFI_STATUS Status = EFI_SUCCESS;

  if (FeaturePcdGet(PcdFixupFdt)) {
    Status |= DeleteFdtNode (FdtAddr, (CHAR8*)PcdGetPtr (PcdFdtConfigRootNode), NULL);

    // Hide the RTC
    Status |= DeleteRtcNode (FdtAddr);
  }

  if (!EFI_ERROR(Status)) {
    fdt_pack(FdtAddr);
  }

  return EFI_SUCCESS;
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
  IN UINTN FdtBlobSize
)
{
  EFI_STATUS  Status;
  VOID        *FdtConfigurationTableBase;

  Status = EFI_SUCCESS;

  FdtConfigurationTableBase = AllocateRuntimeCopyPool (FdtBlobSize, mFdtBlobBase);
  if (FdtConfigurationTableBase == NULL) {
      goto Error;
  }
  Status = FdtFixup((VOID*)FdtConfigurationTableBase);
  if (EFI_ERROR (Status)) {
    FreePool (FdtConfigurationTableBase);
    goto Error;
  }
  //
  // Install the FDT into the Configuration Table
  //
  Status = gBS->InstallConfigurationTable (
                &gFdtTableGuid,
                FdtConfigurationTableBase
              );
  if (EFI_ERROR (Status)) {
    FreePool (FdtConfigurationTableBase);
  }

Error:
  return Status;
}

/**
  Main entry point of the FDT platform driver.

  @param[in]  ImageHandle   The firmware allocated handle for the present driver
                            UEFI image.
  @param[in]  *SystemTable  A pointer to the EFI System table.

  @retval  EFI_SUCCESS           The driver was initialized.
  @retval  EFI_OUT_OF_RESOURCES  The "End of DXE" event could not be allocated or
                                 there was not enough memory in pool to install
                                 the Shell Dynamic Command protocol.
  @retval  EFI_LOAD_ERROR        Unable to add the HII package.

**/
EFI_STATUS
FdtPlatformEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  VOID                *HobList;
  EFI_HOB_GUID_TYPE   *GuidHob;
  UINTN               FdtBlobSize;

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
    FdtBlobSize = fdt_totalsize((VOID *)mFdtBlobBase);

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

  //
  // Install the Device Tree from its expected location
  //
  if (FeaturePcdGet(PcdPublishFdt)) {
    Status = InstallFdt (FdtBlobSize);
  }

  ASSERT_EFI_ERROR(Status);

Error:
  return Status;
}
