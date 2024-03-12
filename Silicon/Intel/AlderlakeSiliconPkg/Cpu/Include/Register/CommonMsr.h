
/** @file
  CommonMsr.h

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _COMMONMSR_h
#define _COMMONMSR_h
#include <Base.h>

/**
  This is a Read Only MSR that is shared in the processor package and used to
  determine the current count of enabled Cores and Threads.
**/
#define MSR_CORE_THREAD_COUNT 0x00000035

typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32 Threadcount : 16;

                            /* Bits[15:0], Access Type=RO_V, default=None*/

                            /*
                               The Thread Count reflects the enabled threads
                               based on the factory-configured thread count and
                               the value of the RESOLVED_CORES_MASK register
                               for Server processors or the PCH Soft Reset Data
                               register for Client processors at reset time.
                            */
    UINT32 Corecount : 16;

                            /* Bits[31:16], Access Type=RO_V, default=None*/

                            /*
                               The Core Count reflects the enabled cores based
                               on the factory-configured core count and the
                               value of the RESOLVED_CORES_MASK register for
                               Server processors or the PCH Soft Reset Data
                               register for Client processors at reset time.
                            */
    UINT32 Rsvd32 : 32;

                            /* Bits[63:32], Access Type=RO, default=None*/

                            /* Reserved */

  } Bits;

  UINT32 Uint32;
  UINT64 Uint64;

} MSR_CORE_THREAD_COUNT_REGISTER;

/**
  Special Chipset Usage MSR
**/
#define MSR_SPCL_CHIPSET_USAGE 0x000001FE

#endif /* _COMMONMSR_h */
