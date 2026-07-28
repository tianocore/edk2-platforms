/** @file
  BaseDtFrameworkLib - real implementation.

  UEFI-style wrappers for Qualcomm DTFramework Device Tree APIs.
  Each function delegates directly to the corresponding DTFramework
  fdt_* C function declared in DTBExtnLib.h.

  The DTFramework DTBExtnLib sources (see BaseDtFrameworkLib.inf) must be
  present in the build for this implementation to link.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <PiPei.h>
#include <DTBExtnLib.h>
#include <Library/DebugLib.h>
#include <Library/DtFrameworkLib.h>
#include <Library/HobLib.h>
#include <Protocol/DeviceTreeExtension.h>
#include <get_dt.h>
#include <libfdt.h>

/*
 * Compile-time layout verification: DTFRAMEWORK_CHIP_PLAT_INFO is a UEFI-typed mirror
 * of chip_plat_info_property.  DtFrameworkGetDt casts between the two types, so
 * their sizes must be identical.  If DTFramework ever adds or reorders fields
 * in chip_plat_info_property this assertion will catch the mismatch at build
 * time rather than silently producing incorrect behaviour at run time.
 */
STATIC_ASSERT (
  sizeof (DTFRAMEWORK_CHIP_PLAT_INFO) == sizeof (chip_plat_info_property),
  "DTFRAMEWORK_CHIP_PLAT_INFO size mismatch with chip_plat_info_property - update DtFrameworkLib.h"
  );

/*
 * DTB_ERR_* codes in DTBDefs.h intentionally start at 1000 to stay
 * clear of libfdt's own error range.  This assertion forces a manual
 * review of that offset if libfdt's FDT_ERR_MAX is ever raised past it.
 */
STATIC_ASSERT (
  FDT_ERR_MAX < DTB_ERR_NULLPTR,
  "libfdt FDT_ERR_MAX has grown into the DTB_ERR_* range - update DTBDefs.h"
  );

/*
 * DT_NODE_HANDLE (DTBDefs.h, PascalCase Blob/Offset members) and
 * fdt_node_handle (DTBCompatibility.h, lowercase blob/offset members) are
 * intentionally two distinct struct definitions - the former follows EDK II
 * naming rules, the latter matches the field names the vendored DTFramework
 * C sources access directly. This assertion, together with the pointer
 * casts at each fdt_* call site below, relies on the two staying
 * layout-identical instead of copying fields in and out on every call.
 */
STATIC_ASSERT (
  sizeof (DT_NODE_HANDLE) == sizeof (fdt_node_handle),
  "DT_NODE_HANDLE size mismatch with fdt_node_handle - update DTBDefs.h or DTBCompatibility.h"
  );

/**
  Select the best-matching DTB from a packed image.

  Thin wrapper around the DTFramework get_dt() function.
  See DtFrameworkLib.h for full parameter and return value documentation.

  @param[in]   DtbsImageStartAddress  Base address of the packed DTB image.
  @param[in]   DtbsImageSize          Size of the packed DTB image.
  @param[in]   ChipPlatInfoProp       Chip and platform identification.
  @param[in]   PropList               Optional caller-defined property list.
  @param[in]   PropNumEntries         Number of entries in PropList.
  @param[out]  DtbAddr                Receives the selected DTB address.
  @param[out]  DtbSize                Receives the selected DTB size.

  @retval  0    DTB selected successfully.
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
  )
{
  return get_dt (
           DtbsImageStartAddress,
           (size_t)DtbsImageSize,
           (chip_plat_info_property *)ChipPlatInfoProp,
           PropList,
           PropNumEntries,
           DtbAddr,
           DtbSize
           );
}

/**
  Register a DTB blob with the DTFramework blob manager.

  Thin wrapper around fdt_set_blob_handle().
  See DtFrameworkLib.h for full parameter and return value documentation.

  @param[in]  Blob    Pointer to the device tree blob.
  @param[in]  BSize   Size of the blob in bytes.
  @param[in]  BlobId  Slot index.

  @retval  DTB_ERR_NOERROR  Handle registered successfully.
  @retval  Other               Error code from DTFramework.

**/
INT32
EFIAPI
DtFrameworkSetBlobHandle (
  IN CONST VOID  *Blob,
  IN UINTN       BSize,
  IN INT32       BlobId
  )
{
  return fdt_set_blob_handle (Blob, (size_t)BSize, BlobId);
}

