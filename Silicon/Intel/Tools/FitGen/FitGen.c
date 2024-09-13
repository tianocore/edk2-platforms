/**@file
This utility is part of build process for IA32/X64 FD.
It generates FIT table.

Copyright (c) 2010-2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "FitGen.h"

//
// FIT spec
//
#pragma pack (1)
typedef struct {
  UINT64     Address;
  UINT8      Size[3];
  UINT8      Rsvd;
  UINT16     Version;
  UINT8      Type:7;
  UINT8      C_V:1;
  UINT8      Checksum;
} FIRMWARE_INTERFACE_TABLE_ENTRY;

//
// Struct for policy
//
typedef struct {
  UINT16     IndexPort;
  UINT16     DataPort;
  UINT8      Width;
  UINT8      Bit;
  UINT16     Index;
  UINT8      Size[3];
  UINT8      Rsvd;
  UINT16     Version; // == 0
  UINT8      Type:7;
  UINT8      C_V:1;
  UINT8      Checksum;
} FIRMWARE_INTERFACE_TABLE_ENTRY_PORT;

#define FIT_ALIGNMENT          0x3F // 0xF is required previously, but if we need exclude FIT, we must set 64 bytes alignment.
#define BIOS_MODULE_ALIGNMENT  0x3F  // 64 bytes for AnC
#define MICROCODE_ALIGNMENT    0x7FF

#define MICROCODE_EXTERNAL_HEADER_SIZE 0x30

#define ACM_PKCS_1_5_RSA_SIGNATURE_SHA256_SIZE          256
#define ACM_PKCS_1_5_RSA_SIGNATURE_SHA384_SIZE          384

#define ACM_XMSS_PUBLIC_KEY_SIZE                        64
#define ACM_XMSS_SIGNATURE_SIZE                         2692

#define ACM_HEADER_VERSION_5                            0x50004
#define ACM_HEADER_VERSION_4                            (4 << 16)
#define ACM_HEADER_VERSION_3                            (3 << 16)
#define ACM_HEADER_VERSION_0                            (0)
#define ACM_MODULE_TYPE_CHIPSET_ACM                     2
#define ACM_MODULE_SUBTYPE_CAPABLE_OF_EXECUTE_AT_RESET  0x1
#define ACM_MODULE_SUBTYPE_ANC_MODULE                   0x2
#define ACM_MODULE_FLAG_PREPRODUCTION                   0x4000
#define ACM_MODULE_FLAG_DEBUG_SIGN                      0x8000

#define NIBBLES_TO_BYTE(A, B)  (UINT8)(((A & (0x0F)) << 4) | (B & 0x0F))
//
//Flash Map 0 Register (Flash Descriptor Records)
//
typedef struct {
  UINT32      Fcba : 8;  //Bits[7:0]: Flash Component Base Address
  UINT32      Nc   : 2;  //Bits[9:8]: Number of Components
  UINT32      Rsvd0: 1;  //Bit10: Reserved
  UINT32      Rsvd1: 1;  //Bit11: Reserved
  UINT32      Rsvd2: 1;  //Bit12: Reserved
  UINT32      Rsvd3: 3;  //Bits[15:13]: Reserved
  UINT32      Frba : 8;  //Bits[23:16]: Flash Region Base Address
  UINT32      Rsvd4: 3;  //Bits[26:24]: Reserved
  UINT32      Rsvd5: 5;  //Bits[31:27]: Reserved
} FLASH_MAP_0_REGISTER;

//
//Flash Region 1 (BIOS) Register (Flash Descriptor Records)
//
typedef struct {
  UINT32      RegionBase : 15;  //Bits[14:0]: Region base
  UINT32      Rsvd       : 1;   //Bit15: Reserved
  UINT32      RegionLimit: 15;  //Bits[30:16]: Region limit
  UINT32      Rsvd1      : 1;   //Bit31: Reserved
} FLASH_REGION_1_BIOS_REGISTER;

#define FLASH_VALID_SIGNATURE                           0x0FF0A55A   //Flash Valid Signature (Flash Descriptor Records)
#define FLVALSIG_BASE_OFFSET                            0x10         //Flash Valid Signature Base Offset
#define FLMAP0_BASE_OFFSET                              0x14         //Flash Map 0 Register Base Offset

#define ACMFV_GUID \
  { 0x8a4b197f, 0x1113, 0x43d0, { 0xa2, 0x3f, 0x26, 0xf3, 0x69, 0xb2, 0xb8, 0x41 }}

typedef struct {
  UINT16     ModuleType;
  UINT16     ModuleSubType;
  UINT32     HeaderLen;
  UINT32     HeaderVersion;
  UINT16     ChipsetID;
  UINT16     Flags;
  UINT32     ModuleVendor;
  UINT32     Date;
  UINT32     Size;
  UINT16     TxtSvn;
  UINT16     SeSvn;
  UINT32     CodeControl;
  UINT32     ErrorEntryPoint;
  UINT32     GDTLimit;
  UINT32     GDTBasePtr;
  UINT32     SegSel;
  UINT32     EntryPoint;
  UINT8      Rsvd2[64];
  UINT32     KeySize; // 64
  UINT32     ScratchSize; // 2 * KeySize + 15
//UINT8      RSAPubKey[64 * 4]; // KeySize * 4
//UINT32     RSAPubExp;
//UINT8      RSASig[256];
  // End of AC module header
//UINT8      Scratch[(64 * 2 + 15) * 4]; // ScratchSize * 4
  // User Area
//UINT8      UserArea[1];
} ACM_FORMAT;

#define CHIPSET_ACM_INFORMATION_TABLE_VERSION_3  0x03
#define CHIPSET_ACM_INFORMATION_TABLE_VERSION_4  0x04

#define CHIPSET_ACM_INFORMATION_TABLE_VERSION    CHIPSET_ACM_INFORMATION_TABLE_VERSION_3

#define CHIPSET_ACM_INFORMATION_TABLE_GUID_V03 \
  { 0x7FC03AAA, 0x18DB46A7, 0x8F69AC2E, 0x5A7F418D }

#define CHIPSET_ACM_TYPE_BIOS   0
#define CHIPSET_ACM_TYPE_SINIT  1

#define DEFAULT_ACM_EXTENDED_MASK 0x00FFFFFF

typedef struct {
  UINT32    Guid0;
  UINT32    Guid1;
  UINT32    Guid2;
  UINT32    Guid3;
} ACM_GUID;

typedef struct {
  ACM_GUID   Guid;
  UINT8      ChipsetACMType;
  UINT8      Version;
  UINT16     Length;
  UINT32     ChipsetIDList;
  UINT32     OsSinitTableVer;
  UINT32     MinMleHeaderVer;
//#if (CHIPSET_ACM_INFORMATION_TABLE_VERSION >= CHIPSET_ACM_INFORMATION_TABLE_VERSION_3)
  UINT32     Capabilities;
  UINT8      AcmVersion;
  UINT8      AcmRevision[3];
//#if (CHIPSET_ACM_INFORMATION_TABLE_VERSION >= CHIPSET_ACM_INFORMATION_TABLE_VERSION_4)
  UINT32     ProcessorIDList;
//#endif
//#endif
} CHIPSET_ACM_INFORMATION_TABLE;

#define ACM_CHIPSET_ID_REVISION_ID_MAKE  0x1

typedef struct {
  UINT32     Flags;
  UINT16     VendorID;
  UINT16     DeviceID;
  UINT16     RevisionID;
  UINT8      Reserved[6];
} ACM_CHIPSET_ID;

typedef struct {
  UINT32           Count;
  ACM_CHIPSET_ID   ChipsetID[1];
} CHIPSET_ID_LIST;

typedef struct {
  UINT32     FMS;
  UINT32     FMSMask;
  UINT64     PlatformID;
  UINT64     PlatformMask;
} ACM_PROCESSOR_ID;

typedef struct {
  UINT32           Count;
  ACM_PROCESSOR_ID ProcessorID[1];
} PROCESSOR_ID_LIST;

typedef union {
  struct {
    UINT32  Stepping      : 4;
    UINT32  Model         : 4;
    UINT32  Family        : 4;
    UINT32  Type          : 2;
    UINT32  Reserved1     : 2;
    UINT32  ExtendedModel : 4;
    UINT32  ExtendedFamily: 8;
    UINT32  Reserved2     : 4;
  } Bits;
  UINT32  Uint32;
} PROCESSOR_ID;

#pragma pack ()


ACM_GUID mChipsetAcmInformationTableGuid03 = CHIPSET_ACM_INFORMATION_TABLE_GUID_V03;


//
// BIOS INFO data structure
// This is self contained data structure for BIOS info
//
#pragma pack (1)
#define BIOS_INFO_SIGNATURE  SIGNATURE_64 ('$', 'B', 'I', 'O', 'S', 'I', 'F', '$')
typedef struct {
  UINT64            Signature;
  UINT32            EntryCount;
  UINT32            Reserved;
//BIOS_INFO_STRUCT  Struct[EntryCount];
} BIOS_INFO_HEADER;

//
// BIOS_INFO_STRUCT attributes
// bits[0:3] means general attributes
// bits[4:7] means type specific attributes
//
#define BIOS_INFO_STRUCT_ATTRIBUTE_GENERAL_EXCLUDE_FROM_FIT  0x01
#define BIOS_INFO_STRUCT_ATTRIBUTE_MICROCODE_WHOLE_REGION    0x10
#define BIOS_INFO_STRUCT_ATTRIBUTE_BIOS_POST_IBB             0x10

typedef struct {
  //
  // FitTable entry type
  //
  UINT8    Type;
  //
  // BIOS_INFO_STRUCT attributes
  //
  UINT8    Attributes;
  //
  // FitTable entry version
  //
  UINT16   Version;
  //
  // FitTable entry real size
  //
  UINT32   Size;
  //
  // FitTable entry address
  //
  UINT64   Address;
} BIOS_INFO_STRUCT;

#pragma pack ()

#define MAX_BIOS_MODULE_ENTRY  0x20
#define MAX_MICROCODE_ENTRY    0x20
#define MAX_STARTUP_ACM_ENTRY  0x20
#define MAX_OPTIONAL_ENTRY     0x20
#define MAX_PORT_ENTRY         0x20

#define DEFAULT_FIT_TABLE_POINTER_OFFSET  0x40
#define DEFAULT_FIT_ENTRY_VERSION         0x0100
#define STARTUP_ACM_FIT_ENTRY_200_VERSION 0x0200

#define MAX_MMCFW_MODULE_ENTRY   0x02

#define TOP_FLASH_ADDRESS  (gFitTableContext.TopFlashAddressRemapValue)

#define MEMORY_TO_FLASH(FileBuffer, FvBuffer, FvSize)  \
                 (UINTN)(TOP_FLASH_ADDRESS - ((UINTN)(FvBuffer) + (UINTN)(FvSize) - (UINTN)(FileBuffer)))
#define FLASH_TO_MEMORY(Address, FvBuffer, FvSize)  \
                 (VOID *)(UINTN)((UINTN)(FvBuffer) + (UINTN)(FvSize) - (TOP_FLASH_ADDRESS - (UINTN)(Address)))

#define FIT_TABLE_TYPE_HEADER                      0
#define FIT_TABLE_TYPE_MICROCODE                   1
#define FIT_TABLE_TYPE_STARTUP_ACM                 2
#define FIT_TABLE_TYPE_DIAGNST_ACM                 3
#define FIT_TABLE_TYPE_PROT_BOOT_POLICY            4
#define FIT_TABLE_TYPE_MMC_FW                      5
#define FIT_TABLE_TYPE_BIOS_MODULE                 7
#define FIT_TABLE_TYPE_TPM_POLICY                  8
#define FIT_TABLE_TYPE_BIOS_POLICY                 9
#define FIT_TABLE_TYPE_TXT_POLICY                  10
#define FIT_TABLE_TYPE_KEY_MANIFEST                11
#define FIT_TABLE_TYPE_BOOT_POLICY_MANIFEST        12
#define FIT_TABLE_TYPE_FSP_BOOT_MANIFEST           13
#define FIT_TABLE_TYPE_CSE_SECURE_BOOT             16
#define FIT_TABLE_SUBTYPE_FIT_PATCH_MANIFEST       12
#define FIT_TABLE_SUBTYPE_ACM_MANIFEST             13
#define FIT_TABLE_TYPE_VAB_PROVISION_TABLE         26
#define FIT_TABLE_TYPE_VAB_BOOT_IMAGE_MANIFEST     27
#define FIT_TABLE_TYPE_VAB_BOOT_KEY_MANIFEST       28

//
// With OptionalModule Address isn't known until free space has been
// identified and the optional module has been copied into the FLASH
// image buffer (or initialized to be populated later by another program).
// This is very dangerous code as it can truncate 64b pointers to
// allocated memory buffers.  The full pointer is in Buffer for that case.
//
typedef struct {
  UINT32  Type;
  UINT32  SubType; // Used by OptionalModule only
  UINT32  Address;
  UINT8   *Buffer; // Used by OptionalModule only
  UINT32  Size;
  UINT32  Version; // Used by OptionalModule and PortModule only
  UINT32  FMS;     // Used by Entry Type 02 (ACM) Ver. 0x200 only
  UINT32  FMSMask; // Used by Entry Type 02 (ACM) Ver. 0x200 only
} FIT_TABLE_CONTEXT_ENTRY;

typedef struct {
  BOOLEAN                    Clear;
  UINT32                     FitTablePointerOffset;
  UINT32                     FitTablePointerOffset2;
  UINT32                     FitEntryNumber;
  UINT32                     BiosModuleNumber;
  UINT32                     MicrocodeNumber;
  UINT32                     MmcFwNumber;
  UINT32                     StartupAcmNumber;
  UINT32                     OptionalModuleNumber;
  UINT32                     PortModuleNumber;
  UINT32                     GlobalVersion;
  UINT32                     FitHeaderVersion;
  FIT_TABLE_CONTEXT_ENTRY    StartupAcm[MAX_STARTUP_ACM_ENTRY];
  UINT32                     StartupAcmFvSize;
  FIT_TABLE_CONTEXT_ENTRY    DiagnstAcm;
  UINT32                     DiagnstAcmVersion;
  FIT_TABLE_CONTEXT_ENTRY    ProtBootPolicy;
  FIT_TABLE_CONTEXT_ENTRY    BiosModule[MAX_BIOS_MODULE_ENTRY];
  UINT32                     BiosModuleVersion;
  FIT_TABLE_CONTEXT_ENTRY    Microcode[MAX_MICROCODE_ENTRY];
  BOOLEAN                    MicrocodeIsAligned;
  UINT32                     MicrocodeAlignValue;
  UINT32                     MicrocodeVersion;
  FIT_TABLE_CONTEXT_ENTRY    OptionalModule[MAX_OPTIONAL_ENTRY];
  FIT_TABLE_CONTEXT_ENTRY    PortModule[MAX_PORT_ENTRY];
  FIT_TABLE_CONTEXT_ENTRY    MmcFw[MAX_MMCFW_MODULE_ENTRY];
  UINT64                     TopFlashAddressRemapValue;
} FIT_TABLE_CONTEXT;

FIT_TABLE_CONTEXT   gFitTableContext = {0};

unsigned int
xtoi (
  char  *str
  );

/**
  Pass in supported CPU extended family/extended model/type
  /family/model without stepping or CPU FMS >> 4.

  @param FitEntry               Pointer to Fit Entry table.
  @param AcmFamilyModel         Acm Family Model stepping.
  @param AcmMask                ACM mask.

  @return STATUS_SUCCESS  The file found and data read.
**/
STATUS
SetFirmwareInterfaceTableEntryAcmFms(
  FIRMWARE_INTERFACE_TABLE_ENTRY  *FitEntry,
  UINT32                          AcmFamilyModel,
  UINT32                          AcmMask
)
{
  if (FitEntry == NULL) {
    return STATUS_ERROR;
  }

  FitEntry->Checksum = (UINT8)(((AcmFamilyModel & 0x000F0000) >> 16) | (((AcmMask & 0x000F0000) >> 16) << 4));
  FitEntry->Rsvd = (UINT8)((AcmMask & 0x0000FF00) >> 8);
  FitEntry->Size[2] = (UINT8)(AcmMask & 0x000000FF);
  FitEntry->Size[1] = (UINT8)((AcmFamilyModel & 0x0000FF00) >> 8);
  FitEntry->Size[0] = (UINT8)(AcmFamilyModel & 0x000000FF);
  return STATUS_SUCCESS;
}

/**
  Set the FIT Entry Size.

  @param FitEntry                Pointer to Fit Entry table.
  @param SizeEntry               Size of FIT entry.

  @return STATUS_SUCCESS  The file found and data read.
**/
STATUS
SetFirmwareInterfaceTableEntrySize (
  FIRMWARE_INTERFACE_TABLE_ENTRY  *FitEntry,
  UINT32                          SizeEntry
)
{
  if (FitEntry == NULL) {
    return STATUS_ERROR;
  }
  FitEntry->Size[2] = (UINT8)((SizeEntry & 0x00FF0000) >> 16);
  FitEntry->Size[1] = (UINT8)((SizeEntry & 0x0000FF00) >> 8);
  FitEntry->Size[0] = (UINT8)(SizeEntry  & 0x000000FF);
  return STATUS_SUCCESS;
}

/**
  Get the FIT Entry Size.

  @param  FitEntry                Pointer to Fit Entry table.

  @return FitEntry pointer
**/
UINT32
GetFirmwareInterfaceTableEntrySize (
  FIRMWARE_INTERFACE_TABLE_ENTRY  *FitEntry
)
{
  if (FitEntry == NULL) {
    return 0;
  }
  return (((UINT32)FitEntry->Size[2] << 16) | ((UINT32)FitEntry->Size[1] << 8) | (UINT32)FitEntry->Size[0]);
}

/**
  Displays the FIT utility info

  @param                           None

  @return None
**/
VOID
PrintUtilityInfo (
  VOID
  )
{
  printf (
    "%s - Tiano IA32/X64 FIT table generation Utility for FIT spec revision %i.%i."" Version %i.%i\n\n",
    UTILITY_NAME,
    FIT_SPEC_VERSION_MAJOR,
    FIT_SPEC_VERSION_MINOR,
    UTILITY_MAJOR_VERSION,
    UTILITY_MINOR_VERSION
    );
}

