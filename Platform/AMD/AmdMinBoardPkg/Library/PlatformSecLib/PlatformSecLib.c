/** @file
Platform SEC Library.

  Copyright (c) 2013 - 2015, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2023 - 2025 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Ppi/SecPlatformInformation.h>
#include <Ppi/TemporaryRamSupport.h>
#include <Library/PcdLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/MtrrLib.h>
#include <Library/SecBoardInitLib.h>
#include <Library/TestPointCheckLib.h>

#include <AGESA.h>

//
// Bitfield Description : Not shared between threads.
// Controls access to Core::X86::Msr::MtrrFix_64K through Core::X86::Msr::MtrrFix_4K_7 [RdDram ,WrDram].
// This bit should be set to 1 during BIOS initialization of the fixed MTRRs, then cleared to 0 for operation.
//
#define SYS_CFG_MTRR_FIX_DRAM_MOD_EN_OFFSET  19       // Refer to AMD64 Architecture Programming manual.
#define SYS_CFG_MTRR_FIX_DRAM_MOD_EN_WIDTH   1        // Refer to AMD64 Architecture Programming manual.
#define SYS_CFG_MTRR_FIX_DRAM_MOD_EN_MASK    0x80000  // Refer to AMD64 Architecture Programming manual.

VOID
AsmSecPlatformDisableTemporaryMemory (
  VOID
  );

UINT32 *
AsmSecPlatformGetTemporaryStackBase (
  VOID
  );

/**

  Entry point to the C language phase of SEC. After the SEC assembly
  code has initialized some temporary memory and set up the stack,
  the control is transferred to this function.

  @param[in] SizeOfRam           Size of the temporary memory available for use.
  @param[in] TempRamBase         Base address of temporary ram
  @param[in] BootFirmwareVolume  Base address of the Boot Firmware Volume.

**/
VOID
EFIAPI
SecStartup (
  IN UINT32  SizeOfRam,
  IN UINT32  TempRamBase,
  IN VOID    *BootFirmwareVolume
  );

/**
  Auto-generated function that calls the library constructors for all of the module's
  dependent libraries.  This function must be called by the SEC Core once a stack has
  been established.

**/
VOID
EFIAPI
ProcessLibraryConstructorList (
  VOID
  );

/**

  Entry point to the C language phase of PlatformSecLib.  After the SEC assembly
  code has initialized some temporary memory and set up the stack, control is
  transferred to this function.

**/
VOID
EFIAPI
PlatformSecLibStartup (
  VOID
  )
{
  //
  // Process all library constructor functions linked to SecCore.
  // This function must be called before any library functions are called
  //
  ProcessLibraryConstructorList ();

  AsmMsrBitFieldOr64 (
    SYS_CFG,
    SYS_CFG_MTRR_FIX_DRAM_MOD_EN_OFFSET,
    SYS_CFG_MTRR_FIX_DRAM_MOD_EN_OFFSET,
    0x1
    );
  AsmWriteMsr64 (AMD_AP_MTRR_FIX64k_00000, 0x1E1E1E1E1E1E1E1E);
  AsmWriteMsr64 (AMD_AP_MTRR_FIX16k_80000, 0x1E1E1E1E1E1E1E1E);
  AsmWriteMsr64 (AMD_AP_MTRR_FIX16k_A0000, 0x1E1E1E1E1E1E1E1E);
  AsmWriteMsr64 (AMD_AP_MTRR_FIX4k_C0000, 0x1E1E1E1E1E1E1E1E);
  AsmWriteMsr64 (AMD_AP_MTRR_FIX4k_C8000, 0x1E1E1E1E1E1E1E1E);
  AsmWriteMsr64 (AMD_AP_MTRR_FIX4k_D0000, 0x1E1E1E1E1E1E1E1E);
  AsmWriteMsr64 (AMD_AP_MTRR_FIX4k_D8000, 0x1E1E1E1E1E1E1E1E);
  AsmWriteMsr64 (AMD_AP_MTRR_FIX4k_E0000, 0x1E1E1E1E1E1E1E1E);
  AsmWriteMsr64 (AMD_AP_MTRR_FIX4k_E8000, 0x1E1E1E1E1E1E1E1E);
  AsmWriteMsr64 (AMD_AP_MTRR_FIX4k_F0000, 0x1E1E1E1E1E1E1E1E);
  AsmWriteMsr64 (AMD_AP_MTRR_FIX4k_F8000, 0x1E1E1E1E1E1E1E1E);
  AsmMsrBitFieldAnd64 (
    SYS_CFG,
    SYS_CFG_MTRR_FIX_DRAM_MOD_EN_OFFSET,
    SYS_CFG_MTRR_FIX_DRAM_MOD_EN_OFFSET,
    0x0
    );

  //
  // Pass control to SecCore module passing in the base address and size
  // of the temporary RAM
  //
  SecStartup (
    PcdGet32 (PcdTempRamSize),
    PcdGet32 (PcdTempRamBase),
    (VOID *)(UINTN)PcdGet32 (PcdBootFvBase)
    );
}

