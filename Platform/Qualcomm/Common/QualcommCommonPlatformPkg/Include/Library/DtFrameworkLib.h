/** @file
  DtFrameworkLib - UEFI-style wrappers for the Qualcomm DTFramework
  Device Tree APIs used in this platform package.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Uefi.h>
#include <DTBDefs.h>

///
/// Opaque node handle used by all DTFramework node-navigation and
/// property-access APIs.  Alias of the FDT_NODE_HANDLE type defined in
/// DTBDefs.h, shared with the DTB_EXTN_PROTOCOL definition.
///
typedef FDT_NODE_HANDLE DT_NODE_HANDLE;

///
/// UEFI-typed mirror of the DTFramework chip_plat_info_property struct.
///
/// Layout is identical to chip_plat_info_property on all supported
/// Qualcomm UEFI targets (AArch64): UINT16 == uint16_t, UINT8 == uint8_t,
/// UINT32 == uint32_t, CHAR8* == char*.  The real implementation casts
/// DTFRAMEWORK_CHIP_PLAT_INFO* directly to chip_plat_info_property*.
///
typedef struct {
  UINT16    ChipFamily;
  UINT16    ChipId;
  UINT16    ChipMajVersion;
  UINT16    ChipMinVersion;
  UINT8     PlatformType;
  UINT8     PlatformSubtype;
  UINT8     PlatformMajVersion;
  UINT8     PlatformMinVersion;
  UINT32    OemVar;
  CHAR8     *DtbCompatibleStringStartsWith;
  CHAR8     *SocDtboCompatibleStringStartsWith;
  CHAR8     *PlatDtboCompatibleStringStartsWith;
} DTFRAMEWORK_CHIP_PLAT_INFO;

/**
  Select the best-matching DTB (and apply SOC/platform overlays) from a
  packed DTB image, returning a pointer to the final merged blob.

  This is a thin wrapper around the DTFramework get_dt() function.

  @param[in]   DtbsImageStartAddress  Base address of the packed DTB image.
  @param[in]   DtbsImageSize          Size of the packed DTB image (0 = scan
                                       until a valid DTB/DTBO is found).
  @param[in]   ChipPlatInfoProp       Chip and platform identification used
                                       for DTB selection.  Must not be NULL.
  @param[in]   PropList               Optional caller-defined property list
                                       for additional matching criteria.
  @param[in]   PropNumEntries         Number of entries in @PropList.
  @param[out]  DtbAddr                Receives the address of the selected
                                       (and merged) DTB blob.
  @param[out]  DtbSize                Receives the size of the selected DTB.

  @retval  0    DTB selected and returned successfully.
  @retval  -1   Selection failed.
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
  );

/**
  Register a DTB blob with the DTFramework blob-handle table.

  @param[in]  Blob    Pointer to the device tree blob.
  @param[in]  BSize   Size of the blob in bytes.
  @param[in]  BlobId  Slot index (0 .. DTB_MAX_BLOB_ID-1).

  @retval  DTB_ERR_NOERROR  Handle registered successfully.
  @retval  Other               Error code from DTFramework.
**/
INT32
EFIAPI
DtFrameworkSetBlobHandle (
  IN CONST VOID  *Blob,
  IN UINTN       BSize,
  IN INT32       BlobId
  );

/**
  Obtain a node handle by full path string.

  @param[in,out]  Node  Pointer to the DT_NODE_HANDLE to populate.
  @param[in]      Blob  Pointer to the device tree blob (may be NULL to use
                        the default blob registered via DtFrameworkSetBlobHandle).
  @param[in]      Name  Full path of the node, e.g. "/soc/uart@0".

  @retval  DTB_ERR_NOERROR  Node found and handle populated.
  @retval  Other               Error code from DTFramework.
**/
INT32
EFIAPI
DtFrameworkGetNodeHandle (
  IN OUT DT_NODE_HANDLE  *Node,
  IN     CONST VOID      *Blob,
  IN     CHAR8           *Name
  );

