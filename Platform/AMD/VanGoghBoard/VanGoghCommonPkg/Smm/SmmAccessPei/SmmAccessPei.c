/** @file
This is the driver that publishes the SMM Access Ppi
instance for the Quark SOC.

Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>
Copyright (c) 2013-2019 Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiPei.h>
#include <Ppi/SmmAccess.h>
#include <Guid/SmramMemoryReserve.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PciLib.h>
#include <Library/PeiServicesLib.h>
#include <AGESA.h>
#define SMMMASK_ADDRESS  0xC0010113

#define SMM_ACCESS_PRIVATE_DATA_FROM_THIS(a) \
  CR ( \
  a, \
  SMM_ACCESS_PRIVATE_DATA, \
  SmmAccess, \
  SMM_ACCESS_PRIVATE_DATA_SIGNATURE \
  )

typedef struct {
  UINTN                   Signature;
  EFI_HANDLE              Handle;
  PEI_SMM_ACCESS_PPI      SmmAccess;
  UINTN                   NumberRegions;
  EFI_SMRAM_DESCRIPTOR    *SmramDesc;
} SMM_ACCESS_PRIVATE_DATA;

#define SMM_ACCESS_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('4', '5', 's', 'a')

/**
  CpuOpenSMRAM - read/write A0000-BFFFF

  @param         VOID                  None.

  @retval        VOID                  None.
**/
VOID
EFIAPI
OpenSMRAM (
  VOID
  )
{
  volatile UINT64  RegValue;

  // Disable protection in ASeg and TSeg
  RegValue  = AsmReadMsr64 (SMMMASK_ADDRESS);
  RegValue &= (UINT64)(~BIT0);
  RegValue &= (UINT64)(~BIT1);
  AsmWriteMsr64 (SMMMASK_ADDRESS, RegValue);

  // Enable FixMtrrModEn
  RegValue  = AsmReadMsr64 (SYS_CFG);
  RegValue |= (UINT64)(1 << 19);
  AsmWriteMsr64 (SYS_CFG, RegValue);

  // Enable Rd/Wr DRAM in ASeg
  RegValue  = AsmReadMsr64 (AMD_AP_MTRR_FIX16k_A0000);
  RegValue |= 0x1010101010101010;
  RegValue |= 0x0808080808080808;
  AsmWriteMsr64 (AMD_AP_MTRR_FIX16k_A0000, RegValue);

  // Disable FixMtrrModEn
  RegValue  = AsmReadMsr64 (SYS_CFG);
  RegValue &= ~(UINT64)(1 << 19);
  AsmWriteMsr64 (SYS_CFG, RegValue);
}

/**
  CpuSmramWP - write protect from A0000-BFFFF

  @param         VOID                  None.

  @retval        VOID                  None.
**/
VOID
EFIAPI
CloseSmram (
  VOID
  )
{
  volatile UINT64  RegValue;

  // Enable FixMtrrModEn
  RegValue  = AsmReadMsr64 (SYS_CFG);
  RegValue |= (UINT64)(1 << 19);
  AsmWriteMsr64 (SYS_CFG, RegValue);

  // Disable Rd/Wr DRAM in ASeg
  RegValue  = AsmReadMsr64 (AMD_AP_MTRR_FIX16k_A0000);
  RegValue &= 0xEFEFEFEFEFEFEFEF;
  RegValue &= 0xF7F7F7F7F7F7F7F7;
  AsmWriteMsr64 (AMD_AP_MTRR_FIX16k_A0000, RegValue);

  // Disable FixMtrrModEn
  RegValue  = AsmReadMsr64 (SYS_CFG);
  RegValue &= ~(UINT64)(1 << 19);
  AsmWriteMsr64 (SYS_CFG, RegValue);

  RegValue  = AsmReadMsr64 (SMMMASK_ADDRESS);
  RegValue |= (UINT64)BIT0;
  RegValue |= (UINT64)BIT1;
  AsmWriteMsr64 (SMMMASK_ADDRESS, RegValue);
}

/**
  Setting the bit0 of MSRC001_0015 Hardware Configuration (HWCR) to do SMM code lock.

  @param         VOID                  None.

  @retval        VOID                  None.
**/
VOID
EFIAPI
LockSmm (
  VOID
  )
{
  volatile UINT64  Data64;

  Data64  = AsmReadMsr64 (HWCR);
  Data64 |= (UINT64)BIT0; // SMM_LOCK
  AsmWriteMsr64 (HWCR, Data64);
}

