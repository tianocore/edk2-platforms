/** @file

  Internal functions used by platform SPI HC library

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/TimerLib.h>
#include <Library/PciSegmentLib.h>
#include <Protocol/SpiSmmHc.h>
#include <FchRegistersCommon.h>
#include <Library/SpiHcPlatformLib.h>
#include "SpiHcInternal.h"

extern EFI_PHYSICAL_ADDRESS  mHcAddress;

/**
  Check that SPI Conroller is Not Busy.

  @retval EFI_SUCCESS       Spi Execute command executed properly
  @retval EFI_DEVICE_ERROR  Spi Execute command failed
**/
EFI_STATUS
EFIAPI
FchSpiControllerNotBusy (
  )
{
  UINT32  SpiReg00;
  UINT32  LpcDmaStatus;
  UINT32  RetryCount;
  UINTN   DelayMicroseconds;

  DelayMicroseconds = FixedPcdGet32 (PcdSpiNorFlashOperationDelayMicroseconds);
  SpiReg00          = FCH_SPI_BUSY;
  RetryCount        = FixedPcdGet32 (PcdAmdSpiRetryCount);
  do {
    SpiReg00     = MmioRead32 (mHcAddress + FCH_SPI_MMIO_REG4C_SPISTATUS);
    LpcDmaStatus = PciSegmentRead32 (
                     PCI_SEGMENT_LIB_ADDRESS (
                       0x00,
                       FCH_LPC_BUS,
                       FCH_LPC_DEV,
                       FCH_LPC_FUNC,
                       FCH_LPC_REGB8
                       )
                     );
    if (  ((SpiReg00 & FCH_SPI_BUSY) == 0)
       && ((LpcDmaStatus & FCH_LPC_DMA_SPI_BUSY) == 0))
    {
      break;
    }

    MicroSecondDelay (DelayMicroseconds);
    RetryCount--;
  } while (RetryCount > 0);

  if (RetryCount == 0) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Check for SPI transaction failure(s).

  @retval EFI_SUCCESS   Spi Execute command executed properly
  @retval others        Spi Execute command failed
**/
EFI_STATUS
EFIAPI
FchSpiTransactionCheckFailure (
  )
{
  EFI_STATUS  Status;
  UINT32      Data;

  Status = FchSpiControllerNotBusy ();
  if (!EFI_ERROR (Status)) {
    Data = MmioRead32 (mHcAddress + FCH_SPI_MMIO_REG00);
    if ((Data & FCH_SPI_FIFO_PTR_CRL) != 0) {
      Status = EFI_ACCESS_DENIED;
    }
  }

  return Status;
}

/**
  If SPI controller is not busy, execute SPI command.  Then wait until SPI
  controller is not busy.

  @retval EFI_SUCCESS   Spi Execute command executed properly
  @retval others        Spi Execute command failed
**/
EFI_STATUS
EFIAPI
FchSpiExecute (
  )
{
  EFI_STATUS  Status;

  Status = FchSpiControllerNotBusy ();
  if (!EFI_ERROR (Status)) {
    MmioOr8 (mHcAddress + FCH_SPI_MMIO_REG47_CMDTRIGGER, BIT7);
    Status = FchSpiControllerNotBusy ();
    if (!EFI_ERROR (Status)) {
      Status = FchSpiTransactionCheckFailure ();
    }
  }

  return Status;
}

/**
  Block SPI Flash Write Enable Opcode.  This will block anything that requires
  the Opcode equivalent to the SPI Flash Memory Write Enable Opcode.

  RestrictedCmd0..3 (SPIx04[31:0]) will be locked (write protected) when
  SPIx00[23:22] not equal 11b, so you can write SPIx00[23:22]=00b to lock them.
  Once SPIx00[23:22] = 00b, they can only be written in SMM,
  to clear RestrictedCmd0..3, get into SMM, write SPIx00[23:22]=11b,
  then you can clear RestrictedCmd0..3 (SPIx04)

  Calls during DXE will only work until the SPI controller is locked.

  Calls to these functions from SMM will only be valid during SMM, restore state
  will wipe out any changes.

  @param[in]  HcAddress   Host controller physical address.
  @param[in]  Opcode      SPI opcode to be passed.
  @retval     EFI_SUCCESS If executed successfully.
              Others      If fails.
**/
EFI_STATUS
EFIAPI
InternalFchSpiBlockOpcode (
  IN CONST EFI_PHYSICAL_ADDRESS  HcAddress,
  IN UINT8                       Opcode
  )
{
  EFI_STATUS  Status;
  BOOLEAN     OpcodeBlocked;
  UINTN       RestrictedCmd;
  UINT8       Data;

  Status        = EFI_OUT_OF_RESOURCES;
  OpcodeBlocked = FALSE;

  // Allow only one copy of Opcode in RestrictedCmd register
  for (RestrictedCmd = 0; RestrictedCmd <= 3; RestrictedCmd++) {
    Data = MmioRead8 (HcAddress + FCH_SPI_MMIO_REG04 + RestrictedCmd);

    if ((Data == Opcode) && (OpcodeBlocked == FALSE)) {
      OpcodeBlocked = TRUE;
    } else if ((Data == Opcode) && (OpcodeBlocked == TRUE)) {
      MmioWrite8 (HcAddress + FCH_SPI_MMIO_REG04 + RestrictedCmd, 0x00);
    } else if ((Data == 0x00) && (OpcodeBlocked == FALSE)) {
      MmioWrite8 (HcAddress + FCH_SPI_MMIO_REG04 + RestrictedCmd, Opcode);
      OpcodeBlocked = TRUE;
    }
  }

  if (OpcodeBlocked) {
    Status = EFI_SUCCESS;
  }

  return Status;
}

/**
  Un-Block SPI Flash Write Enable Opcode.

  RestrictedCmd0..3 (SPIx04[31:0]) will be locked (write protected) when
  SPIx00[23:22] not equal 11b, so you can write SPIx00[23:22]=00b to lock them.
  Once SPIx00[23:22] = 00b, they can only be written in SMM,
  to clear RestrictedCmd0..3, get into SMM, write SPIx00[23:22]=11b,
  then you can clear RestrictedCmd0..3 (SPIx04)

  Calls during DXE will only work until the SPI controller is locked.

  Calls to these functions from SMM will only be valid during SMM, restore state
  will wipe out any changes.
  @param[in]  HcAddress   Host controller physical address.
  @param[in]  Opcode      SPI opcode to be passed.
  @retval     EFI_SUCCESS If executed successfully.
              Others      If fails.
**/
EFI_STATUS
EFIAPI
InternalFchSpiUnblockOpcode (
  IN CONST EFI_PHYSICAL_ADDRESS  HcAddress,
  IN UINT8                       Opcode
  )
{
  UINTN  RestrictedCmd;

  // Unblock any copies of the Opcode
  for (RestrictedCmd = 0; RestrictedCmd <= 3; RestrictedCmd++) {
    if (MmioRead8 (HcAddress + FCH_SPI_MMIO_REG04 + RestrictedCmd) == Opcode) {
      MmioWrite8 (HcAddress + FCH_SPI_MMIO_REG04 + RestrictedCmd, 0x00);
    }
  }

  return EFI_SUCCESS;
}

/**
  Un-Block any blocked SPI Opcodes.

  RestrictedCmd0..3 (SPIx04[31:0]) will be locked (write protected) when
  SPIx00[23:22] not equal 11b, so you can write SPIx00[23:22]=00b to lock them.
  Once SPIx00[23:22] = 00b, they can only be written in SMM,
  to clear RestrictedCmd0..3, get into SMM, write SPIx00[23:22]=11b,
  then you can clear RestrictedCmd0..3 (SPIx04)

  Calls during DXE will only work until the SPI controller is locked.

  Calls to these functions from SMM will only be valid during SMM, restore state
  will wipe out any changes.

  @param[in]  HcAddress   Host controller physical address
  @retval     EFI_SUCCESS If executed successfully.
              Others      If fails
**/
EFI_STATUS
EFIAPI
InternalFchSpiUnblockAllOpcodes (
  IN CONST EFI_PHYSICAL_ADDRESS  HcAddress
  )
{
  MmioWrite32 (HcAddress + FCH_SPI_MMIO_REG04, 0x00);
  return EFI_SUCCESS;
}

/**
  Lock SPI host controller registers.

  RestrictedCmd0..3 (SPIx04[31:0]) will be locked (write protected) when
  SPIx00[23:22] not equal 11b, so you can write SPIx00[23:22]=00b to lock them.
  Once SPIx00[23:22] = 00b, they can only be written in SMM,
  to clear RestrictedCmd0..3, get into SMM, write SPIx00[23:22]=11b,
  then you can clear RestrictedCmd0..3 (SPIx04)

  @param[in]  HcAddress   Host controller physical address
  @retval     EFI_SUCCESS If executed successfully.
              Others      If fails
**/
EFI_STATUS
EFIAPI
InternalFchSpiLockSpiHostControllerRegisters (
  IN CONST EFI_PHYSICAL_ADDRESS  HcAddress
  )
{
  MmioBitFieldAnd32 (
    HcAddress + FCH_SPI_MMIO_REG00,
    22,
    23,
    0x0
    );
  if (MmioBitFieldRead32 (HcAddress + FCH_SPI_MMIO_REG00, 22, 23)
      == 0x0)
  {
    return EFI_SUCCESS;
  }

  return EFI_DEVICE_ERROR;
}

/**
  Unlock SPI host controller registers.  This unlock function will only work in
  SMM.

  RestrictedCmd0..3 (SPIx04[31:0]) will be locked (write protected) when
  SPIx00[23:22] not equal 11b, so you can write SPIx00[23:22]=00b to lock them.
  Once SPIx00[23:22] = 00b, they can only be written in SMM,
  to clear RestrictedCmd0..3, get into SMM, write SPIx00[23:22]=11b,
  then you can clear RestrictedCmd0..3 (SPIx04).

  @param[in]  HcAddress   Host controller physical address
  @retval     EFI_SUCCESS If executed successfully.
              Others      If fails
**/
EFI_STATUS
EFIAPI
InternalFchSpiUnlockSpiHostControllerRegisters (
  IN CONST EFI_PHYSICAL_ADDRESS  HcAddress
  )
{
  MmioBitFieldOr32 (
    HcAddress + FCH_SPI_MMIO_REG00,
    22,
    23,
    BIT0 | BIT1
    );
  if (MmioBitFieldRead32 (HcAddress + FCH_SPI_MMIO_REG00, 22, 23)
      == (BIT0 | BIT1))
  {
    return EFI_SUCCESS;
  }

  return EFI_DEVICE_ERROR;
}