/**
  Get an FDT node handle by path name.

  Thin wrapper around fdt_get_node_handle().
  See DtFrameworkLib.h for full parameter and return value documentation.

  @param[in,out]  Node  Pointer to the DT_NODE_HANDLE to populate.
  @param[in]      Blob  Pointer to the device tree blob.
  @param[in]      Name  Full path of the node.

  @retval  DTB_ERR_NOERROR  Node found and handle populated.
  @retval  Other               Error code from DTFramework.

**/
INT32
EFIAPI
DtFrameworkGetNodeHandle (
  IN OUT DT_NODE_HANDLE  *Node,
  IN     CONST VOID      *Blob,
  IN     CHAR8           *Name
  )
{
  return fdt_get_node_handle ((fdt_node_handle *)Node, Blob, Name);
}

/**
  Initialize a root FDT node handle using the default registered blob.

  Uses DTB_DEFAULT_BLOB_ID to look up the blob registered via DtFrameworkSetBlobHandle().
  See DtFrameworkLib.h for full parameter and return value documentation.

  @param[in,out]  Node  Pointer to the DT_NODE_HANDLE to initialise.

  @retval  DTB_ERR_NOERROR  Handle initialised.
  @retval  Other               Error code from DTFramework.

**/
INT32
EFIAPI
DtFrameworkInitRootHandleForDriver (
  IN OUT DT_NODE_HANDLE  *Node
  )
{
  /*
   * Use the by-ID variant with DTB_DEFAULT_BLOB_ID (0) to look up the blob
   * registered via DtFrameworkSetBlobHandle().  Using the explicit by-ID API
   * makes the intent unambiguous.
   */
  return fdt_init_root_handle_for_driver_by_id ((fdt_node_handle *)Node, DTB_DEFAULT_BLOB_ID);
}

/**
  Initialize a root FDT node handle using an explicit blob pointer.

  Thin wrapper around fdt_init_root_handle_for_driver().
  See DtFrameworkLib.h for full parameter and return value documentation.

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
  )
{
  return fdt_init_root_handle_for_driver ((fdt_node_handle *)Node, Blob);
}

/**
  Get the number of direct subnodes of a node.

  Thin wrapper around fdt_get_count_of_subnodes().
  See DtFrameworkLib.h for full parameter and return value documentation.

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
  )
{
  return fdt_get_count_of_subnodes ((fdt_node_handle *)Node, Count);
}

/**
  Get a cached array of direct subnode handles.

  Thin wrapper around fdt_get_cache_of_subnodes().
  See DtFrameworkLib.h for full parameter and return value documentation.

  @param[in]   Node   Parent node handle.
  @param[out]  Cache  Array of DT_NODE_HANDLE to populate.
  @param[in]   Count  Number of entries in Cache.

  @retval  DTB_ERR_NOERROR  Cache populated.
  @retval  Other               Error code from DTFramework.

**/
INT32
EFIAPI
DtFrameworkGetCacheOfSubnodes (
  IN  DT_NODE_HANDLE  *Node,
  OUT DT_NODE_HANDLE  *Cache,
  IN  UINT32          Count
  )
{
  return fdt_get_cache_of_subnodes ((fdt_node_handle *)Node, (fdt_node_handle *)Cache, Count);
}

/**
  Get a register (address/size pair) property from a node.

  Thin wrapper around fdt_get_reg().
  See DtFrameworkLib.h for full parameter and return value documentation.

  @param[in]   Node      Node handle.
  @param[in]   RegName   Name of the reg-names entry, or NULL.
  @param[in]   RegIndex  Zero-based index into the reg array.
  @param[in]   AddrCode  Address cell size code.
  @param[in]   SizeCode  Size cell size code.
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
  )
{
  return fdt_get_reg ((fdt_node_handle *)Node, RegName, RegIndex, AddrCode, SizeCode, RegPaddr, RegSize);
}

/**
  Get the byte size of a named property.

  Thin wrapper around fdt_get_prop_size().
  See DtFrameworkLib.h for full parameter and return value documentation.

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
  )
{
  return fdt_get_prop_size ((fdt_node_handle *)Node, PropName, Size);
}

/**
  Get a UINT8 property value from a node.

  Thin wrapper around fdt_get_uint8_prop().
  See DtFrameworkLib.h for full parameter and return value documentation.

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
  )
{
  return fdt_get_uint8_prop ((fdt_node_handle *)Node, PropName, Value);
}

/**
  Get a UINT32 property value from a node.

  Thin wrapper around fdt_get_uint32_prop().
  See DtFrameworkLib.h for full parameter and return value documentation.

  @param[in]   Node      Node handle.
  @param[in]   PropName  Property name.
  @param[out]  Value     Receives the UINT32 value.

  @retval  DTB_ERR_NOERROR  Value returned.
  @retval  Other               Error code from DTFramework.

**/
INT32
EFIAPI
DtFrameworkGetUint32Prop (
  IN  DT_NODE_HANDLE  *Node,
  IN  CONST CHAR8     *PropName,
  OUT UINT32          *Value
  )
{
  return fdt_get_uint32_prop ((fdt_node_handle *)Node, PropName, Value);
}

