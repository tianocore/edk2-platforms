/** @file
 *
 * PCI Host Bridge Library instance for StarFive JH7110 SOC
 *
 * Copyright (c) 2023, StarFive Technology Co., Ltd. All rights reserved.<BR>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * This module initializes the Pci as close to a standard
 * PCI root complex as possible. The general information
 * for this driver was sourced from.
 *
 *
 **/

#include <IndustryStandard/JH7110.h>
#include <IndustryStandard/Pci22.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/PciHostBridgeLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <PiDxe.h>
#include <Protocol/PciHostBridgeResourceAllocation.h>
#include <Library/TimerLib.h>

#define RegWrite(addr, data)  MmioWrite32((addr), (data))
#define RegRead(addr, data)   ((data) = MmioRead32 (addr))

#define STG_SYSCON_BASE  0x10240000

#define STG_SYSCON_K_RP_NEP_MASK    (1 << 8)
#define STG_SYSCON_CKREF_SRC_SHIFT  18
#define STG_SYSCON_CKREF_SRC_MASK   (0x3 << 18)
#define STG_SYSCON_CLKREQ_MASK      (1 << 22)
#define STG_SYSCON_BASE             0x10240000
#define SYS_CLK_BASE                0x13020000
#define STG_CLK_BASE                0x10230000
#define SYS_CLK_NOC_OFFSET          0x98
#define STG_PCIE_CLK_OFFSET         0x20
#define STG_PCIE_CLKS               0xc
#define STG_PCIE_RESET_OFFSET       0x74
#define SYS_GPIO_BASE               0x13040000

#define PREF_MEM_WIN_64_SUPPORT  (1 << 3)
#define PMSG_LTR_SUPPORT         (1 << 2)
#define PDLA_LINK_SPEED_GEN2     (1 << 12)
#define PLDA_FUNCTION_DIS        (1 << 15)
#define PLDA_FUNC_NUM            4
#define PLDA_PHY_FUNC_SHIFT      9
#define PLDA_RP_ENABLE           1

#define PCIE_BASIC_STATUS  0x018
#define PCIE_CFGNUM        0x140
#define IMASK_LOCAL        0x180
#define ISTATUS_LOCAL      0x184
#define IMSI_ADDR          0x190
#define ISTATUS_MSI        0x194
#define CFG_SPACE          0x1000
#define GEN_SETTINGS       0x80
#define PCIE_PCI_IDS       0x9C
#define PCIE_WINROM        0xFC
#define PMSG_SUPPORT_RX    0x3F0
#define PCI_MISC           0xB4

#define STG_SYSCON_AXI4_SLVL_ARFUNC_MASK   0x7FFF00
#define STG_SYSCON_AXI4_SLVL_ARFUNC_SHIFT  0x8
#define STG_SYSCON_AXI4_SLVL_AWFUNC_MASK   0x7FFF
#define STG_SYSCON_AXI4_SLVL_AWFUNC_SHIFT  0x0

#define XR3PCI_ATR_AXI4_SLV0       0x800
#define XR3PCI_ATR_SRC_ADDR_LOW    0x0
#define XR3PCI_ATR_SRC_ADDR_HIGH   0x4
#define XR3PCI_ATR_TRSL_ADDR_LOW   0x8
#define XR3PCI_ATR_TRSL_ADDR_HIGH  0xc
#define XR3PCI_ATR_TRSL_PARAM      0x10
#define XR3PCI_ATR_TABLE_OFFSET    0x20
#define XR3PCI_ATR_MAX_TABLE_NUM   8

#define XR3PCI_ATR_SRC_ADDR_MASK       0xfffff000
#define XR3PCI_ATR_TRSL_ADDR_MASK      0xfffff000
#define XR3PCI_ATR_SRC_WIN_SIZE_SHIFT  1
#define XR3_PCI_ECAM_SIZE              28

#define IDS_PCI_TO_PCI_BRIDGE  0x060400
#define IDS_CLASS_CODE_SHIFT   8
#define SYS_GPIO_OUTPUT_OFF    0x40

UINT32  AtrTableNum;
UINT64  PCIE_CFG_BASE[2]      = { FixedPcdGet32 (PcdPci0Mmio64Base) + FixedPcdGet32 (PcdPci0Mmio64Size),
                                  FixedPcdGet32 (PcdPci1Mmio64Base) + FixedPcdGet32 (PcdPci1Mmio64Size) };
UINT64  PCI_MEMREGION_32[2]   = { FixedPcdGet32 (PcdPci0Mmio32Base), FixedPcdGet32 (PcdPci1Mmio32Base) };
UINT64  PCI_MEMREGION_64[2]   = { FixedPcdGet32 (PcdPci0Mmio64Base), FixedPcdGet32 (PcdPci1Mmio64Base) };
UINT64  PCI_MEMREGION_SIZE[2] = { 27, 30 };
UINT32  STG_ARFUNC_OFFSET[2]  = { 0xc0, 0x270 };
UINT32  STG_AWFUNC_OFFSET[2]  = { 0xc4, 0x274 };
UINT32  STG_RP_REP_OFFSET[2]  = { 0x130, 0x2e0 };
UINT32  PCIE_GPIO[2]          = { 26, 28 };

