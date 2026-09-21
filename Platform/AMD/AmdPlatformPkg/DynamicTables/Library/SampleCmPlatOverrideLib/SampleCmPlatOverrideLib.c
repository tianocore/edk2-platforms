/** @file

  Sample Code for CmPlatOverrideLib library. This library demonstrate the
    functionality to override the default configuration of the Configuration Manager
    Protocol.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <AmdConfigurationManager.h>

static EFI_EVENT  mConfigurationManagerProtocolInstalledEvent = NULL;
static VOID       *mConfigurationManagerProtocolRegistration  = NULL;

/**
  This function is used to override the default configuration of the Configuration Manager.

  @param[in] ConfigurationManager  A pointer to the Configuration Manager Protocol.

  @retval EFI_SUCCESS           The default configuration is successfully overridden.
  @retval EFI_INVALID_PARAMETER ConfigurationManager is NULL.
  @retval Others                The default configuration is not overridden.
**/
EFI_STATUS
EFIAPI
PlatformOverrideConfigurationManager (
  IN EDKII_CONFIGURATION_MANAGER_PROTOCOL  *ConfigurationManager
  )
{
  CM_ARCH_COMMON_POWER_MANAGEMENT_PROFILE_INFO  PmProfile;
  CM_OBJ_DESCRIPTOR                             PmProfileObjDesc;
  EFI_STATUS                                    Status;

  if (ConfigurationManager == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (ConfigurationManager->PlatRepoInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ///
  /// Example 1: Override the default PmProfile configuration using SetConfigurationData API.
  ///
  DEBUG ((DEBUG_INFO, "Override the default PmProfile configuration\n"));
  PmProfile.PowerManagementProfile = 0x02;
  PmProfileObjDesc.ObjectId        = CREATE_CM_OBJECT_ID (EObjNameSpaceArchCommon, EArchCommonObjPowerManagementProfileInfo);
  PmProfileObjDesc.Size            = sizeof (CM_ARCH_COMMON_POWER_MANAGEMENT_PROFILE_INFO);
  PmProfileObjDesc.Data            = (VOID *)&PmProfile;
  PmProfileObjDesc.Count           = 1;
  Status                           = ConfigurationManager->SetObject (
                                                             ConfigurationManager,
                                                             PmProfileObjDesc.ObjectId,
                                                             CM_NULL_TOKEN,
                                                             &PmProfileObjDesc
                                                             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to set PowerManagementProfile configuration with Status: %r\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  This function is called when the Configuration Manager Protocol is installed.
  It is used to override the default configuration of the Configuration Manager.

  @param[in] Event    The Event that is being processed.
  @param[in] Context  Event Context.
**/
VOID
EFIAPI
SampleCmPlatOverrideLibOnConfigurationManagerProtocolInstalled (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EDKII_CONFIGURATION_MANAGER_PROTOCOL  *ConfigurationManager;
  EFI_STATUS                            Status;

  gBS->CloseEvent (Event);
  mConfigurationManagerProtocolInstalledEvent = NULL;

  ///
  /// Locate gEdkiiConfigurationManagerProtocolGuid
  ///
  Status = gBS->LocateProtocol (
                  &gEdkiiConfigurationManagerProtocolGuid,
                  NULL,
                  (VOID **)&ConfigurationManager
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate Configuration Manager Protocol\n"));
    return;
  }

  Status =  PlatformOverrideConfigurationManager (ConfigurationManager);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "PlatformOverrideConfigurationManager failed with Status: %r\n", Status));
  }

  return;
}

/**
  The constructor function initializes the library.

  The constructor function locates the Configuration Manager Protocol and
  overrides the default configuration of the Configuration Manager.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS  The constructor executed successfully.
  @retval Others       The constructor did not complete successfully.
**/
EFI_STATUS
EFIAPI
SampleCmPlatOverrideLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EDKII_CONFIGURATION_MANAGER_PROTOCOL  *ConfigurationManager;
  EFI_STATUS                            Status;

  Status = gBS->LocateProtocol (
                  &gEdkiiConfigurationManagerProtocolGuid,
                  NULL,
                  (VOID **)&ConfigurationManager
                  );
  if (EFI_ERROR (Status)) {
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    SampleCmPlatOverrideLibOnConfigurationManagerProtocolInstalled,
                    NULL,
                    &mConfigurationManagerProtocolInstalledEvent
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to create event for Configuration Manager Protocol installation\n"));
      return Status;
    }

    Status = gBS->RegisterProtocolNotify (
                    &gEdkiiConfigurationManagerProtocolGuid,
                    mConfigurationManagerProtocolInstalledEvent,
                    &mConfigurationManagerProtocolRegistration
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to register for Configuration Manager Protocol installation\n"));
      return Status;
    }

    return EFI_SUCCESS;
  }

  Status =  PlatformOverrideConfigurationManager (ConfigurationManager);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "PlatformOverrideConfigurationManager failed with Status: %r\n", Status));
  }

  return Status;
}

/**
  The destructor function frees resources allocated during initialization.

  The destructor function frees any resources allocated during the initialization
  of the library.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS  The destructor executed successfully.
  @retval Others       The destructor did not complete successfully.
**/
EFI_STATUS
EFIAPI
SampleCmPlatOverrideLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  if (mConfigurationManagerProtocolInstalledEvent != NULL) {
    Status = gBS->CloseEvent (mConfigurationManagerProtocolInstalledEvent);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to close event for Configuration Manager Protocol installation\n"));
      return Status;
    }
  }

  return EFI_SUCCESS;
}
