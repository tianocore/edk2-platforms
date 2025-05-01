/** @file

  FV block I/O protocol driver for SPI flash libary.

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/SpiHc.h>
#include <FchRegistersCommon.h>
#include <Spi/AmdSpiHcChipSelectParameters.h>
#include "AmdSpiHc.h"
#include "AmdSpiHcInstance.h"
#include "AmdSpiHcInternal.h"
#include <Library/AmdPspRomArmorLib.h>

/**
  Assert or deassert the SPI chip select.

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
  @retval EFI_INVALID_PARAMETER  The ChipSeLect value or its contents are
                                 invalid

**/
EFI_STATUS
EFIAPI
ChipSelect (
  IN CONST EFI_SPI_HC_PROTOCOL  *This,
  IN CONST EFI_SPI_PERIPHERAL   *SpiPeripheral,
  IN BOOLEAN                    PinValue
  )
{
  // PinValue doesn't have any context here.  This HC only supports Chip Select
  // value of low active.  This will select which chip select will be toggled
  // on the subsequent transactions.
  EFI_STATUS Status;
  SPI_HOST_CONTROLLER_INSTANCE *Instance;
  CHIP_SELECT_PARAMETERS *ChipSelectParameter;

  Status = EFI_DEVICE_ERROR;
  Instance = SPI_HOST_CONTROLLER_FROM_THIS (This);

  ChipSelectParameter = SpiPeripheral->ChipSelectParameter;

  if (SpiPeripheral->ChipSelect == NULL &&
      ChipSelectParameter->OrValue <= 1) {
    if (!Instance->PspMailboxSpiMode) {
      MmioAndThenOr8 (Instance->HcAddress + FCH_SPI_MMIO_REG1D,
                      ChipSelectParameter->AndValue,
                      ChipSelectParameter->OrValue);
      Status = EFI_SUCCESS;
    } else {
      Instance->SpiCommunicationBuffer.SpiCommand[0].ChipSelect =
        ChipSelectParameter->OrValue + 1;
      Status = EFI_SUCCESS;
    }
  } else {
    Status = EFI_INVALID_PARAMETER;
  }

  return Status;
}

