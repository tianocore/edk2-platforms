/** @file
  This file is SampleCode of the library for Intel PCH PEI Policy initialization.

   Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PeiPchPolicyUpdate.h"
#include <Guid/FmpCapsule.h>
#include <Guid/GlobalVariable.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/GpioConfig.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PchInfoLib.h>
#include <Library/PchPcieRpLib.h>
#include <Library/PeiSiPolicyUpdateLib.h>
#include <Library/SiPolicyLib.h>
#include <ConfigBlock.h>
#include <Ppi/Spi.h>
#include <PlatformBoardConfig.h>
#include <PolicyUpdateMacro.h>
#include <SerialIoDevices.h>
#include <Pins/GpioPinsVer2Lp.h>

/**
  This is helper function for getting I2C Pads Internal Termination settings from Pcd

  @param[in]  Index            I2C Controller Index
**/
UINT8
STATIC
GetSerialIoI2cPadsTerminationFromPcd (
  IN UINT8 Index
  )
{
  switch (Index) {
    case 0:
      return PcdGet8 (PcdPchSerialIoI2c0PadInternalTerm);
    case 1:
      return PcdGet8 (PcdPchSerialIoI2c1PadInternalTerm);
    case 2:
      return PcdGet8 (PcdPchSerialIoI2c2PadInternalTerm);
    case 3:
      return PcdGet8 (PcdPchSerialIoI2c3PadInternalTerm);
    case 4:
      return PcdGet8 (PcdPchSerialIoI2c4PadInternalTerm);
    case 5:
      return PcdGet8 (PcdPchSerialIoI2c5PadInternalTerm);
    case 6:
      return PcdGet8 (PcdPchSerialIoI2c6PadInternalTerm);
    case 7:
      return PcdGet8 (PcdPchSerialIoI2c7PadInternalTerm);
    default:
      ASSERT (FALSE); // Invalid I2C Controller Index
  }
  return 0;
}

/**
  This function performs PCH Serial IO Platform Policy initialization

  @param[in] SiPolicy             Pointer to SI_POLICY_PPI
  @param[in] FspsUpd              A VOID pointer
**/
VOID
UpdateSerialIoConfig (
  IN SI_POLICY_PPI             *SiPolicy,
  IN VOID                      *FspsUpd
  )
{
  UINT8              Index;
  SERIAL_IO_CONFIG   *SerialIoConfig;
  EFI_STATUS         Status;

  Status = GetConfigBlock ((VOID *) SiPolicy, &gSerialIoConfigGuid, (VOID *) &SerialIoConfig);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // I2C
  //
  for (Index = 0; Index < GetPchMaxSerialIoI2cControllersNum (); Index++) {
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.PchSerialIoI2cPadsTermination[Index], SerialIoConfig->I2cDeviceConfig[Index].PadTermination, GetSerialIoI2cPadsTerminationFromPcd (Index));
  }

  if (IsPchP ()) {
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoI2cMode[0],        SerialIoConfig->I2cDeviceConfig[0].Mode,                0);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoI2cMode[1],        SerialIoConfig->I2cDeviceConfig[1].Mode,                0);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoI2cMode[2],        SerialIoConfig->I2cDeviceConfig[2].Mode,                0);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoI2cMode[3],        SerialIoConfig->I2cDeviceConfig[3].Mode,                0);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoI2cMode[4],        SerialIoConfig->I2cDeviceConfig[4].Mode,                0);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoI2cMode[5],        SerialIoConfig->I2cDeviceConfig[5].Mode,                0);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoI2cMode[6],        SerialIoConfig->I2cDeviceConfig[6].Mode,                0);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoI2cMode[7],        SerialIoConfig->I2cDeviceConfig[7].Mode,                0);
  }

    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartMode[0],        SerialIoConfig->UartDeviceConfig[0].Mode,                2);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartMode[1],        SerialIoConfig->UartDeviceConfig[1].Mode,                0);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartMode[2],        SerialIoConfig->UartDeviceConfig[2].Mode,                0);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartMode[3],        SerialIoConfig->UartDeviceConfig[3].Mode,                0);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartMode[4],        SerialIoConfig->UartDeviceConfig[4].Mode,                0);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartMode[5],        SerialIoConfig->UartDeviceConfig[5].Mode,                0);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartMode[6],        SerialIoConfig->UartDeviceConfig[6].Mode,                0);

    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartAutoFlow[0],    SerialIoConfig->UartDeviceConfig[0].Attributes.AutoFlow, 1);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartAutoFlow[1],    SerialIoConfig->UartDeviceConfig[1].Attributes.AutoFlow, 1);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartAutoFlow[2],    SerialIoConfig->UartDeviceConfig[2].Attributes.AutoFlow, 1);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartAutoFlow[3],    SerialIoConfig->UartDeviceConfig[3].Attributes.AutoFlow, 0);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartAutoFlow[4],    SerialIoConfig->UartDeviceConfig[4].Attributes.AutoFlow, 0);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartAutoFlow[5],    SerialIoConfig->UartDeviceConfig[5].Attributes.AutoFlow, 0);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartAutoFlow[6],    SerialIoConfig->UartDeviceConfig[6].Attributes.AutoFlow, 0);

    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartPowerGating[0], SerialIoConfig->UartDeviceConfig[0].PowerGating,         2);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartPowerGating[1], SerialIoConfig->UartDeviceConfig[1].PowerGating,         2);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartPowerGating[2], SerialIoConfig->UartDeviceConfig[2].PowerGating,         2);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartPowerGating[3], SerialIoConfig->UartDeviceConfig[3].PowerGating,         0);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartPowerGating[4], SerialIoConfig->UartDeviceConfig[4].PowerGating,         0);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartPowerGating[5], SerialIoConfig->UartDeviceConfig[5].PowerGating,         0);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartPowerGating[6], SerialIoConfig->UartDeviceConfig[6].PowerGating,         0);

    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartDmaEnable[0],   SerialIoConfig->UartDeviceConfig[0].DmaEnable,           1);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartDmaEnable[1],   SerialIoConfig->UartDeviceConfig[1].DmaEnable,           1);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartDmaEnable[2],   SerialIoConfig->UartDeviceConfig[2].DmaEnable,           1);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartDmaEnable[3],   SerialIoConfig->UartDeviceConfig[3].DmaEnable,           0);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartDmaEnable[4],   SerialIoConfig->UartDeviceConfig[4].DmaEnable,           0);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartDmaEnable[5],   SerialIoConfig->UartDeviceConfig[5].DmaEnable,           0);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.SerialIoUartDmaEnable[6],   SerialIoConfig->UartDeviceConfig[6].DmaEnable,           0);

}


