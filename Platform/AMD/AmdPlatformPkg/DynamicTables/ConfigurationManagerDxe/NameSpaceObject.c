/** @file
  This file implements the Get/Set function for the Configuration Manager.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Library/DebugLib.h>
#include <AcpiTableGenerator.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include "ConfigurationManager.h"

/** A helper function for returning the Configuration Manager Objects.
  @param [in]       CmObjectId     The Configuration Manager Object ID.
  @param [in]       Object         Pointer to the Object(s).
  @param [in]       ObjectSize     Total size of the Object(s).
  @param [in]       ObjectCount    Number of Objects.
  @param [in, out]  CmObjectDesc   Pointer to the Configuration Manager Object
                                   descriptor describing the requested Object.
  @retval EFI_SUCCESS              Success.
**/
STATIC
EFI_STATUS
EFIAPI
HandleCmObject (
  IN  CONST CM_OBJECT_ID                CmObjectId,
  IN        VOID                        *Object,
  IN  CONST UINTN                       ObjectSize,
  IN  CONST UINTN                       ObjectCount,
  IN  OUT   CM_OBJ_DESCRIPTOR   *CONST  CmObjectDesc
  )
{
  if ((ObjectSize > MAX_UINT32) || (ObjectCount > MAX_UINT32)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Object == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CmObjectDesc->ObjectId = CmObjectId;
  CmObjectDesc->Size     = (UINT32)ObjectSize;
  CmObjectDesc->Data     = (VOID *)Object;
  CmObjectDesc->Count    = (UINT32)ObjectCount;
  DEBUG ((
    DEBUG_INFO,
    "INFO: CmObjectId = %x, Ptr = 0x%p, Size = %d, Count = %d\n",
    CmObjectId,
    CmObjectDesc->Data,
    CmObjectDesc->Size,
    CmObjectDesc->Count
    ));
  return EFI_SUCCESS;
}

/** A helper function for setting the Configuration Manager Objects.
  @param [in]       CmObjectId     The Configuration Manager Object ID.
  @param [out]      Object         Pointer to the Object(s).
  @param [in]       ObjectSize     Total size of the Object(s).
  @param [in]       ObjectCount    Number of Objects.
  @param [in]       CmObjectDesc   Pointer to the Configuration Manager Object
                                   descriptor describing the requested Object.
  @retval EFI_SUCCESS              Success.
**/
STATIC
EFI_STATUS
EFIAPI
SetHandleCmObject (
  IN  CONST CM_OBJECT_ID                CmObjectId,
  OUT       VOID                        *Object,
  IN  CONST UINTN                       ObjectSize,
  IN  CONST UINTN                       ObjectCount,
  IN        CM_OBJ_DESCRIPTOR   *CONST  CmObjectDesc
  )
{
  DEBUG ((
    DEBUG_INFO,
    "INFO: Received CmObjectId = %x, Ptr = 0x%p, Size = %d, Count = %d\n",
    CmObjectId,
    CmObjectDesc->Data,
    CmObjectDesc->Size,
    CmObjectDesc->Count
    ));
  if ((CmObjectDesc->Size != ObjectSize) || (CmObjectDesc->Count != ObjectCount)) {
    return EFI_BAD_BUFFER_SIZE;
  }

  CopyMem (Object, CmObjectDesc->Data, (ObjectSize * ObjectCount));

  return EFI_SUCCESS;
}

/** A helper function for setting the Configuration Manager Objects.
  @param [in]       CmObjectId     The Configuration Manager Object ID.
  @param [out]      Object         Pointer to the Object(s).
  @param [in]       ObjectSize     Total size of the Object(s).
  @param [in]       ObjectCount    Number of Objects.
  @param [in]       CmObjectDesc   Pointer to the Configuration Manager Object
                                   descriptor describing the requested Object.
  @retval EFI_SUCCESS              Success.
  @retval EFI_INVALID_PARAMETER    A parameter is invalid.
  @retval EFI_BAD_BUFFER_SIZE      The buffer size is invalid.
  @retval EFI_OUT_OF_RESOURCES     The buffer allocation failed.
  @retval EFI_BUFFER_TOO_SMALL     The buffer is too small.
**/
STATIC
EFI_STATUS
EFIAPI
SetHandleCmObjectBuffer (
  IN        CM_OBJECT_ID                CmObjectId,
  OUT       VOID                        **Object,
  IN  CONST UINTN                       ObjectSize,
  IN        UINTN                       *ObjectCount,
  IN        CM_OBJ_DESCRIPTOR   *CONST  CmObjectDesc
  )
{
  EFI_STATUS  Status;
  VOID        *Buffer;

  if ((Object == NULL) ||
      (*Object == NULL) ||
      (ObjectCount == NULL) ||
      (CmObjectDesc == NULL)
      )
  {
    return EFI_INVALID_PARAMETER;
  }

  if (*ObjectCount == 0) {
    return EFI_BUFFER_TOO_SMALL;
  }

  if (CmObjectDesc->Count != *ObjectCount) {
    Buffer = AllocateZeroPool (CmObjectDesc->Size * CmObjectDesc->Count);
    if (Buffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    *ObjectCount = CmObjectDesc->Count;
  } else {
    Buffer = *Object;
  }

  Status = SetHandleCmObject (CmObjectId, Buffer, CmObjectDesc->Size, *ObjectCount, CmObjectDesc);
  if ((!EFI_ERROR (Status)) && (Buffer != *Object)) {
    FreePool (*Object);
    *Object = Buffer;
  }

  return Status;
}

/** Return a standard namespace object.

  @param [in]        This        Pointer to the Configuration Manager Protocol.
  @param [in]        CmObjectId  The Configuration Manager Object ID.
  @param [in]        Token       An optional token identifying the object. If
                                 unused this must be CM_NULL_TOKEN.
  @param [in, out]   CmObject    Pointer to the Configuration Manager Object
                                 descriptor describing the requested Object.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          The required object information is not found.
**/
EFI_STATUS
EFIAPI
GetStandardNameSpaceObject (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  EFI_STATUS                      Status;
  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo;
  UINT32                          AcpiTableCount;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status         = EFI_NOT_FOUND;
  PlatformRepo   = This->PlatRepoInfo;
  AcpiTableCount = ARRAY_SIZE (PlatformRepo->CmAcpiTableList);

  switch (GET_CM_OBJECT_ID (CmObjectId)) {
    case EStdObjCfgMgrInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->CmInfo,
                 sizeof (PlatformRepo->CmInfo),
                 1,
                 CmObject
                 );
      break;
    case EStdObjAcpiTableList:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->CmAcpiTableList,
                 sizeof (PlatformRepo->CmAcpiTableList),
                 AcpiTableCount,
                 CmObject
                 );
      break;
    default:
    {
      Status = EFI_NOT_FOUND;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Object 0x%x. Status = %r\n",
        CmObjectId,
        Status
        ));
      break;
    }
  }

  return Status;
}

