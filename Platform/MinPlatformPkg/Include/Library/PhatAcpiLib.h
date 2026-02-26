/** @file
  Platform Health Assessment Table Library Definitions

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _PHAT_ACPI_LIB_H_
#define _PHAT_ACPI_LIB_H_

/**
  Convert AIP data block to PHAT ACPI style, and publish it onto
  an existing ACPI PHAT structure or initialize and install a new
  instance.

  @param[in]   InfoBlock          Point to AIP data block.
  @param[in]   InfoBlockSize      The size of AIP data.

  @retval EFI_SUCCESS             Success
  @retval EFI_OUT_OF_RESOURCES    Out of memory space.
  @retval EFI_INVALID_PARAMETER   Either InfoBlock is NULL,
                                  TableKey is NULL, or
                                  AcpiTableBufferSize and the size
                                  field embedded in the ACPI table
                                  pointed to by AcpiTableBuffer
                                  are not in sync.
  @retval EFI_ACCESS_DENIED       The table signature matches a table already
                                  present in the system and platform policy
                                  does not allow duplicate tables of this type.
  @retval EFI_NOT_FOUND           AcpiEntry is NULL.
**/
EFI_STATUS
EFIAPI
InstallPhatTable (
  IN  VOID        *InfoBlock,
  IN  UINTN       InfoBlockSize
  );

#endif  // _PHAT_ACPI_LIB_H_