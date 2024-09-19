/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AcpiNfit.h"
#include "AcpiPlatform.h"

EFI_ACPI_6_3_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE  NfitSPATemplate = {
  EFI_ACPI_6_3_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE_TYPE,
  sizeof (EFI_ACPI_6_3_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE),
  0,                                                                // The uniue index - need to be filled.
  0,                                                                // The flags - need to be filled.
  0,                                                                // Reserved.
  0,                                                                // Proximity domain - need to be filled.
  EFI_ACPI_6_3_NFIT_GUID_BYTE_ADDRESSABLE_PERSISTENT_MEMORY_REGION, // PM range type.
  0,                                                                // Start address - need to be filled.
  0,                                                                // Size - need to be filled.
  EFI_MEMORY_UC | EFI_MEMORY_WC | EFI_MEMORY_WT | EFI_MEMORY_WB |
  EFI_MEMORY_WP | EFI_MEMORY_UCE, // attribute - need to be filled.
};

EFI_ACPI_6_3_NFIT_NVDIMM_CONTROL_REGION_STRUCTURE  NvdimmControlRegionTemplate = {
  EFI_ACPI_6_3_NFIT_NVDIMM_CONTROL_REGION_STRUCTURE_TYPE,
  sizeof (EFI_ACPI_6_3_NFIT_NVDIMM_CONTROL_REGION_STRUCTURE),
  0,     // The unique index - need to be filled.
  0,     // The vendor id - need to be filled.
  0,     // The device id - need to be filled.
  0,     // The revision - need to be filled.
  0,     // The subsystem nvdimm id - need to be filled.
  0,     // The subsystem nvdimm device id - need to be filled.
  0,     // The subsystem revision - need to be filled.
  0,     // The valid field.
  0,     // The manufacturing location - not valid.
  0,     // The manufacturing date - not valid.
  { 0 }, // Reserved.
  0,     // The serial number - need to be filled.
  0,     // The region format interface code - dummy value.
  0,     // The number of block control windows.
  0,     // The size of block control windows.
  0,     // The Command Register Offset in Block Control Window.
  0,     // The Size of Command Register in Block Control Windows.
  0,     // The Status Register Offset in Block Control Window.
  0,     // Size of Status Register in Block Control Windows.
  0,     // The NVDIMM Control Region Flag.
  { 0 }, // Reserved.
};

EFI_ACPI_6_3_NFIT_NVDIMM_REGION_MAPPING_STRUCTURE  NvdimmRegionMappingTemplate = {
  EFI_ACPI_6_3_NFIT_NVDIMM_REGION_MAPPING_STRUCTURE_TYPE,
  sizeof (EFI_ACPI_6_3_NFIT_NVDIMM_REGION_MAPPING_STRUCTURE),
  { 0 }, // _ADR of the NVDIMM device - need to be filled.
  0,     // Dimm smbios handle index - need to be filled.
  0,     // The unique region index - need to be filled.
  0,     // The SPA range index - need to be filled.
  0,     // The control region index - need to be filled.
  0,     // The region size - need to be filled.
  0,     // The region offset - need to be filled.
  0,     // The region base - need to be filled.
  0,     // The interleave structure index - need to be filled.
  0,     // The interleave ways - need to be filled.
  0,     // NVDIMM flags - need to be filled.
  0,     // Reserved.
};

EFI_ACPI_6_3_NVDIMM_FIRMWARE_INTERFACE_TABLE  NFITTableHeaderTemplate = {
  __ACPI_HEADER (
    EFI_ACPI_6_3_NVDIMM_FIRMWARE_INTERFACE_TABLE_STRUCTURE_SIGNATURE,
    0, /* need fill in */
    EFI_ACPI_6_3_NVDIMM_FIRMWARE_INTERFACE_TABLE_REVISION
    ),
  0x00000000, // Reserved
};

NVDIMM_DATA  NvdData[PLATFORM_CPU_MAX_SOCKET] = {
  { 0 }
};

