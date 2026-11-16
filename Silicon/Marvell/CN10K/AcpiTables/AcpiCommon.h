/** @file

  Header file which includes common definitions used to initialize ACPI tables.

  Copyright (c) 2024-2026 Marvell. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/Acpi.h>

#define EFI_ACPI_MRVL_OEM_ID            'M','R','V','L', 0, 0 // OEMID 6 bytes long
#define EFI_ACPI_MRVL_OEM_REVISION      0x00000001
#define EFI_ACPI_MRVL_CREATOR_ID        SIGNATURE_32('M','R','V','L')
#define EFI_ACPI_MRVL_CREATOR_REVISION  0x00000001

// A macro to initialise the common header part of EFI ACPI tables as defined by
// EFI_ACPI_DESCRIPTION_HEADER structure.
#define MARVELL_ACPI_HEADER(Signature, Type, Revision)  {              \
       Signature,                      /* UINT32  Signature */         \
       sizeof (Type),                  /* UINT32  Length */            \
       Revision,                       /* UINT8   Revision */          \
       0,                              /* UINT8   Checksum */          \
       { EFI_ACPI_MRVL_OEM_ID },       /* UINT8   OemId[6] */          \
       EFI_ACPI_MRVL_OEM_TABLE_ID,     /* UINT64  OemTableId */        \
       EFI_ACPI_MRVL_OEM_REVISION,     /* UINT32  OemRevision */       \
       EFI_ACPI_MRVL_CREATOR_ID,       /* UINT32  CreatorId */         \
       EFI_ACPI_MRVL_CREATOR_REVISION  /* UINT32  CreatorRevision */   \
     }
