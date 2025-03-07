/** @file

  Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef NBIO_CXL_SERVICES_PROTOCOL_H_
#define NBIO_CXL_SERVICES_PROTOCOL_H_

#include "AMD.h"
#include <Protocol/FabricTopologyServices2.h>
#include <Protocol/FabricResourceManagerServicesProtocol.h>

#define AMD_NBIO_CXL_SERVICES_REVISION  0x00

/// Forward declaration for the AMD_NBIO_CXL_SERVICES_PROTOCOL
typedef struct _AMD_NBIO_CXL_SERVICES_PROTOCOL AMD_NBIO_CXL_SERVICES_PROTOCOL;

extern EFI_GUID  gAmdNbioCxlServicesProtocolGuid;         ///< CXL services protocol calling

#pragma pack (push, 1)
/// Port Information Structure
typedef struct _AMD_CXL_PORT_INFO_STRUCT {
  PCI_ADDR    EndPointBDF;              ///< Bus/Device/Function of Root Port in PCI_ADDR format
  UINT8       LogicalNbioInstance;      ///< Logical Instance ID of NBIO
  UINT8       PhysicalNbioInstance;     ///< Physical Instance ID of NBIO where this port is located
  UINT8       SocketID;                 ///< Socket ID for this port
  UINT32      UsRcrb;                   ///< Upstream Port RCRB address
  UINT32      DsRcrb;                   ///< Downstream Port RCRB address
  UINT32      UsMemBar0;                ///< Upstream port MEMBAR0
  UINT32      DsMemBar0;                ///< Downstream port MEMBAR0
  UINT8       PortId;                   ///< Physical port location
  UINT8       PortWidth;                ///< Lane width of the port
  UINT32      CxlPortAddress;           ///< CXL root port address (CXL 2.0 root port or CXL 1.1 RCiEP)
  BOOLEAN     IsSwitch;                 ///< CXL Switch flag
} AMD_CXL_PORT_INFO_STRUCT;

/// Port MMIO32 Resources Information Structure
typedef struct _AMD_CXL_RESOURCES_INFO_STRUCT {
  UINT32    Mmio32Base[MAX_SOCKETS_SUPPORTED][MAX_RBS_PER_SOCKET];
  UINT32    Mmio32Size[MAX_SOCKETS_SUPPORTED][MAX_RBS_PER_SOCKET];
  UINT32    Mmio32Gran;
} AMD_CXL_RESOURCES_INFO_STRUCT;
#pragma pack (pop)

// Protocol Definitions

/**
  This function gets information about a specific PCIe root port.

  This
    A pointer to the AMD_NBIO_CXL_SERVICES_PROTOCOL instance.
  EndpointBDF
    Bus/Device/Function of Endpoint in PCI_ADDR format.
  PortInformation
    A pointer to an information structure to be populated by this function to
    identify the location of the CXL port.
**/
typedef
EFI_STATUS
(EFIAPI *AMD_CXL_GET_ROOT_PORT_INFORMATION)(
  IN  AMD_NBIO_CXL_SERVICES_PROTOCOL  *This,                ///< ptr
  IN  UINTN                           PortIndex,            ///< port index
  OUT AMD_CXL_PORT_INFO_STRUCT        *PortInformation      ///< port information ptr
  );

/**
  This function configures a specific PCIe root port for CXL capabilities.

  This
    A pointer to the AMD_NBIO_CXL_SERVICES_PROTOCOL instance.
  EndpointBDF
    Bus/Device/Function of Endpoint in PCI_ADDR format.
  PortConfiguration
    A pointer to a configuration structure that contains the information necessary
    to configurare the CXL port.
  PortInformation OPTIONAL (can be NULL)
    A pointer to an information structure to be populated by this function to
    identify the location of the CXL port.
**/
typedef
EFI_STATUS
(EFIAPI *AMD_CXL_CONFIGURE_ROOT_PORT)(
  IN  AMD_NBIO_CXL_SERVICES_PROTOCOL  *This,          ///< this ptr
  IN  PCI_ADDR                        EndpointBDF     ///< end pt bdf
  );