EFI_STATUS
AcpiNvdInfoInit (
  IN OUT NVDIMM_INFO  *NvdInfoPtr,
  IN     UINTN        NvdId
  )
{
  PLATFORM_INFO_HOB  *PlatformHob;
  VOID               *Hob;

  /* Get the Platform HOB */
  Hob = GetFirstGuidHob (&gPlatformInfoHobGuid);
  if ((Hob == NULL) || (NvdInfoPtr == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  PlatformHob = (PLATFORM_INFO_HOB *)GET_GUID_HOB_DATA (Hob);

  NvdInfoPtr->Enabled  = TRUE;
  NvdInfoPtr->PhysId   = NvdId;
  NvdInfoPtr->NvdSize  = PlatformHob->DimmList.Dimm[NvdId].Info.DimmSize * ONE_GB;
  NvdInfoPtr->VendorId =
    *((UINT16 *)&PlatformHob->DimmList.Dimm[NvdId].SpdData.Data[320]);
  NvdInfoPtr->DeviceId =
    *((UINT16 *)&PlatformHob->DimmList.Dimm[NvdId].SpdData.Data[192]);
  NvdInfoPtr->RevisionId =
    (UINT16)PlatformHob->DimmList.Dimm[NvdId].SpdData.Data[349];
  NvdInfoPtr->SubVendorId =
    *((UINT16 *)&PlatformHob->DimmList.Dimm[NvdId].SpdData.Data[194]);
  NvdInfoPtr->SubDeviceId =
    *((UINT16 *)&PlatformHob->DimmList.Dimm[NvdId].SpdData.Data[196]);
  NvdInfoPtr->SubRevisionId =
    (UINT16)PlatformHob->DimmList.Dimm[NvdId].SpdData.Data[198];
  NvdInfoPtr->SerialNumber =
    *((UINT32 *)&PlatformHob->DimmList.Dimm[NvdId].SpdData.Data[325]);

  return EFI_SUCCESS;
}

EFI_STATUS
AcpiNvdDataInit (
  IN UINTN  Socket
  )
{
  PLATFORM_INFO_HOB  *PlatformHob;
  NVDIMM_INFO        *NvdInfo;
  UINTN              Count;
  VOID               *Hob;
  UINTN              NvdRegionNum, RegionId;

  /* Get the Platform HOB */
  Hob = GetFirstGuidHob (&gPlatformInfoHobGuid);
  if (Hob == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PlatformHob = (PLATFORM_INFO_HOB *)GET_GUID_HOB_DATA (Hob);

  NvdRegionNum = 0;
  for (Count = 0; Count < PlatformHob->DramInfo.NumRegion; Count++) {
    if (  (PlatformHob->DramInfo.NvdRegion[Count] != 0)
       && (PlatformHob->DramInfo.Socket[Count] == Socket))
    {
      NvdData[Socket].NvdRegionId[NvdRegionNum] = Count;
      NvdRegionNum++;
    }
  }

  if (NvdRegionNum == 0) {
    return EFI_SUCCESS;
  }

  NvdData[Socket].NvdRegionNum = NvdRegionNum;
  NvdData[Socket].NvdMode      = PlatformHob->DramInfo.NvdimmMode[Socket];
  if (NvdData[Socket].NvdMode == NvdimmHashed) {
    NvdInfo               = &NvdData[Socket].NvdInfo[NVDIMM_SK0];
    NvdInfo->DeviceHandle =
      (Socket == 0) ? AC01_NVDIMM_NVD1_DEVICE_HANDLE :
      AC01_NVDIMM_NVD3_DEVICE_HANDLE;
    NvdInfo->InterleaveWays = AC01_NVDIMM_HASHED_INTERLEAVE_WAYS;
    NvdInfo->RegionOffset   = 0;
    AcpiNvdInfoInit (
      NvdInfo,
      (Socket == 0) ? AC01_NVDIMM_NVD1_DIMM_ID :
      AC01_NVDIMM_NVD3_DIMM_ID
      );

    NvdInfo               = &NvdData[Socket].NvdInfo[1];
    NvdInfo->DeviceHandle =
      (Socket == 0) ? AC01_NVDIMM_NVD2_DEVICE_HANDLE :
      AC01_NVDIMM_NVD4_DEVICE_HANDLE;
    NvdInfo->InterleaveWays = AC01_NVDIMM_HASHED_INTERLEAVE_WAYS;
    NvdInfo->RegionOffset   = AC01_NVDIMM_HASHED_REGION_OFFSET;
    AcpiNvdInfoInit (
      NvdInfo,
      (Socket == 0) ? AC01_NVDIMM_NVD2_DIMM_ID :
      AC01_NVDIMM_NVD4_DIMM_ID
      );

    /* Update NvdNum */
    NvdData[Socket].NvdNum = 0;
    for (Count = 0; Count < NVDIMM_NUM_PER_SK; Count++) {
      if (NvdData[Socket].NvdInfo[Count].Enabled) {
        NvdData[Socket].NvdNum++;
      }
    }

    return EFI_SUCCESS;
  }

  /* NvdimmNonHashed */
  NvdData[Socket].NvdNum = 0;
  for (Count = 0; Count < NvdData[Socket].NvdRegionNum; Count++) {
    RegionId = NvdData[Socket].NvdRegionId[Count];
    if ((PlatformHob->DramInfo.Base[RegionId] ==
         AC01_NVDIMM_SK0_NHASHED_REGION0_BASE) ||
        (PlatformHob->DramInfo.Base[RegionId] ==
         AC01_NVDIMM_SK1_NHASHED_REGION0_BASE))
    {
      NvdInfo               = &NvdData[Socket].NvdInfo[0];
      NvdInfo->DeviceHandle =
        (Socket == 0) ? AC01_NVDIMM_NVD1_DEVICE_HANDLE :
        AC01_NVDIMM_NVD3_DEVICE_HANDLE;
      NvdInfo->InterleaveWays = AC01_NVDIMM_NHASHED_INTERLEAVE_WAYS;
      NvdInfo->RegionOffset   = 0;
      AcpiNvdInfoInit (
        NvdInfo,
        (Socket == 0) ? AC01_NVDIMM_NVD1_DIMM_ID :
        AC01_NVDIMM_NVD3_DIMM_ID
        );
    } else if ((PlatformHob->DramInfo.Base[RegionId] ==
                AC01_NVDIMM_SK0_NHASHED_REGION1_BASE) ||
               (PlatformHob->DramInfo.Base[RegionId] ==
                AC01_NVDIMM_SK1_NHASHED_REGION1_BASE))
    {
      NvdInfo               = &NvdData[Socket].NvdInfo[1];
      NvdInfo->DeviceHandle =
        (Socket == 0) ? AC01_NVDIMM_NVD2_DEVICE_HANDLE :
        AC01_NVDIMM_NVD4_DEVICE_HANDLE;
      NvdInfo->InterleaveWays = AC01_NVDIMM_NHASHED_INTERLEAVE_WAYS;
      NvdInfo->RegionOffset   = 0;
      AcpiNvdInfoInit (
        NvdInfo,
        (Socket == 0) ? AC01_NVDIMM_NVD2_DIMM_ID :
        AC01_NVDIMM_NVD4_DIMM_ID
        );
    }
  }

  /* Update NvdNum */
  NvdData[Socket].NvdNum = 0;
  for (Count = 0; Count < NVDIMM_NUM_PER_SK; Count++) {
    if (NvdData[Socket].NvdInfo[Count].Enabled) {
      NvdData[Socket].NvdNum++;
    }
  }

  return EFI_SUCCESS;
}

/*
 * Fill in SPA structure
 */
VOID
AcpiNfitFillSPA (
  IN OUT EFI_ACPI_6_3_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE  *NfitSpaPointer,
  IN     UINTN                                                      NvdRegionIndex,
  IN     UINT64                                                     NvdRegionBase,
  IN     UINT64                                                     NvdRegionSize
  )
{
  ASSERT (NfitSpaPointer != NULL);

  NfitSpaPointer->Flags                            = 0;
  NfitSpaPointer->SPARangeStructureIndex           = NvdRegionIndex;
  NfitSpaPointer->SystemPhysicalAddressRangeBase   = NvdRegionBase;
  NfitSpaPointer->SystemPhysicalAddressRangeLength = NvdRegionSize;
}

VOID
NfitFillControlRegion (
  IN OUT EFI_ACPI_6_3_NFIT_NVDIMM_CONTROL_REGION_STRUCTURE  *NfitControlRegionPointer,
  IN     NVDIMM_INFO                                        *NvdInfo,
  IN     UINTN                                              NvdControlRegionIndex
  )
{
  ASSERT (
    NfitControlRegionPointer != NULL
         && NvdInfo != NULL
    );

  NfitControlRegionPointer->NVDIMMControlRegionStructureIndex =
    NvdControlRegionIndex;
  NfitControlRegionPointer->VendorID            = NvdInfo->VendorId;
  NfitControlRegionPointer->DeviceID            = NvdInfo->DeviceId;
  NfitControlRegionPointer->RevisionID          = NvdInfo->RevisionId;
  NfitControlRegionPointer->SubsystemVendorID   = NvdInfo->SubVendorId;
  NfitControlRegionPointer->SubsystemDeviceID   = NvdInfo->SubDeviceId;
  NfitControlRegionPointer->SubsystemRevisionID = NvdInfo->SubRevisionId;
  NfitControlRegionPointer->SerialNumber        = NvdInfo->SerialNumber;
}

VOID
NfitFillRegionMapping (
  IN OUT EFI_ACPI_6_3_NFIT_NVDIMM_REGION_MAPPING_STRUCTURE          *NfitRegionMappingPointer,
  IN     EFI_ACPI_6_3_NFIT_NVDIMM_CONTROL_REGION_STRUCTURE          *NfitControlRegionPointer,
  IN     EFI_ACPI_6_3_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE  *NfitSpaPointer,
  IN     NVDIMM_INFO                                                *NvdInfo,
  IN     UINTN                                                      NvdRegionID
  )
{
  ASSERT (
    NfitRegionMappingPointer != NULL
         && NfitRegionMappingPointer != NULL
         && NfitRegionMappingPointer != NULL
         && NfitRegionMappingPointer != NULL
         && NvdInfo != NULL
    );

  NfitRegionMappingPointer->NVDIMMRegionID              = NvdRegionID;
  NfitRegionMappingPointer->NVDIMMPhysicalID            = NvdInfo->PhysId;
  NfitRegionMappingPointer->InterleaveWays              = NvdInfo->InterleaveWays;
  NfitRegionMappingPointer->RegionOffset                = NvdInfo->RegionOffset;
  NfitRegionMappingPointer->NVDIMMRegionSize            = NvdInfo->NvdSize;
  NfitRegionMappingPointer->NFITDeviceHandle.DIMMNumber =
    NvdInfo->DeviceHandle & 0x0F;
  NfitRegionMappingPointer->NFITDeviceHandle.MemoryChannelNumber =
    (NvdInfo->DeviceHandle >> 4) & 0x0F;
  NfitRegionMappingPointer->NFITDeviceHandle.MemoryControllerID =
    (NvdInfo->DeviceHandle >> 8) & 0x0F;
  NfitRegionMappingPointer->NFITDeviceHandle.SocketID =
    (NvdInfo->DeviceHandle >> 12) & 0x0F;
  NfitRegionMappingPointer->SPARangeStructureIndex =
    NfitSpaPointer->SPARangeStructureIndex;
  NfitRegionMappingPointer->NVDIMMPhysicalAddressRegionBase =
    NfitSpaPointer->SystemPhysicalAddressRangeBase;
  NfitRegionMappingPointer->NVDIMMControlRegionStructureIndex =
    NfitControlRegionPointer->NVDIMMControlRegionStructureIndex;
}

EFI_STATUS
AcpiNfitFillTableBySK (
  IN     EFI_ACPI_6_3_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE  *NfitSpaPointerStart,
  IN OUT EFI_ACPI_6_3_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE  **NfitSpaPointerNext,
  IN     UINTN                                                      Socket
  )
{
  EFI_ACPI_6_3_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE  *NfitSpaPointer;
  EFI_ACPI_6_3_NFIT_NVDIMM_CONTROL_REGION_STRUCTURE          *NfitControlRegionPointer;
  EFI_ACPI_6_3_NFIT_NVDIMM_REGION_MAPPING_STRUCTURE          *NfitRegionMappingPointer;
  PLATFORM_INFO_HOB                                          *PlatformHob;
  VOID                                                       *Hob;
  UINT64                                                     NvdRegionBase,
                                                             NvdRegionSize;
  UINTN  NvdCount, MaxNvdCount, RegionCount;
  UINTN  RegionId, NvdRegionIndex, NvdIndex;

  /* Get the Platform HOB */
  Hob = GetFirstGuidHob (&gPlatformInfoHobGuid);
  if (  (Hob == NULL)
     || (NfitSpaPointerStart == NULL)
     || (NfitSpaPointerNext == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  PlatformHob    = (PLATFORM_INFO_HOB *)GET_GUID_HOB_DATA (Hob);
  NvdRegionIndex = (Socket == 0) ? 0 : NvdData[NVDIMM_SK0].NvdRegionNum;
  NvdIndex       = (Socket == 0) ? 0 : NvdData[NVDIMM_SK0].NvdNum;
  if (NvdData[Socket].NvdMode == NvdimmHashed) {
    /* Table Type 0: SPA Range Structure */
    NfitSpaPointer = NfitSpaPointerStart;
    CopyMem (
      (VOID *)NfitSpaPointer,
      (VOID *)&NfitSPATemplate,
      sizeof (NfitSPATemplate)
      );
    RegionId      = NvdData[Socket].NvdRegionId[0];
    NvdRegionBase = PlatformHob->DramInfo.Base[RegionId];
    NvdRegionSize = PlatformHob->DramInfo.Size[RegionId];
    NvdRegionIndex++;
    AcpiNfitFillSPA (
      NfitSpaPointer,
      NvdRegionIndex,
      NvdRegionBase,
      NvdRegionSize
      );

    NfitControlRegionPointer =
      (EFI_ACPI_6_3_NFIT_NVDIMM_CONTROL_REGION_STRUCTURE *)
      (NfitSpaPointer + 1);
    for (NvdCount = 0; NvdCount < NVDIMM_NUM_PER_SK; NvdCount++) {
      if (!NvdData[Socket].NvdInfo[NvdCount].Enabled) {
        continue;
      }

      NvdIndex++;
      /* Table Type 4: NVDIMM Control Region Structure Mark */
      CopyMem (
        (VOID *)NfitControlRegionPointer,
        (VOID *)&NvdimmControlRegionTemplate,
        sizeof (NvdimmControlRegionTemplate)
        );
      NfitFillControlRegion (
        NfitControlRegionPointer,
        &NvdData[Socket].NvdInfo[NvdCount],
        NvdIndex
        );

      NfitRegionMappingPointer =
        (EFI_ACPI_6_3_NFIT_NVDIMM_REGION_MAPPING_STRUCTURE *)
        (NfitControlRegionPointer + 1);

      /* Table Type 1: NVDIMM Region Mapping Structure */
      CopyMem (
        (VOID *)NfitRegionMappingPointer,
        (VOID *)&NvdimmRegionMappingTemplate,
        sizeof (NvdimmRegionMappingTemplate)
        );
      NfitFillRegionMapping (
        NfitRegionMappingPointer,
        NfitControlRegionPointer,
        NfitSpaPointer,
        &NvdData[Socket].NvdInfo[NvdCount],
        NvdIndex - 1
        );

      NfitControlRegionPointer =
        (EFI_ACPI_6_3_NFIT_NVDIMM_CONTROL_REGION_STRUCTURE *)
        (NfitRegionMappingPointer + 1);
    }

    NfitSpaPointer =
      (EFI_ACPI_6_3_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE *)
      NfitControlRegionPointer;
  } else {
    /* NvdimmNonHashed */
    NfitSpaPointer = NfitSpaPointerStart;
    for (RegionCount = 0; RegionCount < NvdData[Socket].NvdRegionNum;
         RegionCount++)
    {
      /* Table Type 0: SPA Range Structure */
      CopyMem (
        (VOID *)NfitSpaPointer,
        (VOID *)&NfitSPATemplate,
        sizeof (NfitSPATemplate)
        );
      RegionId      = NvdData[Socket].NvdRegionId[RegionCount];
      NvdRegionBase = PlatformHob->DramInfo.Base[RegionId];
      NvdRegionSize = PlatformHob->DramInfo.Size[RegionId];
      NvdRegionIndex++;
      AcpiNfitFillSPA (
        NfitSpaPointer,
        NvdRegionIndex,
        NvdRegionBase,
        NvdRegionSize
        );

      NfitControlRegionPointer =
        (EFI_ACPI_6_3_NFIT_NVDIMM_CONTROL_REGION_STRUCTURE *)
        (NfitSpaPointer + 1);
      NvdCount = ((NvdRegionBase == AC01_NVDIMM_SK0_NHASHED_REGION0_BASE) ||
                  (NvdRegionBase == AC01_NVDIMM_SK1_NHASHED_REGION0_BASE)) ?
                 0 : AC01_NVDIMM_MAX_DIMM_PER_MCU;
      MaxNvdCount = NvdCount + AC01_NVDIMM_MAX_DIMM_PER_MCU;
      for ( ; NvdCount < MaxNvdCount; NvdCount++) {
        if (!NvdData[Socket].NvdInfo[NvdCount].Enabled) {
          continue;
        }

        NvdIndex++;

        /* Table Type 4: NVDIMM Control Region Structure Mark */
        CopyMem (
          (VOID *)NfitControlRegionPointer,
          (VOID *)&NvdimmControlRegionTemplate,
          sizeof (NvdimmControlRegionTemplate)
          );
        NfitFillControlRegion (
          NfitControlRegionPointer,
          &NvdData[Socket].NvdInfo[NvdCount],
          NvdIndex
          );

        NfitRegionMappingPointer =
          (EFI_ACPI_6_3_NFIT_NVDIMM_REGION_MAPPING_STRUCTURE *)
          (NfitControlRegionPointer + 1);

        /* Table Type 1: NVDIMM Region Mapping Structure */
        CopyMem (
          (VOID *)NfitRegionMappingPointer,
          (VOID *)&NvdimmRegionMappingTemplate,
          sizeof (NvdimmRegionMappingTemplate)
          );
        NfitFillRegionMapping (
          NfitRegionMappingPointer,
          NfitControlRegionPointer,
          NfitSpaPointer,
          &NvdData[Socket].NvdInfo[NvdCount],
          NvdIndex - 1
          );

        NfitControlRegionPointer =
          (EFI_ACPI_6_3_NFIT_NVDIMM_CONTROL_REGION_STRUCTURE *)
          (NfitRegionMappingPointer + 1);
      }

      NfitSpaPointer =
        (EFI_ACPI_6_3_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE *)
        NfitControlRegionPointer;
    }
  }

  /* Update NfitSpaPointerNext */
  *NfitSpaPointerNext = NfitSpaPointer;

  return EFI_SUCCESS;
}

EFI_STATUS
AcpiNfitFillTable (
  IN EFI_ACPI_6_3_NVDIMM_FIRMWARE_INTERFACE_TABLE  *NfitTablePointer
  )
{
  EFI_ACPI_6_3_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE  *NfitSpaPointerNext;

  if (NfitTablePointer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  NfitSpaPointerNext = (EFI_ACPI_6_3_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE *)
                       (NfitTablePointer + 1);

  if (NvdData[NVDIMM_SK0].NvdRegionNum != 0) {
    AcpiNfitFillTableBySK (NfitSpaPointerNext, &NfitSpaPointerNext, NVDIMM_SK0);
  }

  if (NvdData[NVDIMM_SK1].NvdRegionNum != 0) {
    AcpiNfitFillTableBySK (NfitSpaPointerNext, &NfitSpaPointerNext, NVDIMM_SK1);
  }

  return EFI_SUCCESS;
}

/*
 * Install NFIT table.
 */
EFI_STATUS
AcpiInstallNfitTable (
  VOID
  )
{
  EFI_ACPI_6_3_NVDIMM_FIRMWARE_INTERFACE_TABLE  *NfitTablePointer;
  EFI_ACPI_TABLE_PROTOCOL                       *AcpiTableProtocol;
  UINTN                                         NfitTableKey = 0;
  EFI_STATUS                                    Status;
  UINTN                                         Size;
  UINTN                                         NvdRegionNum;

  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTableProtocol
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AcpiNvdDataInit (NVDIMM_SK0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AcpiNvdDataInit (NVDIMM_SK1);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  NvdRegionNum = NvdData[NVDIMM_SK0].NvdRegionNum +
                 NvdData[NVDIMM_SK1].NvdRegionNum;
  if (NvdRegionNum == 0) {
    return EFI_INVALID_PARAMETER; /* No NVDIMM Region */
  }

  Size = sizeof (EFI_ACPI_6_3_NVDIMM_FIRMWARE_INTERFACE_TABLE);
  if (NvdData[NVDIMM_SK0].NvdRegionNum != 0) {
    Size +=
      (sizeof (EFI_ACPI_6_3_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE) *
       NvdData[NVDIMM_SK0].NvdRegionNum) +
      (sizeof (EFI_ACPI_6_3_NFIT_NVDIMM_CONTROL_REGION_STRUCTURE) *
       NvdData[NVDIMM_SK0].NvdNum) +
      (sizeof (EFI_ACPI_6_3_NFIT_NVDIMM_REGION_MAPPING_STRUCTURE) *
       NvdData[NVDIMM_SK0].NvdNum);
  }

  if (NvdData[NVDIMM_SK1].NvdRegionNum != 0) {
    Size +=
      (sizeof (EFI_ACPI_6_3_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE) *
       NvdData[NVDIMM_SK1].NvdRegionNum) +
      (sizeof (EFI_ACPI_6_3_NFIT_NVDIMM_CONTROL_REGION_STRUCTURE) *
       NvdData[NVDIMM_SK1].NvdNum) +
      (sizeof (EFI_ACPI_6_3_NFIT_NVDIMM_REGION_MAPPING_STRUCTURE) *
       NvdData[NVDIMM_SK1].NvdNum);
  }

  NfitTablePointer =
    (EFI_ACPI_6_3_NVDIMM_FIRMWARE_INTERFACE_TABLE *)AllocateZeroPool (Size);
  if (NfitTablePointer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (
    (VOID *)NfitTablePointer,
    (VOID *)&NFITTableHeaderTemplate,
    sizeof (NFITTableHeaderTemplate)
    );

  NfitTablePointer->Header.Length = Size;

  Status = AcpiNfitFillTable (NfitTablePointer);
  if (EFI_ERROR (Status)) {
    FreePool ((VOID *)NfitTablePointer);
    return Status;
  }

  AcpiUpdateChecksum ((UINT8 *)NfitTablePointer, NfitTablePointer->Header.Length);
  Status = AcpiTableProtocol->InstallAcpiTable (
                                AcpiTableProtocol,
                                (VOID *)NfitTablePointer,
                                NfitTablePointer->Header.Length,
                                &NfitTableKey
                                );
  if (EFI_ERROR (Status)) {
    FreePool ((VOID *)NfitTablePointer);
  }

  return Status;
}
