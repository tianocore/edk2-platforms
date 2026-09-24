/** @file
  BaseDtFrameworkLib - null implementation.

  Stub implementations of the DTFramework Device Tree APIs exposed by
  BaseDtFrameworkLib.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/DtFrameworkLib.h>

/**
  DTB selection is not supported when DTFramework is absent.

  @param[in]   DtbsImageStartAddress  Ignored.
  @param[in]   DtbsImageSize          Ignored.
  @param[in]   ChipPlatInfoProp       Ignored.
  @param[in]   PropList               Ignored.
  @param[in]   PropNumEntries         Ignored.
  @param[out]  DtbAddr                Ignored.
  @param[out]  DtbSize                Ignored.

  @retval  Always returns -DTB_ERR_NOTSUPPORTED.

**/
INT32
EFIAPI
DtFrameworkGetDt (
  IN  UINTN                       DtbsImageStartAddress,
  IN  UINTN                       DtbsImageSize,
  IN  DTFRAMEWORK_CHIP_PLAT_INFO  *ChipPlatInfoProp,
  IN  VOID                        *PropList,
  IN  UINT32                      PropNumEntries,
  OUT UINTN                       *DtbAddr,
  OUT UINTN                       *DtbSize
  )
{
  return -DTB_ERR_NOTSUPPORTED;
}

/**
  Blob handle registration is not supported when DTFramework is absent.

  @param[in]  Blob    Ignored.
  @param[in]  BSize   Ignored.
  @param[in]  BlobId  Ignored.

  @retval  -DTB_ERR_NOTSUPPORTED  Always.

**/
INT32
EFIAPI
DtFrameworkSetBlobHandle (
  IN CONST VOID  *Blob,
  IN UINTN       BSize,
  IN INT32       BlobId
  )
{
  return -DTB_ERR_NOTSUPPORTED;
}

/**
  Node handle lookup is not supported when DTFramework is absent.

  @param[in,out]  Node  Ignored.
  @param[in]      Blob  Ignored.
  @param[in]      Name  Ignored.

  @retval  -DTB_ERR_NOTSUPPORTED  Always.

**/
INT32
EFIAPI
DtFrameworkGetNodeHandle (
  IN OUT DT_NODE_HANDLE  *Node,
  IN     CONST VOID      *Blob,
  IN     CHAR8           *Name
  )
{
  return -DTB_ERR_NOTSUPPORTED;
}

/**
  Root handle initialization is not supported when DTFramework is absent.

  @param[in,out]  Node  Ignored.

  @retval  -DTB_ERR_NOTSUPPORTED  Always.

**/
INT32
EFIAPI
DtFrameworkInitRootHandleForDriver (
  IN OUT DT_NODE_HANDLE  *Node
  )
{
  return -DTB_ERR_NOTSUPPORTED;
}

/**
  Root handle initialization by blob is not supported when DTFramework is absent.

  @param[in,out]  Node  Ignored.
  @param[in]      Blob  Ignored.

  @retval  -DTB_ERR_NOTSUPPORTED  Always.

**/
INT32
EFIAPI
DtFrameworkInitRootHandleForDriverByBlob (
  IN OUT DT_NODE_HANDLE  *Node,
  IN     CONST VOID      *Blob
  )
{
  return -DTB_ERR_NOTSUPPORTED;
}

/**
  Subnode count is not supported when DTFramework is absent.

  @param[in]   Node   Ignored.
  @param[out]  Count  Ignored.

  @retval  -DTB_ERR_NOTSUPPORTED  Always.

**/
INT32
EFIAPI
DtFrameworkGetCountOfSubnodes (
  IN  DT_NODE_HANDLE  *Node,
  OUT UINT32          *Count
  )
{
  if (Count != NULL) {
    *Count = 0;
  }

  return -DTB_ERR_NOTSUPPORTED;
}

/**
  Subnode cache is not supported when DTFramework is absent.

  @param[in]   Node   Ignored.
  @param[out]  Cache  Ignored.
  @param[in]   Count  Ignored.

  @retval  -DTB_ERR_NOTSUPPORTED  Always.

**/
INT32
EFIAPI
DtFrameworkGetCacheOfSubnodes (
  IN  DT_NODE_HANDLE  *Node,
  OUT DT_NODE_HANDLE  *Cache,
  IN  UINT32          Count
  )
{
  return -DTB_ERR_NOTSUPPORTED;
}

/**
  Register property access is not supported when DTFramework is absent.

  @param[in]   Node      Ignored.
  @param[in]   RegName   Ignored.
  @param[in]   RegIndex  Ignored.
  @param[in]   AddrCode  Ignored.
  @param[in]   SizeCode  Ignored.
  @param[out]  RegPaddr  Ignored.
  @param[out]  RegSize   Ignored.

  @retval  -DTB_ERR_NOTSUPPORTED  Always.

**/
INT32
EFIAPI
DtFrameworkGetReg (
  IN  DT_NODE_HANDLE  *Node,
  IN  CHAR8           *RegName,
  IN  INT32           RegIndex,
  IN  INT32           AddrCode,
  IN  INT32           SizeCode,
  OUT UINT64          *RegPaddr,
  OUT UINT64          *RegSize
  )
{
  return -DTB_ERR_NOTSUPPORTED;
}

