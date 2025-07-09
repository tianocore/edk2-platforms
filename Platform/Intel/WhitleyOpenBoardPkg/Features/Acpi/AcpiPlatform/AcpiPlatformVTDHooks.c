/** @file
  ACPI Platform Driver VT-D Hooks

  @copyright
  Copyright 2012 - 2025 Intel Corporation. <BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "AcpiPlatform.h"
#include "AcpiPlatformHooks.h"
#include <Protocol/PciRootBridgeIo.h>
#include <UncoreCommonIncludes.h>
#include <IioSetupDefinitions.h>
#include <PchInfoHob.h>

extern EFI_PLATFORM_INFO                    *mPlatformInfo;
extern BIOS_ACPI_PARAM                      *mAcpiParameter;
extern EFI_IIO_UDS_PROTOCOL                 *mIioUds2;
extern CPU_CSR_ACCESS_VAR                   *mCpuCsrAccessVarPtr;
extern SYSTEM_CONFIGURATION                 mSystemConfiguration;
extern SOCKET_IIO_CONFIGURATION             mSocketIioConfiguration;
extern SOCKET_PROCESSORCORE_CONFIGURATION   mSocketProcessorCoreConfiguration;
extern EFI_GUID                             mSystemConfigurationGuid;
extern BOOLEAN                              mX2ApicEnabled;


#define EFI_PCI_CAPABILITY_PTR              0x34
#define EFI_PCIE_CAPABILITY_BASE_OFFSET     0x100
#define EFI_PCIE_CAPABILITY_ID_ACS          0x000D
#define EFI_PCI_CAPABILITY_ID_PCIEXP        0x10
#define EFI_PCI_EXPRESS_CAPABILITY_REGISTER 0x02

#define ACS_CAPABILITY_REGISTER             0x04
#define ACS_SOURCE_VALIDATION               BIT0
#define ACS_P2P_REQUEST_REDIRECT            BIT2
#define ACS_P2P_COMPLETION_REDIRECT         BIT3
#define ACS_UPSTREAM_FORWARDING             BIT4

#define ACS_CONTROL_REGISTER                0x06
#define ACS_SOURCE_VALIDATION_ENABLE        BIT0
#define ACS_P2P_REQUEST_REDIRECT_ENABLE     BIT2
#define ACS_P2P_COMPLETION_REDIRECT_ENABLE  BIT3
#define ACS_UPSTREAM_FORWARDING_ENABLE      BIT4

#define R_VTD_GCMD_REG                      0x18
#define R_VTD_GSTS_REG                      0x1C
#define R_VTD_IQT_REG                       0x88
#define R_VTD_IQA_REG                       0x90
#define R_VTD_IRTA_REG                      0xB8

#define VTD_ISOCH_ENGINE_OFFSET             0x1000

//
// a flag to indicate if we should disable Vt-d for ACS WA
//
BOOLEAN                                     mDisableVtd = FALSE;

DMA_REMAP_PROTOCOL                          DmaRemapProt;
#define  VTD_SUPPORT_INSTANCE_FROM_THIS(a)  CR(a, VTD_SUPPORT_INSTANCE, DmaRemapProt, EFI_ACPI_6_2_DMA_REMAPPING_TABLE_SIGNATURE)
#define  EFI_PCI_CAPABILITY_ID_ATS          0x0F

#define SEGMENT0                            0x00
#define MEM_BLK_COUNT                       0x140
#define INTRREMAP                           BIT3
#define MEMORY_SIZE                         (MaxIIO * NUMBER_PORTS_PER_SOCKET)
#define R_VTD_EXT_CAP_LOW                   0x10
#define R_VTD_EXT_CAP_HIGH                  0x14
#define IIO_STACK0                          0
#define IOAT_DEVICE_NUM_10NM                0x01


PCI_NODE  mPciPath0_1[]   = {
  {PCI_DEVICE_NUMBER_PCH_HDA, PCI_FUNCTION_NUMBER_PCH_HDA},
  {(UINT8) -1,  (UINT8) -1},
};

//
// IOAPIC2  - IIO IoApic
//
PCI_NODE  mPciPath2_0_10nm[] = {
    { PCIE_PORT_0_DEV_0, PCIE_PORT_0_FUNC_0 },
    { (UINT8)-1, (UINT8)-1 },
};
PCI_NODE  mPciPath2_1_10nm[] = {
    { PCIE_PORT_1A_DEV_1, PCIE_PORT_1A_FUNC_1 },
    { (UINT8)-1, (UINT8)-1 },
};
PCI_NODE  mPciPath2_2_10nm[] = {
    { PCIE_PORT_1B_DEV_1, PCIE_PORT_1B_FUNC_1 },
    { (UINT8)-1, (UINT8)-1 },
};
PCI_NODE  mPciPath2_3_10nm[] = {
    { PCIE_PORT_1C_DEV_1, PCIE_PORT_1C_FUNC_1 },
    { (UINT8)-1, (UINT8)-1 },
};
PCI_NODE  mPciPath2_4_10nm[] = {
    { PCIE_PORT_1D_DEV_1, PCIE_PORT_1D_FUNC_1 },
    { (UINT8)-1, (UINT8)-1 },
};
PCI_NODE  mPciPath2_5_10nm[] = {
    { PCIE_PORT_2A_DEV_2, PCIE_PORT_2A_FUNC_2 },
    { (UINT8)-1, (UINT8)-1 },
};
PCI_NODE  mPciPath2_6_10nm[] = {
    { PCIE_PORT_2B_DEV_2, PCIE_PORT_2B_FUNC_2 },
    { (UINT8)-1, (UINT8)-1 },
};
PCI_NODE  mPciPath2_7_10nm[] = {
    { PCIE_PORT_2C_DEV_2, PCIE_PORT_2C_FUNC_2 },
    { (UINT8)-1, (UINT8)-1 },
};
PCI_NODE  mPciPath2_8_10nm[] = {
    { PCIE_PORT_2D_DEV_2, PCIE_PORT_2D_FUNC_2 },
    { (UINT8)-1, (UINT8)-1 },
};
PCI_NODE  mPciPath2_9_10nm[] = {
    { PCIE_PORT_3A_DEV_3, PCIE_PORT_3A_FUNC_3 },
    { (UINT8)-1, (UINT8)-1 },
};
PCI_NODE  mPciPath2_10_10nm[] = {
    { PCIE_PORT_3B_DEV_3, PCIE_PORT_3B_FUNC_3 },
    { (UINT8)-1, (UINT8)-1 },
};
PCI_NODE  mPciPath2_11_10nm[] = {
    { PCIE_PORT_3C_DEV_3, PCIE_PORT_3C_FUNC_3 },
    { (UINT8)-1, (UINT8)-1 },
};
PCI_NODE  mPciPath2_12_10nm[] = {
    { PCIE_PORT_3D_DEV_3, PCIE_PORT_3D_FUNC_3 },
    { (UINT8)-1, (UINT8)-1 },
};
PCI_NODE  mPciPath2_13_10nm[] = {
    { PCIE_PORT_4A_DEV_4, PCIE_PORT_4A_FUNC_4 },
    { (UINT8)-1, (UINT8)-1 },
};
PCI_NODE  mPciPath2_14_10nm[] = {
    { PCIE_PORT_4B_DEV_4, PCIE_PORT_4B_FUNC_4 },
    { (UINT8)-1, (UINT8)-1 },
};
PCI_NODE  mPciPath2_15_10nm[] = {
    { PCIE_PORT_4C_DEV_4, PCIE_PORT_4C_FUNC_4 },
    { (UINT8)-1, (UINT8)-1 },
};
PCI_NODE  mPciPath2_16_10nm[] = {
    { PCIE_PORT_4D_DEV_4, PCIE_PORT_4D_FUNC_4 },
    { (UINT8)-1, (UINT8)-1 },
};
PCI_NODE  mPciPath2_17_10nm[] = {
    { PCIE_PORT_5A_DEV_5, PCIE_PORT_5A_FUNC_5 },
    { (UINT8)-1, (UINT8)-1 },
};
PCI_NODE  mPciPath2_18_10nm[] = {
    { PCIE_PORT_5B_DEV_5, PCIE_PORT_5B_FUNC_5 },
    { (UINT8)-1, (UINT8)-1 },
};
PCI_NODE  mPciPath2_19_10nm[] = {
    { PCIE_PORT_5C_DEV_5, PCIE_PORT_5C_FUNC_5 },
    { (UINT8)-1, (UINT8)-1 },
};
PCI_NODE  mPciPath2_20_10nm[] = {
    { PCIE_PORT_5D_DEV_5, PCIE_PORT_5D_FUNC_5 },
    { (UINT8)-1, (UINT8)-1 },
};

DEVICE_SCOPE              mDevScopeDRHD[] = {
  {
    EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT,    // Device type - HD Audio
    00,                                               // Enumeration ID
    DEFAULT_PCI_BUS_NUMBER_PCH,                       // Start Bus Number
    &mPciPath0_1[0]
  },
};

DEVICE_SCOPE              mDevScopeATSR10nm[] = {
  {
    EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE,  // Pcie Port1
    00,                                           // Enumeration ID
    DMI_BUS_NUM,
    &mPciPath2_0_10nm[0]
  },
  {
    EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE,  // Pcie Port2
    00,                                           // Enumeration ID
    DMI_BUS_NUM,
    &mPciPath2_1_10nm[0]
  },
  {
    EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE,  // Pcie Port3
    00,                                           // Enumeration ID
    DMI_BUS_NUM,
    &mPciPath2_2_10nm[0]
  },
  {
    EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE,  // Pcie Port4
    00,                                           // Enumeration ID
    DMI_BUS_NUM,
    &mPciPath2_3_10nm[0]
  },
  {
    EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE,  // Pcie Port5
    00,                                           // Enumeration ID
    DMI_BUS_NUM,
    &mPciPath2_4_10nm[0]
  },
  {
    EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE,  // Pcie Port6
    00,                                           // Enumeration ID
    DMI_BUS_NUM,
    &mPciPath2_5_10nm[0]
  },
  {
    EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE,  // Pcie Port7
    00,                                           // Enumeration ID
    DMI_BUS_NUM,
    &mPciPath2_6_10nm[0]
  },
  {
    EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE,  // Pcie Port8
    00,                                           // Enumeration ID
    DMI_BUS_NUM,
    &mPciPath2_7_10nm[0]
  },
  {
    EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE,  // Pcie Port9
    00,                                           // Enumeration ID
    DMI_BUS_NUM,
    &mPciPath2_8_10nm[0]
  },
  {
    EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE,  // Pcie Port10
    00,                                           // Enumeration ID
    DMI_BUS_NUM,
    &mPciPath2_9_10nm[0]
  },
  {
    EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE,  // Pcie Port11
    00,                                           // Enumeration ID
    DMI_BUS_NUM,
    &mPciPath2_10_10nm[0]
  },
  {
    EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE,  // Pcie Port12
    00,                                           // Enumeration ID
    DMI_BUS_NUM,
    &mPciPath2_11_10nm[0]
  },
  {
    EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE,  // Pcie Port13
    00,                                           // Enumeration ID
    DMI_BUS_NUM,
    &mPciPath2_12_10nm[0]
  },
  {
    EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE,  // Pcie Port14
    00,                                           // Enumeration ID
    DMI_BUS_NUM,
    &mPciPath2_13_10nm[0]
  },
  {
    EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE,  // Pcie Port15
    00,                                           // Enumeration ID
    DMI_BUS_NUM,
        &mPciPath2_14_10nm[0]
  },
  {
    EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE,  // Pcie Port16
    00,                                           // Enumeration ID
    DMI_BUS_NUM,
    &mPciPath2_15_10nm[0]
  },
  {
    EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE,  // Pcie Port17
    00,                                           // Enumeration ID
    DMI_BUS_NUM,
    &mPciPath2_16_10nm[0]
  },
  {
    EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE,  // Pcie Port18
    00,                                           // Enumeration ID
    DMI_BUS_NUM,
    &mPciPath2_17_10nm[0]
  },
  {
    EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE,  // Pcie Port19
    00,                                           // Enumeration ID
    DMI_BUS_NUM,
    &mPciPath2_18_10nm[0]
  },
  {
    EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE,  // Pcie Port20
    00,                                           // Enumeration ID
    DMI_BUS_NUM,
    &mPciPath2_19_10nm[0]
  },
  {
    EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE,  // Pcie Port21
    00,                                           // Enumeration ID
    DMI_BUS_NUM,
    &mPciPath2_20_10nm[0]
  }
};

DMAR_DRHD                 mDrhd = {
  DRHD_SIGNATURE,
  00,                                             // Flags
  SEGMENT0,                                       // Segment number
  00,                                             // Base Address
  00,                                             // Number of dev scope structures
  &mDevScopeDRHD[0]
};

DMAR_DRHD                 mDrhdIsoc = {
  DRHD_SIGNATURE,
  00,                                             // Flags
  SEGMENT0,                                       // Segment number
  00,                                             // Base Address
  00,                                             // Number of dev scope structures
  &mDevScopeDRHD[0]
};

DMAR_ATSR                 mAtsr10nm = {
    ATSR_SIGNATURE,
    SEGMENT0,                                       // Segment number
    00,
    NUMBER_PORTS_PER_SOCKET - 1,
    00,
    &mDevScopeATSR10nm[0]
};

PCI_NODE     mPciPath[] = {
  { 00,      00},
  { (UINT8)-1,   (UINT8)-1},
};

UINT8  IoApicID[] = { PCH_IOAPIC_ID,   //PCH
   PC00_IOAPIC_ID, PC01_IOAPIC_ID, PC02_IOAPIC_ID, PC03_IOAPIC_ID, PC04_IOAPIC_ID, PC05_IOAPIC_ID,  //Socket0
   PC06_IOAPIC_ID, PC07_IOAPIC_ID, PC08_IOAPIC_ID, PC09_IOAPIC_ID, PC10_IOAPIC_ID, PC11_IOAPIC_ID,  //Socket1
   PC12_IOAPIC_ID, PC13_IOAPIC_ID, PC14_IOAPIC_ID, PC15_IOAPIC_ID, PC16_IOAPIC_ID, PC17_IOAPIC_ID,  //Socket2
   PC18_IOAPIC_ID, PC19_IOAPIC_ID, PC20_IOAPIC_ID, PC21_IOAPIC_ID, PC22_IOAPIC_ID, PC23_IOAPIC_ID,  //Socket3
   PC24_IOAPIC_ID, PC25_IOAPIC_ID, PC26_IOAPIC_ID, PC27_IOAPIC_ID, PC28_IOAPIC_ID, PC29_IOAPIC_ID,  //Socket4
   PC30_IOAPIC_ID, PC31_IOAPIC_ID, PC32_IOAPIC_ID, PC33_IOAPIC_ID, PC34_IOAPIC_ID, PC35_IOAPIC_ID,  //Socket5
   PC36_IOAPIC_ID, PC37_IOAPIC_ID, PC38_IOAPIC_ID, PC39_IOAPIC_ID, PC40_IOAPIC_ID, PC41_IOAPIC_ID,  //Socket6
   PC42_IOAPIC_ID, PC43_IOAPIC_ID, PC44_IOAPIC_ID, PC45_IOAPIC_ID, PC46_IOAPIC_ID, PC47_IOAPIC_ID,  //Socket7
};

PCI_NODE     mPciPath7[] = {
  { PCI_DEVICE_NUMBER_PCH_XHCI,      PCI_FUNCTION_NUMBER_PCH_XHCI  },
  { (UINT8)-1,   (UINT8)-1},
};
DEVICE_SCOPE DevScopeRmrr[] = {
  {
    1,                                  // RMRR dev Scope - XHCI
    0,                                  // Enumeration ID
    0,                                  // Start Bus Number
    &mPciPath7[0]
  },
};

DMAR_RMRR    mRmrr = {
  RMRR_SIGNATURE,                       // Signature
  SEGMENT0,                             // Segment number
  ' ',                                  // Reserved Memory RegionBase Address
  ' ',                                  // Reserved Memory RegionLimit Address
  ' ',                                  // Number of Dev Scope structures
  &DevScopeRmrr[0]
};

typedef struct {
    UINT8   aBuf[32];
} MEM_BLK;

DMAR_RHSA                 mRhsa;

typedef struct {
    UINT8   Bus;
    UINT8   Dev;
    UINT8   Func;
} DMAR_DEVICE;

EFI_STATUS
LocateCapRegBlock(
  IN     EFI_PCI_IO_PROTOCOL  *PciIo,
  IN     UINT8                CapID,
  OUT    UINT8                *PciExpressOffset,
  OUT    UINT8                *NextRegBlock
  );

EFI_STATUS
LocatePciExpressCapRegBlock (
  IN     EFI_PCI_IO_PROTOCOL  *PciIo,
  IN     UINT16               CapID,
  OUT    UINT32               *Offset,
  OUT    UINT32               *NextRegBlock
);

DMAR_DRHD                 mDrhd;
DMAR_RHSA                 mRhsa;

DMAR_ATSR* GetDmarAtsrTablePointer (
  VOID
  )
{
  DMAR_ATSR* pAtsr = NULL;

  pAtsr = &mAtsr10nm;

  return pAtsr;
}


/**
  Enable VT-d interrupt remapping.

  This function should be called late at ReadyToBoot event. If called in AcpiVtdTablesInstall()
  would hang in CpuDeadLoop() because of timeout when waiting for invalidation commands complete.
**/
VOID
AcpiVtdIntRemappingEnable (
  VOID
  )
{
  EFI_STATUS                  Status;
  UINT64                      *xApicAddr;
  UINT64                      *IRTA;
  UINT64                      *Addr;
  UINT64                      Value=0;
  UINT16                      IRTECount;
  UINT16                      Count;
  UINT64                      IRTEValue;
  UINT8                       RemapEng;
  UINT8                       RemapEngCount;
  EFI_CPUID_REGISTER          CpuidRegisters;
  UINT32                      VtdBarAddress;
  UINT8                       Stack;
  VTD_SUPPORT_INSTANCE        *DmarPrivateData;
  DMA_REMAP_PROTOCOL          *DmaRemap = NULL;

  static volatile UINT64      TempQWord[MaxIIO] = {0};

  DYNAMIC_SI_LIBARY_PROTOCOL2  *DynamicSiLibraryProtocol2 = NULL;

  Status = gBS->LocateProtocol (&gDynamicSiLibraryProtocol2Guid, NULL, (VOID **) &DynamicSiLibraryProtocol2);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return;
  }

  Status = gBS->LocateProtocol (&gDmaRemapProtocolGuid, NULL, (VOID **) &DmaRemap);
  if (EFI_ERROR (Status) || !DmaRemap->VTdSupport || !DmaRemap->InterruptRemap) {

    DEBUG ((DEBUG_INFO, "[VTD] %a disabled\n",
            (DmaRemap != NULL && DmaRemap->VTdSupport) ? "Interrupt remapping" : "Virtualization Technology for Directed I/O"));
    return;
  }
  ASSERT (mIioUds2);

  IRTEValue = 00;
  RemapEng = 0;
  RemapEngCount = mIioUds2->IioUdsPtr->PlatformData.numofIIO;
  DmarPrivateData = VTD_SUPPORT_INSTANCE_FROM_THIS (DmaRemap);

  if (RemapEngCount > NELEMENTS (TempQWord)) {
    DEBUG ((DEBUG_ERROR, "[ACPI](DMAR) ERROR: Number of IIO exceed internal table (%d > %d)\n", RemapEngCount, NELEMENTS (TempQWord)));
    RemapEngCount = NELEMENTS (TempQWord);
  }

  //
  // Xapic tables update
  //
  IRTECount = 16 * 24;    // Total 24 IRTE entries with 128 bits each.
  //
  // Allocate 4K alligned space for IRTE entries  Added extra space of 500 bytes.
  //
  Status = gBS->AllocatePool (EfiACPIReclaimMemory, IRTECount + 0x1500, (VOID **) &xApicAddr);
  ASSERT_EFI_ERROR (Status);

  //
  // Allocate IRT - Allocate zero-initialized, 4KB aligned, 4KB memory for interrupt-remap-table and mark this memory as "ACPI Reclaim Memory"
  //
  xApicAddr = (UINT64 *)((UINT64)xApicAddr & (~0xFFF));
  ZeroMem (xApicAddr, IRTECount +0x1500);

  //
  // 1. Program IRTE - Initialize the interrupt-remap-table as follows: (this table will be shared by all VT-d units)
  //
  for (Count = 0; Count < 24; Count++)  {

    IRTEValue = 00;
    if (Count == 0) {
      IRTEValue = (7 << 05) + 03;    // Preset flag set, Ext int enabled, FPD set
    }

    AsmCpuid (
      CPUID_EXTENDED_TOPOLOGY,
      &CpuidRegisters.RegEax,
      &CpuidRegisters.RegEbx,
      &CpuidRegisters.RegEcx,
      &CpuidRegisters.RegEdx
      );
    IRTEValue |= (UINT64)CpuidRegisters.RegEdx << 32;    // Destination Processor Apic ID

    *(volatile UINT64 *)((UINT64)xApicAddr + (Count * 16))= IRTEValue;

    //
    // Perform a CLFLUSH instruction for each cachline in this 4KB memory to ensure that updates to the interrupt-remap-table are visible in memory
    //
    AsmFlushCacheLine ((VOID *)((UINT64)xApicAddr + (Count * 16)));
  }
  //
  // 3. Program the VT-D remap engines
  //
  for (RemapEng = 0; RemapEng < RemapEngCount; RemapEng++) {
    for (Stack = 0; Stack < MAX_IIO_STACK; Stack++) {
      //
      // Check for valid stack
      //
      if (!(mIioUds2->IioUdsPtr->PlatformData.CpuQpiInfo[RemapEng].stackPresentBitmap & (1 << Stack))) {
        continue;
      }
      VtdBarAddress = DynamicSiLibraryProtocol2->GetVtdBar (RemapEng, Stack);
      if (VtdBarAddress) {
        //
        // 2. For each VT-d unit in the platform, allocate and initialize the invalidation queue/commands as follows
        //

        //
        // Allocate memory for the queued invalidation.
        //
        Status = gBS->AllocatePool (EfiACPIReclaimMemory, 0x1000 + 0x1000, (VOID **) &Addr);
        if (EFI_ERROR (Status)) {
          ASSERT (FALSE);
          return;
        }
        ZeroMem (Addr, 0x1000 + 0x1000);
        Addr = (UINT64 *)((UINT64)Addr & (~0xFFF));

        //
        // Submit two descriptors to the respective VT-d unit's invalidation queue as follows:
        // Program 1st descriptor in invalidation-queue as Interrupt-Entry-Cache Invalidation Descriptor
        // with G (Granularity) field Clear
        //
        Addr[0] = 0x04;     // Interrupt Entry Cache Invalidate Descriptor
        Addr[1] = 0x00;

        //
        // Program 2nd descriptor in invalidation-queue as Invalidation-Wait-Descriptor as follows:          +Status-Data=1
        // +Status-Address=address of variable tmp[unit +SW=1 +FN=1 +IF=0
        //

        Addr[2] = ((UINT64)1 << 32) + (06 << 04) + 05;      // Invalidation Wait Descriptor

        TempQWord[RemapEng] = 00;
        Addr[3] = (UINTN)&TempQWord[RemapEng];    // Status Address [63:2] bits[127:65]

        //
        // 3. Program the IRTA register to point to the IRT table.
        // For each VT-d unit in the platform, program interrupt-remap-table address and enable extended-interrupt-mode as follows
        //
        IRTA  = (UINT64 *)((UINT64)VtdBarAddress + R_VTD_IRTA_REG);
        Value = *(volatile UINT32 *)((UINT64)VtdBarAddress + R_VTD_GSTS_REG);
        //
        // *(volatile UINT64*)IRTA = 04  + 0x800 + (UINT64)xApicAddr ;   // [0:3] size = 2 Power (X+1). Bit11 =1 Xapic mode Bit[12:63] address
        //
        if ((DmarPrivateData->Dmar->Flags & EFI_ACPI_DMAR_FLAGS_X2APIC_OPT_OUT) != 0) {
          *(volatile UINT64*)IRTA = 07 + (UINT64)xApicAddr ;   // [0:3] size = 2 Power (X+1). Bit11 =1 Xapic mode Bit[12:63] address
          *(volatile UINT32 *)((UINT64)VtdBarAddress+ R_VTD_GCMD_REG) = (UINT32)(Value | BIT23);
        } else {
          *(volatile UINT64*)IRTA = 07  + 0x800 + (UINT64)xApicAddr ;   // [0:3] size = 2 Power (X+1). Bit11 =1 Xapic mode Bit[12:63] addrerss
        }
        //
        // b. Set SIRTP in the command register.
        //
        Count = 0x1000;
        *(volatile UINT32 *)((UINT64)VtdBarAddress+ R_VTD_GCMD_REG) = (UINT32)(Value | BIT24);

        //
        // Wait till the status bit is set indicating the completion of the SIRTP.
        //
        while (Count)  {
          Count--;
          Value = *(volatile UINT32 *)((UINT64)VtdBarAddress + R_VTD_GSTS_REG);
          if (Value & BIT24) {
            break;
          }
        }
        if (Count == 0) {
          ASSERT(FALSE);
          CpuDeadLoop ();
        }
        *(volatile UINT64 *)((UINT64)VtdBarAddress+ R_VTD_IQA_REG) = (UINT64)Addr;
      } // End of if (VtdBarAddress)
    } // End of for (Stack = 0; Stack < MAX_IIO_STACK; Stack++)
  }

  for (RemapEng = 0; RemapEng < RemapEngCount; RemapEng++) {

    for (Stack = 0; Stack < MAX_IIO_STACK; Stack++) {

      //
      // Check for valid stack
      //
      if (!(mIioUds2->IioUdsPtr->PlatformData.CpuQpiInfo[RemapEng].stackPresentBitmap & (1 << Stack))) {
        continue;
      }
      VtdBarAddress = DynamicSiLibraryProtocol2->GetVtdBar (RemapEng, Stack);
      if (VtdBarAddress) {

        //
        // 4. For each VT-d unit in the platform, setup invalidation-queue base registers and enable invalidation as follows
        // Initialize a single descriptor which invalidates all the interrupt entries.
        // IQA register write (zeros IQH and IQT)
        //

        //
        // Enable queued invalidation in the command register.
        //
        Count = 0x1000;
        Value = *(volatile UINT32 *)((UINT64)VtdBarAddress+ R_VTD_GSTS_REG);
        *(volatile UINT32 *)((UINT64)VtdBarAddress + R_VTD_GCMD_REG) = (UINT32)(Value | BIT26);

        while (Count)  {
          Count--;
          Value = *(volatile UINT32 *)((UINT64)VtdBarAddress+ R_VTD_GSTS_REG);
          if( Value & BIT26) {
            break;
          }
        }
        if (Count == 0) {
          ASSERT(FALSE);
          CpuDeadLoop ();
        }

        //
        // Start invalidations, program the IQT register
        // Write the invalidation queue tail (IQT_REG) register as follows to indicate to hardware two descriptors are submitted:
        // +Bits 63:19 are 0 +Bits 18:4 gets value of 2h +Bits 3:0 are 0
        //

        *(volatile UINT64 *)((UINT64)VtdBarAddress + R_VTD_IQT_REG) = (02 << 04); // Set tail to 02
      } // End of if (VtdAddress)
    } //End of for (Stack = 0; Stack < MAX_IIO_STACK; Stack++)
  }

  for (RemapEng = 0; RemapEng < RemapEngCount; RemapEng++) {

    for (Stack = 0; Stack < MAX_IIO_STACK; Stack++) {

      if (!(mIioUds2->IioUdsPtr->PlatformData.CpuQpiInfo[RemapEng].stackPresentBitmap & (1 << Stack))) {
        continue;  // Skip invalid stacks
      }
      VtdBarAddress = DynamicSiLibraryProtocol2->GetVtdBar (RemapEng, Stack);
      if (VtdBarAddress) {
        //
        // 5. For each VT-d unit in the platform, wait for invalidation completion, and enable interrupt remapping as follows
        // Wait till the previously submitted invalidation commands are completed as follows
        // Poll on the variable tmp[unit] in memory, until its value is 1h.
        //
        Count = 0x1000;
        while (Count)  {
          Count--;
          Value = TempQWord[RemapEng];
          if (Value & 01) {
            break;
          }
        }
        if (Count == 0) {
          ASSERT(FALSE);
          CpuDeadLoop ();
        }
      } // End of if VtdBarAddress
    } //End of for (Stack = 0; Stack < MAX_IIO_STACK; Stack++)
  }

  //
  // 5. Enable external interrupts in the IOAPIC RTE entry 0
  //
  *(volatile UINT32 *)((UINT64)PCH_IOAPIC_ADDRESS)        = 0x10;
  *(volatile UINT32 *)((UINT64)PCH_IOAPIC_ADDRESS + 0x10) = 0x00; // Set index to the IRTE0

  *(volatile UINT32 *)((UINT64)PCH_IOAPIC_ADDRESS)        = 0x10+1;
  *(volatile UINT32 *)((UINT64)PCH_IOAPIC_ADDRESS + 0x10) = 0x10000;// Set Remap enable bit
}


