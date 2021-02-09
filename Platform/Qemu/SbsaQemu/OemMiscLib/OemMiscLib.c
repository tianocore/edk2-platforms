/** @file
*  OemMiscLib.c
*
*  Copyright (c) 2021, NUVIA Inc. All rights reserved.
*  Copyright (c) 2020, Linaro Ltd. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/IoLib.h>
#include <Library/OemMiscLib.h>
#include <Library/PcdLib.h>
#include <Library/SerialPortLib.h>
#include <Library/TimerLib.h>
#include <libfdt.h>

/** Returns whether the specified processor is present or not.

  @param ProcessIndex The processor index to check.

  @return TRUE is the processor is present, FALSE otherwise.
**/
BOOLEAN
OemIsSocketPresent (
  UINTN ProcessorIndex
  )
{
  if (ProcessorIndex == 0) {
    return TRUE;
  }

  return FALSE;
}

/** Gets the CPU frequency of the specified processor.

  @param ProcessorIndex Index of the processor to get the frequency for.

  @return               CPU frequency in Hz
**/
UINTN OemGetCpuFreq (
  UINT8 ProcessorIndex
  )
{
  return 2000000000; // 2 GHz
}


/** Walks through the Device Tree created by Qemu and counts the number
    of CPUs present in it.

   Copied from Silicon/Qemu/SbsaQemu/Drivers/SbsaQemuAcpiDxe/SbsaQemuAcpiDxe.c

    @return The number of CPUs present.
**/
UINT16
CountCpusFromFdt (
  VOID
)
{
  VOID   *DeviceTreeBase;
  INT32  Node;
  INT32  Prev;
  INT32  CpuNode;
  INT32  CpuCount;

  DeviceTreeBase = (VOID *)(UINTN)PcdGet64 (PcdDeviceTreeBaseAddress);
  ASSERT (DeviceTreeBase != NULL);

  // Make sure we have a valid device tree blob
  ASSERT (fdt_check_header (DeviceTreeBase) == 0);

  CpuNode = fdt_path_offset (DeviceTreeBase, "/cpus");
  if (CpuNode <= 0) {
    DEBUG ((DEBUG_ERROR, "Unable to locate /cpus in device tree\n"));
    return 0;
  }

  CpuCount = 0;

  // Walk through /cpus node and count the number of subnodes.
  // The count of these subnodes corresponds to the number of
  // CPUs created by Qemu.
  Prev = fdt_first_subnode (DeviceTreeBase, CpuNode);
  while (1) {
    CpuCount++;
    Node = fdt_next_subnode (DeviceTreeBase, Prev);
    if (Node < 0) {
      break;
    }
    Prev = Node;
  }

  return CpuCount;
}

/** Gets information about the specified processor and stores it in
    the structures provided.

  @param ProcessorIndex  Index of the processor to get the information for.
  @param ProcessorStatus Processor status.
  @param ProcessorCharacteristics Processor characteritics.
  @param MiscProcessorData        Miscellaneous processor information.

  @return  TRUE on success, FALSE on failure.
**/
BOOLEAN
OemGetProcessorInformation (
  IN  UINTN                             ProcessorIndex,
  IN OUT PROCESSOR_STATUS_DATA          *ProcessorStatus,
  IN OUT PROCESSOR_CHARACTERISTIC_FLAGS *ProcessorCharacteristics,
  IN OUT OEM_MISC_PROCESSOR_DATA        *MiscProcessorData
  )
{
  UINT16 CoreCount = CountCpusFromFdt ();

  if (ProcessorIndex == 0) {
    ProcessorStatus->Bits.CpuStatus       = 1; // CPU enabled
    ProcessorStatus->Bits.Reserved1       = 0;
    ProcessorStatus->Bits.SocketPopulated = 1; 
    ProcessorStatus->Bits.Reserved2       = 0;
  } else {
    ProcessorStatus->Bits.CpuStatus       = 0; // CPU disabled
    ProcessorStatus->Bits.Reserved1       = 0;
    ProcessorStatus->Bits.SocketPopulated = 0;
    ProcessorStatus->Bits.Reserved2       = 0;
  }

  ProcessorCharacteristics->ProcessorReserved1      = 0;
  ProcessorCharacteristics->ProcessorUnknown        = 0;
  ProcessorCharacteristics->Processor64BitCapable   = 1;
  ProcessorCharacteristics->ProcessorMultiCore      = 1;
  ProcessorCharacteristics->ProcessorHardwareThread = 0;
  ProcessorCharacteristics->ProcessorExecuteProtection      = 1;
  ProcessorCharacteristics->ProcessorEnhancedVirtualization = 0;
  ProcessorCharacteristics->ProcessorPowerPerformanceCtrl   = 0;
  ProcessorCharacteristics->Processor128BitCapable = 0;
  ProcessorCharacteristics->ProcessorArm64SocId = 1;
  ProcessorCharacteristics->ProcessorReserved2  = 0;

  MiscProcessorData->CurrentSpeed = 2000;
  MiscProcessorData->MaxSpeed     = 2000;
  MiscProcessorData->CoreCount    = CoreCount;
  MiscProcessorData->CoresEnabled = CoreCount;
  MiscProcessorData->ThreadCount  = 1;

  return TRUE;
}

