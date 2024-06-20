/** @file
  This header provide all the general purpose input/output device tree bindings

  Copyright (c) 2023, StarFive Technology Co., Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DT_BINDINGS_GPIO_H__
#define __DT_BINDINGS_GPIO_H__

/* Bit 0 express polarity */
#define GPIO_ACTIVE_HIGH  0
#define GPIO_ACTIVE_LOW   1

/* Bit 1 express single-endedness */
#define GPIO_PUSH_PULL     0
#define GPIO_SINGLE_ENDED  2

/* Bit 2 express Open drain or open source */
#define GPIO_LINE_OPEN_SOURCE  0
#define GPIO_LINE_OPEN_DRAIN   4

/*
 * Open Drain/Collector is the combination of single-ended open drain interface.
 * Open Source/Emitter is the combination of single-ended open source interface.
 */
#define GPIO_OPEN_DRAIN   (GPIO_SINGLE_ENDED | GPIO_LINE_OPEN_DRAIN)
#define GPIO_OPEN_SOURCE  (GPIO_SINGLE_ENDED | GPIO_LINE_OPEN_SOURCE)

/* Bit 3 express GPIO suspend/resume and reset persistence */
#define GPIO_PERSISTENT  0
#define GPIO_TRANSITORY  8

/* Bit 4 express pull up */
#define GPIO_PULL_UP  16

/* Bit 5 express pull down */
#define GPIO_PULL_DOWN  32

#endif /* __DT_BINDINGS_GPIO_H__ */
