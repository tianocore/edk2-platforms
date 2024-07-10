/** @file
*
*  Copyright (c) 2020, Linaro Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef SBSAQEMUACPI_H
#define SBSAQEMUACPI_H

// A macro to initialise the common header part of EFI ACPI tables as defined by
// EFI_ACPI_DESCRIPTION_HEADER structure.
#define SBSAQEMU_ACPI_HEADER(Signature, Type, Revision)  {                     \
    Signature,                                    /* UINT32  Signature */       \
    sizeof (Type),                                /* UINT32  Length */          \
    Revision,                                     /* UINT8   Revision */        \
    0,                                            /* UINT8   Checksum */        \
    { 'L', 'I', 'N', 'A', 'R', 'O' },             /* UINT8   OemId[6] */        \
    FixedPcdGet64 (PcdAcpiDefaultOemTableId),     /* UINT64  OemTableId */      \
    FixedPcdGet32 (PcdAcpiDefaultOemRevision),    /* UINT32  OemRevision */     \
    FixedPcdGet32 (PcdAcpiDefaultCreatorId),      /* UINT32  CreatorId */       \
    FixedPcdGet32 (PcdAcpiDefaultCreatorRevision) /* UINT32  CreatorRevision */ \
  }

// Defines for MADT
#define SBSAQEMU_MADT_GIC_VBASE    0x2c020000
#define SBSAQEMU_MADT_GIC_HBASE    0x2c010000
#define SBSAQEMU_MADT_GIC_PMU_IRQ  23
#define SBSAQEMU_MADT_GICR_SIZE    0x4000000
#define SBSAQEMU_MADT_GITS_SIZE    0x20000

// Macro for MADT GIC Redistributor Structure
#define SBSAQEMU_MADT_GICR_INIT()  {                                           \
   EFI_ACPI_6_0_GICR,                        /* Type */                        \
   sizeof (EFI_ACPI_6_0_GICR_STRUCTURE),     /* Length */                      \
   EFI_ACPI_RESERVED_WORD,                   /* Reserved */                    \
   PcdGet64 (PcdGicRedistributorsBase),      /* DiscoveryRangeBaseAddress */   \
   SBSAQEMU_MADT_GICR_SIZE                   /* DiscoveryRangeLength */        \
   }

// Macro for MADT GIC ITS Structure
#define SBSAQEMU_MADT_GIC_ITS_INIT(GicItsId)  {                                \
   EFI_ACPI_6_5_GIC_ITS,                     /* Type */                        \
   sizeof (EFI_ACPI_6_5_GIC_ITS_STRUCTURE),  /* Length */                      \
   EFI_ACPI_RESERVED_WORD,                   /* Reserved */                    \
   GicItsId,                                 /* GicItsId */                    \
   PcdGet64 (PcdGicItsBase),                 /* PhysicalBaseAddress */         \
   EFI_ACPI_RESERVED_DWORD                   /* Reserved2 */                   \
   }

#define SBSAQEMU_ACPI_SCOPE_OP_MAX_LENGTH  5

#define SBSAQEMU_ACPI_SCOPE_NAME  { '_', 'S', 'B', '_' }

#define SBSAQEMU_ACPI_CPU_DEV_LEN   0x1C
#define SBSAQEMU_ACPI_CPU_DEV_NAME  { 'C', '0', '0', '0' }

#define SBSAQEMU_ACPI_CPU_HID  {                                               \
  AML_NAME_OP, AML_NAME_CHAR__, 'H', 'I', 'D',                                 \
  AML_STRING_PREFIX, 'A', 'C', 'P', 'I', '0', '0', '0', '7',                   \
  AML_ZERO_OP                                                                  \
  }

#define SBSAQEMU_ACPI_CPU_UID  {                                               \
   AML_NAME_OP, AML_NAME_CHAR__, 'U', 'I', 'D', AML_WORD_PREFIX,               \
   AML_ZERO_OP, AML_ZERO_OP                                                    \
   }

typedef struct {
  UINT8    device_header[2];
  UINT8    length;
  UINT8    dev_name[4];
  UINT8    hid[15];
  UINT8    uid[8];
} SBSAQEMU_ACPI_CPU_DEVICE;

#define CLUSTER_INDEX     (sizeof (EFI_ACPI_DESCRIPTION_HEADER))
#define L1_D_CACHE_INDEX  (CLUSTER_INDEX + sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR))
#define L1_I_CACHE_INDEX  (L1_D_CACHE_INDEX + sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE))
#define L2_CACHE_INDEX    (L1_I_CACHE_INDEX + sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE))