/** Return an architecture namespace object.

  @param [in]        This        Pointer to the Configuration Manager Protocol.
  @param [in]        CmObjectId  The Configuration Manager Object ID.
  @param [in]        Token       An optional token identifying the object. If
                                 unused this must be CM_NULL_TOKEN.
  @param [in, out]   CmObject    Pointer to the Configuration Manager Object
                                 descriptor describing the requested Object.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          The required object information is not found.
**/
EFI_STATUS
EFIAPI
GetArchNameSpaceObject (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  EFI_STATUS                      Status;
  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status       = EFI_NOT_FOUND;
  PlatformRepo = This->PlatRepoInfo;

  switch (GET_CM_OBJECT_ID (CmObjectId)) {
    case EArchCommonObjPowerManagementProfileInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->PowerManagementProfile,
                 sizeof (PlatformRepo->PowerManagementProfile),
                 1,
                 CmObject
                 );
      break;

    case EArchCommonObjHypervisorVendorIdentity:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->HypervisorVendorId,
                 sizeof (PlatformRepo->HypervisorVendorId),
                 1,
                 CmObject
                 );
      break;

    case EArchCommonObjFixedFeatureFlags:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->FixedFeatureFlags,
                 sizeof (PlatformRepo->FixedFeatureFlags),
                 1,
                 CmObject
                 );
      break;

    case EArchCommonObjSpmiInterfaceInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->SpmiInterfaceInfo,
                 sizeof (PlatformRepo->SpmiInterfaceInfo),
                 1,
                 CmObject
                 );
      break;
    case EArchCommonObjPciConfigSpaceInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 PlatformRepo->PciConfigSpaceInfo,
                 sizeof (*(PlatformRepo->PciConfigSpaceInfo)) * PlatformRepo->PciConfigSpaceInfoCount,
                 PlatformRepo->PciConfigSpaceInfoCount,
                 CmObject
                 );
      break;
    default:
    {
      Status = EFI_NOT_FOUND;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Object 0x%x. Status = %r\n",
        CmObjectId,
        Status
        ));
      break;
    }
  }

  return Status;
}