STATIC inline UINT64
GetPcieRegBase (
  IN UINT32  Port
  )
{
  return PCIE_REG_BASE + Port * 0x1000000;
}

VOID
PcieRegWrite (
  IN UINT32  Port,
  IN UINTN   Offset,
  IN UINT32  Value
  )
{
  UINT64  Base = GetPcieRegBase (Port);

  RegWrite ((UINT64)Base + Offset, Value);
}

UINT32
PcieRegRead (
  IN UINT32  Port,
  IN UINTN   Offset
  )
{
  UINT32  Value = 0;
  UINT64  Base  = GetPcieRegBase (Port);

  RegRead ((UINT64)Base + Offset, Value);
  return Value;
}

STATIC VOID
PcieUpdatebits (
  IN UINT64  Base,
  IN UINTN   Offset,
  IN UINT32  Mask,
  IN UINT32  val
  )
{
  UINT32  Value = 0;

  Value  = MmioRead32 ((UINT64)Base + Offset);
  Value &= ~Mask;
  Value |= val;
  MmioWrite32 ((UINT64)Base + Offset, Value);
}

STATIC
VOID
PcieFuncSet (
  IN UINT32  Port
  )
{
  INTN    i;
  UINT32  Value;
  UINT64  Base = GetPcieRegBase (Port);

  /* Disable physical functions except #0 */
  for (i = 1; i < PLDA_FUNC_NUM; i++) {
    PcieUpdatebits (
                    STG_SYSCON_BASE,
                    STG_ARFUNC_OFFSET[Port],
                    STG_SYSCON_AXI4_SLVL_ARFUNC_MASK,
                    (i << PLDA_PHY_FUNC_SHIFT) <<
                    STG_SYSCON_AXI4_SLVL_ARFUNC_SHIFT
                    );
    PcieUpdatebits (
                    STG_SYSCON_BASE,
                    STG_AWFUNC_OFFSET[Port],
                    STG_SYSCON_AXI4_SLVL_AWFUNC_MASK,
                    (i << PLDA_PHY_FUNC_SHIFT) <<
                    STG_SYSCON_AXI4_SLVL_AWFUNC_SHIFT
                    );
    PcieUpdatebits (
                    Base,
                    PCI_MISC,
                    PLDA_FUNCTION_DIS,
                    PLDA_FUNCTION_DIS
                    );
  }

  PcieUpdatebits (
                  STG_SYSCON_BASE,
                  STG_ARFUNC_OFFSET[Port],
                  STG_SYSCON_AXI4_SLVL_ARFUNC_MASK,
                  0
                  );
  PcieUpdatebits (
                  STG_SYSCON_BASE,
                  STG_AWFUNC_OFFSET[Port],
                  STG_SYSCON_AXI4_SLVL_AWFUNC_MASK,
                  0
                  );

  /* Enable root port*/
  PcieUpdatebits (
                  Base,
                  GEN_SETTINGS,
                  PLDA_RP_ENABLE,
                  PLDA_RP_ENABLE
                  );

  Value = (IDS_PCI_TO_PCI_BRIDGE << IDS_CLASS_CODE_SHIFT);
  PcieRegWrite (Port, PCIE_PCI_IDS, Value);

  PcieUpdatebits (
                  Base,
                  PMSG_SUPPORT_RX,
                  PMSG_LTR_SUPPORT,
                  0
                  );

  /* Prefetchable memory window 64-bit addressing support */
  PcieUpdatebits (
                  Base,
                  PCIE_WINROM,
                  PREF_MEM_WIN_64_SUPPORT,
                  PREF_MEM_WIN_64_SUPPORT
                  );
}

STATIC
VOID
PcieSTGInit (
  IN UINT32  Port
  )
{
  PcieUpdatebits (
                  STG_SYSCON_BASE,
                  STG_RP_REP_OFFSET[Port],
                  STG_SYSCON_K_RP_NEP_MASK,
                  STG_SYSCON_K_RP_NEP_MASK
                  );
  PcieUpdatebits (
                  STG_SYSCON_BASE,
                  STG_AWFUNC_OFFSET[Port],
                  STG_SYSCON_CKREF_SRC_MASK,
                  2 << STG_SYSCON_CKREF_SRC_SHIFT
                  );
  PcieUpdatebits (
                  STG_SYSCON_BASE,
                  STG_AWFUNC_OFFSET[Port],
                  STG_SYSCON_CLKREQ_MASK,
                  STG_SYSCON_CLKREQ_MASK
                  );
}