/**
  Displays the utility usage syntax to STDOUT.

  @param  None

  @return None
**/
VOID
PrintUsage (
  VOID
  )
{
  printf ("Usage (generate): %s [-D] InputFvRecoveryFile OutputFvRecoveryFile\n"
          "\t[-V <FitEntryDefaultVersion>]\n"
          "\t[-F <FitTablePointerOffset>] [-F <FitTablePointerOffset>] [-V <FitHeaderVersion>]\n"
          "\t[-NA]\n"
          "\t[-A <MicrocodeAlignment>]\n"
          "\t[-REMAP <TopFlashAddress>\n"
          "\t[-CLEAR]\n"
          "\t[-L <MicrocodeSlotSize> <MicrocodeFfsGuid>]\n"
          "\t[-LF <MicrocodeSlotSize>]\n"
          "\t[-I <BiosInfoGuid>]\n"
          "\t[-S <StartupAcmAddress StartupAcmSize>|<StartupAcmGuid>] [-I <StartupAcmFMS StartupAcmFMSMask>] [-V <StartupAcmVersion>]\n"
          "\t[-U <DiagnstAcmAddress>|<DiagnstAcmGuid>]\n"
          "\t[-B <BiosModuleAddress BiosModuleSize>] [-B ...] [-V <BiosModuleVersion>]\n"
          "\t[-M <MicrocodeAddress MicrocodeSize>] [-M ...]|[-U <MicrocodeFv MicrocodeBase>|<MicrocodeRegionOffset MicrocodeRegionSize>|<MicrocodeGuid>] [-V <MicrocodeVersion>]\n"
          "\t[-O RecordType <RecordDataAddress RecordDataSize>|<RESERVE RecordDataSize>|<RecordDataGuid>|<RecordBinFile>|<CseRecordSubType RecordBinFile> [-V <RecordVersion>]] [-O ... [-V ...]]\n"
          "\t[-P RecordType <IndexPort DataPort Width Bit Index> [-V <RecordVersion>]] [-P ... [-V ...]]\n"
          "\t[-BP <BootPolicySize>[-V <BootPolicyVersion>]\n"
          "\t[-T <FixedFitLocation>]\n"
          , UTILITY_NAME);
  printf ("  Where:\n");
  printf ("\t-D                     - It is FD file instead of FV file. (The tool will search FV file)\n");
  printf ("\tInputFvRecoveryFile    - Name of the input FvRecovery.fv file.\n");
  printf ("\tOutputFvRecoveryFile   - Name of the output FvRecovery.fv file.\n");
  printf ("\tFitTablePointerOffset  - FIT table pointer offset. 0x%x as default. 0x18 for current soon to be obsoleted CPUs. User can set both.\n", DEFAULT_FIT_TABLE_POINTER_OFFSET);
  printf ("\tBiosInfoGuid           - Guid of BiosInfo Module. If this module exists, StartupAcm/Bios/Microcode can be optional.\n");
  printf ("\tStartupAcmAddress      - Address of StartupAcm.\n");
  printf ("\tStartupAcmSize         - The maximum size value that could place the StartupAcm in.\n");
  printf ("\tStartupAcmGuid         - Guid of StartupAcm Module, if StartupAcm is in a BiosModule, it will be excluded form that.\n");
  printf ("\tStartupAcmFMS          - Value of PROCESSOR ID (Family/Model/Stepping value called \"FMS\") - see detail on FIT spec (1.3).\n");
  printf ("\tStartupAcmFMSMask      - Value use for uCode (if it recognizes 0x200 Type2 entry) to do bitmask logic operation with CPU processor ID.\n");
  printf ("\t                         If the result match to StartupAcmFMS, corresponding ACM will be loaded - see detail on FIT spec (1.3).\n");
  printf ("\tDiagnstAcmAddress      - Address of DiagnstAcm.\n");
  printf ("\tDiagnstAcmGuid         - Guid of DiagnstAcm Module, if DiagnstAcm is in a BiosModule, it will be excluded from that.\n");
  printf ("\tBiosModuleAddress      - Address of BiosModule. User should ensure there is no overlap.\n");
  printf ("\tBiosModuleSize         - Size of BiosModule.\n");
  printf ("\tMicrocodeAddress       - Address of Microcode.\n");
  printf ("\tMicrocodeSize          - Size of Microcode.\n");
  printf ("\tMicrocodeFv            - Name of Microcode.fv file.\n");
  printf ("\tMicrocodeBase          - The base address of Microcode.fv in final FD image.\n");
  printf ("\tMicrocodeRegionOffset  - Offset of Microcode region in input FD image.\n");
  printf ("\tMicrocodeRegionSize    - Size of Microcode region in input FD image.\n");
  printf ("\tMicrocodeGuid          - Guid of Microcode Module.\n");
  printf ("\tMicrocodeSlotSize      - Occupied region size of each Microcode binary.\n");
  printf ("\tMicrocodeFfsGuid       - Guid of FFS which is used to save Microcode binary");
  printf ("\t-LF                    - Microcode Slot mode without FFS check, treat all Microcode FV as slot mode. In this case the Microcode FV should only contain one FFS.\n");
  printf ("\t-NA                    - No 0x800 aligned Microcode requirement. No -NA means Microcode is aligned with option MicrocodeAlignment value.\n");
  printf ("\tMicrocodeAlignment     - HEX value of Microcode alignment. Ignored if \"-NA\" is specified. Default value is 0x800. The Microcode update data must start at a 16-byte aligned linear address.\n");
  printf ("\tRecordType             - FIT entry record type. User should ensure it is ordered.\n");
  printf ("\tRecordDataAddress      - FIT entry record data address.\n");
  printf ("\tRecordDataSize         - FIT entry record data size.\n");
  printf ("\tRecordDataGuid         - FIT entry record data GUID.\n");
  printf ("\tRecordBinFile          - FIT entry record data binary file.\n");
  printf ("\tCseRecordSubType       - FIT entry record subtype. Use to further distinguish CSE entries (see FIT spec revision 1.2 chapter 4.12).\n");
  printf ("\tBootPolicySize         - FIT entry size for type 04 boot policy.\n");
  printf ("\tFitEntryDefaultVersion - The default version for all FIT table entries. 0x%04x is used if this is not specified.\n", DEFAULT_FIT_ENTRY_VERSION);
  printf ("\tFitHeaderVersion       - The version for FIT header. (Override default version)\n");
  printf ("\tStartupAcmVersion      - The version for StartupAcm. (Override default version)\n");
  printf ("\tBiosModuleVersion      - The version for BiosModule. (Override default version)\n");
  printf ("\tMicrocodeVersion       - The version for Microcode. (Override default version)\n");
  printf ("\tRecordVersion          - The version for Record. (Override default version)\n");
  printf ("\tBootPolicyVersion      - The version for BootPolicy.     (Override default version)\n");
  printf ("\tIndexPort              - The Index Port Number.\n");
  printf ("\tDataPort               - The Data Port Number.\n");
  printf ("\tWidth                  - The Width of the port.\n");
  printf ("\tBit                    - The Bit Number of the port.\n");
  printf ("\tIndex                  - The Index Number of the port.\n");
  printf ("\tFixedFitLocation       - Fixed FIT location in flash address. FIT table will be generated at this location and Option Modules will be directly put right before it.\n");
  printf ("\nUsage (view): %s [-view] InputFile -F <FitTablePointerOffset>\n", UTILITY_NAME);
  printf ("  Where:\n");
  printf ("\tInputFile              - Name of the input file.\n");
  printf ("\tFitTablePointerOffset  - FIT table pointer offset from end of file. 0x%x as default.\n", DEFAULT_FIT_TABLE_POINTER_OFFSET);
  printf ("\nTool return values:\n");
  printf ("\tSTATUS_SUCCESS=%d, STATUS_WARNING=%d, STATUS_ERROR=%d\n", STATUS_SUCCESS, STATUS_WARNING, STATUS_ERROR);
}

/**
  Set Value of memory.

  @param Buffer                    The pointer where we need to set the memory.
  @param Length                    Size of memory to be set.
  @param Value                     Value of memory to be set.

  @return Buffer  The pointer address.
**/
VOID *
SetMem (
  OUT VOID                 *Buffer,
  IN UINTN                 Length,
  IN UINT8                 Value
  )
{
  //
  // Declare the local variables that actually move the data elements as
  // volatile to prevent the optimizer from replacing this function with
  // the intrinsic memset()
  //
  volatile UINT8                    *Pointer;

  Pointer = (UINT8*)Buffer;
  while (Length-- > 0) {
    *(Pointer++) = Value;
  }
  return Buffer;
}

/**
  check the input Path.

  @param String         Passed input path.

  @return TRUE          If the input path is correct.
  @return FLASE         if the input path is not correct.
**/
BOOLEAN
CheckPath (
  IN CHAR8 * String
)
{
  //
  //Return FLASE if  input file path include % character or is NULL
  //
  CHAR8 *StrPtr;

  StrPtr = String;
  if (StrPtr == NULL) {
    return FALSE;
  }

  if (*StrPtr == 0) {
    return FALSE;
  }

  while (*StrPtr != '\0') {
    if (*StrPtr == '%') {
      return FALSE;
    }
    StrPtr++;
  }
  return TRUE;
}

/**
  Get fixed FIT location from argument.

  @param argc                Number of command line parameters.
  @param argv                Array of pointers to parameter strings.

  @return FitLocation        The FIT location specified by Argument.
  @return 0                  Argument parse fail.
**/
UINT32
GetFixedFitLocation (
  IN INTN   argc,
  IN CHAR8  **argv
  )
{
  UINT32                      FitLocation;
  INTN                        Index;

  FitLocation = 0;

  for (Index = 0; Index + 1 < argc; Index ++) {

    if ((strcmp (argv[Index], "-T") == 0) ||
        (strcmp (argv[Index], "-t") == 0) ) {
      FitLocation =  xtoi (argv[Index + 1]);
      break;
    }
  }

  return FitLocation;
}

/**
  Read input file.

  @param FileName                    The input file name.
  @param FileData                    The input file data, the memory is aligned.
  @param FileSize                    The input file size.
  @param FileBufferRaw               The memory to hold input file data. The caller must free the memory.

  @return STATUS_SUCCESS             The file found and data read.
  @return STATUS_ERROR               The file data is not read.
  @return STATUS_WARNING             The file is not found.
**/
STATUS
ReadInputFile (
  IN CHAR8    *FileName,
  OUT UINT8   **FileData,
  OUT UINT32  *FileSize,
  OUT UINT8   **FileBufferRaw OPTIONAL
  )
{
  FILE                        *FpIn;
  UINT32                      TempResult;

  //
  //Check the File Path
  //
  if (!CheckPath(FileName)) {

    Error (NULL, 0, 0, "File path is invalid!", NULL);
    return STATUS_ERROR;
  }

  //
  // Open the Input FvRecovery.fv file
  //
  if ((FpIn = fopen (FileName, "rb")) == NULL) {
    //
    // Return WARNING, let caller make decision
    //
//    Error (NULL, 0, 0, "Unable to open file", FileName);
    return STATUS_WARNING;
  }
  //
  // Get the Input FvRecovery.fv file size
  //
  fseek (FpIn, 0, SEEK_END);
  *FileSize = ftell (FpIn);
  //
  // Read the contents of input file to memory buffer
  //
  if (FileBufferRaw != NULL) {
    *FileBufferRaw = (UINT8 *) malloc (*FileSize + 0x10000);
    if (NULL == *FileBufferRaw) {
      Error (NULL, 0, 0, "No sufficient memory to allocate!", NULL);
      fclose (FpIn);
      return STATUS_ERROR;
    }
    TempResult = 0x10000 - (UINT32) ((UINTN)*FileBufferRaw & 0x0FFFF);
    *FileData = (UINT8 *)((UINTN)*FileBufferRaw + TempResult);
  } else {
    *FileData = (UINT8 *) malloc (*FileSize);
     if (NULL == *FileData) {
      Error (NULL, 0, 0, "No sufficient memory to allocate!", NULL);
      fclose (FpIn);
      return STATUS_ERROR;
    }
  }
  fseek (FpIn, 0, SEEK_SET);
  TempResult = fread (*FileData, 1, *FileSize, FpIn);
  if (TempResult != *FileSize) {
    Error (NULL, 0, 0, "Read input file error!", NULL);
    if (FileBufferRaw != NULL) {
      free ((VOID *)*FileBufferRaw);
    } else {
      free ((VOID *)*FileData);
    }
    fclose (FpIn);
    return STATUS_ERROR;
  }

  //
  // Close the input FvRecovery.fv file
  //
  fclose (FpIn);

  return STATUS_SUCCESS;
}

/**
    Find next FvHeader in the FileBuffer.

    @param FileBuffer            The start FileBuffer which needs to be searched.
    @param FileLength            The whole File Length.

    @return FvHeader             The FvHeader is found successfully.
    @return NULL                 The FvHeader is not found.
**/
UINT8 *
FindNextFvHeader (
  IN UINT8 *FileBuffer,
  IN UINTN  FileLength
  )
{
  UINT8                       *FileHeader;
  EFI_FIRMWARE_VOLUME_HEADER  *FvHeader;
  UINT16                      FileChecksum;

  FileHeader = FileBuffer;
  for (; (UINTN)FileBuffer < (UINTN)FileHeader + FileLength; FileBuffer += 8) {
    FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)FileBuffer;
    if (FvHeader->Signature == EFI_FVH_SIGNATURE) {
      //
      // potential candidate
      //

      //
      // Check checksum
      //
      if (FvHeader->FvLength > FileLength) {
        continue;
      }
      if (FvHeader->HeaderLength >= FileLength) {
        continue;
      }
      FileChecksum = CalculateChecksum16 ((UINT16 *)FileBuffer, FvHeader->HeaderLength / sizeof (UINT16));
      if (FileChecksum != 0) {
        continue;
      }

      //
      // Check revision and reserved field
      //
#if (PI_SPECIFICATION_VERSION < 0x00010000)
      if ((FvHeader->Revision == EFI_FVH_REVISION) &&
          (FvHeader->Reserved[0] == 0) &&
          (FvHeader->Reserved[1] == 0) &&
          (FvHeader->Reserved[2] == 0) ){
        return FileBuffer;
      }
#else
      if ((FvHeader->Revision == EFI_FVH_PI_REVISION) &&
          (FvHeader->Reserved[0] == 0) ){
        return FileBuffer;
      }
#endif
    }
  }

  return NULL;
}

/**
  Find File with GUID in an FV.

  @param FvBuffer         FV binary buffer.
  @param FvSize           FV size.
  @param Guid             File GUID value to be searched.
  @param FileSize         Guid File size.

  @return FileLocation    Guid File location.
  @return NULL            Guid File is not found.
**/
UINT8  *
FindFileFromFvByGuid (
  IN UINT8     *FvBuffer,
  IN UINT32    FvSize,
  IN EFI_GUID  *Guid,
  OUT UINT32   *FileSize
  )
{
  EFI_FIRMWARE_VOLUME_HEADER  *FvHeader;
  EFI_FFS_FILE_HEADER         *FileHeader;
  UINT64                      FvLength;
  EFI_GUID                    *TempGuid;
  UINT8                       *FixPoint;
  UINTN                       Offset;
  UINTN                       FileLength;
  UINTN                       FileOccupiedSize;

  //
  // Find the FFS file
  //
  FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)FindNextFvHeader (FvBuffer, FvSize);
  if (NULL == FvHeader) {
    return NULL;
  }
  while (TRUE) {
    FvLength         = FvHeader->FvLength;

    //
    // Prepare to walk the FV image
    //
    InitializeFvLib (FvHeader, (UINT32)FvLength);

    FileHeader       = (EFI_FFS_FILE_HEADER *)((UINTN)FvHeader + FvHeader->HeaderLength);
    Offset           = (UINTN) FileHeader - (UINTN) FvHeader;

    while (Offset < FvLength) {
      TempGuid = (EFI_GUID *)&(FileHeader->Name);
      FileLength = (*(UINT32 *)(FileHeader->Size)) & 0x00FFFFFF;
      FileOccupiedSize = GETOCCUPIEDSIZE(FileLength, 8);
      if ((CompareGuid (TempGuid, Guid)) == 0) {
        //
        // Good! Find it.
        //
        FixPoint = ((UINT8 *)FileHeader + sizeof(EFI_FFS_FILE_HEADER));
        //
        // Find the position of file module, the offset
        // between the position and the end of FvRecovery.fv file
        // should not exceed 128kB to prevent reset vector from
        // outside legacy E and F segment
        //
        if ((UINTN)FvHeader + FvLength - (UINTN)FixPoint > 0x20000) {
  //        printf ("WARNING: The position of file module is not in E and F segment!\n");
  //        return NULL;
        }
        *FileSize = FileLength - sizeof(EFI_FFS_FILE_HEADER);
  #if (PI_SPECIFICATION_VERSION < 0x00010000)
        if (FileHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT) {
          *FileSize -= sizeof(EFI_FFS_FILE_TAIL);
        }
  #endif
        return FixPoint;
      }
      FileHeader = (EFI_FFS_FILE_HEADER *)((UINTN)FileHeader + FileOccupiedSize);
      Offset = (UINTN) FileHeader - (UINTN) FvHeader;
    }

    //
    // Not found, check next FV
    //
    if ((UINTN)FvBuffer + FvSize > (UINTN)FvHeader + FvLength) {
      FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)FindNextFvHeader ((UINT8 *)FvHeader + (UINTN)FvLength, (UINTN)FvBuffer + FvSize - ((UINTN)FvHeader + (UINTN)FvLength));
      if (FvHeader == NULL) {
        break;
      }
    } else {
      break;
    }
  }

  //
  // Not found
  //
  return NULL;
}

/**
  Check whether a string is a GUID.

  @param StringData    The String.
  @param Guid          Guid to hold the value

  @return TRUE         StringData is a GUID, and Guid field is filled.
  @return FALSE        StringData is not a GUID.
**/
BOOLEAN
IsGuidData (
  IN CHAR8     *StringData,
  OUT EFI_GUID *Guid
  )
{
  if (strlen (StringData) != strlen ("00000000-0000-0000-0000-000000000000")) {
    return FALSE;
  }
  if ((StringData[8] != '-') ||
      (StringData[13] != '-') ||
      (StringData[18] != '-') ||
      (StringData[23] != '-') ) {
    return FALSE;
  }

  StringToGuid (StringData, Guid);

  return TRUE;
}

/**
  Get FIT entry number and fill global FIT table context, from argument.

  @param argc             Number of command line parameters.
  @param argv             Array of pointers to parameter strings.
  @param FdBuffer         FD binary buffer.
  @param FdSize           FD size.

  @return FitEntryNumber  The FIT entry number.
  @return 0               Argument parse fail.
**/
VOID
CheckOverlap (
  IN UINT32 Address,
  IN UINT32 Size
  )
{
  INTN  Index;

  for (Index = 0; Index < (INTN)gFitTableContext.BiosModuleNumber; Index ++) {
    if ((gFitTableContext.BiosModule[Index].Address <= Address) &&
        ((gFitTableContext.BiosModule[Index].Size - Size) >= (Address - gFitTableContext.BiosModule[Index].Address))) {
      UINT32  TempSize;
      INT32   SubIndex;

      //
      // Found overlap, split BiosModuleEntry
      // Currently only support StartupAcm in 1 BiosModule. It does not support StartupAcm across 2 BiosModule or more.
      //
      if (gFitTableContext.BiosModuleNumber >= MAX_BIOS_MODULE_ENTRY) {
        Error (NULL, 0, 0, "Too many Bios Module!", NULL);
        return ;
      }

      if (Address != gFitTableContext.BiosModule[Index].Address) {
        //
        // Skip the entry whose start address is same as StartupAcm
        //
        gFitTableContext.BiosModule[gFitTableContext.BiosModuleNumber].Type    = FIT_TABLE_TYPE_BIOS_MODULE;
        gFitTableContext.BiosModule[gFitTableContext.BiosModuleNumber].Address = gFitTableContext.BiosModule[Index].Address;
        gFitTableContext.BiosModule[gFitTableContext.BiosModuleNumber].Size    = Address - gFitTableContext.BiosModule[Index].Address;
        gFitTableContext.BiosModuleNumber ++;
        gFitTableContext.FitEntryNumber++;
      }

      TempSize = gFitTableContext.BiosModule[Index].Address + gFitTableContext.BiosModule[Index].Size;
      gFitTableContext.BiosModule[Index].Address = Address + Size;
      gFitTableContext.BiosModule[Index].Size    = TempSize - gFitTableContext.BiosModule[Index].Address;

      if (gFitTableContext.BiosModule[Index].Size == 0) {
        //
        // remove the entry if size is 0
        //
        for (SubIndex = Index; SubIndex < (INTN)gFitTableContext.BiosModuleNumber - 1; SubIndex ++) {
          gFitTableContext.BiosModule[SubIndex].Address = gFitTableContext.BiosModule[SubIndex + 1].Address;
          gFitTableContext.BiosModule[SubIndex].Size    = gFitTableContext.BiosModule[SubIndex + 1].Size;
        }
        gFitTableContext.BiosModuleNumber --;
        gFitTableContext.FitEntryNumber--;
      }
      break;
    }
  }
}

/**
  Get FIT entry number and fill global FIT table context, from argument.

  @param argc             Number of command line parameters.
  @param argv             Array of pointers to parameter strings.
  @param FdBuffer         FD binary buffer.
  @param FdSize           FD size.

  @return FitEntryNumber  The FIT entry number.
  @return 0               Argument parse fail.
**/
UINT8 *
GetMicrocodeBufferFromFv (
  EFI_FIRMWARE_VOLUME_HEADER *FvHeader
  )
{
  UINT8 *MicrocodeBuffer;
  EFI_FFS_FILE_HEADER *FfsHeader;

  MicrocodeBuffer = NULL;
  //
  // Skip FV header + FV extension header + FFS header
  //
  FfsHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *) FvHeader + FvHeader->HeaderLength);
  while ((UINT8 *) FfsHeader < (UINT8 *) FvHeader + FvHeader->FvLength) {
    if (FfsHeader->Type == EFI_FV_FILETYPE_RAW) {
      //
      // Find the first RAW ffs file as Microcode Buffer
      //
      MicrocodeBuffer = (UINT8 *)(FfsHeader + 1);
      break;
    }
    if (GetFfsFileLength (FfsHeader) == 0xFFFFFF) {
      // spare space is found, and exit
      break;
    }
    FfsHeader = (EFI_FFS_FILE_HEADER *) ((UINT8 *) FfsHeader + ((GetFfsFileLength (FfsHeader)+7)&~7));
  }

  return MicrocodeBuffer;
}

