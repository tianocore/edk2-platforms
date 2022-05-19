/** @file
  Cadence I2C controller UEFI Driver implementation

  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/*
 * This driver have been written based on the Cadence I2C controller data-sheet
 *   Part number         : IP6510
 *   IP Version number   : r116_f02
 *   Data-sheet revision : 1.23
 *
 * Notes with regards to IP parameterization:
 *   cdnsi2c_fifo_mode         - Driver assume FIFO.
 *   cdnsi2c_no_fifo           - Not tested without FIFO.
 *                               Driver is likely to fail.
 *   cdnsi2c_p_fifo_depth      - The exact value not relevant as long it is
 *                               sane. Value is probed during runtime.
 *   cdnsi2c_p_fifo_pntr_width - Should not matter to the driver as long they
 *                               are sane values.
 *   cdnsi2c_p_read_clear      - Driver will always clear the ISR after reading
 *                               it so it doesn't matter what this is set to.
 *   cdnsi2c_p_xfer_size_width - Code assume that FIFO_SIZE+1 is representable.
 *
 * Nothing related to Glitch Filter implemented or tested.
 */

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "Driver.h"
#include "RegisterMap.h"

/// Milliseconds to block waiting for the I2C bus to be inactive
#define BUS_INACTIVE_TIMEOUT_MS  500u

/// Max value possible in CR.DIV_A
#define MAX_DIVISOR_A  ((MAX_UINT16 & CR_DIV_A) >> CR_DIV_A_SHIFT)

/// Max value possible in CR.DIV_B
#define MAX_DIVISOR_B  ((MAX_UINT16 & CR_DIV_B) >> CR_DIV_B_SHIFT)

/// Max valid FIFO size during probe
#define MAX_FIFO_PROBE_SIZE  32

/**
  Reads a device register.

  Reads the device register specified by Register. The read value is returned.

  If Register is unaligned or out of range, then ASSERT().

  @param[in] Dev       Device context.
  @param[in] Register  The register to read.

  @return The register read value.

**/
STATIC
UINT16
RegisterRead (
  IN CADENCE_I2C_CONTEXT  *Dev,
  IN UINT16               Register
  )
{
  ASSERT ((Register & 0x3) == 0);
  ASSERT (Register <= MAX_REGISTER_OFFSET);

  return MmioRead16 (Dev->CadenceI2cInstall->MmioBase + Register);
}

/**
  Writes a device register.

  Writes the device register specified by Register with the value specified by
  Data.

  If Register is unaligned or out of range, then ASSERT().

  @param[in] Dev       Device context.
  @param[in] Register  The device register to read.
  @param[in] Data      The data to write into the register.

**/
STATIC
VOID
RegisterWrite (
  IN CADENCE_I2C_CONTEXT  *Dev,
  IN UINT16               Register,
  IN UINT16               Data
  )
{
  ASSERT ((Register & 0x3) == 0);
  ASSERT (Register <= MAX_REGISTER_OFFSET);

  MmioWrite16 (Dev->CadenceI2cInstall->MmioBase + Register, Data);
}

/**
  Reads a device register, performs a bitwise AND, and writes the result back
  to the device register.

  Reads the device register specified by Register, performs a bitwise AND
  between the read result and the value specified by AndData, and writes the
  result to the device register specified by Register.
  The value written to the device register is returned.

  If Register is unaligned or out of range, then ASSERT().

  @param[in] Dev       Device context.
  @param[in] Register  The device register to modify.
  @param[in] AndData   The value to AND with the read value from the device
                       register.

  @return The value written back to the device register.

**/
STATIC
UINT16
RegisterAnd (
  IN CADENCE_I2C_CONTEXT  *Dev,
  IN UINTN                Register,
  IN UINT16               AndData
  )
{
  ASSERT ((Register & 0x3) == 0);
  ASSERT (Register <= MAX_REGISTER_OFFSET);

  return MmioAnd16 (Dev->CadenceI2cInstall->MmioBase + Register, AndData);
}