/**
  Initialise a root node handle using the default blob (DTB_DEFAULT_BLOB_ID).

  @param[in,out]  Node  Pointer to the DT_NODE_HANDLE to initialise.

  @retval  DTB_ERR_NOERROR  Handle initialised.
  @retval  Other               Error code from DTFramework.
**/
INT32
EFIAPI
DtFrameworkInitRootHandleForDriver (
  IN OUT DT_NODE_HANDLE  *Node
  );

/**
  Initialise a root node handle from an explicit blob pointer.

  @param[in,out]  Node  Pointer to the DT_NODE_HANDLE to initialise.
  @param[in]      Blob  Pointer to the device tree blob.

  @retval  DTB_ERR_NOERROR  Handle initialised.
  @retval  Other               Error code from DTFramework.
**/
INT32
EFIAPI
DtFrameworkInitRootHandleForDriverByBlob (
  IN OUT DT_NODE_HANDLE  *Node,
  IN     CONST VOID      *Blob
  );

/**
  Return the count of immediate subnodes of a node.

  @param[in]   Node   Parent node handle.
  @param[out]  Count  Receives the subnode count.

  @retval  DTB_ERR_NOERROR  Count returned.
  @retval  Other               Error code from DTFramework.
**/
INT32
EFIAPI
DtFrameworkGetCountOfSubnodes (
  IN  DT_NODE_HANDLE  *Node,
  OUT UINT32          *Count
  );

/**
  Populate a caller-supplied array with handles for all immediate subnodes.

  @param[in]   Node   Parent node handle.
  @param[out]  Cache  Array of DT_NODE_HANDLE with at least @Count entries.
  @param[in]   Count  Number of entries in @Cache.

  @retval  DTB_ERR_NOERROR  Cache populated.
  @retval  Other               Error code from DTFramework.
**/
INT32
EFIAPI
DtFrameworkGetCacheOfSubnodes (
  IN  DT_NODE_HANDLE  *Node,
  OUT DT_NODE_HANDLE  *Cache,
  IN  UINT32          Count
  );

/**
  Retrieve a reg entry (address + size) from a node.

  @param[in]   Node      Node handle.
  @param[in]   RegName   Name of the reg-names entry, or NULL.
  @param[in]   RegIndex  Zero-based index into the reg array.
  @param[in]   AddrCode  Address cell size code (-1 = from tree, 1 = 32-bit, 2 = 64-bit).
  @param[in]   SizeCode  Size cell size code (-1 = from tree, 0 = none, 1 = 32-bit, 2 = 64-bit).
  @param[out]  RegPaddr  Receives the physical base address.
  @param[out]  RegSize   Receives the region size.

  @retval  DTB_ERR_NOERROR  Register entry returned.
  @retval  Other               Error code from DTFramework.
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
  );

/**
  Return the raw byte size of a named property.

  @param[in]   Node      Node handle.
  @param[in]   PropName  Property name.
  @param[out]  Size      Receives the property size in bytes.

  @retval  DTB_ERR_NOERROR  Size returned.
  @retval  Other               Error code from DTFramework.
**/
INT32
EFIAPI
DtFrameworkGetPropSize (
  IN  DT_NODE_HANDLE  *Node,
  IN  CONST CHAR8     *PropName,
  OUT UINT32          *Size
  );

/**
  Read a single UINT8 property value.

  @param[in]   Node      Node handle.
  @param[in]   PropName  Property name.
  @param[out]  Value     Receives the UINT8 value.

  @retval  DTB_ERR_NOERROR  Value returned.
  @retval  Other               Error code from DTFramework.
**/
INT32
EFIAPI
DtFrameworkGetUint8Prop (
  IN  DT_NODE_HANDLE  *Node,
  IN  CONST CHAR8     *PropName,
  OUT UINT8           *Value
  );