/**
  Get FIT entry number and fill global FIT table context, from argument.

  @param argc             Number of command line parameters.
  @param argv             Array of pointers to parameter strings.
  @param FdBuffer         FD binary buffer.
  @param FdSize           FD size.

  @return FitEntryNumber  The FIT entry number.
  @return 0               Argument parse fail.
**/
UINT32
GetFitEntryNumber (
  IN INTN   argc,
  IN CHAR8  **argv,
  IN UINT8  *FdBuffer,
  IN UINT32 FdSize
  )
{
  EFI_GUID  Guid;
  EFI_GUID  MicrocodeFfsGuid;
  INTN      Index;
  UINT8     *FileBuffer;
  UINT32    FileSize;
  UINT32    Type;
  UINT32    SubType;
  UINT8     *MicrocodeFileBuffer;
  UINT8     *MicrocodeFileBufferRaw;
  UINT32    MicrocodeFileSize;
  UINT32    MicrocodeBase;
  UINT32    MicrocodeSize;
  UINT8     *MicrocodeBuffer;
  UINT32    MicrocodeBufferSize;
  UINT8     *Walker;
  UINT32    MicrocodeRegionOffset;
  UINT32    MicrocodeRegionSize;
  UINT32    SlotSize;
  UINT32    MMCRegionOffset;
  UINT8     *MMCBuffer;
  UINT8     *MMCFileBuffer;
  EFI_FIRMWARE_VOLUME_HEADER  *MMCFvHeader;
  STATUS    Status;
  EFI_FIRMWARE_VOLUME_HEADER  *FvHeader;
  UINTN                       FitEntryNumber;
  BOOLEAN                     BiosInfoExist;
  BOOLEAN                     SlotMode;
  BOOLEAN                     SlotModeForce;
  BIOS_INFO_HEADER            *BiosInfo;
  BIOS_INFO_STRUCT            *BiosInfoStruct;
  UINTN                       BiosInfoIndex;

  SlotMode      = FALSE;
  SlotModeForce = FALSE;

  //
  // Init index
  //
  Index = 3;
  if (((strcmp (argv[1], "-D") == 0) ||
       (strcmp (argv[1], "-d") == 0)) ) {
    Index ++;
  }

  //
  // Fill Global Version
  //
  if ((Index + 1 >= argc) ||
      ((strcmp (argv[Index], "-V") != 0) &&
       (strcmp (argv[Index], "-v") != 0)) ) {
    gFitTableContext.GlobalVersion = DEFAULT_FIT_ENTRY_VERSION;
  } else {
    gFitTableContext.GlobalVersion = xtoi (argv[Index + 1]);
    Index += 2;
  }

  //
  // 0. FIT Header
  //
  gFitTableContext.FitEntryNumber = 1;
  if ((Index + 1 >= argc) ||
      ((strcmp (argv[Index], "-F") != 0) &&
       (strcmp (argv[Index], "-f") != 0)) ) {
    //
    // Use default address
    //
    gFitTableContext.FitTablePointerOffset = DEFAULT_FIT_TABLE_POINTER_OFFSET;
  } else {
    //
    // Get offset from parameter
    //
    gFitTableContext.FitTablePointerOffset = xtoi (argv[Index + 1]);
    Index += 2;
  }

  //
  // 0.1 FIT Header 2
  //
  if ((Index + 1 >= argc) ||
      ((strcmp (argv[Index], "-F") != 0) &&
       (strcmp (argv[Index], "-f") != 0)) ) {
    //
    // Bypass
    //
    gFitTableContext.FitTablePointerOffset2 = 0;
  } else {
    //
    // Get offset from parameter
    //
    gFitTableContext.FitTablePointerOffset2 = xtoi (argv[Index + 1]);
    Index += 2;
  }

  //
  // 0.2 FIT Header version
  //
  if ((Index + 1 >= argc) ||
      ((strcmp (argv[Index], "-V") != 0) &&
       (strcmp (argv[Index], "-v") != 0)) ) {
    //
    // Bypass
    //
    gFitTableContext.FitHeaderVersion = gFitTableContext.GlobalVersion;
  } else {
    //
    // Get offset from parameter
    //
    gFitTableContext.FitHeaderVersion = xtoi (argv[Index + 1]);
    Index += 2;
  }

  //
  // 0.3 Microcode alignment
  //
  if ((Index >= argc) ||
      ((strcmp (argv[Index], "-NA") != 0) &&
       (strcmp (argv[Index], "-na") != 0) &&
       (strcmp (argv[Index], "-A") != 0) &&
       (strcmp (argv[Index], "-a") != 0))) {
    //
    // by pass
    //
    gFitTableContext.MicrocodeIsAligned = TRUE;
    gFitTableContext.MicrocodeAlignValue = 0x800;
  } else if ((strcmp (argv[Index], "-NA") == 0) || (strcmp (argv[Index], "-na") == 0)) {
    gFitTableContext.MicrocodeIsAligned = FALSE;
    gFitTableContext.MicrocodeAlignValue = 1;
    Index += 1;
  } else if ((strcmp (argv[Index], "-A") == 0) || (strcmp (argv[Index], "-a") == 0)) {
    gFitTableContext.MicrocodeIsAligned = TRUE;
    //
    // Get alignment from parameter
    //
    gFitTableContext.MicrocodeAlignValue = xtoi (argv[Index + 1]);;
    Index += 2;
  }

  if ((Index >= argc) ||
      ((strcmp (argv[Index], "-REMAP") == 0) ||
       (strcmp (argv[Index], "-remap") == 0)) ) {
    //
    // by pass
    //
    gFitTableContext.TopFlashAddressRemapValue = xtoi (argv[Index + 1]);
    Index += 2;
  } else {
    //
    // no remapping
    //
    gFitTableContext.TopFlashAddressRemapValue = 0x100000000;
  }
  printf ("Top Flash Address Value : 0x%llx\n", (unsigned long long) gFitTableContext.TopFlashAddressRemapValue);
  //
  // 0.4 Clear FIT table related memory
  //
  if ((Index >= argc) ||
      ((strcmp (argv[Index], "-CLEAR") != 0) &&
       (strcmp (argv[Index], "-clear") != 0)) ) {
    //
    // by pass
    //
    gFitTableContext.Clear = FALSE;
  } else {
    //
    // Clear FIT table
    //
    gFitTableContext.Clear = TRUE;
    //
    // Do not parse any more
    //
    return 0;
  }

  //
  // 0.5 SlotSize
  //
  if ((Index + 1 >= argc) ||
      ((strcmp (argv[Index], "-L") != 0) &&
       (strcmp (argv[Index], "-l") != 0) &&
       (strcmp (argv[Index], "-LF") != 0) &&
       (strcmp (argv[Index], "-lf") != 0))) {
    //
    // Bypass
    //
    SlotSize = 0;
  } else {
    SlotSize = xtoi (argv[Index + 1]);

    if (SlotSize == 0 || SlotSize & 0xF) {
      printf ("Invalid slotsize = 0x%x, slot size should not be zero, or start at a non-16-byte aligned linear address!\n", SlotSize);
      return 0;
    }
    if (strcmp (argv[Index], "-LF") == 0 || strcmp (argv[Index], "-lf") == 0) {
      SlotModeForce = TRUE;
      Index += 2;
    } else {
      SlotMode = IsGuidData(argv[Index + 2], &MicrocodeFfsGuid);
      if (!SlotMode) {
        printf ("Need a ffs GUID for search uCode ffs\n");
        return 0;
      }
      Index += 3;
    }
  }

  //
  // 0.6 BiosInfo
  //
  if ((Index + 1 >= argc) ||
      ((strcmp (argv[Index], "-I") != 0) &&
       (strcmp (argv[Index], "-i") != 0)) ) {
    //
    // Bypass
    //
    BiosInfoExist = FALSE;
  } else {
    //
    // Get offset from parameter
    //
    BiosInfoExist = TRUE;
    if (IsGuidData (argv[Index + 1], &Guid)) {
      FileBuffer = FindFileFromFvByGuid (FdBuffer, FdSize, &Guid, &FileSize);
      if (FileBuffer == NULL) {
        Error (NULL, 0, 0, "-I Parameter incorrect, GUID not found!", "%s", argv[Index + 1]);
        // not found
        return 0;
      }
      BiosInfo = (BIOS_INFO_HEADER *)FileBuffer;
      for (BiosInfoIndex = 0; BiosInfoIndex < FileSize; BiosInfoIndex++) {
        if (((BIOS_INFO_HEADER *)(FileBuffer + BiosInfoIndex))->Signature == BIOS_INFO_SIGNATURE) {
          BiosInfo = (BIOS_INFO_HEADER *)(FileBuffer + BiosInfoIndex);
        }
      }
      if (BiosInfo->Signature != BIOS_INFO_SIGNATURE) {
        Error (NULL, 0, 0, "-I Parameter incorrect, Signature Error!", NULL);
        // not found
        return 0;
      }
      BiosInfoStruct = (BIOS_INFO_STRUCT *)(BiosInfo + 1);

      for (BiosInfoIndex = 0; BiosInfoIndex < BiosInfo->EntryCount; BiosInfoIndex++) {
        if ((BiosInfoStruct[BiosInfoIndex].Attributes & BIOS_INFO_STRUCT_ATTRIBUTE_GENERAL_EXCLUDE_FROM_FIT) != 0) {
          continue;
        }
        switch (BiosInfoStruct[BiosInfoIndex].Type) {
        case FIT_TABLE_TYPE_HEADER:
          Error (NULL, 0, 0, "-I Parameter incorrect, Header Type unsupported!", NULL);
          return 0;
        case FIT_TABLE_TYPE_STARTUP_ACM:
          if (gFitTableContext.StartupAcmNumber >= MAX_STARTUP_ACM_ENTRY) {
            Error (NULL, 0, 0, "-I Parameter incorrect, too many StartupAcm!", NULL);
            return 0;
          }
          //
          // NOTE: BIOS INFO structure only support the default FIT entry format.
          //
          gFitTableContext.StartupAcm[gFitTableContext.StartupAcmNumber].Type    = FIT_TABLE_TYPE_STARTUP_ACM;
          gFitTableContext.StartupAcm[gFitTableContext.StartupAcmNumber].Address = (UINT32)BiosInfoStruct[BiosInfoIndex].Address;
          gFitTableContext.StartupAcm[gFitTableContext.StartupAcmNumber].Size    = (UINT32)BiosInfoStruct[BiosInfoIndex].Size;
          gFitTableContext.StartupAcm[gFitTableContext.StartupAcmNumber].Version = BiosInfoStruct[BiosInfoIndex].Version;
          gFitTableContext.StartupAcmNumber ++;
          gFitTableContext.FitEntryNumber ++;
          break;
        case FIT_TABLE_TYPE_DIAGNST_ACM:
          if (gFitTableContext.DiagnstAcm.Type != 0) {
            Error (NULL, 0, 0, "-U Parameter incorrect, Duplicated DiagnosticsAcm!", NULL);
            return 0;
          }
          gFitTableContext.DiagnstAcm.Type    = FIT_TABLE_TYPE_DIAGNST_ACM;
          gFitTableContext.DiagnstAcm.Address = (UINT32)BiosInfoStruct[BiosInfoIndex].Address;
          gFitTableContext.DiagnstAcm.Size    = 0;
          gFitTableContext.DiagnstAcmVersion  = DEFAULT_FIT_ENTRY_VERSION;
          gFitTableContext.FitEntryNumber ++;
          break;
        case FIT_TABLE_TYPE_PROT_BOOT_POLICY:
          gFitTableContext.ProtBootPolicy.Type     = FIT_TABLE_TYPE_PROT_BOOT_POLICY;
          gFitTableContext.ProtBootPolicy.Address  = (UINT32)BiosInfoStruct[BiosInfoIndex].Address;
          gFitTableContext.ProtBootPolicy.Size     = (UINT32)BiosInfoStruct[BiosInfoIndex].Size;
          gFitTableContext.ProtBootPolicy.Version  = DEFAULT_FIT_ENTRY_VERSION;
          gFitTableContext.FitEntryNumber ++;
          break;
        case FIT_TABLE_TYPE_MMC_FW:
          if (gFitTableContext.MmcFwNumber >= MAX_MMCFW_MODULE_ENTRY) {
            Error (NULL, 0, 0, "-I Parameter incorrect, Too many Bios Mmc Fw!", NULL);
            return 0;
          }
          MMCRegionOffset = (UINT32)BiosInfoStruct[BiosInfoIndex].Address;
          if (MMCRegionOffset == 0) {
            continue;
          }
          MMCBuffer = FLASH_TO_MEMORY (MMCRegionOffset, FdBuffer, FdSize);
          MMCFvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)MMCBuffer;
          if (MMCFvHeader->Signature == EFI_FVH_SIGNATURE) {
            MMCFileBuffer = GetMicrocodeBufferFromFv (MMCFvHeader);
          } else {
            MMCFileBuffer = MMCBuffer;
          }
          gFitTableContext.MmcFw[gFitTableContext.MmcFwNumber].Type     = BiosInfoStruct[BiosInfoIndex].Type;
          gFitTableContext.MmcFw[gFitTableContext.MmcFwNumber].Address  = (UINT32)BiosInfoStruct[BiosInfoIndex].Address + (UINT32)((UINTN) MMCFileBuffer - (UINTN) MMCBuffer);
          gFitTableContext.MmcFw[gFitTableContext.MmcFwNumber].Size     = (UINT32)BiosInfoStruct[BiosInfoIndex].Size;
          gFitTableContext.MmcFw[gFitTableContext.MmcFwNumber].Version  = DEFAULT_FIT_ENTRY_VERSION;
          gFitTableContext.MmcFwNumber++;
          gFitTableContext.FitEntryNumber++;
          break;
        case FIT_TABLE_TYPE_BIOS_MODULE:
          if ((BiosInfoStruct[BiosInfoIndex].Attributes & BIOS_INFO_STRUCT_ATTRIBUTE_BIOS_POST_IBB) != 0) {
            continue;
          }
          if (gFitTableContext.BiosModuleNumber >= MAX_BIOS_MODULE_ENTRY) {
            Error (NULL, 0, 0, "-I Parameter incorrect, Too many Bios Module!", NULL);
            return 0;
          }
          gFitTableContext.BiosModule[gFitTableContext.BiosModuleNumber].Type    = FIT_TABLE_TYPE_BIOS_MODULE;
          gFitTableContext.BiosModule[gFitTableContext.BiosModuleNumber].Address = (UINT32)BiosInfoStruct[BiosInfoIndex].Address;
          gFitTableContext.BiosModule[gFitTableContext.BiosModuleNumber].Size    = (UINT32)BiosInfoStruct[BiosInfoIndex].Size;
          gFitTableContext.BiosModuleVersion = BiosInfoStruct[BiosInfoIndex].Version;
          gFitTableContext.BiosModuleNumber ++;
          gFitTableContext.FitEntryNumber ++;
          break;
        case FIT_TABLE_TYPE_MICROCODE:
          if ((BiosInfoStruct[BiosInfoIndex].Attributes & BIOS_INFO_STRUCT_ATTRIBUTE_MICROCODE_WHOLE_REGION) == 0) {
            if (gFitTableContext.MicrocodeNumber >= MAX_MICROCODE_ENTRY) {
              Error (NULL, 0, 0, "-I Parameter incorrect, Too many Microcode!", NULL);
              return 0;
            }
            gFitTableContext.Microcode[gFitTableContext.MicrocodeNumber].Type    = FIT_TABLE_TYPE_MICROCODE;
            gFitTableContext.Microcode[gFitTableContext.MicrocodeNumber].Address = (UINT32)BiosInfoStruct[BiosInfoIndex].Address;
            gFitTableContext.Microcode[gFitTableContext.MicrocodeNumber].Size    = (UINT32)BiosInfoStruct[BiosInfoIndex].Size;
            gFitTableContext.MicrocodeVersion = BiosInfoStruct[BiosInfoIndex].Version;
            gFitTableContext.MicrocodeNumber++;
            gFitTableContext.FitEntryNumber++;
          } else {
            MicrocodeRegionOffset = (UINT32)BiosInfoStruct[BiosInfoIndex].Address;
            MicrocodeRegionSize   = (UINT32)BiosInfoStruct[BiosInfoIndex].Size;
            if (MicrocodeRegionOffset == 0) {
              Error (NULL, 0, 0, "-I Parameter incorrect, MicrocodeRegionOffset is 0", NULL);
              return 0;
            }
            if (MicrocodeRegionSize == 0) {
              Error (NULL, 0, 0, "-I Parameter incorrect, MicrocodeRegionSize is 0", NULL);
              return 0;
            }
            if (MicrocodeRegionSize > FdSize) {
              Error (NULL, 0, 0, "-I Parameter incorrect, MicrocodeRegionSize too large", NULL);
              return 0;
            }

            MicrocodeFileBuffer = FLASH_TO_MEMORY (MicrocodeRegionOffset, FdBuffer, FdSize);
            MicrocodeFileSize = MicrocodeRegionSize;
            MicrocodeBase = MicrocodeRegionOffset;

            FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)MicrocodeFileBuffer;
            if (FvHeader->Signature == EFI_FVH_SIGNATURE) {
              MicrocodeBuffer = GetMicrocodeBufferFromFv (FvHeader);
            } else {
              MicrocodeBuffer = MicrocodeFileBuffer;
            }

            if (SlotMode) {
              MicrocodeBuffer = FindFileFromFvByGuid(MicrocodeFileBuffer, MicrocodeFileSize, &MicrocodeFfsGuid, &MicrocodeBufferSize);
              if (MicrocodeBuffer == NULL) {
                printf ("-L Parameter incorrect, GUID not found\n");
                // not found
                return 0;
              }
            }

            while ((UINT32)(MicrocodeBuffer - MicrocodeFileBuffer) < MicrocodeFileSize) {
              if (*(UINT32 *)(MicrocodeBuffer) != 0x1) { // HeaderVersion
                break;
              }
              if (*(UINT32 *)(MicrocodeBuffer + 20) != 0x1) { // LoaderVersion
                break;
              }
              if (*(UINT32 *)(MicrocodeBuffer + 28) == 0) { // DataSize
                MicrocodeSize = 2048;
              } else {
                //
                // MCU might be put at 2KB alignment, if so, we need to adjust the size as 2KB alignment.
                //
                if (gFitTableContext.MicrocodeIsAligned) {
                  if (gFitTableContext.MicrocodeAlignValue & 0xF) {
                    printf ("-A Parameter incorrect, Microcode data must start at a 16-byte aligned linear address!\n");
                    return 0;
                  }
                  MicrocodeSize = ROUNDUP (*(UINT32 *)(MicrocodeBuffer + 32), gFitTableContext.MicrocodeAlignValue);
                } else {
                  MicrocodeSize = (*(UINT32 *)(MicrocodeBuffer + 32));
                }
              }

              //
              // Add Microcode
              //
              if (gFitTableContext.MicrocodeNumber >= MAX_MICROCODE_ENTRY) {
                printf ("-I Parameter incorrect, Too many Microcode!\n");
                return 0;
              }
              gFitTableContext.Microcode[gFitTableContext.MicrocodeNumber].Type = FIT_TABLE_TYPE_MICROCODE;
              gFitTableContext.Microcode[gFitTableContext.MicrocodeNumber].Address = MicrocodeBase + (UINT32)((UINTN) MicrocodeBuffer - (UINTN) MicrocodeFileBuffer);
              gFitTableContext.Microcode[gFitTableContext.MicrocodeNumber].Size = MicrocodeSize;
              gFitTableContext.MicrocodeNumber++;
              gFitTableContext.FitEntryNumber++;

              if (SlotSize != 0) {
                if (SlotSize < MicrocodeSize) {
                  printf ("Parameter incorrect, Slot size: %x is too small for Microcode size: %x!\n", SlotSize, MicrocodeSize);
                  return 0;
                }
                MicrocodeBuffer += SlotSize;
              } else {
                MicrocodeBuffer += MicrocodeSize;
              }
            }

            ///
            /// Check the remaining buffer
            ///
            if (((UINT32)(MicrocodeBuffer - MicrocodeFileBuffer) < MicrocodeFileSize) && (SlotMode || SlotModeForce)) {
              for (Walker = MicrocodeBuffer; Walker < MicrocodeFileBuffer + MicrocodeFileSize; Walker++) {
                if (*Walker != 0xFF) {
                  printf ("Error: detect non-spare space after uCode array, please check uCode array!\n");
                  return 0;
                }
              }

              ///
              /// Split the spare space as empty buffer for save uCode patch.
              ///
              while (MicrocodeBuffer + SlotSize <= MicrocodeFileBuffer + MicrocodeFileSize) {
                gFitTableContext.Microcode[gFitTableContext.MicrocodeNumber].Type = FIT_TABLE_TYPE_MICROCODE;
                gFitTableContext.Microcode[gFitTableContext.MicrocodeNumber].Address = MicrocodeBase + (UINT32)((UINTN) MicrocodeBuffer - (UINTN) MicrocodeFileBuffer);
                gFitTableContext.MicrocodeNumber++;
                gFitTableContext.FitEntryNumber++;

                MicrocodeBuffer += SlotSize;
              }
            }
          }
          break;
        case FIT_TABLE_TYPE_TPM_POLICY:
        case FIT_TABLE_TYPE_BIOS_POLICY:
        case FIT_TABLE_TYPE_TXT_POLICY:
        case FIT_TABLE_TYPE_KEY_MANIFEST:
        case FIT_TABLE_TYPE_BOOT_POLICY_MANIFEST:
        case FIT_TABLE_TYPE_FSP_BOOT_MANIFEST:
        case FIT_TABLE_TYPE_CSE_SECURE_BOOT:
        default :
          if (BiosInfoStruct[BiosInfoIndex].Version != 0) {
            if (gFitTableContext.OptionalModuleNumber >= MAX_OPTIONAL_ENTRY) {
              Error (NULL, 0, 0, "-I Parameter incorrect, Too many Optional Module!", NULL);
              return 0;
            }
            gFitTableContext.OptionalModule[gFitTableContext.OptionalModuleNumber].Type    = BiosInfoStruct[BiosInfoIndex].Type;
            gFitTableContext.OptionalModule[gFitTableContext.OptionalModuleNumber].Address = (UINT32)BiosInfoStruct[BiosInfoIndex].Address;
            gFitTableContext.OptionalModule[gFitTableContext.OptionalModuleNumber].Size    = (UINT32)BiosInfoStruct[BiosInfoIndex].Size;
            gFitTableContext.OptionalModule[gFitTableContext.OptionalModuleNumber].Version = BiosInfoStruct[BiosInfoIndex].Version;
            gFitTableContext.OptionalModuleNumber++;
            gFitTableContext.FitEntryNumber++;
          } else {
            if (gFitTableContext.PortModuleNumber >= MAX_PORT_ENTRY) {
              Error (NULL, 0, 0, "-I Parameter incorrect, Too many Port Module!", NULL);
              return 0;
            }
            gFitTableContext.PortModule[gFitTableContext.PortModuleNumber].Type    = BiosInfoStruct[BiosInfoIndex].Type;
            gFitTableContext.PortModule[gFitTableContext.PortModuleNumber].Address = (UINT32)BiosInfoStruct[BiosInfoIndex].Address;
            gFitTableContext.PortModule[gFitTableContext.PortModuleNumber].Size    = (UINT32)(BiosInfoStruct[BiosInfoIndex].Address >> 32);
            gFitTableContext.PortModule[gFitTableContext.PortModuleNumber].Version = BiosInfoStruct[BiosInfoIndex].Version;
            gFitTableContext.PortModuleNumber++;
            gFitTableContext.FitEntryNumber++;
          }
          break;
        }
      }

    } else {
      Error (NULL, 0, 0, "-I Parameter incorrect, expect GUID!", NULL);
      return 0;
    }
    Index += 2;
  }

  //
  // 1. StartupAcm
  //
  while (TRUE) {
    if ((Index + 1 >= argc) ||
        ((strcmp (argv[Index], "-S") != 0) &&
         (strcmp (argv[Index], "-s") != 0)) ) {
      if (gFitTableContext.StartupAcmNumber == 0) {
        printf ("-S not found. WARNING!\n");
      }
//      Error (NULL, 0, 0, "-S Parameter incorrect, expect -S!", NULL);
//      return 0;
      break;
    }
    if (IsGuidData (argv[Index + 1], &Guid)) {
      FileBuffer = FindFileFromFvByGuid (FdBuffer, FdSize, &Guid, &FileSize);
      if (FileBuffer == NULL) {
        Error (NULL, 0, 0, "-S Parameter incorrect, GUID not found!", "%s", argv[Index + 1]);
        // not found
        return 0;
      }
      gFitTableContext.StartupAcmFvSize = FdSize;
      FileBuffer = (UINT8 *)MEMORY_TO_FLASH(FileBuffer, FdBuffer, FdSize);
      Index += 2;
    } else {
      if (Index + 2 >= argc) {
        Error (NULL, 0, 0, "-S Parameter incorrect, expect Address Size!", NULL);
        return 0;
      }
      FileBuffer = (UINT8 *) (UINTN) xtoi (argv[Index + 1]);
      FileSize = xtoi (argv[Index + 2]);
      Index += 3;
    }
    if (gFitTableContext.StartupAcmNumber >= MAX_STARTUP_ACM_ENTRY) {
      Error (NULL, 0, 0, "-S Parameter incorrect, too many StartupAcm!", NULL);
      return 0;
    }
    gFitTableContext.StartupAcm[gFitTableContext.StartupAcmNumber].Type = FIT_TABLE_TYPE_STARTUP_ACM;
    gFitTableContext.StartupAcm[gFitTableContext.StartupAcmNumber].Address = (UINT32) (UINTN) FileBuffer;
    gFitTableContext.StartupAcm[gFitTableContext.StartupAcmNumber].Size = FileSize;

    //
    // 1.1 Support 0x200 StartupAcm Information
    //     With the -I parameter should assign the type 2 entry with 0x200 version format
    //
    if ((Index + 1 >= argc) ||
        ((strcmp (argv[Index], "-I") != 0) &&
         (strcmp (argv[Index], "-i") != 0)) ) {
      //
      // Bypass
      //
      gFitTableContext.StartupAcm[gFitTableContext.StartupAcmNumber].Version = gFitTableContext.GlobalVersion;
    } else {
      if (Index + 2 >= argc) {
        //
        // Should get two input value, but not sufficient
        //
        Error (NULL, 0, 0, "-I Parameter incorrect, Require two inputs value!", NULL);
        return 0;
      } else {
        //
        // With the -I parameter should assign the type 2 entry version as 0x200 format
        //
        gFitTableContext.StartupAcm[gFitTableContext.StartupAcmNumber].Version = STARTUP_ACM_FIT_ENTRY_200_VERSION;
        gFitTableContext.StartupAcm[gFitTableContext.StartupAcmNumber].FMS = (UINT32)xtoi (argv[Index + 1]);
        gFitTableContext.StartupAcm[gFitTableContext.StartupAcmNumber].FMSMask = (UINT32)xtoi (argv[Index + 2]);

        Index += 3;
      }
    }

    //
    // 1.2 StartupAcm version
    //
    if ((Index + 1 >= argc) ||
        ((strcmp (argv[Index], "-V") != 0) &&
         (strcmp (argv[Index], "-v") != 0)) ) {
      //
      // Bypass
      //
    } else {
      //
      // Get offset from parameter
      //
      gFitTableContext.StartupAcm[gFitTableContext.StartupAcmNumber].Version = gFitTableContext.GlobalVersion;
      gFitTableContext.StartupAcm[gFitTableContext.StartupAcmNumber].Version = xtoi (argv[Index + 1]);
      Index += 2;
    }

    gFitTableContext.StartupAcmNumber ++;
    gFitTableContext.FitEntryNumber ++;
  };

  //
  // 1.5. DiagnosticsAcm
  //
  do {
    if ((Index + 1 >= argc) ||
        ((strcmp (argv[Index], "-U") != 0) &&
         (strcmp (argv[Index], "-u") != 0)) ) {
      if (BiosInfoExist && (gFitTableContext.DiagnstAcm.Type == FIT_TABLE_TYPE_DIAGNST_ACM)) {
        break;
      }
      break;
    }
    if (IsGuidData (argv[Index + 1], &Guid)) {
      FileBuffer = FindFileFromFvByGuid (FdBuffer, FdSize, &Guid, &FileSize);
      if (FileBuffer == NULL) {
        Error (NULL, 0, 0, "-U Parameter incorrect, GUID not found!", "%s", argv[Index + 1]);
        return 0;
      }
      FileBuffer = (UINT8 *)MEMORY_TO_FLASH (FileBuffer, FdBuffer, FdSize);
      Index += 2;
    } else {
      FileBuffer = (UINT8 *) (UINTN) xtoi (argv[Index + 1]);
      Index += 2;
    }
    if (gFitTableContext.DiagnstAcm.Type != 0) {
      Error (NULL, 0, 0, "-U Parameter incorrect, Duplicated DiagnosticsAcm!", NULL);
      return 0;
    }
    gFitTableContext.DiagnstAcm.Type = FIT_TABLE_TYPE_DIAGNST_ACM;
    gFitTableContext.DiagnstAcm.Address = (UINT32) (UINTN) FileBuffer;
    gFitTableContext.DiagnstAcm.Size = 0;
    gFitTableContext.FitEntryNumber ++;
    gFitTableContext.DiagnstAcmVersion = DEFAULT_FIT_ENTRY_VERSION;
  } while (FALSE);

  // 2. BiosModule
  //
  do {
    if ((Index + 2 >= argc) ||
        ((strcmp (argv[Index], "-B") != 0) &&
         (strcmp (argv[Index], "-b") != 0)) ) {
      if (BiosInfoExist && (gFitTableContext.BiosModuleNumber != 0)) {
        break;
      }
//      Error (NULL, 0, 0, "-B Parameter incorrect, expect -B!", NULL);
//      return 0;
      printf ("-B not found. WARNING!\n");
      break;
    }

    FileBuffer = (UINT8 *) (UINTN) xtoi (argv[Index + 1]);
    FileSize = xtoi (argv[Index + 2]);
    gFitTableContext.BiosModule[gFitTableContext.BiosModuleNumber].Type = FIT_TABLE_TYPE_BIOS_MODULE;
    gFitTableContext.BiosModule[gFitTableContext.BiosModuleNumber].Address = (UINT32) (UINTN) FileBuffer;
    gFitTableContext.BiosModule[gFitTableContext.BiosModuleNumber].Size = FileSize;
    gFitTableContext.BiosModuleNumber ++;
    gFitTableContext.FitEntryNumber ++;

    while (TRUE) {
      Index += 3;
      if (Index + 2 >= argc) {
        break;
      }
      if ((strcmp (argv[Index], "-B") != 0) &&
          (strcmp (argv[Index], "-b") != 0) ) {
        break;
      }
      if (gFitTableContext.BiosModuleNumber >= MAX_BIOS_MODULE_ENTRY) {
        Error (NULL, 0, 0, "-B Parameter incorrect, Too many Bios Module!", NULL);
        return 0;
      }
      FileBuffer = (UINT8 *) (UINTN) xtoi (argv[Index + 1]);
      FileSize = xtoi (argv[Index + 2]);
      gFitTableContext.BiosModule[gFitTableContext.BiosModuleNumber].Type = FIT_TABLE_TYPE_BIOS_MODULE;
      gFitTableContext.BiosModule[gFitTableContext.BiosModuleNumber].Address = (UINT32) (UINTN) FileBuffer;
      gFitTableContext.BiosModule[gFitTableContext.BiosModuleNumber].Size = FileSize;
      gFitTableContext.BiosModuleNumber ++;
      gFitTableContext.FitEntryNumber++;
    }

    //
    // 2.1 BiosModule version
    //
    if ((Index + 1 >= argc) ||
        ((strcmp (argv[Index], "-V") != 0) &&
         (strcmp (argv[Index], "-v") != 0)) ) {
      //
      // Bypass
      //
      gFitTableContext.BiosModuleVersion = gFitTableContext.GlobalVersion;
    } else {
      //
      // Get offset from parameter
      //
      gFitTableContext.BiosModuleVersion = xtoi (argv[Index + 1]);
      Index += 2;
    }
  } while (FALSE);

  //
  // 3. Microcode
  //
  while (TRUE) {
    if (Index + 1 >= argc) {
      break;
    }
    if ((strcmp (argv[Index], "-M") != 0) &&
        (strcmp (argv[Index], "-m") != 0) ) {
      break;
    }
    if (IsGuidData (argv[Index + 2], &Guid)) {
      Error (NULL, 0, 0, "-M Parameter incorrect, GUID unsupported!", NULL);
      return 0;
    } else {
      if (Index + 2 >= argc) {
        break;
      }
      FileBuffer = (UINT8 *) (UINTN) xtoi (argv[Index + 1]);
      FileSize = xtoi (argv[Index + 2]);
      Index += 3;
    }
    if (gFitTableContext.MicrocodeNumber >= MAX_MICROCODE_ENTRY) {
      Error (NULL, 0, 0, "-M Parameter incorrect, Too many Microcode!", NULL);
      return 0;
    }
    gFitTableContext.Microcode[gFitTableContext.MicrocodeNumber].Type = FIT_TABLE_TYPE_MICROCODE;
    gFitTableContext.Microcode[gFitTableContext.MicrocodeNumber].Address = (UINT32) (UINTN) FileBuffer;
    gFitTableContext.Microcode[gFitTableContext.MicrocodeNumber].Size = FileSize;
    gFitTableContext.MicrocodeNumber++;
    gFitTableContext.FitEntryNumber++;
  }

  //
  // 3.1 MicrocodeFv
  //
  while (TRUE) {
    if (Index + 1 >= argc) {
      break;
    }
    if ((strcmp (argv[Index], "-U") != 0) &&
        (strcmp (argv[Index], "-u") != 0) ) {
      break;
    }
    //
    // Get Fv
    //
    if (IsGuidData (argv[Index + 1], &Guid)) {
      MicrocodeFileBuffer = FindFileFromFvByGuid (FdBuffer, FdSize, &Guid, &MicrocodeFileSize);
      if (MicrocodeFileBuffer == NULL) {
        Error (NULL, 0, 0, "-U Parameter incorrect, GUID not found!", "%s", argv[Index + 1]);
        // not found
        return 0;
      }
      Index += 2;

      MicrocodeBuffer = MicrocodeFileBuffer;
      MicrocodeFileBufferRaw = NULL;
      MicrocodeRegionOffset = MEMORY_TO_FLASH (MicrocodeFileBuffer, FdBuffer, FdSize);
      MicrocodeRegionSize   = 0;
      MicrocodeBase = MicrocodeRegionOffset;

    } else {
      if (Index + 2 >= argc) {
        break;
      }
      Status = ReadInputFile (argv[Index + 1], &MicrocodeFileBuffer, &MicrocodeFileSize, &MicrocodeFileBufferRaw);
      if (Status != STATUS_SUCCESS) {
        MicrocodeRegionOffset = xtoi (argv[Index + 1]);
        MicrocodeRegionSize   = xtoi (argv[Index + 2]);

        if (MicrocodeRegionOffset == 0) {
          Error (NULL, 0, 0, "-U Parameter incorrect, MicrocodeRegionOffset is 0, or unable to open file", "%s", argv[Index + 1]);
          return 0;
        }
        if (MicrocodeRegionSize == 0) {
          Error (NULL, 0, 0, "-U Parameter incorrect, MicrocodeRegionSize is 0", NULL);
          return 0;
        }
        if (MicrocodeRegionSize > FdSize) {
          Error (NULL, 0, 0, "-U Parameter incorrect, MicrocodeRegionSize too large", NULL);
          return 0;
        }

        Index += 3;

        MicrocodeFileBufferRaw = NULL;
        MicrocodeFileBuffer = FLASH_TO_MEMORY (MicrocodeRegionOffset, FdBuffer, FdSize);
        MicrocodeFileSize = MicrocodeRegionSize;
        MicrocodeBase = MicrocodeRegionOffset;

        FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)MicrocodeFileBuffer;
        if (FvHeader->Signature == EFI_FVH_SIGNATURE) {
          MicrocodeBuffer = GetMicrocodeBufferFromFv (FvHeader);
        } else {
          MicrocodeBuffer = MicrocodeFileBuffer;
        }
      } else {
        MicrocodeBase = xtoi (argv[Index + 2]);
        Index += 3;
        MicrocodeRegionOffset = 0;
        MicrocodeRegionSize   = 0;

        FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)MicrocodeFileBuffer;
        if (FvHeader->Signature == EFI_FVH_SIGNATURE) {
          MicrocodeBuffer = GetMicrocodeBufferFromFv (FvHeader);
        } else {
          MicrocodeBuffer = MicrocodeFileBuffer;
        }
      }
    }
    while ((UINT32)(MicrocodeBuffer - MicrocodeFileBuffer) < MicrocodeFileSize) {
      if (*(UINT32 *)(MicrocodeBuffer) != 0x1) { // HeaderVersion
        break;
      }
      if (*(UINT32 *)(MicrocodeBuffer + 20) != 0x1) { // LoaderVersion
        break;
      }
      if (*(UINT32 *)(MicrocodeBuffer + 28) == 0) { // DataSize
        MicrocodeSize = 2048;
      } else {
        //
        // MCU might be put at 2KB alignment, if so, we need to adjust the size as 2KB alignment.
        //
        if (gFitTableContext.MicrocodeIsAligned) {
          MicrocodeSize = (*(UINT32 *)(MicrocodeBuffer + 32) + (gFitTableContext.MicrocodeAlignValue - 1)) & ~(gFitTableContext.MicrocodeAlignValue - 1);
        } else {
          MicrocodeSize = (*(UINT32 *)(MicrocodeBuffer + 32));
        }
      }

      //
      // Add Microcode
      //
      if (gFitTableContext.MicrocodeNumber >= MAX_MICROCODE_ENTRY) {
        printf ("-U Parameter incorrect, Too many Microcode!\n");
        return 0;
      }
      gFitTableContext.Microcode[gFitTableContext.MicrocodeNumber].Type = FIT_TABLE_TYPE_MICROCODE;
      gFitTableContext.Microcode[gFitTableContext.MicrocodeNumber].Address = MicrocodeBase + (UINT32)((UINTN) MicrocodeBuffer - (UINTN) MicrocodeFileBuffer);
      gFitTableContext.Microcode[gFitTableContext.MicrocodeNumber].Size = MicrocodeSize;
      gFitTableContext.MicrocodeNumber++;
      gFitTableContext.FitEntryNumber++;

      MicrocodeBuffer += MicrocodeSize;
    }

    if (MicrocodeFileBufferRaw != NULL) {
      free ((VOID *)MicrocodeFileBufferRaw);
      MicrocodeFileBufferRaw = NULL;
    }
  }

  //
  // 3.3 Microcode version
  //
  if ((Index + 1 >= argc) ||
      ((strcmp (argv[Index], "-V") != 0) &&
       (strcmp (argv[Index], "-v") != 0)) ) {
    //
    // Bypass
    //
    gFitTableContext.MicrocodeVersion = gFitTableContext.GlobalVersion;
  } else {
    //
    // Get offset from parameter
    //
    gFitTableContext.MicrocodeVersion = xtoi (argv[Index + 1]);
    Index += 2;
  }

  //
  // 4. Optional type
  //
  while (TRUE) {
    if (Index + 2 >= argc) {
      break;
    }
    if ((strcmp (argv[Index], "-O") != 0) &&
        (strcmp (argv[Index], "-o") != 0) ) {
      break;
    }
    Type = xtoi (argv[Index + 1]);
    //
    // 1st, try CSE entry sub-type
    //
    SubType = 0;
    if (Type == FIT_TABLE_TYPE_CSE_SECURE_BOOT) {
      if (Index + 3 >= argc) {
        break;
      }
      SubType = xtoi (argv[Index + 2]);
      //
      // try file
      //
      if (SubType != FIT_TABLE_SUBTYPE_FIT_PATCH_MANIFEST && SubType != FIT_TABLE_SUBTYPE_ACM_MANIFEST) {
        Error (NULL, 0, 0, "-O Parameter incorrect, SubType unsupported!", NULL);
        return 0;
      }
      Status = ReadInputFile (argv[Index + 3], &FileBuffer, &FileSize, NULL);
      if (Status == STATUS_SUCCESS) {
        if (FileSize >= 0x80000000) {
          Error (NULL, 0, 0, "-O Parameter incorrect, FileSize too large!", NULL);
          free (FileBuffer);
          return 0;
        }
        //
        // Set the most significant bit
        // It means the data in memory, not in flash yet.
        // Assume the file size should < 2G.
        //
        FileSize |= 0x80000000;
        Index += 4;
      } else {
        if (Status == STATUS_WARNING) {
          Error (NULL, 0, 0, "-O Parameter incorrect, Unable to open file", argv[Index + 3]);
        }
        return 0;
      }
    } else {
      //
      // 2nd, try GUID
      //
      if (IsGuidData (argv[Index + 2], &Guid)) {
        FileBuffer = FindFileFromFvByGuid (FdBuffer, FdSize, &Guid, &FileSize);
        if (FileBuffer == NULL) {
          Error (NULL, 0, 0, "-O Parameter incorrect, GUID not found!", "%s", argv[Index + 2]);
          // not found
          return 0;
        }
        if (FileSize >= 0x80000000) {
          Error (NULL, 0, 0, "-O Parameter incorrect, FileSize too large!", NULL);
          return 0;
        }
      FileBuffer = (UINT8 *)MEMORY_TO_FLASH (FileBuffer, FdBuffer, FdSize);
        Index += 3;
      } else {
        //
        // 3rd, try file
        //
        Status = ReadInputFile (argv[Index + 2], &FileBuffer, &FileSize, NULL);
        if (Status == STATUS_SUCCESS) {
          if (FileSize >= 0x80000000) {
            Error (NULL, 0, 0, "-O Parameter incorrect, FileSize too large!", NULL);
            free (FileBuffer);
            return 0;
          }
          //
          // Set the most significant bit
          // It means the data in memory, not in flash yet.
          // Assume the file size should < 2G.
          //
          FileSize |= 0x80000000;
          Index += 3;
        } else {
          //
          // 4th, try <RESERVE, Length>
          //
          if (Index + 3 >= argc) {
            break;
          }
          if ((strcmp (argv[Index + 2], "RESERVE") == 0) ||
              (strcmp (argv[Index + 2], "reserve") == 0)) {
            FileSize = xtoi (argv[Index + 3]);
            if (FileSize >= 0x80000000) {
              Error (NULL, 0, 0, "-O Parameter incorrect, FileSize too large!", NULL);
              return 0;
            }
            FileBuffer = malloc (FileSize);
            if (FileBuffer == NULL) {
              Error (NULL, 0, 0, "No sufficient memory to allocate!", NULL);
              return 0;
            }
            SetMem (FileBuffer, FileSize, 0xFF);
            //
            // Set the most significant bit
            // It means the data in memory, not in flash yet.
            // Assume the file size should < 2G.
            //
            FileSize |= 0x80000000;
            Index += 4;
          } else {
            //
            // 5th, try <Address, Length>
            //
            if (Index + 3 >= argc) {
              break;
            }
            FileBuffer = (UINT8 *) (UINTN) xtoi (argv[Index + 2]);
            FileSize = xtoi (argv[Index + 3]);
            if (FileSize >= 0x80000000) {
              Error (NULL, 0, 0, "-O Parameter incorrect, FileSize too large!", NULL);
              return 0;
            }
            Index += 4;
          }
        }
      }
    }
    if (gFitTableContext.OptionalModuleNumber >= MAX_OPTIONAL_ENTRY) {
      Error (NULL, 0, 0, "-O Parameter incorrect, Too many Optional Module!", NULL);
      free (FileBuffer);
      return 0;
    }
    gFitTableContext.OptionalModule[gFitTableContext.OptionalModuleNumber].Type = Type;
    if (gFitTableContext.OptionalModule[gFitTableContext.OptionalModuleNumber].Type == FIT_TABLE_TYPE_CSE_SECURE_BOOT) {
      gFitTableContext.OptionalModule[gFitTableContext.OptionalModuleNumber].SubType = SubType;
    }
    gFitTableContext.OptionalModule[gFitTableContext.OptionalModuleNumber].Address = (UINT32) (UINTN) FileBuffer;
    gFitTableContext.OptionalModule[gFitTableContext.OptionalModuleNumber].Buffer = FileBuffer;
    gFitTableContext.OptionalModule[gFitTableContext.OptionalModuleNumber].Size = FileSize;

    //
    // 4.1 Optional Module version
    //
    if ((Index + 1 >= argc) ||
        ((strcmp (argv[Index], "-V") != 0) &&
         (strcmp (argv[Index], "-v") != 0)) ) {
      //
      // Bypass
      //
      gFitTableContext.OptionalModule[gFitTableContext.OptionalModuleNumber].Version = gFitTableContext.GlobalVersion;
    } else {
      //
      // Get offset from parameter
      //
      gFitTableContext.OptionalModule[gFitTableContext.OptionalModuleNumber].Version = xtoi (argv[Index + 1]);
      Index += 2;
    }

    gFitTableContext.OptionalModuleNumber ++;
    gFitTableContext.FitEntryNumber++;
  }

  //
  // 5. Port type
  //
  while (TRUE) {
    if (Index + 6 >= argc) {
      break;
    }
    if ((strcmp (argv[Index], "-P") != 0) &&
        (strcmp (argv[Index], "-p") != 0) ) {
      break;
    }

    Type = xtoi (argv[Index + 1]);
    if (gFitTableContext.PortModuleNumber >= MAX_PORT_ENTRY) {
      printf ("-P Parameter incorrect, Too many Port Module!\n");
      return 0;
    }

    gFitTableContext.PortModule[gFitTableContext.PortModuleNumber].Type = Type;
    gFitTableContext.PortModule[gFitTableContext.PortModuleNumber].Address = (UINT16)xtoi (argv[Index + 2]) + ((UINT16)xtoi (argv[Index + 3]) << 16);
    gFitTableContext.PortModule[gFitTableContext.PortModuleNumber].Size = (UINT8)xtoi (argv[Index + 4]) + ((UINT8)xtoi (argv[Index + 5]) << 8) + ((UINT16)xtoi (argv[Index + 6]) << 16);
    Index += 7;

    //
    // 5.1 Port Module version
    //
    if ((Index + 1 >= argc) ||
        ((strcmp (argv[Index], "-V") != 0) &&
         (strcmp (argv[Index], "-v") != 0)) ) {
      //
      // Bypass
      //
      gFitTableContext.PortModule[gFitTableContext.PortModuleNumber].Version = 0;
    } else {
      //
      // Get offset from parameter
      //
      gFitTableContext.PortModule[gFitTableContext.PortModuleNumber].Version = xtoi(argv[Index + 1]);
      Index += 2;
    }

    gFitTableContext.PortModuleNumber++;
    gFitTableContext.FitEntryNumber++;
  }

  //
  // 6th, try FIT boot policy data
  //
  if ((Index < argc) &&
      ((strcmp(argv[Index], "-BP") == 0) ||
      (strcmp(argv[Index], "-bp") == 0))) {

    if (Index + 1 >= argc) {
      Error(NULL, 0, 0, "-BP: Invalid Parameters.", NULL);
      FitEntryNumber = 0;
    }

    gFitTableContext.StartupAcmFvSize = GetFvAcmSizeFromFd(FdBuffer, FdSize);
    if (gFitTableContext.StartupAcmFvSize == 0) {
      Error(NULL, 0, 0, "FV_ACM not found in Fd file!", NULL);
    }

    //
    // FIT type 04 record shares FV allocated space with FV_ACM.
    //
    FileSize = xtoi(argv[Index + 1]);

    if (gFitTableContext.StartupAcm[0].Size + FileSize > gFitTableContext.StartupAcmFvSize) {
      Error(NULL, 0, 0, "Error: not enough FV_ACM room for FIT type 04 record!", NULL);
      FitEntryNumber = 0;
    }

    FileBuffer = malloc(FileSize);
    if (FileBuffer == NULL) {
      Error(NULL, 0, 0, "No sufficient memory to allocate!", NULL);
      FitEntryNumber = 0;
    }

    SetMem(FileBuffer, FileSize, 0xFF);

    gFitTableContext.ProtBootPolicy.Type = FIT_TABLE_TYPE_PROT_BOOT_POLICY;
    gFitTableContext.ProtBootPolicy.Address = gFitTableContext.StartupAcm[0].Address + gFitTableContext.StartupAcm[0].Size;
    gFitTableContext.ProtBootPolicy.Size = FileSize;
    gFitTableContext.ProtBootPolicy.Version = 0;

    Index += 2;

    //
    // 6.1 PROT Module version
    //
    if ((Index + 1 >= argc) ||
        ((strcmp (argv[Index], "-V") != 0) &&
         (strcmp (argv[Index], "-v") != 0)) ) {
      //
      // Bypass
      //
      gFitTableContext.ProtBootPolicy.Version = gFitTableContext.GlobalVersion;
    } else {
      //
      // Get offset from parameter
      //
      gFitTableContext.ProtBootPolicy.Version = xtoi(argv[Index + 1]);
      Index += 2;
    }

    gFitTableContext.FitEntryNumber++;
  }

  //
  // Final: Check StartupAcm in BiosModule.
  //
  for (Index = 0; Index < (INTN)gFitTableContext.StartupAcmNumber; Index++) {
    CheckOverlap (gFitTableContext.StartupAcm[Index].Address, gFitTableContext.StartupAcm[Index].Size);
  }
  FitEntryNumber = gFitTableContext.FitEntryNumber;
  for (Index = 0; Index < (INTN)gFitTableContext.OptionalModuleNumber; Index++) {
    if ((gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_BIOS_POLICY) ||
        (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_KEY_MANIFEST) ||
        (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_BOOT_POLICY_MANIFEST) ||
        (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_FSP_BOOT_MANIFEST) ||
        (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_CSE_SECURE_BOOT) ||
        (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_VAB_PROVISION_TABLE) ||
        (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_VAB_BOOT_IMAGE_MANIFEST) ||
        (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_VAB_BOOT_KEY_MANIFEST)) {
      // NOTE: It might be virtual address now. Just put a place holder.
      FitEntryNumber ++;
    }
  }

  return FitEntryNumber;
}