/**
  This routine accepts a request to "open" a region of SMRAM.  The
  region could be legacy ABSEG, HSEG, or TSEG near top of physical memory.
  The use of "open" means that the memory is visible from all PEIM
  and SMM agents.

  @param[in]  PeiServices        General purpose services available to every PEIM.
  @param[in]  This                Pointer to the SMM Access Interface.
  @param[in]  DescriptorIndex     Region of SMRAM to Open.

  @retval  EFI_SUCCESS               The region was successfully opened.
  @retval  EFI_DEVICE_ERROR          The region could not be opened because locked by
                                     chipset.
  @retval  EFI_INVALID_PARAMETER     The descriptor index was out of bounds.

**/
EFI_STATUS
EFIAPI
Open (
  IN EFI_PEI_SERVICES    **PeiServices,
  IN PEI_SMM_ACCESS_PPI  *This,
  IN UINTN               DescriptorIndex
  )
{
  SMM_ACCESS_PRIVATE_DATA  *SmmAccess;

  SmmAccess = SMM_ACCESS_PRIVATE_DATA_FROM_THIS (This);

  if (DescriptorIndex >= SmmAccess->NumberRegions) {
    DEBUG ((DEBUG_WARN, "SMRAM region out of range in Open\n"));
    return EFI_INVALID_PARAMETER;
  } else if (SmmAccess->SmramDesc[DescriptorIndex].RegionState & EFI_SMRAM_LOCKED) {
    DEBUG ((DEBUG_WARN, "Cannot open a locked SMRAM region in Open\n"));
    return EFI_DEVICE_ERROR;
  }

  //
  // Open TSEG
  //
  OpenSMRAM ();

  SmmAccess->SmramDesc[DescriptorIndex].RegionState &= ~(EFI_SMRAM_CLOSED | EFI_ALLOCATED);
  SmmAccess->SmramDesc[DescriptorIndex].RegionState |= EFI_SMRAM_OPEN;
  SmmAccess->SmmAccess.OpenState                     = TRUE;

  return EFI_SUCCESS;
}

/**
  This routine accepts a request to "close" a region of SMRAM.  This is valid for
  compatible SMRAM region.

  @param[in]  PeiServices        General purpose services available to every PEIM.
  @param[in]  This                Pointer to the SMM Access Interface.
  @param[in]  DescriptorIndex     Region of SMRAM to Close.

  @retval  EFI_SUCCESS               The region was successfully closed.
  @retval  EFI_DEVICE_ERROR          The region could not be closed because locked by
                                     chipset.
  @retval  EFI_INVALID_PARAMETER     The descriptor index was out of bounds.

**/
EFI_STATUS
EFIAPI
Close (
  IN EFI_PEI_SERVICES    **PeiServices,
  IN PEI_SMM_ACCESS_PPI  *This,
  IN UINTN               DescriptorIndex
  )
{
  SMM_ACCESS_PRIVATE_DATA  *SmmAccess;
  BOOLEAN                  OpenState;
  UINT8                    Index;

  SmmAccess = SMM_ACCESS_PRIVATE_DATA_FROM_THIS (This);

  if (DescriptorIndex >= SmmAccess->NumberRegions) {
    DEBUG ((DEBUG_WARN, "SMRAM region out of range in Close\n"));
    return EFI_INVALID_PARAMETER;
  } else if (SmmAccess->SmramDesc[DescriptorIndex].RegionState & EFI_SMRAM_LOCKED) {
    DEBUG ((DEBUG_WARN, "SmmAccess Close region is locked:%d\n", DescriptorIndex));
    return EFI_DEVICE_ERROR;
  }

  if (SmmAccess->SmramDesc[DescriptorIndex].RegionState & EFI_SMRAM_CLOSED) {
    DEBUG ((DEBUG_WARN, "SmmAccess Close region is closed already:%d\n", DescriptorIndex));
    return EFI_DEVICE_ERROR;
  }

  //
  // Close TSEG
  //
  CloseSmram ();

  SmmAccess->SmramDesc[DescriptorIndex].RegionState &= ~EFI_SMRAM_OPEN;
  SmmAccess->SmramDesc[DescriptorIndex].RegionState |= (EFI_SMRAM_CLOSED | EFI_ALLOCATED);

  //
  // Find out if any regions are still open
  //
  OpenState = FALSE;
  for (Index = 0; Index < SmmAccess->NumberRegions; Index++) {
    if ((SmmAccess->SmramDesc[Index].RegionState & EFI_SMRAM_OPEN) == EFI_SMRAM_OPEN) {
      OpenState = TRUE;
    }
  }

  SmmAccess->SmmAccess.OpenState = OpenState;

  return EFI_SUCCESS;
}

