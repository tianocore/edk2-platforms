/** @file

  Device Tree Extension Protocol

  This protocol exposes the Qualcomm DTFramework device tree query APIs
  (node navigation, property access, and blob/overlay management) to DXE
  drivers.  It is a Qualcomm-specific protocol, not part of the UEFI or PI
  specifications.  During the PEI phase, BaseDtFrameworkLib populates a
  protocol instance backed by the DTFramework libfdt-style C functions and
  publishes it in a HOB; DtbExtnDxe consumes that HOB and installs the
  protocol so DXE-phase drivers can look up device tree nodes and
  properties without linking against DTFramework directly.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - DtbExtn - Device Tree Extension
**/

#pragma once

#include <Uefi.h>
#include <DTBDefs.h>

/// Protocol GUID for the Qualcomm DTB Extension Protocol.
#define DTB_EXTN_PROTOCOL_GUID \
  { 0x0389b776, 0x625f, 0x11eb, { 0x83, 0xbe, 0xc7, 0x41, 0xa9, 0x13, 0xde, 0x34 } }

extern EFI_GUID  gDtbExtnProtocolGuid;

/// Initial version of the protocol.
#define DTB_EXTN_PROTOCOL_VER_INIT  0x00010000
/// Revision 1 of the protocol.
#define DTB_EXTN_PROTOCOL_VER_REV1  0x00010001
/// Revision 2 of the protocol.
#define DTB_EXTN_PROTOCOL_VER_REV2  0x00010002
/// Revision 3 of the protocol with additional DT APIs for boolean property support.
#define DTB_EXTN_PROTOCOL_VER_REV3  0x00010003

/// Current protocol version.
#define DTB_EXTN_PROTOCOL_VERSION  DTB_EXTN_PROTOCOL_VER_REV3

/**
  Gets the blob handle for a specified blob ID.

  @param[in, out] Blob    On output, pointer to the device tree blob registered for BlobId.
  @param[in]      BlobId  Index of the blob to retrieve (see DTB_DEFAULT_BLOB_ID/DTB_MAX_BLOB_ID).

  @retval DTB_ERR_NOERROR  The blob handle was retrieved successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_BLOB_HANDLE)(
  IN OUT CONST VOID  **Blob,
  IN INT32           BlobId
  );

/**
  Gets the size of the blob.

  @param[in, out] Node  Pointer to a node handle previously initialized against the blob.
  @param[in, out] Size  On output, size of the device tree blob, in bytes.

  @retval DTB_ERR_NOERROR  The blob size was retrieved successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_BLOB_SIZE)(
  IN OUT FDT_NODE_HANDLE  *Node,
  IN OUT UINT32           *Size
  );

/**
  Initializes root handle for driver with a given blob.

  @param[in, out] Node  On output, node handle initialized to the root node of Blob.
  @param[in]      Blob  Pointer to the device tree blob to root the handle against.

  @retval DTB_ERR_NOERROR  The root handle was initialized successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_INIT_ROOT_HANDLE_FOR_DRIVER_BY_BLOB)(
  IN OUT FDT_NODE_HANDLE  *Node,
  IN CONST VOID           *Blob
  );

/**
  Initializes root handle for driver.

  Uses the default registered blob (DTB_DEFAULT_BLOB_ID) as the backing device tree.

  @param[in, out] Node  On output, node handle initialized to the root node of the default blob.

  @retval DTB_ERR_NOERROR  The root handle was initialized successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_INIT_ROOT_HANDLE_FOR_DRIVER)(
  IN OUT FDT_NODE_HANDLE  *Node
  );

/**
  Initializes root handle for driver by blob ID.

  @param[in, out] Node    On output, node handle initialized to the root node of the blob.
  @param[in]      BlobId  Index of the registered blob to root the handle against.

  @retval DTB_ERR_NOERROR  The root handle was initialized successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_INIT_ROOT_HANDLE_FOR_DRIVER_BY_ID)(
  IN OUT FDT_NODE_HANDLE  *Node,
  IN INT32                BlobId
  );

/**
  Sets the blob handle.

  Registers Blob in memory under BlobId so it can later be retrieved via FdtGetBlobHandle
  or used to initialize root handles by ID.

  @param[in] Blob    Pointer to the device tree blob to register.
  @param[in] Size    Size of Blob, in bytes.
  @param[in] BlobId  Index under which to register the blob.

  @retval DTB_ERR_NOERROR  The blob handle was registered successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_SET_BLOB_HANDLE)(
  IN CONST VOID  *Blob,
  IN UINTN       Size,
  IN INT32       BlobId
  );

/**
  Checks if the blob is valid.

  @param[in] Blob  Pointer to the device tree blob to validate.
  @param[in] Size  Size of Blob, in bytes.

  @retval DTB_ERR_NOERROR  Blob is a valid device tree blob of at least Size bytes.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_CHECK_VALID_BLOB)(
  IN CONST VOID  *Blob,
  IN UINTN       Size
  );

/**
  Gets the name index for a property.

  Looks up the string-list property named PName on Node and returns the zero-based
  position of TName within that list.

  @param[in]  Node    Pointer to the node handle whose property is searched.
  @param[in]  PName   Name of the string-list property to search (e.g. "reg-names").
  @param[in]  TName   Target name to locate within the string-list property.
  @param[out] Index   On output, zero-based index of TName within the string-list property.

  @retval DTB_ERR_NOERROR  TName was found and Index was updated.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason
                              (e.g. -FDT_ERR_NOTFOUND if TName is not present in the list).
**/
typedef INT32 (*DTB_GET_NAME_INDEX)(
  IN FDT_NODE_HANDLE  *Node,
  IN CONST CHAR8      *PName,
  IN CHAR8            *TName,
  OUT INT32           *Index
  );

