/** @file
  Provides functions for communication with System Firmware (SMpro/PMpro)
  via interfaces like Mailbox.

  Copyright (c) 2021 - 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SYSTEM_FIRMWARE_INTERFACE_LIB_H_
#define SYSTEM_FIRMWARE_INTERFACE_LIB_H_

//
// Common mailbox message format
//   Bit 31:28 - Message type
//   Bit 27:24 - Message subtype
//   Bit 23:16 - Message control byte
//   Bit 15:0  - Message data specific
//
#define MAILBOX_MESSAGE_TYPE_SHIFT          28
#define MAILBOX_MESSAGE_SUBTYPE_SHIFT       24
#define MAILBOX_MESSAGE_CONTROL_BYTE_SHIFT  16

#define COMMON_MESSAGE_ENCODE(Type, Subtype, Control)             \
          (                                                     \
            ((Type) << MAILBOX_MESSAGE_TYPE_SHIFT) |            \
            ((Subtype) << MAILBOX_MESSAGE_SUBTYPE_SHIFT) |      \
            ((Control) << MAILBOX_MESSAGE_CONTROL_BYTE_SHIFT)   \
          )

#define MAILBOX_MESSAGE_CONTROL_URGENT   BIT7
#define MAILBOX_MESSAGE_CONTROL_TYPICAL  0

//
// Mailbox Message Types
//
#define MAILBOX_MESSAGE_TYPE_DEBUG    0x00
#define MAILBOX_MESSAGE_TYPE_ADDRESS  0x05
#define MAILBOX_MESSAGE_TYPE_USER     0x06

//
// Mailbox Message Type 0x00 - Debug message
//
#define MAILBOX_DEBUG_MESSAGE_SUBTYPE_REGISTER_READ   0x01
#define MAILBOX_DEBUG_MESSAGE_SUBTYPE_REGISTER_WRITE  0x02

//
// Debug message data format
//   Bit 31:16 - Refer to definition of COMMON_MESSAGE_ENCODE
//   Bit 15:0  - Store lower 16-bit of the upper 64-bit address
//
#define MAILBOX_DEBUG_MESSAGE_ENCODE(Subtype, Address)       \
          (                                                 \
            (COMMON_MESSAGE_ENCODE (                        \
               MAILBOX_MESSAGE_TYPE_DEBUG,                  \
               (Subtype),                                   \
               MAILBOX_MESSAGE_CONTROL_TYPICAL)) |          \
            ((Address) & 0xFFFF)                            \
          )

//
// Mailbox Message Type 0x05 - Address message
//
#define MAILBOX_ADDRESS_MESSAGE_SUBTYPE_PCC  0x03

//
// Address message data format
//   Bit 31:16 - Refer to definition of COMMON_MESSAGE_ENCODE
//   Bit 15:8  - Message Parameter
//   Bit 7:4   - Address message control bit
//               0x4: 256 alignment
//               0x0: No alignment
//   Bit 3:0   - Unused
//
#define MAILBOX_ADDRESS_MESSAGE_ENCODE(Subtype, Param, Align) \
          (                                                 \
            (COMMON_MESSAGE_ENCODE (                        \
               MAILBOX_MESSAGE_TYPE_ADDRESS,                \
               (Subtype),                                   \
               MAILBOX_MESSAGE_CONTROL_TYPICAL)) |          \
            ((Param) << 8) |                                \
            ((Align) << 4)                                  \
          )

#define MAILBOX_ADDRESS_URGENT_MESSAGE_ENCODE(Subtype, Param, Align) \
          (                                                        \
            (COMMON_MESSAGE_ENCODE (                               \
               MAILBOX_MESSAGE_TYPE_ADDRESS,                       \
               (Subtype),                                          \
               MAILBOX_MESSAGE_CONTROL_URGENT)) |                  \
            ((Param) << 8) |                                       \
            ((Align) << 4)                                         \
          )

#define MAILBOX_ADDRESS_256_ALIGNMENT  0x4
#define MAILBOX_ADDRESS_NO_ALIGNMENT   0x0

#define MAILBOX_ADDRESS_MESSAGE_PARAM_CPPC  0x01

#define MAILBOX_URGENT_CPPC_MESSAGE                 \
          (                                         \
            MAILBOX_ADDRESS_URGENT_MESSAGE_ENCODE ( \
              MAILBOX_ADDRESS_MESSAGE_SUBTYPE_PCC,  \
              MAILBOX_ADDRESS_MESSAGE_PARAM_CPPC,   \
              MAILBOX_ADDRESS_256_ALIGNMENT)        \
          )

#define MAILBOX_TYPICAL_PCC_MESSAGE                 \
          (                                         \
            MAILBOX_ADDRESS_MESSAGE_ENCODE (        \
              MAILBOX_ADDRESS_MESSAGE_SUBTYPE_PCC,  \
              0,                                    \
              MAILBOX_ADDRESS_256_ALIGNMENT)        \
          )

//
// Mailbox Message Type 0x06 - User message
//
#define MAILBOX_USER_MESSAGE_SUBTYPE_SET_CONFIGURATION  0x02
#define MAILBOX_USER_MESSAGE_SUBTYPE_BOOT_PROGRESS      0x06
#define MAILBOX_USER_MESSAGE_SUBTYPE_TRNG_PROXY         0x07

//
// User message data format
//   Bit 31:16 - Refer to definition of COMMON_MESSAGE_ENCODE
//   Bit 15:8  - Message Parameter 0
//   Bit 7:0   - Message Parameter 1
//
#define MAILBOX_USER_MESSAGE_ENCODE(Subtype, Param0, Param1) \
          (                                                \
            (COMMON_MESSAGE_ENCODE (                       \
               MAILBOX_MESSAGE_TYPE_USER,                  \
               (Subtype),                                  \
               MAILBOX_MESSAGE_CONTROL_TYPICAL)) |         \
            ((Param0) << 8) |                              \
            (Param1)                                       \
          )

//
// Parameters for True RNG Proxy Message
//   Param0: 1 - Get a random number
//   Param1: Unused
//
#define MAILBOX_TRNG_PROXY_GET_RANDOM_NUMBER  1

//
// Parameters for Boot Progress
//   Param0: 1 - Set boot state
//   Param1: Boot stage value
//     0x08: BL33/UEFI Stage
//     0x09: OS Stage
//
#define MAILBOX_BOOT_PROGRESS_COMMAND_SET  1
#define MAILBOX_BOOT_PROGRESS_STAGE_UEFI   8
#define MAILBOX_BOOT_PROGRESS_STAGE_OS     9

//
// Parameters for Set Configuration
//   Param0: Configuration type
//     37: Date configuration
//   Param1: Unused
//
#define MAILBOX_SET_CONFIGURATION_DATE  37

/**
  Read a register which is not accessible from the non-secure world
  by sending a mailbox message to the SMpro processor.

  Note that not all addresses are allowed.

  @param[in]  Socket       Active socket index.
  @param[in]  Address      A 64-bit register address to be read.
  @param[out] Value        A pointer to the read value.

  @retval EFI_SUCCESS           Read the register successfully.
  @retval EFI_UNSUPPORTED       The register is not allowed.
  @retval Otherwise             Errors returned from MailboxWrite/MailboxRead() functions.
**/
EFI_STATUS
EFIAPI
MailboxMsgRegisterRead (
  IN  UINT8   Socket,
  IN  UINTN   Address,
  OUT UINT32  *Value
  );

