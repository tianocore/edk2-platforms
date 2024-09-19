/** @file

  Copyright (c) 2020 - 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ACPI_APEI_H_
#define ACPI_APEI_H_

#include <AcpiConfigNVDataStruct.h>
#include <Base.h>
#include <Guid/AcpiConfigHii.h>
#include <IndustryStandard/Acpi63.h>
#include <Library/AcpiLib.h>
#include <Library/AmpereCpuLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Platform/Ac01.h>
#include <Protocol/AcpiTable.h>

#pragma pack(1)
#define BERT_MSG_SIZE                0x2C
#define BERT_UEFI_FAILURE            5
#define BERT_DEFAULT_ERROR_SEVERITY  0x1
#define GENERIC_ERROR_DATA_REVISION  0x300

#define RAS_TYPE_2P             0x03
#define RAS_TYPE_BERT           0x3F
#define RAS_TYPE_ERROR_MASK     0x3F
#define RAS_TYPE_PAYLOAD_MASK   0xC0
#define RAS_TYPE_PAYLOAD0       0x00
#define RAS_TYPE_PAYLOAD1       0x40
#define RAS_TYPE_PAYLOAD2       0x80
#define RAS_TYPE_PAYLOAD3       0xC0
#define RAS_TYPE_BERT_PAYLOAD3  (RAS_TYPE_BERT | RAS_TYPE_PAYLOAD3)

#define PLAT_CRASH_ITERATOR_SIZE  0x398
#define SMPRO_CRASH_SIZE          0x800
#define PMPRO_CRASH_SIZE          0x800
#define RASIP_CRASH_SIZE          0x1000
#define HEST_NUM_ENTRIES_PER_SOC  3

#define CURRENT_BERT_VERSION  0x12
#define BERT_FLASH_OFFSET     0x91B30000ULL
#define BERT_DDR_OFFSET       0x88230000ULL
#define BERT_DDR_LENGTH       0x50000

typedef struct {
  UINT8     Type;
  UINT8     SubType;
  UINT16    Instance;
  CHAR8     Msg[BERT_MSG_SIZE];
} APEI_BERT_ERROR_DATA;

typedef struct {
  APEI_BERT_ERROR_DATA    Vendor;
  UINT8                   BertRev;
  UINT8                   S0PmproRegisters[PMPRO_CRASH_SIZE];
  UINT8                   S0SmproRegisters[SMPRO_CRASH_SIZE];
  UINT8                   S0RasIpRegisters[RASIP_CRASH_SIZE];
  UINT8                   S1PmproRegisters[PMPRO_CRASH_SIZE];
  UINT8                   S1SmproRegisters[SMPRO_CRASH_SIZE];
  UINT8                   S1RasIpRegisters[RASIP_CRASH_SIZE];
  UINT8                   AtfDump[PLATFORM_CPU_MAX_NUM_CORES * PLAT_CRASH_ITERATOR_SIZE];
} APEI_CRASH_DUMP_DATA;

typedef struct {
  EFI_ACPI_6_3_GENERIC_ERROR_STATUS_STRUCTURE        Ges;
  EFI_ACPI_6_3_GENERIC_ERROR_DATA_ENTRY_STRUCTURE    Ged;
  APEI_CRASH_DUMP_DATA                               Bed;
} APEI_CRASH_DUMP_BERT_ERROR;

/*
 * The BERT regions is 0x50000 in size. The SPI-NOR sector size is 0x1000 (4KB).
 * So there is 80 sectors available for BERT errors.
 *
 * BERT error record must be persistent across boots on catastrophic errors
 * until the BMC and UEFI have had a chance to retrieve the information. To
 * support this, the BERT is sectioned off into two different sections.
 *
 *  -----------------------------------
 *  | BERT Error Record               | sectors 0-61 (4KB * 62 = 248KB)
 *  |                                 |
 *  -----------------------------------
 *  | Unused/Reserved                 | sectors 62-78 (4KB * 17 = 68KB)
 *  |                                 |
 *  -----------------------------------
 *  | BERT Error Record Status        | sector 79 (4KB x 1 = 4KB)
 *  |                                 | offset 79 * 0x1000 = 0x4F000
 *  -----------------------------------
 *
 *   BERT Error Record:
 *     - Catastrophic Full BERT Error Record (persistent until read by BMC and UEFI)
 *   BERT Error Record Status:
 *     - DefaultBert: Indicate Default Bert Present
 *       - Set by UEFI on boot and cleared by SCP on shutdown, reboot, or crash
 *       - 0 = Default BERT not valid
 *       - 1 = Default BERT valid
 *     - Overflow: Indicate Full Crash Capture overflow before
 *       - Set by SCP if PendingUefi or PendingBMC was set and to be cleared by UEFI
 *         if PendingUefi and PendingBMC are cleared.
 *       - 0 = No overflow
 *       - 1 = Record has overflowed
 *     - PendingUefi: Indicate Pending UEFI detection
 *       - Set by SCP and to be cleared by UEFI
 *       - 0 = Pending UEFI detection not valid
 *       - 1 = Pending UEFI detection valid
 *     - PendingBmc:  Indicate Pending BMC detection
 *       - Set by SCP and to be cleared by BMC
 *       - 0 = Pending BMC detection not valid
 *       - 1 = Pending BMC detection valid
 */

typedef struct {
  UINT8    Guid[16];           /* [0x00] set to AMPERE_GUID */
  UINT8    BertRev;            /* [0x10] set to BERT revision */
  UINT8    Reserved0[7];       /* [----] Reserved */
  UINT8    DefaultBert;        /* [0x18] Indicate Default Bert Present */
  UINT8    Overflow;           /* [----] Indicate Full Crash Capture overflow detected */
  UINT8    PendingUefi;        /* [----] Indicate Pending UEFI detection */
  UINT8    PendingBmc;         /* [----] Indicate Pending BMC detection */
  UINT8    Reserved1[4];       /* [----] Reserved */
} BERT_ERROR_STATUS;

#define BERT_SPI_ADDRESS_STATUS  (BERT_FLASH_OFFSET + 0x4F000)

#pragma pack()

VOID
EFIAPI
CreateDefaultBertData (
  APEI_CRASH_DUMP_DATA  *Data
  );

VOID
EFIAPI
WrapBertErrorData (
  APEI_CRASH_DUMP_BERT_ERROR  *WrappedError
  );

VOID
EFIAPI
PullBertSpinorData (
  UINT8   *BertData,
  UINT64  BertSpiAddress,
  UINTN   Length
  );

VOID
EFIAPI
AdjustBERTRegionLen (
  UINT32  Len
  );

BOOLEAN
EFIAPI
IsBertEnabled (
  VOID
  );

VOID
EFIAPI
WriteDDRBertTable (
  APEI_CRASH_DUMP_BERT_ERROR  *Data
  );

VOID
WriteSpinorDefaultBertTable (
  APEI_CRASH_DUMP_DATA  *SpiRefrenceData
  );

EFI_STATUS
EFIAPI
AcpiApeiUpdate (
  VOID
  );

EFI_STATUS
EFIAPI
AcpiPopulateBert (
  VOID
  );

#endif /* ACPI_APEI_H_ */
