/** @file
  PEI Boards Configurations for PreMem phase.

   Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _BOARD_SA_CONFIG_PRE_MEM_H_
#define _BOARD_SA_CONFIG_PRE_MEM_H_

#include <Ppi/SiPolicy.h>
#include <Library/BoardConfigLib.h>

//
// LPDDR3 / LPDDR4 RVP boards (memory soldered down)
// Single SPD EEPROM at 0xA2 serves both C0D0 and C1D0 (LPDDR3/4 is 1DPC only)
//
#define DIMM_SMB_SPD_P0C0D0_STP 0xA2
#define DIMM_SMB_SPD_P0C0D1_STP 0xA0
#define DIMM_SMB_SPD_P0C1D0_STP 0xA2
#define DIMM_SMB_SPD_P0C1D1_STP 0xA0
#define SA_MRC_MAX_RCOMP_TARGETS  (5)

//
// Reference RCOMP resistors on motherboard - MRC will set automatically
//
GLOBAL_REMOVE_IF_UNREFERENCED const UINT16 AdlPRcompResistorZero = 0;

//
// RCOMP target values for RdOdt, WrDS, WrDSCmd, WrDSCtl, WrDSClk - MRC will set automatically
//
GLOBAL_REMOVE_IF_UNREFERENCED const UINT16 RcompTargetAdlP[SA_MRC_MAX_RCOMP_TARGETS] = { 0, 0, 0, 0, 0 };

//
// DQ byte mapping to CMD/CTL/CLK, from the CPU side
//
GLOBAL_REMOVE_IF_UNREFERENCED const UINT8 DqByteMapAdlP[2][6][2] = {
  // Channel 0:
  {
    { 0x0F, 0xF0 }, // CLK0 goes to package 0 - Bytes[3:0], CLK1 goes to package 1 - Bytes[7:4]
    { 0x0F, 0xF0 }, // Cmd CAA goes to Bytes[3:0], Cmd CAB goes to Byte[7:4]
    { 0xFF, 0x00 }, // CTL (CS) goes to all bytes
    { 0x00, 0x00 }, // Unused in ICL MRC
    { 0x00, 0x00 }, // Unused in ICL MRC
    { 0x00, 0x00 }, // Unused in ICL MRC
  },
  // Channel 1:
  {
    { 0x0F, 0xF0 }, // CLK0 goes to package 0 - Bytes[3:0], CLK1 goes to package 1 - Bytes[7:4]
    { 0x0F, 0xF0 }, // Cmd CAA goes to Bytes[3:0], Cmd CAB goes to Byte[7:4]
    { 0xFF, 0x00 }, // CTL (CS) goes to all bytes
    { 0x00, 0x00 }, // Unused in ICL MRC
    { 0x00, 0x00 }, // Unused in ICL MRC
    { 0x00, 0x00 }, // Unused in ICL MRC
  }
};

//
// Display DDI settings for Adl-P Ddr5 Rvp Edp + DP
//
GLOBAL_REMOVE_IF_UNREFERENCED const UINT8 mAdlPDdr5RvpDisplayDdiConfig[16] = {
                                                                               DdiPortEdp,      // DDI Port A Config : DdiPortDisabled = No LFP is Connected, DdiPortEdp = eDP, DdiPortMipiDsi = MIPI DSI
                                                                               DdiPortDisabled, // DDI Port B Config : DdiPortDisabled = No LFP is Connected, DdiPortEdp = eDP, DdiPortMipiDsi = MIPI DSI
                                                                               DdiHpdDisable,   // DDI Port A HPD : DdiHpdDisable = Disable, DdiHpdEnable = Enable HPD
                                                                               DdiHpdEnable,    // DDI Port B HPD : DdiHpdDisable = Disable, DdiHpdEnable = Enable HPD
                                                                               DdiHpdDisable,   // DDI Port C HPD : DdiHpdDisable = Disable, DdiHpdEnable = Enable HPD
                                                                               DdiHpdDisable,   // DDI Port 1 HPD : DdiHpdDisable = Disable, DdiHpdEnable = Enable HPD
                                                                               DdiHpdDisable,   // DDI Port 2 HPD : DdiHpdDisable = Disable, DdiHpdEnable = Enable HPD
                                                                               DdiHpdDisable,   // DDI Port 3 HPD : DdiHpdDisable = Disable, DdiHpdEnable = Enable HPD
                                                                               DdiHpdDisable,   // DDI Port 4 HPD : DdiHpdDisable = Disable, DdiHpdEnable = Enable HPD
                                                                               DdiDisable,      // DDI Port A DDC : DdiDisable = Disable, DdiDdcEnable = Enable DDC
                                                                               DdiDisable,      // DDI Port B DDC : DdiDisable = Disable, DdiDdcEnable = Enable DDC
                                                                               DdiDisable,      // DDI Port C DDC : DdiDisable = Disable, DdiDdcEnable = Enable DDC
                                                                               DdiDisable,      // DDI Port 1 DDC : DdiDisable = Disable, DdiDdcEnable = Enable DDC
                                                                               DdiDisable,      // DDI Port 2 DDC : DdiDisable = Disable, DdiDdcEnable = Enable DDC
                                                                               DdiDisable,      // DDI Port 3 DDC : DdiDisable = Disable, DdiDdcEnable = Enable DDC
                                                                               DdiDisable       // DDI Port 4 DDC : DdiDisable = Disable, DdiDdcEnable = Enable DDC
};

#endif // _BOARD_SA_CONFIG_PRE_MEM_H_