/**
  Reads a device register, performs a bitwise AND followed by a bitwise
  OR, and writes the result back to the device register.

  Reads the device register specified by Register, performs a bitwise AND
  between the read result and the value specified by AndData, performs a
  bitwise OR between the result of the AND operation and the value specified by
  OrData, and writes the result to the device register specified by Register.
  The value written to the MMIO register is returned.

  If Register is unaligned or out of range, then ASSERT().

  @param[in] Dev       Device context.
  @param[in] Register  The device register to modify.
  @param[in] AndData   The value to AND with the read value from the device
                       register.
  @param[in] OrData    The value to OR with the result of the device
                       operation.

  @return The value written back to the device register.

**/
STATIC
VOID
RegisterAndThenOr (
  IN CADENCE_I2C_CONTEXT  *Dev,
  IN UINTN                Register,
  IN UINT16               AndData,
  IN UINT16               OrData
  )
{
  ASSERT ((Register & 0x3) == 0);
  ASSERT (Register <= MAX_REGISTER_OFFSET);

  MmioAndThenOr16 (Dev->CadenceI2cInstall->MmioBase + Register, AndData, OrData);
}

/**
  Reads a device register, performs a bitwise OR, and writes the result back
  to the device register.

  Reads the device register specified by Register, performs a bitwise OR
  between the read result and the value specified by OrData, and writes the
  result to the device register specified by Register.
  The value written to the device register is returned.

  If Register is unaligned or out of range, then ASSERT().

  @param[in] Dev       Device context.
  @param[in] Register  The device register to modify.
  @param[in] OrData    The value to OR with the read value from the device
                       register.

  @return The value written back to the device register.

**/
STATIC
VOID
RegisterOr (
  IN CADENCE_I2C_CONTEXT  *Dev,
  IN UINTN                Register,
  IN UINT16               OrData
  )
{
  ASSERT ((Register & 0x3) == 0);
  ASSERT (Register <= MAX_REGISTER_OFFSET);

  MmioOr16 (Dev->CadenceI2cInstall->MmioBase + Register, OrData);
}

/**
  Modify the DIV_A and DIV_B fields of the device Control Register.

  The DivisorA and DivisorB are shifted as needed and written into the DIV_A
  and DIV_B fields.
  The other fields of the Control register are not modified.

  If DivisorA is out of range, then ASSERT().
  If DivisorB is out of range, then ASSERT().

  @param[in,out] Dev       Device context.
  @param[in]     DivisorA  The value to put into the DIV_A field.
  @param[in]     DivisorB  The value to put into the DIV_B field.

**/
STATIC
VOID
SetDivisors (
  IN OUT CADENCE_I2C_CONTEXT  *Dev,
  IN     UINT16               DivisorA,
  IN     UINT16               DivisorB
  )
{
  CONST UINT16  ShiftedDivisorA = CR_DIV_A & (DivisorA << CR_DIV_A_SHIFT);
  CONST UINT16  ShiftedDivisorB = CR_DIV_B & (DivisorB << CR_DIV_B_SHIFT);
  CONST UINT16  ShiftedDivisor  = ShiftedDivisorA | ShiftedDivisorB;
  CONST UINTN   NonDivisorMask  =
    (UINT16) ~((CR_DIV_A & MAX_UINT16) | (CR_DIV_B & MAX_UINT16));

  ASSERT (ShiftedDivisorA >> CR_DIV_A_SHIFT == DivisorA);
  ASSERT (ShiftedDivisorB >> CR_DIV_B_SHIFT == DivisorB);

  RegisterAndThenOr (Dev, REG_CR, NonDivisorMask, ShiftedDivisor);
}

/**
  Read and clear pending interrupt(s).

  This function will always read and write back the Interrupt Status register
  and therefore the code do not depend on the RTL parameter cdnsi2c_read_clear.

  @param[in,out] Dev  Device context.

  @retval The read value from the Interrupt status register.

**/
STATIC
UINT16
ClearIsr (
  IN OUT CADENCE_I2C_CONTEXT  *Dev
  )
{
  UINT16  Data;

  Data = RegisterRead (Dev, REG_ISR);
  RegisterWrite (Dev, REG_ISR, Data);
  return Data;
}

