/** @file

  SPI HC platform library implementation. This code touches the SPI controllers and performs
  the hardware transaction

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/SpiHc.h>
#include <Spi/AmdSpiHcChipSelectParameters.h>
#include <Spi/AmdSpiDevicePaths.h>
#include <Library/PciSegmentLib.h>
#include <Library/SpiHcPlatformLib.h>
#include "SpiHcInternal.h"
#include <IndustryStandard/SpiNorFlashJedecSfdp.h>
#include <FchRegistersCommon.h>

extern EFI_PHYSICAL_ADDRESS  mHcAddress;

SPI_CONTROLLER_DEVICE_PATH  mFchDevicePath = FCH_DEVICE_PATH;

/**
  This function reports the device path of SPI host controller. This is needed in order for the SpiBus
  to match the correct SPI_BUS to the SPI host controller

  @param[out] DevicePath The device path for this SPI HC is returned in this variable

  @retval EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
GetSpiHcDevicePath (
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath
  )
{
  *DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)&mFchDevicePath;
  return EFI_SUCCESS;
}

/**
  This is the platform specific Spi Chip select function.
  Assert or de-assert the SPI chip select.

  This routine is called at TPL_NOTIFY.
  Update the value of the chip select line for a SPI peripheral. The SPI bus
  layer calls this routine either in the board layer or in the SPI controller
  to manipulate the chip select pin at the start and end of a SPI transaction.

  @param[in] This           Pointer to an EFI_SPI_HC_PROTOCOL structure.
  @param[in] SpiPeripheral  The address of an EFI_SPI_PERIPHERAL data structure
                            describing the SPI peripheral whose chip select pin
                            is to be manipulated. The routine may access the
                            ChipSelectParameter field to gain sufficient
                            context to complete the operation.
  @param[in] PinValue       The value to be applied to the chip select line of
                            the SPI peripheral.

  @retval EFI_SUCCESS            The chip select was set as requested
  @retval EFI_NOT_READY          Support for the chip select is not properly
                                 initialized
  @retval EFI_INVALID_PARAMETER  The ChipSelect value or its contents are
                                 invalid

**/
EFI_STATUS
EFIAPI
PlatformSpiHcChipSelect (
  IN CONST EFI_SPI_HC_PROTOCOL  *This,
  IN CONST EFI_SPI_PERIPHERAL   *SpiPeripheral,
  IN BOOLEAN                    PinValue
  )
{
  EFI_STATUS              Status;
  CHIP_SELECT_PARAMETERS  *ChipSelectParameter;

  Status              = EFI_DEVICE_ERROR;
  ChipSelectParameter = SpiPeripheral->ChipSelectParameter;

  if (ChipSelectParameter->OrValue <= 1) {
    MmioAndThenOr8 (
      mHcAddress + FCH_SPI_MMIO_REG1D,
      ChipSelectParameter->AndValue,
      ChipSelectParameter->OrValue
      );
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_INVALID_PARAMETER;
  }

  return Status;
}

/**
  This function is the platform specific SPI clock function.
  Set up the clock generator to produce the correct clock frequency, phase and
  polarity for a SPI chip.

  This routine is called at TPL_NOTIFY.
  This routine updates the clock generator to generate the correct frequency
  and polarity for the SPI clock.

  @param[in] This           Pointer to an EFI_SPI_HC_PROTOCOL structure.
  @param[in] SpiPeripheral  Pointer to a EFI_SPI_PERIPHERAL data structure from
                            which the routine can access the ClockParameter,
                            ClockPhase and ClockPolarity fields. The routine
                            also has access to the names for the SPI bus and
                            chip which can be used during debugging.
  @param[in] ClockHz        Pointer to the requested clock frequency. The SPI
                            host controller will choose a supported clock
                            frequency which is less then or equal to this
                            value. Specify zero to turn the clock generator
                            off. The actual clock frequency supported by the
                            SPI host controller will be returned.

  @retval EFI_SUCCESS      The clock was set up successfully
  @retval EFI_UNSUPPORTED  The SPI controller was not able to support the
                           frequency requested by ClockHz

**/
EFI_STATUS
EFIAPI
PlatformSpiHcClock (
  IN CONST EFI_SPI_HC_PROTOCOL  *This,
  IN CONST EFI_SPI_PERIPHERAL   *SpiPeripheral,
  IN UINT32                     *ClockHz
  )
{
  EFI_STATUS  Status;
  UINT32      InternalClockHz;
  UINT16      InternalClockValue;

  Status             = EFI_SUCCESS;
  InternalClockHz    = *ClockHz;
  InternalClockValue = 0x00;

  // this value is non-zero and less than the value in the EFI_SPI_PART then this value is used for the maximum clock frequency for the SPI part.
  if ((SpiPeripheral->SpiPart->MaxClockHz != 0) &&
      (SpiPeripheral->SpiPart->MaxClockHz < InternalClockHz))
  {
    InternalClockHz = SpiPeripheral->SpiPart->MaxClockHz;
  }

  if ((SpiPeripheral->MaxClockHz != 0) &&
      (SpiPeripheral->MaxClockHz < InternalClockHz))
  {
    InternalClockHz = SpiPeripheral->MaxClockHz;
  }

  if ((SpiPeripheral->SpiPart->MinClockHz != 0) &&
      (SpiPeripheral->SpiPart->MinClockHz > InternalClockHz))
  {
    Status = EFI_UNSUPPORTED;
  }

  if (!EFI_ERROR (Status)) {
    if (InternalClockHz >= MHz (100)) {
      InternalClockValue = 0x4;
    } else if (InternalClockHz >= MHz (66)) {
      InternalClockValue = 0x0;
    } else if (InternalClockHz >= MHz (33)) {
      InternalClockValue = 0x1;
    } else if (InternalClockHz >= MHz (22)) {
      InternalClockValue = 0x2;
    } else if (InternalClockHz >= MHz (16)) {
      InternalClockValue = 0x3;
    } else if (InternalClockHz >= KHz (800)) {
      InternalClockValue = 0x5;
    } else {
      Status = EFI_UNSUPPORTED;
    }

    if (!EFI_ERROR (Status)) {
      // Enable UseSpi100
      MmioOr8 (
        mHcAddress + FCH_SPI_MMIO_REG20,
        BIT0
        );
      // Set the Value for NormSpeed and FastSpeed
      InternalClockValue = InternalClockValue << 12 | InternalClockValue << 8;
      MmioAndThenOr16 (
        mHcAddress + FCH_SPI_MMIO_REG22,
        0x00FF,
        InternalClockValue
        );
    }
  }

  return Status;
}

