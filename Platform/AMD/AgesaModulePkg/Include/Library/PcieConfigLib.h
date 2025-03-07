/** @file

  Copyright (C) 2008-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PCIECONFIGLIB_LIB_H_
#define PCIECONFIGLIB_LIB_H_

typedef VOID (*PCIE_RUN_ON_ENGINE_CALLBACK) (
  IN      PCIE_ENGINE_CONFIG    *Engine,
  IN OUT  VOID                  *Buffer,
  IN      PCIE_PLATFORM_CONFIG  *Pcie
  );

typedef AGESA_STATUS (*PCIE_RUN_ON_WRAPPER_CALLBACK) (
  IN      PCIE_WRAPPER_CONFIG   *Wrapper,
  IN OUT  VOID                  *Buffer,
  IN      PCIE_PLATFORM_CONFIG  *Pcie
  );

typedef VOID (*PCIE_RUN_ON_ENGINE_CALLBACK2) (
  IN      PCIE_ENGINE_CONFIG   *Engine,
  IN OUT  VOID                 *Buffer,
  IN      PCIE_WRAPPER_CONFIG  *Wrapper
  );

typedef VOID (*PCIE_RUN_ON_WRAPPER_CALLBACK2) (
  IN      PCIE_WRAPPER_CONFIG  *Wrapper,
  IN OUT  VOID                 *Buffer,
  IN      GNB_HANDLE           *GnbHandle
  );

typedef AGESA_STATUS (*PCIE_RUN_ON_DESCRIPTOR_CALLBACK) (
  IN      PCIE_DESCRIPTOR_HEADER  *Descriptor,
  IN OUT  VOID                    *Buffer,
  IN      PCIE_PLATFORM_CONFIG    *Pcie
  );

/**
  Check Port Status

  @param[in]  Engine          Pointer to engine config descriptor
  @param[in]  PortStatus      Check if status asserted for port
  @retval                     TRUE if status asserted
**/
BOOLEAN
PcieConfigCheckPortStatus (
  IN       PCIE_ENGINE_CONFIG  *Engine,
  IN       UINT32              PortStatus
  );

/**
  Set/Reset port status

  @param[in]  Engine          Pointer to engine config descriptor
  @param[in]  SetStatus       SetStatus
  @param[in]  ResetStatus     ResetStatus

**/
UINT16
PcieConfigUpdatePortStatus (
  IN       PCIE_ENGINE_CONFIG       *Engine,
  IN       PCIE_ENGINE_INIT_STATUS  SetStatus,
  IN       PCIE_ENGINE_INIT_STATUS  ResetStatus
  );

/**
  Execute callback on all engine in topology

  @param[in]       DescriptorFlags Engine flags.
  @param[in]       Callback        Pointer to callback function
  @param[in, out]  Buffer          Pointer to buffer to pass information to callback
  @param[in]       Pcie            Pointer to global PCIe configuration
**/
VOID
PcieConfigRunProcForAllEngines (
  IN       UINT32                       DescriptorFlags,
  IN       PCIE_RUN_ON_ENGINE_CALLBACK  Callback,
  IN OUT   VOID                         *Buffer,
  IN       PCIE_PLATFORM_CONFIG         *Pcie
  );

/**
  Execute callback on all engine in wrapper

  @param[in]       DescriptorFlags Engine flags.
  @param[in]       Callback        Pointer to callback function
  @param[in, out]  Buffer          Pointer to buffer to pass information to callback
  @param[in]       Pcie            Pointer to global PCIe configuration
**/
VOID
PcieConfigRunProcForAllEnginesInWrapper (
  IN       UINT32                        DescriptorFlags,
  IN       PCIE_RUN_ON_ENGINE_CALLBACK2  Callback,
  IN OUT   VOID                          *Buffer,
  IN       PCIE_WRAPPER_CONFIG           *Wrapper
  );

/**
  Execute callback on all wrappers in topology

  @param[in]       DescriptorFlags   Wrapper Flags
  @param[in]       Callback          Pointer to callback function
  @param[in, out]  Buffer            Pointer to buffer to pass information to callback
  @param[in]       Pcie              Pointer to global PCIe configuration
**/
AGESA_STATUS
PcieConfigRunProcForAllWrappers (
  IN       UINT32                        DescriptorFlags,
  IN       PCIE_RUN_ON_WRAPPER_CALLBACK  Callback,
  IN OUT   VOID                          *Buffer,
  IN       PCIE_PLATFORM_CONFIG          *Pcie
  );