/**
  Update PCIe Root Port Configuration

  @param[in] SiPolicy             Pointer to SI_POLICY_PPI
  @param[in] FspsUpd              Pointer to FspsUpd structure
  // @param[in] PchSetup             Pointer to PCH_SETUP buffer
  // @param[in] SetupVariables       Pointer to SETUP_DATA buffer
**/
VOID
UpdatePcieRpConfig (
  IN SI_POLICY_PPI             *SiPolicy,
  IN VOID                      *FspsUpd
  )
{
  UINT8                           Index;
  EFI_STATUS                      Status;
  PCH_PCIE_CONFIG                 *PchPcieConfig;
  UINTN                           MaxPciePorts;

  MaxPciePorts = GetPchMaxPciePortNum ();

  PchPcieConfig = NULL;
  Status = GetConfigBlock ((VOID *) SiPolicy, &gPchPcieConfigGuid, (VOID *) &PchPcieConfig);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // PCI express config
  //
  for (Index = 0; Index < MaxPciePorts; Index++) {
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.PcieRpMaxPayload[Index],                    PchPcieConfig->RootPort[Index].PcieRpCommonConfig.MaxPayload,         PchPcieMaxPayload256);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.PcieRpPhysicalSlotNumber[Index],            PchPcieConfig->RootPort[Index].PcieRpCommonConfig.PhysicalSlotNumber, (UINT8) Index);
    UPDATE_POLICY (((FSPS_UPD *) FspsUpd)->FspsConfig.PcieRpClkReqDetect[Index],                  PchPcieConfig->RootPort[Index].PcieRpCommonConfig.ClkReqDetect,       TRUE);
  }
}

/**
  This function performs PCH PEI Policy initialization.

  @retval EFI_SUCCESS             The PPI is installed and initialized.
  @retval EFI ERRORS              The PPI is not successfully installed.
  @retval EFI_OUT_OF_RESOURCES    Do not have enough resources to initialize the driver
**/
EFI_STATUS
EFIAPI
UpdatePeiPchPolicy (
  VOID
  )
{
  EFI_STATUS                      Status;
  VOID                            *FspsUpd;
  SI_POLICY_PPI                   *SiPolicy;

  DEBUG ((DEBUG_INFO, "Update PeiPchPolicyUpdate Pos-Mem Start\n"));

  FspsUpd                 = NULL;
  SiPolicy                = NULL;

  Status = PeiServicesLocatePpi (&gSiPolicyPpiGuid, 0, NULL, (VOID **) &SiPolicy);
  ASSERT_EFI_ERROR (Status);

  UpdatePcieRpConfig (SiPolicy, FspsUpd);
  UpdateSerialIoConfig (SiPolicy, FspsUpd);

  return EFI_SUCCESS;
}
