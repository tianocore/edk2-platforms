/** @file
  Implements FspExportedInterfaceHob.h

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FSP_EXPORTED_INTERFACE_HOB_H
#define FSP_EXPORTED_INTERFACE_HOB_H
#include <Uefi.h>
#include <FspsUpd.h>
#define MAX_SMBIOS_TABLE_COUNT     20
#define MAX_ACPI_SSDT_TABLE_COUNT  9

#define FSP_TO_BOOTLOADER
#define BOOTLOADER_TO_FSP
#define IMPROPRIATE_ARCH

typedef VOID  (EFIAPI *FSP_VIRTUAL_ADDRESS_CHANGE_CALLBACK)(FSPS_UPD *NewUpdAddress);
typedef VOID  *(EFIAPI *BOOTLOADER_CONVERT_POINTER)(VOID *In);

// Use "placeholder" for structure coherence under different CPU modes.
// The GUID of HOB.
extern EFI_GUID  gFspExportedInterfaceHobGuid;

// Include goes here.

#ifndef MDE_CPU_X64
  #include <Ppi/Reset2.h>
#else
  #include <Protocol/SmbusHc.h>
  #include <Protocol/SmmAccess2.h>
  #include <Protocol/SmmControl2.h>
  #include <Protocol/SmmBase2.h>
  #include <Protocol/SmmCommunication.h>
  #include <Protocol/MmCommunication2.h>
  #include <Protocol/HiiDatabase.h>
  #include <Protocol/HiiConfigRouting.h>
  #include <Protocol/HiiString.h>
  #include <Protocol/PciIo.h>
  #include <Protocol/AmdPspFtpmProtocol.h>
  #include <Uefi/UefiSpec.h>
#endif

#pragma pack (push,1)
typedef struct _FSP_EXPORTED_INTERFACE_HOB FSP_EXPORTED_INTERFACE_HOB;
#ifndef MDE_CPU_X64
struct _FSP_EXPORTED_INTERFACE_HOB {
  BOOTLOADER_TO_FSP VOID                  *SmmDriverVolume;
  BOOTLOADER_TO_FSP UINT32                SmmDriverVolumeSize;
  FSP_TO_BOOTLOADER VOID                  *PspFtpmPpi;
  FSP_TO_BOOTLOADER VOID                  *PspFtpmFactoryResetPpi;
  FSP_TO_BOOTLOADER EFI_PEI_RESET2_PPI    *Reset2Ppi;
  IMPROPRIATE_ARCH UINT64                 SmbusProtocol;
  IMPROPRIATE_ARCH UINT64                 SmmAccessProtocol;
  IMPROPRIATE_ARCH UINT64                 SmmControl2Protocol;
  IMPROPRIATE_ARCH UINT64                 PspCommonServiceProtocol;
  IMPROPRIATE_ARCH UINT64                 ApobCommonServiceProtocol;
  IMPROPRIATE_ARCH UINT64                 ApcbDxeServiceProtocol;
  IMPROPRIATE_ARCH UINT64                 SmmBase2Protocol;
  IMPROPRIATE_ARCH UINT64                 SmmCommunicationProtocol;
  IMPROPRIATE_ARCH UINT64                 MmCommunication2Protocol;
  IMPROPRIATE_ARCH UINT64                 FchResetSystem;
  IMPROPRIATE_ARCH UINT64                 PcdAmdSmmCommunicationAddress;
  IMPROPRIATE_ARCH UINT64                 PcdAmdS3LibPrivateDataAddress;
  IMPROPRIATE_ARCH UINT64                 PcdAmdS3LibTableAddress;
  IMPROPRIATE_ARCH UINT64                 PcdAmdS3LibTableSize;
  IMPROPRIATE_ARCH UINT64                 SmbiosPointers[MAX_SMBIOS_TABLE_COUNT];
  IMPROPRIATE_ARCH UINT64                 AcpiSsdtTables[MAX_ACPI_SSDT_TABLE_COUNT];
  IMPROPRIATE_ARCH UINT64                 AcpiTpm2Table;
  IMPROPRIATE_ARCH UINT64                 AcpiCratTable;
  IMPROPRIATE_ARCH UINT64                 AcpiCditTable;
  IMPROPRIATE_ARCH UINT64                 AcpiIvrsTable;
  IMPROPRIATE_ARCH UINT64                 VirtualAddressChangeCallback;
  IMPROPRIATE_ARCH UINT64                 FinalMemoryMap;
  IMPROPRIATE_ARCH UINT64                 FinalMemoryMapSize;
  IMPROPRIATE_ARCH UINT64                 FinalMemoryDescriptorSize;
  IMPROPRIATE_ARCH UINT64                 ConvertPointer;
  IMPROPRIATE_ARCH UINT64                 ExportedInterfaceHobAddressAfterNotifyPhase;
  IMPROPRIATE_ARCH UINT64                 PspPlatformProtocol;
  IMPROPRIATE_ARCH UINT64                 GetVariable;
  IMPROPRIATE_ARCH UINT64                 GetNextVariableName;
  IMPROPRIATE_ARCH UINT64                 QueryVariableInfo;
  IMPROPRIATE_ARCH UINT64                 SetVariable;
  IMPROPRIATE_ARCH UINT64                 HiiProtocol;
  IMPROPRIATE_ARCH UINT64                 HiiStringProtocol;
  IMPROPRIATE_ARCH UINT64                 HiiConfigRoutingProtocol;
  IMPROPRIATE_ARCH UINT64                 S3BootScriptTablePrivateSmmDataPtr;
  IMPROPRIATE_ARCH UINT64                 S3BootScriptTablePrivateDataPtr;
  IMPROPRIATE_ARCH UINT64                 EfiPciIoProtocol;
  IMPROPRIATE_ARCH UINT64                 EfiPciIoProtocolCount;
  IMPROPRIATE_ARCH UINT64                 PspFtpmProtocol;
};

#else
struct _FSP_EXPORTED_INTERFACE_HOB {
  IMPROPRIATE_ARCH UINT32                                  SmmDriverVolume;
  IMPROPRIATE_ARCH UINT32                                  SmmDriverVolumeSize;
  IMPROPRIATE_ARCH UINT32                                  PspFtpmPpi;
  IMPROPRIATE_ARCH UINT32                                  PspFtpmFactoryResetPpi;
  IMPROPRIATE_ARCH UINT32                                  Reset2Ppi;
  FSP_TO_BOOTLOADER EFI_SMBUS_HC_PROTOCOL                  *SmbusProtocol;
  FSP_TO_BOOTLOADER EFI_SMM_ACCESS2_PROTOCOL               *SmmAccessProtocol;
  FSP_TO_BOOTLOADER EFI_SMM_CONTROL2_PROTOCOL              *SmmControl2Protocol;
  FSP_TO_BOOTLOADER VOID                                   *PspCommonServiceProtocol;
  FSP_TO_BOOTLOADER VOID                                   *ApobCommonServiceProtocol;
  FSP_TO_BOOTLOADER VOID                                   *ApcbDxeServiceProtocol;
  FSP_TO_BOOTLOADER EFI_SMM_BASE2_PROTOCOL                 *SmmBase2Protocol;
  FSP_TO_BOOTLOADER EFI_SMM_COMMUNICATION_PROTOCOL         *SmmCommunicationProtocol;
  FSP_TO_BOOTLOADER EFI_MM_COMMUNICATION2_PROTOCOL         *MmCommunication2Protocol;
  FSP_TO_BOOTLOADER EFI_RESET_SYSTEM                       FchResetSystem;
  FSP_TO_BOOTLOADER UINT64                                 PcdAmdSmmCommunicationAddress;
  FSP_TO_BOOTLOADER UINT64                                 PcdAmdS3LibPrivateDataAddress;
  FSP_TO_BOOTLOADER UINT64                                 PcdAmdS3LibTableAddress;
  FSP_TO_BOOTLOADER UINT64                                 PcdAmdS3LibTableSize;
  FSP_TO_BOOTLOADER VOID                                   *SmbiosPointers[MAX_SMBIOS_TABLE_COUNT];
  FSP_TO_BOOTLOADER VOID                                   *AcpiSsdtTables[MAX_ACPI_SSDT_TABLE_COUNT];
  FSP_TO_BOOTLOADER VOID                                   *AcpiTpm2Table;
  FSP_TO_BOOTLOADER VOID                                   *AcpiCratTable;
  FSP_TO_BOOTLOADER VOID                                   *AcpiCditTable;
  FSP_TO_BOOTLOADER VOID                                   *AcpiIvrsTable;
  FSP_TO_BOOTLOADER FSP_VIRTUAL_ADDRESS_CHANGE_CALLBACK    VirtualAddressChangeCallback;
  FSP_TO_BOOTLOADER VOID                                   *FinalMemoryMap;
  FSP_TO_BOOTLOADER UINT64                                 FinalMemoryMapSize;
  FSP_TO_BOOTLOADER UINT64                                 FinalMemoryDescriptorSize;
  BOOTLOADER_TO_FSP BOOTLOADER_CONVERT_POINTER             ConvertPointer;
  FSP_TO_BOOTLOADER FSP_EXPORTED_INTERFACE_HOB             *ExportedInterfaceHobAddressAfterNotifyPhase;
  BOOTLOADER_TO_FSP VOID                                   *PspPlatformProtocol;
  BOOTLOADER_TO_FSP EFI_GET_VARIABLE                       GetVariable;
  BOOTLOADER_TO_FSP EFI_GET_NEXT_VARIABLE_NAME             GetNextVariableName;
  BOOTLOADER_TO_FSP EFI_QUERY_VARIABLE_INFO                QueryVariableInfo;
  BOOTLOADER_TO_FSP EFI_SET_VARIABLE                       SetVariable;
  BOOTLOADER_TO_FSP EFI_HII_DATABASE_PROTOCOL              *HiiProtocol;
  BOOTLOADER_TO_FSP EFI_HII_STRING_PROTOCOL                *HiiStringProtocol;
  BOOTLOADER_TO_FSP EFI_HII_CONFIG_ROUTING_PROTOCOL        *HiiConfigRoutingProtocol;
  FSP_TO_BOOTLOADER UINT64                                 S3BootScriptTablePrivateSmmDataPtr;
  FSP_TO_BOOTLOADER UINT64                                 S3BootScriptTablePrivateDataPtr;
  BOOTLOADER_TO_FSP EFI_PCI_IO_PROTOCOL                    **EfiPciIoProtocol;
  BOOTLOADER_TO_FSP UINT64                                 EfiPciIoProtocolCount;
  FSP_TO_BOOTLOADER PSP_FTPM_PROTOCOL                      *PspFtpmProtocol;
};

#endif
#pragma pack (pop)
#endif
