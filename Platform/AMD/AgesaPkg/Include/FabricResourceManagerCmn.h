/** @file

  Copyright (C) 2008-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FABRIC_RESOURCE_MANAGER_CMN_H_
#define FABRIC_RESOURCE_MANAGER_CMN_H_
#pragma pack (push, 1)

#define MAX_SOCKETS_SUPPORTED  4                ///< Max number of sockets in system.
#define MAX_RBS_PER_SOCKET     20               ///< Max number of root bridges per socket.

/**
 *  @brief DF address aperture structure.
 *  @details This contains information used to define an MMIO region.
 */
typedef struct _FABRIC_ADDR_APERTURE {
  UINT64    Base;                       ///< Aperture base Address.
  UINT64    Size;                       ///< Aperture size.
  UINT64    Alignment;                  ///< Alignment bit map. 0xFFFFF means 1MB align.
} FABRIC_ADDR_APERTURE;

/**
 *  @brief DF Resource for each RootBridge structure.
 *  @details This contains information used to define the MMIO region for each RootBridge.
 */
typedef struct _FABRIC_RESOURCE_FOR_EACH_RB {
  FABRIC_ADDR_APERTURE    NonPrefetchableMmioSizeAbove4G[MAX_SOCKETS_SUPPORTED][MAX_RBS_PER_SOCKET];  ///< Nonprefetchable MMIO resource(s) above 4G. @see FABRIC_ADDR_APERTURE
  FABRIC_ADDR_APERTURE    PrefetchableMmioSizeAbove4G[MAX_SOCKETS_SUPPORTED][MAX_RBS_PER_SOCKET];     ///< Prefetchable MMIO resource(s) above 4G. @see FABRIC_ADDR_APERTURE
  FABRIC_ADDR_APERTURE    NonPrefetchableMmioSizeBelow4G[MAX_SOCKETS_SUPPORTED][MAX_RBS_PER_SOCKET];  ///< Nonprefetchable MMIO resource(s) below 4G. @see FABRIC_ADDR_APERTURE
  FABRIC_ADDR_APERTURE    PrefetchableMmioSizeBelow4G[MAX_SOCKETS_SUPPORTED][MAX_RBS_PER_SOCKET];     ///< Prefetchable MMIO resource(s) below 4G. @see FABRIC_ADDR_APERTURE
  FABRIC_ADDR_APERTURE    PrimaryRbSecondNonPrefetchableMmioSizeBelow4G;                              ///< Primary RootBridge's second nonprefetchable MMIO size below 4G. @see FABRIC_ADDR_APERTURE
  FABRIC_ADDR_APERTURE    PrimaryRbSecondPrefetchableMmioSizeBelow4G;                                 ///< Primary RootBridge's second prefetchable MMIO size below 4G. @see FABRIC_ADDR_APERTURE
  FABRIC_ADDR_APERTURE    IO[MAX_SOCKETS_SUPPORTED][MAX_RBS_PER_SOCKET];                              ///< IO resource(s) @see FABRIC_ADDR_APERTURE
  UINT16                  PciBusNumber[MAX_SOCKETS_SUPPORTED][MAX_RBS_PER_SOCKET];                    ///< PCI bus number(s).
} FABRIC_RESOURCE_FOR_EACH_RB;
#pragma pack (pop)
#endif // FABRIC_RESOURCE_MANAGER_CMN_H_