/**
  This function is the platform specific SPI transaction function
  Perform the SPI transaction on the SPI peripheral using the SPI host
  controller.

  This routine is called at TPL_NOTIFY.
  This routine synchronously returns EFI_SUCCESS indicating that the
  asynchronous SPI transaction was started. The routine then waits for
  completion of the SPI transaction prior to returning the final transaction
  status.

  @param[in] This            Pointer to an EFI_SPI_HC_PROTOCOL structure.
  @param[in] BusTransaction  Pointer to a EFI_SPI_BUS_ TRANSACTION containing
                             the description of the SPI transaction to perform.

  @retval EFI_SUCCESS         The transaction completed successfully
  @retval EFI_BAD_BUFFER_SIZE The BusTransaction->WriteBytes value is invalid,
                              or the BusTransaction->ReadinBytes value is
                              invalid
  @retval EFI_UNSUPPORTED     The BusTransaction-> Transaction Type is
                              unsupported
  @retval EFI_DEVICE_ERROR    SPI Host Controller failed transaction

**/
EFI_STATUS
EFIAPI
PlatformSpiHcTransaction (
  IN CONST EFI_SPI_HC_PROTOCOL  *This,
  IN EFI_SPI_BUS_TRANSACTION    *BusTransaction
  )
{
  EFI_STATUS            Status;
  UINT8                 Opcode;
  UINT32                WriteBytes;
  UINT8                 *WriteBuffer;
  UINT32                ReadBytes;
  UINT8                 *ReadBuffer;
  EFI_PHYSICAL_ADDRESS  HcAddress;

  WriteBytes  = BusTransaction->WriteBytes;
  WriteBuffer = BusTransaction->WriteBuffer;

  ReadBytes  = BusTransaction->ReadBytes;
  ReadBuffer = BusTransaction->ReadBuffer;

  if (  (WriteBytes > This->MaximumTransferBytes + 6)
     || (ReadBytes > (This->MaximumTransferBytes + 6 - WriteBytes))
     || ((WriteBytes != 0) && (WriteBuffer == NULL))
     || ((ReadBytes != 0) && (ReadBuffer == NULL)))
  {
    return EFI_BAD_BUFFER_SIZE;
  }

  Opcode = 0;
  if (WriteBytes >= 1) {
    Opcode = WriteBuffer[0];
    // Skip Opcode
    WriteBytes  -= 1;
    WriteBuffer += 1;
  }

  Status    = EFI_SUCCESS;
  HcAddress = mHcAddress;

  Status = FchSpiControllerNotBusy ();
  if (!EFI_ERROR (Status)) {
    MmioWrite8 (
      HcAddress + FCH_SPI_MMIO_REG48_TX_BYTE_COUNT,
      (UINT8)WriteBytes
      );
    MmioWrite8 (
      HcAddress + FCH_SPI_MMIO_REG4B_RX_BYTE_COUNT,
      (UINT8)ReadBytes
      );

    // Fill in Write Data including Address
    if (WriteBytes != 0) {
      MmioWriteBuffer8 (
        HcAddress + FCH_SPI_MMIO_REG80_FIFO,
        WriteBytes,
        WriteBuffer
        );
    }

    // Set Opcode
    MmioWrite8 (
      HcAddress + FCH_SPI_MMIO_REG45_CMDCODE,
      Opcode
      );

    // Execute the Transaction
    Status = FchSpiExecute ();
    if (!EFI_ERROR (Status)) {
      if (ReadBytes != 0) {
        MmioReadBuffer8 (
          HcAddress
          + FCH_SPI_MMIO_REG80_FIFO
          + WriteBytes,
          ReadBytes,
          ReadBuffer
          );
      }
    }
  }

  return Status;
}