/**
  Property size query is not supported when DTFramework is absent.

  @param[in]   Node      Ignored.
  @param[in]   PropName  Ignored.
  @param[out]  Size      Ignored.

  @retval  -DTB_ERR_NOTSUPPORTED  Always.

**/
INT32
EFIAPI
DtFrameworkGetPropSize (
  IN  DT_NODE_HANDLE  *Node,
  IN  CONST CHAR8     *PropName,
  OUT UINT32          *Size
  )
{
  if (Size != NULL) {
    *Size = 0;
  }

  return -DTB_ERR_NOTSUPPORTED;
}

/**
  Property access is not supported when DTFramework is absent.

  @param[in]   Node      Ignored.
  @param[in]   PropName  Ignored.
  @param[out]  Value     Ignored.

  @retval  -DTB_ERR_NOTSUPPORTED  Always.

**/
INT32
EFIAPI
DtFrameworkGetUint8Prop (
  IN  DT_NODE_HANDLE  *Node,
  IN  CONST CHAR8     *PropName,
  OUT UINT8           *Value
  )
{
  return -DTB_ERR_NOTSUPPORTED;
}

/**
  Property access is not supported when DTFramework is absent.

  @param[in]   Node      Ignored.
  @param[in]   PropName  Ignored.
  @param[out]  Value     Ignored.

  @retval  -DTB_ERR_NOTSUPPORTED  Always.

**/
INT32
EFIAPI
DtFrameworkGetUint32Prop (
  IN  DT_NODE_HANDLE  *Node,
  IN  CONST CHAR8     *PropName,
  OUT UINT32          *Value
  )
{
  return -DTB_ERR_NOTSUPPORTED;
}

/**
  UINT32 property list access is not supported when DTFramework is absent.

  @param[in]   Node      Ignored.
  @param[in]   PropName  Ignored.
  @param[out]  PropList  Ignored.
  @param[in]   Size      Ignored.

  @retval  -DTB_ERR_NOTSUPPORTED  Always.

**/
INT32
EFIAPI
DtFrameworkGetUint32PropList (
  IN  DT_NODE_HANDLE  *Node,
  IN  CONST CHAR8     *PropName,
  OUT UINT32          *PropList,
  IN  UINT32          Size
  )
{
  return -DTB_ERR_NOTSUPPORTED;
}

/**
  Property access is not supported when DTFramework is absent.

  @param[in]   Node      Ignored.
  @param[in]   PropName  Ignored.
  @param[out]  Value     Ignored.

  @retval  -DTB_ERR_NOTSUPPORTED  Always.

**/
INT32
EFIAPI
DtFrameworkGetUint64Prop (
  IN  DT_NODE_HANDLE  *Node,
  IN  CONST CHAR8     *PropName,
  OUT UINT64          *Value
  )
{
  return -DTB_ERR_NOTSUPPORTED;
}

/**
  String property access is not supported when DTFramework is absent.

  @param[in]   Node        Ignored.
  @param[in]   PropName    Ignored.
  @param[out]  StringList  Ignored.
  @param[in]   Size        Ignored.

  @retval  -DTB_ERR_NOTSUPPORTED  Always.

**/
INT32
EFIAPI
DtFrameworkGetStringPropList (
  IN  DT_NODE_HANDLE  *Node,
  IN  CONST CHAR8     *PropName,
  OUT CHAR8           *StringList,
  IN  UINT32          Size
  )
{
  return -DTB_ERR_NOTSUPPORTED;
}

/**
  Property names size query is not supported when DTFramework is absent.

  @param[in]   Node  Ignored.
  @param[out]  Size  Ignored.

  @retval  -DTB_ERR_NOTSUPPORTED  Always.

**/
INT32
EFIAPI
DtFrameworkGetPropNamesSizeOfNode (
  IN  DT_NODE_HANDLE  *Node,
  OUT UINT32          *Size
  )
{
  if (Size != NULL) {
    *Size = 0;
  }

  return -DTB_ERR_NOTSUPPORTED;
}

/**
  Property names retrieval is not supported when DTFramework is absent.

  @param[in]   Node       Ignored.
  @param[out]  PropNames  Ignored.
  @param[in]   Size       Ignored.

  @retval  -DTB_ERR_NOTSUPPORTED  Always.

**/
INT32
EFIAPI
DtFrameworkGetPropNamesOfNode (
  IN  DT_NODE_HANDLE  *Node,
  OUT CHAR8           *PropNames,
  IN  UINT32          Size
  )
{
  return -DTB_ERR_NOTSUPPORTED;
}

/**
  HOB publication is not supported when DTFramework is absent.

  @retval  EFI_UNSUPPORTED  Always.

**/
EFI_STATUS
EFIAPI
DtFrameworkPublishDtbExtnIntfHob (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}