/** Set the data for an architecture namespace object.

  @param [in]        This        Pointer to the Configuration Manager Protocol.
  @param [in]        CmObjectId  The Configuration Manager Object ID.
  @param [in]        Token       An optional token identifying the object. If
                                 unused this must be CM_NULL_TOKEN.
  @param [in]        CmObject    Pointer to the Configuration Manager Object
                                 descriptor describing the requested Object.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          The required object information is not found.
**/
EFI_STATUS
EFIAPI
SetArchNameSpaceObject (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN        CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  EFI_STATUS                      Status;
  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status       = EFI_NOT_FOUND;
  PlatformRepo = This->PlatRepoInfo;

  switch (GET_CM_OBJECT_ID (CmObjectId)) {
    case EArchCommonObjPowerManagementProfileInfo:
      Status = SetHandleCmObject (
                 CmObjectId,
                 &PlatformRepo->PowerManagementProfile,
                 sizeof (PlatformRepo->PowerManagementProfile),
                 1,
                 CmObject
                 );
      break;

    case EArchCommonObjHypervisorVendorIdentity:
      Status = SetHandleCmObject (
                 CmObjectId,
                 &PlatformRepo->HypervisorVendorId,
                 sizeof (PlatformRepo->HypervisorVendorId),
                 1,
                 CmObject
                 );
      break;

    case EArchCommonObjFixedFeatureFlags:
      Status = SetHandleCmObject (
                 CmObjectId,
                 &PlatformRepo->FixedFeatureFlags,
                 sizeof (PlatformRepo->FixedFeatureFlags),
                 1,
                 CmObject
                 );
      break;
    case EArchCommonObjPciConfigSpaceInfo:
      Status = SetHandleCmObjectBuffer (
                 CmObjectId,
                 (VOID **)&PlatformRepo->PciConfigSpaceInfo,
                 sizeof (*(PlatformRepo->PciConfigSpaceInfo)) * PlatformRepo->PciConfigSpaceInfoCount,
                 (VOID *)&PlatformRepo->PciConfigSpaceInfoCount,
                 CmObject
                 );
      break;
    default:
    {
      Status = EFI_NOT_FOUND;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Object 0x%x. Status = %r\n",
        CmObjectId,
        Status
        ));
      break;
    }
  }

  return Status;
}

