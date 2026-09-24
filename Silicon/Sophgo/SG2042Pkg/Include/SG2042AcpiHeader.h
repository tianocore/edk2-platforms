/** @file
*
*  Copyright (c) 2018 - 2022, Arm Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef __SG2042_ACPI_HEADER__
#define __SG2042_ACPI_HEADER__

#include <IndustryStandard/Acpi.h>

//
// ACPI table information used to initialize tables.
//
#define EFI_ACPI_RISCV_OEM_ID           'S','O','P','H','G','O'
#define EFI_ACPI_RISCV_OEM_TABLE_ID     SIGNATURE_64 ('S','G','2','0',' ',' ',' ',' ')
#define EFI_ACPI_RISCV_OEM_REVISION     0x01
#define EFI_ACPI_RISCV_CREATOR_ID       SIGNATURE_32('S','G','2','0')
#define EFI_ACPI_RISCV_CREATOR_REVISION 0x00000099

// A macro to initialise the common header part of EFI ACPI tables as defined by
// EFI_ACPI_DESCRIPTION_HEADER structure.
#define RISCV_ACPI_HEADER(Signature, Type, Revision) {              \
    Signature,                        /* UINT32  Signature */       \
    sizeof (Type),                    /* UINT32  Length */          \
    Revision,                         /* UINT8   Revision */        \
    0,                                /* UINT8   Checksum */        \
    { EFI_ACPI_RISCV_OEM_ID },        /* UINT8   OemId[6] */        \
    EFI_ACPI_RISCV_OEM_TABLE_ID,      /* UINT64  OemTableId */      \
    EFI_ACPI_RISCV_OEM_REVISION,      /* UINT32  OemRevision */     \
    EFI_ACPI_RISCV_CREATOR_ID,        /* UINT32  CreatorId */       \
    EFI_ACPI_RISCV_CREATOR_REVISION   /* UINT32  CreatorRevision */ \
  }

#define CORE_COUNT      64
#define CLUSTER_COUNT   16

// ACPI OSC Status bits
#define OSC_STS_BIT0_RES              (1U << 0)
#define OSC_STS_FAILURE               (1U << 1)
#define OSC_STS_UNRECOGNIZED_UUID     (1U << 2)
#define OSC_STS_UNRECOGNIZED_REV      (1U << 3)
#define OSC_STS_CAPABILITY_MASKED     (1U << 4)
#define OSC_STS_MASK                  (OSC_STS_BIT0_RES          | \
                                       OSC_STS_FAILURE           | \
                                       OSC_STS_UNRECOGNIZED_UUID | \
                                       OSC_STS_UNRECOGNIZED_REV  | \
                                       OSC_STS_CAPABILITY_MASKED)

// ACPI OSC for Platform-Wide Capability
#define OSC_CAP_CPPC_SUPPORT          (1U << 5)
#define OSC_CAP_CPPC2_SUPPORT         (1U << 6)
#define OSC_CAP_PLAT_COORDINATED_LPI  (1U << 7)
#define OSC_CAP_OS_INITIATED_LPI      (1U << 8)

//
// "RHCT" RISC-V Hart Capabilities Table
//
#define EFI_ACPI_6_6_RISCV_HART_CAPABILITIES_TABLE_SIGNATURE  SIGNATURE_32('R', 'H', 'C', 'T')
//
// RHCT Revision (as defined in ACPI 6.5 spec.)
//
#define EFI_ACPI_6_6_RISCV_HART_CAPABILITIES_TABLE_REVISION  0x01

//
// RISC-V Hart Capabilities Table header definition.  The rest of the table
// must be defined in a platform specific manner.
//
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER    Header;
  UINT32                         Flags;
  UINT64                         TimeBaseFreq;
  UINT32                         NumNodes;
  UINT32                         OffsetNodes;
} EFI_ACPI_6_6_RISCV_HART_CAPABILITIES_TABLE_HEADER;

// ISA string node structure
typedef struct {
  UINT16     Type;
  UINT16     Length;
  UINT16     Revision;
  UINT16     ISALen;
  CHAR8      ISAStr[12];
} EFI_ACPI_6_6_ISA_STRING_NODE_STRUCTURE;

// CMO node structure
typedef struct {
  UINT16     Type;
  UINT16     Length;
  UINT16     Revision;
  UINT8      Reserved;
  UINT8      CBOMBlkSize;
  UINT8      CBOPBlkSize;
  UINT8      CBOZBlkSize;
} EFI_ACPI_6_6_CMO_NODE_STRUCTURE;

