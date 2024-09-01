/** @file
*
*  Copyright (c) 2018 - 2023, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Library/AcpiLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PL011UartLib.h>

#include <IoVirtSoCExp.h>
#include <SgiPlatform.h>

VOID
InitVirtioDevices (
  VOID
  );

/**
  Initialize UART controllers connected to IO Virtualization block.

  Use PL011UartLib Library to initialize UART controllers that are present in
  the SoC expansion block. This SoC expansion block is connected to the IO
  virtualization block on Arm infrastructure reference design (RD) platforms.

  @retval  None
**/
STATIC
VOID
InitIoVirtSocExpBlkUartControllers (
  VOID
  )
{
  EFI_STATUS                 Status;
  EFI_PARITY_TYPE            Parity;
  EFI_STOP_BITS_TYPE         StopBits;
  UINT64                     BaudRate;
  UINT32                     ReceiveFifoDepth;
  UINT8                      DataBits;
  UINT8                      UartIdx;
  UINT32                     ChipIdx;
  UINT64                     UartAddr;

  if (FixedPcdGet32 (PcdIoVirtSocExpBlkUartEnable) == 0) {
    return;
  }

  ReceiveFifoDepth = 0;
  Parity = 1;
  DataBits = 8;
  StopBits = 1;
  BaudRate = 115200;

  for (ChipIdx = 0; ChipIdx < FixedPcdGet32 (PcdChipCount); ChipIdx++) {
    for (UartIdx = 0; UartIdx < 2; UartIdx++) {
      UartAddr = SGI_REMOTE_CHIP_MEM_OFFSET(ChipIdx) + UART_START(UartIdx);

      Status = PL011UartInitializePort (
                 (UINTN)UartAddr,
                 FixedPcdGet32 (PcdSerialDbgUartClkInHz),
                 &BaudRate,
                 &ReceiveFifoDepth,
                 &Parity,
                 &DataBits,
                 &StopBits
                 );

      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "Failed to init PL011_UART%u on IO Virt Block port, status: %r\n",
          UartIdx,
          Status
          ));
      }
    }
  }
}

EFI_STATUS
EFIAPI
ArmSgiPkgEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS              Status;

  Status = LocateAndInstallAcpiFromFv (&gArmSgiAcpiTablesGuid);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to install ACPI tables\n", __func__));
    return Status;
  }

  InitVirtioDevices ();
  InitIoVirtSocExpBlkUartControllers ();

  return Status;
}
