/** @file

  Copyright (c) 2020 - 2023, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef GET_ROOT_COMPLEX_INFO_
#define GET_ROOT_COMPLEX_INFO_

#define BITS_PER_BYTE       8
#define BYTE_MASK           0xFF
#define PCIE_ERRATA_SPEED1  0x0001     // Limited speed errata

#define AUTO_BIFURCATION_SETTING_VALUE  0x0A

#ifndef BIT
#define BIT(nr)  (1 << (nr))
#endif

#define PCIE_GET_MAX_WIDTH(Pcie, Max) \
  !((Pcie).MaxWidth) ? (Max) : MIN ((Pcie).MaxWidth, (Max))

VOID
ParseRootComplexNVParamData (
  AC01_ROOT_COMPLEX  *RootComplex
  );

#endif /* GET_ROOT_COMPLEX_INFO_ */
