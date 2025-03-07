/** @file
     Common AMD header file
  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
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
  AGESA_SUCCESS = 0,            ///< 0 -The service completed normally. Info may be logged.
  AGESA_UNSUPPORTED,            ///< 1 - The dispatcher or create struct had an unimplemented function requested.
  ///<      Not logged.
  AGESA_BOUNDS_CHK,             ///< 2 - A dynamic parameter was out of range and the service was not provided.
  ///<      Example, memory address not installed, heap buffer handle not found.
  ///<      Not Logged.
  AGESA_SYNC_MORE_DATA,     ///< 3 - More data is available from PSP communications
  AGESA_SYNC_SLAVE_ASSERT,  ///< 4 - Slave is at an ASSERT (used in ABL)
  // AGESA_STATUS of greater severity (the ones below this line), always have a log entry available.
  AGESA_ALERT,                     ///< 5 - An observed condition, but no loss of function.  See Log.
  AGESA_WARNING,                   ///< 6 - Possible or minor loss of function.  See Log.
  AGESA_ERROR,                     ///< 7 - Significant loss of function, boot may be possible.  See Log.
  AGESA_CRITICAL,                  ///< 8 - Continue boot only to notify user.  See Log.
  AGESA_FATAL,                     ///< 9 - Halt booting.  See Log, however Fatal errors pertaining to heap problems
  ///<      may not be able to reliably produce log events.
  AGESA_OC_FATAL,                 ///< 10 - Halt booting.  Critical Memory Overclock failure.
  AGESA_SKIP_ERROR,               ///< 11 - Error, Skip init steps.
  AgesaStatusMax                  ///< Not a status, for limit checking.
} AGESA_STATUS;

/// For checking whether a status is at or above the mandatory log level.
#define AGESA_STATUS_LOG_LEVEL  AGESA_ALERT

/**
 * Callout method to the host environment.
 *
 * Callout using a dispatch with appropriate thunk layer, which is determined by the host environment.
 *
 * @param[in]        Function      The specific callout function being invoked.
 * @param[in]        FcnData       Function specific data item.
 * @param[in,out]    ConfigPtr     Reference to Callout params.
 */
typedef AGESA_STATUS (*CALLOUT_ENTRY) (
  IN       UINT32  Function,
  IN       UINTN   FcnData,
  IN OUT   VOID    *ConfigPtr
  );

typedef AGESA_STATUS (*IMAGE_ENTRY) (
  VOID  *ConfigPtr
  );
typedef AGESA_STATUS (*MODULE_ENTRY) (
  VOID  *ConfigPtr
  );

/// This allocation type is used by the AmdCreateStruct entry point
typedef enum {
  PreMemHeap = 0,                                           ///< Create heap in cache.
  PostMemDram,                                              ///< Create heap in memory.
  ByHost                                                    ///< Create heap by Host.
} ALLOCATION_METHOD;

/// These width descriptors are used by the library function, and others, to specify the data size
typedef enum ACCESS_WIDTH {
  AccessWidthNone = 0,                                      ///< dummy access width
  AccessWidth8    = 1,                                      ///< Access width is 8 bits.
  AccessWidth16,                                            ///< Access width is 16 bits.
  AccessWidth32,                                            ///< Access width is 32 bits.
  AccessWidth64,                                            ///< Access width is 64 bits.

  AccessS3SaveWidth8 = 0x81,                                ///< Save 8 bits data.
  AccessS3SaveWidth16,                                      ///< Save 16 bits data.
  AccessS3SaveWidth32,                                      ///< Save 32 bits data.
  AccessS3SaveWidth64,                                      ///< Save 64 bits data.
} ACCESS_WIDTH;

/// AGESA struct name
typedef enum {
  // AGESA BASIC FUNCTIONS
  AMD_INIT_RECOVERY = 0x00020000,                           ///< AmdInitRecovery entry point handle
  AMD_CREATE_STRUCT,                                        ///< AmdCreateStruct handle
  AMD_INIT_EARLY,                                           ///< AmdInitEarly entry point handle
  AMD_INIT_ENV,                                             ///< AmdInitEnv entry point handle
  AMD_INIT_LATE,                                            ///< AmdInitLate entry point handle
  AMD_INIT_MID,                                             ///< AmdInitMid entry point handle
  AMD_INIT_POST,                                            ///< AmdInitPost entry point handle
  AMD_INIT_RESET,                                           ///< AmdInitReset entry point handle
  AMD_INIT_RESUME,                                          ///< AmdInitResume entry point handle
  AMD_RELEASE_STRUCT,                                       ///< AmdReleaseStruct handle
  AMD_S3LATE_RESTORE,                                       ///< AmdS3LateRestore entry point handle
  AMD_GET_APIC_ID,                                          ///< AmdGetApicId entry point handle
  AMD_GET_PCI_ADDRESS,                                      ///< AmdGetPciAddress entry point handle
  AMD_IDENTIFY_CORE,                                        ///< AmdIdentifyCore general service handle
  AMD_READ_EVENT_LOG,                                       ///< AmdReadEventLog general service handle
  AMD_GET_EXECACHE_SIZE,                                    ///< AmdGetAvailableExeCacheSize general service handle
  AMD_LATE_RUN_AP_TASK,                                     ///< AmdLateRunApTask entry point handle
  AMD_IDENTIFY_DIMMS,                                       ///< AmdIdentifyDimm general service handle
  AMD_GET_2D_DATA_EYE,                                      ///< AmdGet2DDataEye general service handle
  AMD_S3FINAL_RESTORE,                                      ///< AmdS3FinalRestore entry point handle
  AMD_INIT_RTB                                              ///< AmdInitRtb entry point handle
} AGESA_STRUCT_NAME;