/**
  Build DRHD entry into ACPI DMAR table for specific stack.
  Include IOxAPIC, PCIExpress ports, and CBDMA if C-STACK.

  @param DmaRemap        - pointer to DMA remapping protocol
  @param IioIndex        - IIO index to be processed
  @param Stack           - stack index to be processed
  @param DevScope        - buffer for device scope data structure
  @param PciNode         - buffer for PCI node data structure
  @param PciRootBridgePtr- pointer to PciRootBridgeIo protocol for PCI access

  @retval EFI_SUCCESS - DRHD entry built successfully
**/
EFI_STATUS
BuildDRHDForStack (
  DMA_REMAP_PROTOCOL              *DmaRemap,
  UINT8                           IioIndex,
  UINT8                           Stack,
  DEVICE_SCOPE                    *DevScope,
  PCI_NODE                        *PciNode,
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgePtr,
  UINT8                            ApicIndex
  )
{
  EFI_STATUS                  Status = EFI_SUCCESS;
  UINT8                       Bus;
  UINT8                       Dev;
  UINT8                       Func;
  UINT8                       DevIndex;
  UINT8                       PciNodeIndex;
  UINT8                       PortIndex;
  UINT8                       MaxPortNumberPerSocket;
  UINT8                       CBIndex;
  UINT32                      VtdBase;
  UINT32                      VidDid;
  DMAR_ATSR                   *pAtsr = NULL;
  DYNAMIC_SI_LIBARY_PROTOCOL2  *DynamicSiLibraryProtocol2 = NULL;

  Status = gBS->LocateProtocol (&gDynamicSiLibraryProtocol2Guid, NULL, (VOID **) &DynamicSiLibraryProtocol2);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return EFI_NOT_FOUND;
  }

  ASSERT (mIioUds2);
  if (Stack > MAX_IIO_STACK){
    return EFI_UNSUPPORTED;
  }
  if (PciRootBridgePtr == NULL) {
    ASSERT (!(PciRootBridgePtr == NULL));
    return EFI_INVALID_PARAMETER;
  }

  mDrhd.Flags = 0;  // all non-legacy stack has INCLUDE_ALL flag cleared

  VtdBase = DynamicSiLibraryProtocol2->GetVtdBar (IioIndex, Stack);

  if (VtdBase == 0) {
    return EFI_UNSUPPORTED;
  }

  mDrhd.RegisterBase = VtdBase;

  DevIndex                      = 00;
  PciNodeIndex                  = 00;
  mDrhd.DeviceScopeNumber       = 00;
  ZeroMem (DevScope, MEMORY_SIZE * sizeof (DEVICE_SCOPE));
  ZeroMem (PciNode, MEMORY_SIZE * sizeof (PCI_NODE));

  //
  // DRHD - CBDMA entry
  //
  if (Stack == IIO_STACK0) {

    for (CBIndex = 0; CBIndex <= 7; CBIndex++) {

      DevScope[DevIndex].DeviceType         = EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT;
      DevScope[DevIndex].EnumerationID      = 00;
      DevScope[DevIndex].StartBusNumber     = mCpuCsrAccessVarPtr ->StackBus[IioIndex][IIO_STACK0];
      DevScope[DevIndex].PciNode            = &PciNode[PciNodeIndex];

      PciNode[PciNodeIndex].Device = IOAT_DEVICE_NUM_10NM;
      PciNode[PciNodeIndex].Function        = CBIndex;
      DEBUG ((DEBUG_INFO, "[ACPI](DMAR) [%d.%d] Build DRHD CBDMA: Type %d, EnumId %d, StartBus 0x%x, PciNode %02X.%X\n",
              IioIndex, Stack,
              DevScope[DevIndex].DeviceType, DevScope[DevIndex].EnumerationID, DevScope[DevIndex].StartBusNumber,
              DevScope[DevIndex].PciNode->Device, DevScope[DevIndex].PciNode->Function));
      DevIndex++;
      PciNodeIndex++;
      PciNode[PciNodeIndex].Device    = (UINT8) -1;
      PciNode[PciNodeIndex].Function  = (UINT8) -1;
      PciNodeIndex++;

      mDrhd.DeviceScopeNumber++;
    } // End of for for(CBIndex = 0; CBIndex <= 07; CBIndex++)
  }

  //
  // DRHD - PCI-Ex ports
  //
  pAtsr = GetDmarAtsrTablePointer ();
  MaxPortNumberPerSocket = DynamicSiLibraryProtocol2->GetMaxPortPerSocket (IioIndex);
  for (PortIndex = 1; PortIndex < MaxPortNumberPerSocket; PortIndex++) {

    if (DynamicSiLibraryProtocol2->GetStackPerPort (IioIndex, PortIndex) != Stack) {
      continue;
    }
    Bus = DynamicSiLibraryProtocol2->GetSocketPortBusNum (IioIndex, PortIndex);
    Dev = 0;
    Func = 0;
    if (pAtsr != NULL) {
      Dev = pAtsr->DeviceScope[PortIndex].PciNode->Device;
      Func = pAtsr->DeviceScope[PortIndex].PciNode->Function;
    }
    if (DynamicSiLibraryProtocol2->IioNtbIsEnabled (IioIndex, PortIndex, &Dev, &Func)) {

      DevScope[DevIndex].DeviceType = EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT;
    } else {
      DevScope[DevIndex].DeviceType = EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE;
    }
    //
    // Skip root ports which do not respond to PCI configuration cycles.
    //
    VidDid = 0;
    Status = PciRootBridgePtr->Pci.Read (
                PciRootBridgePtr,
                EfiPciWidthUint32,
                EFI_PCI_ADDRESS (Bus, Dev, Func, 0),
                1,
                &VidDid);
    if (EFI_ERROR (Status) || VidDid == 0xffffffff) {

      DEBUG ((DEBUG_INFO, "[ACPI](DMAR) [%d.%d p%d] %02X:%02X:%02X.%d Hidden (%X) - skip\n",
              IioIndex, Stack, PortIndex,
              mIioUds2->IioUdsPtr->PlatformData.CpuQpiInfo[IioIndex].PcieSegment,
              Bus, Dev, Func, VidDid));
      continue;
    }
    if (DynamicSiLibraryProtocol2->IioVmdPortIsEnabled (IioIndex, PortIndex) || DynamicSiLibraryProtocol2->GetCurrentPXPMap (IioIndex, PortIndex) == 0) {

      DEBUG ((DEBUG_INFO, "[ACPI](DMAR) [%d.%d p%d] %a - skip\n", IioIndex, Stack, PortIndex,
              (DynamicSiLibraryProtocol2->GetCurrentPXPMap (IioIndex, PortIndex) == 0) ? "Link width not set" : "Dummy VMD function"));
      continue;
    }
    DevScope[DevIndex].EnumerationID      = 00;
    DevScope[DevIndex].StartBusNumber     = Bus;
    DevScope[DevIndex].PciNode            = &PciNode[PciNodeIndex];
    PciNode[PciNodeIndex].Device          = Dev;
    PciNode[PciNodeIndex].Function        = Func;
    DEBUG ((DEBUG_INFO, "[ACPI](DMAR) [%d.%d p%d] Build DRHD PCI: Type %d, EnumId %d, StartBus 0x%x, PciNode %02X.%X\n",
            IioIndex, Stack, PortIndex,
            DevScope[DevIndex].DeviceType, DevScope[DevIndex].EnumerationID, DevScope[DevIndex].StartBusNumber,
            DevScope[DevIndex].PciNode->Device, DevScope[DevIndex].PciNode->Function));
    DevIndex++;
    PciNodeIndex++;
    PciNode[PciNodeIndex].Device    = (UINT8) -1;
    PciNode[PciNodeIndex].Function  = (UINT8) -1;
    PciNodeIndex++;

    mDrhd.DeviceScopeNumber++;
  } // for (PortIndex...)

  Status = DynamicSiLibraryProtocol2->IioVmdGetPciLocation (IioIndex, Stack,
                                 &PciNode[PciNodeIndex].Device, &PciNode[PciNodeIndex].Function);
  if (!EFI_ERROR (Status)) {
    //
    // VMD is enabled in this stack, expose VMD PCI device in DMAR for DMA remapping.
    //
    DevScope[DevIndex].DeviceType         = EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT;
    DevScope[DevIndex].EnumerationID      = 00;
    DevScope[DevIndex].StartBusNumber     = mCpuCsrAccessVarPtr->StackBus[IioIndex][Stack];
    DevScope[DevIndex].PciNode            = &PciNode[PciNodeIndex];
    DEBUG ((DEBUG_INFO, "[ACPI](DMAR) [%d.%d] Build DRHD VMD: Type %d, EnumId %d, StartBus 0x%x, PciNode %02X.%X\n",
            IioIndex, Stack,
            DevScope[DevIndex].DeviceType, DevScope[DevIndex].EnumerationID, DevScope[DevIndex].StartBusNumber,
            DevScope[DevIndex].PciNode->Device, DevScope[DevIndex].PciNode->Function));
    DevIndex++;
    PciNodeIndex++;
    PciNode[PciNodeIndex].Device = (UINT8)-1;
    PciNode[PciNodeIndex].Function = (UINT8)-1;
    PciNodeIndex++;

    mDrhd.DeviceScopeNumber++;
  }

  DmaRemap->InsertDmaRemap (DmaRemap, DrhdType, &mDrhd);

  return EFI_SUCCESS;
}


