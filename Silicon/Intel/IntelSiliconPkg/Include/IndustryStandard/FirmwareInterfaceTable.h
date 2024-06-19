/** @file
  Firmware Interface Table (FIT) related definitions.

  Copyright (c) 2016 - 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Specification Reference:
    - Firmware Interface Table Revision 1.5

**/

#ifndef __FIRMWARE_INTERFACE_TABLE_H__
#define __FIRMWARE_INTERFACE_TABLE_H__

//
// FIT entry type definitions.
//
#define FIT_TYPE_00_HEADER                  0x00
#define FIT_TYPE_01_MICROCODE               0x01
#define FIT_TYPE_02_STARTUP_ACM             0x02
#define FIT_TYPE_03_DIAGNOSTIC_ACM          0x03
#define FIT_TYPE_04_PROT_BOOT_POLICY        0x04
#define FIT_TYPE_05_MMC_FIRMWARE_IMAGE      0x05
#define FIT_TYPE_06_FIT_RESET_STATE         0x06
#define FIT_TYPE_07_BIOS_STARTUP_MODULE     0x07
#define FIT_TYPE_08_TPM_POLICY              0x08
#define FIT_TYPE_09_BIOS_POLICY             0x09
#define FIT_TYPE_0A_TXT_POLICY              0x0A
#define FIT_TYPE_0B_KEY_MANIFEST            0x0B
#define FIT_TYPE_0C_BOOT_POLICY_MANIFEST    0x0C
#define FIT_TYPE_0D_FSP_BOOT_MANIFEST       0x0D
#define FIT_TYPE_10_CSE_SECURE_BOOT         0x10
#define FIT_TYPE_1A_VAB_PROVISIONING_TABLE  0x1A
#define FIT_TYPE_1B_VAB_KEY_MANIFEST        0x1B
#define FIT_TYPE_1C_VAB_IMAGE_MANIFEST      0x1C
#define FIT_TYPE_1D_VAB_IMAGE_HASH_DESC     0x1D
#define FIT_TYPE_2C_SACM_DEBUG              0x2C
#define FIT_TYPE_2D_TXTSX_POLICY            0x2D
#define FIT_TYPE_2E_GRANULAR_SCRTM_ERROR    0x2E
#define FIT_TYPE_2F_JMP_DEBUG_POLICY        0x2F
#define FIT_TYPE_7F_SKIP                    0x7F

//
// FIT pointer definitions.
//
#define FIT_POINTER_ADDRESS  0xFFFFFFC0 ///< Fixed address at 4G - 40h

//
// The entire FIT table must reside within the firmware address range of
// (4 GB to 16 MB) to (4 GB-40h).
// If the FIT is located outside this region, the processor will invoke
// a legacy boot process and a root of trust will not be established using FIT.
//
#define FIT_TABLE_LOWER_ADDRESS  (BASE_4GB - SIZE_16MB)
#define FIT_TABLE_UPPER_ADDRESS  FIT_POINTER_ADDRESS

//
// FIT entry version definitions.
//
#define FIT_TYPE_VERSION         0x0100
#define FIT_TYPE_02_VERSION_200  0x0200

#define FIT_TYPE_00_SIGNATURE  SIGNATURE_64 ('_', 'F', 'I', 'T', '_', ' ', ' ', ' ')

#pragma pack (1)

typedef struct {
  /**
    Address is the base address of the firmware component
    must be aligned on 16 byte boundary
  **/
  UINT64 Address;
  UINT8  Size[3];   ///< Size is the span of the component in multiple of 16 bytes
  UINT8  Reserved;  ///< Reserved must be set to 0
  /**
    Component's version number in binary coded decimal (BCD) format.
    For the FIT header entry, the value in this field will indicate the revision
    number of the FIT data structure. The upper byte of the revision field
    indicates the major revision and the lower byte indicates the minor revision.
  **/
  UINT16 Version;
  UINT8  Type : 7;  ///< FIT types 0x00 to 0x7F
  ///
  /// Checksum Valid indicates whether component has valid checksum.
  ///
  UINT8  C_V  : 1;
  /**
    Component's checksum. The modulo sum of all the bytes in the component and
    the value in this field (Chksum) must add up to zero. This field is only
    valid if the C_V flag is non-zero.
  **/
  UINT8  Chksum;
} FIRMWARE_INTERFACE_TABLE_ENTRY;

typedef struct {
  UINT64    Address;                ///< Same definition as generic FIT entry.
  UINT8     Model             : 4;  ///< Processor model field.
  UINT8     Family            : 4;  ///< Processor family field.
  UINT8     ProcessorType     : 4;  ///< Processor type field.
  UINT8     ExtModel          : 4;  ///< Processor extended model field.
  UINT8     ModelMask         : 4;  ///< Processor model field mask.
  UINT8     FamilyMask        : 4;  ///< Processor family field mask.
  UINT8     ProcessorTypeMask : 4;  ///< Processor type field mask.
  UINT8     ExtModelMask      : 4;  ///< Processor extended model field mask.
  UINT16    Version;                ///< Version field must be 0x0200.
  UINT8     Type              : 7;  ///< Type filed must be 0x02.
  UINT8     C_V               : 1;  ///< C_V field must be 0x0.
  UINT8     ExtFamily         : 4;  ///< Processor extended family field.
  UINT8     ExtFamilyMask     : 4;  ///< Processor extended family field mask.
} FIT_TYPE_02_VERSION_200_ENTRY;

#pragma pack ()

#endif
