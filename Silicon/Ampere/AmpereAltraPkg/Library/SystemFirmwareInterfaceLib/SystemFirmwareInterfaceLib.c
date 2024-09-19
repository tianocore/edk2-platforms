/** @file
  Provides functions for communication with System Firmware (SMpro/PMpro and ATF).

  Copyright (c) 2021 - 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/AmpereCpuLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MailboxInterfaceLib.h>
#include <Library/SystemFirmwareInterfaceLib.h>

#define SMPRO_RTC_YEAR_SHIFT   16
#define SMPRO_RTC_YEAR_MASK    0xFFFF0000
#define SMPRO_RTC_MONTH_SHIFT  8
#define SMPRO_RTC_MONTH_MASK   0x0000FF00
#define SMPRO_RTC_DAY_SHIFT    0
#define SMPRO_RTC_DAY_MASK     0x000000FF

#define CONVERT_EFI_TIME_TO_SMPRO_DATE(x) \
          (((x)->Year << SMPRO_RTC_YEAR_SHIFT) & SMPRO_RTC_YEAR_MASK) | \
          (((x)->Month << SMPRO_RTC_MONTH_SHIFT) & SMPRO_RTC_MONTH_MASK) | \
          (((x)->Day << SMPRO_RTC_DAY_SHIFT) & SMPRO_RTC_DAY_MASK)
#define MAILBOX_TRNG_MESSAGE_PARAM1_MASK  0x000000FF

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
  )
{
  EFI_STATUS            Status;
  MAILBOX_MESSAGE_DATA  Message;
  UINT32                AddressLower32Bit;
  UINT32                AddressUpper32Bit;

  if (Socket >= GetNumberOfActiveSockets ()) {
    return EFI_INVALID_PARAMETER;
  }

  AddressLower32Bit = (UINT32)(Address & 0xFFFFFFFF);
  AddressUpper32Bit = (UINT32)RShiftU64 ((UINT64)Address, 32);

  Message.Data = MAILBOX_DEBUG_MESSAGE_ENCODE (
                   MAILBOX_DEBUG_MESSAGE_SUBTYPE_REGISTER_READ,
                   (UINT16)(AddressUpper32Bit & 0xFFFF)
                   );

  Message.ExtendedData[0] = AddressLower32Bit;
  Message.ExtendedData[1] = 0;

  Status = MailboxWrite (Socket, SMproDoorbellChannel0, &Message);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = MailboxRead (Socket, SMproDoorbellChannel0, &Message);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((Message.Data & 0xFF00) == 0) {
    return EFI_UNSUPPORTED;
  }

  if (Value != NULL) {
    *Value = Message.ExtendedData[0];
  }

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS            Status;
  MAILBOX_MESSAGE_DATA  Message;
  UINT32                AddressLower32Bit;
  UINT32                AddressUpper32Bit;

  if (Socket >= GetNumberOfActiveSockets ()) {
    return EFI_INVALID_PARAMETER;
  }

  AddressLower32Bit = (UINT32)(Address & 0xFFFFFFFF);
  AddressUpper32Bit = (UINT32)RShiftU64 ((UINT64)Address, 32);

  Message.Data = MAILBOX_DEBUG_MESSAGE_ENCODE (
                   MAILBOX_DEBUG_MESSAGE_SUBTYPE_REGISTER_WRITE,
                   (UINT16)(AddressUpper32Bit & 0xFFFF)
                   );

  Message.ExtendedData[0] = AddressLower32Bit;
  Message.ExtendedData[1] = Value;

  Status = MailboxWrite (Socket, SMproDoorbellChannel0, &Message);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

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
  )
{
  EFI_STATUS            Status;
  MAILBOX_MESSAGE_DATA  Message;
  UINT8                 AlignBit;
  UINT8                 AlignControl;

  if ((Socket >= GetNumberOfActiveSockets ()) || (Doorbell >= NUMBER_OF_DOORBELLS_PER_SOCKET)) {
    return EFI_INVALID_PARAMETER;
  }

  if (AddressAlign256) {
    AlignBit     = 8;
    AlignControl = MAILBOX_ADDRESS_256_ALIGNMENT;
  } else {
    AlignBit     = 0;
    AlignControl = MAILBOX_ADDRESS_NO_ALIGNMENT;
  }

  Message.Data = MAILBOX_ADDRESS_MESSAGE_ENCODE (
                   MAILBOX_ADDRESS_MESSAGE_SUBTYPE_PCC,
                   0,
                   AlignControl
                   );

  Message.ExtendedData[0] = (UINT32)(RShiftU64 ((UINT64)Address, AlignBit) & 0xFFFFFFFF);
  Message.ExtendedData[1] = (UINT32)(RShiftU64 ((UINT64)Address, 32 + AlignBit));

  Status = MailboxWrite (Socket, Doorbell, &Message);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

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
  )
{
  EFI_STATUS            Status;
  MAILBOX_MESSAGE_DATA  Message;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Message.Data = MAILBOX_USER_MESSAGE_ENCODE (
                   MAILBOX_USER_MESSAGE_SUBTYPE_TRNG_PROXY,
                   MAILBOX_TRNG_PROXY_GET_RANDOM_NUMBER,
                   0
                   );
  Message.ExtendedData[0] = 0;
  Message.ExtendedData[1] = 0;

  Status = MailboxWrite (0, SMproDoorbellChannel6, &Message);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = MailboxRead (0, SMproDoorbellChannel6, &Message);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((Message.Data & MAILBOX_TRNG_MESSAGE_PARAM1_MASK) != 0) {
    return EFI_DEVICE_ERROR;
  }

  CopyMem (Buffer, &Message.ExtendedData[0], sizeof (UINT32));
  CopyMem (Buffer + sizeof (UINT32), &Message.ExtendedData[1], sizeof (UINT32));

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS            Status;
  MAILBOX_MESSAGE_DATA  Message;

  if (Socket >= GetNumberOfActiveSockets ()) {
    return EFI_INVALID_PARAMETER;
  }

  Message.Data = MAILBOX_USER_MESSAGE_ENCODE (
                   MAILBOX_USER_MESSAGE_SUBTYPE_BOOT_PROGRESS,
                   MAILBOX_BOOT_PROGRESS_COMMAND_SET,
                   BootStage
                   );

  //
  // Extended Data Format for Boot Progress Set
  //
  // Data 0:
  //   Bit 31:16 - Boot Status
  //   Bit 15:0  - UEFI Checkpoint lower 16-bit
  //
  // Data 1:
  //   Bit 31:16 - Unused
  //   Bit 15:0  - UEFI Checkpoint upper 16-bit
  //
  Message.ExtendedData[0] = ((UINT32)BootStatus & 0xFFFF) | (((UINT32)Checkpoint << 16) & 0xFFFF0000);
  Message.ExtendedData[1] = (Checkpoint >> 16) & 0xFFFF;

  Status = MailboxWrite (Socket, SMproDoorbellChannel1, &Message);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

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
  )
{
  EFI_STATUS            Status;
  MAILBOX_MESSAGE_DATA  Message;
  UINT32                Date;

  if (Time == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Message.Data = MAILBOX_USER_MESSAGE_ENCODE (
                   MAILBOX_USER_MESSAGE_SUBTYPE_SET_CONFIGURATION,
                   MAILBOX_SET_CONFIGURATION_DATE,
                   0
                   );

  Date = CONVERT_EFI_TIME_TO_SMPRO_DATE (Time);

  Message.ExtendedData[0] = Date;
  Message.ExtendedData[1] = 0;

  Status = MailboxWrite (0, PMproDoorbellChannel1, &Message);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