EFI_STATUS
ReportDmar (
  IN DMA_REMAP_PROTOCOL           *DmaRemap
  )
{
  EFI_STATUS                      Status = EFI_SUCCESS;
  UINT8                           SocketIndex, Bus;
  UINT8                           Dev, Func;
  UINT8                           DevIndex;
  UINT8                           PciNodeIndex;
  UINT8                           PciPortIndex;
  UINT8                           MaxPortNumberPerSocket;
  UINT64                          VtdMmioExtCap;
  UINT32                          VtdBase;
  VTD_SUPPORT_INSTANCE            *DmarPrivateData;
  UINT16                          NumberOfHpets;
  UINT16                          HpetCapIdValue;
  DEVICE_SCOPE                    *DevScope;
  PCI_NODE                        *PciNode;
  EFI_PHYSICAL_ADDRESS            Pointer;
  UINT32                          AlignedSize;
  UINT32                          NumberofPages;
  BOOLEAN                         IntrRemapSupport;
  EFI_CPUID_REGISTER              CpuidRegisters;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgePtr;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL *PciRootBridgeTab[MAX_SOCKET];
  UINT32                          VidDid;
  UINT8                           Index;
  UINT8                           Stack = 0;
  UINT8                           FirstRun = 0;
  VOID                            *HobPtr;
  PCH_INFO_HOB                    *PchInfoHob;
  DMAR_ATSR                       *pAtsr = NULL;
  UINT8                           ApicIndex = 1;
  UINTN                           HandleCount;
  EFI_HANDLE                      *HandleBuffer;
  DYNAMIC_SI_LIBARY_PROTOCOL2     *DynamicSiLibraryProtocol2 = NULL;

  Status = gBS->LocateProtocol (&gDynamicSiLibraryProtocol2Guid, NULL, (VOID **) &DynamicSiLibraryProtocol2);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return FALSE;
  }

  HobPtr = GetFirstGuidHob (&gPchInfoHobGuid);
  if (HobPtr == NULL) {
    ASSERT (HobPtr != NULL);
    return EFI_INVALID_PARAMETER;
  }

  PchInfoHob = (PCH_INFO_HOB*) GET_GUID_HOB_DATA (HobPtr);
  if (PchInfoHob == NULL) {
    ASSERT (PchInfoHob != NULL);
    return EFI_INVALID_PARAMETER;
  }

  ASSERT (mIioUds2);

  DmarPrivateData = VTD_SUPPORT_INSTANCE_FROM_THIS (DmaRemap);
  //
  // Get DMAR_HOST_ADDRESS_WIDTH from CPUID.(EAX=80000008h) return the Phyical Address
  // Size in the EAX register. EAX[7:0]
  // Sync with Brickland code  DMAR_HOST_ADDRESS_WIDTH 45 = 46 - 1
  //
  AsmCpuid (
    CPUID_VIR_PHY_ADDRESS_SIZE,
    &CpuidRegisters.RegEax,
    &CpuidRegisters.RegEbx,
    &CpuidRegisters.RegEcx,
    &CpuidRegisters.RegEdx
  );

  DmarPrivateData->Dmar->HostAddressWidth = (UINT8)((CpuidRegisters.RegEax & 0xFF)-1);
  DmarPrivateData->Dmar->Flags = 0; // INTR_REMAP

  //
  // Locate PCI root bridge I/O protocol, for confirming PCI functions respond
  // to PCI configuration cycles.
  //
  ZeroMem (&PciRootBridgeTab[0], sizeof(PciRootBridgeTab));

  Status = gBS->LocateHandleBuffer (
                ByProtocol,
                &gEfiPciRootBridgeIoProtocolGuid,
                NULL,
                &HandleCount,
                &HandleBuffer
                );

  ASSERT_EFI_ERROR (Status);

  for (Index = 0; Index < HandleCount; Index ++) {
    Status = gBS->HandleProtocol (
                  HandleBuffer[Index],
                  &gEfiPciRootBridgeIoProtocolGuid,
                  (VOID **) &PciRootBridgePtr
                  );

    ASSERT_EFI_ERROR (Status);

    PciRootBridgeTab[PciRootBridgePtr->SegmentNumber] = PciRootBridgePtr;
  }
  FreePool (HandleBuffer);

  //
  // Allocate memory to DevScope structures
  //
  Status = gBS->AllocatePool (EfiACPIMemoryNVS, MEMORY_SIZE * sizeof (DEVICE_SCOPE), (VOID **) &DevScope);
  ASSERT_EFI_ERROR (Status);

  Status = gBS->AllocatePool (EfiACPIMemoryNVS, MEMORY_SIZE * sizeof (PCI_NODE), (VOID **) &PciNode);
  ASSERT_EFI_ERROR (Status);

  for (Index = 1; Index <= MAX_SOCKET; Index++) {
    //
    // VT-d specification request that DHRD entry 0 should be the latest entry of the DMAR table.
    // To accomplish this, the following check will ensure that latest entry will be the one related to Socket 0.
    //
    if (Index == MAX_SOCKET) {
      SocketIndex = 0;
    } else {
      SocketIndex = Index;
    }

    if (SocketIndex >= MAX_SOCKET) {
      return EFI_INVALID_PARAMETER;
    }

    if (!DynamicSiLibraryProtocol2->SocketPresent (SocketIndex)) {
      continue;
    }

    if (mIioUds2->IioUdsPtr->PlatformData.CpuQpiInfo[SocketIndex].PcieSegment >= MAX_SOCKET) {
      return EFI_INVALID_PARAMETER;
    }
    PciRootBridgePtr = PciRootBridgeTab[mIioUds2->IioUdsPtr->PlatformData.CpuQpiInfo[SocketIndex].PcieSegment];

    Stack = IIO_STACK0;
    VtdBase = DynamicSiLibraryProtocol2->GetVtdBar (SocketIndex, Stack);

    DevIndex                      = 00;
    PciNodeIndex                  = 00;

    mDrhd.Signature               = DRHD_SIGNATURE;
    mDrhd.SegmentNumber           = mIioUds2->IioUdsPtr->PlatformData.CpuQpiInfo[SocketIndex].PcieSegment;
    mDrhd.DeviceScopeNumber       = 00;
    mDrhd.DeviceScope             = DevScope;
    mDrhd.RegisterBase            = VtdBase;
    ZeroMem (DevScope, MEMORY_SIZE * sizeof (DEVICE_SCOPE));
    ZeroMem (PciNode, MEMORY_SIZE * sizeof (PCI_NODE));

    VtdMmioExtCap = *(volatile UINT64*)((UINTN)VtdBase + R_VTD_EXT_CAP_LOW);

    //
    // Check Interrupt Remap support.
    //
    IntrRemapSupport = FALSE;
    if (VtdMmioExtCap & INTRREMAP) {
      IntrRemapSupport = TRUE;
    }
    DEBUG ((DEBUG_INFO, "[ACPI](DMAR) [%d.%d] VT-d base 0x%X, ExtCap=0x%X\n",
            SocketIndex, Stack, VtdBase, VtdMmioExtCap));

    if (SocketIndex == 0) {
      ApicIndex = 1;
      //
      // DRHD - Legacy IOH
      //
      // Build DRHD on IIO0 - Stack1 to Stack5, not include C-STACK
      //
      for (Stack = 1; Stack < MAX_IIO_STACK; Stack++) {

        if (!DynamicSiLibraryProtocol2->IfStackPresent (SocketIndex, Stack)) {  // Skip invalid stack
          continue;
        }
        BuildDRHDForStack (DmaRemap, SocketIndex, Stack, DevScope, PciNode, PciRootBridgePtr, ApicIndex);
        ApicIndex++;
      }

      Stack = IIO_STACK0;

      //
      // Re-initialize DRHD template for DRHD entry in legacy socket C-STACK
      //
      DevIndex                      = 00;
      PciNodeIndex                  = 00;
      mDrhd.DeviceScopeNumber       = 00;
      mDrhd.RegisterBase            = DynamicSiLibraryProtocol2->GetVtdBar (SocketIndex, Stack);
      ZeroMem (DevScope, MEMORY_SIZE * sizeof (DEVICE_SCOPE));
      ZeroMem (PciNode, MEMORY_SIZE * sizeof (PCI_NODE));

      mDrhd.Flags = 1;

      DEBUG ((DEBUG_INFO, "[ACPI](DMAR) InterruptRemap is %aabled (%d & %d)\n",
              (DmaRemap->InterruptRemap && IntrRemapSupport) ? "en" : "dis", DmaRemap->InterruptRemap, IntrRemapSupport));
      if (DmaRemap->InterruptRemap && IntrRemapSupport) {

        DmarPrivateData->Dmar->Flags = 0x01; // INTR_REMAP

        if (DmaRemap->X2ApicOptOut) {
          DmarPrivateData->Dmar->Flags |= 0x02; // X2APIC_OPT_OUT
        }
        //
        // PCH - IOAPIC
        // This information will be provided by PCH side
        // Currently is a hard-coded temporal solution to set:
        // Bus = 0; Device and Function (together) = 0xF7;
        // This is the value that is stored in IBDF register:
        //#define V_P2SB_CFG_IBDF_BUS                        0
        //#define V_P2SB_CFG_IBDF_DEV                        30
        //#define V_P2SB_CFG_IBDF_FUNC                       7
        //
        DevScope[DevIndex].DeviceType = EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_IOAPIC;
        DevScope[DevIndex].EnumerationID = PCH_IOAPIC_ID; // PCH team needs confirm this value. (This value affects VTd functionality?)
        DevScope[DevIndex].StartBusNumber = (UINT8)PchInfoHob->IoApicBusNum;
        DevScope[DevIndex].PciNode = &PciNode[PciNodeIndex];

        PciNode[PciNodeIndex].Device = (UINT8)PchInfoHob->IoApicDevNum;
        PciNode[PciNodeIndex].Function = (UINT8)PchInfoHob->IoApicFuncNum;
        DEBUG ((DEBUG_INFO, "[ACPI](DMAR) [%d.%d] Build DRHD PCH IOAPIC: Type %d, EnumId %d, StartBus 0x%x, PciNode %02X.%X\n",
                SocketIndex, Stack,
                DevScope[DevIndex].DeviceType, DevScope[DevIndex].EnumerationID, DevScope[DevIndex].StartBusNumber,
                DevScope[DevIndex].PciNode->Device, DevScope[DevIndex].PciNode->Function));
        DevIndex++;
        PciNodeIndex++;
        PciNode[PciNodeIndex].Device = (UINT8)-1;
        PciNode[PciNodeIndex].Function = (UINT8)-1;
        PciNodeIndex++;

        mDrhd.DeviceScopeNumber++;

        HpetCapIdValue = *(UINT16 *)(UINTN)(HPET_BLOCK_ADDRESS);
        NumberOfHpets = (HpetCapIdValue >> 0x08) & 0x1F;  // Bits [8:12] contains the number of Hpets

        if (NumberOfHpets && (NumberOfHpets != 0x1f) &&
            (*((volatile UINT32 *)(UINTN)(HPET_BLOCK_ADDRESS + 0x100)) & BIT15)) {

            DevScope[DevIndex].DeviceType = EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_MSI_CAPABLE_HPET;
            DevScope[DevIndex].EnumerationID = 00; //Hard-coded
            DevScope[DevIndex].StartBusNumber = (UINT8)PchInfoHob->HpetBusNum;
            DevScope[DevIndex].PciNode = &PciNode[PciNodeIndex];
            PciNode[PciNodeIndex].Device = (UINT8)PchInfoHob->HpetDevNum;
            PciNode[PciNodeIndex].Function = (UINT8)PchInfoHob->HpetFuncNum;
            DEBUG ((DEBUG_INFO, "[ACPI](DMAR) [%d.%d] Build DRHD HPET: Type %d, EnumId %d, StartBus 0x%x, PciNode %02X.%X\n",
                    SocketIndex, Stack,
                    DevScope[DevIndex].DeviceType, DevScope[DevIndex].EnumerationID, DevScope[DevIndex].StartBusNumber,
                    DevScope[DevIndex].PciNode->Device, DevScope[DevIndex].PciNode->Function));
            DevIndex++;
            PciNodeIndex++;
            PciNode[PciNodeIndex].Device = (UINT8)-1;
            PciNode[PciNodeIndex].Function = (UINT8)-1;
            PciNodeIndex++;

          mDrhd.DeviceScopeNumber++;
        }
      } // DmaRemap->InterruptRemap

      DEBUG ((DEBUG_INFO, "[ACPI](DMAR) DMA_CTRL_PLATFORM_OPT_IN_FLAG is %aabled\n", (DmaRemap->DmaCtrlOptIn) ? "En" : "Dis"));
      if (DmaRemap->DmaCtrlOptIn) {

        DmarPrivateData->Dmar->Flags |= 0x04; // DMA_CTRL_PLATFORM_OPT_IN_FLAG
      }
      DmaRemap->InsertDmaRemap (DmaRemap, DrhdType, &mDrhd);

    } else { // End of if (IioSocketId == 0)

      if (FirstRun == 0) {
        ApicIndex = 0;
        for (Stack = 0; Stack < MAX_IIO_STACK; Stack++) {
          //
          // Skip not valid stack
          //
          if (!DynamicSiLibraryProtocol2->IfStackPresent (0 ,Stack)) {
            continue;
          }
          ApicIndex++;
        }
        FirstRun = 1;
      }
      //
      // Build DRHD on IIO1 - Stack0 to Stack5
      //
      for (Stack = 0; Stack < MAX_IIO_STACK; Stack++) {
        //
        // Skip not valid stack
        //
        if (!DynamicSiLibraryProtocol2->IfStackPresent (SocketIndex,Stack)) {
          continue;
        }
        BuildDRHDForStack (DmaRemap, SocketIndex, Stack, DevScope, PciNode, PciRootBridgePtr, ApicIndex);
        ApicIndex++;
      } //for( StackIndex=0; StackIndex<MAX_IIO_STACK ; StackIndex++) {
    } // End of if (IioSocketId == 0)

  } // End of for ( Index = 1; Index <= MAX_SOCKET; Index++)

  //
  // ATSR
  //
  pAtsr = GetDmarAtsrTablePointer ();
  if (DmaRemap->ATS) {
    for (SocketIndex = 0; SocketIndex < MAX_SOCKET; SocketIndex++) {

      DEBUG((DEBUG_ERROR, "T_TEST: Build ATSR SocketIndex=%d.\n", SocketIndex));
      DEBUG((DEBUG_ERROR, "        IIO_resource.valid=%d.\n", mIioUds2->IioUdsPtr->PlatformData.IIO_resource[SocketIndex].Valid));

      if (SocketIndex >= MAX_SOCKET) {
        return EFI_INVALID_PARAMETER;
      }

      if (!DynamicSiLibraryProtocol2->SocketPresent (SocketIndex)) {
        continue;
      }

      if (mIioUds2->IioUdsPtr->PlatformData.CpuQpiInfo[SocketIndex].PcieSegment >= MAX_SOCKET) {
        return EFI_INVALID_PARAMETER;
      }
      PciRootBridgePtr = PciRootBridgeTab[mIioUds2->IioUdsPtr->PlatformData.CpuQpiInfo[SocketIndex].PcieSegment];

      PciNodeIndex            = 00;
      DevIndex                = 00;

      ZeroMem (DevScope, MEMORY_SIZE * sizeof (DEVICE_SCOPE));
      ZeroMem (PciNode, MEMORY_SIZE * sizeof (PCI_NODE));

      if (pAtsr != NULL) {
        pAtsr->Signature = ATSR_SIGNATURE;
        pAtsr->Flags = 00;
        pAtsr->SegmentNumber = mIioUds2->IioUdsPtr->PlatformData.CpuQpiInfo[SocketIndex].PcieSegment;
        pAtsr->DeviceScopeNumber = 00;
        pAtsr->DeviceScope = DevScope;
        pAtsr->ATSRPresentBit = (UINT32)-1;   // Not useful really Backwards project compatability (remove it later)
      }

      //
      // Loop From Port 1 to 15 for Legacy IOH and 0 to 15 for Non-Legacy IOH
      //
      MaxPortNumberPerSocket = DynamicSiLibraryProtocol2->GetMaxPortPerSocket (SocketIndex);
      for (PciPortIndex = 1; PciPortIndex < MaxPortNumberPerSocket; PciPortIndex++)  {
        //
        // Check device IOTLBs supported or not in VT-d Extended capability register
        //
        Stack = DynamicSiLibraryProtocol2->GetStackPerPort (SocketIndex, PciPortIndex);
        //
        // Check for a valid stack
        //
        if (!(DynamicSiLibraryProtocol2->IfStackPresent (SocketIndex, Stack))) {
          DEBUG ((DEBUG_WARN, "[ACPI](DMAR) [%d.%d p%d] Stack not present\n", SocketIndex, Stack, PciPortIndex));
          continue;
        }

        VtdBase = DynamicSiLibraryProtocol2->GetVtdBar (SocketIndex, Stack);
        if (VtdBase != 0) {

          VtdMmioExtCap = *(volatile UINT64*)((UINTN)VtdBase + R_VTD_EXT_CAP_LOW);
          //
          // ATSR is applicable only for platform supporting device IOTLBs through the VT-d extended capability register
          //
          if ((VtdMmioExtCap & BIT2) != 0) {

            Bus = DynamicSiLibraryProtocol2->GetSocketPortBusNum (SocketIndex,PciPortIndex);
            Dev = 0;
            Func = 0;
            Dev = mDevScopeATSR10nm[PciPortIndex].PciNode->Device;
            Func = mDevScopeATSR10nm[PciPortIndex].PciNode->Function;
            if (DynamicSiLibraryProtocol2->IioNtbIsEnabled (SocketIndex, PciPortIndex, &Dev, &Func)) {
              DevScope[DevIndex].DeviceType = EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT;
            } else {
              DevScope[DevIndex].DeviceType = EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE;
            }
            //
            // Skip root ports which do not respond to PCI configuration cycles.
            //
            VidDid = 0;
            Status = PciRootBridgePtr->Pci.Read (
                      PciRootBridgePtr,
                      EfiPciWidthUint32,
                      EFI_PCI_ADDRESS (Bus, Dev, Func, 0),
                      1,
                      &VidDid);
            if (EFI_ERROR (Status) || VidDid == 0xffffffff) {

              DEBUG ((DEBUG_INFO, "[ACPI](DMAR) [%d.%d p%d] %02X:%02X:%02X.%d Hidden (%X) - skip\n",
                      SocketIndex, Stack, PciPortIndex,
                      mIioUds2->IioUdsPtr->PlatformData.CpuQpiInfo[SocketIndex].PcieSegment,
                      Bus, Dev, Func, VidDid));
              continue;
            }
            if (DynamicSiLibraryProtocol2->IioVmdPortIsEnabled (SocketIndex, PciPortIndex) || DynamicSiLibraryProtocol2->GetCurrentPXPMap (SocketIndex, PciPortIndex) == 0) {

              DEBUG ((DEBUG_INFO, "[ACPI](DMAR) [%d.%d p%d] %a - skip\n", SocketIndex, Stack, PciPortIndex,
                      (DynamicSiLibraryProtocol2->GetCurrentPXPMap (SocketIndex, PciPortIndex) == 0) ? "Link width not set" : "Dummy VMD function"));
              continue;
            }
            DevScope[DevIndex].EnumerationID      = 00;
            DevScope[DevIndex].StartBusNumber     = Bus;
            DevScope[DevIndex].PciNode            = &PciNode[PciNodeIndex];
            PciNode[PciNodeIndex].Device   = Dev;
            PciNode[PciNodeIndex].Function = Func;
            DEBUG ((DEBUG_INFO, "[ACPI](DMAR) [%d.%d p%d] Build DRHD PCI: Type %d, EnumId %d, StartBus 0x%x, PciNode %02X.%X\n",
                    SocketIndex, Stack, PciPortIndex,
                    DevScope[DevIndex].DeviceType, DevScope[DevIndex].EnumerationID, DevScope[DevIndex].StartBusNumber,
                    DevScope[DevIndex].PciNode->Device, DevScope[DevIndex].PciNode->Function));
            DevIndex++;
            PciNodeIndex++;
            PciNode[PciNodeIndex].Device    = (UINT8) -1;
            PciNode[PciNodeIndex].Function  = (UINT8) -1;
            PciNodeIndex++;
            if(pAtsr != NULL){
              pAtsr->DeviceScopeNumber++;
            }
          } // End of if ((VtdMmioExtCap & BIT2) != 0)
        } // End of if VtdBase
      } // for (PciPortIndex...)

      if (pAtsr != NULL){
        if (pAtsr->DeviceScopeNumber) {
          DmaRemap->InsertDmaRemap(DmaRemap, AtsrType, pAtsr);
        }
      }
    } // End of for (RootBridgeLoop = 0; RootBridgeLoop < mIioUds2->IioUdsPtr->PlatformData.numofIIO; RootBridgeLoop++)
  } // End of if (ATS) {

  //
  // RMRR
  //
  AlignedSize  = (MEM_BLK_COUNT * sizeof(MEM_BLK));
  if (AlignedSize % 0x1000) {
    AlignedSize  = ( (MEM_BLK_COUNT * sizeof(MEM_BLK)) & (~0xfff) ) + 0x1000;
  } // aligend to 4k Boundary
  NumberofPages = AlignedSize/0x1000;
  //
  // Allocate memory (below 4GB)
  //
  Pointer = 0xffffffff;
  Status = gBS->AllocatePages (
                   AllocateMaxAddress,
                   EfiReservedMemoryType,
                   NumberofPages,
                   &Pointer // Base address need to be 4K aligned for VT-d RMRR
                   );
  ASSERT_EFI_ERROR (Status);

  if (DmaRemap->VTdSupport) {
    //
    // RMRR
    //
    mRmrr.DeviceScope       = &DevScopeRmrr[0];
    //
    // Calculate the right size of DevScope for mRmrr entry
    //
    mRmrr.DeviceScopeNumber = sizeof(DevScopeRmrr) / sizeof(DEVICE_SCOPE);
    mRmrr.RsvdMemBase       = (UINT64)Pointer;
    mRmrr.RsvdMemLimit      = mRmrr.RsvdMemBase + AlignedSize - 1;
    DEBUG ((DEBUG_INFO, "[ACPI](DMAR) RMRR Base 0x%llX, Limit 0x%llX\n", mRmrr.RsvdMemBase, mRmrr.RsvdMemLimit));
    DmaRemap->InsertDmaRemap (DmaRemap, RmrrType, &mRmrr);
  }
  gBS->FreePool (PciNode);
  gBS->FreePool (DevScope);

  return EFI_SUCCESS;
}


