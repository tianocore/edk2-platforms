/** @file
  OemMiscLib implementation for Allwinner H618 SoC.

  Copyright (c) 2026, Khai Do <dovankhai97@gmail.com>. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <AllwinnerH618.h>
#include <IndustryStandard/SmBios.h>
#include <Protocol/Smbios.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OemMiscLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define H618_MAX_SOCKETS         1U
#define H618_CPU_MAX_FREQ_MHZ    1512U
#define H618_CPU_MAX_FREQ_HZ     1512000000ULL
#define SID_POLL_TIMEOUT         10000U

/**
  Read a 32-bit word from the Allwinner H618 Security ID (SID) eFuse controller.

  @param[in] Offset  Byte offset within SID eFuse space.

  @return 32-bit eFuse word value.
**/
STATIC
UINT32
H618ReadSidWord (
  IN UINT32  Offset
  )
{
  UINT32  PrctlVal;
  UINT32  Timeout;
  UINT32  KeyVal;

  PrctlVal = ((Offset & 0x1FFU) << 16) | AW_H618_SID_OP_LOCK | AW_H618_SID_READ_TRIG;
  MmioWrite32 (AW_H618_SID_BASE + AW_H618_SID_PRCTL_OFFSET, PrctlVal);

  for (Timeout = 0; Timeout < SID_POLL_TIMEOUT; Timeout++) {
    if ((MmioRead32 (AW_H618_SID_BASE + AW_H618_SID_PRCTL_OFFSET) & AW_H618_SID_READ_TRIG) == 0) {
      break;
    }
  }

  KeyVal = MmioRead32 (AW_H618_SID_BASE + AW_H618_SID_RDKEY_OFFSET);
  MmioWrite32 (AW_H618_SID_BASE + AW_H618_SID_PRCTL_OFFSET, 0x0);

  return KeyVal;
}

/**
  Returns whether the specified processor socket is present or not.

  @param[in] ProcessorIndex  The processor socket index to check.

  @return TRUE if the processor is present, FALSE otherwise.
**/
BOOLEAN
EFIAPI
OemIsProcessorPresent (
  IN UINTN  ProcessorIndex
  )
{
  return (ProcessorIndex < H618_MAX_SOCKETS);
}

/**
  Gets the CPU frequency of the specified processor.

  @param[in] ProcessorIndex  Index of the processor to get the frequency for.

  @return CPU frequency in Hz.
**/
UINTN
EFIAPI
OemGetCpuFreq (
  IN UINT8  ProcessorIndex
  )
{
  return H618_CPU_MAX_FREQ_HZ;
}

/**
  Gets information about the specified processor and stores it in
  the structures provided.

  @param[in]     ProcessorIndex            Index of the processor to get the information for.
  @param[in,out] ProcessorStatus           Processor status.
  @param[in,out] ProcessorCharacteristics  Processor characteristics.
  @param[in,out] MiscProcessorData         Miscellaneous processor information.

  @return TRUE on success, FALSE on failure.
**/
BOOLEAN
EFIAPI
OemGetProcessorInformation (
  IN     UINTN                           ProcessorIndex,
  IN OUT PROCESSOR_STATUS_DATA           *ProcessorStatus,
  IN OUT PROCESSOR_CHARACTERISTIC_FLAGS  *ProcessorCharacteristics,
  IN OUT OEM_MISC_PROCESSOR_DATA         *MiscProcessorData
  )
{
  UINT16  CoreCount;

  CoreCount = (UINT16)FixedPcdGet32 (PcdCoreCount);

  if (ProcessorIndex < H618_MAX_SOCKETS) {
    ProcessorStatus->Bits.CpuStatus       = 1;
    ProcessorStatus->Bits.Reserved1       = 0;
    ProcessorStatus->Bits.SocketPopulated = 1;
    ProcessorStatus->Bits.Reserved2       = 0;
  } else {
    ProcessorStatus->Bits.CpuStatus       = 0;
    ProcessorStatus->Bits.Reserved1       = 0;
    ProcessorStatus->Bits.SocketPopulated = 0;
    ProcessorStatus->Bits.Reserved2       = 0;
  }

  ProcessorCharacteristics->ProcessorReserved1              = 0;
  ProcessorCharacteristics->ProcessorUnknown                = 0;
  ProcessorCharacteristics->Processor64BitCapable           = 1;
  ProcessorCharacteristics->ProcessorMultiCore              = 1;
  ProcessorCharacteristics->ProcessorHardwareThread         = 0;
  ProcessorCharacteristics->ProcessorExecuteProtection      = 1;
  ProcessorCharacteristics->ProcessorEnhancedVirtualization = 1;
  ProcessorCharacteristics->ProcessorPowerPerformanceCtrl   = 1;
  ProcessorCharacteristics->Processor128BitCapable          = 0;
  ProcessorCharacteristics->ProcessorArm64SocId             = 0;
  ProcessorCharacteristics->ProcessorReserved2              = 0;

  MiscProcessorData->Voltage       = 0x89;  // 0.9V nominal VDD_CPU
  MiscProcessorData->CurrentSpeed  = H618_CPU_MAX_FREQ_MHZ;
  MiscProcessorData->MaxSpeed      = H618_CPU_MAX_FREQ_MHZ;
  MiscProcessorData->ExternalClock = 24;    // 24 MHz DCXO HOSC
  MiscProcessorData->CoreCount     = CoreCount;
  MiscProcessorData->CoresEnabled  = CoreCount;
  MiscProcessorData->ThreadCount   = CoreCount;

  return TRUE;
}