/**
  Gets the next node handle for compatible device.

  @param[in, out] Node        On input, node handle to begin searching from; on output,
                               updated to the next node whose "compatible" property matches Compatible.
  @param[in]      Compatible  Compatible string to search for.

  @retval DTB_ERR_NOERROR  A matching node was found and Node was updated.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_NEXT_NODE_HANDLE_FOR_COMPATIBLE)(
  IN OUT FDT_NODE_HANDLE  *Node,
  IN CONST CHAR8          *Compatible
  );

/**
  Gets register information for a node.

  Decodes the requested entry of the node's "reg" property using the given address/size
  cell widths, optionally selecting the entry by name via "reg-names".

  @param[in]      Node       Pointer to the node handle whose "reg" property is decoded.
  @param[in]      RegName    Name to look up in "reg-names" to select the entry, or NULL to use RegIndex directly.
  @param[in]      RegIndex   Zero-based index of the reg entry to return (used directly if RegName is NULL).
  @param[in]      AddrCode   Address field width in cells: DTB_REG_SIZE_32 or DTB_REG_SIZE_64
                              (DTB_REG_SIZE_BLOB/"from tree" is not currently supported).
  @param[in]      SizeCode   Size field width in cells: DTB_REG_SIZE_NIL (no size field), DTB_REG_SIZE_32, or
                              DTB_REG_SIZE_64 (DTB_REG_SIZE_BLOB/"from tree" is not currently supported).
  @param[in, out] RegPaddr   On output, physical address decoded from the selected reg entry.
  @param[in, out] RegSize    On output, size decoded from the selected reg entry, or 0 if SizeCode is DTB_REG_SIZE_NIL.

  @retval DTB_ERR_NOERROR  The reg entry was decoded successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_REG)(
  IN FDT_NODE_HANDLE  *Node,
  IN CHAR8            *RegName,
  IN INT32            RegIndex,
  IN INT32            AddrCode,
  IN INT32            SizeCode,
  IN OUT UINT64       *RegPaddr,
  IN OUT UINT64       *RegSize
  );

/**
  Gets the count of subnodes.

  @param[in]      Node   Pointer to the node handle whose subnodes are counted.
  @param[in, out] Count  On output, number of immediate subnodes of Node.

  @retval DTB_ERR_NOERROR  Count was updated successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_COUNT_OF_SUBNODES)(
  IN FDT_NODE_HANDLE  *Node,
  IN OUT UINT32       *Count
  );

/**
  Gets node handle by blob.

  @param[in, out] Node  On output, node handle for the node named Name within Blob.
  @param[in]      Blob  Pointer to the device tree blob to search.
  @param[in]      Name  Name of the node to look up.

  @retval DTB_ERR_NOERROR  Node was found and the handle was initialized.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_NODE_HANDLE_BY_BLOB)(
  IN OUT FDT_NODE_HANDLE  *Node,
  IN CONST VOID           *Blob,
  IN CHAR8                *Name
  );

/**
  Gets node handle.

  Equivalent to FdtGetNodeHandleByBlob using the default registered blob (DTB_DEFAULT_BLOB_ID).

  @param[in, out] Node  On output, node handle for the node named Name within the default blob.
  @param[in]      Name  Name of the node to look up.

  @retval DTB_ERR_NOERROR  Node was found and the handle was initialized.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_NODE_HANDLE)(
  IN OUT FDT_NODE_HANDLE  *Node,
  IN CHAR8                *Name
  );

/**
  Gets the parent node handle.

  @param[in]      Node   Pointer to the node handle whose parent is requested.
  @param[in, out] Pnode  On output, node handle for the parent of Node.

  @retval DTB_ERR_NOERROR  Pnode was initialized successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_PARENT_NODE)(
  IN FDT_NODE_HANDLE      *Node,
  IN OUT FDT_NODE_HANDLE  *Pnode
  );

/**
  Gets the phandle node.

  @param[in]      Node     Pointer to a node handle within the same blob as the target phandle.
  @param[in]      Phandle  Phandle value to resolve, as referenced elsewhere in the blob.
  @param[in, out] Pnode    On output, node handle for the node identified by Phandle.

  @retval DTB_ERR_NOERROR  Pnode was initialized successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_PHANDLE_NODE)(
  IN FDT_NODE_HANDLE      *Node,
  IN UINT32               Phandle,
  IN OUT FDT_NODE_HANDLE  *Pnode
  );

/**
  Gets property names of node.

  @param[in]      Node       Pointer to the node handle whose property names are retrieved.
  @param[in, out] PropNames  Buffer to receive the NUL-separated list of property names.
  @param[in]      Size       Size of the PropNames buffer, in bytes (see FdtGetPropNamesSizeOfNode).

  @retval DTB_ERR_NOERROR  PropNames was populated successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_PROP_NAMES_OF_NODE)(
  IN FDT_NODE_HANDLE  *Node,
  IN OUT CHAR8        *PropNames,
  IN UINT32           Size
  );

/**
  Gets the size of property names.

  @param[in] Node  Pointer to the node handle whose property-name buffer size is queried.
  @param[in] Size  On output, required buffer size, in bytes, for FdtGetPropNamesOfNode.

  @retval DTB_ERR_NOERROR  Size was updated successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_PROP_NAMES_SIZE_OF_NODE)(
  IN FDT_NODE_HANDLE  *Node,
  IN UINT32           *Size
  );

/**
  Gets property values of node.

  Copies every property of Node into PackedPropValues according to Format, applying the
  appropriate endian conversion per letter. The destination structure must be packed by
  declaring its fields in descending size order (no #pragma pack), with one letter in
  Format per property in the node, in declaration order. This API should only be used on
  nodes that contain properties only (no subnodes).

  @param[in]      Node                Pointer to the node handle whose properties are packed.
  @param[in]      Format              String with one letter per property, in order:
                                       [b|B] string/byte, [h|H] 16-bit, [w|W] 32-bit,
                                       [d|D] 64-bit, [i|I] ignore (skip) this property.
  @param[in, out] PackedPropValues    Buffer to receive the packed property values.
  @param[in]      Size                Size of the PackedPropValues buffer, in bytes.

  @retval 0                            Success.
  @retval -DTB_ERR_NULLPTR          A required pointer argument was NULL.
  @retval -DTB_ERR_NILVALUE         A property value's size is 0.
  @retval -DTB_ERR_SDNULL           Failed to get the address of a property value in the blob.
  @retval -DTB_ERR_TRUNCATED        Size is smaller than the total size of the node's properties.
  @retval -DTB_ERR_BADFORMAT        Format contains a letter outside the documented set.
  @retval -FDT_ERR_BADOFFSET           The underlying FdtLib call encountered an out-of-bounds offset.
**/
typedef INT32 (*DTB_GET_PROP_VALUES_OF_NODE)(
  IN FDT_NODE_HANDLE  *Node,
  IN CHAR8            *Format,
  IN OUT VOID         *PackedPropValues,
  IN UINT32           Size
  );