/**
  No enough space - it might happen that it is occupied by AP wake vector.
  Last chance - skip this and search again.

  @param FvBuffer         FvRecovery binary buffer.
  @param Address          Address to be searched from.
  @param Size             Size need to be filled.

  @return FitTableOffset  The FIT table offset.
  @return NULL            No enough space for FIT table.
**/
VOID *
FindSpaceSkipApVector (
  IN UINT8     *FvBuffer,
  IN UINT8     *Address,
  IN UINTN     Size
  )
{
  UINT8        *ApVector;
  UINT8        *NewAddress;
  UINTN        Index;

  ApVector = (UINT8 *)((UINTN)Address & ~0xFFF);
  if ((UINTN)ApVector <= (UINTN)FvBuffer) {
    return NULL;
  }

  NewAddress = (UINT8 *)(ApVector - Size);
  for (Index = 0; Index < Size; Index ++) {
    if (NewAddress[Index] != 0xFF) {
      return NULL;
    }
  }
  return NewAddress;
}

/**
  Get free space for FIT table from FvRecovery.

  @param FvBuffer           FvRecovery binary buffer.
  @param FvSize             FvRecovery size.
  @param FitTableSize       The FIT table size.
  @param FixedFitLocation   Fixed FIT location provided by argument.

  @return FitTableOffset    The offset of FIT table in FvRecovery file.
  @return NULL              Free space not found.
**/
VOID *
GetFreeSpaceForFit (
  IN UINT8     *FvBuffer,
  IN UINT32    FvSize,
  IN UINT32    FitTableSize,
  IN UINT32    FixedFitLocation
  )
{
  UINT8       *FitTableOffset;
  INTN        Index;
  INTN        SubIndex;
  UINT8       *OptionalModuleAddress;
  EFI_GUID    VTFGuid = EFI_FFS_VOLUME_TOP_FILE_GUID;
  UINT32      AlignedSize;

  EFI_FIRMWARE_VOLUME_HEADER  *FvHeader;
  EFI_FFS_FILE_HEADER         *FileHeader;
  UINT64                      FvLength;
  UINT32                      Offset;
  UINT32                      FileLength;
  UINT32                      FileOccupiedSize;

  //
  // Check 4G - FitTablePointerOffset
  //
  if ((*(UINT64 *)(FvBuffer + FvSize - gFitTableContext.FitTablePointerOffset) != 0xFFFFFFFFFFFFFFFFull) &&
      (*(UINT64 *)(FvBuffer + FvSize - gFitTableContext.FitTablePointerOffset) != 0) &&
      (*(UINT64 *)(FvBuffer + FvSize - gFitTableContext.FitTablePointerOffset) != 0xEEEEEEEEEEEEEEEEull)) {
    Error (NULL, 0, 0, "4G - FitTablePointerOffset is occupied!", NULL);
    return NULL;
  }
  if (gFitTableContext.FitTablePointerOffset2 != 0) {
    if ((*(UINT64 *)(FvBuffer + FvSize - gFitTableContext.FitTablePointerOffset2) != 0xFFFFFFFFFFFFFFFFull) &&
        (*(UINT64 *)(FvBuffer + FvSize - gFitTableContext.FitTablePointerOffset2) != 0) &&
        (*(UINT64 *)(FvBuffer + FvSize - gFitTableContext.FitTablePointerOffset2) != 0xEEEEEEEEEEEEEEEEull)) {
      Error (NULL, 0, 0, "4G - FitTablePointerOffset is occupied!", NULL);
      return NULL;
    }
  }

  if (FixedFitLocation != 0) {
    //
    // Get Free space from fixed location
    //
    FitTableOffset = (UINT8 *) FLASH_TO_MEMORY (FixedFitLocation, FvBuffer, FvSize);
  } else {
    //
    // Get Free Space from FvRecovery
    //
    FitTableOffset = NULL;

    FvHeader         = (EFI_FIRMWARE_VOLUME_HEADER *)FvBuffer;
    FvLength         = FvHeader->FvLength;
    FileHeader       = (EFI_FFS_FILE_HEADER *)(FvBuffer + FvHeader->HeaderLength);
    Offset           = (UINTN)FileHeader - (UINTN)FvBuffer;

    //
    // Get EFI_FFS_VOLUME_TOP_FILE_GUID location
    //
    while (Offset < FvLength) {
      FileLength = (*(UINT32 *)(FileHeader->Size)) & 0x00FFFFFF;
      FileOccupiedSize = GETOCCUPIEDSIZE(FileLength, 8);
      if ((CompareGuid (&(FileHeader->Name), &VTFGuid)) == 0) {
        // find it
        FitTableOffset = (UINT8 *)FileHeader;
        break;
      }
      FileHeader = (EFI_FFS_FILE_HEADER *)((UINTN)FileHeader + FileOccupiedSize);
      Offset = (UINTN)FileHeader - (UINTN)FvBuffer;
    }

    if (FitTableOffset == NULL) {
      Error (NULL, 0, 0, "EFI_FFS_VOLUME_TOP_FILE_GUID not found!", NULL);
      return NULL;
    }

    FitTableOffset = (UINT8 *)((UINTN)FitTableOffset & ~FIT_ALIGNMENT);
    FitTableOffset = (UINT8 *)(FitTableOffset - FitTableSize);
  }

  //
  // Check the target space for FIT table
  //
  // 1. If FIT table has a fixed location, we assume users can provide an empty space for FIT table
  // and Option Modules.
  // 2. If FIT table location is dynamicly calculated in FvRecovery, we give a last chance to skip
  // space for AP Vector.
  //
  for (Index = 0; Index < (INTN)(FitTableSize); Index ++) {
    if (FitTableOffset[Index] != 0xFF) {

      if (FixedFitLocation != 0) {
        Error (NULL, 0, 0, "Reserved space for FIT table is not empty!", NULL);
        return NULL;
      }

      //
      // No enough space - it might happen that it is occupied by AP wake vector.
      // Last chance - skip this and search again.
      //
      FitTableOffset = FindSpaceSkipApVector (FvBuffer, &FitTableOffset[Index], FitTableSize);
      if (FitTableOffset == NULL) {
        Error (NULL, 0, 0, "No enough space for FIT table!", NULL);
        return NULL;
      }
    }
  }

  //
  // Check space for Optional module
  //
  OptionalModuleAddress = FitTableOffset;
  for (Index = 0; Index < (INTN)gFitTableContext.OptionalModuleNumber; Index++) {
    AlignedSize = gFitTableContext.OptionalModule[Index].Size;
    if ((gFitTableContext.OptionalModule[Index].Size & 0x80000000) != 0) {

      //
      // Need copy binary to file.
      //
      gFitTableContext.OptionalModule[Index].Size &= ~0x80000000;

      AlignedSize = gFitTableContext.OptionalModule[Index].Size;
      if ((gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_BIOS_POLICY) ||
          (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_KEY_MANIFEST) ||
          (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_BOOT_POLICY_MANIFEST) ||
          (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_FSP_BOOT_MANIFEST) ||
          (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_CSE_SECURE_BOOT) ||
          (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_VAB_PROVISION_TABLE) ||
          (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_VAB_BOOT_IMAGE_MANIFEST) ||
          (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_VAB_BOOT_KEY_MANIFEST)) {
        // Let it 64 byte align
        AlignedSize += BIOS_MODULE_ALIGNMENT;
        AlignedSize &= ~BIOS_MODULE_ALIGNMENT;
      }

      OptionalModuleAddress -= AlignedSize;

      if ((gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_BIOS_POLICY) ||
          (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_KEY_MANIFEST) ||
          (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_BOOT_POLICY_MANIFEST) ||
          (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_FSP_BOOT_MANIFEST) ||
          (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_CSE_SECURE_BOOT) ||
          (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_VAB_PROVISION_TABLE) ||
          (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_VAB_BOOT_IMAGE_MANIFEST) ||
          (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_VAB_BOOT_KEY_MANIFEST)) {
          // Let it 64 byte align
        OptionalModuleAddress = (UINT8 *)((UINTN)OptionalModuleAddress & ~BIOS_MODULE_ALIGNMENT);
      }

      for (SubIndex = 0; SubIndex < (INTN)(AlignedSize); SubIndex ++) {
        if (OptionalModuleAddress[SubIndex] != 0xFF) {

          if (FixedFitLocation != 0) {
            Error (NULL, 0, 0, "No enough space for Option Modules!", NULL);
            return NULL;
          }

          //
          // No enough space - it might happen that it is occupied by AP wake vector.
          // Last chance - skip this and search again.
          //
          OptionalModuleAddress = FindSpaceSkipApVector (FvBuffer, &OptionalModuleAddress[SubIndex], AlignedSize);
          if (OptionalModuleAddress == NULL) {
            Error (NULL, 0, 0, "No enough space for OptionalModule!", NULL);
            return NULL;
          }
        }
      }
      memcpy (OptionalModuleAddress, gFitTableContext.OptionalModule[Index].Buffer, gFitTableContext.OptionalModule[Index].Size);
      free (gFitTableContext.OptionalModule[Index].Buffer);
      gFitTableContext.OptionalModule[Index].Address = MEMORY_TO_FLASH (OptionalModuleAddress, FvBuffer, FvSize);
    }
    //
    // Final: Check BiosPolicyData in BiosModule.
    //
    if ((gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_BIOS_POLICY) ||
        (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_KEY_MANIFEST) ||
        (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_BOOT_POLICY_MANIFEST) ||
        (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_FSP_BOOT_MANIFEST) ||
        (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_CSE_SECURE_BOOT) ||
        (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_VAB_PROVISION_TABLE) ||
        (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_VAB_BOOT_IMAGE_MANIFEST) ||
        (gFitTableContext.OptionalModule[Index].Type == FIT_TABLE_TYPE_VAB_BOOT_KEY_MANIFEST)) {

      CheckOverlap (gFitTableContext.OptionalModule[Index].Address, AlignedSize);
    }
  }

  return FitTableOffset;
}