/**
  Get a list of UINT32 property values from a node.

  Thin wrapper around fdt_get_uint32_prop_list().
  See DtFrameworkLib.h for full parameter and return value documentation.

  @param[in]   Node      Node handle.
  @param[in]   PropName  Property name.
  @param[out]  PropList  Buffer to receive the UINT32 values.
  @param[in]   Size      Size of PropList in bytes.

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
  )
{
  return fdt_get_uint32_prop_list ((fdt_node_handle *)Node, PropName, PropList, Size);
}

/**
  Get a UINT64 property value from a node.

  Thin wrapper around fdt_get_uint64_prop().
  See DtFrameworkLib.h for full parameter and return value documentation.

  @param[in]   Node      Node handle.
  @param[in]   PropName  Property name.
  @param[out]  Value     Receives the UINT64 value.

  @retval  DTB_ERR_NOERROR  Value returned.
  @retval  Other               Error code from DTFramework.

**/
INT32
EFIAPI
DtFrameworkGetUint64Prop (
  IN  DT_NODE_HANDLE  *Node,
  IN  CONST CHAR8     *PropName,
  OUT UINT64          *Value
  )
{
  return fdt_get_uint64_prop ((fdt_node_handle *)Node, PropName, Value);
}

/**
  Get a string (or string list) property from a node.

  Thin wrapper around fdt_get_string_prop_list().
  See DtFrameworkLib.h for full parameter and return value documentation.

  @param[in]   Node        Node handle.
  @param[in]   PropName    Property name.
  @param[out]  StringList  Buffer to receive the string data.
  @param[in]   Size        Size of StringList in bytes.

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
  )
{
  return fdt_get_string_prop_list ((fdt_node_handle *)Node, PropName, StringList, Size);
}

/**
  Get the total byte size needed to hold all property names of a node.

  Thin wrapper around fdt_get_prop_names_size_of_node().
  See DtFrameworkLib.h for full parameter and return value documentation.

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
  )
{
  return fdt_get_prop_names_size_of_node ((fdt_node_handle *)Node, Size);
}

/**
  Get all property names of a node into a caller-supplied buffer.

  Thin wrapper around fdt_get_prop_names_of_node().
  See DtFrameworkLib.h for full parameter and return value documentation.

  @param[in]   Node       Node handle.
  @param[out]  PropNames  Buffer to receive the packed name strings.
  @param[in]   Size       Size of PropNames in bytes.

  @retval  DTB_ERR_NOERROR  Names returned.
  @retval  Other               Error code from DTFramework.

**/
INT32
EFIAPI
DtFrameworkGetPropNamesOfNode (
  IN  DT_NODE_HANDLE  *Node,
  OUT CHAR8           *PropNames,
  IN  UINT32          Size
  )
{
  return fdt_get_prop_names_of_node ((fdt_node_handle *)Node, PropNames, Size);
}

/**
  Static wrapper: DTB_INIT_ROOT_HANDLE_FOR_DRIVER takes only (Node) but
  fdt_init_root_handle_for_driver_by_id takes (Node, BlobId).

  @param[in,out]  Node  Pointer to the FDT node handle to initialise.

  @retval  DTB_ERR_NOERROR  Handle initialised using DTB_DEFAULT_BLOB_ID.
  @retval  Other               Error code from DTFramework.

**/
STATIC INT32
DtbExtnInitRootHandleForDriver (
  IN OUT FDT_NODE_HANDLE  *Node
  )
{
  return fdt_init_root_handle_for_driver_by_id ((fdt_node_handle *)Node, DTB_DEFAULT_BLOB_ID);
}

