/** @file

  Copyright (C) 2008-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include  <GnbDxio.h>

/**
  Get number of complexes in platform topology configuration

  @param[in] ComplexList  First complex configuration in complex configuration array
  @retval                 Number of Complexes

**/
UINTN
PcieInputParserGetNumberOfComplexes (
  IN      PCIE_COMPLEX_DESCRIPTOR  *ComplexList
  )
{
  return 0;
}

/**
  Get number of PCIe engines in given complex

  @param[in] Complex     Complex configuration
  @retval                Number of Engines
**/
UINTN
PcieInputParserGetLengthOfPcieEnginesList (
  IN      PCIE_COMPLEX_DESCRIPTOR  *Complex
  )
{
  return 0;
}

/**
  Get number of DDI engines in given complex

  @param[in] Complex     Complex configuration
  @retval                Number of Engines
**/
UINTN
PcieInputParserGetLengthOfDdiEnginesList (
  IN      PCIE_COMPLEX_DESCRIPTOR  *Complex
  )
{
  return 0;
}

/**
  Get number of engines in given complex



  @param[in] Complex     Complex configuration header
  @retval                Number of Engines
**/
UINTN
PcieInputParserGetNumberOfEngines (
  IN      PCIE_COMPLEX_DESCRIPTOR  *Complex
  )
{
  return 0;
}

/**
  Get Complex descriptor by index from given Platform configuration

  @param[in] ComplexList Platform topology configuration
  @param[in] Index       Complex descriptor Index
  @retval                Pointer to Complex Descriptor
**/
PCIE_COMPLEX_DESCRIPTOR *
PcieInputParserGetComplexDescriptor (
  IN      PCIE_COMPLEX_DESCRIPTOR  *ComplexList,
  IN      UINTN                    Index
  )
{
  return NULL;
}

/**
  Get Complex descriptor by index from given Platform configuration

  @param[in] ComplexList  Platform topology configuration
  @param[in] SocketId     Socket Id
  @retval                Pointer to Complex Descriptor
**/
PCIE_COMPLEX_DESCRIPTOR *
PcieInputParserGetComplexDescriptorOfSocket (
  IN      PCIE_COMPLEX_DESCRIPTOR  *ComplexList,
  IN      UINT32                   SocketId
  )
{
  return NULL;
}

/**
  Get Engine descriptor from given complex by index

  @param[in] Complex     Complex descriptor
  @param[in] Index       Engine descriptor index
  @retval                Pointer to Engine Descriptor
**/
PCIE_ENGINE_DESCRIPTOR *
PcieInputParserGetEngineDescriptor (
  IN      PCIE_COMPLEX_DESCRIPTOR  *Complex,
  IN      UINTN                    Index
  )
{
  return NULL;
}
