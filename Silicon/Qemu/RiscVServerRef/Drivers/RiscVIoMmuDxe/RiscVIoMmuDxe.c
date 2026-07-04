/** @file
  RISC-V IOMMU DXE driver - bare (pass-through) mode initialization.

    This driver:
    1. Locates the FDT_CLIENT_PROTOCOL and searches the device tree for
       a node with compatible = "riscv,iommu" to obtain the MMIO base
       address and size from the "reg" property.
    2. Adds the MMIO region to the GCD memory map as MemoryMappedIo and
       sets the EFI_MEMORY_UC attribute before accessing any register.
    3. Reads the capabilities register to verify the hardware is present
       and reports its version.
    4. Writes ddtp.iommu_mode = Bare (0x1), which configures the IOMMU to
       pass all DMA transactions through without address translation or
       access control. This is the correct mode for pre-OS firmware.
    5. Polls ddtp.busy until the hardware acknowledges the mode change.
    6. Installs EDKII_IOMMU_PROTOCOL with identity-mapping Map/Unmap/
       AllocateBuffer/FreeBuffer so that PCI bus drivers (PciBusDxe,
       AtaAtapiPassThru, NvmExpressDxe, etc.) can perform DMA without
       bounce buffers.

  In bare mode:
    - DeviceAddress == HostAddress  (no translation)
    - No page table walks
    - No access faults from the IOMMU
    - SetAttribute() is a no-op (no per-device access control)

  Reference: RISC-V IOMMU Architecture Specification, Version 1.0
             https://github.com/riscv-non-isa/riscv-iommu

  Copyright (c) 2026, Qualcomm Incorporated. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "RiscVIoMmuDxe.h"

#include <Library/DxeServicesTableLib.h>

// ---------------------------------------------------------------------------
// MMIO accessor helpers
// ---------------------------------------------------------------------------

/**
  Read a 64-bit IOMMU register.

  @param[in]  Base    Virtual base address of the IOMMU MMIO region.
  @param[in]  Offset  Register offset in bytes.

  @return  64-bit register value.
**/
STATIC inline UINT64
IommuRead64 (
  IN UINT64  *Base,
  IN UINTN   Offset
  )
{
  return MmioRead64 ((UINTN)Base + Offset);
}

/**
  Write a 64-bit IOMMU register.

  @param[in]  Base    Virtual base address of the IOMMU MMIO region.
  @param[in]  Offset  Register offset in bytes.
  @param[in]  Value   Value to write.
**/
STATIC inline VOID
IommuWrite64 (
  IN UINT64  *Base,
  IN UINTN   Offset,
  IN UINT64  Value
  )
{
  MmioWrite64 ((UINTN)Base + Offset, Value);
}

// ---------------------------------------------------------------------------
// Global driver context (single IOMMU instance)
// ---------------------------------------------------------------------------

STATIC RISCV_IOMMU_CONTEXT  mIoMmuContext;

/**
  Set IOMMU attribute for a DMA mapping.

  In bare mode, the RISC-V IOMMU performs no access control, so this
  function is a no-op. It validates the Mapping pointer and returns
  EFI_SUCCESS unconditionally.

  @param[in]  This          The protocol instance pointer.
  @param[in]  DeviceHandle  The device initiating the DMA request.
  @param[in]  Mapping       The mapping value returned from Map().
  @param[in]  IoMmuAccess   The requested IOMMU access flags.

  @retval EFI_SUCCESS            Always succeeds in bare mode.
  @retval EFI_INVALID_PARAMETER  Mapping is NULL.
**/
STATIC
EFI_STATUS
EFIAPI
RiscVIoMmuSetAttribute (
  IN EDKII_IOMMU_PROTOCOL  *This,
  IN EFI_HANDLE            DeviceHandle,
  IN VOID                  *Mapping,
  IN UINT64                IoMmuAccess
  )
{
  if (Mapping == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: DeviceHandle=0x%p Mapping=0x%p Access=0x%Lx (bare mode no-op)\n",
    __func__,
    DeviceHandle,
    Mapping,
    IoMmuAccess
    ));

  return EFI_SUCCESS;
}