/**
  Static wrapper: DTB_GET_NODE_HANDLE takes (Node, Name) but
  fdt_get_node_handle takes (Node, Blob, Name).  Retrieve the default
  blob via fdt_get_blob_handle and forward the call.

  @param[in,out]  Node  Pointer to the FDT node handle to populate.
  @param[in]      Name  Full path of the node.

  @retval  DTB_ERR_NOERROR   Node found and handle populated.
  @retval  -DTB_ERR_NULLPTR  Node or Name is NULL.
  @retval  Other                Error code from DTFramework.

**/
STATIC INT32
DtbExtnGetNodeHandle (
  IN OUT FDT_NODE_HANDLE  *Node,
  IN CHAR8                *Name
  )
{
  CONST VOID  *Blob;
  INT32       Ret;

  if ((Node == NULL) || (Name == NULL)) {
    return -DTB_ERR_NULLPTR;
  }

  Blob = NULL;
  Ret  = fdt_get_blob_handle (&Blob, DTB_DEFAULT_BLOB_ID);
  if (Ret != 0) {
    return Ret;
  }

  return fdt_get_node_handle ((fdt_node_handle *)Node, Blob, Name);
}

/**
  Build a PEI GUID HOB containing a pointer to the populated
  DTB_EXTN_PROTOCOL interface.

  @retval  EFI_SUCCESS           HOB built successfully.
  @retval  EFI_OUT_OF_RESOURCES  The HOB could not be allocated.

**/
EFI_STATUS
EFIAPI
DtFrameworkPublishDtbExtnIntfHob (
  VOID
  )
{
  STATIC DTB_EXTN_PROTOCOL  DtbExtnProtocol;
  DTB_EXTN_PROTOCOL         *DtbExtnProtocolPtr;

  //
  // Populate the protocol vtable with DTFramework C function pointers.
  // Wrapper functions are used where the protocol signature differs from
  // the underlying DTFramework function signature.
  //
  DtbExtnProtocol.Version                           = DTB_EXTN_PROTOCOL_VERSION;
  DtbExtnProtocol.FdtInitRootHandleForDriver        = (DTB_INIT_ROOT_HANDLE_FOR_DRIVER)DtbExtnInitRootHandleForDriver;
  DtbExtnProtocol.FdtGetNameIndex                   = (DTB_GET_NAME_INDEX)fdt_get_name_index;
  DtbExtnProtocol.FdtGetNextNodeHandleForCompatible =
    (DTB_GET_NEXT_NODE_HANDLE_FOR_COMPATIBLE)fdt_get_next_node_handle_for_compatible;
  DtbExtnProtocol.FdtGetReg                         = (DTB_GET_REG)fdt_get_reg;
  DtbExtnProtocol.FdtGetCountOfSubnodes             = (DTB_GET_COUNT_OF_SUBNODES)fdt_get_count_of_subnodes;
  DtbExtnProtocol.FdtGetNodeHandle                  = (DTB_GET_NODE_HANDLE)DtbExtnGetNodeHandle;
  DtbExtnProtocol.FdtGetParentNode                  = (DTB_GET_PARENT_NODE)fdt_get_parent_node;
  DtbExtnProtocol.FdtGetPhandleNode                 = (DTB_GET_PHANDLE_NODE)fdt_get_phandle_node;
  DtbExtnProtocol.FdtGetPropNamesOfNode             = (DTB_GET_PROP_NAMES_OF_NODE)fdt_get_prop_names_of_node;
  DtbExtnProtocol.FdtGetPropNamesSizeOfNode         = (DTB_GET_PROP_NAMES_SIZE_OF_NODE)fdt_get_prop_names_size_of_node;
  DtbExtnProtocol.FdtGetPropValuesOfNode            = (DTB_GET_PROP_VALUES_OF_NODE)fdt_get_prop_values_of_node;
  DtbExtnProtocol.FdtGetPropValuesSizeOfNode        =
    (DTB_GET_PROP_VALUES_SIZE_OF_NODE)fdt_get_prop_values_size_of_node;
  DtbExtnProtocol.FdtGetSizeOfSubnodeNames          = (DTB_GET_SIZE_OF_SUBNODE_NAMES)fdt_get_size_of_subnode_names;
  DtbExtnProtocol.FdtGetSubnodeNames                = (DTB_GET_SUBNODE_NAMES)fdt_get_subnode_names;
  DtbExtnProtocol.FdtNodeCmp                        = (DTB_NODE_CMP)fdt_node_cmp;
  DtbExtnProtocol.FdtNodeCopy                       = (DTB_NODE_COPY)fdt_node_copy;
  DtbExtnProtocol.FdtGetBooleanProp                 = (DTB_GET_BOOLEAN_PROP)fdt_get_boolean_prop;
  DtbExtnProtocol.FdtGetPropSize                    = (DTB_GET_PROP_SIZE)fdt_get_prop_size;
  DtbExtnProtocol.FdtGetStringPropList              = (DTB_GET_STRING_PROP_LIST)fdt_get_string_prop_list;
  DtbExtnProtocol.FdtGetUint32Prop                  = (DTB_GET_UINT32_PROP)fdt_get_uint32_prop;
  DtbExtnProtocol.FdtGetUint32PropList              = (DTB_GET_UINT32_PROP_LIST)fdt_get_uint32_prop_list;
  DtbExtnProtocol.FdtGetUint32PropListSlice         = (DTB_GET_UINT32_PROP_LIST_SLICE)fdt_get_uint32_prop_list_slice;
  DtbExtnProtocol.FdtGetUint64Prop                  = (DTB_GET_UINT64_PROP)fdt_get_uint64_prop;
  DtbExtnProtocol.FdtGetUint64PropList              = (DTB_GET_UINT64_PROP_LIST)fdt_get_uint64_prop_list;
  DtbExtnProtocol.FdtGetUint8Prop                   = (DTB_GET_UINT8_PROP)fdt_get_uint8_prop;
  DtbExtnProtocol.FdtGetUint8PropList               = (DTB_GET_UINT8_PROP_LIST)fdt_get_uint8_prop_list;
  DtbExtnProtocol.FdtGetUint8PropListSlice          = (DTB_GET_UINT8_PROP_LIST_SLICE)fdt_get_uint8_prop_list_slice;
  DtbExtnProtocol.FdtGetUint16Prop                  = (DTB_GET_UINT16_PROP)fdt_get_uint16_prop;
  DtbExtnProtocol.FdtGetUint16PropList              = (DTB_GET_UINT16_PROP_LIST)fdt_get_uint16_prop_list;
  DtbExtnProtocol.FdtGetUint16PropListSlice         = (DTB_GET_UINT16_PROP_LIST_SLICE)fdt_get_uint16_prop_list_slice;
  DtbExtnProtocol.FdtGetCacheOfSubnodes             = (DTB_GET_CACHE_OF_SUBNODES)fdt_get_cache_of_subnodes;
  DtbExtnProtocol.FdtGetUint64PropListSlice         = (DTB_GET_UINT64_PROP_LIST_SLICE)fdt_get_uint64_prop_list_slice;
  DtbExtnProtocol.FdtSetBlobHandle                  = (DTB_SET_BLOB_HANDLE)fdt_set_blob_handle;
  DtbExtnProtocol.FdtCheckValidBlob                 = (DTB_CHECK_VALID_BLOB)fdt_check_for_valid_blob;
  DtbExtnProtocol.FdtMergeOverlay                   = (DTB_MERGE_OVERLAY)fdt_merge_overlay;
  DtbExtnProtocol.FdtGetBlobHandle                  = (DTB_GET_BLOB_HANDLE)fdt_get_blob_handle;
  DtbExtnProtocol.FdtGetBlobSize                    = (DTB_GET_BLOB_SIZE)fdt_get_blob_size;
  DtbExtnProtocol.FdtInitRootHandleForDriverById    =
    (DTB_INIT_ROOT_HANDLE_FOR_DRIVER_BY_ID)fdt_init_root_handle_for_driver_by_id;
  DtbExtnProtocol.FdtGetNodeHandleByBlob            = (DTB_GET_NODE_HANDLE_BY_BLOB)fdt_get_node_handle;
  DtbExtnProtocol.FdtInitRootHandleForDriverByBlob  =
    (DTB_INIT_ROOT_HANDLE_FOR_DRIVER_BY_BLOB)fdt_init_root_handle_for_driver;
  DtbExtnProtocol.FdtGetBoolProp                    = (DTB_GET_BOOL_PROP)fdt_get_bool_prop;

  //
  // The HOB data is a pointer to the protocol structure.  DtbExtnDxe reads
  // the HOB data as a UINTN * and dereferences it to get the protocol pointer.
  //
  DtbExtnProtocolPtr = &DtbExtnProtocol;
  if (BuildGuidDataHob (&gDtbExtensionInterfaceHobGuid, &DtbExtnProtocolPtr, sizeof (DtbExtnProtocolPtr)) == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to build DTB extension interface HOB\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}