/**
  Gets the maximum number of processor sockets supported by the platform.

  @return The maximum number of processor sockets.
**/
UINT8
EFIAPI
OemGetMaxProcessors (
  VOID
  )
{
  return H618_MAX_SOCKETS;
}

/**
  Gets information about the cache at the specified cache level.

  @param[in]     ProcessorIndex    The processor to get information for.
  @param[in]     CacheLevel        The cache level to get information for.
  @param[in]     DataCache         Whether the cache is a data cache.
  @param[in]     UnifiedCache      Whether the cache is a unified cache.
  @param[in,out] SmbiosCacheTable  The SMBIOS Type7 cache information structure.

  @return TRUE on success, FALSE on failure.
**/
BOOLEAN
EFIAPI
OemGetCacheInformation (
  IN     UINT8               ProcessorIndex,
  IN     UINT8               CacheLevel,
  IN     BOOLEAN             DataCache,
  IN     BOOLEAN             UnifiedCache,
  IN OUT SMBIOS_TABLE_TYPE7  *SmbiosCacheTable
  )
{
  SmbiosCacheTable->CacheConfiguration = (UINT16)(CacheLevel - 1U);

  if ((CacheLevel == 1U) && !DataCache && !UnifiedCache) {
    // L1 Instruction Cache (Write-through / operational mode)
    SmbiosCacheTable->CacheConfiguration |= (0x80U | (3U << 8));
  } else {
    // L1 Data Cache and L2 Unified Cache (Write-back enabled)
    SmbiosCacheTable->CacheConfiguration |= (0x80U | (1U << 8));
  }

  return TRUE;
}

/**
  Gets the type of chassis for the system.

  @return The type of the chassis.
**/
MISC_CHASSIS_TYPE
EFIAPI
OemGetChassisType (
  VOID
  )
{
  return MiscChassisTypeLowProfileDesktop;
}

STATIC BOOLEAN  mMemoryTablesInstalled = FALSE;