// AGESA Structures

/// The standard header for all AGESA services.
/// For internal AGESA naming conventions, see @ref amdconfigparamname .
typedef struct {
  IN       UINT32           ImageBasePtr;           ///< The AGESA Image base address.
  IN       UINT32           Func;                   ///< The service desired
  IN       UINT32           AltImageBasePtr;        ///< Alternate Image location
  IN       CALLOUT_ENTRY    CalloutPtr;             ///< For Callout from AGESA
  IN       UINT8            HeapStatus;             ///< For heap status from boot time slide.
  IN       UINT64           HeapBasePtr;            ///< Location of the heap
  IN OUT   UINT8            Reserved[7];            ///< This space is reserved for future use.
} AMD_CONFIG_PARAMS;

/// Create Struct Interface.
typedef struct {
  IN       AMD_CONFIG_PARAMS    StdHeader;         ///< Standard configuration header
  IN       AGESA_STRUCT_NAME    AgesaFunctionName; ///< The service to init
  IN       ALLOCATION_METHOD    AllocationMethod;  ///< How to handle buffer allocation
  IN OUT   UINT32               NewStructSize;     ///< The size of the allocated data, in for ByHost, else out only.
  IN OUT   VOID                 *NewStructPtr;     ///< The struct for the service.
                                                   ///< The struct to init for ByHost allocation,
                                                   ///< the initialized struct on return.
} AMD_INTERFACE_PARAMS;

/// AGESA Binary module header structure
typedef struct {
  IN  UINT32    Signature;                        ///< Binary Signature
  IN  CHAR8     CreatorID[8];                     ///< 8 characters ID
  IN  CHAR8     Version[12];                      ///< 12 characters version
  IN  UINT32    ModuleInfoOffset;                 ///< Offset of module
  IN  UINT32    EntryPointAddress;                ///< Entry address
  IN  UINT32    ImageBase;                        ///< Image base
  IN  UINT32    RelocTableOffset;                 ///< Relocate Table offset
  IN  UINT32    ImageSize;                        ///< Size
  IN  UINT16    Checksum;                         ///< Checksum
  IN  UINT8     ImageType;                        ///< Type
  IN  UINT8     V_Reserved;                       ///< Reserved
} AMD_IMAGE_HEADER;

/// AGESA Binary module header structure
typedef struct _AMD_MODULE_HEADER {
  IN  UINT32                       ModuleHeaderSignature; ///< Module signature
  IN  CHAR8                        ModuleIdentifier[8];   ///< 8 characters ID
  IN  CHAR8                        ModuleVersion[12];     ///< 12 characters version
  IN  VOID                         *ModuleDispatcher;     ///< A pointer point to dispatcher
  IN  struct _AMD_MODULE_HEADER    *NextBlock;            ///< Next module header link
} AMD_MODULE_HEADER;

/// AGESA_CODE_SIGNATURE
typedef struct {
  IN  CHAR8    Signature[8];                      ///< code header Signature
  IN  CHAR8    ComponentName[16];                 ///< 16 character name of the code module
  IN  CHAR8    Version[12];                       ///< 12 character version string
  IN  CHAR8    TerminatorNull;                    ///< null terminated string
  IN  CHAR8    VerReserved[7];                    ///< reserved space
} AMD_CODE_HEADER;

//   SBDFO - Segment Bus Device Function Offset
//   31:28   Segment (4-bits)
//   27:20   Bus     (8-bits)
//   19:15   Device  (5-bits)
//   14:12   Function(3-bits)
//   11:00   Offset  (12-bits)

#define MAKE_SBDFO(Seg, Bus, Dev, Fun, Off)  ((((UINT32) (Seg)) << 28) | (((UINT32) (Bus)) << 20) |\
                   (((UINT32)(Dev)) << 15) | (((UINT32)(Fun)) << 12) | ((UINT32)(Off)))
#define ILLEGAL_SBDFO  0xFFFFFFFFul

/// CPUID data received registers format
typedef struct {
  OUT UINT32    EAX_Reg;                          ///< CPUID instruction result in EAX
  OUT UINT32    EBX_Reg;                          ///< CPUID instruction result in EBX
  OUT UINT32    ECX_Reg;                          ///< CPUID instruction result in ECX
  OUT UINT32    EDX_Reg;                          ///< CPUID instruction result in EDX
} CPUID_DATA;

// Topology Services definitions and macros
#define TOPOLOGY_LIST_TERMINAL  0xFF                        ///< End of list.

#endif // AMD_H_