/**
  Install ACPI DMAR table for VT-d.

  This function needs gEfiPciIoProtocolGuid so it can run only after PCI Enumeraion is complete.

  @retval EFI_SUCCESS          DMAR installed successfuly.
  @retval EFI_NOT_FOUND        gEfiPciIoProtocolGuid or gDmaRemapProtocolGuid not found.
  @retval EFI_OUT_OF_RESOURCES Could not allocate resources.
**/
EFI_STATUS
AcpiVtdTablesInstall (
  VOID
  )
{
  EFI_STATUS                  Status;
  EFI_ACPI_TABLE_VERSION      TableVersion;
  DMA_REMAP_PROTOCOL          *DmaRemap;
  UINTN                       TableHandle;
  EFI_ACPI_COMMON_HEADER      *CurrentTable;
  EFI_ACPI_TABLE_PROTOCOL     *AcpiTable;

  UINTN                       HandleCount;
  EFI_HANDLE                  *HandleBuffer;
  UINTN                       Index;
  EFI_PCI_IO_PROTOCOL         *PciIo;
  PCI_TYPE01                  PciConfigHeader;
  UINTN                       Segment;
  UINTN                       Bus;
  UINTN                       Device;
  UINTN                       Function;
  UINT8                       PciExpressOffset;
  UINT32                      AcsOffset;
  UINT16                      PciExpressCapabilityReg;
  UINT8                       AcsCapCount;
  UINT16                      RequiredAcsCap;
  UINT32                      AcsCapRegValue;
  UINT16                      AcsConRegValue;
  USRA_PCIE_ADDR_TYPE         *AcsDevArray;
  USRA_ADDRESS                Address;

  DYNAMIC_SI_LIBARY_PROTOCOL  *DynamicSiLibraryProtocol = NULL;

  Status = gBS->LocateProtocol (&gDynamicSiLibraryProtocolGuid, NULL, (VOID **) &DynamicSiLibraryProtocol);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return EFI_NOT_FOUND;
  }

  PciExpressOffset = 0;
  AcsOffset        = 0;
  AcsCapCount      = 0;
  AcsCapRegValue   = 0;
  AcsConRegValue   = 0;
  RequiredAcsCap   =  ACS_SOURCE_VALIDATION | ACS_P2P_REQUEST_REDIRECT | ACS_P2P_COMPLETION_REDIRECT | ACS_UPSTREAM_FORWARDING;

  //
  // Locate all PciIo protocols
  //
  Status = gBS->LocateHandleBuffer (
               ByProtocol,
               &gEfiPciIoProtocolGuid,
               NULL,
               &HandleCount,
               &HandleBuffer
               );
  if (EFI_ERROR (Status)) {

    DEBUG((DEBUG_ERROR, "[ACPI](DMAR) ERROR: Cannot locate gEfiPciIoProtocolGuid (%r)\n", Status));
    ASSERT (FALSE);
    return Status;
  }
  AcsDevArray = AllocateZeroPool (sizeof (USRA_PCIE_ADDR_TYPE) * HandleCount);
  if (AcsDevArray == NULL) {
    ASSERT (AcsDevArray != NULL);
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < HandleCount; Index ++) {

    gBS->HandleProtocol (HandleBuffer[Index], &gEfiPciIoProtocolGuid, (VOID **) &PciIo);
    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, 0, sizeof(PciConfigHeader) / sizeof(UINT32), &PciConfigHeader);
    if ((PciConfigHeader.Hdr.ClassCode[0] == 0x00 || PciConfigHeader.Hdr.ClassCode[0] == 0x01) && PciConfigHeader.Hdr.ClassCode[1] == 0x04 && PciConfigHeader.Hdr.ClassCode[2] == 0x06) {
      //
      // 060400h or 060401h indicates it's PCI-PCI bridge, get its bus number, device number and function number
      //

      PciIo->GetLocation (PciIo, &Segment, &Bus, &Device, &Function);

      USRA_PCIE_SEG_ADDRESS(Address, UsraWidth16, Segment, Bus, Device, Function, 0);

      if (PciConfigHeader.Hdr.Status == EFI_PCI_STATUS_CAPABILITY) {
        //
        // the bridge support Capability list and offset 0x34 is the pointer to the data structure
        //
        // Detect if PCI Express Device
        //
        Status = LocateCapRegBlock (PciIo, EFI_PCI_CAPABILITY_ID_PCIEXP, &PciExpressOffset, NULL);

        if (Status == EFI_SUCCESS) {
          //
          // this bridge device is a PCI Express bridge
          // Check if it is downstream port of PCIE switch
          //
          Address.Pcie.Offset = PciExpressOffset + EFI_PCI_EXPRESS_CAPABILITY_REGISTER;
          DynamicSiLibraryProtocol->RegisterRead(&Address, &PciExpressCapabilityReg);

          //
          // BIT 7:4 indicate Device/port type, 0110b indicates downstream port of PCI express switch
          //
          if ((PciExpressCapabilityReg & 0x00F0) == 0x60) {
            //
            // it is downstream port of PCI Express switch
            // Look for ACS capability register in PCI express configuration space
            //
            Status = LocatePciExpressCapRegBlock (PciIo, EFI_PCIE_CAPABILITY_ID_ACS, &AcsOffset, NULL);
            DEBUG((DEBUG_ERROR, "ACS capable port is B%x.D%x.F%x - ACS Cap offset - 0x%x\n", Bus, Device, Function, AcsOffset));

            if (Status == EFI_SUCCESS) {
              //
              // Read ACS capability register
              //
              Address.Pcie.Offset = AcsOffset + ACS_CAPABILITY_REGISTER;
              Address.Attribute.AccessWidth = UsraWidth32;
              DynamicSiLibraryProtocol->RegisterRead(&Address, &AcsCapRegValue);
              DEBUG((DEBUG_INFO, "Bus =%x, Device=%x, Function=%x, AcsCapRegValue = %x \n", Bus, Device, Function, AcsCapRegValue));

              if ((AcsCapRegValue & RequiredAcsCap) == RequiredAcsCap) {
                //
                // The PCI express downstream port support ACS, record this port
                //
                AcsDevArray[AcsCapCount].Bus = (UINT32)Bus;
                AcsDevArray[AcsCapCount].Dev = (UINT32)Device;
                AcsDevArray[AcsCapCount].Func = (UINT32)Function;
                AcsDevArray[AcsCapCount].Offset = AcsOffset;
                AcsDevArray[AcsCapCount].Seg = (UINT32)Segment;
                AcsCapCount++;
              }
            }
          }
        }
      }
    }
  }  /// End for

  //
  // Free the Handle buffer
  //
  if (HandleBuffer != NULL) {
    gBS->FreePool (HandleBuffer);
  }

  ASSERT (AcsCapCount <= HandleCount);

  //
  // all PCI express switch downstream ports support ACS and meet the required ACS capabilities
  // for each downstream ports, enable the required Capabilities in ACS control register
  //
  Address.Attribute.AccessWidth = UsraWidth16;
  for (Index = 0; Index < AcsCapCount; Index ++) {
    //
    // Program the corresponding bits in ACS control register
    //
    Address.Pcie = AcsDevArray[Index];
    Address.Pcie.Offset += ACS_CONTROL_REGISTER;
    DynamicSiLibraryProtocol->RegisterRead (&Address, &AcsConRegValue);
    DEBUG ((DEBUG_ERROR, "AcsConRegValue is 0x%x\n", AcsConRegValue));
    AcsConRegValue |= (ACS_SOURCE_VALIDATION_ENABLE | ACS_P2P_REQUEST_REDIRECT_ENABLE | ACS_P2P_COMPLETION_REDIRECT_ENABLE | ACS_UPSTREAM_FORWARDING_ENABLE);
    DEBUG ((DEBUG_ERROR, "After Enable BITs AcsConRegValue is 0x%x\n", AcsConRegValue));
    DynamicSiLibraryProtocol->RegisterWrite (&Address, &AcsConRegValue);
    //
    // report VT-d and other features to OS/VMM, report DMAR and remapping engine to OS/VMM
    //
  }

  //
  // Find the AcpiSupport protocol
  //
  Status = LocateSupportProtocol (&gEfiAcpiTableProtocolGuid, gEfiAcpiTableStorageGuid, (VOID **) &AcpiTable, FALSE);
  ASSERT_EFI_ERROR (Status);

  TableVersion = EFI_ACPI_TABLE_VERSION_2_0;

  Status = gBS->LocateProtocol (&gDmaRemapProtocolGuid, NULL, (VOID **) &DmaRemap);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[ACPI](DMAR) ERROR: Cannot locate gDmaRemapProtocolGuid (%r)\n", Status));
  } else {
    if (DmaRemap->VTdSupport) {
      ReportDmar (DmaRemap);
      Status = DmaRemap->GetDmarTable (DmaRemap, (VOID **) &CurrentTable);

      if (EFI_ERROR (Status)) {
        ASSERT_EFI_ERROR (Status);
      } else {
        //
        // Perform any table specific updates.
        //
        Status = PlatformUpdateTables (CurrentTable, &TableVersion);
        ASSERT_EFI_ERROR (Status);

        TableHandle = 0;
        Status = AcpiTable->InstallAcpiTable (
                            AcpiTable,
                            CurrentTable,
                            CurrentTable->Length,
                            &TableHandle
                            );
        ASSERT_EFI_ERROR (Status);
      }
    }
  }
  FreePool (AcsDevArray);

  return EFI_SUCCESS;
}