/**
  This interface conveys state information out of the Security (SEC) phase into PEI.

  @param[in]      PeiServices               Pointer to the PEI Services Table.
  @param[in, out] StructureSize             Pointer to the variable describing size of the input buffer.
  @param[out] PlatformInformationRecord Pointer to the EFI_SEC_PLATFORM_INFORMATION_RECORD.

  @retval EFI_SUCCESS           The data was successfully returned.
  @retval EFI_BUFFER_TOO_SMALL  The buffer was too small.

**/
EFI_STATUS
EFIAPI
SecPlatformInformation (
  IN CONST EFI_PEI_SERVICES                  **PeiServices,
  IN OUT   UINT64                            *StructureSize,
  OUT   EFI_SEC_PLATFORM_INFORMATION_RECORD  *PlatformInformationRecord
  )
{
  UINT32             *BistPointer;
  UINT32             BistSize;
  UINT32             Count;
  EFI_HOB_GUID_TYPE  *GuidHob;
  UINT32             *TopOfStack;

  DEBUG ((DEBUG_INFO, "%a: - ENTRY\n", __func__));

  ASSERT (StructureSize);

  GuidHob = GetFirstGuidHob (&gEfiSecPlatformInformationPpiGuid);
  if (GuidHob != NULL) {
    DEBUG ((DEBUG_INFO, " Found GuidHob!\n"));
    BistSize    = GET_GUID_HOB_DATA_SIZE (GuidHob);
    BistPointer = GET_GUID_HOB_DATA (GuidHob);
    DEBUG ((
      DEBUG_INFO,
      "  BistSize = %d, BistPointer = 0x%X\n",
      BistSize,
      BistPointer
      ));
  } else {
    //
    // The entries of BIST information, together with the number of them,
    // reside in the bottom of stack, left untouched by normal stack operation.
    // This routine copies the BIST information to the buffer pointed by
    // PlatformInformationRecord for output.
    //
    TopOfStack  = AsmSecPlatformGetTemporaryStackBase ();
    Count       = *(TopOfStack - 1);
    BistSize    = Count * sizeof (IA32_HANDOFF_STATUS);
    BistPointer = (UINT32 *)(UINTN)((UINTN)TopOfStack - sizeof (Count) - BistSize);

    //
    // Copy Data from Stack to Hob to avoid data is lost after memory is ready.
    //
    DEBUG ((DEBUG_INFO, " Building GuidHob:\n"));
    DEBUG ((
      DEBUG_INFO,
      "  BEFORE: BistSize = %d, BistPointer = 0x%X\n",
      BistSize,
      BistPointer
      ));
    BuildGuidDataHob (
      &gEfiSecPlatformInformationPpiGuid,
      BistPointer,
      (UINTN)BistSize
      );

    GuidHob = GetFirstGuidHob (&gEfiSecPlatformInformationPpiGuid);
    DEBUG ((DEBUG_INFO, " Reading the built GuidHob... "));
    if (GuidHob != NULL) {
      DEBUG ((DEBUG_INFO, "OK!\n"));
      BistSize    = GET_GUID_HOB_DATA_SIZE (GuidHob);
      BistPointer = GET_GUID_HOB_DATA (GuidHob);
    } else {
      DEBUG ((DEBUG_INFO, "FAILED!\n"));
    }

    DEBUG ((
      DEBUG_INFO,
      "  AFTER: BistSize = %d, BistPointer = 0x%X\n",
      BistSize,
      BistPointer
      ));
  }

  DEBUG ((
    DEBUG_INFO,
    " StructureSize: 0x%X, [ 0x%X ]\n",
    StructureSize,
    *StructureSize
    ));
  DEBUG ((
    DEBUG_INFO,
    " PlatformInformationRecord: 0x%X\n",
    PlatformInformationRecord
    ));

  if (((*StructureSize) < (UINT64)BistSize) ||
      (PlatformInformationRecord == NULL))
  {
    *StructureSize = BistSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  CopyMem (PlatformInformationRecord, BistPointer, BistSize);
  *StructureSize = BistSize;

  DEBUG ((DEBUG_INFO, "%a: - EXIT\n", __func__));
  return EFI_SUCCESS;
}

/**
  This interface disables temporary memory in SEC Phase.
**/
VOID
EFIAPI
SecPlatformDisableTemporaryMemory (
  VOID
  )
{
  DEBUG ((DEBUG_ERROR, "%a: - ENTRY\n", __func__));
  AsmSecPlatformDisableTemporaryMemory ();
  DEBUG ((DEBUG_ERROR, "%a: - EXIT\n", __func__));
}

/**
  Wrapper function to Early Board Init interface in SEC Phase.
**/
VOID
EFIAPI
BoardAfterTempRamInitWrapper (
  VOID
  )
{
  BoardAfterTempRamInit ();
  TestPointTempMemoryFunction (
    (VOID *)(UINTN)PcdGet32 (PcdTempRamBase),
    (VOID *)(UINTN)(PcdGet32 (PcdTempRamBase) + PcdGet32 (PcdTempRamSize))
    );
}