/**
  Converts milliseconds into number of ticks of the performance counter.

  @param[in] Milliseconds  Milliseconds to convert into ticks.

  @retval Milliseconds expressed as number of ticks.

**/
STATIC
UINT64
MilliSecondsToTicks (
  IN UINTN  Milliseconds
  )
{
  CONST UINT64  NanoSecondsPerTick = GetTimeInNanoSecond (1);

  return (Milliseconds * 1000000) / NanoSecondsPerTick;
}

/**
  Block until the bus is inactive or until BUS_INACTIVE_TIMEOUT_MS have passed.

  @param[in,out] Dev  Device context.

  @retval EFI_SUCCESS Bus is currently inactive.
  @retval EFI_TIMEOUT Bus did not become inactive before the time-out.

**/
STATIC
EFI_STATUS
BusyWaitOnBusInactive (
  IN OUT CADENCE_I2C_CONTEXT  *Dev
  )
{
  CONST UINT64  TickOut =
    GetPerformanceCounter () + MilliSecondsToTicks (BUS_INACTIVE_TIMEOUT_MS);

  while (RegisterRead (Dev, REG_SR) & SR_BA) {
    if (GetPerformanceCounter () > TickOut) {
      DEBUG ((
        DEBUG_ERROR,
        "[%a@0x%x]: Time-out while waiting for bus to be inactive!\n",
        gEfiCallerBaseName,
        Dev->CadenceI2cInstall->MmioBase
        ));
      return EFI_TIMEOUT;
    }
  }

  return EFI_SUCCESS;
}

/**
  Set the clear FIFO bit in the Control register and block until acknowledge.

  Set the clear FIFO bit in the Control register.
  The function will block until hardware acknowledge the FIFO have been cleared.

  @param[in,out] Dev  Device context.

**/
STATIC
VOID
ClearHwFifo (
  IN OUT CADENCE_I2C_CONTEXT  *Dev
  )
{
  RegisterOr (Dev, REG_CR, CR_CLRFIFO);

  // we assume that the input clock is not off (or we will loop forever)
  while (RegisterRead (Dev, REG_CR) & CR_CLRFIFO) {
  }
}

/**
  Start a transfer by writing the device address.

  If Address have reserved bits set, then ASSERT().

  @param[in,out] Dev      Device context.
  @param[in]     Address  Address to start a transfer against.

**/
STATIC
VOID
StartTransfer (
  IN OUT CADENCE_I2C_CONTEXT  *Dev,
  IN     UINT16               Address
  )
{
  ASSERT ((AR_ADD & Address) == Address);
  RegisterWrite (Dev, REG_AR, Address);
}

/**
  Probe RTL parameter cdnsi2c_p_xfer_size_width.

  @param[in,out] Dev  Device context.

  @retval 0  Probe failed.
  @retval *  Probed RTL parameter value.
**/
STATIC
UINT16
ProbeMaxTransferSize (
  IN OUT CADENCE_I2C_CONTEXT  *Dev
  )
{
  RegisterAndThenOr (Dev, REG_CR, ~CR_HOLD, CR_RW);
  RegisterWrite (Dev, REG_TSR, 0);
  ClearIsr (Dev);
  ClearHwFifo (Dev);
  RegisterWrite (Dev, REG_TSR, MAX_UINT16);
  return RegisterRead (Dev, REG_TSR);
}

/**
  Probe RTL parameter cdnsi2c_p_fifo_depth.

  @param[in,out]  Dev       Device context.
  @param[out]     FifoSize  Pointer to where to write the probed RTL parameter
                            value.

  @retval EFI_SUCCESS       RTL parameter probed successfully.
  @retval EFI_DEVICE_ERROR  Failed to probe the RTL parameter.
**/
STATIC
EFI_STATUS
ProbeFifoSize (
  IN OUT CADENCE_I2C_CONTEXT  *Dev,
  OUT    UINT16               *FifoSize
  )
{
  UINT16  ProbeSize;

  RegisterAnd (Dev, REG_CR, ~CR_RW);
  ClearIsr (Dev);
  ClearHwFifo (Dev);

  for (ProbeSize = 0; ProbeSize <= MAX_FIFO_PROBE_SIZE; ProbeSize++) {
    RegisterWrite (Dev, REG_DR, 0);
    CONST UINT16  Isr = ClearIsr (Dev);
    if ((BOOLEAN)(Isr & ~ISR_TX_OVF)) {
      DEBUG ((
        DEBUG_ERROR,
        "[%a@0x%x]: unexpected interrupt during FIFO size probing!\n",
        gEfiCallerBaseName,
        Dev->CadenceI2cInstall->MmioBase
        ));
      return EFI_DEVICE_ERROR;
    }

    if ((BOOLEAN)(Isr & ISR_TX_OVF)) {
      *FifoSize = ProbeSize;
      return EFI_SUCCESS;
    }
  }

  return EFI_DEVICE_ERROR;
}

