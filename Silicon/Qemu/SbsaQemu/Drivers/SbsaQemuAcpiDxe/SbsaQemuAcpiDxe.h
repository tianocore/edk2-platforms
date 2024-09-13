/** @file
*  This file is an ACPI driver for the Qemu SBSA platform.
*
*  Copyright (c) Linaro Ltd. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef SBSAQEMU_ACPI_DXE_H
#define SBSAQEMU_ACPI_DXE_H

typedef struct {
  EFI_ACPI_6_0_IO_REMAPPING_ITS_NODE    Node;
  UINT32                                Identifiers;
} SBSA_EFI_ACPI_6_0_IO_REMAPPING_ITS_NODE;

typedef struct {
  EFI_ACPI_6_0_IO_REMAPPING_SMMU3_NODE    SmmuNode;
  EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE      SmmuIdMap;
} SBSA_EFI_ACPI_6_0_IO_REMAPPING_SMMU3_NODE;

typedef struct {
  EFI_ACPI_6_0_IO_REMAPPING_RC_NODE     RcNode;
  EFI_ACPI_6_0_IO_REMAPPING_ID_TABLE    RcIdMap;
} SBSA_EFI_ACPI_6_0_IO_REMAPPING_RC_NODE;

typedef struct {
  EFI_ACPI_6_0_IO_REMAPPING_TABLE              Iort;
  SBSA_EFI_ACPI_6_0_IO_REMAPPING_ITS_NODE      ItsNode;
  SBSA_EFI_ACPI_6_0_IO_REMAPPING_SMMU3_NODE    SmmuNode;
  SBSA_EFI_ACPI_6_0_IO_REMAPPING_RC_NODE       RcNode;
} SBSA_IO_REMAPPING_STRUCTURE;

typedef struct {
  EFI_ACPI_6_3_GENERIC_TIMER_DESCRIPTION_TABLE         mGtdt;
  EFI_ACPI_6_3_GTDT_SBSA_GENERIC_WATCHDOG_STRUCTURE    mGwdt;
} GENERIC_TIMER_DESCRIPTION_TABLES;

#ifndef SYSTEM_TIMER_BASE_ADDRESS
#define SYSTEM_TIMER_BASE_ADDRESS  MAX_ADDRESS
#endif

#define GTDT_TIMER_LEVEL_TRIGGERED  0
#define GTDT_TIMER_ACTIVE_LOW       EFI_ACPI_6_3_GTDT_TIMER_FLAG_TIMER_INTERRUPT_POLARITY
#define GTDT_TIMER_ALWAYS_ON        EFI_ACPI_6_3_GTDT_TIMER_FLAG_ALWAYS_ON_CAPABILITY

#define GTDT_GTIMER_FLAGS  (GTDT_TIMER_ACTIVE_LOW |          \
                                     GTDT_TIMER_LEVEL_TRIGGERED | \
                                     GTDT_TIMER_ALWAYS_ON)

#define SBSA_PLATFORM_WATCHDOG_COUNT  1
#define SBSA_PLATFORM_TIMER_COUNT     (SBSA_PLATFORM_WATCHDOG_COUNT)

#define SBSAQEMU_WDT_REFRESH_FRAME_BASE  0x50010000
#define SBSAQEMU_WDT_CONTROL_FRAME_BASE  0x50011000
#define SBSAQEMU_WDT_IRQ                 48

#define GTDT_WDTIMER_LEVEL_TRIGGERED  0
#define GTDT_WDTIMER_ACTIVE_HIGH      0

#define GTDT_WDTIMER_FLAGS  (GTDT_WDTIMER_ACTIVE_HIGH | GTDT_WDTIMER_LEVEL_TRIGGERED)

#define SBSAQEMU_ACPI_MEMORY_AFFINITY_STRUCTURE_INIT(                             \
                                                                                  ProximityDomain, Base, Length, Flags)                                   \
  {                                                                               \
    1,                                                  /* Type */                \
    sizeof (EFI_ACPI_6_4_MEMORY_AFFINITY_STRUCTURE),    /* Length */              \
    ProximityDomain,                                    /* Proximity Domain */    \
    0,                                                  /* Reserved */            \
    (Base) & 0xffffffff,                                /* Base Address Low */    \
    ((Base) >> 32) & 0xffffffff ,                       /* Base Address High */   \
    (Length) & 0xffffffff,                              /* Length Low */          \
    ((Length) >> 32) & 0xffffffff,                      /* Length High */         \
    0,                                                  /* Reserved */            \
    Flags,                                              /* Flags */               \
    0                                                   /* Reserved */            \
  }

#define SBSAQEMU_ACPI_GICC_AFFINITY_STRUCTURE_INIT(                               \
                                                                                  ProximityDomain, ACPIProcessorUID, Flags, ClockDomain)                  \
  {                                                                               \
    3,                                                  /* Type */                \
    sizeof (EFI_ACPI_6_4_GICC_AFFINITY_STRUCTURE),      /* Length */              \
    ProximityDomain,                                    /* Proximity Domain */    \
    ACPIProcessorUID,                                   /* ACPI Processor UID */  \
    Flags,                                              /* Flags */               \
    ClockDomain                                         /* Clock Domain */        \
  }

#define SBSAQEMU_ACPI_PROCESSOR_HIERARCHY_NODE_STRUCTURE_INIT(Flags, Parent, ACPIProcessorID, NumberOfPrivateResources)             \
  {                                                                                                                                 \
    EFI_ACPI_6_5_PPTT_TYPE_PROCESSOR,                                                            /* Type */                         \
    sizeof (EFI_ACPI_6_5_PPTT_STRUCTURE_PROCESSOR) + NumberOfPrivateResources * sizeof (UINT32), /* Length */                       \
    { EFI_ACPI_RESERVED_BYTE, EFI_ACPI_RESERVED_BYTE },                                          /* Reserved */                     \
    Flags,                                                                                       /* Flags */                        \
    Parent,                                                                                      /* Parent */                       \
    ACPIProcessorID,                                                                             /* ACPI Processor ID */            \
    NumberOfPrivateResources                                                                     /* Number of private resources */  \
  }

#endif
