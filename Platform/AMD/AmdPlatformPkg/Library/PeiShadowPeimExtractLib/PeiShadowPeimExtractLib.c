/** @file
  To support PEI shadow extraction library.

  Copyright (C) 2024 - 2025, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Pi/PiFirmwareFile.h>
#include <Library/HobLib.h>
#include <Library/ExtractGuidedSectionLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeCoffLib.h>
#include "PeiShadowPeimExtractLib.h"

/**
  Support routine for the PE/COFF Loader that reads a buffer from a PE/COFF file.
  The function is used for XIP code to have optimized memory copy.

  @param[in]  FileHandle        The handle to the PE/COFF file.
  @param[in]  FileOffset        The offset, in bytes, into the file to read.
  @param[in]  ReadSize          The number of bytes to read from the file starting at FileOffset.
  @param[out] Buffer            A pointer to the buffer to read the data into.

  @retval     EFI_SUCCESS       ReadSize bytes of data were read into Buffer from the PE/COFF file starting at FileOffset.

**/
EFI_STATUS
EFIAPI
PeiImageRead (
  IN     VOID   *FileHandle,
  IN     UINTN  FileOffset,
  IN     UINTN  *ReadSize,
  OUT    VOID   *Buffer
  )
{
  CHAR8  *Destination8;
  CHAR8  *Source8;

  Destination8 = Buffer;
  Source8      = (CHAR8 *)((UINTN)FileHandle + FileOffset);
  if (Destination8 != Source8) {
    CopyMem (Destination8, Source8, *ReadSize);
  }

  return EFI_SUCCESS;
}