/**
  This routine accepts a request to "lock" SMRAM.  The
  region could be legacy AB or TSEG near top of physical memory.
  The use of "lock" means that the memory can no longer be opened
  to PEIM.

  @param[in]  PeiServices        General purpose services available to every PEIM.
  @param[in]  This                Pointer to the SMM Access Interface.
  @param[in]  DescriptorIndex     Region of SMRAM to Lock.

  @retval  EFI_SUCCESS               The region was successfully locked.
  @retval  EFI_DEVICE_ERROR          The region could not be locked because at least
                                     one range is still open.
  @retval  EFI_INVALID_PARAMETER     The descriptor index was out of bounds.

**/
EFI_STATUS
EFIAPI
Lock (
  IN EFI_PEI_SERVICES    **PeiServices,
  IN PEI_SMM_ACCESS_PPI  *This,
  IN UINTN               DescriptorIndex
  )
{
  SMM_ACCESS_PRIVATE_DATA  *SmmAccess;

  SmmAccess = SMM_ACCESS_PRIVATE_DATA_FROM_THIS (This);

  if (DescriptorIndex >= SmmAccess->NumberRegions) {
    DEBUG ((DEBUG_WARN, "SMRAM region out of range in Lock\n"));
    return EFI_INVALID_PARAMETER;
  } else if (SmmAccess->SmmAccess.OpenState) {
    DEBUG ((DEBUG_WARN, "Cannot lock SMRAM when SMRAM regions are still open\n"));
    return EFI_DEVICE_ERROR;
  }

  //
  // Lock TSEG
  //
  LockSmm ();
  SmmAccess->SmramDesc[DescriptorIndex].RegionState |= EFI_SMRAM_LOCKED;
  SmmAccess->SmmAccess.LockState                     = TRUE;

  return EFI_SUCCESS;
}

/**
  This routine services a user request to discover the SMRAM
  capabilities of this platform.  This will report the possible
  ranges that are possible for SMRAM access, based upon the
  memory controller capabilities.

  @param[in]       PeiServices     General purpose services available to every PEIM.
  @param[in]       This             Pointer to the SMRAM Access Interface.
  @param[in, out]  SmramMapSize     Pointer to the variable containing size of the
                                    buffer to contain the description information.
  @param[in, out]  SmramMap         Buffer containing the data describing the Smram
                                    region descriptors.

  @retval  EFI_BUFFER_TOO_SMALL     The user did not provide a sufficient buffer.
  @retval  EFI_SUCCESS              The user provided a sufficiently-sized buffer.

**/
EFI_STATUS
EFIAPI
GetCapabilities (
  IN EFI_PEI_SERVICES          **PeiServices,
  IN PEI_SMM_ACCESS_PPI        *This,
  IN OUT UINTN                 *SmramMapSize,
  IN OUT EFI_SMRAM_DESCRIPTOR  *SmramMap
  )
{
  EFI_STATUS               Status;
  SMM_ACCESS_PRIVATE_DATA  *SmmAccess;
  UINTN                    BufferSize;

  SmmAccess  = SMM_ACCESS_PRIVATE_DATA_FROM_THIS (This);
  BufferSize = SmmAccess->NumberRegions * sizeof (EFI_SMRAM_DESCRIPTOR);

  if (*SmramMapSize < BufferSize) {
    DEBUG ((DEBUG_WARN, "SMRAM Map Buffer too small\n"));
    Status = EFI_BUFFER_TOO_SMALL;
  } else {
    CopyMem (SmramMap, SmmAccess->SmramDesc, BufferSize);
    Status = EFI_SUCCESS;
  }

  *SmramMapSize = BufferSize;

  return Status;
}