/**
  Gets the size of property values.

  @param[in]      Node  Pointer to the node handle whose packed-property buffer size is queried.
  @param[in, out] Size  On output, required buffer size, in bytes, for FdtGetPropValuesOfNode.

  @retval DTB_ERR_NOERROR  Size was updated successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_PROP_VALUES_SIZE_OF_NODE)(
  IN FDT_NODE_HANDLE  *Node,
  IN OUT UINT32       *Size
  );

/**
  Gets size of subnode names.

  @param[in]      Node       Pointer to the node handle whose subnode name sizes are queried.
  @param[in, out] NameSizes  Array to receive the size, in bytes, of each subnode's name.
  @param[in]      Count      Number of entries in the NameSizes array (see FdtGetCountOfSubnodes).

  @retval DTB_ERR_NOERROR  NameSizes was populated successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_SIZE_OF_SUBNODE_NAMES)(
  IN FDT_NODE_HANDLE  *Node,
  IN OUT UINT32       *NameSizes,
  IN UINT32           Count
  );

/**
  Gets subnode names.

  @param[in]      Node   Pointer to the node handle whose subnode names are retrieved.
  @param[in, out] Names  Buffer to receive the subnode names.
  @param[in]      Size   Size of the Names buffer, in bytes.

  @retval DTB_ERR_NOERROR  Names was populated successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_SUBNODE_NAMES)(
  IN FDT_NODE_HANDLE  *Node,
  IN OUT VOID         *Names,
  IN UINT32           Size
  );

/**
  Compares two nodes.

  @param[in] NodeA  Pointer to the first node handle to compare.
  @param[in] NodeB  Pointer to the second node handle to compare.

  @retval 0                            NodeA and NodeB refer to the same node.
  @retval -DTB_ERR_NODE_DIFFERENT   NodeA and NodeB refer to different nodes.
  @retval Others                       A negative DTB_ERR_ (or underlying libfdt) error code
                                        indicating the failure reason (e.g. a NULL node pointer).
**/
typedef INT32 (*DTB_NODE_CMP)(
  IN FDT_NODE_HANDLE  *NodeA,
  IN FDT_NODE_HANDLE  *NodeB
  );

