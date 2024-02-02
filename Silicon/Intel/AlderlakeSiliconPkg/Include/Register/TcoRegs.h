/** @file
  Register names for PCH TCO device

  Conventions:

  - Register definition format:
    Prefix_[GenerationName]_[ComponentName]_SubsystemName_RegisterSpace_RegisterName
  - Prefix:
    Definitions beginning with "R_" are registers
    Definitions beginning with "B_" are bits within registers
    Definitions beginning with "V_" are meaningful values within the bits
    Definitions beginning with "S_" are register size
    Definitions beginning with "N_" are the bit position
  - [GenerationName]:
    Three letter acronym of the generation is used (e.g. SKL,KBL,CNL etc.).
    Register name without GenerationName applies to all generations.
  - [ComponentName]:
    This field indicates the component name that the register belongs to (e.g. PCH, SA etc.)
    Register name without ComponentName applies to all components.
    Register that is specific to -H denoted by "_PCH_H_" in component name.
    Register that is specific to -LP denoted by "_PCH_LP_" in component name.
  - SubsystemName:
    This field indicates the subsystem name of the component that the register belongs to
    (e.g. PCIE, USB, SATA, GPIO, PMC etc.).
  - RegisterSpace:
    MEM - MMIO space register of subsystem.
    IO  - IO space register of subsystem.
    PCR - Private configuration register of subsystem.
    CFG - PCI configuration space register of subsystem.
  - RegisterName:
    Full register name.

   Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
   SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#ifndef _TCO_REGS_H_
#define _TCO_REGS_H_

//
// TCO register I/O map
//
#define R_TCO_IO_TCO1_STS                            0x04
#define S_TCO_IO_TCO1_STS                            2
#define B_TCO_IO_TCO1_STS_DMISERR                    BIT12
#define B_TCO_IO_TCO1_STS_DMISMI                     BIT10
#define B_TCO_IO_TCO1_STS_DMISCI                     BIT9
#define B_TCO_IO_TCO1_STS_BIOSWR                     BIT8
#define B_TCO_IO_TCO1_STS_NEWCENTURY                 BIT7
#define B_TCO_IO_TCO1_STS_TIMEOUT                    BIT3
#define B_TCO_IO_TCO1_STS_TCO_INT                    BIT2
#define B_TCO_IO_TCO1_STS_SW_TCO_SMI                 BIT1
#define N_TCO_IO_TCO1_STS_DMISMI                     10
#define N_TCO_IO_TCO1_STS_BIOSWR                     8
#define N_TCO_IO_TCO1_STS_NEWCENTURY                 7
#define N_TCO_IO_TCO1_STS_TIMEOUT                    3
#define N_TCO_IO_TCO1_STS_SW_TCO_SMI                 1
#define N_TCO_IO_TCO1_STS_NMI2SMI                    0

#define R_TCO_IO_TCO2_STS                            0x06
#define S_TCO_IO_TCO2_STS                            2
#define N_TCO_IO_TCO2_STS_INTRD_DET                  0

#define R_TCO_IO_TCO1_CNT                            0x08
#define S_TCO_IO_TCO1_CNT                            2
#define B_TCO_IO_TCO1_CNT_TMR_HLT                    BIT11
#define N_TCO_IO_TCO1_CNT_NMI2SMI_EN                 9

#define R_TCO_IO_TCO2_CNT                            0x0A
#define S_TCO_IO_TCO2_CNT                            2
#define N_TCO_IO_TCO2_CNT_INTRD_SEL                  2

#endif