/**
  Execute callback on all wrappers in NBIO


  @param[in]       DescriptorFlags   Wrapper Flags
  @param[in]       Callback          Pointer to callback function
  @param[in, out]  Buffer            Pointer to buffer to pass information to callback
  @param[in]       Pcie              Pointer to global PCIe configuration
**/
VOID
PcieConfigRunProcForAllWrappersInNbio (
  IN       UINT32                         DescriptorFlags,
  IN       PCIE_RUN_ON_WRAPPER_CALLBACK2  Callback,
  IN OUT   VOID                           *Buffer,
  IN       GNB_HANDLE                     *GnbHandle
  );

/**
  Execute callback on all descriptor of specific type

  @param[in]       InDescriptorFlags    Include descriptor flags
  @param[in]       OutDescriptorFlags   Exclude descriptor flags
  @param[in]       TerminationFlags     Termination flags
  @param[in]       Callback             Pointer to callback function
  @param[in, out]  Buffer               Pointer to buffer to pass information to callback
  @param[in]       Pcie                 Pointer to global PCIe configuration
**/
AGESA_STATUS
PcieConfigRunProcForAllDescriptors (
  IN       UINT32                           InDescriptorFlags,
  IN       UINT32                           OutDescriptorFlags,
  IN       UINT32                           TerminationFlags,
  IN       PCIE_RUN_ON_DESCRIPTOR_CALLBACK  Callback,
  IN OUT   VOID                             *Buffer,
  IN       PCIE_PLATFORM_CONFIG             *Pcie
  );

/**
  Get parent descriptor of specific type

  @param[in]       Type            Descriptor type
  @param[in]       Descriptor      Pointer to buffer to pass information to callback
**/
PCIE_DESCRIPTOR_HEADER *
PcieConfigGetParent (
  IN       UINT32                  Type,
  IN       PCIE_DESCRIPTOR_HEADER  *Descriptor
  );

/**
  Get child descriptor of specific type

  @param[in]       Type            Descriptor type
  @param[in]       Descriptor      Pointer to buffer to pass information to callback
**/
PCIE_DESCRIPTOR_HEADER *
PcieConfigGetChild (
  IN       UINT32                  Type,
  IN       PCIE_DESCRIPTOR_HEADER  *Descriptor
  );

/**
  Get peer descriptor of specific type

  @param[in]       Type            Descriptor type
  @param[in]       Descriptor      Pointer to buffer to pass information to callback
**/
PCIE_DESCRIPTOR_HEADER *
PcieConfigGetPeer (
  IN       UINT32                  Type,
  IN       PCIE_DESCRIPTOR_HEADER  *Descriptor
  );

/**
  Check is engine is active or potentially active

  @param[in]  Engine      Pointer to engine descriptor
  @retval                 TRUE  - engine active
  @retval                 FALSE - engine not active
**/
BOOLEAN
PcieConfigIsActivePcieEngine (
  IN      PCIE_ENGINE_CONFIG  *Engine
  );

/**
  Locate SB engine on wrapper

  @param[in]  Wrapper     Pointer to wrapper config descriptor
  @retval                 SB engine pointer or NULL
**/
PCIE_ENGINE_CONFIG *
PcieConfigLocateSbEngine (
  IN      PCIE_WRAPPER_CONFIG  *Wrapper
  );

/**
  Helper function to dump configuration to debug out

  @param[in]  Pcie                Pointer to global PCIe configuration
**/
VOID
PcieConfigDebugDump (
  IN      PCIE_PLATFORM_CONFIG  *Pcie
  );

/**
  Helper function to dump wrapper configuration

  @param[in]  WrapperList           Wrapper Configuration
**/
VOID
PcieConfigWrapperDebugDump (
  IN      PCIE_WRAPPER_CONFIG  *WrapperList
  );

/**
  Helper function to dump engine configuration

  @param[in]  EngineList           Engine Configuration
**/
VOID
PcieConfigEngineDebugDump (
  IN      PCIE_ENGINE_CONFIG  *EngineList
  );

/**
  Helper function to dump input configuration to debug out

  @param[in]  ComplexDescriptor   Pointer to user defined complex descriptor
**/
VOID
PcieUserConfigConfigDump (
  IN      PCIE_COMPLEX_DESCRIPTOR  *ComplexDescriptor
  );

