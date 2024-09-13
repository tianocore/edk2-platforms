/** @file

  DISCLAIMER: the FDT_CLIENT_PROTOCOL introduced here is a work in progress,
  and should not be used outside of the EDK II tree.

  Copyright (C) 2023 Marvell
  Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __FDT_CLIENT_H__
#define __FDT_CLIENT_H__

#define FDT_CLIENT_PROTOCOL_GUID { \
  0xE11FACA0, 0x4710, 0x4C8E, {0xA7, 0xA2, 0x01, 0xBA, 0xA2, 0x59, 0x1B, 0x4C} \
  }

#define FdtToCpu32(Value) SwapBytes32(Value)
#define CpuToFdt32(Value) SwapBytes32(Value)

#define FdtToCpu64(Value) SwapBytes64(Value)
#define CpuToFdt64(Value) SwapBytes64(Value)

//
// Protocol interface structure
//
typedef int FDT_HANDLE;
#define FDT_START_HANDLE -1
typedef struct _MRVL_FDT_CLIENT_PROTOCOL MRVL_FDT_CLIENT_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *MRVL_FDT_CLIENT_GET_NODE_PROPERTY) (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  IN  FDT_HANDLE                Node,
  IN  CONST CHAR8               *PropertyName,
  OUT CONST VOID                **Prop,
  OUT UINT32                    *PropSize OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *MRVL_FDT_CLIENT_SET_NODE_PROPERTY) (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  IN  FDT_HANDLE                Node,
  IN  CONST CHAR8               *PropertyName,
  IN  CONST VOID                *Prop,
  IN  UINT32                    PropSize
  );

typedef
EFI_STATUS
(EFIAPI *MRVL_FDT_CLIENT_FIND_COMPATIBLE_NODE) (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  IN  CONST CHAR8               *CompatibleString,
  IN  FDT_HANDLE                PrevNode,
  OUT FDT_HANDLE                *Node
  );

typedef
EFI_STATUS
(EFIAPI *MRVL_FDT_CLIENT_FIND_COMPATIBLE_NODE_PROPERTY) (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  IN  CONST CHAR8               *CompatibleString,
  IN  CONST CHAR8               *PropertyName,
  OUT CONST VOID                **Prop,
  OUT UINT32                    *PropSize OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *MRVL_FDT_CLIENT_GET_OR_INSERT_CHOSEN_NODE) (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  OUT FDT_HANDLE                *Node
  );

typedef
EFI_STATUS
(EFIAPI *MRVL_FDT_CLIENT_GET_NODE_DEPTH) (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  IN  FDT_HANDLE                Node,
  OUT FDT_HANDLE                *Depth
);

typedef
EFI_STATUS
(EFIAPI *MRVL_FDT_CLIENT_GET_PARENT_NODE) (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  IN  FDT_HANDLE                Node,
  OUT FDT_HANDLE                *Parent
);

typedef
EFI_STATUS
(EFIAPI *MRVL_FDT_CLIENT_GET_NODE) (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  IN  CONST CHAR8               *Path,
  OUT FDT_HANDLE                *Node
);

typedef
EFI_STATUS
(EFIAPI *MRVL_FDT_CLIENT_GET_NODE_PATH) (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  IN  CONST FDT_HANDLE          Node,
  OUT CHAR8                     *Path,
  IN  INT32                     Size
);

typedef
EFI_STATUS
(EFIAPI *MRVL_FDT_CLIENT_GET_NODE_BY_PROPERTY_VALUE) (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  IN  CONST FDT_HANDLE          StartNode,
  IN  CHAR8                     *Property,
  IN  VOID                      *Value,
  IN  INT32                     Size,
  OUT FDT_HANDLE                *Node
);

typedef
EFI_STATUS
(EFIAPI *MRVL_FDT_CLIENT_GET_SUBNODE_BY_PROPERTY_VALUE) (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  IN  CONST FDT_HANDLE          Parent,
  IN  CHAR8                     *PropertyName,
  IN  VOID                      *PropertyValue,
  IN  INT32                     PropertyLength,
  OUT FDT_HANDLE                *Node
);

typedef
EFI_STATUS
(EFIAPI *MRVL_FDT_CLIENT_GET_NODE_BY_PHANDLE) (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  IN  CONST FDT_HANDLE          PHandle,
  OUT FDT_HANDLE                *Node
);

typedef
EFI_STATUS
(EFIAPI *MRVL_FDT_CLIENT_GET_FIRST_SUBNODE) (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  IN  CONST FDT_HANDLE          Parent,
  OUT FDT_HANDLE                *Node
  );

typedef
EFI_STATUS
(EFIAPI *MRVL_FDT_CLIENT_GET_NEXT_SUBNODE) (
  IN  MRVL_FDT_CLIENT_PROTOCOL  *This,
  IN  CONST FDT_HANDLE          Subnode,
  OUT FDT_HANDLE                *Next
  );

struct _MRVL_FDT_CLIENT_PROTOCOL {
  MRVL_FDT_CLIENT_GET_NODE_PROPERTY             GetNodeProperty;
  MRVL_FDT_CLIENT_SET_NODE_PROPERTY             SetNodeProperty;

  MRVL_FDT_CLIENT_FIND_COMPATIBLE_NODE          FindCompatibleNode;

  MRVL_FDT_CLIENT_GET_OR_INSERT_CHOSEN_NODE     GetOrInsertChosenNode;

  MRVL_FDT_CLIENT_GET_NODE_DEPTH                GetNodeDepth;
  MRVL_FDT_CLIENT_GET_PARENT_NODE               GetParentNode;
  MRVL_FDT_CLIENT_GET_NODE                      GetNode;
  MRVL_FDT_CLIENT_GET_NODE_PATH                 GetNodePath;
  MRVL_FDT_CLIENT_GET_NODE_BY_PROPERTY_VALUE    GetNodeByPropertyValue;
  MRVL_FDT_CLIENT_GET_SUBNODE_BY_PROPERTY_VALUE GetSubnodeByPropertyValue;
  MRVL_FDT_CLIENT_GET_NODE_BY_PHANDLE           GetNodeByPHandle;
  MRVL_FDT_CLIENT_GET_FIRST_SUBNODE             GetFirstSubnode;
  MRVL_FDT_CLIENT_GET_NEXT_SUBNODE              GetNextSubnode;

};

extern EFI_GUID gMrvlFdtClientProtocolGuid;

#endif
