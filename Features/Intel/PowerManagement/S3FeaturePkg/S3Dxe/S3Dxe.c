/** @file
  Source code file for S3 DXE module

Copyright (c) 2022, Baruch Binyamin Doron.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Guid/AcpiS3Context.h>
#include <Guid/MemoryTypeInformation.h>
#include <AcpiS3MemoryNvData.h>

#define PEI_ADDITIONAL_MEMORY_SIZE    (16 * EFI_PAGE_SIZE)

/**
  Get the mem size in memory type information table.

  @return the mem size in memory type information table.
**/
UINT64
EFIAPI
GetMemorySizeInMemoryTypeInformation (
  VOID
  )
{
  EFI_STATUS                  Status;
  EFI_MEMORY_TYPE_INFORMATION *MemoryData;
  UINT8                       Index;
  UINTN                       TempPageNum;

  Status = EfiGetSystemConfigurationTable (&gEfiMemoryTypeInformationGuid, (VOID **) &MemoryData);

  if (EFI_ERROR (Status) || MemoryData == NULL) {
    return 0;
  }

  TempPageNum = 0;
  for (Index = 0; MemoryData[Index].Type != EfiMaxMemoryType; Index++) {
    //
    // Accumulate default memory size requirements
    //
    TempPageNum += MemoryData[Index].NumberOfPages;
  }

  return TempPageNum * EFI_PAGE_SIZE;
}

/**
  Get the mem size need to be consumed and reserved for PEI phase resume.

  @return the mem size to be reserved for PEI phase resume.
**/
UINT64
EFIAPI
GetPeiMemSize (
  VOID
  )
{
  UINT64  Size;

  Size = GetMemorySizeInMemoryTypeInformation ();

  return PcdGet32 (PcdPeiMinMemSize) + Size + PEI_ADDITIONAL_MEMORY_SIZE;
}

/**
  Allocate EfiACPIMemoryNVS below 4G memory address.

  This function allocates EfiACPIMemoryNVS below 4G memory address.

  @param  Size         Size of memory to allocate.

  @return Allocated address for output.

**/
VOID *
EFIAPI
AllocateAcpiNvsMemoryBelow4G (
  IN   UINTN  Size
  )
{
  UINTN                 Pages;
  EFI_PHYSICAL_ADDRESS  Address;
  EFI_STATUS            Status;
  VOID                  *Buffer;

  Pages   = EFI_SIZE_TO_PAGES (Size);
  Address = 0xffffffff;

  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  Pages,
                  &Address
                  );
  ASSERT_EFI_ERROR (Status);

  Buffer = (VOID *)(UINTN)Address;
  ZeroMem (Buffer, Size);

  return Buffer;
}

/**
  Allocates memory to use on S3 resume.

  @param[in]  ImageHandle          Not used.
  @param[in]  SystemTable          General purpose services available to every DXE driver.

  @retval     EFI_SUCCESS          The function completes successfully
  @retval     EFI_OUT_OF_RESOURCES Insufficient resources to create database
**/
EFI_STATUS
EFIAPI
S3DxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINTN           S3PeiMemSize;
  UINTN           S3PeiMemBase;
  ACPI_S3_MEMORY  S3MemoryInfo;
  EFI_STATUS      Status;

  DEBUG ((DEBUG_INFO, "%a() Start\n", __func__));

  S3PeiMemSize = (UINTN) GetPeiMemSize ();
  S3PeiMemBase = (UINTN) AllocateAcpiNvsMemoryBelow4G (S3PeiMemSize);
  ASSERT (S3PeiMemBase != 0);

  S3MemoryInfo.S3PeiMemBase = (EFI_PHYSICAL_ADDRESS) S3PeiMemBase;
  S3MemoryInfo.S3PeiMemSize = (UINT64) S3PeiMemSize;

  DEBUG ((DEBUG_INFO, "S3PeiMemBase: 0x%x\n", S3PeiMemBase));
  DEBUG ((DEBUG_INFO, "S3PeiMemSize: 0x%x\n", S3PeiMemSize));

  Status = gRT->SetVariable (
                  ACPI_S3_MEMORY_NV_NAME,
                  &gEfiAcpiVariableGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  sizeof (S3MemoryInfo),
                  &S3MemoryInfo
                  );
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "%a() End\n", __func__));
  return EFI_SUCCESS;
}
