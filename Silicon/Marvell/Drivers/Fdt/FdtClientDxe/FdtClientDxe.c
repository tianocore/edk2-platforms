/** @file
*  FDT client driver
*
*  Copyright (c) 2016, Cavium Inc. All rights reserved.<BR>
*  Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/HobLib.h>
#include <libfdt.h>

#include <Guid/FdtHob.h>

#include <Protocol/FdtClient.h>

STATIC VOID  *mDeviceTreeBase;

STATIC
EFI_STATUS
GetNodeProperty (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  IN  FDT_HANDLE                Node,
  IN  CONST CHAR8               *PropertyName,
  OUT CONST VOID                **Prop,
  OUT UINT32                    *PropSize OPTIONAL
  )
{
  INT32 Len;

  ASSERT (mDeviceTreeBase != NULL);
  ASSERT (Prop != NULL);

  *Prop = fdt_getprop (mDeviceTreeBase, Node, PropertyName, &Len);
  if (*Prop == NULL) {
    return EFI_NOT_FOUND;
  }

  if (PropSize != NULL) {
    *PropSize = Len;
  }
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
SetNodeProperty (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  IN  FDT_HANDLE                Node,
  IN  CONST CHAR8               *PropertyName,
  IN  CONST VOID                *Prop,
  IN  UINT32                    PropSize
  )
{
  INT32 Ret;

  ASSERT (mDeviceTreeBase != NULL);

  Ret = fdt_setprop (mDeviceTreeBase, Node, PropertyName, Prop, PropSize);
  if (Ret != 0) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
FindCompatibleNode (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  IN  CONST CHAR8               *CompatibleString,
  IN  FDT_HANDLE                PrevNode,
  OUT FDT_HANDLE                *Node
  )
{
  FDT_HANDLE  Offset;

  ASSERT (mDeviceTreeBase != NULL);
  ASSERT (Node != NULL);

  Offset = fdt_node_offset_by_compatible (mDeviceTreeBase, PrevNode, CompatibleString);

  if (Offset < 0) {
    return EFI_NOT_FOUND;
  }

  *Node = Offset;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
GetOrInsertChosenNode (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  OUT INT32                     *Node
  )
{
  INT32 NewNode;

  ASSERT (mDeviceTreeBase != NULL);
  ASSERT (Node != NULL);

  NewNode = fdt_path_offset (mDeviceTreeBase, "/chosen");

  if (NewNode < 0) {
    NewNode = fdt_add_subnode (mDeviceTreeBase, 0, "/chosen");
  }

  if (NewNode < 0) {
    return EFI_OUT_OF_RESOURCES;
  }

  *Node = NewNode;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
GetNodeDepth (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  IN  FDT_HANDLE                Node,
  OUT INT32                     *Depth
)
{
  *Depth = fdt_node_depth (mDeviceTreeBase, Node);

  if (*Depth < 0) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
GetParentNode (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  IN  FDT_HANDLE                Node,
  OUT FDT_HANDLE                *Parent
)
{
  *Parent = fdt_parent_offset (mDeviceTreeBase, Node);

  if (*Parent < 0) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
GetNode (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  IN  CONST CHAR8               *Path,
  OUT FDT_HANDLE                *Node
)
{
  *Node = fdt_path_offset (mDeviceTreeBase, Path);

  if (*Node < 0) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
GetNodePath (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  IN  CONST FDT_HANDLE          Node,
  OUT CHAR8                     *Path,
  IN  INT32                     Size
)
{
  INT32 Result;

  Result = fdt_get_path (mDeviceTreeBase, Node, Path, Size);

  if (Result < 0) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
GetNodeByPropertyValue (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  IN  CONST FDT_HANDLE          StartNode,
  IN  CHAR8                     *Property,
  IN  VOID                      *Value,
  IN  INT32                     Size,
  OUT FDT_HANDLE                *Node
)
{
  INT32          Offset;

  ASSERT (mDeviceTreeBase != NULL);
  ASSERT (Node != NULL);

  Offset = fdt_node_offset_by_prop_value (mDeviceTreeBase, StartNode,
                                          Property, Value,
                                          Size);

  if (Offset < 0) {
    DEBUG ((DEBUG_ERROR, "Result: %d\n", Offset));
    return EFI_NOT_FOUND;
  }

  *Node = Offset;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
GetSubnodeByPropertyValue(
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  IN  CONST FDT_HANDLE          Parent,
  IN  CHAR8                     *PropertyName,
  IN  VOID                      *PropertyValue,
  IN  INT32                     PropertyLength,
  OUT FDT_HANDLE                *Node
)
{
  INT32          Offset;
  CONST VOID     *Property;
  INT32          Length;

  ASSERT (mDeviceTreeBase != NULL);
  ASSERT (Node != NULL);

  Offset = fdt_first_subnode (mDeviceTreeBase, Parent);

  while (Offset > 0) {
    Property = fdt_getprop (mDeviceTreeBase, Offset, PropertyName, &Length);

    if ((Property != NULL) &&
        (PropertyLength == Length) &&
        (CompareMem (Property, PropertyValue, Length) == 0)) {
      *Node = Offset;
      return EFI_SUCCESS;
    }

    Offset = fdt_next_subnode(mDeviceTreeBase, Offset);
  }

  return EFI_NOT_FOUND;
}

STATIC
EFI_STATUS
GetNodeByPHandle (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  IN  CONST FDT_HANDLE          PHandle,
  OUT FDT_HANDLE                *Node
)
{
  INT32          Offset;

  ASSERT (mDeviceTreeBase != NULL);
  ASSERT (Node != NULL);

  Offset = fdt_node_offset_by_phandle (mDeviceTreeBase, PHandle);

  if (Offset < 0) {
    DEBUG ((DEBUG_ERROR, "Result: %d\n", Offset));
    return EFI_NOT_FOUND;
  }

  *Node = Offset;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
GetFirstSubnode (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  IN  CONST FDT_HANDLE          Parent,
  OUT FDT_HANDLE                *Node
)
{
  INT32          Offset;

  ASSERT (mDeviceTreeBase != NULL);
  ASSERT (Node != NULL);

  Offset = fdt_first_subnode (mDeviceTreeBase, Parent);

  if (Offset < 0) {
    DEBUG ((DEBUG_ERROR, "Result: %d\n", Offset));
    return EFI_NOT_FOUND;
  }

  *Node = Offset;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
GetNextSubnode (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  IN  CONST FDT_HANDLE          Subnode,
  OUT FDT_HANDLE                *Next
)
{
  INT32          Offset;

  ASSERT (mDeviceTreeBase != NULL);
  ASSERT (Next != NULL);

  Offset = fdt_next_subnode (mDeviceTreeBase, Subnode);

  if (Offset < 0) {
    DEBUG ((DEBUG_ERROR, "Result: %d\n", Offset));
    return EFI_NOT_FOUND;
  }

  *Next = Offset;

  return EFI_SUCCESS;
}

STATIC MRVL_FDT_CLIENT_PROTOCOL mFdtClientProtocol = {
  GetNodeProperty,
  SetNodeProperty,
  FindCompatibleNode,
  GetOrInsertChosenNode,
  GetNodeDepth,
  GetParentNode,
  GetNode,
  GetNodePath,
  GetNodeByPropertyValue,
  GetSubnodeByPropertyValue,
  GetNodeByPHandle,
  GetFirstSubnode,
  GetNextSubnode
};

EFI_STATUS
EFIAPI
InitializeFdtClientDxe (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  VOID              *Hob;
  VOID              *DeviceTreeBase;

  Hob = GetFirstGuidHob (&gFdtHobGuid);

  if (Hob == NULL) {
    return EFI_NOT_FOUND;
  }

  DeviceTreeBase = GET_GUID_HOB_DATA (Hob);
  mDeviceTreeBase = (VOID *)*(UINT64 *)DeviceTreeBase;
  if (fdt_check_header (mDeviceTreeBase)) {
    DEBUG ((DEBUG_ERROR, "No DTB found @ 0x%p\n", DeviceTreeBase));
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_INFO, "DTB @ 0x%p\n", mDeviceTreeBase));

  return gBS->InstallMultipleProtocolInterfaces (&ImageHandle,
                                        &gMrvlFdtClientProtocolGuid, &mFdtClientProtocol,
                                        NULL);
}