STATIC
VOID
PcieClockInit (
  IN UINT32  Port
  )
{
  RegWrite (
            STG_CLK_BASE + STG_PCIE_CLK_OFFSET
            + Port * STG_PCIE_CLKS,
            1 << 31
            ); /*axi mst0*/
  RegWrite (
            STG_CLK_BASE + STG_PCIE_CLK_OFFSET
            + Port * STG_PCIE_CLKS + 4,
            1 << 31
            ); /* apb */
  RegWrite (
            STG_CLK_BASE + STG_PCIE_CLK_OFFSET
            + Port * STG_PCIE_CLKS + 8,
            1 << 31
            ); /* tl0 */
}

STATIC
VOID
PcieResetDeassert (
  IN UINT32  Port
  )
{
  UINT32  PortOffset = Port * 6 + 11;

  PcieUpdatebits (
                  STG_CLK_BASE,
                  STG_PCIE_RESET_OFFSET,
                  0x3f << (PortOffset),
                  0
                  ); /*reset all*/
}

VOID
PcieResetAssert (
  IN UINT32  Port
  )
{
  UINT32  PortOffset = Port * 6 + 11;

  PcieUpdatebits (
                  STG_CLK_BASE,
                  STG_PCIE_RESET_OFFSET,
                  0x3f << (PortOffset),
                  0x3f << (PortOffset)
                  ); /*axi mst0*/
}

STATIC
VOID
PcieGpioResetSet (
  IN UINT32  Port,
  IN UINT32  Value
  )
{
  UINT32  Remain, Mask;

  Remain = PCIE_GPIO[Port] & 0x3;
  Mask   = 0xff << (Remain * 8);
  PcieUpdatebits (
                  SYS_GPIO_BASE,
                  SYS_GPIO_OUTPUT_OFF + (PCIE_GPIO[Port] & 0xfffc),
                  Mask,
                  Value << (Remain * 8)
                  );
}

STATIC
VOID
PcieAtrInit (
  IN UINT32  Port,
  IN UINT64  SrcAddr,
  IN UINT64  TrslAddr,
  IN UINT32  WinSize,
  IN UINT32  Config
  )
{
  UINT64  Base = GetPcieRegBase (Port) + XR3PCI_ATR_AXI4_SLV0;
  UINT32  Value;

  Base +=  XR3PCI_ATR_TABLE_OFFSET * AtrTableNum;
  AtrTableNum++;

  /* X3PCI_ATR_SRC_ADDR_LOW:
   *   - bit 0: enable entry,
   *   - bits 1-6: ATR window size: total size in bytes: 2^(ATR_WSIZE + 1)
   *   - bits 7-11: reserved
   *   - bits 12-31: start of source address
   */
  Value = SrcAddr;

  RegWrite (
            Base + XR3PCI_ATR_SRC_ADDR_LOW,
            (Value & XR3PCI_ATR_SRC_ADDR_MASK) | ((WinSize - 1) << 1) | 0x1
            );
  Value = SrcAddr >> 32;

  RegWrite (Base + XR3PCI_ATR_SRC_ADDR_HIGH, Value);
  Value = TrslAddr;
  RegWrite (Base + XR3PCI_ATR_TRSL_ADDR_LOW, Value);
  Value = TrslAddr >> 32;
  RegWrite (Base + XR3PCI_ATR_TRSL_ADDR_HIGH, Value);
  RegWrite (Base + XR3PCI_ATR_TRSL_PARAM, Config);
}

EFI_STATUS
EFIAPI
JH7110PciHostBridgeLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINT32  PortIndex;

  DEBUG ((DEBUG_ERROR, "PCIe RootBridge constructor\n"));
  for (PortIndex = 0; PortIndex < 2; PortIndex++) {
    PcieSTGInit (PortIndex);
    RegWrite (SYS_CLK_BASE + SYS_CLK_NOC_OFFSET, 1 << 31);
    PcieClockInit (PortIndex);
    PcieResetDeassert (PortIndex);
    PcieGpioResetSet (PortIndex, 0);
    PcieFuncSet (PortIndex);

    PcieAtrInit (
                 PortIndex,
                 PCIE_CFG_BASE[PortIndex],
                 0,
                 XR3_PCI_ECAM_SIZE,
                 1
                 );
    PcieAtrInit (
                 PortIndex,
                 PCI_MEMREGION_32[PortIndex],
                 PCI_MEMREGION_32[PortIndex],
                 PCI_MEMREGION_SIZE[0],
                 0
                 );
    PcieAtrInit (
                 PortIndex,
                 PCI_MEMREGION_64[PortIndex],
                 PCI_MEMREGION_64[PortIndex],
                 PCI_MEMREGION_SIZE[1],
                 0
                 );
    PcieGpioResetSet (PortIndex, 1);
    MicroSecondDelay (300);

    DEBUG ((DEBUG_ERROR, "PCIe port %d init\n", PortIndex));
  }

  return EFI_SUCCESS;
}