// MMU node structure
typedef struct {
  UINT16     Type;
  UINT16     Length;
  UINT16     Revision;
  UINT8      Reserved;
  UINT8      MMUType;
} EFI_ACPI_6_6_MMU_NODE_STRUCTURE;

// Hart Info Node Structure
typedef struct {
  UINT16     Type;
  UINT16     Length;
  UINT16     Revision;
  UINT16     NumOffset;
  UINT32     AcpiProcessorUid;
  UINT32     Offsets[3];
} EFI_ACPI_6_6_HART_INFO_NODE_STRUCTURE;

// RHCT Node[N] starts at offset 56
#define RHCT_NODE_ARRAY_OFFSET     56
#define RHCT_ISA_STRING_NODE_SIZE  sizeof(EFI_ACPI_6_6_ISA_STRING_NODE_STRUCTURE)
#define RHCT_CMO_NODE_SIZE         sizeof(EFI_ACPI_6_6_CMO_NODE_STRUCTURE)

// EFI_ACPI_6_6_HART_INFO_NODE_INIT
#define EFI_ACPI_6_6_HART_INFO_NODE_INIT(AcpiProcessorUid) {                   \
    65535,                                                                     \
    sizeof (EFI_ACPI_6_6_HART_INFO_NODE_STRUCTURE),                            \
    1,                                                                         \
    3,                                                                         \
    AcpiProcessorUid,                                                          \
    {                                                                          \
      RHCT_NODE_ARRAY_OFFSET,                                                  \
      RHCT_NODE_ARRAY_OFFSET + RHCT_ISA_STRING_NODE_SIZE,                      \
      RHCT_NODE_ARRAY_OFFSET + RHCT_ISA_STRING_NODE_SIZE + RHCT_CMO_NODE_SIZE  \
    }                                                                          \
  }

// Cache type identifier used to calculate unique cache ID for PPTT
typedef enum {
  L1DataCache = 1,
  L1InstructionCache,
  L2Cache,
} TH_PPTT_CACHE_TYPE;

#pragma pack(1)
// PPTT processor core structure
typedef struct {
  EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR  Core;
  UINT32                                 ResourceOffset[2];
  EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE      DCache;
  EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE      ICache;
} TH_PPTT_CORE;

// PPTT processor cluster structure
typedef struct {
  EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR  Cluster;
  UINT32                                 ResourceOffset;
  EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE      L2Cache;
  TH_PPTT_CORE                           Core[CORE_COUNT];
} TH_PPTT_CLUSTER;
#pragma pack ()

//
// PPTT processor structure flags for different SoC components as defined in
// ACPI 6.5 specification
//

// Processor structure flags for SoC package
#define PPTT_PROCESSOR_PACKAGE_FLAGS                                           \
  {                                                                            \
    EFI_ACPI_6_5_PPTT_PACKAGE_PHYSICAL,                                        \
    EFI_ACPI_6_5_PPTT_PROCESSOR_ID_INVALID,                                    \
    EFI_ACPI_6_5_PPTT_PROCESSOR_IS_NOT_THREAD,                                 \
    EFI_ACPI_6_5_PPTT_NODE_IS_NOT_LEAF,                                        \
    EFI_ACPI_6_5_PPTT_IMPLEMENTATION_IDENTICAL                                 \
  }

// Processor structure flags for cluster
#define PPTT_PROCESSOR_CLUSTER_FLAGS                                           \
  {                                                                            \
    EFI_ACPI_6_5_PPTT_PACKAGE_NOT_PHYSICAL,                                    \
    EFI_ACPI_6_5_PPTT_PROCESSOR_ID_VALID,                                      \
    EFI_ACPI_6_5_PPTT_PROCESSOR_IS_NOT_THREAD,                                 \
    EFI_ACPI_6_5_PPTT_NODE_IS_NOT_LEAF,                                        \
    EFI_ACPI_6_5_PPTT_IMPLEMENTATION_IDENTICAL                                 \
  }

// Processor structure flags for cluster with multi-thread core
#define PPTT_PROCESSOR_CLUSTER_THREADED_FLAGS                                  \
  {                                                                            \
    EFI_ACPI_6_5_PPTT_PACKAGE_NOT_PHYSICAL,                                    \
    EFI_ACPI_6_5_PPTT_PROCESSOR_ID_INVALID,                                    \
    EFI_ACPI_6_5_PPTT_PROCESSOR_IS_NOT_THREAD,                                 \
    EFI_ACPI_6_5_PPTT_NODE_IS_NOT_LEAF,                                        \
    EFI_ACPI_6_5_PPTT_IMPLEMENTATION_IDENTICAL                                 \
  }

