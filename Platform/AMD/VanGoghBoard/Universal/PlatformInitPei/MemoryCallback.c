/** @file
  Implements MemoryCallback.C

  This file includes a memory call back function notified when MRC is done,
  following action is performed in this file,
    1. ICH initialization after MRC.
    2. SIO initialization.
    3. Install ResetSystem and FinvFv PPI.
    4. Set MTRR for PEI
    5. Create FV HOB and Flash HOB
  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2013 - 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CommonHeader.h"
#include <Ppi/SmmControl.h>

EFI_STATUS
EFIAPI
S3PostScriptTableCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

EFI_STATUS
EFIAPI
S3EndOfPeiSignalCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

EFI_PEI_NOTIFY_DESCRIPTOR  mNotifyOnS3ResumeList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gPeiPostScriptTablePpiGuid,
    S3PostScriptTableCallback
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiEndOfPeiSignalPpiGuid,
    S3EndOfPeiSignalCallback
  }
};

/**

  Trigger the S3 PostScriptTable notify SW SMI

  @param  PeiServices       PEI Services table.
  @param  NotifyDescriptor  Notify Descriptor.
  @param  Ppi               Ppi

  @return EFI_SUCCESS  The function completed successfully.

**/
EFI_STATUS
EFIAPI
S3PostScriptTableCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS           Status;
  PEI_SMM_CONTROL_PPI  *SmmControl;
  UINT16               SmiCommand;
  UINTN                SmiCommandSize;

  Status = PeiServicesLocatePpi (
             &gPeiSmmControlPpiGuid,
             0,
             NULL,
             (VOID **)&SmmControl
             );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "PeiServicesLocatePpi gPeiSmmControlPpiGuid: %r \n", Status));
    return Status;
  }

  SmiCommand = PcdGet8 (PcdFchOemBeforePciRestoreSwSmi);
  DEBUG ((DEBUG_INFO, "Trigger SW SMI PcdFchOemBeforePciRestoreSwSmi: 0x%X\n", SmiCommand));
  SmiCommandSize = sizeof (SmiCommand);
  Status         = SmmControl->Trigger (
                                 (EFI_PEI_SERVICES **)GetPeiServicesTablePointer (),
                                 SmmControl,
                                 (INT8 *)&SmiCommand,
                                 &SmiCommandSize,
                                 FALSE,
                                 0
                                 );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**

  Trigger the S3 EndOfPeiSignal notify SW SMI

  @param  PeiServices       PEI Services table.
  @param  NotifyDescriptor  Notify Descriptor.
  @param  Ppi               Ppi

  @return EFI_SUCCESS  The function completed successfully.

**/
EFI_STATUS
EFIAPI
S3EndOfPeiSignalCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS           Status;
  PEI_SMM_CONTROL_PPI  *SmmControl;
  UINT16               SmiCommand;
  UINTN                SmiCommandSize;

  Status = PeiServicesLocatePpi (
             &gPeiSmmControlPpiGuid,
             0,
             NULL,
             (VOID **)&SmmControl
             );

  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "PeiServicesLocatePpi gPeiSmmControlPpiGuid: %r \n", Status));
    return Status;
  }

  SmiCommand = PcdGet8 (AcpiRestoreSwSmi);
  DEBUG ((DEBUG_INFO, "Trigger SW SMI AcpiRestoreSwSmi: 0x%X\n", SmiCommand));
  SmiCommandSize = sizeof (SmiCommand);
  Status         = SmmControl->Trigger (
                                 (EFI_PEI_SERVICES **)GetPeiServicesTablePointer (),
                                 SmmControl,
                                 (INT8 *)&SmiCommand,
                                 &SmiCommandSize,
                                 FALSE,
                                 0
                                 );
  ASSERT_EFI_ERROR (Status);

  SmiCommand = PcdGet8 (PcdFchOemAfterPciRestoreSwSmi);
  DEBUG ((DEBUG_INFO, "Trigger SW SMI PcdFchOemAfterPciRestoreSwSmi: 0x%X\n", SmiCommand));
  SmiCommandSize = sizeof (SmiCommand);
  Status         = SmmControl->Trigger (
                                 (EFI_PEI_SERVICES **)GetPeiServicesTablePointer (),
                                 SmmControl,
                                 (INT8 *)&SmiCommand,
                                 &SmiCommandSize,
                                 FALSE,
                                 0
                                 );
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**

  Callback function after Memory discovered.

  @param  PeiServices       PEI Services table.
  @param  NotifyDescriptor  Notify Descriptor.
  @param  Ppi               Ppi

  @return EFI_STATUS

