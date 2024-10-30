/** @file

  Copyright (c) 2020 - 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Guid/PlatformInfoHob.h>
#include <Library/AmpereCpuLib.h>
#include <Library/ArmLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PL011UartLib.h>
#include <Library/SerialPortLib.h>
#include <Platform/Ac01.h>
#include <Ppi/ArmMpCoreInfo.h>
#include <Ppi/TemporaryRamSupport.h>

ARM_CORE_INFO  mArmPlatformMpCoreInfoTable[PLATFORM_CPU_MAX_NUM_CORES];

/**
  Return the current Boot Mode

  This function returns the boot reason on the platform

  @return   Return the current Boot Mode of the platform

**/
EFI_BOOT_MODE
ArmPlatformGetBootMode (
  VOID
  )
{
  return BOOT_WITH_FULL_CONFIGURATION;
}

/**
  Initialize controllers that must setup in the normal world

  This function is called by the ArmPlatformPkg/PrePi or ArmPlatformPkg/PlatformPei
  in the PEI phase.

**/
EFI_STATUS
ArmPlatformInitialize (
  IN UINTN  MpId
  )
{
  RETURN_STATUS       Status;
  UINT64              BaudRate;
  UINT32              ReceiveFifoDepth;
  EFI_PARITY_TYPE     Parity;
  UINT8               DataBits;
  EFI_STOP_BITS_TYPE  StopBits;

  Status = EFI_SUCCESS;

  if (FixedPcdGet64 (PcdSerialDbgRegisterBase) != 0) {
    /* Debug port should use the same parameters with console */
    BaudRate         = FixedPcdGet64 (PcdSerialDbgUartBaudRate);
    ReceiveFifoDepth = FixedPcdGet32 (PcdUartDefaultReceiveFifoDepth);
    Parity           = (EFI_PARITY_TYPE)FixedPcdGet8 (PcdUartDefaultParity);
    DataBits         = FixedPcdGet8 (PcdUartDefaultDataBits);
    StopBits         = (EFI_STOP_BITS_TYPE)FixedPcdGet8 (PcdUartDefaultStopBits);

    /* Initialize uart debug port */
    Status = PL011UartInitializePort (
               (UINTN)FixedPcdGet64 (PcdSerialDbgRegisterBase),
               FixedPcdGet32 (PL011UartClkInHz),
               &BaudRate,
               &ReceiveFifoDepth,
               &Parity,
               &DataBits,
               &StopBits
               );
  }

  return Status;
}

EFI_STATUS
PrePeiCoreGetMpCoreInfo (
  OUT UINTN          *CoreCount,
  OUT ARM_CORE_INFO  **ArmCoreTable
  )
{
  UINTN  mArmPlatformCoreCount;
  UINTN  ClusterId;
  UINTN  SocketId;
  UINTN  Index;

  ASSERT (CoreCount != NULL);
  ASSERT (ArmCoreTable != NULL);

  mArmPlatformCoreCount = 0;
  for (Index = 0; Index < PLATFORM_CPU_MAX_NUM_CORES; Index++) {
    if (!IsCpuEnabled (Index)) {
      continue;
    }

    SocketId  = SOCKET_ID (Index);
    ClusterId = CLUSTER_ID (Index);

    mArmPlatformMpCoreInfoTable[mArmPlatformCoreCount].Mpidr =
      AC01_GET_MPIDR ((UINT64)SocketId, ClusterId, (Index % PLATFORM_CPU_NUM_CORES_PER_CPM));

    mArmPlatformMpCoreInfoTable[mArmPlatformCoreCount].MailboxClearAddress = 0;
    mArmPlatformMpCoreInfoTable[mArmPlatformCoreCount].MailboxClearValue   = 0;
    mArmPlatformMpCoreInfoTable[mArmPlatformCoreCount].MailboxGetAddress   = 0;
    mArmPlatformMpCoreInfoTable[mArmPlatformCoreCount].MailboxSetAddress   = 0;
    mArmPlatformCoreCount++;
  }

  *CoreCount = mArmPlatformCoreCount;

  *ArmCoreTable = mArmPlatformMpCoreInfoTable;
  ASSERT (*ArmCoreTable != NULL);

  return EFI_SUCCESS;
}

ARM_MP_CORE_INFO_PPI  mMpCoreInfoPpi = { PrePeiCoreGetMpCoreInfo };

EFI_PEI_PPI_DESCRIPTOR  gPlatformPpiTable[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gArmMpCoreInfoPpiGuid,
    &mMpCoreInfoPpi
  },
};

/**
  Return the Platform specific PPIs

  This function exposes the Platform Specific PPIs. They can be used by any PrePi modules or passed
  to the PeiCore by PrePeiCore.

  @param[out]   PpiListSize         Size in Bytes of the Platform PPI List
  @param[out]   PpiList             Platform PPI List

**/
VOID
ArmPlatformGetPlatformPpiList (
  OUT UINTN                   *PpiListSize,
  OUT EFI_PEI_PPI_DESCRIPTOR  **PpiList
  )
{
  ASSERT (PpiListSize != NULL);
  ASSERT (PpiList != NULL);

  if (ArmIsMpCore ()) {
    *PpiListSize = sizeof (gPlatformPpiTable);
    *PpiList     = gPlatformPpiTable;
  } else {
    *PpiListSize = 0;
    *PpiList     = NULL;
  }
}
