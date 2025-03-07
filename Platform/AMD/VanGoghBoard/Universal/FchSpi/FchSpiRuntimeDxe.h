/** @file
Header file for the PCH SPI Runtime Driver.

Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FCH_SPI_RUNTIME_DXE_H_
#define FCH_SPI_RUNTIME_DXE_H_

#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PciExpressLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/TimerLib.h>
#include <Library/SpiFlashDeviceLib.h>

#include <Protocol/Spi.h>
#include <Protocol/SpiCommon.h>

#define EFI_INTERNAL_POINTER  0x00000004

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
  );

#endif
