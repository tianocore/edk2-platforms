/** @file
  Declarations required for SMBIOS DXE driver.

  Function declarations and data type declarations required for SMBIOS DXE
  driver of the Arm Morello System Development Platform.

  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef SMBIOS_PLATFORM_DXE_H_
#define SMBIOS_PLATFORM_DXE_H_

/**
  Install SMBIOS BIOS information Table.

  Install the SMBIOS BIOS information (type 0) table for
  Arm Morello System Development Platform.

  @param[in] Smbios   SMBIOS protocol.

  @retval EFI_SUCCESS           Record was added.
  @retval EFI_OUT_OF_RESOURCES  Record was not added.
  @retval EFI_ALREADY_STARTED   The SmbiosHandle passed is already in use.
**/
EFI_STATUS
EFIAPI
InstallType0BiosInformation (
  IN     EFI_SMBIOS_PROTOCOL  *Smbios
  );

/**
  Install SMBIOS System information Table.

  Install the SMBIOS system information (type 1) table for
  Arm Morello System Development Platform.

  @param[in]  Smbios   SMBIOS protocol.

  @retval EFI_SUCCESS           Record was added.
  @retval EFI_NOT_FOUND         Unknown product id.
  @retval EFI_OUT_OF_RESOURCES  Record was not added.
  @retval EFI_ALREADY_STARTED   The SmbiosHandle passed is already in use.
**/
EFI_STATUS
EFIAPI
InstallType1SystemInformation (
  IN     EFI_SMBIOS_PROTOCOL  *Smbios
  );

/**
  Install SMBIOS System Enclosure Table

  Install the SMBIOS System Enclosure (type 3) table for
  Arm Morello System Development Platform.

  @param[in]  Smbios   SMBIOS protocol.

  @retval EFI_SUCCESS           Record was added.
  @retval EFI_OUT_OF_RESOURCES  Record was not added.
  @retval EFI_ALREADY_STARTED   The SmbiosHandle passed is already in use.
**/
EFI_STATUS
EFIAPI
InstallType3SystemEnclosure (
  IN     EFI_SMBIOS_PROTOCOL  *Smbios
  );

/**
  Install SMBIOS Processor information Table

  Install the SMBIOS Processor information (type 4) table for
  Arm Morello System Development Platform.

  @param[in]  Smbios   SMBIOS protocol.

  @retval EFI_SUCCESS           Record was added.
  @retval EFI_NOT_FOUND         Unknown product id.
  @retval EFI_OUT_OF_RESOURCES  Record was not added.
  @retval EFI_ALREADY_STARTED   The SmbiosHandle passed is already in use.
**/
EFI_STATUS
EFIAPI
InstallType4ProcessorInformation (
  IN     EFI_SMBIOS_PROTOCOL  *Smbios
  );

/**
  Install SMBIOS Cache information Table

  Install the SMBIOS Cache information (type 7) table for
  Arm Morello System Development Platform.

  @param[in] Smbios   SMBIOS protocol.

  @retval EFI_SUCCESS           Record was added.
  @retval EFI_NOT_FOUND         Unknown product id.
  @retval EFI_OUT_OF_RESOURCES  Record was not added.
  @retval EFI_ALREADY_STARTED   The SmbiosHandle passed is already in use.
**/
EFI_STATUS
EFIAPI
InstallType7CacheInformation (
  IN     EFI_SMBIOS_PROTOCOL  *Smbios
  );

/**
  Install SMBIOS physical memory array table.

  Install the SMBIOS physical memory array (type 16) table for
  Arm Morello System Development Platform.

  @param[in] Smbios   SMBIOS protocol.

  @retval EFI_SUCCESS           Record was added.
  @retval EFI_OUT_OF_RESOURCES  Record was not added.
  @retval EFI_ALREADY_STARTED   The SmbiosHandle passed is already in use.
**/
EFI_STATUS
EFIAPI
InstallType16PhysicalMemoryArray (
  IN     EFI_SMBIOS_PROTOCOL  *Smbios
  );

/**
  Install SMBIOS memory device table.

  Install the SMBIOS memory device (type 17) table for
  Arm Morello System Development platform.

  @param[in] Smbios   SMBIOS protocol.

  @retval EFI_SUCCESS           Record was added.
  @retval EFI_OUT_OF_RESOURCES  Record was not added.
  @retval EFI_ALREADY_STARTED   The SmbiosHandle passed is already in use.
**/
EFI_STATUS
EFIAPI
InstallType17MemoryDevice (
  IN     EFI_SMBIOS_PROTOCOL  *Smbios
  );

/**
  Install SMBIOS memory array mapped address table

  Install the SMBIOS memory array mapped address (type 19) table for
  Arm Morello System Development platform.

  @param[in] Smbios   SMBIOS protocol.

  @retval EFI_SUCCESS           Record was added.
  @retval EFI_OUT_OF_RESOURCES  Record was not added.
  @retval EFI_ALREADY_STARTED   The SmbiosHandle passed is already in use.
**/
EFI_STATUS
EFIAPI
InstallType19MemoryArrayMappedAddress (
  IN     EFI_SMBIOS_PROTOCOL  *Smbios
  );

/**
  Install SMBIOS system boot information

  Install the SMBIOS system boot information (type 32) table for
  Arm Morello System Development platform.

  @param[in] Smbios   SMBIOS protocol.

  @retval EFI_SUCCESS           Record was added.
  @retval EFI_OUT_OF_RESOURCES  Record was not added.
  @retval EFI_ALREADY_STARTED   The SmbiosHandle passed is already in use.
**/
EFI_STATUS
EFIAPI
InstallType32SystemBootInformation (
  IN     EFI_SMBIOS_PROTOCOL  *Smbios
  );

/**
  Determine the Dram Block2 Size

  Determine the Dram Block2 Size by using the data in the Platform
  ID Descriptor HOB to lookup for a matching Dram Block2 Size.

  @retval EFI_SUCCESS    Identified Dram Block2 size on the platform.
  @retval EFI_NOT_FOUND  Failed to identify Dram Block2 Size.
**/
EFI_STATUS
MorelloGetDramBlock2Size (
  UINT64  *DramBlock2Size
  );

typedef enum {
  SMBIOS_HANDLE_ENCLOSURE = 0x1000,
  SMBIOS_HANDLE_CLUSTER1,
  SMBIOS_HANDLE_L1I_CACHE,
  SMBIOS_HANDLE_L1D_CACHE,
  SMBIOS_HANDLE_L2_CACHE,
  SMBIOS_HANDLE_L3_CACHE,
  SMBIOS_HANDLE_L4_CACHE,
  SMBIOS_HANDLE_PHYSICAL_MEMORY,
  SMBIOS_HANDLE_MEMORY_DEVICE0000,  // Chip 0 Bank 0
  SMBIOS_HANDLE_MEMORY_DEVICE0001,  // Chip 0 Bank 1
} SMBIOS_REFERENCE_HANDLES;

#endif // SMBIOS_PLATFORM_DXE_H_
