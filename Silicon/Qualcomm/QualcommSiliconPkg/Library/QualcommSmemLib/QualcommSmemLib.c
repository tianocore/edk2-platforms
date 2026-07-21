/** @file
  Qualcomm Shared Memory (SMEM) - Common Core.

  Implements SMEM initialization, partition mapping, and item lookup.
  Platform-specific operations are delegated to the interface defined in
  QualcommSmemPlatform.h.  Internal protocol constants, ABI structures, and
  driver state type are defined in QualcommSmemInternal.h.

  @par Glossary:
    - Abi  - Application Binary Interface
    - Smem - Shared Memory
    - Toc  - Table of Contents

  Copyright (C) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/QualcommSmemLib.h>
#include "QualcommSmemInternal.h"
#include "QualcommSmemPlatform.h"

static QUALCOMM_SMEM_INFO  mQualcommSmemInfo;

// -------------------------------------------------------------------------
// Internal helpers
// -------------------------------------------------------------------------

/**
  Read a 16-bit value from shared memory.

  Uses CopyMem to avoid undefined behaviour on unaligned reads.

  @param[in]  Ptr  Pointer to the source location in shared memory.

  @return  The 16-bit value at Ptr.
**/
static
UINT16
SmemRd16 (
  IN  CONST VOID  *Ptr
  )
{
  UINT16  Value;

  CopyMem (&Value, Ptr, sizeof (Value));
  return Value;
}

/**
  Read a 32-bit value from shared memory.

  Uses CopyMem to avoid undefined behaviour on unaligned reads.

  @param[in]  Ptr  Pointer to the source location in shared memory.

  @return  The 32-bit value at Ptr.
**/
static
UINT32
SmemRd32 (
  IN  CONST VOID  *Ptr
  )
{
  UINT32  Value;

  CopyMem (&Value, Ptr, sizeof (Value));
  return Value;
}

