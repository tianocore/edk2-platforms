/** @file

  FV block I/O protocol driver for SPI flash libary.

  Copyright (C) 2026 Advanced Micro Devices, Inc., or its affiliates.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Spi/AmdSpiHcChipSelectParameters.h>
#include <Protocol/SpiConfiguration.h>
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

CONST EFI_SPI_PART  FakeDevice = {
  L"Fake",                        // Vendor
  L"Fake Device",                 // PartNumber
  0,                              // MinClockHz
  MHz (22),                       // MaxClockHz
  FALSE                           // ChipSelectPolarity
};

EFI_SPI_PERIPHERAL  mPeripherallist[] = {
  {                                 // Flash Memory = SPI ROM
    NULL,                           // *NextSpiPeripheral
    L"Device",                      // *FriendlyName
    &gAmdPlatformFakeSpiDriverGuid, // *SpiPeripheralDriverGuid
    &FakeDevice,                    // *SpiPart
    MHz (33),                       // MaxClockHz
    1,                              // ClockPolarity
    0,                              // ClockPhase
    0,                              // Attributes
    NULL,                           // *ConfigurationData
    NULL,                           // *SpiBus
    NULL,                           // ChipSelect()
    (VOID *)&ChipSelect2            // *ChipSelectParameter
  },
  {                                  // Flash Memory = SPI ROM
    NULL,                            // *NextSpiPeripheral
    L"Flash Memory",                 // *FriendlyName
    &gEdk2JedecSfdpSpiDxeDriverGuid, // *SpiPeripheralDriverGuid
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

/*
EFI_DEVICE_PATH_PROTOCOL mDevicePath = {
  0x01,
  0xFF,
  {0x4}
};
*/

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

EFI_SPI_CONFIGURATION_PROTOCOL  mBoardSpiConfigProtocol = {
  0x1,                              // BusCount
  mSpiBusList                       // BusList
};

/**
  Return the reset SPI speed setting from PcdResetSpiSpeed.

  Declared as a weak symbol so unit tests can override it without modifying
  this source file.

  @return  PcdResetSpiSpeed value.
**/
UINT8
#ifdef __GNUC__
__attribute__ ((weak))
#endif
EFIAPI
GetResetSpiSpeed (
  VOID
  ) {
  return PcdGet8 (PcdResetSpiSpeed);
}

/**
  Build the SPI peripheral list.

  @retval EFI_SUCCESS       The SPI peripheral list is built successfully.
  @retval EFI_DEVICE_ERROR  Failed to build the SPI peripheral list.
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
  switch (GetResetSpiSpeed ()) {
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

    if (Index == 1) {
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

  Status = BuildSpiList ();

  Status = gBS->InstallProtocolInterface (
                  &mSpiConfigHandle,
                  &gEfiSpiConfigurationProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mBoardSpiConfigProtocol
                  );

  DEBUG ((DEBUG_INFO, "%a - EXIT (Status = %r)\n", __func__, Status));

  return Status;
}