/**
  Copies node data.

  @param[in, out] Dst  Pointer to the destination node handle.
  @param[in]      Src  Pointer to the source node handle to copy from.

  @retval DTB_ERR_NOERROR  Dst was populated successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_NODE_COPY)(
  IN OUT FDT_NODE_HANDLE  *Dst,
  IN FDT_NODE_HANDLE      *Src
  );

/**
  Gets boolean property value.

  Corner case: a zero-length property is not treated as an error; Value is left unmodified
  in that case, so callers should pre-initialize it with a sentinel to detect this.

  @param[in]      Node      Pointer to the node handle whose property is read.
  @param[in]      PropName  Name of the property to read.
  @param[in, out] Value     On output, non-zero property value (unmodified if the property is zero-length).

  @retval DTB_ERR_NOERROR  The call completed successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_BOOLEAN_PROP)(
  IN FDT_NODE_HANDLE  *Node,
  IN CONST CHAR8      *PropName,
  IN OUT UINT32       *Value
  );

/**
  Gets boolean property.

  Tests for the presence of PropName on Node. A boolean property in the device tree blob
  always has a length of 0.

  @param[in] Node      Pointer to the node handle to check.
  @param[in] PropName  Name of the property to check for.

  @retval DTB_ERR_NOERROR  PropName is present on Node.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason
                              (e.g. the property is not present).
**/
typedef INT32 (*DTB_GET_BOOL_PROP)(
  IN FDT_NODE_HANDLE  *Node,
  IN CONST CHAR8      *PropName
  );

