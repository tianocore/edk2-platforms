/** @file
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * Copyright (c) 2024, Bosc. All rights reserved.<BR>
 *
 */

#ifndef RISCV_BIT_OP_H
#define RISCV_BIT_OP_H

#define BIT(nr)         (1UL << (nr))
#define GENMASK(h, l) \
    (((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

#endif