/** Return an architecture namespace object.

  @param [in]        This        Pointer to the Configuration Manager Protocol.
  @param [in]        CmObjectId  The Configuration Manager Object ID.
  @param [in]        Token       An optional token identifying the object. If
                                 unused this must be CM_NULL_TOKEN.
  @param [in, out]   CmObject    Pointer to the Configuration Manager Object
                                 descriptor describing the requested Object.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          The required object information is not found.
**/
EFI_STATUS
EFIAPI
GetX64NameSpaceObject (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  EFI_STATUS                      Status;
  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status       = EFI_NOT_FOUND;
  PlatformRepo = This->PlatRepoInfo;

  switch (GET_CM_OBJECT_ID (CmObjectId)) {
    case EX64ObjFadtSciInterrupt:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->SciInterrupt,
                 sizeof (PlatformRepo->SciInterrupt),
                 1,
                 CmObject
                 );
      break;
    case EX64ObjFadtSciCmdInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->SciCmdinfo,
                 sizeof (PlatformRepo->SciCmdinfo),
                 1,
                 CmObject
                 );
      break;
    case EX64ObjFadtPmBlockInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->PmBlockInfo,
                 sizeof (PlatformRepo->PmBlockInfo),
                 1,
                 CmObject
                 );
      break;
    case EX64ObjFadtGpeBlockInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->GpeBlockInfo,
                 sizeof (PlatformRepo->GpeBlockInfo),
                 1,
                 CmObject
                 );
      break;
    case EX64ObjFadtXpmBlockInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->XpmBlockInfo,
                 sizeof (PlatformRepo->XpmBlockInfo),
                 1,
                 CmObject
                 );
      break;
    case EX64ObjFadtXgpeBlockInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->XgpeBlockInfo,
                 sizeof (PlatformRepo->XgpeBlockInfo),
                 1,
                 CmObject
                 );
      break;
    case EX64ObjFadtSleepBlockInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->SleepBlockInfo,
                 sizeof (PlatformRepo->SleepBlockInfo),
                 1,
                 CmObject
                 );
      break;
    case EX64ObjFadtResetBlockInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->ResetBlockInfo,
                 sizeof (PlatformRepo->ResetBlockInfo),
                 1,
                 CmObject
                 );
      break;
    case EX64ObjFadtMiscInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->FadtMiscInfo,
                 sizeof (PlatformRepo->FadtMiscInfo),
                 1,
                 CmObject
                 );
      break;
    case EX64ObjHpetInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->HpetInfo,
                 sizeof (PlatformRepo->HpetInfo),
                 1,
                 CmObject
                 );
      break;
    case EX64ObjWsmtFlagsInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->WsmtFlagsInfo,
                 sizeof (PlatformRepo->WsmtFlagsInfo),
                 1,
                 CmObject
                 );
      break;
    case EX64ObjMadtInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 &PlatformRepo->MadtInfo,
                 sizeof (PlatformRepo->MadtInfo),
                 1,
                 CmObject
                 );
      break;
    case EX64ObjLocalApicX2ApicInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 PlatformRepo->LocalApicX2ApicInfo,
                 sizeof (*(PlatformRepo->LocalApicX2ApicInfo)) * PlatformRepo->LocalApicX2ApicInfoCount,
                 PlatformRepo->LocalApicX2ApicInfoCount,
                 CmObject
                 );
      break;
    case EX64ObjIoApicInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 PlatformRepo->IoApicInfo,
                 sizeof (*(PlatformRepo->IoApicInfo)) * PlatformRepo->IoApicInfoCount,
                 PlatformRepo->IoApicInfoCount,
                 CmObject
                 );
      break;
    case EX64ObjIntrSourceOverrideInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 PlatformRepo->IntrSourceOverrideInfo,
                 sizeof (*(PlatformRepo->IntrSourceOverrideInfo)) * PlatformRepo->IntrSourceOverrideInfoCount,
                 PlatformRepo->IntrSourceOverrideInfoCount,
                 CmObject
                 );
      break;
    case EX64ObjLocalApicX2ApicNmiInfo:
      Status = HandleCmObject (
                 CmObjectId,
                 PlatformRepo->LocalApicX2ApicNmiInfo,
                 sizeof (*(PlatformRepo->LocalApicX2ApicNmiInfo)) * PlatformRepo->LocalApicX2ApicNmiInfoCount,
                 PlatformRepo->LocalApicX2ApicNmiInfoCount,
                 CmObject
                 );
      break;
    default:
    {
      Status = EFI_NOT_FOUND;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Object 0x%x. Status = %r\n",
        CmObjectId,
        Status
        ));
      break;
    }
  }

  return Status;
}

