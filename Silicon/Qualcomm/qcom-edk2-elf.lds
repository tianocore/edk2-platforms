/** @file

  Qualcomm ELF payload linker script

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

ENTRY(ELFENTRY)
SECTIONS
{
	. = ELFENTRY;

	.data : {
		*(.data*)
	}
}
