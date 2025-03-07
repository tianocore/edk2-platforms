/** @file

  Copyright (C) 2018-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FABRIC_NUMA_SERVICES2_H_
#define FABRIC_NUMA_SERVICES2_H_
#include "AMD.h"

#pragma pack (push, 1)

#define  MAX_PXM_VALUES_PER_QUADRANT  16

/// Domain type
typedef enum {
  NumaDram,
  NumaSLink,
  NumaCxl,
  MaxNumaDomainType2
} DOMAIN_TYPE2;

/// Reported Domain Info
typedef struct {
  DOMAIN_TYPE2    Type;           ///< Type
  UINT32          SocketMap;      ///< Bitmap indicating physical socket location
  UINT32          PhysicalDomain; ///< Physical domain number
} DOMAIN_INFO2;

/// Physical Dram Info
typedef struct {
  UINT32    NormalizedCsMap;                  ///< Bitmap of CSs comprising this physical domain
  UINT32    SharingEntityCount;               ///< Number of virtual domains sharing this physical domain
  UINT32    SharingEntityMap;                 ///< Bitmap of reported domains that share this physical domain
  UINT32    Reserved;                         ///< Reserved
} PHYS_DOMAIN_INFO;

/// Proximity Domain Info
typedef struct {
  UINTN    Count;                               ///< Entries in Domain array
  UINTN    Domain[MAX_PXM_VALUES_PER_QUADRANT]; ///< Domains in Quadrant
} PXM_DOMAIN_INFO;

///
/// Forward declaration for the FABRIC_NUMA_SERVICES2_PROTOCOL
///
typedef struct _FABRIC_NUMA_SERVICES2_PROTOCOL FABRIC_NUMA_SERVICES2_PROTOCOL;

/**
 * @brief Get the numa domain information.
 *
 * @details Get the numa domain information.
 *
 * @param[in]   This                       A pointer to the FABRIC_NUMA_SERVICES2_PROTOCOL instance.
 * @param[out]  NumberOfDomainsInSystem    Number of unique NUMA domains
 * @param[out]  DomainInfo                 An array with information about each domain
 * @param[out]  CcxAsNumaDomain            TRUE: each core complex is its own domain
 *                                         FALSE: physical mapping is employed
 * @retval EFI_STATUS                      0: Success, NonZero: Standard EFI Error.
 */
typedef
EFI_STATUS
(EFIAPI *FABRIC_NUMA_SERVICES2_GET_DOMAIN_INFO)(
  IN       FABRIC_NUMA_SERVICES2_PROTOCOL  *This,
  OUT   UINT32                          *NumberOfDomainsInSystem,
  OUT   DOMAIN_INFO2                   **DomainInfo,
  OUT   BOOLEAN                         *CcxAsNumaDomain
  );

/**
 * @brief Translates a core's physical location to the appropriate NUMA domain.
 *
 * @details Translates a core's physical location to the appropriate NUMA domain.
 *
 * @param[in]  This               A pointer to the FABRIC_NUMA_SERVICES2_PROTOCOL instance.
 * @param[in]  Socket             Zero based socket that the core is attached to
 * @param[in]  Die                DF die on socket that the core is attached to
 * @param[in]  Ccd                Logical CCD the core is on
 * @param[in]  Ccx                Logical core complex
 * @param[out] Domain             Domain the core belongs to
 * @retval EFI_STATUS             0: Success, NonZero: Standard EFI Error.
 */
typedef
EFI_STATUS
(EFIAPI *FABRIC_NUMA_SERVICES2_DOMAIN_XLAT)(
  IN       FABRIC_NUMA_SERVICES2_PROTOCOL  *This,
  IN       UINTN                            Socket,
  IN       UINTN                            Die,
  IN       UINTN                            Ccd,
  IN       UINTN                            Ccx,
  OUT   UINT32                          *Domain
  );

/**
 * @brief Get physical numa domain information.
 *
 * @details Get physical numa domain information.
 *
 * @param[in]  This                           A pointer to the FABRIC_NUMA_SERVICES2_PROTOCOL instance.
 * @param[out] NumberOfPhysDomainsInSystem    Number of valid domains in the system
 * @param[out] PhysDomainInfo                 An array with information about each physical domain
 * @param[out] PhysNodesPerSocket             Actual NPS as determined by ABL (not including SLink)
 * @param[out] NumberOfSystemSLinkDomains     Number of domains describing SLink connected memory
 * @retval EFI_STATUS                         0: Success, NonZero: Standard EFI Error.
 */
typedef
EFI_STATUS
(EFIAPI *FABRIC_NUMA_SERVICES2_GET_PHYSICAL_DOMAIN_INFO)(
  IN       FABRIC_NUMA_SERVICES2_PROTOCOL  *This,
  OUT   UINT32                          *NumberOfPhysDomainsInSystem,
  OUT   PHYS_DOMAIN_INFO               **PhysDomainInfo,
  OUT   UINT32                          *PhysNodesPerSocket,
  OUT   UINT32                          *NumberOfSystemSLinkDomains
  );

/**
 * @brief Get the proximity domain information about a PCIe root-port bridge
 *
 * @details Get the proximity domain information about a PCIe root-port bridge
 *
 * @param[in]  This                           A pointer to the FABRIC_NUMA_SERVICES2_PROTOCOL instance.
 * @param[in]  RootPortBDF                    BDF for root-port bridge in PCI_ADDR format.
 * @param[out] PxmDomainInfo                  Pointer to a structure returning associated NUMA node(s).
 * @retval EFI_STATUS                         0: Success, NonZero: Standard EFI Error.
 */
typedef
EFI_STATUS
(EFIAPI *FABRIC_NUMA_SERVICES2_GET_PROXIMITY_DOMAIN_INFO)(
  IN       FABRIC_NUMA_SERVICES2_PROTOCOL  *This,
  IN       PCI_ADDR                         RootPortBDF,
  OUT   PXM_DOMAIN_INFO                 *PxmDomainInfo
  );

///
/// When installed, the Fabric NUMA Services 2 Protocol produces a collection of
/// services that return various information associated with non-uniform memory
/// architecture.
///
struct _FABRIC_NUMA_SERVICES2_PROTOCOL {
  UINTN                                              Revision;          ///< Revision Number
  FABRIC_NUMA_SERVICES2_GET_DOMAIN_INFO              GetDomainInfo;     ///< Get Domain Info
  FABRIC_NUMA_SERVICES2_DOMAIN_XLAT                  DomainXlat;        ///< Domain Translation
  FABRIC_NUMA_SERVICES2_GET_PHYSICAL_DOMAIN_INFO     GetPhysDomainInfo; ///< Get Physical Domain Info
  FABRIC_NUMA_SERVICES2_GET_PROXIMITY_DOMAIN_INFO    GetPxmDomainInfo;  ///< Get Proximity Domain Info
};

///
/// Guid declaration for the FABRIC_NUMA_SERVICES2_PROTOCOL.
///
extern EFI_GUID  gAmdFabricNumaServices2ProtocolGuid;

#pragma pack (pop)
#endif // _FABRIC_NUMA_SERVICES2_H_
