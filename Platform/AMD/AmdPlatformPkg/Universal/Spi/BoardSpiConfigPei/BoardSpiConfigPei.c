/** @file

  Board SPI configuration PEIM. This is the PEI-phase analogue of
  BoardSpiConfigDxe: it publishes EFI_PEI_SPI_CONFIGURATION_PPI describing the SPI
  bus and NOR flash peripheral present on the FCH SPI controller so the SPI bus
  PEIM can enumerate it. The shared board-configuration data model
  (EFI_SPI_BUS, EFI_SPI_PERIPHERAL, EFI_SPI_PART) is reused from
  Protocol/SpiConfiguration.h.

  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/PeiServicesLib.h>
#include <Protocol/DevicePath.h>
#include <Spi/AmdSpiHcChipSelectParameters.h>
#include <Protocol/SpiConfiguration.h>
#include <Spi/AmdSpiDevicePaths.h>
#include <Ppi/SpiConfiguration.h>

SPI_CONTROLLER_DEVICE_PATH  mFchDevicePath = FCH_DEVICE_PATH;

CHIP_SELECT_PARAMETERS  ChipSelect1 = CHIP_SELECT_1;

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
    &gEdk2JedecSfdpSpiPeiDriverGuid, // *SpiPeripheralDriverGuid
    &Mx25u6435f,                     // *SpiPart
    MHz (0),                         // MaxClockHz, set in BuildSpiList()
    1,                               // ClockPolarity
    0,                               // ClockPhase
    0,                               // Attributes, only support 1 bit bus width
    NULL,                            // *ConfigurationData
    NULL,                            // *SpiBus
    NULL,                            // ChipSelect()
    (VOID *)&ChipSelect1             // *ChipSelectParameter
  }
};

EFI_SPI_BUS  mSpiBus1 = {
  L"FCH SPI BUS",                               // FriendlyName
  NULL,                                         // Peripherallist
  (EFI_DEVICE_PATH_PROTOCOL *)&mFchDevicePath,  // ControllerPath
  NULL,                                         // Clock
  NULL                                          // ClockParameter
};

CONST EFI_SPI_BUS *CONST  mSpiBusList[] = {
  &mSpiBus1
};

EFI_PEI_SPI_CONFIGURATION_PPI  mBoardSpiConfigPpi = {
  0x1,                              // BusCount
  mSpiBusList                       // BusList
};

EFI_PEI_PPI_DESCRIPTOR  mBoardSpiConfigPpiDescriptor = {
  EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gEfiPeiSpiConfigurationPpiGuid,
  &mBoardSpiConfigPpi
};

/**
  Build the SPI peripheral list.

  Resolves the reset SPI clock speed from PcdResetSpiSpeed and links the single
  NOR flash peripheral onto the FCH SPI bus.

  @retval EFI_SUCCESS  The SPI peripheral list is built successfully.
**/
EFI_STATUS
EFIAPI
BuildSpiList (
  VOID
  )
{
  UINT32  MaxClockHz;

  DEBUG ((DEBUG_INFO, "%a: Entry\n", __func__));
  // PcdResetSpiSpeed encoding matches FCH SPI controller speed select:
  //   1 = 66 MHz, 2 = 33 MHz, 3 = 22 MHz, 4 = 16 MHz, 5 = 100 MHz, 6 = 800 KHz
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

  // Single peripheral on the single SpiBus
  mPeripherallist[0].SpiBus     = &mSpiBus1;
  mPeripherallist[0].MaxClockHz = MaxClockHz;

  // Put Peripheral list in bus
  mSpiBus1.Peripherallist = &mPeripherallist[0];

  DEBUG ((DEBUG_INFO, "%a: Exit\n", __func__));
  return EFI_SUCCESS;
}

/**
  Entry point of the Board SPI Configuration PEIM.

  @param[in] FileHandle   Handle of the file being invoked.
  @param[in] PeiServices  Pointer to the PEI Services Table.

  @retval EFI_SUCCESS  The SPI configuration PPI was installed successfully.
  @retval Otherwise    Failed to install the SPI configuration PPI.
**/
EFI_STATUS
EFIAPI
BoardSpiConfigPeiEntry (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "%a - ENTRY\n", __func__));

  Status = BuildSpiList ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: BuildSpiList failed: %r\n", __func__, Status));
    return Status;
  }

  Status = PeiServicesInstallPpi (&mBoardSpiConfigPpiDescriptor);

  DEBUG ((DEBUG_INFO, "%a - EXIT (Status = %r)\n", __func__, Status));
  return Status;
}