/** Gets the maximum number of sockets supported by the platform.

  @return The maximum number of sockets.
**/
UINT8
OemGetProcessorMaxSockets (
  VOID
  )
{
  return 1;
}

/** Gets information about the cache at the specified cache level.

  @param ProcessorIndex The processor to get information for.
  @param CacheLevel     The cache level to get information for.
  @param DataCache      Whether the cache is a data cache.
  @param UnifiedCache   Whether the cache is a unified cache.
  @param SmbiosCacheTable The SMBIOS Type7 cache information structure.

  @return TRUE on success, FALSE on failure.
**/
EFIAPI
BOOLEAN
OemGetCacheInformation (
  IN UINT8     ProcessorIndex,
  IN UINT8     CacheLevel,
  IN BOOLEAN   DataCache,
  IN BOOLEAN   UnifiedCache,
  IN OUT SMBIOS_TABLE_TYPE7 *SmbiosCacheTable
  )
{
  SmbiosCacheTable->CacheConfiguration = CacheLevel - 1;

  if (CacheLevel == 1 && !DataCache && !UnifiedCache) {
    // Unknown operational mode
    SmbiosCacheTable->CacheConfiguration |= (3 << 8);
  } else {
    // Write back operational mode
    SmbiosCacheTable->CacheConfiguration |= (1 << 8);
  }

  return TRUE;
}

/** Gets the type of chassis for the system.

  @param ChassisType The type of the chassis.

  @retval EFI_SUCCESS The chassis type was fetched successfully.
**/
EFI_STATUS
EFIAPI
OemGetChassisType (
  UINT8 *ChassisType
  )
{
  *ChassisType = MiscChassisTypeUnknown;
  return EFI_SUCCESS;
}

/** Updates the HII string for the specified field.

  @param mHiiHandle    The HII handle.
  @param TokenToUpdate The string to update.
  @param Offset        The field to get information about.
**/
VOID
OemUpdateSmbiosInfo (
  IN EFI_HII_HANDLE mHiiHandle,
  IN EFI_STRING_ID TokenToUpdate,
  IN OEM_MISC_SMBIOS_HII_STRING_FIELD Offset
  )
{
  // These values are fixed for now, but should be configurable via
  // something like an emulated SCP.
  switch (Offset) {
    case SystemManufacturerType01:
      HiiSetString (mHiiHandle, TokenToUpdate, L"QEMU", NULL);
      break;
    case SerialNumType01:
      HiiSetString (mHiiHandle, TokenToUpdate, L"SN0000", NULL);
      break;
    case SkuNumberType01:
      HiiSetString (mHiiHandle, TokenToUpdate, L"SK0000", NULL);
      break;
    case FamilyType01:
      HiiSetString (mHiiHandle, TokenToUpdate, L"ArmVirt", NULL);
      break;
    case AssertTagType02:
      HiiSetString (mHiiHandle, TokenToUpdate, L"AT0000", NULL);
      break;
    case SerialNumberType02:
      HiiSetString (mHiiHandle, TokenToUpdate, L"SN0000", NULL);
      break;
    case BoardManufacturerType02:
      HiiSetString (mHiiHandle, TokenToUpdate, L"QEMU", NULL);
      break;
    case SkuNumberType02:
      HiiSetString (mHiiHandle, TokenToUpdate, L"SK0000", NULL);
      break;
    case ChassisLocationType02:
      HiiSetString (mHiiHandle, TokenToUpdate, L"Bottom", NULL);
      break;
    case SerialNumberType03:
      HiiSetString (mHiiHandle, TokenToUpdate, L"SN0000", NULL);
      break;
    case VersionType03:
      HiiSetString (mHiiHandle, TokenToUpdate, L"1.0", NULL);
      break;
    case ManufacturerType03:
      HiiSetString (mHiiHandle, TokenToUpdate, L"QEMU", NULL);
      break;
    case AssetTagType03:
      HiiSetString (mHiiHandle, TokenToUpdate, L"AT0000", NULL);
      break;
    case SkuNumberType03:
      HiiSetString (mHiiHandle, TokenToUpdate, L"SK0000", NULL);
      break;
    default:
      break;
  }
}

