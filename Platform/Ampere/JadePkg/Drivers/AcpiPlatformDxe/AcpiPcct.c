/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AcpiPlatform.h"

#include <Library/MailboxInterfaceLib.h>
#include <Library/SystemFirmwareInterfaceLib.h>
#include <Library/TimerLib.h>

//
// The communication data for advertising the shared memory address to the SMpro/PMpro.
//
#define PCC_PAYLOAD_ADVERTISE_ADDRESS  0x0F000000
#define PCC_PAYLOAD_SIZE               12        // Bytes

//
// ACPI Platform Communication Channel (PCC)
//
#define PCC_SUBSPACE_SHARED_MEM_SIGNATURE  0x50434300    // "PCC"
#define PCC_SUBSPACE_SHARED_MEM_SIZE       0x4000        // Number of Bytes

//
// Mask for available doorbells. This is used to reserve
// doorbells which are for other communications with OS.
//
// Each bit in the mask represents each doorbell channel
// from PMpro Doorbell 0 to PMpro Doorbell 7 and SMpro
// Doorbell channel 0 to SMpro Doorbell channel 7.
//
#define PCC_AVAILABLE_DOORBELL_MASK  0xEFFFEFFF

#define PCC_MAX_VALID_DOORBELL_COUNT  (NUMBER_OF_DOORBELLS_PER_SOCKET * PLATFORM_CPU_MAX_SOCKET)

#define PCC_NOMINAL_LATENCY_US          1000       // us
#define PCC_MAX_PERIODIC_ACCESS_RATE    0          // no limitation
#define PCC_MIN_REQ_TURNAROUND_TIME_US  0

// Polling interval for PCC Command Complete
#define PCC_COMMAND_POLL_INTERVAL_US  10

#define PCC_COMMAND_POLL_COUNT  (PCC_NOMINAL_LATENCY_US / PCC_COMMAND_POLL_INTERVAL_US)

//
// Doorbell Channel for CPPC
//
#define PCC_CPPC_DOORBELL_ID  (PMproDoorbellChannel2)

#define PCC_CPPC_NOMINAL_LATENCY_US          100
#define PCC_CPPC_MIN_REQ_TURNAROUND_TIME_US  110

//
// Shared memory regions allocated for PCC subspaces
//
STATIC EFI_PHYSICAL_ADDRESS  mPccSharedMemoryAddress;
STATIC UINTN                 mPccSharedMemorySize;

EFI_ACPI_6_3_PCCT_SUBSPACE_2_HW_REDUCED_COMMUNICATIONS  PcctSubspaceTemplate = {
  EFI_ACPI_6_3_PCCT_SUBSPACE_TYPE_2_HW_REDUCED_COMMUNICATIONS,
  sizeof (EFI_ACPI_6_3_PCCT_SUBSPACE_2_HW_REDUCED_COMMUNICATIONS),
  0,                                                                                    // PlatformInterrupt
  0,                                                                                    // PlatformInterruptFlags
  0,                                                                                    // Reserved
  0,                                                                                    // BaseAddress
  0x100,                                                                                // AddressLength
  { 0,                                                            0x20,  0, 0x3, 0x0 }, // DoorbellRegister
  0,                                                                                    // DoorbellPreserve
  0x53000040,                                                                           // DoorbellWrite
  1,                                                                                    // NominalLatency
  1,                                                                                    // MaximumPeriodicAccessRate
  1,                                                                                    // MinimumRequestTurnaroundTime
  { 0,                                                            0x20,  0, 0x3, 0x0 }, // PlatformInterruptAckRegister
  0,                                                                                    // PlatformInterruptAckPreserve
  0x10001,                                                                              // PlatformInterruptAckWrite
};

EFI_ACPI_6_3_PLATFORM_COMMUNICATION_CHANNEL_TABLE_HEADER  PcctTableHeaderTemplate = {
  __ACPI_HEADER (
    EFI_ACPI_6_3_PLATFORM_COMMUNICATIONS_CHANNEL_TABLE_SIGNATURE,
    EFI_ACPI_6_3_PLATFORM_COMMUNICATION_CHANNEL_TABLE_HEADER,
    EFI_ACPI_6_3_PLATFORM_COMMUNICATION_CHANNEL_TABLE_REVISION
    ),
  EFI_ACPI_6_3_PCCT_FLAGS_PLATFORM_INTERRUPT,
};

