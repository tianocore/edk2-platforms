/** @file

  Copyright (C) 2008-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef NBIO_PCIE_SERVICES_PROTOCOL_H_
#define NBIO_PCIE_SERVICES_PROTOCOL_H_

// Current PROTOCOL revision
#define AMD_NBIO_PCIE_SERVICES_REVISION  0x00

///
/// Forward declaration for the NBIO_PCIE_SERVICES_PROTOCOL
///
typedef struct _DXE_AMD_NBIO_PCIE_SERVICES_PROTOCOL DXE_AMD_NBIO_PCIE_SERVICES_PROTOCOL;

//
// Protocol Definitions
//

/**
  Returns the NBIO debug options configuration structure
  This
    A pointer to the DXE_AMD_NBIO_SMU_SERVICES_PROTOCOL instance.
  DebugOptions
    A pointer to a pointer to store the address of the PCIe topology structure
**/
typedef
EFI_STATUS
(EFIAPI *AMD_NBIO_PCIE_GET_TOPOLOGY_STRUCT)(
  IN  DXE_AMD_NBIO_PCIE_SERVICES_PROTOCOL  *This,
  OUT UINT32                               **DebugOptions
  );

///
/// The Dxe of PCIE Services
///
struct _DXE_AMD_NBIO_PCIE_SERVICES_PROTOCOL {
  AMD_NBIO_PCIE_GET_TOPOLOGY_STRUCT    PcieGetTopology;  ///<
};

extern EFI_GUID  gAmdNbioPcieServicesProtocolGuid;

#endif /* NBIO_PCIE_SERVICES_PROTOCOL_H */