/**
  Read a single UINT32 property value.

  @param[in]   Node      Node handle.
  @param[in]   PropName  Property name.
  @param[out]  Value     Receives the UINT32 value (host byte order).

  @retval  DTB_ERR_NOERROR  Value returned.
  @retval  Other               Error code from DTFramework.
**/
INT32
EFIAPI
DtFrameworkGetUint32Prop (
  IN  DT_NODE_HANDLE  *Node,
  IN  CONST CHAR8     *PropName,
  OUT UINT32          *Value
  );

/**
  Read a list of UINT32 property values into a caller-supplied buffer.

  @param[in]   Node      Node handle.
  @param[in]   PropName  Property name.
  @param[out]  PropList  Buffer to receive the UINT32 values (host byte order).
  @param[in]   Size      Size of @PropList in bytes.

  @retval  DTB_ERR_NOERROR  Values returned.
  @retval  Other               Error code from DTFramework.
**/
INT32
EFIAPI
DtFrameworkGetUint32PropList (
  IN  DT_NODE_HANDLE  *Node,
  IN  CONST CHAR8     *PropName,
  OUT UINT32          *PropList,
  IN  UINT32          Size
  );

/**
  Read a single UINT64 property value.

  @param[in]   Node      Node handle.
  @param[in]   PropName  Property name.
  @param[out]  Value     Receives the UINT64 value (host byte order).

  @retval  DTB_ERR_NOERROR  Value returned.
  @retval  Other               Error code from DTFramework.
**/
INT32
EFIAPI
DtFrameworkGetUint64Prop (
  IN  DT_NODE_HANDLE  *Node,
  IN  CONST CHAR8     *PropName,
  OUT UINT64          *Value
  );

/**
  Read a string-list property into a caller-supplied buffer.

  The buffer receives the raw NUL-separated string data as stored in the DTB.

  @param[in]   Node        Node handle.
  @param[in]   PropName    Property name.
  @param[out]  StringList  Buffer to receive the string data.
  @param[in]   Size        Size of @StringList in bytes.

  @retval  DTB_ERR_NOERROR  Strings returned.
  @retval  Other               Error code from DTFramework.
**/
INT32
EFIAPI
DtFrameworkGetStringPropList (
  IN  DT_NODE_HANDLE  *Node,
  IN  CONST CHAR8     *PropName,
  OUT CHAR8           *StringList,
  IN  UINT32          Size
  );

/**
  Return the total byte size needed to hold all property names of a node.

  @param[in]   Node  Node handle.
  @param[out]  Size  Receives the required buffer size in bytes.

  @retval  DTB_ERR_NOERROR  Size returned.
  @retval  Other               Error code from DTFramework.
**/
INT32
EFIAPI
DtFrameworkGetPropNamesSizeOfNode (
  IN  DT_NODE_HANDLE  *Node,
  OUT UINT32          *Size
  );

/**
  Return a packed buffer of NUL-terminated property names for a node.

  @param[in]   Node       Node handle.
  @param[out]  PropNames  Buffer to receive the packed name strings.
  @param[in]   Size       Size of @PropNames in bytes.

  @retval  DTB_ERR_NOERROR  Names returned.
  @retval  Other               Error code from DTFramework.
**/
INT32
EFIAPI
DtFrameworkGetPropNamesOfNode (
  IN  DT_NODE_HANDLE  *Node,
  OUT CHAR8           *PropNames,
  IN  UINT32          Size
  );

/**
  Build a PEI GUID HOB containing a pointer to the populated
  DTB_EXTN_PROTOCOL interface.

  This function must be called from PEI phase after the DTB blob has been
  initialised (i.e. after DtFrameworkSetBlobHandle() has been called).  The HOB
  is identified by gDtbExtensionInterfaceHobGuid and is consumed by DtbExtnDxe
  in the DXE phase to install the DTB_EXTN_PROTOCOL.

  The null implementation (BaseDtFrameworkNullLib) returns EFI_UNSUPPORTED.

  @retval  EFI_SUCCESS     HOB built successfully.
  @retval  EFI_UNSUPPORTED DTFramework is absent (null implementation).

**/
EFI_STATUS
EFIAPI
DtFrameworkPublishDtbExtnIntfHob (
  VOID
  );
