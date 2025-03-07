/** @file

  Copyright (C) 2008-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef NBIOHANDLELIB_LIB_H_
#define NBIOHANDLELIB_LIB_H_

#include <GnbDxio.h>

/**
  Get GNB handle

 @param[in]       Pcie           Pointer to global PCIe configuration

**/
GNB_HANDLE *
NbioGetHandle (
  IN PCIE_PLATFORM_CONFIG  *Pcie
  );

/**
  Get GNB handle of alternate host bridge (e.g. MI200)

  @param[in]       Pcie           Pointer to global PCIe configuration
**/
GNB_HANDLE *
NbioGetAltHandle (
  IN PCIE_PLATFORM_CONFIG  *Pcie
  );

/**
  Get GNB handle of next socket

  @param[in]       NbioHandle        Pointer to current GnbHandle
**/
GNB_HANDLE *
NbioGetNextSocket (
  IN GNB_HANDLE  *NbioHandle
  );

/**
  Get PCI_ADDR of GNB

  @param[in]  Handle           Pointer to GNB_HANDLE
**/
PCI_ADDR
NbioGetHostPciAddress (
  IN      GNB_HANDLE  *Handle
  );

#define GnbGetNextHandle(Descriptor)  (GNB_HANDLE *) PcieConfigGetNextTopologyDescriptor (Descriptor, DESCRIPTOR_TERMINATE_TOPOLOGY)
#define GnbGetSocketId(Handle)        (Handle != NULL ? (Handle)->SocketId : 0)
#define GnbGetDieNumber(Handle)       (Handle != NULL ? (Handle)->DieNumber : 0)
#define GnbGetRBIndex(Handle)         (Handle != NULL ? (Handle)->RBIndex : 0)

#endif // NBIOHANDLELIB_LIB_H_