/**
  Calculate Bus frequency as described in the data-sheet.
  If DivisorA is out of range, then ASSERT().
  If DivisorB is out of range, then ASSERT().

  @param[in]  Dev       Device context.
  @param[in]  DivisorA  Devisor A.
  @param[in]  DivisorB  Devisor B.

  @retval Calculated bus frequency.

**/
STATIC
UINTN
CalculateActualBusFrequency (
  IN CONST CADENCE_I2C_CONTEXT  *Dev,
  IN UINTN                      DivisorA,
  IN UINTN                      DivisorB
  )
{
  CONST UINT16  ShiftedDivisorA = CR_DIV_A & (DivisorA << CR_DIV_A_SHIFT);
  CONST UINT16  ShiftedDivisorB = CR_DIV_B & (DivisorB << CR_DIV_B_SHIFT);

  ASSERT (ShiftedDivisorA >> CR_DIV_A_SHIFT == DivisorA);
  ASSERT (ShiftedDivisorB >> CR_DIV_B_SHIFT == DivisorB);
  (void)ShiftedDivisorA;
  (void)ShiftedDivisorB;

  return Dev->CadenceI2cInstall->
           InputClockHz / (22 * ((DivisorA + 1)*(DivisorB + 1)));
}

/**
  Execute one I2C read operation.

  Execute one I2C read operation.
  This function assumes that the hardware is in suitable state to execute a bus
  operation and that correct address mode is set to handle SlaveAddress.

  @param[in,out]  Dev           Device context.
  @param[in]      SlaveAddress  Address of the device on the I2C bus.
  @param[in]      Length        Size of read operation.
  @param[out]     Data          Data buffer for the incoming data.
                                At-least Length bytes large.

  @retval  EFI_SUCCESS      Read operation successful and Data is updated.
  @retval  *                Other errors are possible. Data might be updated.

**/
STATIC
EFI_STATUS
I2cRead (
  IN OUT CADENCE_I2C_CONTEXT  *Dev,
  IN     UINTN                SlaveAddress,
  IN     UINTN                Length,
  OUT    UINT8                *Data
  )
{
  UINT16   Isr;
  UINTN    BytesToDrain;
  UINTN    NewLength;
  UINT16   Tsr;
  BOOLEAN  LastChunk;

  /*
   * !!!
   * !!! Some important note on how we handle reads whose size can not be
   * !!! represented in the TSR.
   * !!!
   *
   * When TSR reach 0 the read operation is stopped unconditionally regardless
   * of hold mode.
   * The issue is that we need to modify the TSR during an active transfer but
   * as that is a non-atomic read-modify-write operation special care must be
   * taken to "pause" the operation.
   * To accommodate reads larger than what is representable in the TSR the
   * read operation is "chunked" into FIFO sized bytes.
   *
   * By "chunking" the transfer size into FIFO sized bytes and setting TSR to
   * FIFO_SIZE+1 the transfer will be "on hold" (full FIFO during read will
   * cause the hardware to "pause" the transfer) when TSR=1 and therefore safe
   * to modify.
   * When reaching TSR=1 we re-arm the TSR once again with FIFO_SIZE+1
   * (safe because transfer is on hold so HW will not decrease the TSR behind
   * our back).
   *
   * When the remaining expected bytes can fit into a chunk we set the TSR to
   * the true value as we now want to finish the transfer.
   */

  if (Length > Dev->FifoSize) {
    RegisterWrite (Dev, REG_TSR, Dev->FifoSize + 1);
  } else {
    RegisterWrite (Dev, REG_TSR, Length);
  }

  RegisterOr (Dev, REG_CR, CR_RW);
  StartTransfer (Dev, SlaveAddress);
  while (Length != 0) {
    Isr = ClearIsr (Dev);
    if ((BOOLEAN)(Isr & ISR_TO)) {
      return EFI_TIMEOUT;
    }

    if ((BOOLEAN)(Isr & ISR_NACK)) {
      return EFI_DEVICE_ERROR;
    }

    // we handle FIFO reload by tracking TSR so ignore
    Isr &= ~ISR_DATA;

    // we handle transfer complete by manual book-keeping so ignore
    Isr &= ~ISR_COMP;

    if (Isr != 0) {
      // any other interrupts is weird
      DEBUG ((
        DEBUG_ERROR,
        "[%a@0x%x]: unexpected interrupt (ISR=0x%08x) during read!\n",
        gEfiCallerBaseName,
        Dev->CadenceI2cInstall->MmioBase,
        Isr
        ));
      return EFI_DEVICE_ERROR;
    }

    LastChunk = Length <= Dev->FifoSize;
    if (LastChunk) {
      if (RegisterRead (Dev, REG_SR) & SR_RXDV) {
        *Data = RegisterRead (Dev, REG_DR);
        Data++;
        Length--;
      }

      continue;
    }

    Tsr = RegisterRead (Dev, REG_TSR);
    ASSERT (Tsr);
    if ( Tsr != 1 ) {
      continue;
    }

    /*
     * As mentioned in the NOTE above now when the TSR==1 and we are not on the
     * last chunk we know that the transfer is on hold until we start popping
     * the FIFO.
     * Therefore it is safe to modify the TSR.
     */
    NewLength = Length - Dev->FifoSize;
    if (NewLength > Dev->FifoSize) {
      RegisterWrite (Dev, REG_TSR, Dev->FifoSize + 1);
    } else {
      RegisterWrite (Dev, REG_TSR, NewLength);
    }

    // drain the FIFO with a fixed amount of byte to keep our book-keeping and
    // the HW state consistent
    for (BytesToDrain = Dev->FifoSize; BytesToDrain; --BytesToDrain) {
      *Data = RegisterRead (Dev, REG_DR);
      Data++;
      Length--;
    }
  }

  return EFI_SUCCESS;
}