EFI_STATUS
LocateCapRegBlock (
  IN     EFI_PCI_IO_PROTOCOL  *PciIo,
  IN     UINT8                CapID,
  OUT    UINT8                *PciExpressOffset,
  OUT    UINT8                *NextRegBlock
  )
{
  UINT16  CapabilityID;
  UINT32  Temp;
  UINT8   CapabilityPtr;
  UINT16  CapabilityEntry;

  PciIo->Pci.Read (
            PciIo,
            EfiPciIoWidthUint32,
            PCI_CAPBILITY_POINTER_OFFSET,
            1,
            &Temp
            );

  CapabilityPtr = (UINT8)Temp;
  //
  // According to the PCI spec a value of 0x00
  // is the end of the list
  //
  while (CapabilityPtr >= 0x40) {
    //
    // Mask it to DWORD alignment per PCI spec
    //
    CapabilityPtr &= 0xFC;
    PciIo->Pci.Read (
               PciIo,
               EfiPciIoWidthUint16,
               CapabilityPtr,
               1,
               &CapabilityEntry
               );

    CapabilityID = (UINT8) CapabilityEntry;

    if (CapabilityID == CapID) {
      *PciExpressOffset = CapabilityPtr;
      if (NextRegBlock != NULL) {
        *NextRegBlock = (UINT8) ((CapabilityEntry >> 8) & 0xFC);
      }

      return EFI_SUCCESS;
    }

    CapabilityPtr = (UINT8) ((CapabilityEntry >> 8) & 0xFC);
  }

  return EFI_NOT_FOUND;
}