// Processor structure flags for single-thread core
#define PPTT_PROCESSOR_CORE_FLAGS                                              \
  {                                                                            \
    EFI_ACPI_6_5_PPTT_PACKAGE_NOT_PHYSICAL,                                    \
    EFI_ACPI_6_5_PPTT_PROCESSOR_ID_VALID,                                      \
    EFI_ACPI_6_5_PPTT_PROCESSOR_IS_NOT_THREAD,                                 \
    EFI_ACPI_6_5_PPTT_NODE_IS_LEAF                                             \
  }

// Processor structure flags for multi-thread core
#define PPTT_PROCESSOR_CORE_THREADED_FLAGS                                     \
  {                                                                            \
    EFI_ACPI_6_5_PPTT_PACKAGE_NOT_PHYSICAL,                                    \
    EFI_ACPI_6_5_PPTT_PROCESSOR_ID_INVALID,                                    \
    EFI_ACPI_6_5_PPTT_PROCESSOR_IS_NOT_THREAD,                                 \
    EFI_ACPI_6_5_PPTT_NODE_IS_NOT_LEAF,                                        \
    EFI_ACPI_6_5_PPTT_IMPLEMENTATION_IDENTICAL                                 \
  }

// Processor structure flags for CPU thread
#define PPTT_PROCESSOR_THREAD_FLAGS                                            \
  {                                                                            \
    EFI_ACPI_6_5_PPTT_PACKAGE_NOT_PHYSICAL,                                    \
    EFI_ACPI_6_5_PPTT_PROCESSOR_ID_VALID,                                      \
    EFI_ACPI_6_5_PPTT_PROCESSOR_IS_THREAD,                                     \
    EFI_ACPI_6_5_PPTT_NODE_IS_LEAF                                             \
  }

// PPTT cache structure flags as defined in ACPI 6.5 Specification
#define PPTT_CACHE_STRUCTURE_FLAGS                                             \
  {                                                                            \
    EFI_ACPI_6_5_PPTT_CACHE_SIZE_VALID,                                        \
    EFI_ACPI_6_5_PPTT_NUMBER_OF_SETS_VALID,                                    \
    EFI_ACPI_6_5_PPTT_ASSOCIATIVITY_VALID,                                     \
    EFI_ACPI_6_5_PPTT_ALLOCATION_TYPE_VALID,                                   \
    EFI_ACPI_6_5_PPTT_CACHE_TYPE_VALID,                                        \
    EFI_ACPI_6_5_PPTT_WRITE_POLICY_VALID,                                      \
    EFI_ACPI_6_5_PPTT_LINE_SIZE_VALID,                                         \
    EFI_ACPI_6_5_PPTT_CACHE_ID_VALID                                           \
  }

// PPTT cache attributes for data cache
#define PPTT_DATA_CACHE_ATTR                                                   \
  {                                                                            \
    EFI_ACPI_6_5_CACHE_ATTRIBUTES_ALLOCATION_READ_WRITE,                       \
    EFI_ACPI_6_5_CACHE_ATTRIBUTES_CACHE_TYPE_DATA,                             \
    EFI_ACPI_6_5_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK                      \
  }

// PPTT cache attributes for instruction cache
#define PPTT_INST_CACHE_ATTR                                                   \
  {                                                                            \
    EFI_ACPI_6_5_CACHE_ATTRIBUTES_ALLOCATION_READ,                             \
    EFI_ACPI_6_5_CACHE_ATTRIBUTES_CACHE_TYPE_INSTRUCTION,                      \
    EFI_ACPI_6_5_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK                      \
  }

// PPTT cache attributes for unified cache
#define PPTT_UNIFIED_CACHE_ATTR                                                \
  {                                                                            \
    EFI_ACPI_6_5_CACHE_ATTRIBUTES_ALLOCATION_READ_WRITE,                       \
    EFI_ACPI_6_5_CACHE_ATTRIBUTES_CACHE_TYPE_UNIFIED,                          \
    EFI_ACPI_6_5_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK                      \
  }

