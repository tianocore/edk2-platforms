/** @file
  The library implements the hardware Mailbox (Doorbell) interface for communication
  between the Application Processor (ARMv8) and the System Control Processors (SMpro/PMpro).

  Copyright (c) 2021 - 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/AmpereCpuLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MailboxInterfaceLib.h>
#include <Library/TimerLib.h>
#include <Library/IoLib.h>
#include <Platform/Ac01.h>

//
// Hardware Doorbells
//
#define SMPRO_DB0_IRQ_OFST      40
#define SMPRO_DB0_BASE_ADDRESS  (FixedPcdGet64 (PcdSmproDbBaseReg))

#define PMPRO_DB0_IRQ_OFST      56
#define PMPRO_DB0_BASE_ADDRESS  (FixedPcdGet64 (PcdPmproDbBaseReg))

//
// The base SPI interrupt number of the Slave socket
//
#define SLAVE_SOCKET_SPI_INTERRUPT  352

#define SLAVE_SOCKET_DOORBELL_INTERRUPT_BASE(Socket)  ((Socket) * SLAVE_SOCKET_SPI_INTERRUPT - 32)

//
// Doorbell base register stride size
//
#define DB_BASE_REG_STRIDE  0x00001000

#define SMPRO_DBx_ADDRESS(socket, db) \
        ((socket) * SLAVE_SOCKET_BASE_ADDRESS_OFFSET + SMPRO_DB0_BASE_ADDRESS + DB_BASE_REG_STRIDE * (db))

#define PMPRO_DBx_ADDRESS(socket, db) \
        ((socket) * SLAVE_SOCKET_BASE_ADDRESS_OFFSET + PMPRO_DB0_BASE_ADDRESS + DB_BASE_REG_STRIDE * (db))

//
// Doorbell Status Bits
//
#define DB_STATUS_AVAIL_BIT  BIT16
#define DB_STATUS_ACK_BIT    BIT0

UINTN  gDoorbellBaseAddress[PLATFORM_CPU_MAX_SOCKET][NUMBER_OF_DOORBELLS_PER_SOCKET] = {
  { 0 }
};

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
  )
{
  UINTN  SocketId;
  UINTN  DoorbellId;

  if (  (Socket >= GetNumberOfActiveSockets ())
     || (Doorbell >= NUMBER_OF_DOORBELLS_PER_SOCKET))
  {
    return 0;
  }

  //
  // Setup Doorbell base address at the first attempt
  //
  if (gDoorbellBaseAddress[0][0] == 0) {
    for (SocketId = 0; SocketId < PLATFORM_CPU_MAX_SOCKET; SocketId++) {
      for (DoorbellId = 0; DoorbellId < NUMBER_OF_DOORBELLS_PER_SOCKET; DoorbellId++) {
        if (DoorbellId >= SMproDoorbellChannel0) {
          gDoorbellBaseAddress[SocketId][DoorbellId] = SMPRO_DBx_ADDRESS (SocketId, (UINT8)(DoorbellId - SMproDoorbellChannel0));
        } else {
          gDoorbellBaseAddress[SocketId][DoorbellId] = PMPRO_DBx_ADDRESS (SocketId, (UINT8)DoorbellId);
        }
      }
    }
  }

  return gDoorbellBaseAddress[Socket][Doorbell];
}

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
  )
{
  UINT32  DoorbellInterruptNumber;

  if (  (Socket >= GetNumberOfActiveSockets ())
     || (Doorbell >= NUMBER_OF_DOORBELLS_PER_SOCKET))
  {
    return 0;
  }

  DoorbellInterruptNumber = 0;

  if (Socket > 0) {
    DoorbellInterruptNumber = SLAVE_SOCKET_DOORBELL_INTERRUPT_BASE (Socket);
  }

  if (Doorbell >= SMproDoorbellChannel0) {
    DoorbellInterruptNumber += SMPRO_DB0_IRQ_OFST + (UINT8)(Doorbell - SMproDoorbellChannel0);
  } else {
    DoorbellInterruptNumber += PMPRO_DB0_IRQ_OFST + (UINT8)Doorbell;
  }

  return DoorbellInterruptNumber;
}

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
  )
{
  UINTN  TimeoutCount;
  UINTN  DoorbellAddress;

  if (  (Socket >= GetNumberOfActiveSockets ())
     || (Doorbell >= NUMBER_OF_DOORBELLS_PER_SOCKET)
     || (Message == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  TimeoutCount = MAILBOX_POLL_COUNT;

  DoorbellAddress = MailboxGetDoorbellAddress (Socket, Doorbell);
  ASSERT (DoorbellAddress != 0);

  //
  // Polling Doorbell status
  //
  while ((MmioRead32 ((DoorbellAddress + DB_STATUS_REG_OFST)) & DB_STATUS_AVAIL_BIT) == 0) {
    MicroSecondDelay (MAILBOX_POLL_INTERVAL_US);
    if (--TimeoutCount == 0) {
      return EFI_TIMEOUT;
    }
  }

  Message->ExtendedData[0] = MmioRead32 (DoorbellAddress + DB_DIN0_REG_OFST);
  Message->ExtendedData[1] = MmioRead32 (DoorbellAddress + DB_DIN1_REG_OFST);
  Message->Data            = MmioRead32 (DoorbellAddress + DB_IN_REG_OFST);

  //
  // Write 1 to clear the AVAIL status
  //
  MmioWrite32 (DoorbellAddress + DB_STATUS_REG_OFST, DB_STATUS_AVAIL_BIT);

  return EFI_SUCCESS;
}

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
  )
{
  UINTN  TimeoutCount;
  UINTN  DoorbellAddress;

  if (  (Socket >= GetNumberOfActiveSockets ())
     || (Doorbell >= NUMBER_OF_DOORBELLS_PER_SOCKET)
     || (Message == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  TimeoutCount = MAILBOX_POLL_COUNT;

  DoorbellAddress = MailboxGetDoorbellAddress (Socket, Doorbell);
  ASSERT (DoorbellAddress != 0);

  //
  // Clear previous pending ack if any
  //
  if ((MmioRead32 (DoorbellAddress + DB_STATUS_REG_OFST) & DB_STATUS_ACK_BIT) != 0) {
    MmioWrite32 (DoorbellAddress + DB_STATUS_REG_OFST, DB_STATUS_ACK_BIT);
  }

  //
  // Send message
  //
  MmioWrite32 (DoorbellAddress + DB_DOUT0_REG_OFST, Message->ExtendedData[0]);
  MmioWrite32 (DoorbellAddress + DB_DOUT1_REG_OFST, Message->ExtendedData[1]);
  MmioWrite32 (DoorbellAddress + DB_OUT_REG_OFST, Message->Data);

  //
  // Wait for ACK
  //
  while ((MmioRead32 (DoorbellAddress + DB_STATUS_REG_OFST) & DB_STATUS_ACK_BIT) == 0) {
    MicroSecondDelay (MAILBOX_POLL_INTERVAL_US);
    if (--TimeoutCount == 0) {
      return EFI_TIMEOUT;
    }
  }

  //
  // Write 1 to clear the ACK status
  //
  MmioWrite32 (DoorbellAddress + DB_STATUS_REG_OFST, DB_STATUS_ACK_BIT);

  return EFI_SUCCESS;
}

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
  )
{
  UINTN  DoorbellAddress;

  if (  (Socket >= GetNumberOfActiveSockets ())
     || (Doorbell >= NUMBER_OF_DOORBELLS_PER_SOCKET))
  {
    return EFI_INVALID_PARAMETER;
  }

  DoorbellAddress = MailboxGetDoorbellAddress (Socket, Doorbell);
  ASSERT (DoorbellAddress != 0);

  MmioWrite32 (DoorbellAddress + DB_STATUS_MASK_REG_OFST, ~DB_STATUS_AVAIL_BIT);

  return EFI_SUCCESS;
}