/**
  Provides the controller-specific addresses required to access system
  memory from a DMA bus master.

  In bare mode, the device address equals the host physical address.
  No bounce buffer is required. A MAP_INFO structure is allocated to
  allow Unmap() to validate the Mapping token.

  @param[in]   This           The protocol instance pointer.
  @param[in]   Operation      The DMA operation type.
  @param[in]   HostAddress    The system memory address to map.
  @param[in,out] NumberOfBytes  On input, bytes to map. On output, bytes mapped.
  @param[out]  DeviceAddress  The device-visible address (== HostAddress).
  @param[out]  Mapping        Opaque token to pass to Unmap().

  @retval EFI_SUCCESS             Mapping created successfully.
  @retval EFI_INVALID_PARAMETER   One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES    MAP_INFO allocation failed.
**/
STATIC
EFI_STATUS
EFIAPI
RiscVIoMmuMap (
  IN     EDKII_IOMMU_PROTOCOL   *This,
  IN     EDKII_IOMMU_OPERATION  Operation,
  IN     VOID                   *HostAddress,
  IN OUT UINTN                  *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS   *DeviceAddress,
  OUT    VOID                   **Mapping
  )
{
  RISCV_IOMMU_MAP_INFO  *MapInfo;

  if ((HostAddress == NULL) ||
      (NumberOfBytes == NULL) ||
      (DeviceAddress == NULL) ||
      (Mapping == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((Operation < EdkiiIoMmuOperationBusMasterRead) ||
      (Operation >= EdkiiIoMmuOperationMaximum))
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Allocate a MAP_INFO to track this mapping.
  //
  MapInfo = AllocateZeroPool (sizeof (RISCV_IOMMU_MAP_INFO));
  if (MapInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  MapInfo->Signature     = RISCV_IOMMU_MAP_INFO_SIGNATURE;
  MapInfo->Operation     = Operation;
  MapInfo->HostAddress   = (EFI_PHYSICAL_ADDRESS)(UINTN)HostAddress;
  MapInfo->NumberOfBytes = *NumberOfBytes;

  //
  // Bare mode: device address is identical to host physical address.
  // No address translation, no bounce buffer.
  //
  MapInfo->DeviceAddress = MapInfo->HostAddress;
  *DeviceAddress         = MapInfo->DeviceAddress;
  *Mapping               = MapInfo;

  //
  // Track in the global list for validation in Unmap().
  //
  InsertTailList (
    &mIoMmuContext.MapInfoList,
    &MapInfo->Link
    );

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: Op=%d Host=0x%Lx Device=0x%Lx Bytes=0x%Lx\n",
    __func__,
    (INT32)Operation,
    MapInfo->HostAddress,
    MapInfo->DeviceAddress,
    (UINT64)*NumberOfBytes
    ));

  return EFI_SUCCESS;
}

/**
  Completes the Map() operation and releases the associated MAP_INFO.

  @param[in]  This     The protocol instance pointer.
  @param[in]  Mapping  The mapping value returned from Map().

  @retval EFI_SUCCESS            Mapping released.
  @retval EFI_INVALID_PARAMETER  Mapping is NULL or has an invalid signature.
**/
STATIC
EFI_STATUS
EFIAPI
RiscVIoMmuUnmap (
  IN EDKII_IOMMU_PROTOCOL  *This,
  IN VOID                  *Mapping
  )
{
  RISCV_IOMMU_MAP_INFO  *MapInfo;

  if (Mapping == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  MapInfo = (RISCV_IOMMU_MAP_INFO *)Mapping;

  if (MapInfo->Signature != RISCV_IOMMU_MAP_INFO_SIGNATURE) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Invalid Mapping signature 0x%08x\n",
      __func__,
      MapInfo->Signature
      ));
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: Mapping=0x%p Host=0x%Lx Bytes=0x%Lx\n",
    __func__,
    Mapping,
    MapInfo->HostAddress,
    (UINT64)MapInfo->NumberOfBytes
    ));

  RemoveEntryList (&MapInfo->Link);
  FreePool (MapInfo);

  return EFI_SUCCESS;
}

