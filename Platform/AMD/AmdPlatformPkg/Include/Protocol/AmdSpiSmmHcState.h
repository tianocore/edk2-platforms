/** @file
  Header file of AMD SMM SPI host controller state protocol

  Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AMD_SMM_SPI_HC_STATE_PROTOCOL_H_
#define AMD_SMM_SPI_HC_STATE_PROTOCOL_H_

typedef struct _SMM_EFI_SPI_HC_STATE_PROTOCOL SMM_EFI_SPI_HC_STATE_PROTOCOL;

/**
  Save/Restore the state of the SPI Host Controller

  Use a chipset specific method to save the state of the SPI Host controller so
  it can be used without disturbing other transactions.

  @param[in] This           Pointer to an SMM_EFI_SPI_HC_STATE_PROTOCOL structure.

  @retval EFI_SUCCESS            The State was saved successfully
  @retval DEVICE_ERROR           SPI Executes command failed

**/
typedef EFI_STATUS
(EFIAPI *SMM_SPI_HC_STATE)(
  IN CONST SMM_EFI_SPI_HC_STATE_PROTOCOL  *This
  );

/**
  Lock/Unlock the SPI host controller register

  Use a chipset specific method to lock or unlock SPI host controller register.

  @param[in] This           Pointer to an SMM_EFI_SPI_HC_STATE_PROTOCOL structure.

  @retval EFI_SUCCESS      The clock was set up successfully
  @retval DEVICE_ERROR     SPI Executes command failed


**/
typedef EFI_STATUS
(EFIAPI *SMM_SPI_HC_LOCK_UNLOCK)(
  IN CONST SMM_EFI_SPI_HC_STATE_PROTOCOL  *This
  );

/**
  Block/Unblock SPI opcode

  Use a chipset specific method to block and unclock specific SPI opcode.

  @param[in] This           Pointer to an SMM_EFI_SPI_HC_STATE_PROTOCOL structure.

  @retval EFI_SUCCESS      The clock was set up successfully
  @retval DEVICE_ERROR     SPI Executes command failed

**/
typedef EFI_STATUS
(EFIAPI *SMM_SPI_HC_BLOCK_UNBLOCK_OPCODE)(
  IN CONST SMM_EFI_SPI_HC_STATE_PROTOCOL  *This,
  IN UINT8 Opcode
  );

/**
  Block/Unblock any SPI opcodes

  Use a chipset specific method to block and unclock all SPI opcodes.

  @param[in] This           Pointer to an SMM_EFI_SPI_HC_STATE_PROTOCOL structure.

  @retval EFI_SUCCESS      The clock was set up successfully
  @retval DEVICE_ERROR     SPI Executes command failed

**/
typedef EFI_STATUS
(EFIAPI *SMM_SPI_HC_BLOCK_UNBLOCK_ALL_OPCODES)(
  IN CONST SMM_EFI_SPI_HC_STATE_PROTOCOL  *This
  );

///
/// Manage and control the SPI host controller state.
///
struct _SMM_EFI_SPI_HC_STATE_PROTOCOL {
  ///
  /// Save and Restore SPI host controller state.
  ///
  SMM_SPI_HC_STATE                        SaveState;
  SMM_SPI_HC_STATE                        RestoreState;

  ///
  /// Lock and Unlock SPI host controller registers
  ///
  SMM_SPI_HC_LOCK_UNLOCK                  Lock;
  SMM_SPI_HC_LOCK_UNLOCK                  Unlock;

  ///
  /// Block and Unblock SPI Opcode
  ///
  SMM_SPI_HC_BLOCK_UNBLOCK_OPCODE         BlockOpcode;
  SMM_SPI_HC_BLOCK_UNBLOCK_OPCODE         UnblockOpcode;
  SMM_SPI_HC_BLOCK_UNBLOCK_ALL_OPCODES    UnblockAllOpcodes;
};

extern EFI_GUID  gAmdSpiHcStateProtocolGuid;

#endif // AMD_SMM_SPI_HC_STATE_PROTOCOL_H__