/**
  Execute one I2C write operation.

  Execute one I2C write operation.
  This function assumes that the hardware is in suitable state to execute a bus
  operation and that correct address mode is set to handle SlaveAddress.

  @param[in,out] Dev           Device context.
  @param[in]     SlaveAddress  Address of the device on the I2C bus.
  @param[in]     Length        Size of write operation.
  @param[in]     Data          Data buffer for the outgoing data.
                               At-least Length bytes large.

  @retval  EFI_SUCCESS      Write operation successful.
  @retval  *                Other errors are possible.

**/
STATIC
EFI_STATUS
I2cWrite (
  IN OUT CADENCE_I2C_CONTEXT  *Dev,
  IN     UINTN                SlaveAddress,
  IN     UINTN                Length,
  IN     CONST UINT8          *Data
  )
{
  /*
   * !!!
   * !!! Care must be take that the FIFO do not run dry unless it is because
   * !!! there are no more data to write.
   * !!! From IP data-sheet:
   * !!! > If FIFO is implemented, the programmer must ensure that the FIFO does
   * !!! > not completely empty before all the data has been written into it, as
   * !!! > the transfer then terminates.
   * !!!
   *
   * To avoid running the FIFO dry we must ensure TSR < FIFO_SIZE-1 and
   * enter hold mode and refill the FIFO when the transfer is on hold.
   * We chunk the transfer into FIFO_SIZE-1 sized chunks (except the last one)
   * using TSR.
   * When TSR is 0 the hardware enter hold mode in which we refill the FIFO and
   * set the TSR to FIFO_SIZE-1 again.
   * Rinse and repeat.
   *
   * When the remaining expected bytes can fit into a chunk we set the TSR to
   * the remaining byte count as we now want to finish the transfer.
   */

  RegisterAnd (Dev, REG_CR, ~CR_RW);

  // real TSR calculated in the loop, we just want to start the transfer
  RegisterWrite (Dev, REG_TSR, 0);
  StartTransfer (Dev, SlaveAddress);

  // each loop iteration will start with one byte in the FIFO and TSR=0
  RegisterWrite (Dev, REG_DR, *Data);
  Data++;
  Length--;
  while (1) {
    CONST BOOLEAN  FinalTransfer = Length <= Dev->FifoSize - 1;
    CONST UINTN    ChunkSize     = FinalTransfer ? Length : Dev->FifoSize - 1;
    UINTN          Index;
    for (Index = ChunkSize; Index; Index--) {
      RegisterWrite (Dev, REG_DR, *Data);
      Data++;
      Length--;
    }

    if (FinalTransfer) {
      // make sure to include the extra byte added outside the loop FIFO
      RegisterWrite (Dev, REG_TSR, ChunkSize + 1);
    } else {
      RegisterWrite (Dev, REG_TSR, ChunkSize);
    }

    UINT16  Isr;
    do {
      // we handle FIFO reload by tracking TSR so ignore ISR_DATA
      Isr = ClearIsr (Dev) & ~(ISR_DATA);
    } while (Isr == 0);

    if (Isr & ~ISR_COMP) {
      if (Isr & ISR_TO) {
        return EFI_TIMEOUT;
      }

      if (Isr & ISR_NACK) {
        return EFI_DEVICE_ERROR;
      }

      // any other interrupts is weird
      DEBUG ((
        DEBUG_ERROR,
        "[%a@0x%x]: unexpected interrupt (ISR=0x%08x) during wait "
        "for transfer complete!\n",
        gEfiCallerBaseName,
        Dev->CadenceI2cInstall->MmioBase,
        Isr
        ));
      return EFI_DEVICE_ERROR;
    }

    if (FinalTransfer) {
      return EFI_SUCCESS;
    }
  }

  return EFI_SUCCESS;
}

