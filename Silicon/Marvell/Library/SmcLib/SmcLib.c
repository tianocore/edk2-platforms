/** @file

  Source file for the Marvell SMC interface.

  Copyright (c) 2023, 2026 Marvell. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/SmcLib.h>
#include <Library/ArmSmcLib.h>  // ArmCallSmc
#include <Library/DebugLib.h>

UINTN SmcGetRamSize ( IN UINTN Node )
{
  ARM_SMC_ARGS ArmSmcArgs;

  ArmSmcArgs.Arg0 = MV_SMC_ID_DRAM_SIZE;
  ArmSmcArgs.Arg1 = Node;
  ArmCallSmc (&ArmSmcArgs);

  return ArmSmcArgs.Arg0;
}

UINTN
SmcNodeCount (
  VOID
  )
{
  ARM_SMC_ARGS  ArmSmcArgs;

  ArmSmcArgs.Arg0 = MV_SMC_ID_NODE_COUNT;
  ArmCallSmc (&ArmSmcArgs);

  return ArmSmcArgs.Arg0;
}

UINTN
SmcMcsCalibrate (
  VOID
  )
{
  ARM_SMC_ARGS  ArmSmcArgs;

  ArmSmcArgs.Arg0 = MV_SMC_MCS_CALIBRATE;
  ArmSmcArgs.Arg1 = 0;
  ArmSmcArgs.Arg2 = 0;
  ArmSmcArgs.Arg3 = 0;

  ArmCallSmc (&ArmSmcArgs);
  return ArmSmcArgs.Arg0; // 0 = success, non-zero = timeout/error
}

/**
  Shut down network RVU interface.

  @retval 0              EFI_SUCCESS
  @retval Other          On error.

**/
UINTN
SmcShutdownRvu (
  VOID
  )
{
  ARM_SMC_ARGS  ArmSmcArgs;

  ArmSmcArgs.Arg0 = MV_SMC_DISABLE_RVU_LFS;
  ArmCallSmc (&ArmSmcArgs);

  return ArmSmcArgs.Arg0;
}

/**
  Notify firmware of a boot success.
  The firmware will then disable the watchdog.

  @retval 0              EFI_SUCCESS
  @retval Other          On error.

**/
UINTN
SmcBootSuccess (
  VOID
  )
{
  ARM_SMC_ARGS  ArmSmcArgs;

  ArmSmcArgs.Arg0 = MV_SMC_FSAFE_BOOT_SUCCESS;
  ArmCallSmc (&ArmSmcArgs);

  return ArmSmcArgs.Arg0;
}

/**
  Update firmware image in SPI NOR flash.

  @param[in]  Buffer   Pointer to buffer that contains the firmware image
  @param[in]  Size     Size of the buffer

  @retval 0            EFI_SUCCESS
  @retval Other        On error.

**/
UINTN
SmcSpiSecureUpdate (
  IN VOID   *Buffer,
  IN UINTN  Size
  )
{
  ARM_SMC_ARGS  ArmSmcArgs;

  ArmSmcArgs.Arg0 = MV_SMC_SPI_SECURE_UPDATE;
  ArmSmcArgs.Arg1 = (UINTN)Buffer;
  ArmSmcArgs.Arg2 = Size;
  ArmSmcArgs.Arg3 = 0;
  ArmCallSmc (&ArmSmcArgs);

  return ArmSmcArgs.Arg0;
}

/**
  Set MAC address of given interface.

  @param[in]  MacIndex   Index of MAC address to be set
  @param[in]  MacValue   New MAC address to be set

  @retval 0              EFI_SUCCESS
  @retval Other          On error.

**/
UINTN
SmcSetMacAddress (
  IN UINT32  MacIndex,
  IN UINT64  MacValue
  )
{
  ARM_SMC_ARGS  ArmSmcArgs;

  DEBUG ((DEBUG_INFO, "%a: MAC to set: 0x%016lx, Index = %d\n", __FUNCTION__, MacValue, MacIndex));

  ArmSmcArgs.Arg0 = MV_SMC_SET_MAC;
  ArmSmcArgs.Arg1 = MacIndex; // MAC address index
  ArmSmcArgs.Arg2 = MacValue; // MAC address value
  ArmCallSmc (&ArmSmcArgs);

  return ArmSmcArgs.Arg0;
}

#ifdef SMC_DIMM_HANDLE

/**
  Set SMBIOS DIMM handle.

  @param[in]  DSSIndex  Specifies the dss index.
  @param[in]  Handle    SMBIOS handle for the DIMM

  @retval 0              EFI_SUCCESS
  @retval Other          On error.

**/
UINTN
SmcSetDimmHandle (
  IN UINT32  DSSIndex,
  IN UINT32  Handle
  )
{
  ARM_SMC_ARGS  ArmSmcArgs;

  DEBUG ((DEBUG_INFO, "%a: DSS Index = 0x%x, Handle = 0x%x\n", __FUNCTION__, DSSIndex, Handle));

  ArmSmcArgs.Arg0 = MV_SMC_SET_DIMM_HANDLE;
  ArmSmcArgs.Arg1 = DSSIndex; // DSS index
  ArmSmcArgs.Arg2 = Handle;   // DIMM handle for type17
  ArmCallSmc (&ArmSmcArgs);

  return ArmSmcArgs.Arg0;
}

#endif