/**
  Allocates pages suitable for BusMasterCommonBuffer DMA operations.

  In bare mode, this is a standard EFI page allocation. No special
  alignment or memory attribute manipulation is required.

  @param[in]   This         The protocol instance pointer.
  @param[in]   Type         Allocation type (ignored; AllocateAnyPages used).
  @param[in]   MemoryType   EfiBootServicesData or EfiRuntimeServicesData.
  @param[in]   Pages        Number of 4KB pages to allocate.
  @param[out]  HostAddress  Receives the base address of the allocation.
  @param[in]   Attributes   Requested attributes (WRITE_COMBINE, CACHED, DAC).

  @retval EFI_SUCCESS             Pages allocated.
  @retval EFI_INVALID_PARAMETER   HostAddress is NULL or MemoryType is invalid.
  @retval EFI_UNSUPPORTED         Attributes contains unsupported bits.
  @retval EFI_OUT_OF_RESOURCES    Allocation failed.
**/
STATIC
EFI_STATUS
EFIAPI
RiscVIoMmuAllocateBuffer (
  IN     EDKII_IOMMU_PROTOCOL  *This,
  IN     EFI_ALLOCATE_TYPE     Type,
  IN     EFI_MEMORY_TYPE       MemoryType,
  IN     UINTN                 Pages,
  IN OUT VOID                  **HostAddress,
  IN     UINT64                Attributes
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  PhysAddr;

  if (HostAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((MemoryType != EfiBootServicesData) &&
      (MemoryType != EfiRuntimeServicesData))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((Attributes & EDKII_IOMMU_ATTRIBUTE_INVALID_FOR_ALLOCATE_BUFFER) != 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // In bare mode, allocate from anywhere. If the caller did not set
  // DUAL_ADDRESS_CYCLE, constrain to below 4 GB for 32-bit bus masters.
  //
  if ((Attributes & EDKII_IOMMU_ATTRIBUTE_DUAL_ADDRESS_CYCLE) != 0) {
    PhysAddr = MAX_UINT64;
    Status   = gBS->AllocatePages (
                      AllocateAnyPages,
                      MemoryType,
                      Pages,
                      &PhysAddr
                      );
  } else {
    PhysAddr = SIZE_4GB - 1;
    Status   = gBS->AllocatePages (
                      AllocateMaxAddress,
                      MemoryType,
                      Pages,
                      &PhysAddr
                      );
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  *HostAddress = (VOID *)(UINTN)PhysAddr;

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: Pages=0x%Lx HostAddress=0x%Lx\n",
    __func__,
    (UINT64)Pages,
    PhysAddr
    ));

  return EFI_SUCCESS;
}

/**
  Frees memory allocated by AllocateBuffer().

  @param[in]  This         The protocol instance pointer.
  @param[in]  Pages        Number of pages to free.
  @param[in]  HostAddress  Base address of the allocation.

  @retval EFI_SUCCESS             Pages freed.
  @retval EFI_INVALID_PARAMETER   HostAddress is NULL.
**/
STATIC
EFI_STATUS
EFIAPI
RiscVIoMmuFreeBuffer (
  IN EDKII_IOMMU_PROTOCOL  *This,
  IN UINTN                 Pages,
  IN VOID                  *HostAddress
  )
{
  if (HostAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: Pages=0x%Lx HostAddress=0x%p\n",
    __func__,
    (UINT64)Pages,
    HostAddress
    ));

  return gBS->FreePages ((EFI_PHYSICAL_ADDRESS)(UINTN)HostAddress, Pages);
}