/**
  Initialize a Cadence I2C controller and connect it to the device context
  instance Dev.

  @param[in,out] Dev  Device context.

  @retval  EFI_SUCCESS      Hardware probed and connected to the device context
                            Dev.
  @retval  EFI_UNSUPPORTED  The hardware is not compatible with this driver.
  @retval  *                Other errors are possible.

**/
EFI_STATUS
DriverStart (
  IN OUT CADENCE_I2C_CONTEXT  *Dev
  )
{
  EFI_STATUS  Status;

  if (  (Dev->CadenceI2cInstall->InputClockHz == 0)
     || (CalculateActualBusFrequency (Dev, MAX_DIVISOR_A, MAX_DIVISOR_B) == 0)
        )
  {
    DEBUG ((
      DEBUG_ERROR,
      "[%a@0x%x]: Input clock (%u Hz) is unreasonable low!\n",
      gEfiCallerBaseName,
      Dev->CadenceI2cInstall->MmioBase,
      Dev->CadenceI2cInstall->InputClockHz
      ));
    return EFI_UNSUPPORTED;
  }

  Status = DriverReset (Dev);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ProbeFifoSize (Dev, &Dev->FifoSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a@0x%x]: unable to probe FIFO size!\n",
      gEfiCallerBaseName,
      Dev->CadenceI2cInstall->MmioBase
      ));
    return EFI_UNSUPPORTED;
  }

  DEBUG ((
    DEBUG_INFO,
    "[%a@0x%x]: detected FIFO size as %u bytes\n",
    gEfiCallerBaseName,
    Dev->CadenceI2cInstall->MmioBase,
    Dev->FifoSize
    ));

  CONST UINTN  MaxTransferSize = ProbeMaxTransferSize (Dev);

  DEBUG ((
    DEBUG_INFO,
    "[%a@0x%x]: detected max transfer size as %u bytes\n",
    gEfiCallerBaseName,
    Dev->CadenceI2cInstall->MmioBase,
    MaxTransferSize
    ));

  // the transfer functions assumes that the TSR can represent FIFO_SIZE + 1
  CONST UINTN  RequiredMaxTsr = Dev->FifoSize + 1;

  if (MaxTransferSize < RequiredMaxTsr) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a@0x%x]: unsupported combination of max transfer size and FIFO size\n",
      gEfiCallerBaseName,
      Dev->CadenceI2cInstall->MmioBase
      ));
    return EFI_UNSUPPORTED;
  }

  return DriverReset (Dev);
}

