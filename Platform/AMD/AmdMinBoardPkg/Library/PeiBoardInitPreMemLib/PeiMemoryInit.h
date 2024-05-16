/** @file
  This file contains definitions required for memory initialization in PEI phase.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef PEI_MEMORY_INIT_PEI_H_
#define PEI_MEMORY_INIT_PEI_H_

#include <Uefi/UefiBaseType.h>
#include <Pi/PiPeiCis.h>
#include <Library/PeiServicesLib.h>
#include <Ppi/PlatformMemorySize.h>
#include <Guid/SmramMemoryReserve.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>

#define SYSTEM_MEMORY_ATTRIBUTES  (                \
  EFI_RESOURCE_ATTRIBUTE_PRESENT |                 \
  EFI_RESOURCE_ATTRIBUTE_INITIALIZED |             \
  EFI_RESOURCE_ATTRIBUTE_TESTED |                  \
  EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |             \
  EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |       \
  EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE | \
  EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE      \
  )

#define MEMORY_MAPPED_IO_ATTRIBUTES  ( \
  EFI_RESOURCE_ATTRIBUTE_PRESENT |     \
  EFI_RESOURCE_ATTRIBUTE_INITIALIZED | \
  EFI_RESOURCE_ATTRIBUTE_TESTED |      \
  EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE   \
  )

/**
  A Callback routine only AmdMemoryInfoHob is ready.

  @retval EFI_SUCCESS   Platform Pre Memory initialization is successfull.
          EFI_STATUS    Various failure from underlying routine calls.
**/
EFI_STATUS
EFIAPI
EndofAmdMemoryInfoHobPpiGuidCallBack (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

#endif // PEI_MEMORY_INIT_PEI_H_