**/
EFI_STATUS
EFIAPI
MemoryDiscoveredPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS     Status;
  EFI_BOOT_MODE  BootMode;
  UINT8          CpuAddressWidth;
  UINT32         RegEax;

 #ifndef FV_RECOVERY_MAIN_COMBINE_SUPPORT
  UINT32  Pages;
  VOID    *Memory;
 #endif

  //
  // Get boot mode
  //
  Status = PeiServicesGetBootMode (&BootMode);
  ASSERT_EFI_ERROR (Status);

  if (BootMode == BOOT_ON_S3_RESUME) {
    Status = PeiServicesNotifyPpi (&mNotifyOnS3ResumeList[0]);
    ASSERT_EFI_ERROR (Status);
  }

  if (PcdGet8 (PcdFspModeSelection) == 0) {
    // Dispatch Mode
    SetPeiCacheMode ((const struct _EFI_PEI_SERVICES **)PeiServices);
  }

  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_MAPPED_IO,
    (EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE),
    SPI_BASE,
    0x1000
    );

  //
  // Local APIC
  //
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_MAPPED_IO,
    (EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE),
    PcdGet32 (PcdCpuLocalApicBaseAddress),
    0x1000
    );

  //
  // IO APIC
  //
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_MAPPED_IO,
    (EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE),
    PcdGet32 (PcdIoApicBaseAddress),
    0x1000
    );

  AsmCpuid (0x80000001, &RegEax, NULL, NULL, NULL);
  if (((RegEax >> 20) & 0xFF) == 0x8) {
    // For F17: Reserved memory from BootFvBase - (BootFvBase+BootFvSize-1)
    DEBUG ((DEBUG_INFO, "Family 17: Reserved memory for BFV\n"));
    BuildMemoryAllocationHob (
      PcdGet32 (PcdMemoryFvRecoveryBase),
      PcdGet32 (PcdFlashFvRecoverySize),
      EfiReservedMemoryType
      );
  }

  DEBUG ((DEBUG_INFO, "PcdMemoryFvRecoveryBase: %x,PcdFlashFvMainBase: %x\n", PcdGet32 (PcdMemoryFvRecoveryBase), PcdGet32 (PcdFlashFvMainBase)));

  if ((BootMode != BOOT_ON_S3_RESUME) && (BootMode != BOOT_IN_RECOVERY_MODE)) {
 #ifndef FV_RECOVERY_MAIN_COMBINE_SUPPORT
    Pages  =  PcdGet32 (PcdFlashFvMainSize)/0x1000;
    Memory = AllocatePages (Pages);
    CopyMem (Memory, (VOID *)PcdGet32 (PcdFlashFvMainBase), PcdGet32 (PcdFlashFvMainSize));
 #endif
    //
    // DXE FV
    //
    PeiServicesInstallFvInfoPpi (
      NULL,
 #ifdef FV_RECOVERY_MAIN_COMBINE_SUPPORT
      (VOID *)PcdGet32 (PcdFlashFvMainBase),
 #else
      (VOID *)Memory,
 #endif
      PcdGet32 (PcdFlashFvMainSize),
      NULL,
      NULL
      );
  }

  //
  // Adding the Flashpart to the E820 memory table as type 2 memory.
  //
  BuildResourceDescriptorHob (
    EFI_RESOURCE_FIRMWARE_DEVICE,
    (EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE),
    FixedPcdGet32 (PcdFlashAreaBaseAddress),
    FixedPcdGet32 (PcdFlashAreaSize)
    );
  DEBUG ((DEBUG_INFO, "FLASH_BASE_ADDRESS : 0x%x\n", FixedPcdGet32 (PcdFlashAreaBaseAddress)));

  //
  // Create a CPU hand-off information
  //
  CpuAddressWidth = 36;

  BuildCpuHob (CpuAddressWidth, 16);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