/**
  Disconnect the hardware from the driver instance.

  @param[in,out] Dev  Device context.
**/
VOID
DriverStop (
  IN OUT CADENCE_I2C_CONTEXT  *Dev OPTIONAL
  )
{
}

/**
  Set the frequency for the I2C clock line.

  Only extensions implemented by this driver is documented here.
  For non-extension details see EFI_I2C_MASTER_PROTOCOL_SET_BUS_FREQUENCY.

  @param[in,out] Dev            Device context.
  @param[in,out] BusClockHertz  Pointer to the requested I2C bus clock frequency
                                in Hertz.
                                Upon return this value contains the actual
                                frequency in use by the I2C controller.

  @retval EFI_SUCCESS     The bus frequency was set successfully.
  @retval EFI_UNSUPPORTED The controller does not support this frequency.

**/
EFI_STATUS
DriverSetBusFrequency (
  IN OUT CADENCE_I2C_CONTEXT  *Dev,
  IN OUT UINTN                *BusClockHertz
  )
{
  CONST UINTN  RequestedBusFreq = *BusClockHertz;
  UINTN        Divisor;
  UINTN        DivisorA;
  UINTN        DivisorB;
  UINTN        ActuallyBusFreq;

  if (RequestedBusFreq == 0) {
    return EFI_UNSUPPORTED;
  }

  Divisor =  Dev->CadenceI2cInstall->InputClockHz / (22 * RequestedBusFreq);
  if (Divisor > 0) {
    Divisor  = Divisor - 1;
    DivisorA = Divisor & MAX_DIVISOR_A;
    DivisorB = (Divisor >> (CR_DIV_B_SHIFT - CR_DIV_A_SHIFT));

    if (DivisorB > MAX_DIVISOR_B) {
      DivisorA = MAX_DIVISOR_A;
      DivisorB = MAX_DIVISOR_B;
    }
  } else {
    DivisorA = 0;
    DivisorB = 0;
  }

  // identify a near _lower_ possible bus speed
  do {
    ActuallyBusFreq = CalculateActualBusFrequency (Dev, DivisorA, DivisorB);
    if (ActuallyBusFreq <= RequestedBusFreq) {
      SetDivisors (Dev, DivisorA, DivisorB);
      *BusClockHertz = ActuallyBusFreq;
      return EFI_SUCCESS;
    }

    if (DivisorA == MAX_DIVISOR_A) {
      DivisorB += 1;
      DivisorA  = DivisorA/2 + 1;
    } else {
      DivisorA += 1;
    }
  } while (DivisorB <= MAX_DIVISOR_B);

  DEBUG ((
    DEBUG_ERROR,
    "[%a@0x%x]: impossible slow bus speed (%u Hz) requested while slowest possible is %u Hz\n",
    gEfiCallerBaseName,
    Dev->CadenceI2cInstall->MmioBase,
    RequestedBusFreq,
    CalculateActualBusFrequency (Dev, MAX_DIVISOR_A, MAX_DIVISOR_B)
    ));
  return EFI_UNSUPPORTED;
}

/**
  Reset the I2C controller and configure it for use.

  Only extensions implemented by this driver is documented here.
  For non-extension details see EFI_I2C_MASTER_PROTOCOL_RESET.

  @param[in,out] Dev  Instance context.

  @retval EFI_SUCCESS            The reset completed successfully.
  @retval EFI_DEVICE_ERROR       The reset operation failed.

**/
EFI_STATUS
DriverReset (
  IN OUT CADENCE_I2C_CONTEXT  *Dev
  )
{
  RegisterWrite (Dev, REG_CR, CR_MS | CR_ACKEN);
  ClearHwFifo (Dev);

  // ensure IP default time-out
  RegisterWrite (Dev, REG_TOR, 0x001F);

  // disable all interrupt sources
  RegisterWrite (Dev, REG_IER, 0x0000u);
  RegisterWrite (Dev, REG_IDR, 0x02FFu);
  if (RegisterRead (Dev, REG_IMR) != 0x02FFu) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a@0x%x]: failed to disable all interrupt sources!\n",
      gEfiCallerBaseName,
      Dev->CadenceI2cInstall->MmioBase
      ));
    return EFI_DEVICE_ERROR;
  }

  ClearIsr (Dev);
  return EFI_SUCCESS;
}

