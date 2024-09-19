/** @file

  Copyright (c) 2021 - 2023, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ROOT_COMPLEX_INFO_HOB_H_
#define ROOT_COMPLEX_INFO_HOB_H_

#define ROOT_COMPLEX_INFO_HOB_GUID \
  { 0x568a258a, 0xcaa1, 0x47e9, { 0xbb, 0x89, 0x65, 0xa3, 0x73, 0x9b, 0x58, 0x75 } }

extern GUID  gRootComplexInfoHobGuid;

#define PRESET_INVALID  0xFF

//
// PCIe link width
//
#define LINK_WIDTH_NONE  0x00
#define LINK_WIDTH_X1    0x01
#define LINK_WIDTH_X2    0x02
#define LINK_WIDTH_X4    0x04
#define LINK_WIDTH_X8    0x08
#define LINK_WIDTH_X16   0x10

//
// PCIe link speed
//
#define LINK_SPEED_NONE  0x00
#define LINK_SPEED_GEN1  0x01
#define LINK_SPEED_GEN2  0x02
#define LINK_SPEED_GEN3  0x04
#define LINK_SPEED_GEN4  0x08

typedef enum {
  DevMapMode1 = 0,
  DevMapMode2,
  DevMapMode3,
  DevMapMode4,
  DevMapModeAuto,
  MaxDevMapMode = DevMapModeAuto
} DEV_MAP_MODE;

//
// PCIe controller index
//
typedef enum {
  PcieController0 = 0,
  PcieController1,
  PcieController2,
  PcieController3,
  PcieController4,
  MaxPcieControllerOfRootComplexA = PcieController4,
  PcieController5,
  PcieController6,
  PcieController7,
  MaxPcieController,
  MaxPcieControllerOfRootComplexB = MaxPcieController
} AC01_PCIE_CONTROLLER_INDEX;

//
// Root Complex type
//
typedef enum {
  RootComplexTypeA,
  RootComplexTypeB,
  MaxRootComplexType = RootComplexTypeB
} AC01_ROOT_COMPLEX_TYPE;

//
// Root Complex index
//
typedef enum {
  RootComplexA0 = 0,
  RootComplexA1,
  RootComplexA2,
  RootComplexA3,
  MaxRootComplexA,
  RootComplexB0 = MaxRootComplexA,
  RootComplexB1,
  RootComplexB2,
  RootComplexB3,
  MaxRootComplex,
  MaxRootComplexB = MaxRootComplex
} AC01_ROOT_COMPLEX_INDEX;

#pragma pack(1)

//
// Data structure to store the PCIe controller information
//
typedef struct {
  PHYSICAL_ADDRESS    CsrBase;             // Base address of CSR block
  PHYSICAL_ADDRESS    SnpsRamBase;         // Base address of Synopsys SRAM
  UINT8               MaxGen;              // Max speed Gen-1/-2/-3/-4
  UINT8               CurrentGen;          // Current speed Gen-1/-2/-3/-4
  UINT8               MaxWidth;            // Max lanes x2/x4/x8/x16
  UINT8               CurWidth;            // Current lanes x2/x4/x8/x16
  UINT8               ID;                  // ID of the controller within Root Complex
  UINT8               DevNum;              // Device number as part of Bus:Dev:Func
  BOOLEAN             Active;              // Active? Used in bi-furcation mode
  BOOLEAN             LinkUp;              // PHY and PCIE linkup
  BOOLEAN             HotPlug;             // Hotplug support
} AC01_PCIE_CONTROLLER;

//
// Data structure to store the Root Complex information
//
typedef struct {
  PHYSICAL_ADDRESS          CsrBase;
  PHYSICAL_ADDRESS          TcuBase;
  PHYSICAL_ADDRESS          HostBridgeBase;
  PHYSICAL_ADDRESS          SerdesBase;
  PHYSICAL_ADDRESS          MmcfgBase;
  PHYSICAL_ADDRESS          MmioBase;
  PHYSICAL_ADDRESS          MmioSize;
  PHYSICAL_ADDRESS          Mmio32Base;
  PHYSICAL_ADDRESS          Mmio32Size;
  AC01_PCIE_CONTROLLER      Pcie[MaxPcieController];
  UINT8                     MaxPcieController;
  AC01_ROOT_COMPLEX_TYPE    Type;
  UINT8                     ID;
  DEV_MAP_MODE              DevMapHigh        : 3; // Copy of High Devmap programmed to Host bridge
  DEV_MAP_MODE              DevMapLow         : 3; // Copy of Low Devmap programmed to Host bridge
  DEV_MAP_MODE              DefaultDevMapHigh : 3; // Default of High devmap based on board settings
  DEV_MAP_MODE              DefaultDevMapLow  : 3; // Default of Low devmap based on board settings
  UINT8                     Socket;
  BOOLEAN                   Active;
  BOOLEAN                   DefaultActive;
  UINT16                    Logical;
  UINT32                    Flags;
  UINT8                     PresetGen3[MaxPcieController];
  UINT8                     PresetGen4[MaxPcieController];
} AC01_ROOT_COMPLEX;

#pragma pack()

#endif /* ROOT_COMPLEX_INFO_HOB_H_ */
