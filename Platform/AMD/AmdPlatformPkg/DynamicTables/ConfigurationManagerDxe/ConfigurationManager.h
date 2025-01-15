/** @file

  Copyright (C) 2024 - 2025 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
**/

#ifndef CONFIGURATION_MANAGER_H_
#define CONFIGURATION_MANAGER_H_

#include <ConfigurationManagerObject.h>
#include <StandardNameSpaceObjects.h>
#include <X64NameSpaceObjects.h>
#include <ArchCommonNameSpaceObjects.h>
#include <ConfigurationManagerObject.h>
#include <AmdConfigurationManager.h>

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
  );

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
  );

/** Updates ACPI MCFG table information in the platform repository.

  @param [in]  PlatformRepo  Pointer to the platform repository.

  @retval EFI_SUCCESS        The ACPI MCFG table information is updated.
  @retval EFI_INVALID_PARAMETER  The input parameter is invalid.
  @retval EFI_UNSUPPORTED    The operation is not supported.
**/
EFI_STATUS
EFIAPI
UpdateMcfgTableInfo (
  IN  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo
  );

/** The UpdateMadtTable function updates the MADT table.

    @param [in, out]  PlatformRepo  Pointer to the platform repository information.

    @retval EFI_SUCCESS            The MADT table is updated successfully.
    @retval EFI_INVALID_PARAMETER  The input parameter is invalid.
**/
EFI_STATUS
EFIAPI
UpdateMadtTable (
  IN OUT EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo
  );

/**
  Update HPET table information in Platform Repository.

  @param[in]  PlatformRepo  The pointer to the Platform Repository.

  @retval EFI_SUCCESS  The HPET table information is updated successfully.
  @retval EFI_INVALID_PARAMETER  The input parameter is NULL.
**/
EFI_STATUS
EFIAPI
UpdateHpetTableInfo (
  IN  EDKII_PLATFORM_REPOSITORY_INFO  *PlatformRepo
  );

#endif // CONFIGURATION_MANAGER_H_
