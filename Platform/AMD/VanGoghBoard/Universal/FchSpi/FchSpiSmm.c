/** @file

PCH SPI SMM Driver implements the SPI Host Controller Compatibility Interface.

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

#include "FchSpiSmm.h"

CONST BOOLEAN  gInSmm = TRUE;

//
// Global variables
//
SPI_INSTANCE  *mSpiInstance;

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
InstallFchSpiSmm (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  // VOID                            *SmmReadyToBootRegistration;

  DEBUG ((DEBUG_INFO, "InstallFchSpiSmm() Start\n"));

  Status = gSmst->SmmAllocatePool (
                    EfiRuntimeServicesData,
                    sizeof (SPI_INSTANCE),
                    (VOID **)&mSpiInstance
                    );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ZeroMem (mSpiInstance, sizeof (SPI_INSTANCE));

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
  Status = gSmst->SmmInstallProtocolInterface (
                    &(mSpiInstance->Handle),
                    &gEfiSmmSpiProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &(mSpiInstance->SpiProtocol)
                    );
  if (EFI_ERROR (Status)) {
    FreePool (mSpiInstance);
    return EFI_DEVICE_ERROR;
  }

  Status = mSpiInstance->SpiProtocol.Init (&(mSpiInstance->SpiProtocol));
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "InstallFchSpiSmm() End\n"));

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
