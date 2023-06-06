/** @file

   Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

#include <Library/BoardConfigLib.h>
#include "GpioTableAdlPPostMem.h"
#include <Library/PeiServicesLib.h>
#include <Library/GpioLib.h>
#include <Library/IoLib.h>
#include <PlatformBoardId.h>
#include <PlatformBoardConfig.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Library/PchInfoLib.h>
#include <Library/HobLib.h>



/**
  Alderlake P boards configuration init function for PEI post memory phase.

  @retval EFI_SUCCESS             The function completed successfully.
**/
EFI_STATUS
EFIAPI
AdlPInit (
  VOID
  )
{
  UINT16            GpioCount;
  UINTN             Size;
  EFI_STATUS        Status;
  GPIO_INIT_CONFIG  *GpioTable;
  //
  // GPIO Table Init
  //
  Status = EFI_SUCCESS;
  GpioCount = 0;
  Size = 0;
  GpioTable = NULL;
  //
  // GPIO Table Init
  //
  //
  // GPIO Table Init, Update PostMem GPIO table to PcdBoardGpioTable
  //
  GpioTable = (GPIO_INIT_CONFIG *)PcdGetPtr(VpdPcdBoardGpioTable);

  GetGpioTableSize (GpioTable, &GpioCount);
  //
  // Increase GpioCount for the zero terminator.
  //
  GpioCount ++;
  Size = (UINTN) (GpioCount * sizeof (GPIO_INIT_CONFIG));
  Status = PcdSetPtrS (PcdBoardGpioTable, &Size, GpioTable);
  ASSERT_EFI_ERROR (Status);

  PcdSet8S (PcdSataPortsEnable0, 0x1);

  return Status;
}

/**
  Board I2C pads termination configuration init function for PEI pre-memory phase.
**/
VOID
AdlPSerialIoI2cPadsTerminationInit (
  VOID
  )
{
}

/**
  Configures GPIO

  @param[in]  GpioTable       Point to Platform Gpio table
  @param[in]  GpioTableCount  Number of Gpio table entries

**/
VOID
ConfigureGpio (
  IN GPIO_INIT_CONFIG                 *GpioDefinition,
  IN UINT16                           GpioTableCount
  )
{
  DEBUG ((DEBUG_INFO, "ConfigureGpio() Start\n"));

  GpioConfigurePads (GpioTableCount, GpioDefinition);

  DEBUG ((DEBUG_INFO, "ConfigureGpio() End\n"));
}

/**
  Misc. init function for PEI post memory phase.
**/
VOID
AdlPBoardMiscInit (
  VOID
  )
{
  UINT16    BoardId;
  BoardId = PcdGet16 (PcdBoardId);

  PcdSetBoolS (PcdSataLedEnable, FALSE);
  PcdSetBoolS (PcdVrAlertEnable, FALSE);

  switch (BoardId) {
    case BoardIdAdlPDdr5Rvp:
      PcdSet8S (PcdPcieSlot1RootPort, 8);
      PcdSetBoolS (PcdPcieSlot1PwrEnableGpioPolarity, PIN_GPIO_ACTIVE_LOW);
      break;
  }
  //
  // MIPI CAM
  //
  PcdSetBoolS (PcdMipiCamGpioEnable, TRUE);

  switch (BoardId) {
    case BoardIdAdlPDdr5Rvp:
      //
      // PCH M.2 SSD and Sata port
      //
      PcdSet32S (PcdPchSsd1PwrEnableGpioNo, GPIO_VER2_LP_GPP_D16);              // PCH M.2 SSD power enable gpio pin
      PcdSetBoolS (PcdPchSsd1PwrEnableGpioPolarity, PIN_GPIO_ACTIVE_HIGH);      // PCH M.2 SSD power enable gpio pin polarity

      break;
  }


  return;
}

/**
  PMC-PD solution enable init lib
**/
VOID
AdlPBoardPmcPdInit (
  VOID
  )
{
  PcdSetBoolS (PcdBoardPmcPdEnable, 1);
}


