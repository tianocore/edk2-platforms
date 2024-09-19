/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <AcpiHeader.h>
#include <Guid/RootComplexInfoHob.h>
#include <IndustryStandard/Acpi30.h>
#include <Library/AcpiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Platform/Ac01.h>
#include <Protocol/AcpiTable.h>

// Required to be 1 to match the kernel quirk for ECAM
#define EFI_ACPI_MCFG_OEM_REVISION  1

STATIC AC01_ROOT_COMPLEX  *mRootComplexList;

#pragma pack(1)

typedef struct {
  UINT64    BaseAddress;
  UINT16    SegGroupNum;
  UINT8     StartBusNum;
  UINT8     EndBusNum;
  UINT32    Reserved2;
} EFI_MCFG_CONFIG_STRUCTURE;

typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER    Header;
  UINT64                         Reserved1;
} EFI_MCFG_TABLE_CONFIG;

#pragma pack()

EFI_MCFG_TABLE_CONFIG  mMcfgHeader = {
  {
    EFI_ACPI_6_1_PCI_EXPRESS_MEMORY_MAPPED_CONFIGURATION_SPACE_BASE_ADDRESS_DESCRIPTION_TABLE_SIGNATURE,
    0,  // To be filled
    1,
    0x00,  // Checksum will be updated at runtime
    EFI_ACPI_OEM_ID,
    EFI_ACPI_OEM_TABLE_ID,
    EFI_ACPI_MCFG_OEM_REVISION,
    EFI_ACPI_CREATOR_ID,
    EFI_ACPI_CREATOR_REVISION
  },
  0x0000000000000000,  // Reserved
};

EFI_MCFG_CONFIG_STRUCTURE  mMcfgNodeTemplate = {
  .BaseAddress = 0,
  .SegGroupNum = 0,
  .StartBusNum = 0,
  .EndBusNum   = 255,
  .Reserved2   = 0,
};

STATIC
VOID
ConstructMcfg (
  VOID    *McfgBuffer,
  UINT32  McfgCount,
  INT32   *EnabledRCs
  )
{
  AC01_ROOT_COMPLEX  *RootComplex;
  UINT32             Idx;
  VOID               *Iter = McfgBuffer;

  mMcfgHeader.Header.Length = McfgCount;
  CopyMem (Iter, &mMcfgHeader, sizeof (EFI_MCFG_TABLE_CONFIG));

  Iter += sizeof (EFI_MCFG_TABLE_CONFIG);
  for (Idx = 0; EnabledRCs[Idx] != -1; Idx++) {
    RootComplex                   = &mRootComplexList[EnabledRCs[Idx]];
    mMcfgNodeTemplate.BaseAddress = RootComplex->MmcfgBase;
    mMcfgNodeTemplate.SegGroupNum = RootComplex->Logical;
    CopyMem (Iter, &mMcfgNodeTemplate, sizeof (EFI_MCFG_CONFIG_STRUCTURE));
    Iter += sizeof (EFI_MCFG_CONFIG_STRUCTURE);
  }
}

EFI_STATUS
EFIAPI
AcpiInstallMcfg (
  VOID
  )
{
  EFI_ACPI_TABLE_PROTOCOL  *AcpiTableProtocol;
  EFI_STATUS               Status;
  INT32                    EnabledRCs[AC01_PCIE_MAX_ROOT_COMPLEX];
  UINT32                   RcCount, McfgCount;
  UINT8                    Idx;
  UINTN                    TableKey;
  VOID                     *Hob;
  VOID                     *McfgBuffer;

  Hob = GetFirstGuidHob (&gRootComplexInfoHobGuid);
  if (Hob == NULL) {
    return EFI_NOT_FOUND;
  }

  mRootComplexList = (AC01_ROOT_COMPLEX *)GET_GUID_HOB_DATA (Hob);

  for (Idx = 0, RcCount = 0; Idx < AC01_PCIE_MAX_ROOT_COMPLEX; Idx++) {
    if (mRootComplexList[Idx].Active) {
      EnabledRCs[RcCount++] = Idx;
    }
  }

  EnabledRCs[RcCount] = -1;

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTableProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "MCFG: Unable to locate ACPI table entry\n"));
    return Status;
  }

  McfgCount  = sizeof (EFI_MCFG_TABLE_CONFIG) + sizeof (EFI_MCFG_CONFIG_STRUCTURE) * RcCount;
  McfgBuffer = AllocateZeroPool (McfgCount);
  if (McfgBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ConstructMcfg (McfgBuffer, McfgCount, EnabledRCs);

  Status = AcpiTableProtocol->InstallAcpiTable (
                                AcpiTableProtocol,
                                McfgBuffer,
                                McfgCount,
                                &TableKey
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "MCFG: Unable to install MCFG table entry\n"));
  }

  FreePool (McfgBuffer);
  return Status;
}