/**
  Start an I2C transaction on the host controller.

  Only extensions implemented by this driver is documented here.
  For non-extension details see EFI_I2C_MASTER_PROTOCOL_START_REQUEST.

  @param[in,out] Dev              Instance context.
  @param[in]     SlaveAddress     Address of the device on the I2C bus.
  @param[in]     ExtendedAddress  TRUE if SlaveAddress is 10 bits.
  @param[in,out] RequestPacket    Pointer to an EFI_I2C_REQUEST_PACKET
                                  structure describing the I2C transaction.
  @param[in]     Event            Event to signal for asynchronous transactions,
                                  NULL for synchronous transactions.
  @param[out]    I2cStatus        Optional buffer to receive the I2C transaction
                                  completion status.

  @retval EFI_SUCCESS           The asynchronous transaction was successfully
                                started when Event is not NULL.
  @retval EFI_SUCCESS           The transaction completed successfully when
                                Event is NULL.
  @retval EFI_ALREADY_STARTED   The controller is busy with another transaction.
  @retval EFI_DEVICE_ERROR      There was an I2C error (NACK) during the
                                transaction.
  @retval EFI_NOT_FOUND         Reserved bit set in the SlaveAddress parameter
  @retval EFI_NO_RESPONSE       The I2C device is not responding to the slave
                                address.  EFI_DEVICE_ERROR will be returned if
                                the controller cannot distinguish when the NACK
                                occurred.
  @retval EFI_UNSUPPORTED       The controller does not support the requested
                                transaction.

**/
EFI_STATUS
DriverStartRequest (
  IN OUT CADENCE_I2C_CONTEXT     *Dev,
  IN     UINTN                   SlaveAddress,
  IN     BOOLEAN                 ExtendedAddress,
  IN OUT EFI_I2C_REQUEST_PACKET  *RequestPacket,
  IN     EFI_EVENT               Event,
  OUT    EFI_STATUS              *I2cStatus
  )
{
  EFI_I2C_OPERATION  *Operation;
  EFI_STATUS         Status;
  UINTN              Index;

  RegisterAndThenOr (Dev, REG_CR, ~CR_NEA, ExtendedAddress ? 0 : CR_NEA);
  RegisterAnd (Dev, REG_CR, ~CR_HOLD);
  Status = BusyWaitOnBusInactive (Dev);
  if (EFI_ERROR (Status)) {
    goto done;
  }

  RegisterOr (Dev, REG_CR, CR_HOLD);

  for (Index = 0, Operation = RequestPacket->Operation;
       Index < RequestPacket->OperationCount;
       Index++, Operation++)
  {
    if (Operation->LengthInBytes == 0) {
      continue;
    }

    ClearHwFifo (Dev);
    ClearIsr (Dev);

    // we have not implemented anything other than pure I2C read and write
    if (Operation->Flags & ~I2C_FLAG_READ) {
      DEBUG ((
        DEBUG_ERROR,
        "[%a@0x%x]: unsupported operation[%u] flag(s) 0x%08x set!\n",
        gEfiCallerBaseName,
        Dev->CadenceI2cInstall->MmioBase,
        Operation->Flags & ~I2C_FLAG_READ
        ));
      return EFI_UNSUPPORTED;
    }

    if (Operation->Flags & I2C_FLAG_READ) {
      Status = I2cRead (
                 Dev,
                 SlaveAddress,
                 Operation->LengthInBytes,
                 Operation->Buffer
                 );
    } else {
      Status = I2cWrite (
                 Dev,
                 SlaveAddress,
                 Operation->LengthInBytes,
                 Operation->Buffer
                 );
    }

    if (EFI_ERROR (Status)) {
      break;
    }
  }

done:
  RegisterAnd (Dev, REG_CR, ~CR_HOLD);
  ClearHwFifo (Dev);

  if (I2cStatus == NULL) {
    return Status;
  }

  *I2cStatus = Status;
  gBS->SignalEvent (Event);
  return EFI_SUCCESS;
}
