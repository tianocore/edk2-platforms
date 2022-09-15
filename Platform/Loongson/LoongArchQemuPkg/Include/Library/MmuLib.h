/** @file

  Copyright (c) 2022 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - EXC     - execute
**/
#ifndef MMU_LIB_H_
#define MMU_LIB_H_
/**
  write operation is performed Count times from the first element of Buffer.
  Convert EFI Attributes to Loongarch Attributes.
  @param[in]  EfiAttributes     Efi Attributes.

  @retval  LoongArch Attributes.
**/
UINTN
EfiAttributeToLoongArchAttribute (
  IN UINTN  EfiAttributes
  );

/**
  Finds the length and memory properties of the memory region corresponding to the specified base address.

  @param[in]  BaseAddress    To find the base address of the memory region.
  @param[in]  EndAddress     To find the end address of the memory region.
  @param[out]  RegionLength    The length of the memory region found.
  @param[out]  RegionAttributes    Properties of the memory region found.

  @retval  EFI_SUCCESS    The corresponding memory area was successfully found
           EFI_NOT_FOUND    No memory area found
**/
EFI_STATUS
GetLoongArchMemoryRegion (
  IN     UINTN  BaseAddress,
  IN     UINTN  EndAddress,
  OUT    UINTN  *RegionLength,
  OUT    UINTN  *RegionAttributes
  );

/**
  Sets the Attributes  of the specified memory region

  @param[in]  BaseAddress  The base address of the memory region to set the Attributes.
  @param[in]  Length       The length of the memory region to set the Attributes.
  @param[in]  Attributes   The Attributes to be set.

  @retval  EFI_SUCCESS    The Attributes was set successfully
**/
EFI_STATUS
LoongArchSetMemoryAttributes (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINTN                 Length,
  IN UINTN                 Attributes
  );

/**
  Sets the non-executable Attributes for the specified memory region

  @param[in]  BaseAddress  The base address of the memory region to set the Attributes.
  @param[in]  Length       The length of the memory region to set the Attributes.

  @retval  EFI_SUCCESS    The Attributes was set successfully
**/
EFI_STATUS
LoongArchSetMemoryRegionNoExec (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINTN                Length
  );

/**
  Create a page table and initialize the MMU.

  @param[] VOID

  @retval  VOID
**/
VOID
EFIAPI
ConfigureMmu (
  VOID
  );
#endif // MMU_LIB_H_