/**
  Discover the RISC-V IOMMU MMIO base address from the Flattened Device Tree.

  @param[out]  IoMmuBase  Receives the IOMMU MMIO physical base address.
  @param[out]  IoMmuSize  Receives the IOMMU MMIO region size in bytes.

  @retval EFI_SUCCESS       Node found; IoMmuBase and IoMmuSize are valid.
  @retval EFI_NOT_FOUND     No "riscv,iommu" compatible node in the FDT.
  @retval EFI_PROTOCOL_ERROR  "reg" property missing or has unexpected size.
  @retval other             FDT_CLIENT_PROTOCOL could not be located.
**/
STATIC
EFI_STATUS
DiscoverIoMmuBase (
  OUT UINT64  *IoMmuBase,
  OUT UINT64  *IoMmuSize
  )
{
  EFI_STATUS           Status;
  FDT_CLIENT_PROTOCOL  *FdtClient;
  INT32                Node;
  CONST UINT64         *Reg;
  UINT32               PropSize;

  if ((IoMmuBase == NULL) || (IoMmuSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *IoMmuBase = 0;
  *IoMmuSize = 0;

  Status = gBS->LocateProtocol (
                  &gFdtClientProtocolGuid,
                  NULL,
                  (VOID **)&FdtClient
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to locate FDT_CLIENT_PROTOCOL (%r)\n",
      __func__,
      Status
      ));
    return Status;
  }

  Status = FdtClient->FindCompatibleNode (
                        FdtClient,
                        "riscv,iommu",
                        &Node
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "%a: No 'riscv,iommu' compatible DT node found (%r)\n",
      __func__,
      Status
      ));
    return EFI_NOT_FOUND;
  }

  Status = FdtClient->GetNodeProperty (
                        FdtClient,
                        Node,
                        "reg",
                        (CONST VOID **)&Reg,
                        &PropSize
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: 'reg' property not found on 'riscv,iommu' node (%r)\n",
      __func__,
      Status
      ));
    return EFI_PROTOCOL_ERROR;
  }

  if (PropSize != 2 * sizeof (UINT64)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: 'reg' property has unexpected size %u (expected %u)\n",
      __func__,
      PropSize,
      (UINT32)(2 * sizeof (UINT64))
      ));
    return EFI_PROTOCOL_ERROR;
  }

  *IoMmuBase = SwapBytes64 (Reg[0]);
  *IoMmuSize = SwapBytes64 (Reg[1]);

  DEBUG ((
    DEBUG_INFO,
    "%a: Found 'riscv,iommu' node: base=0x%Lx size=0x%Lx\n",
    __func__,
    *IoMmuBase,
    *IoMmuSize
    ));

  return EFI_SUCCESS;
}

/**
  Add the IOMMU MMIO region to the GCD memory map and set the
  uncacheable (EFI_MEMORY_UC) attribute.

  @param[in]  Base  Physical base address of the MMIO region.
  @param[in]  Size  Size in bytes of the MMIO region.

  @retval EFI_SUCCESS       Region added and attribute set.
  @retval EFI_ALREADY_STARTED  Region already present in GCD (non-fatal;
                               treated as success by the caller).
  @retval other             gDS call failed.
**/
STATIC
EFI_STATUS
AddIoMmuMmioSpace (
  IN UINT64  Base,
  IN UINT64  Size
  )
{
  EFI_STATUS  Status;

  Status = gDS->AddMemorySpace (
                  EfiGcdMemoryTypeMemoryMappedIo,
                  Base,
                  Size,
                  EFI_MEMORY_UC
                  );
  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to add GCD MMIO space [0x%Lx+0x%Lx): %r\n",
      __func__,
      Base,
      Size,
      Status
      ));
    return Status;
  }

  Status = gDS->SetMemorySpaceAttributes (Base, Size, EFI_MEMORY_UC);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to set EFI_MEMORY_UC on [0x%Lx+0x%Lx): %r\n",
      __func__,
      Base,
      Size,
      Status
      ));
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: IOMMU MMIO [0x%Lx+0x%Lx) added to GCD as UC\n",
    __func__,
    Base,
    Size
    ));

  return EFI_SUCCESS;
}