#define SBSAQEMU_ACPI_PPTT_L1_D_CACHE_STRUCT  {                                \
    EFI_ACPI_6_5_PPTT_TYPE_CACHE,                                              \
    sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE),                                \
    { EFI_ACPI_RESERVED_BYTE, EFI_ACPI_RESERVED_BYTE },                        \
    {                                                                          \
      1,                       /* SizePropertyValid */                         \
      1,                       /* NumberOfSetsValid */                         \
      1,                       /* AssociativityValid */                        \
      1,                       /* AllocationTypeValid */                       \
      1,                       /* CacheTypeValid */                            \
      1,                       /* WritePolicyValid */                          \
      1,                       /* LineSizeValid */                             \
      1,                       /* CacheIdValid */                              \
    },                                                                         \
    0,                         /* NextLevelOfCache */                          \
    0,                         /* Size */                                      \
    0,                         /* NumberOfSets */                              \
    0,                         /* Associativity */                             \
    {                                                                          \
      EFI_ACPI_6_5_CACHE_ATTRIBUTES_ALLOCATION_READ_WRITE,                     \
      EFI_ACPI_6_5_CACHE_ATTRIBUTES_CACHE_TYPE_DATA,                           \
      EFI_ACPI_6_5_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK,                   \
    },                                                                         \
    64,                        /* LineSize */                                  \
    0                          /* CacheId */                                   \
  }

#define SBSAQEMU_ACPI_PPTT_L1_I_CACHE_STRUCT  {                                \
    EFI_ACPI_6_5_PPTT_TYPE_CACHE,                                              \
    sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE),                                \
    { EFI_ACPI_RESERVED_BYTE, EFI_ACPI_RESERVED_BYTE },                        \
    {                                                                          \
      1,                       /* SizePropertyValid */                         \
      1,                       /* NumberOfSetsValid */                         \
      1,                       /* AssociativityValid */                        \
      1,                       /* AllocationTypeValid */                       \
      1,                       /* CacheTypeValid */                            \
      1,                       /* WritePolicyValid */                          \
      1,                       /* LineSizeValid */                             \
      1,                       /* CacheIdValid */                              \
    },                                                                         \
    0,                         /* NextLevelOfCache */                          \
    0,                         /* Size */                                      \
    0,                         /* NumberOfSets */                              \
    0,                         /* Associativity */                             \
    {                                                                          \
      EFI_ACPI_6_5_CACHE_ATTRIBUTES_ALLOCATION_READ,                           \
      EFI_ACPI_6_5_CACHE_ATTRIBUTES_CACHE_TYPE_INSTRUCTION,                    \
      0,                                                                       \
    },                                                                         \
    64,                        /* LineSize */                                  \
    0                          /* CacheId */                                   \
  }

#define SBSAQEMU_ACPI_PPTT_L2_CACHE_STRUCT  {                                  \
    EFI_ACPI_6_5_PPTT_TYPE_CACHE,                                              \
    sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_CACHE),                                \
    { EFI_ACPI_RESERVED_BYTE, EFI_ACPI_RESERVED_BYTE },                        \
    {                                                                          \
      1,                     /* SizePropertyValid */                           \
      1,                     /* NumberOfSetsValid */                           \
      1,                     /* AssociativityValid */                          \
      1,                     /* AllocationTypeValid */                         \
      1,                     /* CacheTypeValid */                              \
      1,                     /* WritePolicyValid */                            \
      1,                     /* LineSizeValid */                               \
      1,                     /* CacheIdValid */                                \
    },                                                                         \
    0,                         /* NextLevelOfCache */                          \
    0,                         /* Size */                                      \
    0,                         /* NumberOfSets */                              \
    0,                         /* Associativity */                             \
    {                                                                          \
      EFI_ACPI_6_5_CACHE_ATTRIBUTES_ALLOCATION_READ_WRITE,                     \
      EFI_ACPI_6_5_CACHE_ATTRIBUTES_CACHE_TYPE_UNIFIED,                        \
      EFI_ACPI_6_5_CACHE_ATTRIBUTES_WRITE_POLICY_WRITE_BACK,                   \
    },                                                                         \
    64,                      /* LineSize */                                    \
    0                        /* CacheId */                                     \
  }

#endif