/**
  Gets property size.

  @param[in]      Node      Pointer to the node handle whose property size is queried.
  @param[in]      PropName  Name of the property to query.
  @param[in, out] Size      On output, size of the property value, in bytes.

  @retval DTB_ERR_NOERROR  Size was updated successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_PROP_SIZE)(
  IN FDT_NODE_HANDLE  *Node,
  IN CONST CHAR8      *PropName,
  IN OUT UINT32       *Size
  );

/**
  Gets string property list.

  @param[in]      Node        Pointer to the node handle whose property is read.
  @param[in]      PropName    Name of the string-list property to read.
  @param[in, out] StringList  Buffer to receive the NUL-separated list of strings.
  @param[in]      Size        Size of the StringList buffer, in bytes (see FdtGetPropSize).

  @retval DTB_ERR_NOERROR  StringList was populated successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_STRING_PROP_LIST)(
  IN FDT_NODE_HANDLE  *Node,
  IN CONST CHAR8      *PropName,
  IN OUT CHAR8        *StringList,
  IN UINT32           Size
  );

/**
  Gets UINT32 property value.

  Corner case: a zero-length property is not treated as an error; Value is left unmodified
  in that case, so callers should pre-initialize it with a sentinel to detect this. When
  reading multiple properties from one node, FdtGetPropValuesOfNode performs better.

  @param[in]      Node      Pointer to the node handle whose property is read.
  @param[in]      PropName  Name of the property to read.
  @param[in, out] Value     On output, property value (unmodified if the property is zero-length).

  @retval DTB_ERR_NOERROR  The call completed successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_UINT32_PROP)(
  IN FDT_NODE_HANDLE  *Node,
  IN CONST CHAR8      *PropName,
  IN OUT UINT32       *Value
  );

/**
  Gets UINT32 property list.

  @param[in]      Node      Pointer to the node handle whose property is read.
  @param[in]      PropName  Name of the property to read.
  @param[in, out] PropList  Buffer to receive the list of UINT32 values.
  @param[in]      Size      Size of the PropList buffer, in bytes (see FdtGetPropSize).

  @retval DTB_ERR_NOERROR  PropList was populated successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_UINT32_PROP_LIST)(
  IN FDT_NODE_HANDLE  *Node,
  IN CONST CHAR8      *PropName,
  IN OUT UINT32       *PropList,
  IN UINT32           Size
  );

/**
  Gets UINT32 property list slice.

  @param[in]      Node      Pointer to the node handle whose property is read.
  @param[in]      PropName  Name of the property to read.
  @param[in, out] PropList  Buffer to receive Count UINT32 values, starting at Index.
  @param[in]      Index     Zero-based starting index within the property's value list.
  @param[in]      Count     Number of values to extract.

  @retval DTB_ERR_NOERROR  PropList was populated successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_UINT32_PROP_LIST_SLICE)(
  IN FDT_NODE_HANDLE  *Node,
  IN CONST CHAR8      *PropName,
  IN OUT UINT32       *PropList,
  IN UINT32           Index,
  IN UINT32           Count
  );

/**
  Gets UINT64 property value.

  Corner case: a zero-length property is not treated as an error; Value is left unmodified
  in that case, so callers should pre-initialize it with a sentinel to detect this. When
  reading multiple properties from one node, FdtGetPropValuesOfNode performs better.

  @param[in]      Node      Pointer to the node handle whose property is read.
  @param[in]      PropName  Name of the property to read.
  @param[in, out] Value     On output, property value (unmodified if the property is zero-length).

  @retval DTB_ERR_NOERROR  The call completed successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_UINT64_PROP)(
  IN FDT_NODE_HANDLE  *Node,
  IN CONST CHAR8      *PropName,
  IN OUT UINT64       *Value
  );

/**
  Gets UINT64 property list.

  @param[in]      Node      Pointer to the node handle whose property is read.
  @param[in]      PropName  Name of the property to read.
  @param[in, out] PropList  Buffer to receive the list of UINT64 values.
  @param[in]      Size      Size of the PropList buffer, in bytes (see FdtGetPropSize).

  @retval DTB_ERR_NOERROR  PropList was populated successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_UINT64_PROP_LIST)(
  IN FDT_NODE_HANDLE  *Node,
  IN CONST CHAR8      *PropName,
  IN OUT UINT64       *PropList,
  IN UINT32           Size
  );

/**
  Gets UINT8 property value.

  Corner case: a zero-length property is not treated as an error; Value is left unmodified
  in that case, so callers should pre-initialize it with a sentinel to detect this. When
  reading multiple properties from one node, FdtGetPropValuesOfNode performs better.

  @param[in]      Node      Pointer to the node handle whose property is read.
  @param[in]      PropName  Name of the property to read.
  @param[in, out] Value     On output, property value (unmodified if the property is zero-length).

  @retval DTB_ERR_NOERROR  The call completed successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_UINT8_PROP)(
  IN FDT_NODE_HANDLE  *Node,
  IN CONST CHAR8      *PropName,
  IN OUT UINT8        *Value
  );

/**
  Gets UINT8 property list.

  @param[in]      Node      Pointer to the node handle whose property is read.
  @param[in]      PropName  Name of the property to read.
  @param[in, out] PropList  Buffer to receive the list of UINT8 values.
  @param[in]      Size      Size of the PropList buffer, in bytes (see FdtGetPropSize).

  @retval DTB_ERR_NOERROR  PropList was populated successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_UINT8_PROP_LIST)(
  IN FDT_NODE_HANDLE  *Node,
  IN CONST CHAR8      *PropName,
  IN OUT UINT8        *PropList,
  IN UINT32           Size
  );

/**
  Gets UINT8 property list slice.

  @param[in]      Node      Pointer to the node handle whose property is read.
  @param[in]      PropName  Name of the property to read.
  @param[in, out] PropList  Buffer to receive Count UINT8 values, starting at Index.
  @param[in]      Index     Zero-based starting index within the property's value list.
  @param[in]      Count     Number of values to extract.

  @retval DTB_ERR_NOERROR  PropList was populated successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_UINT8_PROP_LIST_SLICE)(
  IN FDT_NODE_HANDLE  *Node,
  IN CONST CHAR8      *PropName,
  IN OUT UINT8        *PropList,
  IN UINT32           Index,
  IN UINT32           Count
  );

/**
  Gets UINT16 property value.

  Corner case: a zero-length property is not treated as an error; Value is left unmodified
  in that case, so callers should pre-initialize it with a sentinel to detect this. When
  reading multiple properties from one node, FdtGetPropValuesOfNode performs better.

  @param[in]      Node      Pointer to the node handle whose property is read.
  @param[in]      PropName  Name of the property to read.
  @param[in, out] Value     On output, property value (unmodified if the property is zero-length).

  @retval DTB_ERR_NOERROR  The call completed successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_UINT16_PROP)(
  IN FDT_NODE_HANDLE  *Node,
  IN CONST CHAR8      *PropName,
  IN OUT UINT16       *Value
  );

/**
  Gets UINT16 property list.

  @param[in]      Node      Pointer to the node handle whose property is read.
  @param[in]      PropName  Name of the property to read.
  @param[in, out] PropList  Buffer to receive the list of UINT16 values.
  @param[in]      Size      Size of the PropList buffer, in bytes (see FdtGetPropSize).

  @retval DTB_ERR_NOERROR  PropList was populated successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_UINT16_PROP_LIST)(
  IN FDT_NODE_HANDLE  *Node,
  IN CONST CHAR8      *PropName,
  IN OUT UINT16       *PropList,
  IN UINT32           Size
  );

/**
  Gets UINT16 property list slice.

  @param[in]      Node      Pointer to the node handle whose property is read.
  @param[in]      PropName  Name of the property to read.
  @param[in, out] PropList  Buffer to receive Count UINT16 values, starting at Index.
  @param[in]      Index     Zero-based starting index within the property's value list.
  @param[in]      Count     Number of values to extract.

  @retval DTB_ERR_NOERROR  PropList was populated successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_UINT16_PROP_LIST_SLICE)(
  IN FDT_NODE_HANDLE  *Node,
  IN CONST CHAR8      *PropName,
  IN OUT UINT16       *PropList,
  IN UINT32           Index,
  IN UINT32           Count
  );

/**
  Gets cache of subnodes.

  Populates Cache with a node handle for every immediate subnode of Node, avoiding the
  string lookups that would otherwise be needed to enumerate subnodes individually.

  @param[in]      Node   Pointer to the node handle whose subnodes are cached.
  @param[in, out] Cache  Array of Count node handles to receive one entry per subnode.
  @param[in]      Count  Number of entries in the Cache array (see FdtGetCountOfSubnodes).

  @retval DTB_ERR_NOERROR  Cache was populated successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_CACHE_OF_SUBNODES)(
  IN FDT_NODE_HANDLE      *Node,
  IN OUT FDT_NODE_HANDLE  *Cache,
  IN UINT32               Count
  );

/**
  Gets UINT64 property list slice.

  @param[in]      Node      Pointer to the node handle whose property is read.
  @param[in]      PropName  Name of the property to read.
  @param[in, out] PropList  Buffer to receive Count UINT64 values, starting at Index.
  @param[in]      Index     Zero-based starting index within the property's value list.
  @param[in]      Count     Number of values to extract.

  @retval DTB_ERR_NOERROR  PropList was populated successfully.
  @retval Others              A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_GET_UINT64_PROP_LIST_SLICE)(
  IN FDT_NODE_HANDLE  *Node,
  IN CONST CHAR8      *PropName,
  IN OUT UINT64       *PropList,
  IN UINT32           Index,
  IN UINT32           Count
  );

/**
  Merges overlay blob with primary blob.

  Grows/copies PrimaryBlob into MergeBlob (via fdt_open_into) and applies OverlayBlob to it
  in place (via fdt_overlay_apply). Both PrimaryBlob and OverlayBlob are validated as
  well-formed device tree blobs before merging.

  @param[in]      PrimaryBlob  Pointer to the primary (base) device tree blob.
  @param[in]      PbSize       Size of PrimaryBlob, in bytes.
  @param[in]      OverlayBlob  Pointer to the overlay device tree blob to apply.
  @param[in]      ObSize       Size of OverlayBlob, in bytes.
  @param[in, out] MergeBlob    Buffer to receive the merged device tree blob.
  @param[in]      MergeSize    Size of the MergeBlob buffer, in bytes; must be large enough
                                to hold PrimaryBlob grown by the contents of OverlayBlob.

  @retval DTB_ERR_NOERROR   The overlay was merged successfully.
  @retval -DTB_ERR_BUF2SMALL  MergeBlob is not large enough to hold the merged blob.
  @retval Others               A negative DTB_ERR_ (or underlying libfdt) error code indicating the failure reason.
**/
typedef INT32 (*DTB_MERGE_OVERLAY)(
  IN CONST VOID  *PrimaryBlob,
  IN UINTN       PbSize,
  IN VOID        *OverlayBlob,
  IN UINTN       ObSize,
  IN OUT VOID    *MergeBlob,
  IN UINTN       MergeSize
  );

