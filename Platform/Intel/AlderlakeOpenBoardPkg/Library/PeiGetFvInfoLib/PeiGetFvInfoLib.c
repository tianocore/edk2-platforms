/** @file
  Helper Library for PEI Graphics PEIM

   Copyright (c) 2022 - 2026, Intel Corporation. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/PeiGetFvInfoLib.h>
#include <Guid/FirmwareFileSystem2.h>
#include <Ppi/FirmwareVolume.h>
#include <Pi/PiPeiCis.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/HobLib.h>

/**
  PeiGetSectionFromFv finds the file in FV and gets file Address and Size

  @param[in] NameGuid              - File GUID
  @param[out] Address              - Pointer to the File Address
  @param[out] Size                 - Pointer to File Size

  @retval EFI_SUCCESS                Successfull in reading the section from FV
  @retval EFI_NOT_FOUND              File not found
**/
EFI_STATUS
EFIAPI
PeiGetSectionFromFv (
  IN CONST  EFI_GUID        NameGuid,
  OUT VOID                  **Address,
  OUT UINT32                *Size
  )
{
  EFI_STATUS                           Status;
  EFI_PEI_FIRMWARE_VOLUME_PPI          *FvPpi;
  EFI_FV_FILE_INFO                     FvFileInfo;
  EFI_PEI_FV_HANDLE                    FvHandle;
  EFI_PEI_FILE_HANDLE                  FileHandle;
  EFI_GUID                             *FileGuid;
  EFI_COMMON_SECTION_HEADER            *Section;
  EFI_HOB_GUID_TYPE                    *GuidHob;
  VOID                                 *HobData;

  Status = PeiServicesLocatePpi(
             &gEfiFirmwareFileSystem2Guid,
             0,
             NULL,
             (VOID **)&FvPpi
             );
  ASSERT_EFI_ERROR(Status);

  GuidHob = GetFirstGuidHob (&gPlatformInitFvLocationGuid);
  if (GuidHob != NULL) {
    HobData = GET_GUID_HOB_DATA(GuidHob);
    FvHandle = *(EFI_PEI_FV_HANDLE *) HobData;

    //
    // File typically resides in current FV or previous FV, so searching both of them.
    //
    Status = FvPpi->FindFileByName (FvPpi, &NameGuid, &FvHandle, &FileHandle);

    if (!EFI_ERROR(Status) && FileHandle != NULL) {

      DEBUG((DEBUG_INFO, "Find SectionByType \n"));

      Status = FvPpi->FindSectionByType(FvPpi, EFI_SECTION_RAW, FileHandle, (VOID **)&FileGuid);
      if (!EFI_ERROR(Status)) {

        DEBUG((DEBUG_INFO, "GetFileInfo \n"));

        Status = FvPpi->GetFileInfo(FvPpi, FileHandle, &FvFileInfo);
        Section = (EFI_COMMON_SECTION_HEADER *)FvFileInfo.Buffer;

        if (IS_SECTION2(Section)) {
          ASSERT(SECTION2_SIZE(Section) > 0x00FFFFFF);
          *Size = SECTION2_SIZE(Section) - sizeof (EFI_COMMON_SECTION_HEADER2);
          *Address = ((UINT8 *)Section + sizeof (EFI_COMMON_SECTION_HEADER2));
        } else {
          *Size = SECTION_SIZE(Section) - sizeof (EFI_COMMON_SECTION_HEADER);
          *Address = ((UINT8 *)Section + sizeof (EFI_COMMON_SECTION_HEADER));
        }
        return EFI_SUCCESS;
      }
    }
  } else {
    DEBUG ((DEBUG_INFO, "Hob not found\n"));
  }
  return EFI_NOT_FOUND;
}
