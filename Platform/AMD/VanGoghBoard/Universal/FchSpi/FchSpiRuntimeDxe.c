/** @file
PCH SPI Runtime Driver implements the SPI Host Controller Compatibility Interface.

Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifdef _MSC_VER
  #pragma optimize( "", off )
#endif

#ifdef __GNUC__
  #ifndef __clang__
    #pragma GCC push_options
    #pragma GCC optimize ("O0")
  #else
    #pragma clang optimize off
  #endif
#endif

#include "FchSpiRuntimeDxe.h"

extern EFI_GUID  gEfiEventVirtualAddressChangeGuid;
CONST BOOLEAN    gInSmm = FALSE;

//
// Global variables
//
SPI_INSTANCE  *mSpiInstance;

/**

  Fixup internal data pointers so that the services can be called in virtual mode.

  @param Event     The event registered.
  @param Context   Event context. Not used in this event handler.

  @retval   None

**/
VOID
EFIAPI
FchSpiVirtualddressChangeEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  gRT->ConvertPointer (EFI_INTERNAL_POINTER, (VOID *)&(mSpiInstance->SpiBar));
  gRT->ConvertPointer (EFI_INTERNAL_POINTER, (VOID *)&(mSpiInstance->SpiProtocol.Init));
  gRT->ConvertPointer (EFI_INTERNAL_POINTER, (VOID *)&(mSpiInstance->SpiProtocol.Lock));
  gRT->ConvertPointer (EFI_INTERNAL_POINTER, (VOID *)&(mSpiInstance->SpiProtocol.Execute));
  gRT->ConvertPointer (EFI_INTERNAL_POINTER, (VOID *)&(mSpiInstance));
}

/**

  Entry point for the SPI host controller driver.

  @param ImageHandle       Image handle of this driver.
  @param SystemTable       Global system service table.

  @retval EFI_SUCCESS           Initialization complete.
  @retval EFI_UNSUPPORTED       The chipset is unsupported by this driver.
  @retval EFI_OUT_OF_RESOURCES  Do not have enough resources to initialize the driver.
  @retval EFI_DEVICE_ERROR      Device error, driver exits abnormally.

**/
EFI_STATUS
EFIAPI
InstallFchSpiRuntimeDxe (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                       Status;
  UINT64                           BaseAddress;
  UINT64                           Length;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  GcdMemorySpaceDescriptor;
  UINT64                           Attributes;
  EFI_EVENT                        Event;

  DEBUG ((DEBUG_INFO, "InstallFchSpiRuntimeDxe() Start\n"));

  //
  // Allocate Runtime memory for the SPI protocol instance.
  //
  mSpiInstance = AllocateRuntimeZeroPool (sizeof (SPI_INSTANCE));
  if (mSpiInstance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initialize the SPI protocol instance
  //
  Status = SpiProtocolConstructor (mSpiInstance);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install the EFI_SPI_PROTOCOL interface
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &(mSpiInstance->Handle),
                  &gEfiSpiProtocolGuid,
                  &(mSpiInstance->SpiProtocol),
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    FreePool (mSpiInstance);
    return EFI_DEVICE_ERROR;
  }

  Status = mSpiInstance->SpiProtocol.Init (&(mSpiInstance->SpiProtocol));
  ASSERT_EFI_ERROR (Status);

  //
  // Set Spi space in GCD to be RUNTIME so that the range will be supported in
  // virtual address mode in EFI aware OS runtime.
  // It will assert if Spi Memory Space is not allocated
  // The caller is responsible for the existence and allocation of the Spi Memory Spaces
  //
  BaseAddress = (EFI_PHYSICAL_ADDRESS)(mSpiInstance->SpiBar);
  Length      = 0x1000;

  Status = gDS->GetMemorySpaceDescriptor (BaseAddress, &GcdMemorySpaceDescriptor);
  ASSERT_EFI_ERROR (Status);

  Attributes = GcdMemorySpaceDescriptor.Attributes | EFI_MEMORY_RUNTIME;

  Status = gDS->SetMemorySpaceAttributes (
                  BaseAddress,
                  Length,
                  Attributes
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  FchSpiVirtualddressChangeEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &Event
                  );
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "InstallFchSpiRuntimeDxe() End\n"));

  return EFI_SUCCESS;
}

#ifdef _MSC_VER
  #pragma optimize( "", on )
#endif
#ifdef __GNUC__
  #ifndef __clang__
    #pragma GCC pop_options
  #endif
#endif
