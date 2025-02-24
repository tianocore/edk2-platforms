/** @file
  To support PEI shadow extraction library.

  Copyright (C) 2024 - 2025, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PEI_SHADOW_PEIM_EXTRACT_LIB_H_
#define PEI_SHADOW_PEIM_EXTRACT_LIB_H_

typedef union {
  EFI_COMMON_SECTION_HEADER             *CommonHeader;
  EFI_COMPRESSION_SECTION               *CompressionSection;
  EFI_GUID_DEFINED_SECTION              *GuidDefinedSection;
  EFI_PE32_SECTION                      *Pe32Section;
  EFI_PIC_SECTION                       *PicSection;
  EFI_TE_SECTION                        *TeSection;
  EFI_PEI_DEPEX_SECTION                 *PeimHeaderSection;
  EFI_DXE_DEPEX_SECTION                 *DependencySection;
  EFI_VERSION_SECTION                   *VersionSection;
  EFI_USER_INTERFACE_SECTION            *UISection;
  EFI_COMPATIBILITY16_SECTION           *Code16Section;
  EFI_FIRMWARE_VOLUME_IMAGE_SECTION     *FVImageSection;
  EFI_FREEFORM_SUBTYPE_GUID_SECTION     *FreeformSubtypeSection;
  EFI_RAW_SECTION                       *RawSection;
  //
  // For section whose size is equal or greater than 0x1000000
  //
  EFI_COMMON_SECTION_HEADER2            *CommonHeader2;
  EFI_COMPRESSION_SECTION2              *CompressionSection2;
  EFI_GUID_DEFINED_SECTION2             *GuidDefinedSection2;
  EFI_PE32_SECTION2                     *Pe32Section2;
  EFI_PIC_SECTION2                      *PicSection2;
  EFI_TE_SECTION2                       *TeSection2;
  EFI_PEI_DEPEX_SECTION2                *PeimHeaderSection2;
  EFI_DXE_DEPEX_SECTION2                *DependencySection2;
  EFI_VERSION_SECTION2                  *VersionSection2;
  EFI_USER_INTERFACE_SECTION2           *UISection2;
  EFI_COMPATIBILITY16_SECTION2          *Code16Section2;
  EFI_FIRMWARE_VOLUME_IMAGE_SECTION2    *FVImageSection2;
  EFI_FREEFORM_SUBTYPE_GUID_SECTION2    *FreeformSubtypeSection2;
  EFI_RAW_SECTION2                      *RawSection2;
} EFI_FILE_SECTION_POINTER;

#endif // PEI_SHADOW_PEIM_EXTRACT_LIB_H_