/**
  Check whether the Doorbell is reserved or not.

**/
BOOLEAN
PcctIsDoorbellReserved (
  IN UINT16  Doorbell
  )
{
  ASSERT (Doorbell <= PCC_MAX_VALID_DOORBELL_COUNT);

  if (((1 << Doorbell) & PCC_AVAILABLE_DOORBELL_MASK) == 0) {
    return TRUE;
  }

  return FALSE;
}

/**
  Get number of available doorbells for the usage of PCC communication.

**/
UINT16
PcctGetNumberOfAvailableDoorbells (
  VOID
  )
{
  UINT16  Doorbell;
  UINT16  AvailableDoorbellCount;

  AvailableDoorbellCount = 0;
  for (Doorbell = 0; Doorbell < NUMBER_OF_DOORBELLS_PER_SOCKET; Doorbell++ ) {
    if (((1 << Doorbell) & PCC_AVAILABLE_DOORBELL_MASK) != 0) {
      AvailableDoorbellCount++;
    }
  }

  ASSERT (AvailableDoorbellCount <= PCC_MAX_VALID_DOORBELL_COUNT);

  return AvailableDoorbellCount;
}

/**
  Allocate memory pages for the PCC shared memory region.

**/
EFI_STATUS
PcctAllocateSharedMemory (
  IN  UINT16                SubspaceCount,
  OUT EFI_PHYSICAL_ADDRESS  *PccSharedMemoryPtr
  )
{
  EFI_STATUS  Status;

  mPccSharedMemorySize = PCC_SUBSPACE_SHARED_MEM_SIZE * SubspaceCount;

  Status = gBS->AllocatePages (
                  AllocateAnyPages,
                  EfiRuntimeServicesData,
                  EFI_SIZE_TO_PAGES (mPccSharedMemorySize),
                  &mPccSharedMemoryAddress
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate PCC shared memory\n"));
    mPccSharedMemorySize = 0;
    return Status;
  }

  *PccSharedMemoryPtr = mPccSharedMemoryAddress;

  return EFI_SUCCESS;
}

EFI_PHYSICAL_ADDRESS
PcctGetSharedMemoryAddress (
  IN  UINT8   Socket,
  IN  UINT16  Subspace
  )
{
  ASSERT (Socket < PLATFORM_CPU_MAX_SOCKET);

  return (mPccSharedMemoryAddress + PCC_SUBSPACE_SHARED_MEM_SIZE * Subspace);
}

/**
  Free the whole shared memory region that is allocated by
  the PcctAllocateSharedMemory() function.

**/
VOID
PcctFreeSharedMemory (
  VOID
  )
{
  if ((mPccSharedMemoryAddress != 0) && (mPccSharedMemorySize != 0)) {
    gBS->FreePages (
           mPccSharedMemoryAddress,
           EFI_SIZE_TO_PAGES (mPccSharedMemorySize)
           );

    mPccSharedMemoryAddress = 0;
  }
}

