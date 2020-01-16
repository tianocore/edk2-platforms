/** @file

  Copyright (c) 2020, Pete Batard <pete@akeo.ie>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef BCM_GENET_H__
#define BCM_GENET_H__

#include <Library/PcdLib.h>

#if (FixedPcdGetBool (PcdBcmGenetRpiMacInit))
#define GENET_MAC_USE_RPI_INIT
#endif

#define GENET_BASE_ADDRESS         (FixedPcdGet64 (PcdBcmGenetRegistersAddress))

#define GENET_SYS_RBUF_FLUSH_CTRL  0x0008
#define GENET_UMAC_MAC0            0x080c
#define GENET_UMAC_MAC1            0x0810

#endif /* BCM_GENET_H__ */