/**
  Output FIT table information.

  @param None

  @return None
**/
VOID
PrintFitData (
  VOID
  )
{
  UINT32                          Index;

  printf ("FIT Table Pointer Offset: 0x%x\n", gFitTableContext.FitTablePointerOffset);
  if (gFitTableContext.FitTablePointerOffset2 != 0) {
    printf ("FIT Table Pointer Offset: 0x%x\n", gFitTableContext.FitTablePointerOffset2);
  }
  printf ("Total FIT Entry number: 0x%x\n", gFitTableContext.FitEntryNumber);
  printf ("FitHeader version: 0x%04x\n", gFitTableContext.FitHeaderVersion);
  for (Index = 0; Index < gFitTableContext.StartupAcmNumber; Index++) {
    printf("StartupAcm[%d] - (0x%08x, 0x%08x, 0x%04x)\n", Index, gFitTableContext.StartupAcm[Index].Address, gFitTableContext.StartupAcm[Index].Size, gFitTableContext.StartupAcm[Index].Version);
  }
  if (gFitTableContext.DiagnstAcm.Address != 0) {
    printf ("DiagnosticAcm - (0x%08x, 0x%08x, 0x%04x)\n", gFitTableContext.DiagnstAcm.Address, gFitTableContext.DiagnstAcm.Size, gFitTableContext.DiagnstAcmVersion);
  }

  if (gFitTableContext.ProtBootPolicy.Address != 0) {
    printf ("ProtBootPolicy - (0x%08x, 0x%08x, 0x%04x)\n", gFitTableContext.ProtBootPolicy.Address, gFitTableContext.ProtBootPolicy.Size, gFitTableContext.ProtBootPolicy.Version);
  }
  for (Index = 0; Index < gFitTableContext.BiosModuleNumber; Index++) {
    printf ("BiosModule[%d] - (0x%08x, 0x%08x, 0x%04x)\n", Index, gFitTableContext.BiosModule[Index].Address, gFitTableContext.BiosModule[Index].Size, gFitTableContext.BiosModuleVersion);
  }
  for (Index = 0; Index < gFitTableContext.MicrocodeNumber; Index++) {
    printf ("Microcode[%d] - (0x%08x, 0x%08x, 0x%04x)\n", Index, gFitTableContext.Microcode[Index].Address, gFitTableContext.Microcode[Index].Size, gFitTableContext.MicrocodeVersion);
  }
  for (Index = 0; Index < gFitTableContext.MmcFwNumber; Index++) {
    printf ("MmcFw[%d] - (0x%08x, 0x%08x)\n", Index, gFitTableContext.MmcFw[Index].Address, gFitTableContext.MmcFw[Index].Size);
  }
  for (Index = 0; Index < gFitTableContext.OptionalModuleNumber; Index++) {
    printf ("OptionalModule[%d] - (0x%08x, 0x%08x, 0x%02x, 0x%04x)\n", Index, gFitTableContext.OptionalModule[Index].Address, gFitTableContext.OptionalModule[Index].Size, gFitTableContext.OptionalModule[Index].Type, gFitTableContext.OptionalModule[Index].Version);
  }
  for (Index = 0; Index < gFitTableContext.PortModuleNumber; Index++) {
    printf ("PortModule[%d] - (0x%04x, 0x%04x, 0x%02x, 0x%02x, 0x%04x, 0x%02x, 0x%04x)\n", Index,
      (UINT16)gFitTableContext.PortModule[Index].Address, (UINT16)(gFitTableContext.PortModule[Index].Address >> 16),
      (UINT8)gFitTableContext.PortModule[Index].Size, (UINT8)(gFitTableContext.PortModule[Index].Size >> 8), (UINT16)(gFitTableContext.PortModule[Index].Size >> 16),
      gFitTableContext.PortModule[Index].Type, gFitTableContext.PortModule[Index].Version);
  }

  printf ("\n");
  return ;
}

CHAR8 *mFitCseSubTypeStr[] = {
  "CSE_RSVD   ",
  "CSE_K_HASH1",
  "CSE_M_HASH ",
  "CSE_BPOLICY",
  "CSE_OTHR_BP",
  "CSE_OEMSMIP",
  "CSE_MRCDATA",
  "CSE_IBBL_H ",
  "CSE_IBB_H  ",
  "CSE_OEM_ID ",
  "CSEOEMSKUID",
  "CSE_BD_IND ",
  "CSE_FPM    ",
  "CSE_ACMM   "
};

CHAR8 *mFitTypeStr[] = {
  "           ",
  "MICROCODE  ",
  "STARTUP_ACM",
  "DIAGNST_ACM",
  "BOOT_POLICY",
  "MMCFW      ",
  "           ",
  "BIOS_MODULE",
  "TPM_POLICY ",
  "BIOS_POLICY",
  "TXT_POLICY ",
  "KEYMANIFEST",
  "BP_MANIFEST",
  "FSP_BOOT_MF",
  " ",
  " ",
  "CSE_SECUREB"
};

/**
  Convert FitEntry type to a string.

  @param FitEntry     Fit entry

  @return String
**/
CHAR8  mFitSignature[] = "'_FIT_   ' ";
CHAR8  mFitSignatureInHeader[] = "'        ' ";
CHAR8 *
FitTypeToStr (
  IN FIRMWARE_INTERFACE_TABLE_ENTRY  *FitEntry
  )
{
  if (FitEntry->Type == FIT_TABLE_TYPE_HEADER) {
    CopyMem (&mFitSignatureInHeader[1], &FitEntry->Address, sizeof(FitEntry->Address));
    return mFitSignatureInHeader;
  }
  if (FitEntry->Type < sizeof (mFitTypeStr)/sizeof(mFitTypeStr[0])) {
    if (FitEntry->Type == FIT_TABLE_TYPE_CSE_SECURE_BOOT) {
      //
      // "Reserved" field is used to distinguish CSE Secure Boot entries (see FIT spec revision 1.2)
      //
      if (FitEntry->Rsvd < sizeof (mFitCseSubTypeStr)/sizeof(mFitCseSubTypeStr[0])) {
        return mFitCseSubTypeStr[FitEntry->Rsvd];
      }
    }
    return mFitTypeStr[FitEntry->Type];
  } else {
    return "           ";
  }
}

/**
  Print Fit table in flash image.

  @param FvBuffer               FvRecovery binary buffer.
  @param FvSize                 FvRecovery size.

  @return None
**/
VOID
PrintFitTable (
  IN UINT8                       *FvBuffer,
  IN UINT32                      FvSize
  )
{
  FIRMWARE_INTERFACE_TABLE_ENTRY  *FitEntry;
  UINT32                          EntryNum;
  UINT32                          Index;
  UINT32                          FitTableOffset;
  FIRMWARE_INTERFACE_TABLE_ENTRY_PORT   *FitEntryPort;

  printf ("##############\n");
  printf ("# FIT Table: #\n");
  printf ("##############\n");

  printf ("FIT Pointer Offset: 0x%x\n", gFitTableContext.FitTablePointerOffset);
  FitTableOffset = *(UINT32 *)(FvBuffer + FvSize - gFitTableContext.FitTablePointerOffset);
  printf ("FIT Table Address:  0x%x\n", FitTableOffset);
  FitEntry = (FIRMWARE_INTERFACE_TABLE_ENTRY *)FLASH_TO_MEMORY(FitTableOffset, FvBuffer, FvSize);

  //
  // Check FitEntry is 16 byte aligned
  //
  if (((UINTN)FitEntry & 0xF) != 0) {
    printf("ERROR: invalid FitEntry address 0x%X!\n", (UINT32) (UINTN) FitEntry);
    return;
  }

  EntryNum = *(UINT32 *)(&FitEntry[0].Size[0]) & 0xFFFFFF;
  printf ("====== ================ ====== ======== ============== ==== ======== (====== ==== ====== ==== ======)\n");
  printf ("Index:      Address      Size  Version       Type      C_V  Checksum (Index  Data Width  Bit  Offset)\n");
  printf ("====== ================ ====== ======== ============== ==== ======== (====== ==== ====== ==== ======)\n");
  for (Index = 0; Index < EntryNum; Index++) {
    printf (" %02d:   %016llx %06x   %04x   %02x-%s  %02x     %02x   ",
      Index,
      (unsigned long long) FitEntry[Index].Address,
      *(UINT32 *)(&FitEntry[Index].Size[0]) & 0xFFFFFF,
      FitEntry[Index].Version,
      FitEntry[Index].Type,
      FitTypeToStr(&FitEntry[Index]),
      FitEntry[Index].C_V,
      FitEntry[Index].Checksum
      );

    if (Index == 0) {
      if (FitEntry[Index].Type != FIT_TABLE_TYPE_HEADER) {
        printf("ERROR: FIT Entry 0 is not Header Type %d!\n", FIT_TABLE_TYPE_HEADER);
        return;
      }
      if (strcmp(mFitSignatureInHeader, mFitSignature) != 0) {
        printf("ERROR: FIT Entry 0 signature invalid (%s, expected %s)!\n", mFitSignatureInHeader, mFitSignature);
        return;
      }

    }

    switch (FitEntry[Index].Type) {
    case FIT_TABLE_TYPE_TPM_POLICY:
    case FIT_TABLE_TYPE_TXT_POLICY:
      if (FitEntry[Index].Version == 0) {
        FitEntryPort = (FIRMWARE_INTERFACE_TABLE_ENTRY_PORT *)&FitEntry[Index];
        printf (" ( %04x  %04x   %02x    %02x   %04x )\n",
          FitEntryPort->IndexPort,
          FitEntryPort->DataPort,
          FitEntryPort->Width,
          FitEntryPort->Bit,
          FitEntryPort->Index
          );
        break;
      }
      // Not Port Configure, pass through
    default:
      printf ("\n");
      break;
    }
  }
  printf ("====== ================ ====== ======== ============== ==== ======== (====== ==== ====== ==== ======)\n");
  printf ("Index:      Address      Size  Version       Type      C_V  Checksum (Index  Data Width  Bit  Offset)\n");
  printf ("====== ================ ====== ======== ============== ==== ======== (====== ==== ====== ==== ======)\n");
}

