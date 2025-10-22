/** @file

  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
**/

#ifndef SOC_CONFIGURATION_MANAGER_H_
#define SOC_CONFIGURATION_MANAGER_H_

#include "ConfigurationManager.h"

/** The number of ACPI tables to install
*/
#define PLAT_ACPI_TABLE_COUNT  10

/** A helper macro for mapping a reference token
*/
#define REFERENCE_TOKEN_SOC(Field)                                \
  (CM_OBJECT_TOKEN)((UINT8*)&MorelloSocRepositoryInfo +           \
    OFFSET_OF (EDKII_SOC_PLATFORM_REPOSITORY_INFO, Field))

/** C array containing the compiled AML template.
    These symbols are defined in the auto generated C file
    containing the AML bytecode array.
*/
extern CHAR8  dsdtsoc_aml_code[];
extern CHAR8  ssdtpcisoc_aml_code[];

/** A structure describing the SoC Platform specific information
*/
typedef struct SocPlatformRepositoryInfo {
  /// List of ACPI tables
  CM_STD_OBJ_ACPI_TABLE_INFO              CmAcpiTableList[PLAT_ACPI_TABLE_COUNT];

  /// GIC ITS information
  CM_ARM_GIC_ITS_INFO                     GicItsInfo[4];

  /// ITS Group node
  CM_ARM_ITS_GROUP_NODE                   ItsGroupInfo[4];

  /// ITS Identifier array
  CM_ARM_ITS_IDENTIFIER                   ItsIdentifierArray[4];

  /// SMMUv3 node
  CM_ARM_SMMUV3_NODE                      SmmuV3Info[2];

  /// PCI Root complex node
  CM_ARM_ROOT_COMPLEX_NODE                RootComplexInfo[2];

  /// Array of DeviceID mapping
  CM_ARM_ID_MAPPING                       DeviceIdMapping[3][2];

  /// PCI configuration space information
  CM_ARCH_COMMON_PCI_CONFIG_SPACE_INFO    PciConfigInfo[2];
} EDKII_SOC_PLATFORM_REPOSITORY_INFO;

/** A structure describing the platform configuration
    manager repository information
*/
typedef struct PlatformRepositoryInfo {
  /// Common information
  EDKII_COMMON_PLATFORM_REPOSITORY_INFO    *CommonPlatRepoInfo;

  /// SoC Platform specific information
  EDKII_SOC_PLATFORM_REPOSITORY_INFO       *SocPlatRepoInfo;
} EDKII_PLATFORM_REPOSITORY_INFO;

extern EDKII_COMMON_PLATFORM_REPOSITORY_INFO  CommonPlatformInfo;

/** Return platform specific ARM namespace object.

  @param [in]      This        Pointer to the Configuration Manager Protocol.
  @param [in]      CmObjectId  The Configuration Manager Object ID.
  @param [in]      Token       An optional token identifying the object. If
                               unused this must be CM_NULL_TOKEN.
  @param [in, out] CmObject    Pointer to the Configuration Manager Object
                               descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
**/
/**
EFI_STATUS
EFIAPI
GetArchCommonNameSpaceObjectPlat (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  );
**/

/** Return platform specific ARM namespace object.

  @param [in]      This        Pointer to the Configuration Manager Protocol.
  @param [in]      CmObjectId  The Configuration Manager Object ID.
  @param [in]      Token       An optional token identifying the object. If
                               unused this must be CM_NULL_TOKEN.
  @param [in, out] CmObject    Pointer to the Configuration Manager Object
                               descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
**/
EFI_STATUS
EFIAPI
GetArmNameSpaceObjectPlat (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  );

/** Return platform specific standard namespace object.

  @param [in]      This        Pointer to the Configuration Manager Protocol.
  @param [in]      CmObjectId  The Configuration Manager Object ID.
  @param [in]      Token       An optional token identifying the object. If
                               unused this must be CM_NULL_TOKEN.
  @param [in, out] CmObject    Pointer to the Configuration Manager Object
                               descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
**/
EFI_STATUS
EFIAPI
GetStandardNameSpaceObjectPlat (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  );

#endif // SOC_CONFIGURATION_MANAGER_H_