/**
  Helper function to dump input configuration to user engine descriptor

  @param[in]  EngineDescriptor   Pointer to engine descriptor
**/
VOID
PcieUserDescriptorConfigDump (
  IN      PCIE_ENGINE_DESCRIPTOR  *EngineDescriptor
  );

#define PcieConfigGetParentWrapper(Descriptor)                            ((PCIE_WRAPPER_CONFIG *) PcieConfigGetParent (DESCRIPTOR_ALL_WRAPPERS, &((Descriptor)->Header)))
#define PcieConfigGetParentSilicon(Descriptor)                            ((PCIE_SILICON_CONFIG *) PcieConfigGetParent (DESCRIPTOR_SILICON, &((Descriptor)->Header)))
#define PcieConfigGetParentComplex(Descriptor)                            ((PCIE_COMPLEX_CONFIG *) PcieConfigGetParent (DESCRIPTOR_COMPLEX, &((Descriptor)->Header)))
#define PcieConfigGetPlatform(Descriptor)                                 ((PCIE_PLATFORM_CONFIG *) PcieConfigGetParent (DESCRIPTOR_PLATFORM, &((Descriptor)->Header)))
#define PcieConfigGetChildWrapper(Descriptor)                             ((PCIE_WRAPPER_CONFIG *) PcieConfigGetChild (DESCRIPTOR_ALL_WRAPPERS, &((Descriptor)->Header)))
#define PcieConfigGetChildEngine(Descriptor)                              ((PCIE_ENGINE_CONFIG *) PcieConfigGetChild (DESCRIPTOR_ALL_ENGINES, &((Descriptor)->Header)))
#define PcieConfigGetChildSilicon(Descriptor)                             ((PCIE_SILICON_CONFIG *) PcieConfigGetChild (DESCRIPTOR_SILICON, &((Descriptor)->Header)))
#define PcieConfigGetNextDescriptor(Descriptor)                           ((((Descriptor->Header.DescriptorFlags & DESCRIPTOR_TERMINATE_LIST) != 0) ? NULL : ((Descriptor + 1))))
#define PcieConfigIsPcieEngine(Descriptor)                                ((Descriptor != NULL) ? ((Descriptor->Header.DescriptorFlags & DESCRIPTOR_PCIE_ENGINE) != 0) : FALSE)
#define PcieConfigIsSbPcieEngine(Engine)                                  ((Engine != NULL) ? ((BOOLEAN) (Engine->Type.Port.PortData.MiscControls.SbLink)) : FALSE)
#define PcieConfigIsEngineAllocated(Descriptor)                           ((Descriptor != NULL) ? ((Descriptor->Header.DescriptorFlags & DESCRIPTOR_ALLOCATED) != 0) : FALSE)
#define PcieConfigSetDescriptorFlags(Descriptor, SetDescriptorFlags)      if (Descriptor != NULL) (Descriptor)->Header.DescriptorFlags |= SetDescriptorFlags
#define PcieConfigResetDescriptorFlags(Descriptor, ResetDescriptorFlags)  if (Descriptor != NULL) ((PCIE_DESCRIPTOR_HEADER *) Descriptor)->DescriptorFlags &= (~(ResetDescriptorFlags))
#define PcieInputParsetGetNextDescriptor(Descriptor)                      ((Descriptor == NULL) ? NULL : ((Descriptor->Flags & DESCRIPTOR_TERMINATE_LIST) != 0) ? NULL : (Descriptor + 1))
#define PcieConfigGetNextTopologyDescriptor(Descriptor, Termination)      ((Descriptor == NULL) ? NULL : ((((PCIE_DESCRIPTOR_HEADER *) Descriptor)->DescriptorFlags & Termination) != 0) ? NULL : ((UINT8 *) Descriptor + ((PCIE_DESCRIPTOR_HEADER *) Descriptor)->Peer))
#define GnbGetNextHandle(Descriptor)                                      (GNB_HANDLE *) PcieConfigGetNextTopologyDescriptor (Descriptor, DESCRIPTOR_TERMINATE_TOPOLOGY)
#define PcieConfigGetNextDataDescriptor(Descriptor)                       ((Descriptor->Flags & DESCRIPTOR_TERMINATE_LIST) != 0 ? NULL : (Descriptor + 1))

#endif // PCIECONFIGLIB_LIB_H_
