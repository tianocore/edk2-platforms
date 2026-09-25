/** @file

  FV block I/O protocol driver for SPI flash libary.

  Copyright (C) Advanced Micro Devices, Inc., or its affiliates.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Spi/AmdSpiHcChipSelectParameters.h>
#include <Protocol/SpiSmmConfiguration.h>
#include <Library/IoLib.h>
#include <FchRegistersCommon.h>
#include <Spi/AmdSpiDevicePaths.h>

SPI_CONTROLLER_DEVICE_PATH  mFchDevicePath = FCH_DEVICE_PATH;

CHIP_SELECT_PARAMETERS  ChipSelect1 = CHIP_SELECT_1;
CHIP_SELECT_PARAMETERS  ChipSelect2 = CHIP_SELECT_2;

EFI_HANDLE  mSpiConfigHandle = NULL;

CONST EFI_SPI_PART  Mx25u6435f = {
  L"Macronix",                      // Vendor
  L"MX25U6435F",                    // PartNumber
  0,                                // MinClockHz
  MHz (104),                        // MaxClockHz
  FALSE                             // ChipSelectPolarity
};

EFI_SPI_PERIPHERAL  mPeripherallist[] = {
  {                                  // Flash Memory = SPI ROM
    NULL,                            // *NextSpiPeripheral
    L"Flash Memory",                 // *FriendlyName
    &gEdk2JedecSfdpSpiSmmDriverGuid, // *SpiPeripheralDriverGuid
    &Mx25u6435f,                     // *SpiPart
    MHz (0),                         // MaxClockHz, set in BuildSpiList()
    1,                               // ClockPolarity
    0,                               // ClockPhase
    0,                               // Attributes
    NULL,                            // *ConfigurationData
    NULL,                            // *SpiBus
    NULL,                            // ChipSelect()
    (VOID *)&ChipSelect1             // *ChipSelectParameter
  }
};

EFI_SPI_BUS  mSpiBus1 = {
  L"FCH SPI BUS",                                 // FriendlyName
  NULL,                                           // Peripherallist
  (EFI_DEVICE_PATH_PROTOCOL *)&mFchDevicePath,    // ControllerPath
  NULL,                                           // Clock
  NULL                                            // ClockParameter
};

CONST EFI_SPI_BUS *CONST  mSpiBusList[] = {
  &mSpiBus1
};

EFI_SPI_CONFIGURATION_PROTOCOL  mBoardSpiConfigProtocol = {
  0x1,                              // BusCount
  mSpiBusList                       // BusList
};

/**
  Check if SAFS mode is enabled. Valid eSPI SAFS RomType configs:
  2'b01: Boot from eSPI SAFS Channel with address high bits = 0x3F.
  2'b10: Boot from eSPI SAFS Channel (with address high bits = 0x00).

  @retval TRUE                   SAFS mode is enabled.
  @retval FALSE                  MAFS mode is enabled

**/
BOOLEAN
EFIAPI
IsEspiSafsMode (
  )
{
  UINT32  Misc80;
  UINT32  RomType;

  Misc80 = MmioRead32 (ACPI_MMIO_BASE + MISC_BASE + FCH_MISC_REG80);

  // RomType_1 is BIT3, RomType_0 is BIT1
  RomType = ((Misc80 & BIT3) >> 2) | ((Misc80 & BIT1) >> 1);

  if ((RomType == 0x1) || (RomType == 0x2)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Build the SPI list

  @retval EFI_SUCCESS           The SPI list is built successfully.
  @retval EFI_DEVICE_ERROR      Failed to build the SPI list.
**/
EFI_STATUS
EFIAPI
BuildSpiList (
  )
{
  EFI_STATUS                Status;
  UINTN                     Index;
  CONST EFI_SPI_BUS         **SpiBus;
  CONST EFI_SPI_PERIPHERAL  **Peripheral;
  UINT32                    MaxClockHz;

  DEBUG ((DEBUG_INFO, "%a: Entry\n", __func__));
  Status     = EFI_DEVICE_ERROR;
  MaxClockHz = 0;
  switch (PcdGet8 (PcdResetSpiSpeed)) {
    case 1:
      MaxClockHz = MHz (66);
      break;
    case 2:
      MaxClockHz = MHz (33);
      break;
    case 3:
      MaxClockHz = MHz (22);
      break;
    case 4:
      MaxClockHz = MHz (16);
      break;
    case 5:
      MaxClockHz = MHz (100);
      break;
    case 6:
      MaxClockHz = KHz (800);
      break;
    default:
      MaxClockHz = MHz (33);
      break;
  }

  // Update all Peripherals for the single SpiBus
  for (Index = 0;
       Index < (sizeof (mPeripherallist) / sizeof (EFI_SPI_PERIPHERAL));
       Index++)
  {
    DEBUG ((
      DEBUG_INFO,
      "%a: Setting up SpiPeripheral: %s, by %s, PN=%s\n",
      __func__,
      mPeripherallist[Index].FriendlyName,
      mPeripherallist[Index].SpiPart->Vendor,
      mPeripherallist[Index].SpiPart->PartNumber
      ));
    // Put bus address in peripheral
    SpiBus  = (CONST EFI_SPI_BUS **)&mPeripherallist[Index].SpiBus;
    *SpiBus = (CONST EFI_SPI_BUS *)&mSpiBus1;

    if (Index == 0) {
      mPeripherallist[Index].MaxClockHz = MaxClockHz;
    }

    if (Index > 0) {
      // Fill NextSpiPeripheral
      Peripheral =
        (CONST EFI_SPI_PERIPHERAL **)&mPeripherallist[Index - 1].NextSpiPeripheral;
      *Peripheral = &mPeripherallist[Index];
    }
  }

  // Put Peripheral list in bus
  Peripheral  = (CONST EFI_SPI_PERIPHERAL **)&mSpiBus1.Peripherallist;
  *Peripheral = &mPeripherallist[0];

  Status = EFI_SUCCESS;
  DEBUG ((DEBUG_INFO, "%a: Exit Status=%r\n", __func__, Status));
  return Status;
}

/**
  Entry point of the Board SPI Configuration driver.

  @param ImageHandle  Image handle of this driver.
  @param SystemTable  Pointer to standard EFI system table.

  @retval EFI_SUCCESS       Succeed.
  @retval EFI_DEVICE_ERROR  Fail to install EFI_SPI_SMM_HC_PROTOCOL protocol.
**/
EFI_STATUS
EFIAPI
BoardSpiConfigProtocolEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "%a - ENTRY\n", __func__));

  if (IsEspiSafsMode ()) {
    Status = gSmst->SmmInstallProtocolInterface (
                      &mSpiConfigHandle,
                      &gEdk2EspiSmmDriverProtocolGuid,
                      EFI_NATIVE_INTERFACE,
                      NULL
                      );
    DEBUG ((DEBUG_INFO, "%a - eSPI SAFS boot mode detected (Status = %r)\n", __func__, Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "%a - Default (SPIROM) boot mode detected!\n", __func__));
  Status = BuildSpiList ();

  Status = gSmst->SmmInstallProtocolInterface (
                    &mSpiConfigHandle,
                    &gEfiSpiSmmConfigurationProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mBoardSpiConfigProtocol
                    );

  DEBUG ((DEBUG_INFO, "%a - EXIT (Status = %r)\n", __func__, Status));

  return Status;
}