/** Helper macro to calculate a unique cache ID

  Macro to calculate a unique 32 bit cache ID. The 32-bit encoding format of the
  cache ID is
  * Bits[31:24]: Unused
  * Bits[23:20]: Package number the cache belongs to
  * Bits[19:12]: Cluster number the cache belongs to (0, if not a cluster cache)
  * Bits[11:4] : Core number the cache belongs to (0, if not a CPU cache)
  * Bits[3:0]  : Cache Type (as defined by TH_PPTT_CACHE_TYPE)

  Note: Cache ID zero is invalid as per ACPI 6.4=5 specification. Also this
  calculation is not based on any specification.

  @param [in] PackageId Package instance number.
  @param [in] ClusterId Cluster instance number (for Cluster cache, 0 otherwise)
  @param [in] CpuId     CPU instance number (for CPU cache, 0 otherwise).
  @param [in] CacheType Identifier for cache type as defined by
                        TH_PPTT_CACHE_TYPE.
**/
#define TH_PPTT_CACHE_ID(PackageId, ClusterId, CoreId, CacheType)              \
    (                                                                          \
      (((PackageId) & 0xF) << 20) | (((ClusterId) & 0xFF) << 12) |             \
      (((CoreId) & 0xFF) << 4) | ((CacheType) & 0xF)                           \
    )

#define ACPI_BUILD_INTC_ID(socket, index) ((socket << 24) | (index))

// EFI_ACPI_6_6_RINTC_STRUCTURE
#define EFI_ACPI_6_6_RINTC_STRUCTURE_INIT(Flags, HartId, AcpiCpuUid,               \
  ExternalInterruptId, IMSICBase, IMSICSize) {                                     \
    0x18,                                   /* Type */                             \
    sizeof (EFI_ACPI_6_6_RINTC_STRUCTURE),  /* Length */                           \
    1,                                      /* Version */                          \
    EFI_ACPI_RESERVED_BYTE,                 /* Reserved */                         \
    Flags,                                  /* Flags */                            \
    HartId,                                 /* Hart ID */                          \
    AcpiCpuUid,                             /* AcpiProcessorUid */                 \
    ExternalInterruptId,                    /* External Interrupt Controller ID */ \
    IMSICBase,                              /* IMSIC Base address */               \
    IMSICSize,                              /* IMSIC Size */                       \
  }

// EFI_ACPI_6_6_PLIC_STRUCTURE
#define EFI_ACPI_6_6_PLIC_STRUCTURE_INIT(PlicId, HwId, TotalExtIntSrcsSup,     \
  MaxPriority, PLICSize, PLICBase, SystemVectorBase) {                         \
    0x1B,                                 /* Type */                           \
    sizeof (EFI_ACPI_6_6_PLIC_STRUCTURE),                                      \
    1,                                    /* Version */                        \
    PlicId,                               /* PlicId */                         \
    {0, 0, 0, 0, 0, 0, 0, HwId},          /* Hardware ID */                    \
    TotalExtIntSrcsSup,                   /* Total External Interrupt Sources Supported */   \
    MaxPriority,                          /* Maximum interrupt priority */     \
    0,                                    /* Flags */                          \
    PLICSize,                             /* PLIC Size */                      \
    PLICBase,                             /* PLIC Address */                   \
    SystemVectorBase                      /* Global System Interrupt Vector Base */         \
  }

#pragma pack(1)
//
// Define the number of each table type.
// This is where the table layout is modified.
//
#define EFI_ACPI_MEMORY_AFFINITY_STRUCTURE_COUNT 5
#define EFI_ACPI_PROCESSOR_LOCAL_APIC_SAPIC_AFFINITY_STRUCTURE_COUNT 64

typedef struct {
  EFI_ACPI_6_5_SYSTEM_RESOURCE_AFFINITY_TABLE_HEADER          Header;
  EFI_ACPI_6_5_MEMORY_AFFINITY_STRUCTURE                      Memory[EFI_ACPI_MEMORY_AFFINITY_STRUCTURE_COUNT];
  EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC_SAPIC_AFFINITY_STRUCTURE  APIC[EFI_ACPI_PROCESSOR_LOCAL_APIC_SAPIC_AFFINITY_STRUCTURE_COUNT];
} EFI_ACPI_STATIC_RESOURCE_AFFINITY_TABLE;
#pragma pack ()