/**
  Poll ddtp.busy until the hardware clears it or timeout expires.

  @param[in]  RegisterBase  IOMMU MMIO base address.

  @retval EFI_SUCCESS   Busy bit cleared within timeout.
  @retval EFI_TIMEOUT   Busy bit did not clear within timeout.
**/
STATIC
EFI_STATUS
WaitForDdtpNotBusy (
  IN UINT64  *RegisterBase
  )
{
  UINT64  Ddtp;
  UINTN   Elapsed;

  Elapsed = 0;

  do {
    Ddtp = IommuRead64 (RegisterBase, RISCV_IOMMU_REG_DDTP);
    if ((Ddtp & RISCV_IOMMU_DDTP_BUSY) == 0) {
      return EFI_SUCCESS;
    }

    MicroSecondDelay (10);
    Elapsed += 10;
  } while (Elapsed < RISCV_IOMMU_DDTP_BUSY_TIMEOUT_US);

  DEBUG ((
    DEBUG_ERROR,
    "%a: Timeout waiting for ddtp.busy to clear (ddtp=0x%Lx)\n",
    __func__,
    Ddtp
    ));

  return EFI_TIMEOUT;
}

/**
  Initialize the RISC-V IOMMU hardware in bare (pass-through) mode.

    Steps performed:
    1. Add the MMIO region to the GCD memory map (EFI_MEMORY_UC).
    2. Read and validate the capabilities register.
    3. Write ddtp with iommu_mode = Bare (0x1), PPN = 0.
    4. Poll ddtp.busy until the hardware acknowledges the write.
    5. Verify the mode was accepted by reading ddtp back.

  @param[in]  IoMmuBase  Physical base address of the IOMMU MMIO region.
  @param[in]  IoMmuSize  Size in bytes of the IOMMU MMIO region.

  @retval EFI_SUCCESS       IOMMU initialized in bare mode.
  @retval EFI_DEVICE_ERROR  Hardware did not accept the bare mode setting.
  @retval EFI_TIMEOUT       ddtp.busy did not clear within timeout.
  @retval EFI_UNSUPPORTED   capabilities.version is not recognized.
**/
STATIC
EFI_STATUS
InitializeIoMmuBareMode (
  IN UINT64  IoMmuBase,
  IN UINT64  IoMmuSize
  )
{
  EFI_STATUS  Status;
  UINT64      Capabilities;
  UINT64      Version;
  UINT64      Ddtp;
  UINT64      *RegisterBase;

  Status = AddIoMmuMmioSpace (IoMmuBase, IoMmuSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Use the physical address directly as the MMIO base.
  // On RISC-V with identity-mapped firmware, VA == PA during DXE.
  //
  RegisterBase = (UINT64 *)IoMmuBase;

  Capabilities = IommuRead64 (RegisterBase, RISCV_IOMMU_REG_CAPABILITIES);
  Version      = Capabilities & RISCV_IOMMU_CAP_VERSION_MASK;

  DEBUG ((
    DEBUG_INFO,
    "%a: IOMMU capabilities = 0x%Lx (version=%Lu)\n",
    __func__,
    Capabilities,
    Version
    ));

  //
  // Spec version field: major.minor encoded as (major << 4 | minor).
  // Version 0x10 = 1.0, 0x11 = 1.1, etc.
  // Reject anything below 1.0.
  //
  if (Version < 0x10) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Unsupported IOMMU version 0x%Lx (expected >= 0x10)\n",
      __func__,
      Version
      ));
    return EFI_UNSUPPORTED;
  }

  Ddtp = IommuRead64 (RegisterBase, RISCV_IOMMU_REG_DDTP);
  DEBUG ((
    DEBUG_INFO,
    "%a: Current ddtp = 0x%Lx (mode=%Lu)\n",
    __func__,
    Ddtp,
    Ddtp & RISCV_IOMMU_DDTP_MODE_MASK
    ));

  if ((Ddtp & RISCV_IOMMU_DDTP_MODE_MASK) == RISCV_IOMMU_DDTP_MODE_BARE) {
    DEBUG ((DEBUG_INFO, "%a: IOMMU already in bare mode\n", __func__));
    mIoMmuContext.RegisterBase = RegisterBase;
    mIoMmuContext.Capabilities = Capabilities;
    return EFI_SUCCESS;
  }

  //
  // Write ddtp with iommu_mode = Bare.
  //
  // Per spec: when setting mode to Bare, the PPN field must be zero.
  // Preserve no other bits — write a clean value.
  //
  Ddtp = RISCV_IOMMU_DDTP_MODE_BARE;

  DEBUG ((
    DEBUG_INFO,
    "%a: Writing ddtp = 0x%Lx (mode=Bare)\n",
    __func__,
    Ddtp
    ));

  Status = WaitForDdtpNotBusy (RegisterBase);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  IommuWrite64 (RegisterBase, RISCV_IOMMU_REG_DDTP, Ddtp);

  Status = WaitForDdtpNotBusy (RegisterBase);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Read back ddtp and verify mode was accepted.
  //
  Ddtp = IommuRead64 (RegisterBase, RISCV_IOMMU_REG_DDTP);

  DEBUG ((
    DEBUG_INFO,
    "%a: ddtp readback = 0x%Lx (mode=%Lu)\n",
    __func__,
    Ddtp,
    Ddtp & RISCV_IOMMU_DDTP_MODE_MASK
    ));

  if ((Ddtp & RISCV_IOMMU_DDTP_MODE_MASK) != RISCV_IOMMU_DDTP_MODE_BARE) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: IOMMU did not accept bare mode (ddtp=0x%Lx)\n",
      __func__,
      Ddtp
      ));
    return EFI_DEVICE_ERROR;
  }

  mIoMmuContext.RegisterBase = RegisterBase;
  mIoMmuContext.Capabilities = Capabilities;

  DEBUG ((DEBUG_INFO, "%a: IOMMU successfully set to bare mode\n", __func__));

  return EFI_SUCCESS;
}