/**
  Configure GPIO, TouchPanel, HDA, PMC, TBT etc.

  @retval  EFI_SUCCESS   Operation success.
**/
EFI_STATUS
EFIAPI
AdlPBoardInitBeforeSiliconInit (
  VOID
  )
{
  AdlPInit ();

  AdlPSerialIoI2cPadsTerminationInit ();
  AdlPBoardMiscInit ();
  AdlPBoardPmcPdInit ();
  GpioInit (PcdGetPtr (PcdBoardGpioTable));



  return EFI_SUCCESS;
}
VOID
AdlPBoardSpecificGpioInitPostMem (
  VOID
  )
{
  UINT16    BoardId;
  BoardId = PcdGet16 (PcdBoardId);
  //
  // Assign FingerPrint, Gnss, Bluetooth & TouchPanel relative GPIO.
  //
  switch (BoardId) {
    case BoardIdAdlPDdr5Rvp:
      //
      //Update PcdBoardRtd3TableSignature per Setup
      //
      PcdSet64S (PcdBoardRtd3TableSignature, SIGNATURE_64 ('A', 'd', 'l', 'P', '_', 'R', 'v', 'p'));
      //
      // Update OEM table ID
      //
      PcdSet64S (PcdXhciAcpiTableSignature, SIGNATURE_64 ('x', 'h', '_', 'a', 'd', 'l', 'L', 'P'));
      break;
    default:
      //
      //Update PcdBoardRtd3TableSignature per Setup
      //
      PcdSet64S (PcdBoardRtd3TableSignature, SIGNATURE_64 ('A', 'd', 'l', 'P', '_', 'R', 'v', 'p'));
      //
      // Update OEM table ID
      //
      PcdSet64S (PcdXhciAcpiTableSignature, SIGNATURE_64 ('x', 'h', '_', 'a', 'd', 'l', 'L', 'P'));
      break;
  }
  //Modify Preferred_PM_Profile field based on Board SKU's. Default is set to Mobile
  //
  PcdSet8S (PcdPreferredPmProfile, EFI_ACPI_2_0_PM_PROFILE_MOBILE);
  if (PcdGet8 (PcdPlatformFlavor) == FlavorUpServer) {
    PcdSet8S (PcdPreferredPmProfile, EFI_ACPI_2_0_PM_PROFILE_ENTERPRISE_SERVER);
  }
}

VOID
AdlPInitCommonPlatformPcdPostMem (
  VOID
  )
{
  UINT16                          BoardId;
  BoardId = PcdGet16(PcdBoardId);

  PcdSetBoolS (PcdPssReadSN, TRUE);
  PcdSet8S (PcdPssI2cSlaveAddress, 0x6E);
  PcdSet8S (PcdPssI2cBusNumber, 0x05);
  PcdSetBoolS (PcdSpdAddressOverride, FALSE);

  //
  // Battery Present
  // Real & Virtual battery is need to supported in all except Desktop
  //
      PcdSet8S (PcdBatteryPresent, BOARD_REAL_BATTERY_SUPPORTED | BOARD_VIRTUAL_BATTERY_SUPPORTED);
  //
  // Real Battery 1 Control & Real Battery 2 Control
  //
  PcdSet8S (PcdRealBattery1Control, 1);
  PcdSet8S (PcdRealBattery2Control, 2);


  //
  // H8S2113 SIO, UART
  //
  PcdSetBoolS (PcdH8S2113SIO, FALSE);
    PcdSetBoolS (PcdH8S2113UAR, FALSE);
  //
  // NCT6776F COM, SIO & HWMON
  //
  PcdSetBoolS (PcdNCT6776FCOM, FALSE);
  PcdSetBoolS (PcdNCT6776FSIO, FALSE);
  PcdSetBoolS (PcdNCT6776FHWMON, FALSE);
  //
  // SMC Runtime Sci Pin
  // EC will use eSpi interface to generate SCI
  //
  PcdSet32S (PcdSmcRuntimeSciPin, 0x00);

  //
  // Virtual Button Volume Up & Done Support
  // Virtual Button Home Button Support
  // Virtual Button Rotation Lock Support
  //
  PcdSetBoolS (PcdVirtualButtonVolumeUpSupport, TRUE);
  PcdSetBoolS (PcdVirtualButtonVolumeDownSupport, TRUE);
  PcdSetBoolS (PcdVirtualButtonHomeButtonSupport, TRUE);
  PcdSetBoolS (PcdVirtualButtonRotationLockSupport, TRUE);
  //
  // Slate Mode Switch Support
  //
  PcdSetBoolS (PcdSlateModeSwitchSupport, TRUE);
  //
  // Virtual Gpio Button Support
  //
  PcdSetBoolS (PcdVirtualGpioButtonSupport, TRUE);

  //
  // Acpi Enable All Button Support
  //
  PcdSetBoolS (PcdAcpiEnableAllButtonSupport, TRUE);
  //
  // Acpi Hid Driver Button Support
  //
  PcdSetBoolS (PcdAcpiHidDriverButtonSupport, TRUE);

  //
  // ADL-P supports EC-PD design, for communication between EC and PD.
  //
  PcdSetBoolS (PcdUsbcEcPdNegotiation, TRUE);


}

/**
  Board init for PEI after Silicon initialized

  @retval  EFI_SUCCESS   Operation success.
**/
EFI_STATUS
EFIAPI
AdlPBoardInitAfterSiliconInit (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "ADLPBoardInitAfterSiliconInit \n"));
  //AdlPBoardSpecificGpioInitPostMem ();
  AdlPInitCommonPlatformPcdPostMem ();

  return EFI_SUCCESS;
}