EFI_STATUS
LocatePciExpressCapRegBlock (
  IN     EFI_PCI_IO_PROTOCOL  *PciIo,
  IN     UINT16               CapID,
  OUT    UINT32               *Offset,
  OUT    UINT32               *NextRegBlock
)
{
  UINT32  CapabilityPtr;
  UINT32  CapabilityEntry;
  UINT16  CapabilityID;

  CapabilityPtr = EFI_PCIE_CAPABILITY_BASE_OFFSET;

  while ((CapabilityPtr != 0) && (CapabilityPtr < 0x1000)) {
    //
    // Mask it to DWORD alignment per PCI spec
    //
    CapabilityPtr &= 0xFFC;
    PciIo->Pci.Read (
               PciIo,
               EfiPciIoWidthUint32,
               CapabilityPtr,
               1,
               &CapabilityEntry
               );

    CapabilityID = (UINT16) CapabilityEntry;

    if (CapabilityID == CapID) {
      *Offset = CapabilityPtr;
      if (NextRegBlock != NULL) {
        *NextRegBlock = (CapabilityEntry >> 20) & 0xFFF;
      }

      return EFI_SUCCESS;
    }

    CapabilityPtr = (CapabilityEntry >> 20) & 0xFFF;
  }

  return EFI_NOT_FOUND;
}


