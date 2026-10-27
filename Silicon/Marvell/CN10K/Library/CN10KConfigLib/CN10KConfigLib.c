/** @file

  Source file for CN10K SMC Interface.

  Copyright (c) 2015, Cavium Inc. All rights reserved.<BR>
  Copyright (c) 2026, Marvell. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/CN10KConfigLib.h>
#include <Library/PrintLib.h>
#include <Library/FdtLib.h>

#include <Guid/FdtHob.h>

/**
  Get a pointer to the Flattened Device Tree.

  @return  A pointer to the FDT.
**/
VOID *
CN10KGetFdt (
  VOID
  )
{
  VOID               *HobList;
  EFI_HOB_GUID_TYPE  *GuidHob;
  VOID               *FdtBlobBase = NULL;
  UINTN              FdtBlobSize;

  /* Get the HOB list.  If it is not present, then ASSERT. */
  HobList = GetHobList ();
  ASSERT (HobList != NULL);

  /*
   * Search for the Memory Configuration GUID HOB.  If it is not present, then
   * there's nothing we can do. It may not exist on the update path.
   */
  GuidHob = GetNextGuidHob (&gFdtHobGuid, HobList);
  if (GuidHob != NULL) {
    FdtBlobBase = (VOID *)*(UINT64 *)(GET_GUID_HOB_DATA (GuidHob));
    FdtBlobSize = FdtTotalSize ((VOID *)FdtBlobBase);

    /*
     * Ensure that the FDT header is valid and that the Size of the Device Tree
     * is smaller than the size of the read file
     */
    if ((FdtCheckHeader (FdtBlobBase) != 0) ||
        ((UINTN)FdtTotalSize (FdtBlobBase) > FdtBlobSize))
    {
      DEBUG ((
        DEBUG_ERROR,
        "CN10KGetConfigProperty() - FDT seems to be corrupt\n"
        ));
      return NULL;
    }
  }

  return FdtBlobBase;
}

/**
  Get the value of a property in the configuration node.

  @param[in]  Name    The property name.
  @param[out] Length  The length of the property's value.

  @return  A pointer to the property's value.
**/
CONST VOID *
CN10KGetConfigProperty (
  IN  CONST CHAR8  *Name,
  OUT INT32        *Length
  )
{
  VOID  *FdtBlobBase;
  INTN  Offset;

  FdtBlobBase = CN10KGetFdt ();
  Offset      = FdtPathOffset (
                  FdtBlobBase,
                  (CHAR8 *)PcdGetPtr (PcdFdtConfigRootNode)
                  );

  if (Offset < 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a node is not found\n",
      (CHAR8 *)PcdGetPtr (PcdFdtConfigRootNode)
      ));
    return NULL;
  }

  return FdtGetProp (FdtBlobBase, Offset, Name, Length);
}

/**
  Get the value of a property in the configuration node as an integer.

  @param[in]  Name    The property name.
  @param[out] Value   The property's value.
  @param[in]  Base    The base to use for the string->integer conversion.

  @retval  EFI_SUCCESS            On success.
  @retval  EFI_NOT_FOUND          The property cannot be found.
  @retval  EFI_INVALID_PARAMETER  The Base argument is neither 10 nor 16.
**/
EFI_STATUS
CN10KGetConfigPropertyInt (
  IN  CONST CHAR8  *Name,
  OUT UINT64       *Value,
  IN  UINTN        Base
  )
{
  INT32        PropertyLength;
  CONST CHAR8  *Property;

  DEBUG ((DEBUG_INFO, "######## FDT: %a(%a)\n", __FUNCTION__, Name));

  Property = CN10KGetConfigProperty (Name, &PropertyLength);

  if (Property == NULL) {
    return EFI_NOT_FOUND;
  }

  if (Base == 0) {
    if ((AsciiStrnLenS (Property, 2) >= 2) &&
        ((Property[0] == '0') && ((Property[1] == 'x') || (Property[1] == 'X'))))
    {
      Base = 16;
    } else {
      Base = 10;
    }
  }

  switch (Base) {
    case 16:
      *Value = AsciiStrHexToUint64 (Property);
      break;
    case 10:
      *Value = AsciiStrDecimalToUint64 (Property);
      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Get the value of a property in the configuration node.

  @param[in]  Name    The property name.
  @param[out] Value   A pointer to a buffer that will be filled with the value.
  @param[in]  Length  The length of the Value buffer.

  @retval  EFI_SUCCESS            On success.
  @retval  EFI_NOT_FOUND          The property cannot be found.
  @retval  EFI_BUFFER_TOO_SMALL   The buffer passed as argument is too small.
**/
EFI_STATUS
CN10KGetConfigPropertyAscii (
  IN  CONST CHAR8  *Name,
  OUT CHAR8        *Value,
  IN  UINTN        Length
  )
{
  INT32        PropertyLength;
  CONST CHAR8  *Property;

  DEBUG ((DEBUG_INFO, "######## FDT: %a(%a)\n", __FUNCTION__, Name));

  Property = CN10KGetConfigProperty (Name, &PropertyLength);

  if (Property == NULL) {
    return EFI_NOT_FOUND;
  }

  if (PropertyLength > Length) {
    return EFI_BUFFER_TOO_SMALL;
  }

  AsciiStrnCpyS (Value, 100, Property, PropertyLength);

  Value[PropertyLength - 1] = '\0';
  return EFI_SUCCESS;
}
