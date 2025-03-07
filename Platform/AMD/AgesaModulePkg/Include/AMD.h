/** @file

  Copyright (C) 2008-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef AMD_H_
#define AMD_H_

/// The return status for all AGESA public services.
///
/// Services return the most severe status of any logged event.  Status other than SUCCESS, UNSUPPORTED, and BOUNDS_CHK
/// will have log entries with more detail.
///
typedef enum {
  AGESA_SUCCESS = 0,           ///< 0 - The service completed normally. Info may be logged.
  AGESA_UNSUPPORTED,           ///< 1 -  The dispatcher or create struct had an unimplemented function requested.
                               ///<      Not logged.
  AGESA_BOUNDS_CHK,            ///< 2 -  A dynamic parameter was out of range and the service was not provided.
                               ///<      Example, memory address not installed, heap buffer handle not found.
                               ///<      Not Logged.
  AGESA_SYNC_MORE_DATA,        ///< 3 -  More data is available from PSP communications (used in ABL)
  AGESA_SYNC_SLAVE_ASSERT,     ///< 4 -  Slave is at an ASSERT (used in ABL)

  // AGESA_STATUS of greater severity (the ones below this line), always have a log entry available.
  AGESA_ALERT,                 ///< 5 -  An observed condition, but no loss of function.  See Log.
  AGESA_WARNING,               ///< 6 -  Possible or minor loss of function.  See Log.
  AGESA_ERROR,                 ///< 7 -  Significant loss of function, boot may be possible.  See Log.
  AGESA_CRITICAL,              ///< 8 -  Continue boot only to notify user.  See Log.
  AGESA_FATAL,                 ///< 9 -  Halt booting.  See Log, however Fatal errors pertaining to heap problems
                               ///<      may not be able to reliably produce log events.
  AGESA_OC_FATAL,              ///< 10 - Halt booting.  Critical Memory Overclock failure. (used in ABL)
  AGESA_SKIP_ERROR,            ///< 11 - Error, Skip init steps. (used in ABL)
  AgesaStatusMax               ///< Not a status, for limit checking.
} AGESA_STATUS;

/**
  Callout method to the host environment.

  Callout using a dispatch with appropriate thunk layer, which is determined by the host environment.

  @param[in]        Function      The specific callout function being invoked.
  @param[in]        FcnData       Function specific data item.
  @param[in,out]    ConfigPtr     Reference to Callout params.
**/
typedef AGESA_STATUS (*CALLOUT_ENTRY) (
  IN       UINT32  Function,
  IN       UINTN   FcnData,
  IN OUT   VOID    *ConfigPtr
  );

/// AGESA Structures

/// Extended PCI address format
typedef struct {
  IN OUT  UINT32    Register : 12;                ///< Register offset
  IN OUT  UINT32    Function : 3;                 ///< Function number
  IN OUT  UINT32    Device   : 5;                 ///< Device number
  IN OUT  UINT32    Bus      : 8;                 ///< Bus number
  IN OUT  UINT32    Segment  : 4;                 ///< Segment
} EXT_PCI_ADDR;

/// Union type for PCI address
typedef union _PCI_ADDR {
  IN  UINT32          AddressValue;               ///< Formal address
  IN  EXT_PCI_ADDR    Address;                    ///< Extended address
} PCI_ADDR;

///
/// The standard header for all AGESA services.
///
typedef struct {
  IN       UINT32           ImageBasePtr;           ///< The AGESA Image base address.
  IN       UINT32           Func;                   ///< The service desired
  IN       UINT32           AltImageBasePtr;        ///< Alternate Image location
  IN       CALLOUT_ENTRY    CalloutPtr;             ///< For Callout from AGESA
  IN       UINT8            HeapStatus;             ///< For heap status from boot time slide.
  IN       UINT64           HeapBasePtr;            ///< Location of the heap
  IN OUT   UINT8            Reserved[7];            ///< This space is reserved for future use.
} AMD_CONFIG_PARAMS;

/// CPUID data received registers format
typedef struct {
  OUT UINT32    EAX_Reg;                          ///< CPUID instruction result in EAX
  OUT UINT32    EBX_Reg;                          ///< CPUID instruction result in EBX
  OUT UINT32    ECX_Reg;                          ///< CPUID instruction result in ECX
  OUT UINT32    EDX_Reg;                          ///< CPUID instruction result in EDX
} CPUID_DATA;
#endif // AMD_H_