/**
  This function dump raw data.

  @param  Data    Raw data.
  @param  Size    Raw data size.

 @return none

**/
VOID
DumpData (
  IN UINT8  *Data,
  IN UINT32 Size
  )
{
  UINT32 Index;
  for (Index = 0; Index < Size; Index++) {
    printf ("%02x", Data[Index]);
  }
}

/**
  This function dump raw data with colume format.

  @param  Data     Raw data
  @param  Size     Raw data size

 @return  none
**/
VOID
DumpHex (
  IN UINT8  *Data,
  IN UINT32 Size
  )
{
  UINT32  Index;
  UINT32  Count;
  UINT32  Left;

#define COLUME_SIZE  (16 * 2)

  Count = Size / COLUME_SIZE;
  Left  = Size % COLUME_SIZE;
  for (Index = 0; Index < Count; Index++) {
    printf ("%04x: ", Index * COLUME_SIZE);
    DumpData (Data + Index * COLUME_SIZE, COLUME_SIZE);
    printf ("\n");
  }

  if (Left != 0) {
    printf ("%04x: ", Index * COLUME_SIZE);
    DumpData (Data + Index * COLUME_SIZE, Left);
    printf ("\n");
  }
}

//
// This table defines the ACM type string
//
CHAR8 *mAcmTypeStr[] = {
  "BIOS ACM",
  "SINIT ACM",
};

//
// This table defines the ACM capability string
//
CHAR8 *mCapabilityStr[] = {
  "GETSEC[WAKEUP] for RLP   ",
  "MONITOR address for RLP  ",
  "ECX for MLE PageTable    ",
  "STM support              ",
};

/**
  DumpAcm information

  @param Acm       - ACM buffer

  @retval None
**/
VOID
DumpAcm (
  IN ACM_FORMAT                    *Acm
  )
{
  CHIPSET_ACM_INFORMATION_TABLE *ChipsetAcmInformationTable;
  CHIPSET_ID_LIST               *ChipsetIdList;
  PROCESSOR_ID_LIST             *ProcessorIdList;
  UINT32                        Index;
  UINT8                         *Buffer;

  printf (
    "*****************************************************************************\n"
    "*         ACM                                                               *\n"
    "*****************************************************************************\n"
    );

  printf ("ACM: (%08x)\n", (UINT32) (UINTN) Acm);
  printf ("  ModuleType                 - %04x\n", Acm->ModuleType);
  if (Acm->ModuleType == ACM_MODULE_TYPE_CHIPSET_ACM) {
    printf ("    Chipset ACM\n");
  }
  printf ("  ModuleSubType              - %04x\n", Acm->ModuleSubType);
  if ((Acm->ModuleSubType & ACM_MODULE_SUBTYPE_CAPABLE_OF_EXECUTE_AT_RESET) != 0) {
    printf ("    Capable of be Executed at Reset\n");
  }
  if ((Acm->ModuleSubType & ACM_MODULE_SUBTYPE_ANC_MODULE) != 0) {
    printf ("    AnC Module\n");
  }
  printf ("  HeaderLen                  - %08x\n", Acm->HeaderLen);
  printf ("  HeaderVersion              - %08x\n", Acm->HeaderVersion);
  printf ("  ChipsetID                  - %04x\n", Acm->ChipsetID);
  printf ("  Flags                      - %04x\n", Acm->Flags);
  printf ("    PreProduction            - %04x\n", Acm->Flags & ACM_MODULE_FLAG_PREPRODUCTION);
  printf ("    Debug Signed             - %04x\n", Acm->Flags & ACM_MODULE_FLAG_DEBUG_SIGN);
  printf ("  ModuleVendor               - %08x\n", Acm->ModuleVendor);
  printf ("  Date                       - %08x\n", Acm->Date);
  printf ("  Size                       - %08x\n", Acm->Size);
  printf ("  TxtSvn                     - %04x\n", Acm->TxtSvn);
  printf ("  SeSvn                      - %04x\n", Acm->SeSvn);
  printf ("  CodeControl                - %08x\n", Acm->CodeControl);
  printf ("  ErrorEntryPoint            - %08x\n", Acm->ErrorEntryPoint);
  printf ("  GDTLimit                   - %08x\n", Acm->GDTLimit);
  printf ("  GDTBasePtr                 - %08x\n", Acm->GDTBasePtr);
  printf ("  SegSel                     - %08x\n", Acm->SegSel);
  printf ("  EntryPoint                 - %08x\n", Acm->EntryPoint);
  printf ("  KeySize                    - %08x\n", Acm->KeySize);
  printf ("  ScratchSize                - %08x\n", Acm->ScratchSize);

  Buffer = (UINT8 *)(Acm + 1);
  printf ("  RSAPubKey                  - \n");
  DumpHex (Buffer, Acm->KeySize * 4);
  printf ("\n");
  Buffer += Acm->KeySize * 4;  //add public key size (taken from header variable * 4) to buffer.

  //add signature size to pointer.
  if (Acm->HeaderVersion == ACM_HEADER_VERSION_3) {
    printf ("  RSASig                     - \n");
    DumpHex (Buffer, ACM_PKCS_1_5_RSA_SIGNATURE_SHA384_SIZE); // PKCS #1.5 RSA Signature
    printf ("\n");
    Buffer += ACM_PKCS_1_5_RSA_SIGNATURE_SHA384_SIZE;
  }
  else if ((Acm->HeaderVersion == ACM_HEADER_VERSION_4) || (Acm->HeaderVersion == ACM_HEADER_VERSION_5)) {
    Buffer += ACM_PKCS_1_5_RSA_SIGNATURE_SHA384_SIZE;
    Buffer += ACM_XMSS_PUBLIC_KEY_SIZE;
    Buffer += ACM_XMSS_SIGNATURE_SIZE;
  }
  else {
    printf ("  RSAPubExp                  - %08x\n", *(UINT32 *)Buffer);
    Buffer += 4;

    printf ("  RSASig                     - \n");
    DumpHex (Buffer, ACM_PKCS_1_5_RSA_SIGNATURE_SHA256_SIZE); // PKCS #1.5 RSA Signature
    printf ("\n");
    Buffer += ACM_PKCS_1_5_RSA_SIGNATURE_SHA256_SIZE;
  }
  Buffer += Acm->ScratchSize * 4;

  if ((Acm->HeaderVersion == ACM_HEADER_VERSION_4) || (Acm->HeaderVersion == ACM_HEADER_VERSION_5)) {
    Buffer += 60;  //add reserved bytes.
  }

  if ((Acm->ModuleSubType & ACM_MODULE_SUBTYPE_ANC_MODULE) == 0) {
    ChipsetAcmInformationTable = (CHIPSET_ACM_INFORMATION_TABLE *)Buffer;
    printf ("Chipset ACM info:\n");
    printf (
      "  Guid                       - {%08x-%08x-%08x-%08x}\n",
      ChipsetAcmInformationTable->Guid.Guid0,
      ChipsetAcmInformationTable->Guid.Guid1,
      ChipsetAcmInformationTable->Guid.Guid2,
      ChipsetAcmInformationTable->Guid.Guid3
      );
    printf ("  ChipsetACMType             - %02x\n", ChipsetAcmInformationTable->ChipsetACMType);
    if (ChipsetAcmInformationTable->ChipsetACMType < sizeof(mAcmTypeStr)/sizeof(mAcmTypeStr[0])) {
      printf ("    %s\n", mAcmTypeStr[ChipsetAcmInformationTable->ChipsetACMType]);
    }
    printf ("  Version                    - %02x\n", ChipsetAcmInformationTable->Version);
    printf ("  Length                     - %04x\n", ChipsetAcmInformationTable->Length);
    printf ("  ChipsetIDList              - %08x\n", ChipsetAcmInformationTable->ChipsetIDList);
    printf ("  OsSinitTableVer            - %08x\n", ChipsetAcmInformationTable->OsSinitTableVer);
    printf ("  MinMleHeaderVer            - %08x\n", ChipsetAcmInformationTable->MinMleHeaderVer);
    if (ChipsetAcmInformationTable->Version >= CHIPSET_ACM_INFORMATION_TABLE_VERSION_3) {
      printf ("  Capabilities               - %08x\n", ChipsetAcmInformationTable->Capabilities);
      for (Index = 0; Index < sizeof(mCapabilityStr)/sizeof(mCapabilityStr[0]); Index++) {
        if (mCapabilityStr[Index] == NULL) {
          continue;
        }
        printf (
          "    %s- %08x\n",
          mCapabilityStr[Index],
          (ChipsetAcmInformationTable->Capabilities & (1 << Index))
          );
      }
      printf ("  AcmVersion                 - %02x\n", ChipsetAcmInformationTable->AcmVersion);
      printf ("  AcmRevision                - %02x.%02x.%02x\n", ChipsetAcmInformationTable->AcmRevision[0], ChipsetAcmInformationTable->AcmRevision[1], ChipsetAcmInformationTable->AcmRevision[2]);
    }
    if (ChipsetAcmInformationTable->Version >= CHIPSET_ACM_INFORMATION_TABLE_VERSION_4) {
      printf ("  ProcessorIDList            - %08x\n", ChipsetAcmInformationTable->ProcessorIDList);
    }

    ChipsetIdList = (CHIPSET_ID_LIST *)((UINTN)Acm + ChipsetAcmInformationTable->ChipsetIDList);
    printf ("Chipset ID List info:\n");
    printf ("  Count                      - %08x\n", ChipsetIdList->Count);
    for (Index = 0; Index < ChipsetIdList->Count; Index++) {
      printf ("  ID[%d]:\n", Index);
      printf ("    Flags                    - %08x\n", ChipsetIdList->ChipsetID[Index].Flags);
      printf ("      RevisionIdMask         - %08x\n", ChipsetIdList->ChipsetID[Index].Flags & ACM_CHIPSET_ID_REVISION_ID_MAKE);
      printf ("    VendorID                 - %04x\n", ChipsetIdList->ChipsetID[Index].VendorID);
      printf ("    DeviceID                 - %04x\n", ChipsetIdList->ChipsetID[Index].DeviceID);
      printf ("    RevisionID               - %04x\n", ChipsetIdList->ChipsetID[Index].RevisionID);
    }
    if (ChipsetAcmInformationTable->Version < CHIPSET_ACM_INFORMATION_TABLE_VERSION_4) {
      goto End;
    }
    ProcessorIdList = (PROCESSOR_ID_LIST *)((UINTN)Acm + ChipsetAcmInformationTable->ProcessorIDList);
    printf ("Processor ID List info:\n");
    printf ("  Count                      - %08x\n", ProcessorIdList->Count);
    for (Index = 0; Index < ProcessorIdList->Count; Index++) {
      printf ("  ID[%d]:\n", Index);
      printf ("    FMS                      - %08x\n", ProcessorIdList->ProcessorID[Index].FMS);
      printf ("    FMSMask                  - %08x\n", ProcessorIdList->ProcessorID[Index].FMSMask);
      printf ("    PlatformID               - %016llx\n", (unsigned long long) ProcessorIdList->ProcessorID[Index].PlatformID);
      printf ("    PlatformMask             - %016llx\n", (unsigned long long) ProcessorIdList->ProcessorID[Index].PlatformMask);
    }
  }

End:
  printf (
    "*****************************************************************************\n\n"
    );
}

/**
  Get ACM FMS information.

  @param Acm          ACM buffer.
  @param AcmFms       Get ACM FMS.
  @param AcmMask      Get ACM Mask.

  @retval NULL
**/
VOID
GetAcmFms(
  IN ACM_FORMAT *Acm,
  OUT UINT32 *AcmFms,
  OUT UINT32 *AcmMask
)
{
  UINT32 FmsOffset = 0;
  UINT32 TmpFms = 0;
  UINT32 TmpMask = 0;
  UINT32 Index = 0;
  PROCESSOR_ID_LIST *ProcessorIdList = NULL;

  if ((Acm == NULL) || (AcmFms == NULL) || (AcmMask == NULL))
    return;

  *AcmFms = 0;
  *AcmMask = 0;

  switch (Acm->HeaderVersion) {
  case ACM_HEADER_VERSION_3:
    FmsOffset = *(UINT32*)((UINT8*)Acm + 0x6E8); //AcmInfoTable at 0x6C0, +0x28 for ProcessorIdList
    break;
  case ACM_HEADER_VERSION_4:
  case ACM_HEADER_VERSION_5:
    FmsOffset = *(UINT32*)((UINT8*)Acm + 0x1CA8); //AcmInfoTable at 0x1C80, +0x28 for ProcessorIdList
    break;
  default:
    return;
  }

  ProcessorIdList = (PROCESSOR_ID_LIST *)((UINT8*)Acm + FmsOffset);

  if (ProcessorIdList->Count > 0) {
    TmpFms = ProcessorIdList->ProcessorID[0].FMS;
    *AcmFms = TmpFms;
    *AcmMask = DEFAULT_ACM_EXTENDED_MASK;
  }

  for (Index = 1; Index < ProcessorIdList->Count; Index++) {
    TmpMask = (TmpFms ^ ProcessorIdList->ProcessorID[Index].FMS);
    TmpFms &= ProcessorIdList->ProcessorID[Index].FMS;
  }

  *AcmMask = ~TmpMask;
  return;
}

/**
  Check Acm information.

  @param Acm          ACM buffer.
  @param AcmMaxSize   ACM max size.

  @retval TRUE    ACM is valid.
  @retval FALSE   ACM is invalid.
**/
BOOLEAN
CheckAcm (
  IN ACM_FORMAT                        *Acm,
  IN UINTN                             AcmMaxSize
  )
{
  CHIPSET_ACM_INFORMATION_TABLE *ChipsetAcmInformationTable;
  CHIPSET_ID_LIST               *ChipsetIdList;
  PROCESSOR_ID_LIST             *ProcessorIdList;
  UINT8                         *Buffer;

  if (Acm->ModuleType != ACM_MODULE_TYPE_CHIPSET_ACM) {
    printf ("ACM invalid : ModuleType!\n");
    return FALSE;
  }
  if (Acm->Size * 4 > AcmMaxSize) {
    printf ("ACM invalid : Size!\n");
    return FALSE;
  }

  //move buffer pointer to address past generic ACM header (post scratchsize)
  Buffer = (UINT8 *)(Acm + 1);
  Buffer += Acm->KeySize * 4;
  if (Acm->HeaderVersion == ACM_HEADER_VERSION_3) {
    Buffer += ACM_PKCS_1_5_RSA_SIGNATURE_SHA384_SIZE;
  }
  else if ((Acm->HeaderVersion == ACM_HEADER_VERSION_4) || (Acm->HeaderVersion == ACM_HEADER_VERSION_5)) {
    Buffer += ACM_PKCS_1_5_RSA_SIGNATURE_SHA384_SIZE;
    Buffer += ACM_XMSS_PUBLIC_KEY_SIZE;
    Buffer += ACM_XMSS_SIGNATURE_SIZE;
  }
  else {
    Buffer += 4;
    Buffer += ACM_PKCS_1_5_RSA_SIGNATURE_SHA256_SIZE;
  }
  Buffer += Acm->ScratchSize * 4;

  if ((Acm->HeaderVersion == ACM_HEADER_VERSION_4) || (Acm->HeaderVersion == ACM_HEADER_VERSION_5)) {
    Buffer += 60;  //add reserved bytes.
  }

  if ((Acm->ModuleSubType & ACM_MODULE_SUBTYPE_ANC_MODULE) == 0) {
    ChipsetAcmInformationTable = (CHIPSET_ACM_INFORMATION_TABLE *)Buffer;
    if ((UINTN)ChipsetAcmInformationTable >= (UINTN)Acm + AcmMaxSize) {
      printf ("ACM invalid : ChipsetAcmInformationTable!\n");
      return FALSE;
    }

    if (CompareGuid ((EFI_GUID *)&ChipsetAcmInformationTable->Guid, (EFI_GUID *)&mChipsetAcmInformationTableGuid03) != 0) {
      printf ("ACM invalid : ChipsetACMGuid!\n");
      return FALSE;
    }
    if (ChipsetAcmInformationTable->ChipsetACMType != CHIPSET_ACM_TYPE_BIOS) {
      printf ("ACM invalid : ChipsetACMType!\n");
      return FALSE;
    }
    if (ChipsetAcmInformationTable->Version < CHIPSET_ACM_INFORMATION_TABLE_VERSION_3) {
      printf ("ACM invalid : ChipsetACMVersion!\n");
      return FALSE;
    }
    if ((UINTN)ChipsetAcmInformationTable + ChipsetAcmInformationTable->Length > (UINTN)Acm + AcmMaxSize) {
      printf ("ACM invalid : ChipsetACMLength!\n");
      return FALSE;
    }

    if (ChipsetAcmInformationTable->ChipsetIDList >= AcmMaxSize) {
      printf ("ACM invalid : ChipsetACMChipsetIDList!\n");
      return FALSE;
    }
    ChipsetIdList = (CHIPSET_ID_LIST *)((UINTN)Acm + ChipsetAcmInformationTable->ChipsetIDList);
    if (ChipsetIdList->Count == 0) {
      printf ("ACM invalid : ChipsetACMChipsetIDListCount!\n");
      return FALSE;
    }
    if (ChipsetAcmInformationTable->ChipsetIDList + sizeof(CHIPSET_ID_LIST) + (ChipsetIdList->Count - 1) * sizeof(ACM_CHIPSET_ID) > AcmMaxSize) {
      printf ("ACM invalid : ChipsetACMChipsetIDList!\n");
      return FALSE;
    }

    if (ChipsetAcmInformationTable->Version < CHIPSET_ACM_INFORMATION_TABLE_VERSION_4) {
      goto End;
    }

    if (ChipsetAcmInformationTable->ProcessorIDList >= AcmMaxSize) {
      printf ("ACM invalid : ChipsetACMProcessorIDList!\n");
      return FALSE;
    }
    ProcessorIdList = (PROCESSOR_ID_LIST *)((UINTN)Acm + ChipsetAcmInformationTable->ProcessorIDList);
    if (ProcessorIdList->Count == 0) {
      printf ("ACM invalid : ChipsetACMProcessorIdListCount!\n");
      return FALSE;
    }
    if (ChipsetAcmInformationTable->ChipsetIDList + sizeof(PROCESSOR_ID_LIST) + (ChipsetIdList->Count - 1) * sizeof(ACM_PROCESSOR_ID) > AcmMaxSize) {
      printf ("ACM invalid : ChipsetACMProcessorIdList!\n");
      return FALSE;
    }
  }

End:

  return TRUE;
}