/**
  This is the constructor for the SMM Access Ppi

  @param[in]    FfsHeader        FfsHeader.
  @param[in]    PeiServices      General purpose services available to every PEIM.

  @retval  EFI_SUCCESS       Protocol successfully started and installed.
  @retval  EFI_UNSUPPORTED   Protocol can't be started.
**/
EFI_STATUS
EFIAPI
SmmAccessPeiEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                      Status;
  EFI_BOOT_MODE                   BootMode;
  UINTN                           Index;
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK  *DescriptorBlock = NULL;
  SMM_ACCESS_PRIVATE_DATA         *SmmAccessPrivate;
  EFI_PEI_PPI_DESCRIPTOR          *PpiList;
  EFI_HOB_GUID_TYPE               *GuidHob;

  Status = (*PeiServices)->GetBootMode (PeiServices, &BootMode);
  if (EFI_ERROR (Status) || (BootMode != BOOT_ON_S3_RESUME)) {
    //
    // If not in S3 boot path. do nothing
    //
    return EFI_SUCCESS;
  }

  //
  // Initialize private data
  //
  SmmAccessPrivate = AllocateZeroPool (sizeof (*SmmAccessPrivate));
  ASSERT (SmmAccessPrivate != NULL);

  PpiList = AllocateZeroPool (sizeof (*PpiList));
  ASSERT (PpiList != NULL);

  //
  // Build SMM related information
  //
  SmmAccessPrivate->Signature = SMM_ACCESS_PRIVATE_DATA_SIGNATURE;
  SmmAccessPrivate->Handle    = NULL;

  //
  // Get Hob list
  //
  GuidHob = GetFirstGuidHob (&gEfiSmmPeiSmramMemoryReserveGuid);
  ASSERT (GuidHob != NULL);
  DescriptorBlock = GET_GUID_HOB_DATA (GuidHob);
  ASSERT (DescriptorBlock != NULL);

  //
  // Alloc space for SmmAccessPrivate->SmramDesc
  //
  SmmAccessPrivate->SmramDesc = AllocateZeroPool ((DescriptorBlock->NumberOfSmmReservedRegions) * sizeof (EFI_SMRAM_DESCRIPTOR));
  if (SmmAccessPrivate->SmramDesc == NULL) {
    DEBUG ((DEBUG_ERROR, "Alloc SmmAccessPrivate->SmramDesc fail.\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Use the hob to publish SMRAM capabilities
  //
  for (Index = 0; Index < DescriptorBlock->NumberOfSmmReservedRegions; Index++) {
    SmmAccessPrivate->SmramDesc[Index].PhysicalStart = DescriptorBlock->Descriptor[Index].PhysicalStart;
    SmmAccessPrivate->SmramDesc[Index].CpuStart      = DescriptorBlock->Descriptor[Index].CpuStart;
    SmmAccessPrivate->SmramDesc[Index].PhysicalSize  = DescriptorBlock->Descriptor[Index].PhysicalSize;
    SmmAccessPrivate->SmramDesc[Index].RegionState   = DescriptorBlock->Descriptor[Index].RegionState;
  }

  SmmAccessPrivate->NumberRegions             = Index;
  SmmAccessPrivate->SmmAccess.Open            = Open;
  SmmAccessPrivate->SmmAccess.Close           = Close;
  SmmAccessPrivate->SmmAccess.Lock            = Lock;
  SmmAccessPrivate->SmmAccess.GetCapabilities = GetCapabilities;
  SmmAccessPrivate->SmmAccess.LockState       = FALSE;
  SmmAccessPrivate->SmmAccess.OpenState       = FALSE;

  PpiList->Flags = (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);
  PpiList->Guid  = &gPeiSmmAccessPpiGuid;
  PpiList->Ppi   = &SmmAccessPrivate->SmmAccess;

  Status = (**PeiServices).InstallPpi (PeiServices, PpiList);
  ASSERT_EFI_ERROR (Status);

  DEBUG (
    (DEBUG_INFO, "SMM Base:Size %08X:%08X\n",
     (UINTN)(SmmAccessPrivate->SmramDesc[SmmAccessPrivate->NumberRegions-1].PhysicalStart),
     (UINTN)(SmmAccessPrivate->SmramDesc[SmmAccessPrivate->NumberRegions-1].PhysicalSize)
    ));

  return EFI_SUCCESS;
}