/**
  This function configures a specific PCIe root port for CXL presence

  This
    A pointer to the AMD_NBIO_CXL_SERVICES_PROTOCOL instance.
  EndpointBDF
    Bus/Device/Function of Endpoint in PCI_ADDR format.
  PortConfiguration
    A pointer to a configuration structure that contains the information necessary
    to configurare the CXL port such as Socket id,rbindex,port id,segment,bus base,limit or presence.
**/
typedef
EFI_STATUS
(EFIAPI *AMD_CXL_RESOURCES_AVAILABLE)(
                                                            ///< cxl port presence info
  IN     AMD_NBIO_CXL_SERVICES_PROTOCOL  *This,             ///<
  IN OUT FABRIC_RESOURCE_FOR_EACH_RB     *ResourceForEachRb ///<
  );

typedef
EFI_STATUS
(EFIAPI *AMD_CXL_GET_PORT_RB_LOCATION)(
                                                 ///< get port rb location
  IN  AMD_NBIO_CXL_SERVICES_PROTOCOL  *This,     ///<
  IN  UINT8                           Segment,   ///<
  IN  UINT8                           BusBase,   ///<
  OUT UINT8                           *SocketId, ///<
  OUT UINT8                           *RbIndex   ///<
  );

/**
  This function gets the CXL MMIO32 values to be used by the fabric resource manager

  This
    A pointer to the AMD_NBIO_CXL_SERVICES_PROTOCOL instance.
  CxlMmio32ResourceForEachRb
    A pointer to the structure that will hold the MMIO32 base and size values for each root bridge
**/
typedef
EFI_STATUS
(EFIAPI *AMD_CXL_RESOURCES_INFORMATION)(
                                                             ///< cxl port presence info
  IN    AMD_NBIO_CXL_SERVICES_PROTOCOL  *This,               ///<
  IN OUT AMD_CXL_RESOURCES_INFO_STRUCT   *CxlMmio32Resources ///<
  );

/**
 * @brief
 *
 */
typedef
EFI_STATUS
(EFIAPI *AMD_CXL_REPORT_TO_MPIO)(
  IN    AMD_NBIO_CXL_SERVICES_PROTOCOL  *This                         ///< ptr to protocol
  );

/**
 * @brief
 *
 */
typedef
EFI_STATUS
(EFIAPI *AMD_CXL_FIND_2P0_DEVICES)(
  IN  AMD_NBIO_CXL_SERVICES_PROTOCOL  *This,                        ///< ptr to protocol
  IN  UINTN                           PortIndex                     ///< port index
  );

/**
 * @brief
 *
 */
typedef
VOID
(EFIAPI *AMD_CXL_ENABLE_SCM_PMEM)(
  IN  AMD_NBIO_CXL_SERVICES_PROTOCOL  *This,                        ///< ptr to protocol
  IN  PCI_ADDR                        CxlPciAddress                 ///< PCI address
  );

/// The Protocol Definition for CXL Services
struct _AMD_NBIO_CXL_SERVICES_PROTOCOL {
  UINT32                               Revision;                        ///< revision
  UINTN                                CxlCount;                        ///< CXL count
  AMD_CXL_GET_ROOT_PORT_INFORMATION    CxlGetRootPortInformation;       ///< CXL root port information
  AMD_CXL_CONFIGURE_ROOT_PORT          CxlConfigureRootPort;            ///< configuring the root port
  AMD_CXL_GET_PORT_RB_LOCATION         GetCxlPortRBLocation;            ///< CXL port RB location
  AMD_CXL_RESOURCES_AVAILABLE          GetCxlAvailableResources;        ///< Get resources allocated for CXL RCiEP
  AMD_CXL_RESOURCES_INFORMATION        GetCxlMmio32Resources;           ///< Get CXL MMIO resources for CXL RCiEP
  AMD_CXL_REPORT_TO_MPIO               CxlReportToMpio;                 ///< Sends the CXL info to MPIO
  AMD_CXL_FIND_2P0_DEVICES             CxlFind2p0Devices;               ///< Finds CXL 2.0 devices after PCIe enumeration
  AMD_CXL_ENABLE_SCM_PMEM              CxlEnableScmForPersistentMemory; ///< Notifies SMU that CXL persistent memory is present
};

#endif /* NBIO_CXL_SERVICES_PROTOCOL_H */