VOID
DisableAriForwarding (
  VOID
  )
{
  EFI_STATUS                  Status;
  UINTN                       HandleCount;
  EFI_HANDLE                  *HandleBuffer;
  UINTN                       Index;
  EFI_PCI_IO_PROTOCOL         *PciIo;
  PCI_TYPE01                  PciConfigHeader;
  UINTN                       Segment;
  UINTN                       Bus;
  UINTN                       Device;
  UINTN                       Function;
  UINT8                       PciExpressOffset;
  PCI_REG_PCIE_DEVICE_CONTROL2 DevCtl2;

  //
  // Disable ARI forwarding before handoff to OS, as it may not be ARI-aware
  //
  //
  // ARI forwarding exist in bridge
  //

  //
  // Locate all PciIo protocol
  //
  Status = gBS->LocateHandleBuffer (
               ByProtocol,
               &gEfiPciIoProtocolGuid,
               NULL,
               &HandleCount,
               &HandleBuffer
               );
  for (Index = 0; Index < HandleCount; Index ++) {
    gBS->HandleProtocol (
          HandleBuffer[Index],
          &gEfiPciIoProtocolGuid,
          (VOID **) &PciIo
          );
    PciIo->Pci.Read (
                PciIo,
                EfiPciIoWidthUint32,
                0,
                sizeof (PciConfigHeader) / sizeof (UINT32),
                &PciConfigHeader
                );
    if ((PciConfigHeader.Hdr.ClassCode[0] == 0x00 || PciConfigHeader.Hdr.ClassCode[0] == 0x01) && PciConfigHeader.Hdr.ClassCode[1] == 0x04 && PciConfigHeader.Hdr.ClassCode[2] == 0x06) {
      //
      // 060400h or 060401h indicates it's PCI-PCI bridge, get its bus number, device number and function number
      //
      PciIo->GetLocation (
              PciIo,
              &Segment,
              &Bus,
              &Device,
              &Function
              );
      if (PciConfigHeader.Hdr.Status == EFI_PCI_STATUS_CAPABILITY) {
        //
        // the bridge support Capability list and offset 0x34 is the pointer to the data structure
        //
        //
        // Detect if PCI Express Device
        //
        Status = LocateCapRegBlock (PciIo, EFI_PCI_CAPABILITY_ID_PCIEXP, &PciExpressOffset, NULL);
        if (Status == EFI_SUCCESS) {
          //
          // this bridge device is a PCI Express bridge, Check ARI forwarding bit in Device Control 2 register
          //
          PciIo->Pci.Read (
                  PciIo,
                  EfiPciIoWidthUint16,
                  PciExpressOffset + OFFSET_OF (PCI_CAPABILITY_PCIEXP, DeviceControl2),
                  1,
                  &DevCtl2
                  );
          if (DevCtl2.Bits.AriForwarding) {
            //
            // ARI forwarding enable bit is set, we need to clear this bit before handing off control to OS
            // because OS may not ARI aware
            //
            DEBUG((DEBUG_INFO, "[VTD] %02X:%02X:%02X.%X: ARI forwarding disable before booting OS, DevCtl2 0x%02X -> 0x%02X\n",
                   Segment, Bus, Device, Function, DevCtl2.Uint16, DevCtl2.Uint16 & ~BIT5));
            DevCtl2.Bits.AriForwarding = 0;
            PciIo->Pci.Write (
                  PciIo,
                  EfiPciIoWidthUint16,
                  PciExpressOffset + OFFSET_OF (PCI_CAPABILITY_PCIEXP, DeviceControl2),
                  1,
                  &DevCtl2
                  );
          }
        }
      }
    }
  }
} // DisableAriForwarding()
