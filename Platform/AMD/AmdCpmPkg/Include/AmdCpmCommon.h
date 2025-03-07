/** @file

  Copyright (C) 2012-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef AMD_CPM_COMMON_H_
#define AMD_CPM_COMMON_H_

#pragma pack(push)

#include <AGESA.h>
#include <AmdPcieComplex.h>
#include <AmdCpmDefine.h>
#include <AmdCpmFunction.h>

/// The signatures of CPM table

typedef enum {
  CPM_SIGNATURE_DXIO_TOPOLOGY    =            SIGNATURE_32 ('$', 'A', '2', '6'), ///< The signature of Socket 0 AMD_CPM_DXIO_TOPOLOGY_TABLE
  CPM_SIGNATURE_DXIO_TOPOLOGY_S1 =            SIGNATURE_32 ('$', 'A', '2', '7')  ///< The signature of Socket 1 AMD_CPM_DXIO_TOPOLOGY_TABLE
} AMD_CPM_TABLE_SIGNATURE;

/// CPM table header
typedef struct {
  UINT32    TableSignature;                                   ///< Signature of CPM table
  UINT16    TableSize;                                        ///< Table size
  UINT8     FormatRevision;                                   ///< Revision of table format
  UINT8     ContentRevision;                                  ///< Revision of table content
  UINT32    PlatformMask;                                     ///< The mask of platform table supports
  UINT32    Attribute;                                        ///< Table attribute
} AMD_CPM_TABLE_COMMON_HEADER;

/// Table pointer
typedef union {
  VOID      *Pointer;                                         ///< Table pointer
  UINT64    Raw;                                              ///< Table pointer value
} AMD_CPM_POINTER;

/// DXIO Topology Table
typedef struct {
  AMD_CPM_TABLE_COMMON_HEADER    Header;                                ///< Table header
  UINT32                         SocketId;                              ///< Socket Id
  DXIO_PORT_DESCRIPTOR           Port[AMD_DXIO_PORT_DESCRIPTOR_SIZE];   ///< DXIO Port Descriptor List
} AMD_CPM_DXIO_TOPOLOGY_TABLE;

/// AMD CPM Main Table
typedef struct {
  AMD_CPM_TABLE_COMMON_HEADER    Header;                          ///< Table header
  UINT8                          PlatformName[32];                ///< Platform name
  UINT8                          BiosType;                        ///< BIOS type
  UINT16                         CurrentPlatformId;               ///< Current Platform Id
  UINT32                         PcieMemIoBaseAddr;               ///< PcieMemIoBaseAddr
  UINT32                         AcpiMemIoBaseAddr;               ///< AcpiMemIoBaseAddr
  AMD_CPM_POINTER                Service;                         ///< Reserved for internal use
  AMD_CPM_POINTER                TableInRomList;                  ///< Reserved for internal use
  AMD_CPM_POINTER                TableInRamList;                  ///< Reserved for internal use
  AMD_CPM_POINTER                TableInHobList;                  ///< Reserved for internal use
  AMD_CPM_POINTER                HobTablePtr;                     ///< Reserved for internal use

  UINT8                          ExtClkGen;                       ///< External ClkGen Config. 0x00~0x7F
  UINT8                          UnusedGppClkOffEn;               ///< Config to turn off unused GPP clock
  UINT8                          LpcUartEn;                       ///< LpcUartEn
  UINT64                         AltAcpiMemIoBaseAddr;            ///< Alternate AcpiMemIoBaseAddr for Slave FCH
} AMD_CPM_MAIN_TABLE;

/// Structure for Chip Id
typedef struct {
  UINT8    Cpu;                                               ///< CPU/APU Chip Id
  UINT8    Sb;                                                ///< SB Chip Id
  UINT8    Reserved[6];
} AMD_CPM_CHIP_ID;
#pragma pack (pop)

#endif //AMD_CPM_COMMON_H_