/**
  Fill the FIT table information to FvRecovery.

  @param FvBuffer         FvRecovery binary buffer.
  @param FvSize           FvRecovery size.
  @param FitTableOffset   The offset of FIT table in FvRecovery file.

  @retval None
**/
VOID
FillFitTable (
  IN UINT8     *FvBuffer,
  IN UINT32    FvSize,
  IN UINT8     *FitTableOffset
  )
{
  FIRMWARE_INTERFACE_TABLE_ENTRY *FitEntry;
  UINT32                          FitIndex;
  UINT32                          FitEntrySizeValue;
  UINT32                          Index;
  UINT8                           Checksum;
  UINTN                           SubIndex;
  FIT_TABLE_CONTEXT_ENTRY         TempContextEntry;
  FIRMWARE_INTERFACE_TABLE_ENTRY  TempTableEntry;
  PROCESSOR_ID                    FMS;
  PROCESSOR_ID                    FMSMask;

  //
  // 1. FitPointer
  //
  *(UINT64 *)(FvBuffer + FvSize - gFitTableContext.FitTablePointerOffset) = (UINT64)(UINTN)MEMORY_TO_FLASH (FitTableOffset, FvBuffer, FvSize);
  if (gFitTableContext.FitTablePointerOffset2 != 0) {
    *(UINT64 *)(FvBuffer + FvSize - gFitTableContext.FitTablePointerOffset2) = (UINT64)(UINTN)MEMORY_TO_FLASH (FitTableOffset, FvBuffer, FvSize);
  }

  FitEntry = (FIRMWARE_INTERFACE_TABLE_ENTRY *)FitTableOffset;
  FitIndex = 0;

  //
  // 2. FitHeader
  //
  FitEntrySizeValue           = gFitTableContext.FitEntryNumber;
  FitEntry[FitIndex].Address  = *(UINT64 *)"_FIT_   ";
  FitEntry[FitIndex].Size[0]  = (UINT8)FitEntrySizeValue;
  FitEntry[FitIndex].Size[1]  = (UINT8)(FitEntrySizeValue >> 8);
  FitEntry[FitIndex].Size[2]  = (UINT8)(FitEntrySizeValue >> 16);
  FitEntry[FitIndex].Rsvd     = 0;
  FitEntry[FitIndex].Version  = (UINT16)gFitTableContext.FitHeaderVersion;
  FitEntry[FitIndex].Type     = FIT_TABLE_TYPE_HEADER;
  FitEntry[FitIndex].C_V      = 1;
  //
  // Checksum will be updated later...
  //
  FitEntry[FitIndex].Checksum = 0;

  //
  // 3. Microcode
  //
  FitIndex++;
  for (Index = 0; Index < gFitTableContext.MicrocodeNumber; Index++) {
    FitEntrySizeValue           = 0; // gFitTableContext.Microcode[Index].Size / 16
    FitEntry[FitIndex].Address  = gFitTableContext.Microcode[Index].Address;
    FitEntry[FitIndex].Size[0]  = (UINT8)FitEntrySizeValue;
    FitEntry[FitIndex].Size[1]  = (UINT8)(FitEntrySizeValue >> 8);
    FitEntry[FitIndex].Size[2]  = (UINT8)(FitEntrySizeValue >> 16);
    FitEntry[FitIndex].Rsvd     = 0;
    FitEntry[FitIndex].Version  = (UINT16)gFitTableContext.MicrocodeVersion;
    FitEntry[FitIndex].Type     = FIT_TABLE_TYPE_MICROCODE;
    FitEntry[FitIndex].C_V      = 0;
    FitEntry[FitIndex].Checksum = 0;
    FitIndex++;
  }

  //
  // 4. StartupAcm
  //
  for (Index = 0; Index < gFitTableContext.StartupAcmNumber; Index++) {
    if (gFitTableContext.StartupAcm[Index].Version == STARTUP_ACM_FIT_ENTRY_200_VERSION) {
      printf("ACM version 0x200\n");
      FMS.Uint32 = gFitTableContext.StartupAcm[Index].FMS;
      FMSMask.Uint32 = gFitTableContext.StartupAcm[Index].FMSMask;
      printf("ACM FMS:%08x\n", FMS.Uint32);
      printf("ACM FMSMask:%08x\n", FMSMask.Uint32);
      FitEntry[FitIndex].Address  = gFitTableContext.StartupAcm[Index].Address;
      FitEntry[FitIndex].Size[0]  = NIBBLES_TO_BYTE (FMS.Bits.Family, FMS.Bits.Model);
      FitEntry[FitIndex].Size[1]  = NIBBLES_TO_BYTE (FMS.Bits.ExtendedModel, FMS.Bits.Type);
      FitEntry[FitIndex].Size[2]  = NIBBLES_TO_BYTE (FMSMask.Bits.Family, FMSMask.Bits.Model);
      FitEntry[FitIndex].Rsvd     = NIBBLES_TO_BYTE (FMSMask.Bits.ExtendedModel, FMSMask.Bits.Type);
      FitEntry[FitIndex].Version = (UINT16)gFitTableContext.StartupAcm[Index].Version;
      FitEntry[FitIndex].Type     = FIT_TABLE_TYPE_STARTUP_ACM;
      FitEntry[FitIndex].C_V      = 0;
      FitEntry[FitIndex].Checksum = NIBBLES_TO_BYTE (FMSMask.Bits.ExtendedFamily, FMS.Bits.ExtendedFamily);
    } else {
      FitEntrySizeValue           = 0; // gFitTableContext.StartupAcm.Size / 16
      FitEntry[FitIndex].Address  = gFitTableContext.StartupAcm[Index].Address;
      FitEntry[FitIndex].Size[0]  = (UINT8)FitEntrySizeValue;
      FitEntry[FitIndex].Size[1]  = (UINT8)(FitEntrySizeValue >> 8);
      FitEntry[FitIndex].Size[2]  = (UINT8)(FitEntrySizeValue >> 16);
      FitEntry[FitIndex].Rsvd     = 0;
      FitEntry[FitIndex].Version = (UINT16)gFitTableContext.StartupAcm[Index].Version;
      FitEntry[FitIndex].Type     = FIT_TABLE_TYPE_STARTUP_ACM;
      FitEntry[FitIndex].C_V      = 0;
      FitEntry[FitIndex].Checksum = 0;
    }
    FitIndex++;
  }

  for (Index = 0; Index < gFitTableContext.MmcFwNumber; Index++) {
    FitEntrySizeValue           = 0; // gFitTableContext.MMCVersion.Size / 16
    FitEntry[FitIndex].Address  = gFitTableContext.MmcFw[Index].Address;
    FitEntry[FitIndex].Size[0]  = (UINT8)FitEntrySizeValue;
    FitEntry[FitIndex].Size[1]  = (UINT8)(FitEntrySizeValue >> 8);
    FitEntry[FitIndex].Size[2]  = (UINT8)(FitEntrySizeValue >> 16);
    FitEntry[FitIndex].Rsvd     = 0;
    FitEntry[FitIndex].Version  = (UINT16)gFitTableContext.MmcFw[Index].Version;
    FitEntry[FitIndex].Type     = 0x5;
    FitEntry[FitIndex].C_V      = 0;
    FitEntry[FitIndex].Checksum = 0;
    FitIndex++;
  }

  //
  // 4.5. DiagnosticAcm
  //
  if (gFitTableContext.DiagnstAcm.Address != 0) {
    FitEntrySizeValue           = 0; // gFitTableContext.DiagnstAcmVersion.Size / 16
    FitEntry[FitIndex].Address  = gFitTableContext.DiagnstAcm.Address;
    FitEntry[FitIndex].Size[0]  = (UINT8)FitEntrySizeValue;
    FitEntry[FitIndex].Size[1]  = (UINT8)(FitEntrySizeValue >> 8);
    FitEntry[FitIndex].Size[2]  = (UINT8)(FitEntrySizeValue >> 16);
    FitEntry[FitIndex].Rsvd     = 0;
    FitEntry[FitIndex].Version  = (UINT16)gFitTableContext.DiagnstAcmVersion;
    FitEntry[FitIndex].Type     = FIT_TABLE_TYPE_DIAGNST_ACM;
    FitEntry[FitIndex].C_V      = 0;
    FitEntry[FitIndex].Checksum = 0;
    FitIndex++;
  }
  //
  // 5. (4) Bootable BootPolicy Data
  //
  if (gFitTableContext.ProtBootPolicy.Address != 0) {
    FitEntry[FitIndex].Address                 = gFitTableContext.ProtBootPolicy.Address;
    FitEntry[FitIndex].Size[0]                 = (UINT8) (gFitTableContext.ProtBootPolicy.Size);
    FitEntry[FitIndex].Size[1]                 = (UINT8) (gFitTableContext.ProtBootPolicy.Size >> 8);
    FitEntry[FitIndex].Size[2]                 = (UINT8) (gFitTableContext.ProtBootPolicy.Size >> 16);
    FitEntry[FitIndex].Rsvd                    = 0;
    FitEntry[FitIndex].Version                 = (UINT16)gFitTableContext.ProtBootPolicy.Version;
    FitEntry[FitIndex].Type                    = FIT_TABLE_TYPE_PROT_BOOT_POLICY;
    FitEntry[FitIndex].C_V                     = 0;
    FitEntry[FitIndex].Checksum                = 0;
    FitIndex++;
  }

  //
  // 6. BiosModule
  //
  //
  // BiosModule segments order needs to be put from low address to high for Btg requirement
  //
  if (gFitTableContext.BiosModuleNumber > 1) {
    for (Index = 0; Index < (UINTN)gFitTableContext.BiosModuleNumber - 1; Index++){
      for (SubIndex = 0; SubIndex < gFitTableContext.BiosModuleNumber - Index - 1; SubIndex++) {
        if (gFitTableContext.BiosModule[SubIndex].Address > gFitTableContext.BiosModule[SubIndex + 1].Address) {
          CopyMem (&TempContextEntry, &gFitTableContext.BiosModule[SubIndex], sizeof(FIT_TABLE_CONTEXT_ENTRY));
          CopyMem (&gFitTableContext.BiosModule[SubIndex], &gFitTableContext.BiosModule[SubIndex + 1], sizeof(FIT_TABLE_CONTEXT_ENTRY));
          CopyMem (&gFitTableContext.BiosModule[SubIndex + 1], &TempContextEntry, sizeof(FIT_TABLE_CONTEXT_ENTRY));
        }
      }
    }
  }
  for (Index = 0; Index < gFitTableContext.BiosModuleNumber; Index++) {
    FitEntrySizeValue           = gFitTableContext.BiosModule[Index].Size / 16;
    FitEntry[FitIndex].Address  = gFitTableContext.BiosModule[Index].Address;
    FitEntry[FitIndex].Size[0]  = (UINT8)FitEntrySizeValue;
    FitEntry[FitIndex].Size[1]  = (UINT8)(FitEntrySizeValue >> 8);
    FitEntry[FitIndex].Size[2]  = (UINT8)(FitEntrySizeValue >> 16);
    FitEntry[FitIndex].Rsvd     = 0;
    FitEntry[FitIndex].Version  = (UINT16)gFitTableContext.BiosModuleVersion;
    FitEntry[FitIndex].Type     = FIT_TABLE_TYPE_BIOS_MODULE;
    FitEntry[FitIndex].C_V      = 0;
    FitEntry[FitIndex].Checksum = 0;
    FitIndex++;
  }

  //
  // 7. Optional module
  //
  for (Index = 0; Index < gFitTableContext.OptionalModuleNumber; Index++) {
    FitEntrySizeValue           = gFitTableContext.OptionalModule[Index].Size;
    FitEntry[FitIndex].Address  = gFitTableContext.OptionalModule[Index].Address;
    FitEntry[FitIndex].Size[0]  = (UINT8)FitEntrySizeValue;
    FitEntry[FitIndex].Size[1]  = (UINT8)(FitEntrySizeValue >> 8);
    FitEntry[FitIndex].Size[2]  = (UINT8)(FitEntrySizeValue >> 16);
    FitEntry[FitIndex].Version  = (UINT16)gFitTableContext.OptionalModule[Index].Version;
    FitEntry[FitIndex].Type     = (UINT8)gFitTableContext.OptionalModule[Index].Type;
    if (FitEntry[FitIndex].Type == FIT_TABLE_TYPE_CSE_SECURE_BOOT) {
      FitEntry[FitIndex].Rsvd   = (UINT8)gFitTableContext.OptionalModule[Index].SubType;
    }
    FitEntry[FitIndex].C_V      = 0;
    FitEntry[FitIndex].Checksum = 0;
    FitIndex++;
  }

  //
  // 8. Port module
  //
  for (Index = 0; Index < gFitTableContext.PortModuleNumber; Index++) {
    FitEntrySizeValue           = 0;
    FitEntry[FitIndex].Address  = gFitTableContext.PortModule[Index].Address + ((UINT64)gFitTableContext.PortModule[Index].Size << 32);
    FitEntry[FitIndex].Size[0]  = (UINT8)FitEntrySizeValue;
    FitEntry[FitIndex].Size[1]  = (UINT8)(FitEntrySizeValue >> 8);
    FitEntry[FitIndex].Size[2]  = (UINT8)(FitEntrySizeValue >> 16);
    FitEntry[FitIndex].Rsvd     = 0;
    FitEntry[FitIndex].Version  = (UINT16)gFitTableContext.PortModule[Index].Version;
    FitEntry[FitIndex].Type     = (UINT8)gFitTableContext.PortModule[Index].Type;
    FitEntry[FitIndex].C_V      = 0;
    FitEntry[FitIndex].Checksum = 0;
    FitIndex++;
  }

  //
  // The FIT records must always be arranged in the ascending order of their type attribute in the FIT.
  //
  for (Index = 0; Index < (UINTN)FitIndex - 1; Index++){
    for (SubIndex = 0; SubIndex < FitIndex - Index - 1; SubIndex++) {
      if (FitEntry[SubIndex].Type > FitEntry[SubIndex + 1].Type) {
        CopyMem (&TempTableEntry, &FitEntry[SubIndex], sizeof(FIRMWARE_INTERFACE_TABLE_ENTRY));
        CopyMem (&FitEntry[SubIndex], &FitEntry[SubIndex + 1], sizeof(FIRMWARE_INTERFACE_TABLE_ENTRY));
        CopyMem (&FitEntry[SubIndex + 1], &TempTableEntry, sizeof(FIRMWARE_INTERFACE_TABLE_ENTRY));
      }
    }
  }

  //
  // Update FIT header signature as final step
  //
  Checksum = CalculateChecksum8 ((UINT8 *)&FitEntry[0], sizeof (FIRMWARE_INTERFACE_TABLE_ENTRY) * FitIndex);
  FitEntry[0].Checksum = Checksum;
}

/**
  Clear the FIT table information to Fvrecovery.

  @param FvBuffer       - Fvrecovery binary buffer.
  @param FvSize         - Fvrecovery size.

  @retval None
**/
VOID
ClearFitTable (
  IN UINT8     *FvBuffer,
  IN UINT32    FvSize
  )
{
  FIRMWARE_INTERFACE_TABLE_ENTRY *FitEntry;
  UINT32                          EntryNum;
  UINT32                          FitIndex;
  UINT64                          FitTablePointer;
  UINT8                           *Buffer;
  UINT32                          BufferSize;

  FitTablePointer = *(UINT64 *)(FvBuffer + FvSize - gFitTableContext.FitTablePointerOffset);
  FitEntry = (FIRMWARE_INTERFACE_TABLE_ENTRY *)FLASH_TO_MEMORY (FitTablePointer, FvBuffer, FvSize);

  //
  // Clear FIT pointer
  //
  *(UINT64 *)(FvBuffer + FvSize - gFitTableContext.FitTablePointerOffset) = 0xEEEEEEEEEEEEEEEEull;
  if (gFitTableContext.FitTablePointerOffset2 != 0) {
    *(UINT64 *)(FvBuffer + FvSize - gFitTableContext.FitTablePointerOffset2) = 0xEEEEEEEEEEEEEEEEull;
  }

  //
  // Clear FIT table
  //
  EntryNum = *(UINT32 *)(&FitEntry[0].Size[0]) & 0xFFFFFF;
  for (FitIndex = 0; FitIndex < EntryNum; FitIndex++) {
    switch (FitEntry[FitIndex].Type) {
    case FIT_TABLE_TYPE_BIOS_POLICY:
    case FIT_TABLE_TYPE_KEY_MANIFEST:
    case FIT_TABLE_TYPE_BOOT_POLICY_MANIFEST:
    case FIT_TABLE_TYPE_FSP_BOOT_MANIFEST:
    case FIT_TABLE_TYPE_CSE_SECURE_BOOT:
      //
      // Clear FIT table data buffer
      //
      Buffer = FLASH_TO_MEMORY (FitEntry[FitIndex].Address, FvBuffer, FvSize);
      BufferSize = (*(UINT32 *)FitEntry[FitIndex].Size) & 0xFFFFFF;
      SetMem (Buffer, BufferSize, 0xFF);
      break;
    default:
      break;
    }
    //
    // Clear FIT table itself
    //
    SetMem (&FitEntry[FitIndex], sizeof(FitEntry[FitIndex]), 0xFF);
  }
}

/**
  Read input file.

  @param FileName          The input file name.
  @param FileData          The input file data.
  @paramFileSize           The input file size.

  @retval STATUS_SUCCESS   Write file data successfully.
  @retval STATUS_ERROR     The file data is not written.
**/
STATUS
WriteOutputFile (
  IN CHAR8   *FileName,
  IN UINT8   *FileData,
  IN UINT32  FileSize
  )
{
  FILE                        *FpOut;

  //
  //Check the File Path
  //
  if (!CheckPath(FileName)) {

    Error (NULL, 0, 0, "File path is invalid!", NULL);
    return STATUS_ERROR;
  }

  //
  // Open the output FvRecovery.fv file
  //
  if ((FpOut = fopen (FileName, "w+b")) == NULL) {
    Error (NULL, 0, 0, "Unable to open file", "%s", FileName);
    return STATUS_ERROR;
  }
  //
  // Write the output FvRecovery.fv file
  //
  if ((fwrite (FileData, 1, FileSize, FpOut)) != FileSize) {
    Error (NULL, 0, 0, "Write output file error!", NULL);
    fclose (FpOut);
    return STATUS_ERROR;
  }

  //
  // Close the output FvRecovery.fv file
  //
  fclose (FpOut);

  return STATUS_SUCCESS;
}


UINT32
GetFvAcmSizeFromFd(
  IN UINT8                       *FdBuffer,
  IN UINT32                      FdFileSize
)
/**
  Get FV_ACM Size information from Fd file.

  @param FdBuffer       Fd file buffer.
  @param FdFileSize     Fd file size.

  @retval FvACM size

**/
{
  UINT8                         *FileBuffer = NULL;
  UINT32                        FvAcmSize = 0;
  EFI_GUID                      ACMGuid = ACMFV_GUID;
  UINT32                        FvLength;
  UINT32                        FileLength;

  //*FvRecovery = NULL;
  FileBuffer = FindNextFvHeader(FdBuffer, FdFileSize);
  if (FileBuffer == NULL) {
    return 0;
  }

  while ((UINTN)FileBuffer < (UINTN)FdBuffer + FdFileSize) {
    FvLength = (UINT32)((EFI_FIRMWARE_VOLUME_HEADER *)FileBuffer)->FvLength;

    if (FindFileFromFvByGuid(FileBuffer, FvLength, &ACMGuid, &FileLength) != NULL) {
      //
      // Found the ACM
      //
      FvAcmSize = FvLength;
    }

    //
    // Next fv
    //
    FileBuffer = (UINT8 *)FileBuffer + FvLength;
    if ((UINTN)FileBuffer >= (UINTN)FdBuffer + FdFileSize) {
      break;
    }
    FileBuffer = FindNextFvHeader(FileBuffer, (UINTN)FdBuffer + FdFileSize - (UINTN)FileBuffer);
    if (FileBuffer == NULL) {
      break;
    }

  }

  return FvAcmSize;
}

/**

  Get FvRecovery information from Fd file.

  @param FdBuffer               Fd file buffer.
  @param FdFileSize             Fd file size.
  @param FvRecovery             FvRecovery pointer in Fd file buffer

  @retval FvRecovery file size

**/
UINT32
GetFvRecoveryInfoFromFd (
  IN UINT8                       *FdBuffer,
  IN UINT32                      FdFileSize,
  OUT UINT8                      **FvRecovery
  )
{
  UINT8                         *FileBuffer = NULL;
  UINT32                        FvRecoveryFileSize =0;
  EFI_GUID                      VTFGuid = EFI_FFS_VOLUME_TOP_FILE_GUID;
  UINT32                        FvLength;
  UINT32                        FileLength;

  *FvRecovery = NULL;
  FileBuffer = FindNextFvHeader (FdBuffer, FdFileSize);
  if (FileBuffer == NULL) {
    return 0;
  }

  while ((UINTN)FileBuffer < (UINTN)FdBuffer + FdFileSize) {
    FvLength         = (UINT32)((EFI_FIRMWARE_VOLUME_HEADER *)FileBuffer)->FvLength;

    if (FindFileFromFvByGuid (FileBuffer, FvLength, &VTFGuid, &FileLength) != NULL) {
      //
      // Found the VTF
      //
      FvRecoveryFileSize = FvLength;
      *FvRecovery = FileBuffer;
    }

    //
    // Next fv
    //
    FileBuffer = (UINT8 *)FileBuffer + FvLength;
    if ((UINTN)FileBuffer >= (UINTN)FdBuffer + FdFileSize) {
      break;
    }
    FileBuffer = FindNextFvHeader (FileBuffer, (UINTN)FdBuffer + FdFileSize - (UINTN)FileBuffer);
    if (FileBuffer == NULL) {
      break;
    }

  }

  //
  // Return
  //
  return FvRecoveryFileSize;
}

