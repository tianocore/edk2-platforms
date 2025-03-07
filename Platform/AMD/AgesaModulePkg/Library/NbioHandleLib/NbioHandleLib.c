/** @file

  Copyright (C) 2008-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include  <GnbDxio.h>
#include  <Library/NbioHandleLib.h>
#include  <Library/AmdBaseLib.h>

/**
  Get GNB handle

 @param[in]       Pcie           Pointer to global PCIe configuration

**/
GNB_HANDLE *
NbioGetHandle (
  IN PCIE_PLATFORM_CONFIG  *Pcie
  )
{
  return NULL;
}

/**
  Get GNB handle of alternate host bridge (e.g. MI200)

  @param[in]       Pcie           Pointer to global PCIe configuration
**/
GNB_HANDLE *
NbioGetAltHandle (
  IN PCIE_PLATFORM_CONFIG  *Pcie
  )
{
  return NULL;
}

/**
  Get GNB handle of next socket

  @param[in]       NbioHandle        Pointer to current GnbHandle
**/
GNB_HANDLE *
NbioGetNextSocket (
  IN GNB_HANDLE  *NbioHandle
  )
{
  return NULL;
}

/**
  Get PCI_ADDR of GNB

  @param[in]  Handle           Pointer to GNB_HANDLE
**/
PCI_ADDR
NbioGetHostPciAddress (
  IN      GNB_HANDLE  *Handle
  )
{
  PCI_ADDR  PciAddr;

  PciAddr.AddressValue = 0;
  return PciAddr;
}