/**
  Install SMBIOS Type 16: Physical Memory Array.

  @param[in]  Smbios        Pointer to the SMBIOS protocol.
  @param[out] Type16Handle  Handle assigned to the Type 16 table.

  @retval EFI_SUCCESS       Table installed successfully.
  @retval other             Error status.
**/
STATIC
EFI_STATUS
H618InstallSmbiosType16 (
  IN  EFI_SMBIOS_PROTOCOL  *Smbios,
  OUT EFI_SMBIOS_HANDLE    *Type16Handle
  )
{
  SMBIOS_TABLE_TYPE16  *Record;
  UINTN                Size;
  EFI_STATUS           Status;
  EFI_SMBIOS_HANDLE    Handle;
  UINT64               MemorySize;

  // Round up to physical DRAM size (e.g. 1024 MB)
  MemorySize = (FixedPcdGet64 (PcdSystemMemorySize) + 0x000FFFFFULL) & ~0x000FFFFFULL;
  Size       = sizeof (SMBIOS_TABLE_TYPE16) + 2;
  Record     = AllocateZeroPool (Size);
  if (Record == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Record->Hdr.Type                     = EFI_SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY;
  Record->Hdr.Length                   = sizeof (SMBIOS_TABLE_TYPE16);
  Record->Location                     = MemoryArrayLocationSystemBoard;
  Record->Use                          = MemoryArrayUseSystemMemory;
  Record->MemoryErrorCorrection        = MemoryErrorCorrectionNone;
  Record->MaximumCapacity              = (UINT32)(MemorySize / SIZE_1KB);
  Record->MemoryErrorInformationHandle = 0xFFFE;
  Record->NumberOfMemoryDevices        = 1;

  Handle = SMBIOS_HANDLE_PI_RESERVED;
  Status = Smbios->Add (Smbios, NULL, &Handle, (EFI_SMBIOS_TABLE_HEADER *)Record);
  if (!EFI_ERROR (Status)) {
    *Type16Handle = Handle;
  }

  FreePool (Record);
  return Status;
}

/**
  Install SMBIOS Type 17: Memory Device.

  @param[in] Smbios        Pointer to the SMBIOS protocol.
  @param[in] Type16Handle  Handle of the parent Type 16 table.

  @retval EFI_SUCCESS      Table installed successfully.
  @retval other            Error status.
**/
STATIC
EFI_STATUS
H618InstallSmbiosType17 (
  IN EFI_SMBIOS_PROTOCOL  *Smbios,
  IN EFI_SMBIOS_HANDLE    Type16Handle
  )
{
  SMBIOS_TABLE_TYPE17  *Record;
  UINTN                Size;
  EFI_STATUS           Status;
  EFI_SMBIOS_HANDLE    Handle;
  UINT64               MemorySize;
  CHAR8                *Str;
  CONST CHAR8          *Str1;
  CONST CHAR8          *Str2;
  UINTN                Str1Len;
  UINTN                Str2Len;

  Str1       = "DRAM";
  Str2       = "Allwinner";
  // Round up to physical DRAM size (e.g. 1024 MB)
  MemorySize = (FixedPcdGet64 (PcdSystemMemorySize) + 0x000FFFFFULL) & ~0x000FFFFFULL;
  Str1Len    = AsciiStrLen (Str1) + 1;
  Str2Len    = AsciiStrLen (Str2) + 1;
  Size       = sizeof (SMBIOS_TABLE_TYPE17) + Str1Len + Str2Len + 1;

  Record = AllocateZeroPool (Size);
  if (Record == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Record->Hdr.Type                     = EFI_SMBIOS_TYPE_MEMORY_DEVICE;
  Record->Hdr.Length                   = sizeof (SMBIOS_TABLE_TYPE17);
  Record->MemoryArrayHandle            = Type16Handle;
  Record->MemoryErrorInformationHandle = 0xFFFE;
  Record->TotalWidth                   = 32;
  Record->DataWidth                    = 32;
  Record->Size                         = (UINT16)(MemorySize / SIZE_1MB);
  Record->FormFactor                   = MemoryFormFactorChip;
  Record->DeviceLocator                = 1;
  Record->MemoryType                   = MemoryTypeLpddr4;
  Record->TypeDetail.Synchronous       = 1;
  Record->Speed                        = 1600;
  Record->Manufacturer                 = 2;
  Record->ConfiguredMemoryClockSpeed   = 1600;
  Record->MemoryTechnology             = MemoryTechnologyDram;
  Record->MemoryOperatingModeCapability.Bits.VolatileMemory = 1;
  Record->VolatileSize                 = MemorySize;

  Str = (CHAR8 *)Record + sizeof (SMBIOS_TABLE_TYPE17);
  CopyMem (Str, Str1, Str1Len);
  Str += Str1Len;
  CopyMem (Str, Str2, Str2Len);
  Str += Str2Len;
  *Str = '\0';

  Handle = SMBIOS_HANDLE_PI_RESERVED;
  Status = Smbios->Add (Smbios, NULL, &Handle, (EFI_SMBIOS_TABLE_HEADER *)Record);
  FreePool (Record);
  return Status;
}

/**
  Install SMBIOS Type 19: Memory Array Mapped Address.

  @param[in] Smbios        Pointer to the SMBIOS protocol.
  @param[in] Type16Handle  Handle of the parent Type 16 table.

  @retval EFI_SUCCESS      Table installed successfully.
  @retval other            Error status.
**/
STATIC
EFI_STATUS
H618InstallSmbiosType19 (
  IN EFI_SMBIOS_PROTOCOL  *Smbios,
  IN EFI_SMBIOS_HANDLE    Type16Handle
  )
{
  SMBIOS_TABLE_TYPE19  *Record;
  UINTN                Size;
  EFI_STATUS           Status;
  EFI_SMBIOS_HANDLE    Handle;
  UINT64               MemoryBase;
  UINT64               MemorySize;
  UINT64               StartKb;
  UINT64               EndKb;

  // Round base and size to physical DRAM boundary (e.g. 0x40000000, 1024 MB)
  MemoryBase = FixedPcdGet64 (PcdSystemMemoryBase) & ~0x000FFFFFULL;
  MemorySize = (FixedPcdGet64 (PcdSystemMemorySize) + 0x000FFFFFULL) & ~0x000FFFFFULL;
  if (MemorySize < SIZE_1MB) {
    return EFI_INVALID_PARAMETER;
  }

  StartKb = MemoryBase / SIZE_1KB;
  EndKb   = StartKb + (MemorySize / SIZE_1KB) - 1;

  Size   = sizeof (SMBIOS_TABLE_TYPE19) + 2;
  Record = AllocateZeroPool (Size);
  if (Record == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Record->Hdr.Type          = EFI_SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS;
  Record->Hdr.Length        = sizeof (SMBIOS_TABLE_TYPE19);
  Record->StartingAddress   = (UINT32)StartKb;
  Record->EndingAddress     = (UINT32)EndKb;
  Record->MemoryArrayHandle = Type16Handle;
  Record->PartitionWidth    = 1;

  Handle = SMBIOS_HANDLE_PI_RESERVED;
  Status = Smbios->Add (Smbios, NULL, &Handle, (EFI_SMBIOS_TABLE_HEADER *)Record);
  FreePool (Record);
  return Status;
}

/**
  Install SMBIOS Type 16, 17, and 19 memory tables if not yet installed.
**/
STATIC
VOID
H618InstallMemorySmbiosTables (
  VOID
  )
{
  EFI_STATUS               Status;
  EFI_SMBIOS_PROTOCOL      *Smbios;
  EFI_SMBIOS_HANDLE        Type16Handle;
  EFI_SMBIOS_HANDLE        SmbiosHandle;
  EFI_SMBIOS_TYPE          Type;
  EFI_SMBIOS_TABLE_HEADER  *Record;

  if (mMemoryTablesInstalled) {
    return;
  }

  Status = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid, NULL, (VOID **)&Smbios);
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Prevent duplicate installation if Type 16 record already exists
  //
  Type         = EFI_SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY;
  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  Status       = Smbios->GetNext (Smbios, &SmbiosHandle, &Type, &Record, NULL);
  if (!EFI_ERROR (Status)) {
    mMemoryTablesInstalled = TRUE;
    return;
  }

  mMemoryTablesInstalled = TRUE;

  Type16Handle = SMBIOS_HANDLE_PI_RESERVED;
  Status       = H618InstallSmbiosType16 (Smbios, &Type16Handle);
  if (!EFI_ERROR (Status)) {
    H618InstallSmbiosType17 (Smbios, Type16Handle);
    H618InstallSmbiosType19 (Smbios, Type16Handle);
  }
}

/**
  Updates the HII string for the specified SMBIOS field.

  @param[in] HiiHandle      The HII handle.
  @param[in] TokenToUpdate  The string to update.
  @param[in] Field          The field to get information about.
**/
VOID
EFIAPI
OemUpdateSmbiosInfo (
  IN EFI_HII_HANDLE                    HiiHandle,
  IN EFI_STRING_ID                     TokenToUpdate,
  IN OEM_MISC_SMBIOS_HII_STRING_FIELD  Field
  )
{
  CHAR16  *String;
  CHAR16  SidSerialBuffer[36];
  UINT32  Word0;
  UINT32  Word1;
  UINT32  Word2;
  UINT32  Word3;

  if (Field == SystemManufacturerType01) {
    H618InstallMemorySmbiosTables ();
  }

  String = NULL;

  switch (Field) {
    case SystemManufacturerType01:
    case BoardManufacturerType02:
    case ManufacturerType03:
      String = L"Xunlong Software CO.,Limited";
      break;

    case ProductNameType01:
    case ProductNameType02:
      String = L"Orange Pi Zero 2W";
      break;

    case VersionType01:
    case VersionType02:
    case VersionType03:
      String = L"1.0";
      break;

    case SkuNumberType01:
    case SkuNumberType02:
    case SkuNumberType03:
      String = L"OPi-Zero2W-H618";
      break;

    case FamilyType01:
      String = L"Orange Pi";
      break;

    case AssetTagType02:
    case AssetTagType03:
      String = L"OrangePi-Zero2W";
      break;

    case ChassisLocationType02:
      String = L"On Board";
      break;

    case ProcessorPartNumType04:
      String = L"Allwinner H618";
      break;

    case ProcessorVersionType04:
      String = L"Quad-Core ARM Cortex-A53";
      break;

    case ProcessorSocketTypeType04:
      String = L"BGA284";
      break;

    case SerialNumType01:
    case SerialNumberType02:
    case SerialNumberType03:
    case ProcessorSerialNumType04:
      Word0 = H618ReadSidWord (0x00);
      Word1 = H618ReadSidWord (0x04);
      Word2 = H618ReadSidWord (0x08);
      Word3 = H618ReadSidWord (0x0C);
      UnicodeSPrint (
        SidSerialBuffer,
        sizeof (SidSerialBuffer),
        L"%08X%08X%08X%08X",
        Word0,
        Word1,
        Word2,
        Word3
        );
      String = SidSerialBuffer;
      break;

    default:
      String = NULL;
      break;
  }

  if (String != NULL) {
    HiiSetString (HiiHandle, TokenToUpdate, String, NULL);
  }
}

/**
  Fetches the Type 32 boot information status.

  @return Boot status.
**/
MISC_BOOT_INFORMATION_STATUS_DATA_TYPE
EFIAPI
OemGetBootStatus (
  VOID
  )
{
  return BootInformationStatusNoError;
}

/**
  Fetches the chassis status when it was last booted.

  @return Chassis status.
**/
MISC_CHASSIS_STATE
EFIAPI
OemGetChassisBootupState (
  VOID
  )
{
  return ChassisStateSafe;
}

/**
  Fetches the chassis power supply/supplies status when last booted.

  @return Chassis power supply/supplies status.
**/
MISC_CHASSIS_STATE
EFIAPI
OemGetChassisPowerSupplyState (
  VOID
  )
{
  return ChassisStateSafe;
}

/**
  Fetches the chassis thermal status when last booted.

  @return Chassis thermal status.
**/
MISC_CHASSIS_STATE
EFIAPI
OemGetChassisThermalState (
  VOID
  )
{
  return ChassisStateSafe;
}

/**
  Fetches the chassis security status when last booted.

  @return Chassis security status.
**/
MISC_CHASSIS_SECURITY_STATE
EFIAPI
OemGetChassisSecurityStatus (
  VOID
  )
{
  return ChassisSecurityStatusNone;
}

/**
  Fetches the chassis height in RMUs (Rack Mount Units).

  @return The height of the chassis.
**/
UINT8
EFIAPI
OemGetChassisHeight (
  VOID
  )
{
  return 1U;
}

/**
  Fetches the number of power cords.

  @return The number of power cords.
**/
UINT8
EFIAPI
OemGetChassisNumPowerCords (
  VOID
  )
{
  return 1U;
}

/**
  Fetches the system UUID derived from the 128-bit Allwinner H618 SID eFuse.

  @param[out] SystemUuid  The pointer to the buffer to store the System UUID.
**/
VOID
EFIAPI
OemGetSystemUuid (
  OUT GUID  *SystemUuid
  )
{
  UINT32  Word0;
  UINT32  Word1;
  UINT32  Word2;
  UINT32  Word3;

  if (SystemUuid == NULL) {
    return;
  }

  Word0 = H618ReadSidWord (0x00);
  Word1 = H618ReadSidWord (0x04);
  Word2 = H618ReadSidWord (0x08);
  Word3 = H618ReadSidWord (0x0C);

  SystemUuid->Data1    = Word0;
  SystemUuid->Data2    = (UINT16)(Word1 >> 16);
  SystemUuid->Data3    = (UINT16)(Word1 & 0xFFFFU);
  SystemUuid->Data4[0] = (UINT8)(Word2 >> 24);
  SystemUuid->Data4[1] = (UINT8)(Word2 >> 16);
  SystemUuid->Data4[2] = (UINT8)(Word2 >> 8);
  SystemUuid->Data4[3] = (UINT8)(Word2 & 0xFFU);
  SystemUuid->Data4[4] = (UINT8)(Word3 >> 24);
  SystemUuid->Data4[5] = (UINT8)(Word3 >> 16);
  SystemUuid->Data4[6] = (UINT8)(Word3 >> 8);
  SystemUuid->Data4[7] = (UINT8)(Word3 & 0xFFU);
}

/**
  Fetches the BIOS release.

  @return The BIOS release.
**/
UINT16
EFIAPI
OemGetBiosRelease (
  VOID
  )
{
  return FixedPcdGet16 (PcdSystemBiosRelease);
}

/**
  Fetches the embedded controller firmware release.

  @return The embedded controller firmware release.
**/
UINT16
EFIAPI
OemGetEmbeddedControllerFirmwareRelease (
  VOID
  )
{
  return FixedPcdGet16 (PcdEmbeddedControllerFirmwareRelease);
}
