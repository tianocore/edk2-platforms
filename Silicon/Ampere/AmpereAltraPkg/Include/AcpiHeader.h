/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ACPI_HEADER_H_
#define ACPI_HEADER_H_

#include <IndustryStandard/Acpi.h>

//
// ACPI table information used to initialize tables.
//
#define EFI_ACPI_OEM_ID            {'A','m','p','e','r','e'}
#define EFI_ACPI_OEM_TABLE_ID      SIGNATURE_64('A','l','t','r','a',' ',' ',' ')
#define EFI_ACPI_OEM_REVISION      FixedPcdGet32 (PcdAcpiDefaultOemRevision)
#define EFI_ACPI_CREATOR_ID        SIGNATURE_32('A','M','P','.')
#define EFI_ACPI_CREATOR_REVISION  FixedPcdGet32 (PcdAcpiDefaultCreatorRevision)

// A macro to initialise the common header part of EFI ACPI tables as defined by
// EFI_ACPI_DESCRIPTION_HEADER structure.
#define __ACPI_HEADER(Signature, Type, Revision)  {               \
    Signature,                /* UINT32  Signature */       \
    sizeof (Type),            /* UINT32  Length */          \
    Revision,                 /* UINT8   Revision */        \
    0,                        /* UINT8   Checksum */        \
    EFI_ACPI_OEM_ID,          /* UINT8   OemId[6] */        \
    EFI_ACPI_OEM_TABLE_ID,    /* UINT64  OemTableId */      \
    EFI_ACPI_OEM_REVISION,    /* UINT32  OemRevision */     \
    EFI_ACPI_CREATOR_ID,      /* UINT32  CreatorId */       \
    EFI_ACPI_CREATOR_REVISION /* UINT32  CreatorRevision */ \
  }

#endif /* ACPI_HEADER_H_ */