/**
  Set the data for X64 namespace object.

  @param [in]        This        Pointer to the Configuration Manager Protocol.
  @param [in]        CmObjectId  The Configuration Manager Object ID.
  @param [in]        Token       An optional token identifying the object.
                                 If unused this must be CM_NULL_TOKEN.
  @param [in]        CmObject    Pointer to the Configuration Manager Object.
**/
EFI_STATUS
EFIAPI
SetX64NameSpaceObject (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN        CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  EFI_STATUS                      Status;
  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  Status       = EFI_NOT_FOUND;
  PlatformRepo = This->PlatRepoInfo;

  switch (GET_CM_OBJECT_ID (CmObjectId)) {
    case EX64ObjFadtSciInterrupt:
      Status = SetHandleCmObject (
                 CmObjectId,
                 &PlatformRepo->SciInterrupt,
                 sizeof (PlatformRepo->SciInterrupt),
                 1,
                 CmObject
                 );
      break;
    case EX64ObjFadtSciCmdInfo:
      Status = SetHandleCmObject (
                 CmObjectId,
                 &PlatformRepo->SciCmdinfo,
                 sizeof (PlatformRepo->SciCmdinfo),
                 1,
                 CmObject
                 );
      break;
    case EX64ObjFadtPmBlockInfo:
      Status = SetHandleCmObject (
                 CmObjectId,
                 &PlatformRepo->PmBlockInfo,
                 sizeof (PlatformRepo->PmBlockInfo),
                 1,
                 CmObject
                 );
      break;
    case EX64ObjFadtGpeBlockInfo:
      Status = SetHandleCmObject (
                 CmObjectId,
                 &PlatformRepo->GpeBlockInfo,
                 sizeof (PlatformRepo->GpeBlockInfo),
                 1,
                 CmObject
                 );
      break;
    case EX64ObjFadtXpmBlockInfo:
      Status = SetHandleCmObject (
                 CmObjectId,
                 &PlatformRepo->XpmBlockInfo,
                 sizeof (PlatformRepo->XpmBlockInfo),
                 1,
                 CmObject
                 );
      break;
    case EX64ObjFadtXgpeBlockInfo:
      Status = SetHandleCmObject (
                 CmObjectId,
                 &PlatformRepo->XgpeBlockInfo,
                 sizeof (PlatformRepo->XgpeBlockInfo),
                 1,
                 CmObject
                 );
      break;
    case EX64ObjFadtSleepBlockInfo:
      Status = SetHandleCmObject (
                 CmObjectId,
                 &PlatformRepo->SleepBlockInfo,
                 sizeof (PlatformRepo->SleepBlockInfo),
                 1,
                 CmObject
                 );
      break;
    case EX64ObjFadtResetBlockInfo:
      Status = SetHandleCmObject (
                 CmObjectId,
                 &PlatformRepo->ResetBlockInfo,
                 sizeof (PlatformRepo->ResetBlockInfo),
                 1,
                 CmObject
                 );
      break;
    case EX64ObjFadtMiscInfo:
      Status = SetHandleCmObject (
                 CmObjectId,
                 &PlatformRepo->FadtMiscInfo,
                 sizeof (PlatformRepo->FadtMiscInfo),
                 1,
                 CmObject
                 );
      break;
    case EX64ObjHpetInfo:
      Status = SetHandleCmObject (
                 CmObjectId,
                 &PlatformRepo->HpetInfo,
                 sizeof (PlatformRepo->HpetInfo),
                 1,
                 CmObject
                 );
      break;
    case EX64ObjWsmtFlagsInfo:
      Status = SetHandleCmObject (
                 CmObjectId,
                 &PlatformRepo->WsmtFlagsInfo,
                 sizeof (PlatformRepo->WsmtFlagsInfo),
                 1,
                 CmObject
                 );
      break;
    case EX64ObjMadtInfo:
      Status = SetHandleCmObject (
                 CmObjectId,
                 &PlatformRepo->MadtInfo,
                 sizeof (PlatformRepo->MadtInfo),
                 1,
                 CmObject
                 );
      break;
    case EX64ObjLocalApicX2ApicInfo:
      Status = SetHandleCmObjectBuffer (
                 CmObjectId,
                 (VOID **)&PlatformRepo->LocalApicX2ApicInfo,
                 sizeof (*(PlatformRepo->LocalApicX2ApicInfo)) * PlatformRepo->LocalApicX2ApicInfoCount,
                 (VOID *)&PlatformRepo->LocalApicX2ApicInfoCount,
                 CmObject
                 );
      break;
    case EX64ObjIoApicInfo:
      Status = SetHandleCmObjectBuffer (
                 CmObjectId,
                 (VOID **)&PlatformRepo->IoApicInfo,
                 sizeof (*(PlatformRepo->IoApicInfo)) * PlatformRepo->IoApicInfoCount,
                 (VOID *)&PlatformRepo->IoApicInfoCount,
                 CmObject
                 );
      break;
    case EX64ObjIntrSourceOverrideInfo:
      Status = SetHandleCmObjectBuffer (
                 CmObjectId,
                 (VOID **)&PlatformRepo->IntrSourceOverrideInfo,
                 sizeof (*(PlatformRepo->IntrSourceOverrideInfo)) * PlatformRepo->IntrSourceOverrideInfoCount,
                 (VOID *)&PlatformRepo->IntrSourceOverrideInfoCount,
                 CmObject
                 );
      break;
    case EX64ObjLocalApicX2ApicNmiInfo:
      Status = SetHandleCmObjectBuffer (
                 CmObjectId,
                 (VOID **)&PlatformRepo->LocalApicX2ApicNmiInfo,
                 sizeof (*(PlatformRepo->LocalApicX2ApicNmiInfo)) * PlatformRepo->LocalApicX2ApicNmiInfoCount,
                 (VOID *)&PlatformRepo->LocalApicX2ApicNmiInfoCount,
                 CmObject
                 );
      break;
    default:
      Status = EFI_NOT_FOUND;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Object 0x%x. Status = %r\n",
        CmObjectId,
        Status
        ));
      break;
  }

  return Status;
}