/**
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
Clock (
  IN CONST EFI_SPI_HC_PROTOCOL  *This,
  IN CONST EFI_SPI_PERIPHERAL   *SpiPeripheral,
  IN UINT32                      *ClockHz
  )
{
  EFI_STATUS Status;
  SPI_HOST_CONTROLLER_INSTANCE *Instance;
  UINT32 InternalClockHz;
  UINT16 InternalClockValue;

  Instance = SPI_HOST_CONTROLLER_FROM_THIS (This);

  Status = EFI_SUCCESS;
  InternalClockHz = *ClockHz;
  InternalClockValue = 0x00;
  if (SpiPeripheral->MaxClockHz != 0 &&
      SpiPeripheral->MaxClockHz < InternalClockHz) {
    InternalClockHz = SpiPeripheral->MaxClockHz;
  }
  if (SpiPeripheral->SpiPart->MaxClockHz != 0 &&
      SpiPeripheral->SpiPart->MaxClockHz < InternalClockHz) {
    InternalClockHz = SpiPeripheral->SpiPart->MaxClockHz;
  }
  if (SpiPeripheral->SpiPart->MinClockHz != 0 &&
      SpiPeripheral->SpiPart->MinClockHz > InternalClockHz) {
    Status = EFI_UNSUPPORTED;
  }
  if (!EFI_ERROR (Status)) {
    if (InternalClockHz >= MHz(100)) {
      InternalClockValue = 0x4;
    } else if (InternalClockHz >= MHz(66)) {
      InternalClockValue = 0x0;
    } else if (InternalClockHz >= MHz(33)) {
      InternalClockValue = 0x1;
    } else if (InternalClockHz >= MHz(22)) {
      InternalClockValue = 0x2;
    } else if (InternalClockHz >= MHz(16)) {
      InternalClockValue = 0x3;
    } else if (InternalClockHz >= KHz(800)) {
      InternalClockValue = 0x5;
    } else {
      Status = EFI_UNSUPPORTED;
    }
    if (!EFI_ERROR (Status)) {
      if (!Instance->PspMailboxSpiMode) {
        // Enable UseSpi100
        MmioOr8 (Instance->HcAddress + FCH_SPI_MMIO_REG20,
                BIT0);
        // Set the Value for NormSpeed and FastSpeed
        InternalClockValue = InternalClockValue << 12 | InternalClockValue << 8;
        MmioAndThenOr16 (Instance->HcAddress + FCH_SPI_MMIO_REG22,
                        0x00FF, InternalClockValue);
      } else {
        Instance->SpiCommunicationBuffer.SpiCommand[0].Frequency =
          (UINT8)InternalClockValue;
      }
    }
  }

  return Status;
}

/**
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
Transaction (
  IN CONST EFI_SPI_HC_PROTOCOL  *This,
  IN EFI_SPI_BUS_TRANSACTION    *BusTransaction
  )
{
  EFI_STATUS Status;
  UINT8 Opcode;
  UINT32 WriteBytes;
  UINT8  *WriteBuffer;
  UINT32 ReadBytes;
  UINT8  *ReadBuffer;
  EFI_PHYSICAL_ADDRESS HcAddress;
  SPI_HOST_CONTROLLER_INSTANCE *Instance;

  WriteBytes = BusTransaction->WriteBytes;
  WriteBuffer = BusTransaction->WriteBuffer;

  ReadBytes = BusTransaction->ReadBytes;
  ReadBuffer = BusTransaction->ReadBuffer;

  if (WriteBytes > This->MaximumTransferBytes + 6
      || ReadBytes > (This->MaximumTransferBytes + 6 - WriteBytes)
      || (WriteBytes != 0 && WriteBuffer == NULL)
      || (ReadBytes != 0 && ReadBuffer == NULL)) {
    return EFI_BAD_BUFFER_SIZE;
  }

  Opcode = 0;
  if (WriteBytes >= 1) {
    Opcode = WriteBuffer[0];
    // Skip Opcode
    WriteBytes -= 1;
    WriteBuffer += 1;
  }

  Status = EFI_SUCCESS;
  Instance = SPI_HOST_CONTROLLER_FROM_THIS (This);

  HcAddress = Instance->HcAddress;

  if (!Instance->PspMailboxSpiMode) {
    Status = FchSpiControllerNotBusy (Instance);
    if (!EFI_ERROR (Status)) {
      MmioWrite8 (
          HcAddress + FCH_SPI_MMIO_REG48_TX_BYTECOUNT,
          (UINT8)WriteBytes
          );
      MmioWrite8 (
          HcAddress + FCH_SPI_MMIO_REG4B_RXBYTECOUNT,
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
      Status = FchSpiExecute (Instance);
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
  } else {
    // Execute SPI transaction through PSP Mailbox
    Instance->SpiCommunicationBuffer.SpiCommand[0].OpCode = Opcode;
    Instance->SpiCommunicationBuffer.SpiCommand[0].BytesToTx = (UINT8)WriteBytes;
    if (WriteBytes != 0) {
      CopyMem (
        &Instance->SpiCommunicationBuffer.SpiCommand[0].Buffer,
        WriteBuffer,
        WriteBytes);
    }
    Instance->SpiCommunicationBuffer.SpiCommand[0].BytesToRx = (UINT8)ReadBytes;
    Instance->SpiCommunicationBuffer.CommandCount = 1;
    Instance->SpiCommunicationBuffer.SpiCommunicationResult.Value = 0x0;
    Instance->SpiCommunicationBuffer.ReadyToRun = TRUE;
    Status = PspExecuteSpiCommand ();
    if (!EFI_ERROR (Status)) {
      if (Instance->SpiCommunicationBuffer.SpiCommunicationResult.Value == 0x1000) {
        Status = EFI_INVALID_PARAMETER;
      } else {
        switch (Instance->SpiCommunicationBuffer.SpiCommunicationResult.Field.Command0Result) {
          case SPI_COMMAND_MALFORMED:
            Status = EFI_INVALID_PARAMETER;
            break;
          case SPI_COMMAND_COMPLETED:
            Status = EFI_SUCCESS;
            if (ReadBytes > 0) {
              CopyMem (
                ReadBuffer,
                &Instance->SpiCommunicationBuffer.SpiCommand[0].Buffer[0] + WriteBytes,
                ReadBytes
                );
            }
            break;
          case SPI_COMMAND_NOT_ALLOWED:
            Status = EFI_WRITE_PROTECTED;
            break;
          default:
            Status = EFI_DEVICE_ERROR;
            break;
        }
      }
    }
  }

  return Status;
}