/**
  GetInfo gets raw data size and attribute of the input guided section.
  It first checks whether the input guid section is supported.
  If not, EFI_INVALID_PARAMETER will return.

  @param[in]  InputSection       Buffer containing the input GUIDed section to be processed.
  @param[out] OutputBufferSize   The size of OutputBuffer.
  @param[out] ScratchBufferSize  The size of ScratchBuffer.
  @param[out] SectionAttribute   The attribute of the input guided section.

  @retval EFI_SUCCESS            The size of destination buffer, the size of scratch buffer and
                                 the attribute of the input section are successfully retrieved.
  @retval EFI_INVALID_PARAMETER  The GUID in InputSection does not match this instance guid.

**/
EFI_STATUS
EFIAPI
ShadowPeimGuidedSectionGetInfo (
  IN  CONST VOID  *InputSection,
  OUT UINT32      *OutputBufferSize,
  OUT UINT32      *ScratchBufferSize,
  OUT UINT16      *SectionAttribute
  )
{
  EFI_STATUS                    Status;
  EFI_BOOT_MODE                 BootMode;
  VOID                          *Pe32Data;
  UINT32                        Pe32SectionSize;
  EFI_FILE_SECTION_POINTER      Pe32Section;
  PE_COFF_LOADER_IMAGE_CONTEXT  ImageContext;
  UINT32                        AlignImageSize;

  if (IS_SECTION2 (InputSection)) {
    //
    // Check whether the input guid section is recognized.
    //
    if (!CompareGuid (
           &gAmdShadowPeimExtractionGuid,
           &(((EFI_GUID_DEFINED_SECTION2 *)InputSection)->SectionDefinitionGuid)
           ))
    {
      return EFI_INVALID_PARAMETER;
    }

    *SectionAttribute         = ((EFI_GUID_DEFINED_SECTION2 *)InputSection)->Attributes;
    *ScratchBufferSize        = 0;
    Pe32Section.CommonHeader2 = (EFI_COMMON_SECTION_HEADER2 *)((UINT8 *)InputSection + ((EFI_GUID_DEFINED_SECTION2 *)InputSection)->DataOffset);
  } else {
    //
    // Check whether the input guid section is recognized.
    //
    if (!CompareGuid (
           &gAmdShadowPeimExtractionGuid,
           &(((EFI_GUID_DEFINED_SECTION *)InputSection)->SectionDefinitionGuid)
           ))
    {
      return EFI_INVALID_PARAMETER;
    }

    *SectionAttribute        = ((EFI_GUID_DEFINED_SECTION *)InputSection)->Attributes;
    *ScratchBufferSize       = 0;
    Pe32Section.CommonHeader = (EFI_COMMON_SECTION_HEADER *)((UINT8 *)InputSection + ((EFI_GUID_DEFINED_SECTION *)InputSection)->DataOffset);
  }

  ASSERT ((Pe32Section.CommonHeader->Type == EFI_SECTION_PE32) || (Pe32Section.CommonHeader->Type == EFI_SECTION_TE));
  if ((Pe32Section.CommonHeader->Type != EFI_SECTION_PE32) && (Pe32Section.CommonHeader->Type != EFI_SECTION_TE)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get Pe32/TE section position and size
  //
  ZeroMem (&ImageContext, sizeof (ImageContext));
  if (IS_SECTION2 (Pe32Section.CommonHeader)) {
    Pe32Data        = (VOID *)((UINT8 *)Pe32Section.CommonHeader2 + sizeof (EFI_COMMON_SECTION_HEADER2));
    Pe32SectionSize = SECTION2_SIZE (Pe32Section.CommonHeader2);
  } else {
    Pe32Data        = (VOID *)((UINT8 *)Pe32Section.CommonHeader + sizeof (EFI_COMMON_SECTION_HEADER));
    Pe32SectionSize = SECTION_SIZE (Pe32Section.CommonHeader);
  }

  //
  // If EDKII Kernel shadow feature has been enabled, do not need to relocate
  //
  BootMode = GetBootModeHob ();
  if (((BootMode != BOOT_ON_S3_RESUME) && PcdGetBool (PcdShadowPeimOnBoot)) ||
      ((BootMode == BOOT_ON_S3_RESUME) && PcdGetBool (PcdShadowPeimOnS3Boot)))
  {
    *OutputBufferSize = Pe32SectionSize;
    return EFI_SUCCESS;
  }

  ImageContext.Handle    = Pe32Data;
  ImageContext.ImageRead = PeiImageRead;

  Status = PeCoffLoaderGetImageInfo (&ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Calculate more buffer size for alignment requirement.
  //
  if (ImageContext.IsTeImage) {
    AlignImageSize =   Pe32SectionSize
                     + ((EFI_TE_IMAGE_HEADER *)Pe32Data)->StrippedSize
                     - sizeof (EFI_TE_IMAGE_HEADER)
                     - (IS_SECTION2 (Pe32Section.CommonHeader) ? sizeof (EFI_COMMON_SECTION_HEADER2) : sizeof (EFI_COMMON_SECTION_HEADER));
  } else {
    AlignImageSize =   Pe32SectionSize
                     + ALIGN_VALUE_ADDEND (IS_SECTION2 (Pe32Section.CommonHeader) ? sizeof (EFI_COMMON_SECTION_HEADER2) : sizeof (EFI_COMMON_SECTION_HEADER), ImageContext.SectionAlignment);
  }

  //
  // Rare case
  //
  if (ImageContext.SectionAlignment > EFI_PAGE_SIZE) {
    AlignImageSize += ImageContext.SectionAlignment;
  }

  *OutputBufferSize = AlignImageSize;

  return EFI_SUCCESS;
}

/**
  Extraction handler tries to extract raw data from the input guided section.
  It first checks whether the input guid section is supported.
  If not, EFI_INVALID_PARAMETER will return.

  @param[in]  InputSection          Buffer containing the input GUIDed section to be processed.
  @param[out] OutputBuffer          Buffer to contain the output raw data allocated by the caller.
  @param[in]  ScratchBuffer         A pointer to a caller-allocated buffer for function internal use.
  @param[out] AuthenticationStatus  A pointer to a caller-allocated UINT32 that indicates the
                                    authentication status of the output buffer.

  @retval EFI_SUCCESS            Section Data and Auth Status is extracted successfully.
  @retval EFI_INVALID_PARAMETER  The GUID in InputSection does not match this instance guid.

**/
EFI_STATUS
EFIAPI
ShadowPeimGuidedSectionHandler (
  IN CONST  VOID    *InputSection,
  OUT       VOID    **OutputBuffer,
  IN        VOID    *ScratchBuffer         OPTIONAL,
  OUT       UINT32  *AuthenticationStatus
  )
{
  EFI_STATUS                    Status;
  EFI_BOOT_MODE                 BootMode;
  VOID                          *Destination;
  UINT32                        Pe32SectionSize;
  VOID                          *Pe32Data;
  EFI_FILE_SECTION_POINTER      Pe32Section;
  PE_COFF_LOADER_IMAGE_CONTEXT  ImageContext;
  UINT32                        AlignImageSize;
  UINT32                        HeadRawSectionSize;
  UINT32                        TailRawSectionSize;

  if (IS_SECTION2 (InputSection)) {
    //
    // Check whether the input guid section is recognized.
    //
    if (!CompareGuid (
           &gAmdShadowPeimExtractionGuid,
           &(((EFI_GUID_DEFINED_SECTION2 *)InputSection)->SectionDefinitionGuid)
           ))
    {
      return EFI_INVALID_PARAMETER;
    }

    Pe32Section.CommonHeader2 = (EFI_COMMON_SECTION_HEADER2 *)((UINT8 *)InputSection + ((EFI_GUID_DEFINED_SECTION2 *)InputSection)->DataOffset);
  } else {
    //
    // Check whether the input guid section is recognized.
    //
    if (!CompareGuid (
           &gAmdShadowPeimExtractionGuid,
           &(((EFI_GUID_DEFINED_SECTION *)InputSection)->SectionDefinitionGuid)
           ))
    {
      return EFI_INVALID_PARAMETER;
    }

    Pe32Section.CommonHeader = (EFI_COMMON_SECTION_HEADER *)((UINT8 *)InputSection + ((EFI_GUID_DEFINED_SECTION *)InputSection)->DataOffset);
  }

  ASSERT ((Pe32Section.CommonHeader->Type == EFI_SECTION_PE32) || (Pe32Section.CommonHeader->Type == EFI_SECTION_TE));
  if ((Pe32Section.CommonHeader->Type != EFI_SECTION_PE32) && (Pe32Section.CommonHeader->Type != EFI_SECTION_TE)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If EDKII Kernel shadow feature has been enabled, do not need to relocate
  //
  BootMode = GetBootModeHob ();
  if (((BootMode != BOOT_ON_S3_RESUME) && PcdGetBool (PcdShadowPeimOnBoot)) ||
      ((BootMode == BOOT_ON_S3_RESUME) && PcdGetBool (PcdShadowPeimOnS3Boot)))
  {
    *OutputBuffer         = Pe32Section.CommonHeader;
    *AuthenticationStatus = EFI_AUTH_STATUS_IMAGE_SIGNED;
    return EFI_SUCCESS;
  }

  //
  // Get Pe32/TE section position and size
  //
  ZeroMem (&ImageContext, sizeof (ImageContext));
  if (IS_SECTION2 (Pe32Section.CommonHeader)) {
    Pe32Data        = (VOID *)((UINT8 *)Pe32Section.CommonHeader2 + sizeof (EFI_COMMON_SECTION_HEADER2));
    Pe32SectionSize = SECTION2_SIZE (Pe32Section.CommonHeader2);
  } else {
    Pe32Data        = (VOID *)((UINT8 *)Pe32Section.CommonHeader + sizeof (EFI_COMMON_SECTION_HEADER));
    Pe32SectionSize = SECTION_SIZE (Pe32Section.CommonHeader);
  }

  ImageContext.Handle    = Pe32Data;
  ImageContext.ImageRead = PeiImageRead;

  Status = PeCoffLoaderGetImageInfo (&ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Calculate more buffer size for alignment requirement.
  //
  if (ImageContext.IsTeImage) {
    AlignImageSize =   Pe32SectionSize
                     + ((EFI_TE_IMAGE_HEADER *)Pe32Data)->StrippedSize
                     - sizeof (EFI_TE_IMAGE_HEADER)
                     - (IS_SECTION2 (Pe32Section.CommonHeader) ? sizeof (EFI_COMMON_SECTION_HEADER2) : sizeof (EFI_COMMON_SECTION_HEADER));
  } else {
    AlignImageSize =   Pe32SectionSize
                     + ALIGN_VALUE_ADDEND (IS_SECTION2 (Pe32Section.CommonHeader) ? sizeof (EFI_COMMON_SECTION_HEADER2) : sizeof (EFI_COMMON_SECTION_HEADER), ImageContext.SectionAlignment);
  }

  if (ImageContext.SectionAlignment > EFI_PAGE_SIZE) {
    AlignImageSize += ImageContext.SectionAlignment;
  }

  //
  // Calculate the postion where PE32/TE section loaded to.
  //
  Destination = *OutputBuffer;
  if (ImageContext.SectionAlignment > EFI_PAGE_SIZE) {
    Destination = ALIGN_POINTER (Destination, ImageContext.SectionAlignment);
  }

  if (ImageContext.IsTeImage) {
    Destination =   (UINT8 *)Destination
                  + ((EFI_TE_IMAGE_HEADER *)Pe32Data)->StrippedSize
                  - sizeof (EFI_TE_IMAGE_HEADER)
                  - (IS_SECTION2 (Pe32Section.CommonHeader) ? sizeof (EFI_COMMON_SECTION_HEADER2) : sizeof (EFI_COMMON_SECTION_HEADER));
  } else {
    Destination =   (UINT8 *)Destination
                  + ALIGN_VALUE_ADDEND (IS_SECTION2 (Pe32Section.CommonHeader) ? sizeof (EFI_COMMON_SECTION_HEADER2) : sizeof (EFI_COMMON_SECTION_HEADER), ImageContext.SectionAlignment);
  }

  //
  // Use RAW section to occupy the region been skipped to make sure image alignment
  //
  HeadRawSectionSize = (UINT32)((UINTN)Destination - (UINTN)*OutputBuffer);
  ASSERT (HeadRawSectionSize >= sizeof (EFI_RAW_SECTION));
  TailRawSectionSize = AlignImageSize - Pe32SectionSize - HeadRawSectionSize;

  //
  // RAW section before PE32/TE section
  //
  *(UINT32 *)*OutputBuffer                 = HeadRawSectionSize;
  ((EFI_RAW_SECTION *)*OutputBuffer)->Type = EFI_SECTION_RAW;

  //
  // Copy PE32/TE section to correct destination
  //
  CopyMem (Destination, (VOID *)Pe32Section.CommonHeader, (UINTN)Pe32SectionSize);

  //
  // It's rare case, just occur when (ImageContext.SectionAlignment > EFI_PAGE_SIZE)
  //
  if (TailRawSectionSize >= sizeof (EFI_RAW_SECTION)) {
    *(UINT32 *)((UINTN)Destination + (UINTN)Pe32SectionSize)                 = TailRawSectionSize;
    ((EFI_RAW_SECTION *)((UINTN)Destination + (UINTN)Pe32SectionSize))->Type = EFI_SECTION_RAW;
  }

  *AuthenticationStatus = EFI_AUTH_STATUS_IMAGE_SIGNED;

  return EFI_SUCCESS;
}

/**
  Register the handler to extract ShadowPeim guided section.

  @param[in]  FileHandle   The handle of FFS header the loaded driver.
  @param[in]  PeiServices  The pointer to the PEI services.

  @retval  EFI_SUCCESS           Register successfully.
  @retval  EFI_OUT_OF_RESOURCES  Not enough memory to register this handler.

**/
EFI_STATUS
EFIAPI
PeiShadowPeimExtractLibConstructor (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  return ExtractGuidedSectionRegisterHandlers (
           &gAmdShadowPeimExtractionGuid,
           ShadowPeimGuidedSectionGetInfo,
           ShadowPeimGuidedSectionHandler
           );
}