/**
  Entry point for the RISC-V IOMMU DXE driver.

  Discovers the IOMMU MMIO base, initializes the hardware in bare mode,
  and installs EDKII_IOMMU_PROTOCOL on a new handle.

  @param[in]  ImageHandle  The firmware-allocated handle for this image.
  @param[in]  SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS  Protocol installed (either IoMmu or IoMmuAbsent).
  @retval other        A fatal error occurred during installation.
**/
EFI_STATUS
EFIAPI
RiscVIoMmuEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINT64      IoMmuBase;
  UINT64      IoMmuSize;
  EFI_HANDLE  Handle;

  DEBUG ((DEBUG_INFO, "%a: RISC-V IOMMU DXE driver starting\n", __func__));

  //
  // Initialize the driver context.
  //
  ZeroMem (&mIoMmuContext, sizeof (mIoMmuContext));
  mIoMmuContext.Signature = RISCV_IOMMU_CONTEXT_SIGNATURE;
  InitializeListHead (&mIoMmuContext.MapInfoList);

  //
  // Populate the EDKII_IOMMU_PROTOCOL function pointers.
  //
  mIoMmuContext.IoMmu.Revision       = EDKII_IOMMU_PROTOCOL_REVISION;
  mIoMmuContext.IoMmu.SetAttribute   = RiscVIoMmuSetAttribute;
  mIoMmuContext.IoMmu.Map            = RiscVIoMmuMap;
  mIoMmuContext.IoMmu.Unmap          = RiscVIoMmuUnmap;
  mIoMmuContext.IoMmu.AllocateBuffer = RiscVIoMmuAllocateBuffer;
  mIoMmuContext.IoMmu.FreeBuffer     = RiscVIoMmuFreeBuffer;

  //
  // Discover the IOMMU MMIO base address from the FDT.
  //
  Status = DiscoverIoMmuBase (&IoMmuBase, &IoMmuSize);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_WARN,
      "%a: No IOMMU base found (%r)\n",
      __func__,
      Status
      ));
    return EFI_NOT_FOUND;
  }

  //
  // Initialize the IOMMU hardware in bare mode.
  //
  Status = InitializeIoMmuBareMode (IoMmuBase, IoMmuSize);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_WARN,
      "%a: IOMMU bare mode init failed (%r), installing IoMmuAbsent\n",
      __func__,
      Status
      ));
    return EFI_DEVICE_ERROR;
  }

  //
  // Install EDKII_IOMMU_PROTOCOL.
  //
  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEdkiiIoMmuProtocolGuid,
                  &mIoMmuContext.IoMmu,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to install EDKII_IOMMU_PROTOCOL (%r)\n",
      __func__,
      Status
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: EDKII_IOMMU_PROTOCOL installed (bare mode)\n",
    __func__
    ));

  return Status;
}