/**
  Write a value to a register which is not accessible from the non-secure world
  by sending a mailbox message to the SMpro processor.

  Note that not all addresses are allowed.

  @param[in]  Socket       Active socket index.
  @param[in]  Address      A 64-bit register address to be written.
  @param[in]  Value        The value to be written to the register.

  @retval EFI_SUCCESS      Write the register successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval Otherwise        Errors returned from the MailboxWrite() function.
**/
EFI_STATUS
EFIAPI
MailboxMsgRegisterWrite (
  IN UINT8   Socket,
  IN UINTN   Address,
  IN UINT32  Value
  );

/**
  Set the PCC shared Memory Address to service handlers in the System Control Processors,
  using for communication between the System Firmware and OSPM.

  @param[in]  Socket           Active socket index.
  @param[in]  Doorbell         Doorbell index which is numbered like DOORBELL_CHANNELS.
  @param[in]  AddressAlign256  Enable/Disable 256 alignment.
  @param[in]  Address          The shared memory address.

  @retval EFI_SUCCESS           Set the shared memory address successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval Otherwise             Errors returned from the MailboxWrite() functions.
**/
EFI_STATUS
EFIAPI
MailboxMsgSetPccSharedMem (
  IN UINT8    Socket,
  IN UINT8    Doorbell,
  IN BOOLEAN  AddressAlign256,
  IN UINTN    Address
  );

/**
  The True RNG is provided by the SMpro processor. This function is to send a mailbox
  message to the SMpro to request a 64-bit random number.

  @param[out]  Buffer           A pointer to the read 64-bit random number.

  @retval EFI_SUCCESS           The operation succeeds.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval Otherwise             Errors returned from the MailboxWrite/MailboxRead() functions.
**/
EFI_STATUS
EFIAPI
MailboxMsgGetRandomNumber64 (
  OUT UINT8  *Buffer
  );

/**
  Report the UEFI boot progress to the SMpro.

  @param[in]  Socket           Active socket index.
  @param[in]  BootStage        The stage of the system.
  @param[in]  BootStatus       The status of the stage.
  @param[in]  Checkpoint       The UEFI Checkpoint value.

  @retval EFI_SUCCESS           Set the boot progress successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval Otherwise             Errors returned from the MailboxWrite() functions.
**/
EFI_STATUS
EFIAPI
MailboxMsgSetBootProgress (
  IN UINT8   Socket,
  IN UINT8   BootStage,
  IN UINT8   BootStatus,
  IN UINT32  Checkpoint
  );

/**
  Configure date in SMpro/PMpro.

  @param[in]  Time              A pointer to the date time for configuration.

  @retval EFI_SUCCESS           Configure the date successfully.
  @retval EFI_INVALID_PARAMETER The time parameter is NULL.
  @retval Otherwise             Errors returned from the MailboxWrite() functions.

**/
EFI_STATUS
EFIAPI
MailboxMsgDateConfig (
  IN EFI_TIME  *Time
  );

/**
  Setup runtime date configuration.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval Otherwise             Errors returned from the MailboxRuntimeSetup() functions.

**/
EFI_STATUS
EFIAPI
MailboxMsgDateConfigRuntimeSetup (
  VOID
  );

#endif /* SYSTEM_FIRMWARE_INTERFACE_LIB_H_ */
