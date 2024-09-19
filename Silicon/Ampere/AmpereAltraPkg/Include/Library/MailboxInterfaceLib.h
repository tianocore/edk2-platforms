/** @file
  The library implements the hardware Mailbox (Doorbell) interface for communication
  between the Application Processor (ARMv8) and the System Control Processors (SMpro/PMpro).

  A transfer to SMpro/PMpro is performed on a doorbell channel which is implemented through
  hardware doorbell registers. Each transfer can be up to 12 bytes long, including 4 bytes
  for the message and two 4 bytes for additional data.

  Copyright (c) 2021 - 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MAILBOX_INTERFACE_LIB_H_
#define MAILBOX_INTERFACE_LIB_H_

#define SMPRO_DB_MAX                    8
#define PMPRO_DB_MAX                    8
#define NUMBER_OF_DOORBELLS_PER_SOCKET  (SMPRO_DB_MAX + PMPRO_DB_MAX)

//
// General address offset of Doorbell registers
//
#define DB_IN_REG_OFST           0x00000000  // Doorbell In
#define DB_DIN0_REG_OFST         0x00000004  // Doorbell In Data
#define DB_DIN1_REG_OFST         0x00000008  // Doorbell In Data
#define DB_OUT_REG_OFST          0x00000010  // Doorbell Out
#define DB_DOUT0_REG_OFST        0x00000014  // Doorbell Out Data
#define DB_DOUT1_REG_OFST        0x00000018  // Doorbell Out Data
#define DB_STATUS_REG_OFST       0x00000020  // Doorbell Interrupt Status
#define DB_STATUS_MASK_REG_OFST  0x00000024  // Doorbell Interrupt Status Mask

//
// List of supported doorbells
//
typedef enum {
  //
  // PMpro Doorbells
  //
  PMproDoorbellChannel0 = 0,
  PMproDoorbellChannel1,
  PMproDoorbellChannel2,
  PMproDoorbellChannel3,
  PMproDoorbellChannel4,
  PMproDoorbellChannel5,
  PMproDoorbellChannel6,
  PMproDoorbellChannel7,
  //
  // SMpro Doorbells
  //
  SMproDoorbellChannel0 = PMPRO_DB_MAX,
  SMproDoorbellChannel1,
  SMproDoorbellChannel2,
  SMproDoorbellChannel3,
  SMproDoorbellChannel4,
  SMproDoorbellChannel5,
  SMproDoorbellChannel6,
  SMproDoorbellChannel7
} DOORBELL_CHANNELS;

#pragma pack(1)
//
// Mailbox Message Data
//
// A mailbox transaction supports up to 12 bytes long,
// including 4 bytes for message and two 4 bytes for extended data.
//
typedef struct {
  UINT32    Data;
  UINT32    ExtendedData[2];
} MAILBOX_MESSAGE_DATA;

#pragma pack()

//
// Timeout configuration when waiting for an doorbell interrupt status
//
#define MAILBOX_POLL_TIMEOUT_US   10000000
#define MAILBOX_POLL_INTERVAL_US  1000
#define MAILBOX_POLL_COUNT        (MAILBOX_POLL_TIMEOUT_US / MAILBOX_POLL_INTERVAL_US)

/**
  Get the base address of a doorbell.

  @param[in]  Socket            Active socket index.
  @param[in]  Doorbell          Doorbell channel for communication with the SMpro/PMpro.

  @retval UINT32                The base address of the doorbell.
                                The returned value is 0 indicate that the input parameters are invalid.

**/
UINTN
EFIAPI
MailboxGetDoorbellAddress (
  IN UINT8              Socket,
  IN DOORBELL_CHANNELS  Doorbell
  );

/**
  Get the interrupt number of a doorbell.

  @param[in]  Socket            Active socket index.
  @param[in]  Doorbell          Doorbell channel for communication with the SMpro/PMpro.

  @retval UINT32                The interrupt number.
                                The returned value is 0 indicate that the input parameters are invalid.

**/
UINT32
EFIAPI
MailboxGetDoorbellInterruptNumber (
  IN UINT8              Socket,
  IN DOORBELL_CHANNELS  Doorbell
  );

/**
  Read a message via the hardware Doorbell interface.

  @param[in]  Socket            Active socket index.
  @param[in]  Doorbell          Doorbell channel for communication with the SMpro/PMpro.
  @param[out] Message           Pointer to the Mailbox message.

  @retval EFI_SUCCESS           Read the message successfully.
  @retval EFI_TIMEOUT           Timeout occurred when waiting for available message in the mailbox.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
EFI_STATUS
EFIAPI
MailboxRead (
  IN  UINT8                 Socket,
  IN  DOORBELL_CHANNELS     Doorbell,
  OUT MAILBOX_MESSAGE_DATA  *Message
  );

/**
  Write a message via the hardware Doorbell interface.

  @param[in]  Socket            Active socket index.
  @param[in]  Doorbell          Doorbel channel for communication with the SMpro/PMpro.
  @param[in]  Message           Pointer to the Mailbox message.

  @retval EFI_SUCCESS           Write the message successfully.
  @retval EFI_TIMEOUT           Timeout occurred when waiting for acknowledge signal from the mailbox.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
EFI_STATUS
EFIAPI
MailboxWrite (
  IN UINT8                 Socket,
  IN DOORBELL_CHANNELS     Doorbell,
  IN MAILBOX_MESSAGE_DATA  *Message
  );

/**
  Unmask the Doorbell interrupt status.

  @param  Socket    Active socket index.
  @param  Doorbell  Doorbel channel for communication with the SMpro/PMpro.

  @retval EFI_SUCCESS            Unmask the Doorbell interrupt successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.

**/
EFI_STATUS
EFIAPI
MailboxUnmaskInterrupt (
  IN UINT8   Socket,
  IN UINT16  Doorbell
  );

/**
  Setup Doorbell for running at runtime.

  @param[in]  Socket            Active socket index.
  @param[in]  Doorbell          Doorbell channel.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.

**/
EFI_STATUS
EFIAPI
MailboxRuntimeSetup (
  IN UINT8              Socket,
  IN DOORBELL_CHANNELS  Doorbell
  );

#endif /* MAILBOX_INTERFACE_LIB_H_ */