/**
  This function is to advertise the shared memory region address
  to the platform (SMpro/PMpro).

**/
EFI_STATUS
PcctAdvertiseSharedMemoryAddress (
  IN UINT8   Socket,
  IN UINT16  Doorbell,
  IN UINT16  Subspace
  )
{
  EFI_STATUS                                             Status;
  EFI_ACPI_6_3_PCCT_GENERIC_SHARED_MEMORY_REGION_HEADER  *PccSharedMemoryRegion;
  UINT32                                                 CommunicationData;
  UINTN                                                  Timeout;

  if (  (Socket >= PLATFORM_CPU_MAX_SOCKET)
     || (Doorbell >= NUMBER_OF_DOORBELLS_PER_SOCKET))
  {
    return EFI_INVALID_PARAMETER;
  }

  PccSharedMemoryRegion = (EFI_ACPI_6_3_PCCT_GENERIC_SHARED_MEMORY_REGION_HEADER *)
                          PcctGetSharedMemoryAddress (Socket, Subspace);
  ASSERT (PccSharedMemoryRegion != NULL);

  //
  // Zero shared memory region for each PCC subspace
  //
  SetMem (
    (VOID *)PccSharedMemoryRegion,
    sizeof (EFI_ACPI_6_3_PCCT_GENERIC_SHARED_MEMORY_REGION_HEADER) + PCC_PAYLOAD_SIZE,
    0
    );

  // Advertise shared memory address to Platform (SMpro/PMpro)
  // by ringing the doorbell with dummy PCC message
  //
  CommunicationData = PCC_PAYLOAD_ADVERTISE_ADDRESS;

  //
  // Write Data into Communication Space Region
  //
  CopyMem ((VOID *)(PccSharedMemoryRegion + 1), &CommunicationData, sizeof (CommunicationData));

  PccSharedMemoryRegion->Status.CommandComplete = 0;
  PccSharedMemoryRegion->Signature              = PCC_SUBSPACE_SHARED_MEM_SIGNATURE | Subspace;

  Status = MailboxMsgSetPccSharedMem (Socket, Doorbell, TRUE, (UINT64)PccSharedMemoryRegion);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to send mailbox message!\n", __func__));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  //
  // Polling CMD_COMPLETE bit
  //
  Timeout = PCC_COMMAND_POLL_COUNT;
  while (PccSharedMemoryRegion->Status.CommandComplete != 1) {
    if (--Timeout <= 0) {
      DEBUG ((DEBUG_ERROR, "%a - Timeout occurred when polling the PCC Status Complete\n", __func__));
      return EFI_TIMEOUT;
    }

    MicroSecondDelay (PCC_COMMAND_POLL_INTERVAL_US);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
AcpiPcctInitializeSharedMemory (
  VOID
  )
{
  UINT8   SocketCount;
  UINT8   Socket;
  UINT16  Doorbell;
  UINT16  Subspace;

  SocketCount = GetNumberOfActiveSockets ();
  Subspace    = 0;

  for (Socket = 0; Socket < SocketCount; Socket++) {
    for (Doorbell = 0; Doorbell < NUMBER_OF_DOORBELLS_PER_SOCKET; Doorbell++ ) {
      if (PcctIsDoorbellReserved (Doorbell + NUMBER_OF_DOORBELLS_PER_SOCKET * Socket)) {
        continue;
      }

      PcctAdvertiseSharedMemoryAddress (Socket, Doorbell, Subspace);
      MailboxUnmaskInterrupt (Socket, Doorbell);

      Subspace++;
    }
  }

  return EFI_SUCCESS;
}

/**
  Install PCCT table.

  PCC channels are associated with hardware doorbell instances to provide
  bi-directional communication between the OS and platform entities.

**/
EFI_STATUS
AcpiInstallPcctTable (
  VOID
  )
{
  EFI_STATUS                                                Status;
  EFI_ACPI_6_3_PLATFORM_COMMUNICATION_CHANNEL_TABLE_HEADER  *PcctTableHeader;
  EFI_ACPI_6_3_PCCT_SUBSPACE_2_HW_REDUCED_COMMUNICATIONS    *PccSubspacePtr;
  EFI_PHYSICAL_ADDRESS                                      PccSharedMemoryPtr;
  EFI_ACPI_TABLE_PROTOCOL                                   *AcpiTableProtocol;
  UINTN                                                     PcctTableKey;
  UINT8                                                     SocketCount;
  UINT8                                                     Socket;
  UINT16                                                    Doorbell;
  UINT16                                                    Subspace;
  UINT16                                                    SubspaceCount;
  UINTN                                                     Size;
  UINTN                                                     DoorbellAddress;
  UINTN                                                     DoorbellCount;

  Subspace      = 0;
  SocketCount   = GetNumberOfActiveSockets ();
  DoorbellCount = PcctGetNumberOfAvailableDoorbells ();

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTableProtocol
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SubspaceCount = DoorbellCount * SocketCount;

  PcctAllocateSharedMemory (SubspaceCount, &PccSharedMemoryPtr);
  if (PccSharedMemoryPtr == (EFI_PHYSICAL_ADDRESS)(UINTN)NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Size = sizeof (EFI_ACPI_6_3_PLATFORM_COMMUNICATION_CHANNEL_TABLE_HEADER) +
         SubspaceCount * sizeof (EFI_ACPI_6_3_PCCT_SUBSPACE_2_HW_REDUCED_COMMUNICATIONS);

  PcctTableHeader = (EFI_ACPI_6_3_PLATFORM_COMMUNICATION_CHANNEL_TABLE_HEADER *)AllocateZeroPool (Size);
  if (PcctTableHeader == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  PccSubspacePtr = (EFI_ACPI_6_3_PCCT_SUBSPACE_2_HW_REDUCED_COMMUNICATIONS *)(PcctTableHeader + 1);

  for (Socket = 0; Socket < SocketCount; Socket++) {
    for (Doorbell = 0; Doorbell < NUMBER_OF_DOORBELLS_PER_SOCKET; Doorbell++ ) {
      if (PcctIsDoorbellReserved (Doorbell + NUMBER_OF_DOORBELLS_PER_SOCKET * Socket)) {
        continue;
      }

      CopyMem (
        PccSubspacePtr,
        &PcctSubspaceTemplate,
        sizeof (EFI_ACPI_6_3_PCCT_SUBSPACE_2_HW_REDUCED_COMMUNICATIONS)
        );

      PccSubspacePtr->BaseAddress   = (UINT64)PccSharedMemoryPtr + PCC_SUBSPACE_SHARED_MEM_SIZE * Subspace;
      PccSubspacePtr->AddressLength = PCC_SUBSPACE_SHARED_MEM_SIZE;

      DoorbellAddress = MailboxGetDoorbellAddress (Socket, Doorbell);

      PccSubspacePtr->DoorbellRegister.Address             = DoorbellAddress + DB_OUT_REG_OFST;
      PccSubspacePtr->PlatformInterrupt                    = MailboxGetDoorbellInterruptNumber (Socket, Doorbell);
      PccSubspacePtr->PlatformInterruptAckRegister.Address = DoorbellAddress + DB_STATUS_REG_OFST;

      if (Doorbell == PCC_CPPC_DOORBELL_ID) {
        PccSubspacePtr->DoorbellWrite                = MAILBOX_URGENT_CPPC_MESSAGE;
        PccSubspacePtr->NominalLatency               = PCC_CPPC_NOMINAL_LATENCY_US;
        PccSubspacePtr->MinimumRequestTurnaroundTime = PCC_CPPC_MIN_REQ_TURNAROUND_TIME_US;
      } else {
        PccSubspacePtr->DoorbellWrite                = MAILBOX_TYPICAL_PCC_MESSAGE;
        PccSubspacePtr->NominalLatency               = PCC_NOMINAL_LATENCY_US;
        PccSubspacePtr->MinimumRequestTurnaroundTime = PCC_MIN_REQ_TURNAROUND_TIME_US;
      }

      PccSubspacePtr->MaximumPeriodicAccessRate = PCC_MAX_PERIODIC_ACCESS_RATE;

      PccSubspacePtr++;
      Subspace++;
    }
  }

  CopyMem (
    PcctTableHeader,
    &PcctTableHeaderTemplate,
    sizeof (EFI_ACPI_6_3_PLATFORM_COMMUNICATION_CHANNEL_TABLE_HEADER)
    );

  PcctTableHeader->Header.Length = Size;

  AcpiUpdateChecksum (
    (UINT8 *)PcctTableHeader,
    PcctTableHeader->Header.Length
    );

  Status = AcpiTableProtocol->InstallAcpiTable (
                                AcpiTableProtocol,
                                (VOID *)PcctTableHeader,
                                PcctTableHeader->Header.Length,
                                &PcctTableKey
                                );
  if (EFI_ERROR (Status)) {
    PcctFreeSharedMemory ();
    FreePool ((VOID *)PcctTableHeader);
    return Status;
  }

  return EFI_SUCCESS;
}