/**
  Qualcomm Device Tree Extension Protocol structure.
**/
typedef struct {
  ///
  /// Protocol interface version, DTB_EXTN_PROTOCOL_VERSION.
  ///
  UINT64                                     Version;
  DTB_INIT_ROOT_HANDLE_FOR_DRIVER            FdtInitRootHandleForDriver;
  DTB_GET_NAME_INDEX                         FdtGetNameIndex;
  DTB_GET_NEXT_NODE_HANDLE_FOR_COMPATIBLE    FdtGetNextNodeHandleForCompatible;
  DTB_GET_REG                                FdtGetReg;
  DTB_GET_COUNT_OF_SUBNODES                  FdtGetCountOfSubnodes;
  DTB_GET_NODE_HANDLE                        FdtGetNodeHandle;
  DTB_GET_PARENT_NODE                        FdtGetParentNode;
  DTB_GET_PHANDLE_NODE                       FdtGetPhandleNode;
  DTB_GET_PROP_NAMES_OF_NODE                 FdtGetPropNamesOfNode;
  DTB_GET_PROP_NAMES_SIZE_OF_NODE            FdtGetPropNamesSizeOfNode;
  DTB_GET_PROP_VALUES_OF_NODE                FdtGetPropValuesOfNode;
  DTB_GET_PROP_VALUES_SIZE_OF_NODE           FdtGetPropValuesSizeOfNode;
  DTB_GET_SIZE_OF_SUBNODE_NAMES              FdtGetSizeOfSubnodeNames;
  DTB_GET_SUBNODE_NAMES                      FdtGetSubnodeNames;
  DTB_NODE_CMP                               FdtNodeCmp;
  DTB_NODE_COPY                              FdtNodeCopy;
  DTB_GET_BOOLEAN_PROP                       FdtGetBooleanProp;
  DTB_GET_PROP_SIZE                          FdtGetPropSize;
  DTB_GET_STRING_PROP_LIST                   FdtGetStringPropList;
  DTB_GET_UINT32_PROP                        FdtGetUint32Prop;
  DTB_GET_UINT32_PROP_LIST                   FdtGetUint32PropList;
  DTB_GET_UINT32_PROP_LIST_SLICE             FdtGetUint32PropListSlice;
  DTB_GET_UINT64_PROP                        FdtGetUint64Prop;
  DTB_GET_UINT64_PROP_LIST                   FdtGetUint64PropList;
  DTB_GET_UINT8_PROP                         FdtGetUint8Prop;
  DTB_GET_UINT8_PROP_LIST                    FdtGetUint8PropList;
  DTB_GET_UINT8_PROP_LIST_SLICE              FdtGetUint8PropListSlice;
  DTB_GET_UINT16_PROP                        FdtGetUint16Prop;
  DTB_GET_UINT16_PROP_LIST                   FdtGetUint16PropList;
  DTB_GET_UINT16_PROP_LIST_SLICE             FdtGetUint16PropListSlice;
  DTB_GET_CACHE_OF_SUBNODES                  FdtGetCacheOfSubnodes;
  DTB_GET_UINT64_PROP_LIST_SLICE             FdtGetUint64PropListSlice;
  DTB_SET_BLOB_HANDLE                        FdtSetBlobHandle;
  DTB_CHECK_VALID_BLOB                       FdtCheckValidBlob;
  DTB_MERGE_OVERLAY                          FdtMergeOverlay;
  DTB_GET_BLOB_HANDLE                        FdtGetBlobHandle;
  DTB_GET_BLOB_SIZE                          FdtGetBlobSize;
  DTB_INIT_ROOT_HANDLE_FOR_DRIVER_BY_ID      FdtInitRootHandleForDriverById;
  DTB_GET_NODE_HANDLE_BY_BLOB                FdtGetNodeHandleByBlob;
  DTB_INIT_ROOT_HANDLE_FOR_DRIVER_BY_BLOB    FdtInitRootHandleForDriverByBlob;
  DTB_GET_BOOL_PROP                          FdtGetBoolProp;
} DTB_EXTN_PROTOCOL;