// EFI_ACPI_6_5_MEMORY_AFFINITY_STRUCTURE
#define EFI_ACPI_6_5_MEMORY_AFFINITY_STRUCTURE_INIT(                                    \
    ProximityDomain, AddressBaseLow, AddressBaseHigh, LengthLow, LengthHigh, Flags)     \
  {                                                                                     \
    EFI_ACPI_6_5_MEMORY_AFFINITY, sizeof (EFI_ACPI_6_5_MEMORY_AFFINITY_STRUCTURE),      \
    ProximityDomain, EFI_ACPI_RESERVED_WORD, AddressBaseLow, AddressBaseHigh,           \
    LengthLow, LengthHigh, EFI_ACPI_RESERVED_DWORD, Flags, EFI_ACPI_RESERVED_QWORD      \
  }

// EFI_ACPI_6_5_APIC_SAPIC_AFFINITY_STRUCTURE
#define EFI_ACPI_6_5_APIC_SAPIC_AFFINITY_STRUCTURE_INIT(                                \
    ProximityDomainL, APICID, Flags, LocalSAPICEID, ClockDomain)                        \
  {                                                                                     \
    EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC_SAPIC_AFFINITY,                                   \
    sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC_SAPIC_AFFINITY_STRUCTURE),                \
    ProximityDomainL, APICID, Flags, LocalSAPICEID, {0x00, 0x00, 0x00}, ClockDomain     \
  }

// EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR
#define EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR_INIT(Length, Flag, Parent,       \
  ACPIProcessorID, NumberOfPrivateResource)                                    \
  {                                                                            \
    EFI_ACPI_6_5_PPTT_TYPE_PROCESSOR,                 /* Type 0 */             \
    Length,                                           /* Length */             \
    {                                                                          \
      EFI_ACPI_RESERVED_BYTE,                                                  \
      EFI_ACPI_RESERVED_BYTE,                                                  \
    },                                                                         \
    Flag,                                             /* Processor flags */    \
    Parent,                                           /* Ref to parent node */ \
    ACPIProcessorID,                                  /* UID, as per MADT */   \
    NumberOfPrivateResource                           /* Resource count */     \
  }

// EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE
#define EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE_INIT(Flag, NextLevelCache, Size,     \
  NoOfSets, Associativity, Attributes, LineSize, CacheId)                      \
  {                                                                            \
    EFI_ACPI_6_5_PPTT_TYPE_CACHE,                     /* Type 1 */             \
    sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE),       /* Length */             \
    {                                                                          \
      EFI_ACPI_RESERVED_BYTE,                                                  \
      EFI_ACPI_RESERVED_BYTE,                                                  \
    },                                                                         \
    Flag,                                             /* Cache flags */        \
    NextLevelCache,                                   /* Ref to next level */  \
    Size,                                             /* Size in bytes */      \
    NoOfSets,                                         /* Num of sets */        \
    Associativity,                                    /* Num of ways */        \
    Attributes,                                       /* Cache attributes */   \
    LineSize,                                         /* Line size in bytes */ \
    CacheId                                           /* Cache id */           \
  }