/**
  Validate a single TOC entry.

  Checks that the partition described by Entry has a non-zero size and
  lies entirely within the SMEM region.

  @param[in]  Entry  TOC entry to validate.

  @retval  EFI_SUCCESS       Entry is valid.
  @retval  EFI_DEVICE_ERROR  Entry is malformed.
**/
static
EFI_STATUS
SmemValidateTocEntry (
  IN  CONST QUALCOMM_SMEM_TOC_ENTRY  *Entry
  )
{
  UINT32  Off;
  UINT32  Size;
  UINT64  End;

  Off  = SmemRd32 (&Entry->Offset);
  Size = SmemRd32 (&Entry->Size);

  // Partition must be large enough to hold the partition header.
  if (Size < (UINT32)sizeof (QUALCOMM_SMEM_PARTITION_HEADER)) {
    return EFI_DEVICE_ERROR;
  }

  // Offset + Size must not overflow and must lie within SMEM.
  End = (UINT64)Off + (UINT64)Size;
  if (End > (UINT64)mQualcommSmemInfo.SmemSize) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Check if a TOC entry involves the local host.

  A partition is relevant to the local host if it is:
    - the common partition (Host0 == Host1 == QUALCOMM_SMEM_HOST_COMMON), or
    - an edge-pair partition where LocalHost is one of the endpoints.

  @param[in]  Entry  TOC entry to test.

  @retval  TRUE   Partition involves the local host.
  @retval  FALSE  Partition does not involve the local host.
**/
static
BOOLEAN
SmemPartInvolvesLocal (
  IN  CONST QUALCOMM_SMEM_TOC_ENTRY  *Entry
  )
{
  UINT16  Lh;
  UINT16  H0;
  UINT16  H1;

  Lh = (UINT16)mQualcommSmemInfo.LocalHost;
  H0 = SmemRd16 (&Entry->Host0);
  H1 = SmemRd16 (&Entry->Host1);

  // Common partition - accessible to all hosts.
  if ((H0 == (UINT16)QUALCOMM_SMEM_HOST_COMMON) &&
      (H1 == (UINT16)QUALCOMM_SMEM_HOST_COMMON))
  {
    return TRUE;
  }

  // Edge-pair partition: local host is one endpoint.
  return ((H0 == Lh) || (H1 == Lh)) ? TRUE : FALSE;
}

/**
  Check if a TOC entry matches the requested host pair.

  - If Host == QUALCOMM_SMEM_HOST_COMMON: match the common partition.
  - Otherwise: match an edge-pair partition where one endpoint is
    LocalHost and the other is Host.

  @param[in]  Entry  TOC entry to test.
  @param[in]  Host   Requested remote host.

  @retval  TRUE   Entry matches the requested host pair.
  @retval  FALSE  Entry does not match.
**/
static
BOOLEAN
SmemPartMatches (
  IN  CONST QUALCOMM_SMEM_TOC_ENTRY  *Entry,
  IN  QUALCOMM_SMEM_HOST             Host
  )
{
  UINT16  Lh;
  UINT16  Rh;
  UINT16  H0;
  UINT16  H1;

  Lh = (UINT16)mQualcommSmemInfo.LocalHost;
  Rh = (UINT16)Host;
  H0 = SmemRd16 (&Entry->Host0);
  H1 = SmemRd16 (&Entry->Host1);

  // Common partition lookup.
  if (Host == QUALCOMM_SMEM_HOST_COMMON) {
    return ((H0 == (UINT16)QUALCOMM_SMEM_HOST_COMMON) &&
            (H1 == (UINT16)QUALCOMM_SMEM_HOST_COMMON)) ? TRUE : FALSE;
  }

  // Edge-pair: one endpoint must be local, the other must be remote.
  return (((H0 == Lh) && (H1 == Rh)) ||
          ((H0 == Rh) && (H1 == Lh))) ? TRUE : FALSE;
}

/**
  Scan the uncached/upward item list in a partition.

  Items are stored sequentially starting immediately after the partition
  header, growing upward toward ScanLimit.  Each item occupies:

    [QUALCOMM_SMEM_ITEM_HEADER][PaddingHeader bytes][data (Size bytes)]

  where the last PaddingData bytes of data are unused padding.

  @param[in]   Base       Base address of the partition in virtual memory.
  @param[in]   ScanLimit  Offset of the first free uncached byte (exclusive).
  @param[in]   ItemId     Item identifier to search for.
  @param[out]  Addr       Set to the item payload address on success.
  @param[out]  Size       Set to the item payload size on success (may be NULL).

  @retval  EFI_SUCCESS       Item found; Addr and Size (if non-NULL) are set.
  @retval  EFI_NOT_FOUND     Item not present in the uncached region.
  @retval  EFI_DEVICE_ERROR  Corrupted item metadata detected.
**/
static
EFI_STATUS
SmemScanUncached (
  IN      CONST UINT8  *Base,
  IN      UINT32       ScanLimit,
  IN      UINT16       ItemId,
  OUT     VOID         **Addr,
  OUT     UINTN        *Size       OPTIONAL
  )
{
  CONST UINT8                      *Limit;
  CONST UINT8                      *Ptr;
  CONST QUALCOMM_SMEM_ITEM_HEADER  *Ihdr;
  UINT32                           ItemSize;
  UINT32                           Step;

  // ScanLimit must cover at least the partition header.
  if (ScanLimit < (UINT32)sizeof (QUALCOMM_SMEM_PARTITION_HEADER)) {
    return EFI_DEVICE_ERROR;
  }

  Limit = Base + ScanLimit;
  Ptr   = Base + sizeof (QUALCOMM_SMEM_PARTITION_HEADER);

  while (Ptr < Limit) {
    // Ensure there is room for a complete item header.
    if ((UINTN)Limit - (UINTN)Ptr < sizeof (QUALCOMM_SMEM_ITEM_HEADER)) {
      return EFI_DEVICE_ERROR;
    }

    Ihdr = (CONST QUALCOMM_SMEM_ITEM_HEADER *)(CONST VOID *)Ptr;

    // Validate canary - detects corruption or scan overrun.
    if (SmemRd16 (&Ihdr->Canary) != (UINT16)QUALCOMM_SMEM_ITEM_CANARY) {
      return EFI_DEVICE_ERROR;
    }

    ItemSize = SmemRd32 (&Ihdr->Size);

    // Size must be non-zero; PaddingData must not exceed Size.
    if (ItemSize == 0U) {
      return EFI_DEVICE_ERROR;
    }

    if ((UINT32)SmemRd16 (&Ihdr->PaddingData) > ItemSize) {
      return EFI_DEVICE_ERROR;
    }

    //
    // Step = sizeof (header) + PaddingHeader + ItemSize.
    // A wrapped result is always < ItemSize; detect overflow.
    //
    Step = (UINT32)sizeof (QUALCOMM_SMEM_ITEM_HEADER) +
           (UINT32)SmemRd16 (&Ihdr->PaddingHeader) +
           ItemSize;
    if (Step < ItemSize) {
      return EFI_DEVICE_ERROR;
    }

    if ((UINTN)Limit - (UINTN)Ptr < (UINTN)Step) {
      return EFI_DEVICE_ERROR;
    }

    if (SmemRd16 (&Ihdr->Item) == ItemId) {
      //
      // Found: payload starts after header + PaddingHeader.
      // Cast away CONST: callers may write to the payload.
      //
      *Addr = (VOID *)(Ptr +
                       sizeof (QUALCOMM_SMEM_ITEM_HEADER) +
                       (UINTN)SmemRd16 (&Ihdr->PaddingHeader));
      if (Size != NULL) {
        *Size = (UINTN)(ItemSize - (UINTN)SmemRd16 (&Ihdr->PaddingData));
      }

      return EFI_SUCCESS;
    }

    Ptr += (UINTN)Step;
  }

  return EFI_NOT_FOUND;
}

/**
  Search a single partition for an item.

  Retrieves the partition virtual address, validates the static header
  fields (magic, size), issues a memory barrier, acquires the HW lock,
  reads and validates the mutable heap pointers, then delegates to
  SmemScanUncached().

  The HW lock is released on every return path after a successful acquire.

  @param[in]   PartOffset  Byte offset of the partition from SMEM base.
  @param[in]   PartSize    Partition size in bytes.
  @param[in]   ItemId      Item identifier to search for.
  @param[out]  Addr        Set to the item payload address on success.
  @param[out]  Size        Set to the item payload size on success (may be NULL).

  @retval  EFI_SUCCESS       Item found.
  @retval  EFI_NOT_FOUND     Item not found in uncached region.
  @retval  EFI_DEVICE_ERROR  Corrupted partition metadata.
  @retval  EFI_ACCESS_DENIED Partition not mapped.
**/
static
EFI_STATUS
SmemSearchPartition (
  IN      UINT32  PartOffset,
  IN      UINT32  PartSize,
  IN      UINT16  ItemId,
  OUT     VOID    **Addr,
  OUT     UINTN   *Size       OPTIONAL
  )
{
  CONST QUALCOMM_SMEM_PARTITION_HEADER  *Phdr;
  VOID                                  *Va;
  EFI_STATUS                            Status;
  UINT32                                OffsetFreeUncached;
  UINT32                                OffsetFreeCached;
  UINT32                                MinUncached;

  // Validate partition range before any memory access.
  if (PartSize < (UINT32)sizeof (QUALCOMM_SMEM_PARTITION_HEADER)) {
    return EFI_DEVICE_ERROR;
  }

  if ((UINT64)PartOffset + (UINT64)PartSize > (UINT64)mQualcommSmemInfo.SmemSize) {
    return EFI_DEVICE_ERROR;
  }

  Va = QualcommSmemPlatGetAddr (PartOffset);
  if (Va == NULL) {
    return EFI_ACCESS_DENIED;
  }

  Phdr = (CONST QUALCOMM_SMEM_PARTITION_HEADER *)Va;

  //
  // Validate static partition fields.
  // Magic and Size are written once at partition creation time and
  // never change, so they can be read without the HW lock.
  //
  if (SmemRd32 (&Phdr->Magic) != QUALCOMM_SMEM_PART_MAGIC) {
    QualcommSmemPlatLogErr (
      "smem: bad partition magic 0x%08x\n",
      (unsigned int)SmemRd32 (&Phdr->Magic)
      );
    return EFI_DEVICE_ERROR;
  }

  if (SmemRd32 (&Phdr->Size) != PartSize) {
    QualcommSmemPlatLogErr (
      "smem: partition size mismatch (header=%u toc=%u)\n",
      (unsigned int)SmemRd32 (&Phdr->Size),
      (unsigned int)PartSize
      );
    return EFI_DEVICE_ERROR;
  }

  //
  // Memory barrier before reading mutable partition metadata.
  // Ensures writes by remote processors are visible on this core.
  //
  QualcommSmemPlatMemBarrier ();

  //
  // Acquire HW lock before reading mutable heap pointers.
  // Remote processors update OffsetFreeUncached and OffsetFreeCached
  // when allocating new items.
  //
  Status = QualcommSmemPlatHwlockAcquire ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Read mutable heap pointers under lock.
  OffsetFreeUncached = SmemRd32 (&Phdr->OffsetFreeUncached);
  OffsetFreeCached   = SmemRd32 (&Phdr->OffsetFreeCached);

  //
  // Validate heap pointers:
  //   OffsetFreeUncached >= sizeof (QUALCOMM_SMEM_PARTITION_HEADER)
  //   OffsetFreeUncached <= OffsetFreeCached
  //   OffsetFreeCached   <= PartSize
  //
  MinUncached = (UINT32)sizeof (QUALCOMM_SMEM_PARTITION_HEADER);
  if ((OffsetFreeUncached < MinUncached) ||
      (OffsetFreeUncached > OffsetFreeCached) ||
      (OffsetFreeCached > PartSize))
  {
    QualcommSmemPlatHwlockRelease ();
    QualcommSmemPlatLogErr (
      "smem: invalid partition heap pointers uncached=%u cached=%u size=%u\n",
      (unsigned int)OffsetFreeUncached,
      (unsigned int)OffsetFreeCached,
      (unsigned int)PartSize
      );
    return EFI_DEVICE_ERROR;
  }

  Status = SmemScanUncached (
             (CONST UINT8 *)Va,
             OffsetFreeUncached,
             ItemId,
             Addr,
             Size
             );

  // Release HW lock on every return path after successful acquire.
  QualcommSmemPlatHwlockRelease ();
  return Status;
}

/**
  Validate the BOOT SMEM version word.

  Reads Ver[QUALCOMM_SMEM_VERSION_BOOT_OFFSET] from the static header and
  checks that the major version matches QUALCOMM_SMEM_VERSION_ID.  A minor
  version mismatch is tolerated with a warning log.

  Called from QualcommSmemInit() after the BOOT info page has been mapped.

  @param[in]  StaticHdr  Pointer to the mapped BOOT info page.

  @retval  EFI_SUCCESS      Major version matches.
  @retval  EFI_UNSUPPORTED  Major version mismatch; SMEM layout is incompatible.
**/
static
EFI_STATUS
SmemValidateBootVersion (
  IN  CONST QUALCOMM_SMEM_STATIC_HEADER  *StaticHdr
  )
{
  UINT32  BootVersion;
  UINT32  BootMajor;
  UINT32  LocalMajor;

  BootVersion = SmemRd32 (&StaticHdr->Ver[QUALCOMM_SMEM_VERSION_BOOT_OFFSET]);
  BootMajor   = BootVersion & QUALCOMM_SMEM_MAJOR_VERSION_MASK;
  LocalMajor  = QUALCOMM_SMEM_VERSION_ID & QUALCOMM_SMEM_MAJOR_VERSION_MASK;

  if (BootMajor != LocalMajor) {
    QualcommSmemPlatLogErr (
      "smem: BOOT version major mismatch: got 0x%08x expected 0x%08x\n",
      (unsigned int)BootVersion,
      (unsigned int)LocalMajor
      );
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Validate the TOC header and entry count.

  Checks the TOC magic word, version, and NumEntries field.  Writes the
  validated entry count to *NumEntriesOut on success.

  Called from QualcommSmemInit() after the TOC page has been mapped and a
  memory barrier has been issued.

  @param[in]   Toc            Pointer to the mapped TOC header.
  @param[out]  NumEntriesOut  Receives the validated entry count.

  @retval  EFI_SUCCESS       TOC header is valid; *NumEntriesOut is set.
  @retval  EFI_DEVICE_ERROR  Bad magic or invalid entry count.
  @retval  EFI_UNSUPPORTED   Unsupported TOC version.
**/
static
EFI_STATUS
SmemValidateTocHeader (
  IN   CONST QUALCOMM_SMEM_TOC_HEADER  *Toc,
  OUT  UINT32                          *NumEntriesOut
  )
{
  UINT32  NumEntries;

  if (SmemRd32 (&Toc->Magic) != QUALCOMM_SMEM_TOC_MAGIC) {
    QualcommSmemPlatLogErr (
      "smem: bad TOC magic 0x%08x\n",
      (unsigned int)SmemRd32 (&Toc->Magic)
      );
    return EFI_DEVICE_ERROR;
  }

  if (SmemRd32 (&Toc->Version) != QUALCOMM_SMEM_TOC_VERSION) {
    QualcommSmemPlatLogErr (
      "smem: unsupported TOC version %u\n",
      (unsigned int)SmemRd32 (&Toc->Version)
      );
    return EFI_UNSUPPORTED;
  }

  NumEntries = SmemRd32 (&Toc->NumEntries);
  if ((NumEntries == 0U) || (NumEntries > QUALCOMM_SMEM_TOC_MAX_ENTRIES)) {
    QualcommSmemPlatLogErr (
      "smem: invalid TOC NumEntries %u\n",
      (unsigned int)NumEntries
      );
    return EFI_DEVICE_ERROR;
  }

  // Entries array must fit within the TOC page.
  if ((UINT32)sizeof (QUALCOMM_SMEM_TOC_HEADER) +
      NumEntries * (UINT32)sizeof (QUALCOMM_SMEM_TOC_ENTRY) >
      QUALCOMM_SMEM_TOC_SIZE)
  {
    QualcommSmemPlatLogErr ("smem: TOC entries overflow TOC page\n");
    return EFI_DEVICE_ERROR;
  }

  *NumEntriesOut = NumEntries;
  return EFI_SUCCESS;
}

/**
  Walk the TOC, map local partitions, and cache the common partition.

  Iterates over all NumEntries TOC entries.  For each entry that:
    - passes SmemValidateTocEntry() (bounds check), and
    - passes SmemPartInvolvesLocal() (involves the local host),
  the partition is mapped read-write via QualcommSmemPlatMap().

  Additionally, if the common partition (Host0 == Host1 ==
  QUALCOMM_SMEM_HOST_COMMON) is found, its offset and size are stored in
  mQualcommSmemInfo for fast O(1) access by QualcommSmemLookup().

  Unrelated partitions are deliberately not mapped to prevent speculative
  CPU accesses to regions that may not be accessible from this security
  domain.

  Called from QualcommSmemInit() after mQualcommSmemInfo.LocalHost,
  mQualcommSmemInfo.SmemSize, and mQualcommSmemInfo.NumTocEntries have been
  populated.

  @param[in]  Entries     Pointer to the first TOC entry.
  @param[in]  NumEntries  Number of TOC entries to process.

  @retval  EFI_SUCCESS  All relevant partitions mapped successfully.
  @retval  other        First mapping failure; mQualcommSmemInfo is cleared by caller.
**/
static
EFI_STATUS
SmemMapPartitions (
  IN  CONST QUALCOMM_SMEM_TOC_ENTRY  *Entries,
  IN  UINT32                         NumEntries
  )
{
  UINT32                         Index;
  EFI_STATUS                     Status;
  CONST QUALCOMM_SMEM_TOC_ENTRY  *Entry;

  for (Index = 0U; Index < NumEntries; Index++) {
    Entry = &Entries[Index];

    if (EFI_ERROR (SmemValidateTocEntry (Entry))) {
      continue; // skip malformed entries silently
    }

    if (!SmemPartInvolvesLocal (Entry)) {
      continue; // not relevant partition - do not map
    }

    Status = QualcommSmemPlatMap (
               SmemRd32 (&Entry->Offset),
               (UINTN)SmemRd32 (&Entry->Size),
               QUALCOMM_SMEM_PLAT_MAP_RW
               );

    if (EFI_ERROR (Status)) {
      QualcommSmemPlatLogErr (
        "smem: failed to map partition host0=%u host1=%u offset=%u: 0x%lx\n",
        (unsigned int)SmemRd16 (&Entry->Host0),
        (unsigned int)SmemRd16 (&Entry->Host1),
        (unsigned int)SmemRd32 (&Entry->Offset),
        (unsigned long)(UINTN)Status
        );
      return Status;
    }

    if ((SmemRd16 (&Entry->Host0) == (UINT16)QUALCOMM_SMEM_HOST_COMMON) &&
        (SmemRd16 (&Entry->Host1) == (UINT16)QUALCOMM_SMEM_HOST_COMMON) &&
        (mQualcommSmemInfo.CommonPartOffset == 0U))
    {
      mQualcommSmemInfo.CommonPartOffset = SmemRd32 (&Entry->Offset);
      mQualcommSmemInfo.CommonPartSize   = SmemRd32 (&Entry->Size);
    }
  }

  return EFI_SUCCESS;
}

// -------------------------------------------------------------------------
// Public API
// -------------------------------------------------------------------------

/**
  Construct a SMEM host identifier.

  Constructs a valid SMEM host identifier from the given parameters.
  The internal bit encoding is an implementation detail; callers must
  use this function rather than constructing host values directly.

  @param[in]   ProcId   Processor ID (use QUALCOMM_SMEM_PROC_* macros).
  @param[in]   ProcNum  Processor instance number.
  @param[in]   PdNum    Protection-domain number.
  @param[in]   Chiplet  Chiplet identifier.
  @param[out]  Host     Output host identifier (must not be NULL).

  @retval  EFI_SUCCESS            Success.
  @retval  EFI_INVALID_PARAMETER  Host is NULL, any parameter is out of range,
                                  or the encoded value would collide with a
                                  reserved host identifier.
**/
EFI_STATUS
EFIAPI
QualcommSmemHostId (
  IN   UINT16              ProcId,
  IN   UINT16              ProcNum,
  IN   UINT16              PdNum,
  IN   UINT16              Chiplet,
  OUT  QUALCOMM_SMEM_HOST  *Host
  )
{
  UINT16  Encoded;

  if (Host == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Range checks: each field must fit in its allocated bit-width.
  if ((UINT32)ProcId > HOST_PROC_ID_MASK) {
    return EFI_INVALID_PARAMETER;
  }

  if ((UINT32)ProcNum > HOST_PROC_NUM_MASK) {
    return EFI_INVALID_PARAMETER;
  }

  if ((UINT32)PdNum > HOST_PD_NUM_MASK) {
    return EFI_INVALID_PARAMETER;
  }

  if ((UINT32)Chiplet > HOST_CHIPLET_MASK) {
    return EFI_INVALID_PARAMETER;
  }

  Encoded = (UINT16)(
                     ((UINT16)ProcId  << HOST_PROC_ID_SHIFT)  |
                     ((UINT16)ProcNum << HOST_PROC_NUM_SHIFT) |
                     ((UINT16)PdNum   << HOST_PD_NUM_SHIFT)   |
                     ((UINT16)Chiplet << HOST_CHIPLET_SHIFT));

  //
  // Reject any encoding that collides with a reserved/special host.
  // These checks protect against accidental construction of reserved
  // values even when the individual field ranges permit it.
  //
  if (Encoded == (UINT16)QUALCOMM_SMEM_HOST_COMMON) {
    return EFI_INVALID_PARAMETER;
  }

  if (Encoded == (UINT16)QUALCOMM_SMEM_HOST_INVALID) {
    return EFI_INVALID_PARAMETER;
  }

  if (Encoded == (UINT16)QUALCOMM_SMEM_HOST_MULTIHOST) {
    return EFI_INVALID_PARAMETER;
  }

  *Host = (QUALCOMM_SMEM_HOST)Encoded;
  return EFI_SUCCESS;
}

/**
  Initialize the SMEM common core.

  Performs the following steps:
    1. Calls QualcommSmemPlatInit() to obtain platform target parameters.
    2. Validates PlatInfo fields.
    3. Maps the first 4 KiB BOOT/static metadata page read-only.
    4. Calls SmemValidateBootVersion() to check the BOOT SMEM version.
    5. Maps the TOC page (last 4 KiB) read-only.
    6. Calls SmemValidateTocHeader() to check TOC magic/version/count.
    7. Calls SmemMapPartitions() to map local partitions and cache common.
    8. Marks the driver as initialized.

  @retval  EFI_SUCCESS          Success.
  @retval  EFI_ALREADY_STARTED  Already initialized.
  @retval  other                EFI_STATUS error code on failure.
**/
EFI_STATUS
EFIAPI
QualcommSmemInit (
  VOID
  )
{
  QUALCOMM_SMEM_PLAT_INFO            PlatInfo;
  CONST QUALCOMM_SMEM_STATIC_HEADER  *StaticHdr;
  CONST QUALCOMM_SMEM_TOC_HEADER     *Toc;
  CONST QUALCOMM_SMEM_TOC_ENTRY      *Entries;
  VOID                               *Va;
  UINT32                             SmemSize;
  UINT32                             TocOffset;
  UINT32                             NumEntries;
  EFI_STATUS                         Status;

  if (mQualcommSmemInfo.Initialized) {
    return EFI_ALREADY_STARTED;
  }

  // Step 1: obtain platform target parameters.
  Status = QualcommSmemPlatInit (&PlatInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Step 2: validate PlatInfo fields.
  if (PlatInfo.LocalHost == (QUALCOMM_SMEM_HOST)QUALCOMM_SMEM_HOST_INVALID) {
    QualcommSmemPlatLogErr ("smem: invalid LocalHost\n");
    return EFI_INVALID_PARAMETER;
  }

  //
  // SmemSize must hold at least the BOOT info page and the TOC page
  // without overlap.  The constant sum cannot overflow UINTN.
  //
  if (PlatInfo.SmemSize <
      (UINTN)(QUALCOMM_SMEM_BOOT_INFO_SIZE + QUALCOMM_SMEM_TOC_SIZE))
  {
    QualcommSmemPlatLogErr ("smem: SmemSize too small\n");
    return EFI_INVALID_PARAMETER;
  }

  // SmemSize must fit in UINT32 (offsets are UINT32).
  if (PlatInfo.SmemSize > (UINTN)(UINT32)(-1)) {
    QualcommSmemPlatLogErr ("smem: SmemSize exceeds UINT32\n");
    return EFI_INVALID_PARAMETER;
  }

  if (PlatInfo.MaxItems == 0U) {
    QualcommSmemPlatLogErr ("smem: MaxItems is zero\n");
    return EFI_INVALID_PARAMETER;
  }

  SmemSize  = (UINT32)PlatInfo.SmemSize;
  TocOffset = SmemSize - QUALCOMM_SMEM_TOC_SIZE;

  // Step 3: map the BOOT/static metadata page read-only.
  Status = QualcommSmemPlatMap (
             0U,
             QUALCOMM_SMEM_BOOT_INFO_SIZE,
             QUALCOMM_SMEM_PLAT_MAP_RO
             );

  if (EFI_ERROR (Status)) {
    QualcommSmemPlatLogErr (
      "smem: failed to map BOOT info: 0x%lx\n",
      (unsigned long)(UINTN)Status
      );
    return Status;
  }

  Va = QualcommSmemPlatGetAddr (0U);
  if (Va == NULL) {
    QualcommSmemPlatLogErr ("smem: BOOT info not mapped\n");
    return EFI_DEVICE_ERROR;
  }

  StaticHdr = (CONST QUALCOMM_SMEM_STATIC_HEADER *)Va;

  // Step 4: validate the BOOT SMEM version.
  Status = SmemValidateBootVersion (StaticHdr);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Step 5: map the TOC page read-only.
  Status = QualcommSmemPlatMap (
             TocOffset,
             QUALCOMM_SMEM_TOC_SIZE,
             QUALCOMM_SMEM_PLAT_MAP_RO
             );

  if (EFI_ERROR (Status)) {
    QualcommSmemPlatLogErr (
      "smem: failed to map TOC: 0x%lx\n",
      (unsigned long)(UINTN)Status
      );
    return Status;
  }

  Va = QualcommSmemPlatGetAddr (TocOffset);
  if (Va == NULL) {
    QualcommSmemPlatLogErr ("smem: TOC not mapped\n");
    return EFI_DEVICE_ERROR;
  }

  Toc     = (CONST QUALCOMM_SMEM_TOC_HEADER *)Va;
  Entries = (CONST QUALCOMM_SMEM_TOC_ENTRY *)
            ((CONST UINT8 *)Va + sizeof (QUALCOMM_SMEM_TOC_HEADER));

  // Memory barrier before reading shared-memory metadata.
  QualcommSmemPlatMemBarrier ();

  // Step 6: validate the TOC header.
  Status = SmemValidateTocHeader (Toc, &NumEntries);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Populate mQualcommSmemInfo fields needed by SmemValidateTocEntry(),
  // SmemPartInvolvesLocal(), and SmemMapPartitions() before calling them.
  //
  mQualcommSmemInfo.LocalHost     = PlatInfo.LocalHost;
  mQualcommSmemInfo.MaxItems      = PlatInfo.MaxItems;
  mQualcommSmemInfo.SmemSize      = SmemSize;
  mQualcommSmemInfo.TocOffset     = TocOffset;
  mQualcommSmemInfo.NumTocEntries = NumEntries;

  //
  // Step 7: map local partitions and cache the common partition.
  //
  // SmemMapPartitions() walks the TOC, maps every partition that
  // involves the local host, and stores the common partition offset
  // and size in mQualcommSmemInfo for fast O(1) access.
  //
  Status = SmemMapPartitions (Entries, NumEntries);
  if (EFI_ERROR (Status)) {
    // Roll back partially populated state.
    ZeroMem (&mQualcommSmemInfo, sizeof (mQualcommSmemInfo));
    return Status;
  }

  // Step 8: mark driver as initialized.
  mQualcommSmemInfo.Initialized = TRUE;

  return EFI_SUCCESS;
}

/**
  Look up an existing SMEM item.

  Fast path (QUALCOMM_SMEM_HOST_COMMON):
    Uses the cached CommonPartOffset / CommonPartSize from mQualcommSmemInfo
    to call SmemSearchPartition() directly, bypassing the TOC walk.

  Slow path (edge-pair host):
    Walks the TOC to find a partition matching the requested host pair,
    then calls SmemSearchPartition() for the first matching entry.

  @param[in]   RemoteHost  Remote host for a host-pair partition, or
                           QUALCOMM_SMEM_HOST_COMMON for the common partition.
  @param[in]   Item        SMEM item ID.
  @param[in]   Flags       Must be QUALCOMM_SMEM_FLAG_NONE.
  @param[out]  ItemPtr     Pointer to item payload (must not be NULL).
  @param[out]  ItemSize    Size of item payload in bytes (may be NULL).

  @retval  EFI_SUCCESS            ItemPtr points to item payload.
  @retval  EFI_INVALID_PARAMETER  Invalid argument.
  @retval  EFI_NOT_STARTED        Driver not initialized.
  @retval  EFI_NOT_FOUND          Item or partition not found.
  @retval  EFI_ACCESS_DENIED      Partition not mapped.
  @retval  EFI_DEVICE_ERROR       Corrupted shared-memory metadata.
**/
EFI_STATUS
EFIAPI
QualcommSmemLookup (
  IN      QUALCOMM_SMEM_HOST  RemoteHost,
  IN      UINT16              Item,
  IN      UINT32              Flags,
  OUT     VOID                **ItemPtr,
  OUT     UINTN               *ItemSize    OPTIONAL
  )
{
  CONST QUALCOMM_SMEM_TOC_ENTRY  *Entries;
  CONST QUALCOMM_SMEM_TOC_ENTRY  *Entry;
  VOID                           *TocVa;
  UINT32                         Index;
  EFI_STATUS                     Status;

  // Validate arguments.
  if (ItemPtr == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (RemoteHost == QUALCOMM_SMEM_HOST_INVALID) {
    return EFI_INVALID_PARAMETER;
  }

  if (Flags != 0U) {
    return EFI_INVALID_PARAMETER;
  }

  if (!mQualcommSmemInfo.Initialized) {
    return EFI_NOT_STARTED;
  }

  if ((UINT32)Item >= (UINT32)mQualcommSmemInfo.MaxItems) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Fast path: common partition lookup.
  //
  // The common partition offset and size were cached during
  // QualcommSmemInit() by SmemMapPartitions().  Use them directly
  // to avoid walking the TOC on every common-partition lookup.
  //
  // If no common partition was found during init (CommonPartOffset == 0
  // and CommonPartSize == 0), fall through to the TOC walk which will
  // also return EFI_NOT_FOUND.
  //
  if (RemoteHost == QUALCOMM_SMEM_HOST_COMMON) {
    if ((mQualcommSmemInfo.CommonPartOffset != 0U) ||
        (mQualcommSmemInfo.CommonPartSize   != 0U))
    {
      return SmemSearchPartition (
               mQualcommSmemInfo.CommonPartOffset,
               mQualcommSmemInfo.CommonPartSize,
               Item,
               ItemPtr,
               ItemSize
               );
    }

    // No common partition mapped - item cannot exist.
    return EFI_NOT_FOUND;
  }

  //
  // Slow path: edge-pair partition lookup.
  //
  // Walk the TOC looking for a partition matching {LocalHost, RemoteHost}.
  // First matching valid partition wins.
  //
  TocVa = QualcommSmemPlatGetAddr (mQualcommSmemInfo.TocOffset);
  if (TocVa == NULL) {
    return EFI_DEVICE_ERROR;
  }

  Entries = (CONST QUALCOMM_SMEM_TOC_ENTRY *)
            ((CONST UINT8 *)TocVa + sizeof (QUALCOMM_SMEM_TOC_HEADER));

  // Memory barrier before reading TOC metadata.
  QualcommSmemPlatMemBarrier ();

  for (Index = 0U; Index < mQualcommSmemInfo.NumTocEntries; Index++) {
    Entry = &Entries[Index];

    if (EFI_ERROR (SmemValidateTocEntry (Entry))) {
      continue;
    }

    if (!SmemPartMatches (Entry, RemoteHost)) {
      continue;
    }

    Status = SmemSearchPartition (
               SmemRd32 (&Entry->Offset),
               SmemRd32 (&Entry->Size),
               Item,
               ItemPtr,
               ItemSize
               );

    if (Status != EFI_NOT_FOUND) {
      return Status; // found, or hard error
    }
  }

  return EFI_NOT_FOUND;
}