/**
  Get FMS information from FIT Entry.

  Note: Since FIT entry not record all the processor ID information.
        The value would not the same as the real value.

  +----------+-----------------------+-----------------------+-----------------------+-----------------------+
  |   Byte   |          15           |          14           |         13:12         |          11           |
  +----------+-----------------------+-----------------------+-----------------------+-----------------------+
  |Bit Fields|   [7:4]   |   [3:0]   |   [7:7]   |   [6:0]   |   [7:4]   |   [3:0]   |   [7:4]   |   [3:0]   |
  +----------+-----------------------+-----------------------+-----------------------+-----------------------+
  | Ver. 100 |       Checksum        |    C_V    |    Type   |        Version        |        Reserved       |
  +----------+-----------------------+-----------------------+-----------------------+-----------------------+
  | Ver. 200 |  FMSMask  |    FMS    |    C_V    |    Type   |        Version        |  FMSMask  |  FMSMask  |
  |          | ExtFamily | ExtFamily |           |           |                       |  ExtModel |    Type   |
  +----------+-----------------------+-----------------------+-----------------------+-----------------------+

  +----------+-----------------------+-----------------------+-----------------------+-----------------------+
  |   Byte   |          10           |           9           |           8           |          7:0          |
  +----------+-----------------------+-----------------------+-----------------------+-----------------------+
  |Bit Fields|   [7:4]   |   [3:0]   |   [7:4]   |   [3:0]   |   [7:4]   |   [3:0]   |   [7:4]   |   [3:0]   |
  +----------+-----------------------+-----------------------+-----------------------+-----------------------+
  | Ver. 100 |        Size[2]        |        Size[1]        |        Size[0]        |        Address        |
  +----------+-----------------------+-----------------------+-----------------------+-----------------------+
  | Ver. 200 |  FMSMask  |  FMSMask  |    FMS    |    FMS    |    FMS    |    FMS    |        Address        |
  |          |   Family  |   Model   |  ExtModel |    Type   |   Family  |   Model   |                       |
  +----------+-----------------------+-----------------------+-----------------------+-----------------------+

  @param  FitEntry  FIT entry information.
  @param  FMS       Processor ID information.
  @param  FMSMask   Processor ID mask information.

  @retval None
**/
void
GetFMSFromFitEntry (
  IN      FIRMWARE_INTERFACE_TABLE_ENTRY   FitEntry,
  IN OUT  PROCESSOR_ID                     *FMS,
  IN OUT  PROCESSOR_ID                     *FMSMask
  )
{

  FMS->Bits.Family         = (FitEntry.Size[0]  & 0xF0) >> 4;
  FMS->Bits.Model          = (FitEntry.Size[0]  & 0x0F);
  FMS->Bits.ExtendedModel  = (FitEntry.Size[1]  & 0xF0) >> 4;
  FMS->Bits.Type           = (FitEntry.Size[1]  & 0x0F);
  FMS->Bits.ExtendedFamily = (FitEntry.Checksum & 0x0F);

  FMSMask->Bits.Family         = (FitEntry.Size[2]  & 0xF0) >> 4;
  FMSMask->Bits.Model          = (FitEntry.Size[2]  & 0x0F);
  FMSMask->Bits.ExtendedModel  = (FitEntry.Rsvd     & 0xF0) >> 4;
  FMSMask->Bits.Type           = (FitEntry.Rsvd     & 0x0F);
  FMSMask->Bits.ExtendedFamily = (FitEntry.Checksum & 0xF0) >> 4;
}

/**
  Get the FIT table information from Fvrecovery.

  @param FvBuffer         Fvrecovery binary buffer.
  @param FvSize           Fvrecovery size.

  @retval 0 - Fit Table not found
**/
UINT32
GetFitEntryInfo (
  IN UINT8     *FvBuffer,
  IN UINT32    FvSize
  )
{
  FIRMWARE_INTERFACE_TABLE_ENTRY *FitEntry;
  UINT32                          FitEntrySizeValue;
  UINT32                          FitIndex;
  UINT32                          FitTableOffset;
  PROCESSOR_ID                    FMS;
  PROCESSOR_ID                    FMSMask;

  FMS.Uint32     = 0;
  FMSMask.Uint32 = 0;

  //
  // 1. FitPointer
  //
  if (gFitTableContext.FitTablePointerOffset == 0) {
    gFitTableContext.FitTablePointerOffset = DEFAULT_FIT_TABLE_POINTER_OFFSET;
  }
  gFitTableContext.FitTablePointerOffset2 = 0;

  FitTableOffset = *(UINT32 *)(FvBuffer + FvSize - gFitTableContext.FitTablePointerOffset);

  FitEntry = (FIRMWARE_INTERFACE_TABLE_ENTRY *)FLASH_TO_MEMORY(FitTableOffset, FvBuffer, FvSize);
  FitIndex = 0;

  //
  // 2. FitHeader
  //
  if (FitEntry[FitIndex].Address != *(UINT64 *)"_FIT_   ") {
    return 0;
  }
  if (FitEntry[FitIndex].Type != FIT_TABLE_TYPE_HEADER) {
    return 0;
  }
  FitEntrySizeValue = (((UINT32)FitEntry[FitIndex].Size[2]) << 16) + (((UINT32)FitEntry[FitIndex].Size[1]) << 8) + ((UINT32)FitEntry[FitIndex].Size[0]);
  gFitTableContext.FitEntryNumber = FitEntrySizeValue;
  gFitTableContext.FitHeaderVersion = FitEntry[FitIndex].Version;

  //
  // 3. FitEntry
  //
  FitIndex++;
  for (; FitIndex < gFitTableContext.FitEntryNumber; FitIndex++) {
    FitEntrySizeValue = (((UINT32)FitEntry[FitIndex].Size[2]) << 16) + (((UINT32)FitEntry[FitIndex].Size[1]) << 8) + ((UINT32)FitEntry[FitIndex].Size[0]);
    switch (FitEntry[FitIndex].Type) {
    case FIT_TABLE_TYPE_MICROCODE:
      gFitTableContext.Microcode[gFitTableContext.MicrocodeNumber].Address = (UINT32)FitEntry[FitIndex].Address;
      gFitTableContext.MicrocodeVersion                                    = FitEntry[FitIndex].Version;
      gFitTableContext.MicrocodeNumber ++;
      break;
    case FIT_TABLE_TYPE_STARTUP_ACM:
      gFitTableContext.StartupAcm[gFitTableContext.StartupAcmNumber].Address = (UINT32)FitEntry[FitIndex].Address;
      gFitTableContext.StartupAcm[gFitTableContext.StartupAcmNumber].Size    = FitEntrySizeValue;
      gFitTableContext.StartupAcm[gFitTableContext.StartupAcmNumber].Type    = FitEntry[FitIndex].Type;
      gFitTableContext.StartupAcm[gFitTableContext.StartupAcmNumber].Version = FitEntry[FitIndex].Version;

      if (gFitTableContext.StartupAcm[gFitTableContext.StartupAcmNumber].Version == STARTUP_ACM_FIT_ENTRY_200_VERSION) {
        GetFMSFromFitEntry (FitEntry[FitIndex], &FMS, &FMSMask);
        gFitTableContext.StartupAcm[gFitTableContext.StartupAcmNumber].FMS     = FMS.Uint32;
        gFitTableContext.StartupAcm[gFitTableContext.StartupAcmNumber].FMSMask = FMSMask.Uint32;
      }
      gFitTableContext.StartupAcmNumber++;
      break;
    case FIT_TABLE_TYPE_PROT_BOOT_POLICY:
      gFitTableContext.ProtBootPolicy.Address = (UINT32)FitEntry[FitIndex].Address;
      gFitTableContext.ProtBootPolicy.Version = FitEntry[FitIndex].Version;
      gFitTableContext.ProtBootPolicy.Size = GetFirmwareInterfaceTableEntrySize (&FitEntry[FitIndex]);
      break;
    case FIT_TABLE_TYPE_MMC_FW:
      gFitTableContext.MmcFw[gFitTableContext.MmcFwNumber].Address = (UINT32)FitEntry[FitIndex].Address;
      gFitTableContext.MmcFw[gFitTableContext.MmcFwNumber].Version = FitEntry[FitIndex].Version;
      gFitTableContext.MmcFwNumber++;
      break;
    case FIT_TABLE_TYPE_BIOS_MODULE:
      gFitTableContext.BiosModule[gFitTableContext.BiosModuleNumber].Address = (UINT32)FitEntry[FitIndex].Address;
      gFitTableContext.BiosModule[gFitTableContext.BiosModuleNumber].Size    = FitEntrySizeValue * 16;
      gFitTableContext.BiosModuleVersion                                     = FitEntry[FitIndex].Version;
      gFitTableContext.BiosModuleNumber ++;
      break;
    case FIT_TABLE_TYPE_TPM_POLICY:
    case FIT_TABLE_TYPE_TXT_POLICY:
      if (FitEntry[FitIndex].Version == 0) {
        gFitTableContext.PortModule[gFitTableContext.PortModuleNumber].Address = (UINT32)FitEntry[FitIndex].Address;
        gFitTableContext.PortModule[gFitTableContext.PortModuleNumber].Size    = (UINT32)(FitEntry[FitIndex].Address >> 32);
        gFitTableContext.PortModule[gFitTableContext.PortModuleNumber].Version = FitEntry[FitIndex].Version;
        gFitTableContext.PortModule[gFitTableContext.PortModuleNumber].Type    = FitEntry[FitIndex].Type;
        gFitTableContext.PortModuleNumber ++;
        break;
      }
      // Not Port Configure, pass through
    default: // Others
      gFitTableContext.OptionalModule[gFitTableContext.OptionalModuleNumber].Address = (UINT32)FitEntry[FitIndex].Address;
      gFitTableContext.OptionalModule[gFitTableContext.OptionalModuleNumber].Size    = FitEntrySizeValue;
      gFitTableContext.OptionalModule[gFitTableContext.OptionalModuleNumber].Version = FitEntry[FitIndex].Version;
      gFitTableContext.OptionalModule[gFitTableContext.OptionalModuleNumber].Type    = FitEntry[FitIndex].Type;
      gFitTableContext.OptionalModuleNumber ++;
      break;
    }
  }

  return gFitTableContext.FitEntryNumber;
}

/**
  Main function for FitGen.

  @param argc             Number of command line parameters.
  @param argv             Array of pointers to parameter strings.

  @retval STATUS_SUCCESS  Utility exits successfully.
  @retval STATUS_ERROR    Some error occurred during execution.
**/
STATUS
FitGen (
  IN INTN   argc,
  IN CHAR8  **argv
  )
{
  UINT32                      FvRecoveryFileSize;
  UINT8                       *FileBuffer;
  UINT8                       *FileBufferRaw;
  UINTN                       FitEntryNumber;
  UINT8                       *FitTableOffset;
  STATUS                      Status;
  UINT32                      FitTableSize;

  BOOLEAN                     IsFv;
  UINT8                       *FdFileBuffer;
  UINT32                      FdFileSize;

  UINT8                       *AcmBuffer;
  INTN                        Index = 0;
  UINT32                      FixedFitLocation;

  FileBufferRaw = NULL;
  //
  // Step 0: Check FV or FD
  //
  if (((strcmp (argv[1], "-D") == 0) ||
       (strcmp (argv[1], "-d") == 0)) ) {
    IsFv = FALSE;
  } else {
    IsFv = TRUE;
  }

  //
  // Step 1: Read InputFvRecovery.fv data
  //
  if (IsFv) {
    Status = ReadInputFile (argv[1], &FileBuffer, &FvRecoveryFileSize, &FileBufferRaw);
    if (Status != STATUS_SUCCESS) {
      Error (NULL, 0, 0, "Unable to open file", "%s", argv[1]);
      goto exitFunc;
    }
    FdFileBuffer = FileBuffer;
    FdFileSize = FvRecoveryFileSize;
  } else {
    Status = ReadInputFile (argv[2], &FdFileBuffer, &FdFileSize, &FileBufferRaw);
    if (Status != STATUS_SUCCESS) {
      Error (NULL, 0, 0, "Unable to open file", "%s", argv[2]);
      goto exitFunc;
    }

    //
    // Get Fvrecovery information
    //
    FvRecoveryFileSize = GetFvRecoveryInfoFromFd (FdFileBuffer, FdFileSize, &FileBuffer);
    if ((FvRecoveryFileSize == 0) || (FileBuffer == NULL)) {
      Error (NULL, 0, 0, "FvRecovery not found in Fd file!", NULL);
      Status = STATUS_ERROR;
      goto exitFunc;
    }
  }

  //
  // Step 2: Calculate FIT entry number.
  //
  FitEntryNumber = GetFitEntryNumber (argc, argv, FdFileBuffer, FdFileSize);
  if (!gFitTableContext.Clear) {
    if (FitEntryNumber == 0) {
      Status = STATUS_ERROR;
      goto exitFunc;
    }

    //
    // For debug
    //
    PrintFitData ();

    //
    // Add 1 more FitEntry as place holder, because we need exclude FIT table itself
    //
    FitEntryNumber++;
    FitTableSize = FitEntryNumber * sizeof(FIRMWARE_INTERFACE_TABLE_ENTRY);
    FitTableSize += FIT_ALIGNMENT;
    FitTableSize &= ~FIT_ALIGNMENT;

    //
    // Step 3: Get enough space for FIT
    //
    FixedFitLocation = GetFixedFitLocation (argc, argv);
    if (FixedFitLocation != 0 &&
      (FixedFitLocation < TOP_FLASH_ADDRESS - FdFileSize || FixedFitLocation + FitTableSize > TOP_FLASH_ADDRESS)) {
      //
      // FixedFitLocation is out of the input FD region, still find space from FvRecovery
      //
      FixedFitLocation = 0;
      printf ("The fixed FIT location is not in valid flash region, find space from FvRecovery ...\n");
    }

    FitTableOffset = GetFreeSpaceForFit (FileBuffer, FvRecoveryFileSize, FitTableSize, FixedFitLocation);
    if (FitTableOffset == NULL) {
      printf ("Error - FitTableOffset is NULL\n");
      return STATUS_ERROR;
    }

    CheckOverlap (
      MEMORY_TO_FLASH (FitTableOffset, FdFileBuffer, FdFileSize),
      FitTableSize
      );

    //
    // Get ACM buffer
    //
    for (Index = 0; Index < (INTN)gFitTableContext.StartupAcmNumber; Index ++) {
      printf("ACM address:%08x\n", gFitTableContext.StartupAcm[Index].Address);
      printf("ACM size:%08x\n", gFitTableContext.StartupAcm[Index].Size);
      if (gFitTableContext.StartupAcm[Index].Address != 0) {
        printf("get AcmBuffer\n");
        AcmBuffer = FLASH_TO_MEMORY(gFitTableContext.StartupAcm[Index].Address, FdFileBuffer, FdFileSize);
        if ((AcmBuffer < FdFileBuffer) || (AcmBuffer + gFitTableContext.StartupAcm[Index].Size > FdFileBuffer + FdFileSize)) {
          printf ("ACM out of range - can not validate it\n");
          AcmBuffer = NULL;
        }

        if (AcmBuffer != NULL) {
          if (CheckAcm((ACM_FORMAT *)AcmBuffer, gFitTableContext.StartupAcm[Index].Size)) {
            DumpAcm((ACM_FORMAT *)AcmBuffer);

            if (gFitTableContext.StartupAcm[Index].Version >= 0x200) {
              if ((gFitTableContext.StartupAcm[Index].FMS == 0) && (gFitTableContext.StartupAcm[Index].FMSMask == 0)) {
                //
                // FMS and FMSMask is not assigned via -I argument. Get it from ACM
                //
                GetAcmFms((ACM_FORMAT *)AcmBuffer, &gFitTableContext.StartupAcm[Index].FMS, &gFitTableContext.StartupAcm[Index].FMSMask);
                printf("ACM FMS:%08x\n", gFitTableContext.StartupAcm[Index].FMS);
                printf("ACM FMS Mask:%08x\n", gFitTableContext.StartupAcm[Index].FMSMask);
              }
            }
          }
          else {
            if (Index == 0) {
              Status = STATUS_ERROR;
              goto exitFunc;
            }
          }
        }
      }
    }

    //
    // Step 4: Fill the FIT table one by one
    //
    FillFitTable (FdFileBuffer, FdFileSize, FitTableOffset);

    //
    // For debug
    //
    PrintFitTable (FdFileBuffer, FdFileSize);
  } else {
    printf ("Clear FIT table ...\n");
    //
    // Step 3: Get FIT table info
    //
    FitEntryNumber = GetFitEntryInfo (FdFileBuffer, FdFileSize);
    if (FitEntryNumber == 0) {
      Error (NULL, 0, 0, "No FIT table found", NULL);
      return STATUS_ERROR;
    }

    //
    // For debug
    //
    PrintFitTable (FdFileBuffer, FdFileSize);

    //
    // Step 4: Clear FIT table
    //
    ClearFitTable (FdFileBuffer, FdFileSize);
    printf ("Clear FIT table Done!\n");
  }

  //
  // Step 5: Write OutputFvRecovery.fv data
  //
  if (IsFv) {
    Status = WriteOutputFile (argv[2], FileBuffer, FvRecoveryFileSize);
  } else {
    Status = WriteOutputFile (argv[3], FdFileBuffer, FdFileSize);
  }

exitFunc:
  if (FileBufferRaw != NULL) {
    free ((VOID *)FileBufferRaw);
  }
  return Status;
}

/**
  View function for FitGen.

  @param argc             Number of command line parameters.
  @param argv             Array of pointers to parameter strings

  @retval STATUS_SUCCESS  Utility exits successfully.
  @retval STATUS_ERROR    Some error occurred during execution.
**/
STATUS
FitView (
  IN INTN   argc,
  IN CHAR8  **argv
  )
{
  UINT32                        FvRecoveryFileSize;
  UINT8                         *FileBuffer;
  UINT8                         *FileBufferRaw = NULL;
  STATUS                        Status;
  FILE                          *FpIn;
  UINT32                        FlashValidSig = 0;
  UINT32                        Frba;
  UINT32                        BiosRegionBaseOffset;
  FLASH_MAP_0_REGISTER          FlashMap0;
  FLASH_REGION_1_BIOS_REGISTER  FlashRegion1;

  //
  // Step 1: Read input file
  //
  Status = ReadInputFile (argv[2], &FileBuffer, &FvRecoveryFileSize, &FileBufferRaw);
  if (Status != STATUS_SUCCESS) {
    Error (NULL, 0, 0, "Unable to open file", "%s", argv[2]);
    goto exitFunc;
  }

  // no -f option, use default FIT pointer offset
  if (argc == 3) {
    //
    // Use default address
    //
    gFitTableContext.FitTablePointerOffset = DEFAULT_FIT_TABLE_POINTER_OFFSET;
  } else if (stricmp (argv[3], "-f") == 0) {
    if (argc == 5) {
      //
      // Get offset from parameter
      //
      gFitTableContext.FitTablePointerOffset = xtoi (argv[3 + 1]);
    } else {
      Error (NULL, 0, 0, "FIT offset not specified!", NULL);
      Status = STATUS_ERROR;
      goto exitFunc;
    }
  } else {
    Error (NULL, 0, 0, "Invalid view option: ", "%s", argv[3]);
    Status = STATUS_ERROR;
    goto exitFunc;
  }

  //
  //Check the File Path
  //
  if (!CheckPath (argv[2])) {
    Error (NULL, 0, 0, "File path is invalid!", NULL);
    Status = STATUS_ERROR;
    goto exitFunc;
  }
  //
  // Open the Input file
  //
  if ((FpIn = fopen (argv[2], "rb")) == NULL) {
    Error (NULL, 0, 0, "Unable open the file!", NULL);
    Status = STATUS_WARNING;
    goto exitFunc;
  }
  //
  //Seek and read the Flash Valid Signature;
  //
  fseek (FpIn, FLVALSIG_BASE_OFFSET, SEEK_SET);
  fread (&FlashValidSig, 4, 1, FpIn);
  if (FlashValidSig == FLASH_VALID_SIGNATURE) {
    //
    //Seek and read the Flash Map 0 Register;
    //
    fseek (FpIn, FLMAP0_BASE_OFFSET, SEEK_SET);
    fread (&FlashMap0, 4, 1, FpIn);
    Frba = FlashMap0.Frba << 4 & 0xFF0;  //FRBA identifies address bits [11:4] for the region portion of the flashdescriptor, bits [26:12] and bits [3:0] are 0
    //
    //Seek and read the Flash Region 1 (BIOS) Register;
    //
    BiosRegionBaseOffset = Frba + 0x4;
    fseek (FpIn, BiosRegionBaseOffset, SEEK_SET);
    fread (&FlashRegion1, 4, 1, FpIn);
    FileBuffer = (UINT8 *)(FileBuffer + (FlashRegion1.RegionBase << 12));  // RegionBase specifies address bits [26:12] for the Region Base.
    FvRecoveryFileSize = ((FlashRegion1.RegionLimit << 12 | 0xFFF) + 1) - (FlashRegion1.RegionBase << 12);  //RegionLimit specifies bits [26:12] of the ending address for this region, bits [11:0] are assumed to be FFFh.
  }
  //
  // Close the Input file
  //
  fclose (FpIn);

  //
  // For debug
  //
  PrintFitTable (FileBuffer, FvRecoveryFileSize);

exitFunc:
  if (FileBufferRaw != NULL) {
    free ((VOID *)FileBufferRaw);
  }
  return Status;
}

/**
  Main function.

  @param argc             Number of command line parameters.
  @param argv             Array of pointers to parameter strings

  @retval STATUS_SUCCESS  Utility exits successfully.
  @retval STATUS_ERROR    Some error occurred during execution.
**/
int
main (
  int   argc,
  char  **argv
  )
{
  SetUtilityName (UTILITY_NAME);

  //
  // Display utility information
  //
  PrintUtilityInfo ();

  //
  // Verify the correct number of arguments
  //
  if (argc >= MIN_VIEW_ARGS && stricmp (argv[1], "-view") == 0) {
    return FitView (argc, argv);
  } else if (argc >= MIN_ARGS) {
    return FitGen (argc, argv);
  } else {
    Error (NULL, 0, 0, "invalid number of input parameters specified", NULL);
    PrintUsage ();
    return STATUS_ERROR;
  }
}
/**
  Convert hex string to uint

  @param str          The string

  @retval   Integer value

**/
unsigned int
xtoi (
  char  *str
  )
{
  unsigned int u;
  char         c;
  unsigned int m;

  if (str == NULL) {
    return 0;
  }

  m = (unsigned int) -1 >> 4;
  //
  // skip preceeding white space
  //
  while (*str && *str == ' ') {
    str += 1;
  }
  //
  // skip preceeding zeros
  //
  while (*str && *str == '0') {
    str += 1;
  }
  //
  // skip preceeding x/X character
  //
  if (*str && (*str == 'x' || *str == 'X')) {
    str += 1;
  }
  //
  // convert hex digits
  //
  u = 0;
  c = *(str++);
  while (c) {
    if (c >= 'a' && c <= 'f') {
      c -= 'a' - 'A';
    }

    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')) {
      if (u > m) {
        return (unsigned int) -1;
      }

      u = (u << 4) | (c - (c >= 'A' ? 'A' - 10 : '0'));
    } else {
      //
      // Let application exit immediately
      //
      Error (NULL, 0, 0, "Hex value is expected!", NULL);
      exit (0);
      break;
    }

    c = *(str++);
  }

  return u;
}