/** Helper macro for CPPC _CPC object initialization. Use of this macro is
    restricted to ASL file and not to TDL file.

    @param [in] DesiredPerfReg      Fastchannel address for desired performance
                                    register.
    @param [in] PerfLimitedReg      Fastchannel address for performance limited
                                    register.
    @param [in] GranularityMHz      Granularity of the performance scale.
    @param [in] HighestPerf         Highest performance in linear scale.
    @param [in] NominalPerf         Nominal performance in linear scale.
    @param [in] LowestNonlinearPerf Lowest non-linear performnce in linear
                                    scale.
    @param [in] LowestPerf          Lowest performance in linear scale.
    @param [in] RefPerf             Reference performance in linear scale.
**/
#define CPPC_PACKAGE_INIT(DesiredPerfReg, PerfLimitedReg, GranularityMHz,      \
  HighestPerf, NominalPerf, LowestNonlinearPerf, LowestPerf, RefPerf)          \
  {                                                                            \
    23,                                 /* NumEntries */                       \
    3,                                  /* Revision */                         \
    HighestPerf,                        /* Highest Performance */              \
    NominalPerf,                        /* Nominal Performance */              \
    LowestNonlinearPerf,                /* Lowest Nonlinear Performance */     \
    LowestPerf,                         /* Lowest Performance */               \
    /* Guaranteed Performance Register */                                      \
    ResourceTemplate () { Register (SystemMemory, 0, 0, 0, 0) },               \
    /* Desired Performance Register */                                         \
    ResourceTemplate () { Register (FFixedHW, 64, 0, DesiredPerfReg, 4) },     \
    /* Minimum Performance Register */                                         \
    ResourceTemplate () { Register (SystemMemory, 0, 0, 0, 0) },               \
    /* Maximum Performance Register */                                         \
    ResourceTemplate () { Register (SystemMemory, 0, 0, 0, 0) },               \
    /* Performance Reduction Tolerance Register */                             \
    ResourceTemplate () { Register (SystemMemory, 0, 0, 0, 0) },               \
    /* Time Window Register */                                                 \
    ResourceTemplate () { Register (FFixedHW, 64, 0, 0x1000000000000009, 4) }, \
    /* Counter Wraparound Time */                                              \
    ResourceTemplate () { Register (SystemMemory, 0, 0, 0, 0) },               \
    /* Reference Performance Counter Register */                               \
    ResourceTemplate () { Register (FFixedHW, 64, 0, 0x2000000000000C01, 4) }, \
    /* Delivered Performance Counter Register */                               \
    ResourceTemplate () { Register (FFixedHW, 64, 0, 0x100000000000000C, 4) }, \
    /* Performance Limited Register */                                         \
    ResourceTemplate () { Register (FFixedHW, 64, 0, PerfLimitedReg, 4) },     \
    /* CPPC Enable Register */                                                 \
    ResourceTemplate () { Register (SystemMemory, 0, 0, 0, 0) },               \
    /* Autonomous Selection Enable Register */                                 \
    ResourceTemplate () { Register (SystemMemory, 0, 0, 0, 0) },               \
    /* Autonomous Activity Window Register */                                  \
    ResourceTemplate () { Register (SystemMemory, 0, 0, 0, 0) },               \
    /* Energy Performance Preference Register */                               \
    ResourceTemplate () { Register (SystemMemory, 0, 0, 0, 0) },               \
    RefPerf,                            /* Reference Performance */            \
    (LowestPerf * GranularityMHz),      /* Lowest Frequency */                 \
    (NominalPerf * GranularityMHz),     /* Nominal Frequency */                \
  }

/** Helper macro for _LPI object initialization. Use of this macro is
    restricted to ASL file and not to TDL file.

    @param [in] MinResidency            Min Residency (us)
    @param [in] WorstWakeupLatency      Worst case wakeup latency (us)
    @param [in] Flags                   Flags
    @param [in] ContextLostFlags        Arch. Context Lost Flags
    @param [in] ResidencyCounterFreq    Residency Counter Frequency
    @param [in] EnabledParentState      Enabled Parent State
    @param [in] EntryMethodReg          Entry Method Register Address
    @param [in] StateName               State Name String
**/
#define LPI_PACKAGE_INIT(MinResidency, WorstWakeupLatency, Flags, ContextLostFlags,   \
  ResidencyCounterFreq, EnabledParentState, EntryMethodReg, StateName)                \
  {                                                                                   \
    MinResidency,                       /* Min Residency (us)*/                       \
    WorstWakeupLatency,                 /* Worst case wakeup latency (us)*/           \
    Flags,                              /* Flags */                                   \
    ContextLostFlags,                   /* Arch. Context Lost Flags */                \
    ResidencyCounterFreq,               /* Residency Counter Frequency */             \
    EnabledParentState,                 /* Enabled Parent State */                    \
    /* Entry Method */                                                                \
    ResourceTemplate () { Register (FFixedHW, 64, 0, EntryMethodReg, 4) },            \
    /* Residency Counter Register */                                                  \
    ResourceTemplate () { Register (SystemMemory, 0, 0, 0, 0) },                      \
    /* Usage Counter Register */                                                      \
    ResourceTemplate () { Register (SystemMemory, 0, 0, 0, 0) },                      \
    /* State Name */                                                                  \
    StateName                                                                         \
  }

// Power state dependancy (_PSD) for CPPC

/** Helper macro to initialize Power state dependancy (_PSD) object required
    for CPPC. Use of this macro is restricted to ASL file and not to TDL file.

    @param [in] Domain              The dependency domain number to which this
                                    P-state entry belongs.
**/
#define PSD_INIT(Domain)                                                       \
  {                                                                            \
    5,              /* Entries */                                              \
    0,              /* Revision */                                             \
    Domain,         /* Domain */                                               \
    0xFD,           /* Coord Type- SW_ANY */                                   \
    1               /* Processors */                                           \
  }

#endif /* __SG2042_ACPI_HEADER__ */
