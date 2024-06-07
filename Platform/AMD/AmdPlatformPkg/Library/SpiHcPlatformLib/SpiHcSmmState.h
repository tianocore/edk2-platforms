/** @file

  SPI HC SMM state registration function declarations

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SPI_HC_SMM_STATE_H_
#define SPI_HC_SMM_STATE_H_

#include <Base.h>
#include <Uefi/UefiBaseType.h>
#include <Protocol/AmdSpiSmmHcState.h>
#include "SpiHcInternal.h"

typedef struct {
  UINT32    Register;
  UINT8     Size;     // Size in Bytes
  UINT8     Count;    // Number of contiguous registers to store
} SPI_HC_REGISTER_STATE;

/**
  Allocate the save state space and update the instance structure

  @retval EFI_SUCCESS            The Save State space was allocated
  @retval EFI_OUT_OF_RESOURCES   The Save State space failed to allocate
**/
EFI_STATUS
EFIAPI
AllocateState (
  );

/**
  Save the Host Controller state to restore after transaction is complete

  @param[in] This           SPI host controller Preserve State Protocol;

  @retval EFI_SUCCESS       Spi Execute command executed properly
  @retval EFI_DEVICE_ERROR  Spi Execute command failed
**/
EFI_STATUS
EFIAPI
SaveState (
  IN CONST SMM_EFI_SPI_HC_STATE_PROTOCOL  *This
  );

/**
  Restore the Host Controller state

  @param[in] This           SPI host controller Preserve State Protocol;

  @retval EFI_SUCCESS       Spi Execute command executed properly
  @retval EFI_DEVICE_ERROR  Spi Execute command failed
**/
EFI_STATUS
EFIAPI
RestoreState (
  IN CONST SMM_EFI_SPI_HC_STATE_PROTOCOL  *This
  );

/**
  Block SPI Flash Write Enable Opcode.  This will block anything that requires
  the Opcode equivalent to the SPI Flash Memory Write Enable Opcode.

  RestrictedCmd0..3 (SPIx04[31:0]) will be locked (write protected) when
  SPIx00[23:22] not equal 11b, so you can write SPIx00[23:22]=00b to lock them.
  Once SPIx00[23:22] = 00b, they can only be written in SMM,
  to clear RestrictedCmd0..3, get into SMM, write SPIx00[23:22]=11b,
  then you can clear RestrictedCmd0..3 (SPIx04)
**/
EFI_STATUS
EFIAPI
FchSpiBlockOpcode (
  IN CONST SMM_EFI_SPI_HC_STATE_PROTOCOL  *This,
  IN UINT8                                Opcode
  );

/**
  Un-Block SPI Flash Write Enable Opcode.

  RestrictedCmd0..3 (SPIx04[31:0]) will be locked (write protected) when
  SPIx00[23:22] not equal 11b, so you can write SPIx00[23:22]=00b to lock them.
  Once SPIx00[23:22] = 00b, they can only be written in SMM,
  to clear RestrictedCmd0..3, get into SMM, write SPIx00[23:22]=11b,
  then you can clear RestrictedCmd0..3 (SPIx04)
**/
EFI_STATUS
EFIAPI
FchSpiUnblockOpcode (
  IN CONST SMM_EFI_SPI_HC_STATE_PROTOCOL  *This,
  IN UINT8                                Opcode
  );

/**
  Un-Block any blocked SPI Opcodes.

  RestrictedCmd0..3 (SPIx04[31:0]) will be locked (write protected) when
  SPIx00[23:22] not equal 11b, so you can write SPIx00[23:22]=00b to lock them.
  Once SPIx00[23:22] = 00b, they can only be written in SMM,
  to clear RestrictedCmd0..3, get into SMM, write SPIx00[23:22]=11b,
  then you can clear RestrictedCmd0..3 (SPIx04)
**/
EFI_STATUS
EFIAPI
FchSpiUnblockAllOpcodes (
  IN CONST SMM_EFI_SPI_HC_STATE_PROTOCOL  *This
  );

/**
  Lock SPI host controller registers.

  RestrictedCmd0..3 (SPIx04[31:0]) will be locked (write protected) when
  SPIx00[23:22] not equal 11b, so you can write SPIx00[23:22]=00b to lock them.
  Once SPIx00[23:22] = 00b, they can only be written in SMM,
  to clear RestrictedCmd0..3, get into SMM, write SPIx00[23:22]=11b,
  then you can clear RestrictedCmd0..3 (SPIx04)
**/
EFI_STATUS
EFIAPI
FchSpiLockSpiHostControllerRegisters (
  IN CONST SMM_EFI_SPI_HC_STATE_PROTOCOL  *This
  );

/**
  Unlock SPI host controller registers.  This unlock function will only work in
  SMM.

  RestrictedCmd0..3 (SPIx04[31:0]) will be locked (write protected) when
  SPIx00[23:22] not equal 11b, so you can write SPIx00[23:22]=00b to lock them.
  Once SPIx00[23:22] = 00b, they can only be written in SMM,
  to clear RestrictedCmd0..3, get into SMM, write SPIx00[23:22]=11b,
  then you can clear RestrictedCmd0..3 (SPIx04)
**/
EFI_STATUS
EFIAPI
FchSpiUnlockSpiHostControllerRegisters (
  IN CONST SMM_EFI_SPI_HC_STATE_PROTOCOL  *This
  );

#endif // SPI_HC_SMM_STATE_H__