/** The GetObject function defines the interface implemented by the
    Configuration Manager Protocol for returning the Configuration
    Manager Objects.

  @param [in]        This        Pointer to the Configuration Manager Protocol.
  @param [in]        CmObjectId  The Configuration Manager Object ID.
  @param [in]        Token       An optional token identifying the object. If
                                 unused this must be CM_NULL_TOKEN.
  @param [in, out]   CmObject    Pointer to the Configuration Manager Object
                                 descriptor describing the requested Object.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          The required object information is not found.
**/
EFI_STATUS
EFIAPI
AmdPlatformGetObject (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  EFI_STATUS  Status;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  switch (GET_CM_NAMESPACE_ID (CmObjectId)) {
    case EObjNameSpaceStandard:
      Status = GetStandardNameSpaceObject (This, CmObjectId, Token, CmObject);
      break;

    case EObjNameSpaceArchCommon:
      Status = GetArchNameSpaceObject (This, CmObjectId, Token, CmObject);
      break;

    case EObjNameSpaceX64:
      Status = GetX64NameSpaceObject (This, CmObjectId, Token, CmObject);
      break;

    default:
    {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Unknown Namespace Object = 0x%x. Status = %r\n",
        CmObjectId,
        Status
        ));
      break;
    }
  }

  return Status;
}

/** The SetObject function defines the interface implemented by the
    Configuration Manager Protocol for updating the Configuration
    Manager Objects.

  @param [in]        This        Pointer to the Configuration Manager Protocol.
  @param [in]        CmObjectId  The Configuration Manager Object ID.
  @param [in]        Token       An optional token identifying the object. If
                                 unused this must be CM_NULL_TOKEN.
  @param [in]        CmObject    Pointer to the Configuration Manager Object
                                 descriptor describing the Object.

  @retval EFI_UNSUPPORTED        This operation is not supported.
**/
EFI_STATUS
EFIAPI
AmdPlatformSetObject (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN        CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  EFI_STATUS  Status;

  if ((This == NULL) || (CmObject == NULL)) {
    ASSERT (This != NULL);
    ASSERT (CmObject != NULL);
    return EFI_INVALID_PARAMETER;
  }

  switch (GET_CM_NAMESPACE_ID (CmObjectId)) {
    case EObjNameSpaceArchCommon:
      Status = SetArchNameSpaceObject (This, CmObjectId, Token, CmObject);
      break;

    case EObjNameSpaceX64:
      Status = SetX64NameSpaceObject (This, CmObjectId, Token, CmObject);
      break;

    default:
    {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Unknown Namespace Object = 0x%x. Status = %r\n",
        CmObjectId,
        Status
        ));
      break;
    }
  }

  return Status;
}
