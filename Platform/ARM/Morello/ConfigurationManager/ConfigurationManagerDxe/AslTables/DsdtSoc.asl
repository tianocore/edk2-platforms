/** @file
  Differentiated System Description Table Fields (DSDT)

  Copyright (c) 2021 - 2023, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI for Arm Components  1.0, Platform Design Document
  - ACPI for CoreSight 1.2, Platform Design Document

**/

#include "ConfigurationManager.h"
#include "MorelloPlatform.h"

#define ACPI_GRAPH_REV        0
#define ACPI_GRAPH_DSD_UUID   "ab02a46b-74c7-45a2-bd68-f7d344ef2153"
#define ACPI_DEVICE_PROP_UUID "daffd814-6eba-4d8c-8a91-bc9bbf4aa301"

#define CORESIGHT_GRAPH_UUID  "3ecbc8b6-1d0e-4fb3-8107-e627f805c6cd"

#define CS_LINK_MAIN          1
#define CS_LINK_SECONDARY     0

#define DSD_CS_GRAPH_BEGIN(_nports)     \
        Package () {                    \
          1,                            \
          ToUUID(CORESIGHT_GRAPH_UUID), \
          _nports,

#define DSD_CS_GRAPH_END  \
        }

#define DSD_GRAPH_BEGIN(_nports)      \
     ToUUID(ACPI_GRAPH_DSD_UUID),     \
     Package() {                      \
       ACPI_GRAPH_REV,                \
       1,                             \
       DSD_CS_GRAPH_BEGIN(_nports)

#define DSD_GRAPH_END     \
        DSD_CS_GRAPH_END  \
      }

#define DSD_PORTS_BEGIN(_nports)    \
    Name (_DSD,  Package () {       \
      DSD_GRAPH_BEGIN(_nports)

#define DSD_PORTS_END     \
    DSD_GRAPH_END         \
  })

#define CS_PORT(_port, _rport, _rphandle, _dir)       \
    Package () { _port, _rport, _rphandle, _dir}

#define CS_INPUT_PORT(_port, _rport, _rphandle)       \
    CS_PORT(_port, _rport, _rphandle, CS_LINK_SECONDARY)

#define CS_OUTPUT_PORT(_port, _rport, _rphandle)      \
    CS_PORT(_port, _rport, _rphandle, CS_LINK_MAIN)


DefinitionBlock("Dsdt.aml", "DSDT", 2, "ARMLTD", "MORELLO", CFG_MGR_OEM_REVISION) {
  Scope(_SB) {
    /* _OSC: Operating System Capabilities */
    Method (_OSC, 4, Serialized) {
      CreateDWordField (Arg3, 0x00, STS0)
      CreateDWordField (Arg3, 0x04, CAP0)

      /* Platform-wide Capabilities */
      If (LEqual (Arg0, ToUUID("0811b06e-4a27-44f9-8d60-3cbbc22e7b48"))) {
        /* OSC rev 1 supported, for other version, return failure */
        If (LEqual (Arg1, One)) {
          And (STS0, Not (OSC_STS_MASK), STS0)

          If (And (CAP0, OSC_CAP_OS_INITIATED_LPI)) {
            /* OS initiated LPI not supported */
            And (CAP0, Not (OSC_CAP_OS_INITIATED_LPI), CAP0)
            Or (STS0, OSC_STS_CAPABILITY_MASKED, STS0)
          }

          If (And (CAP0, OSC_CAP_PLAT_COORDINATED_LPI)) {
            if (LEqual (FixedPcdGet32 (PcdOscLpiEnable), Zero)) {
              And (CAP0, Not (OSC_CAP_PLAT_COORDINATED_LPI), CAP0)
              Or (STS0, OSC_STS_CAPABILITY_MASKED, STS0)
            }
          }

        } Else {
          And (STS0, Not (OSC_STS_MASK), STS0)
          Or (STS0, Or (OSC_STS_FAILURE, OSC_STS_UNRECOGNIZED_REV), STS0)
        }
      } Else {
        And (STS0, Not (OSC_STS_MASK), STS0)
        Or (STS0, Or (OSC_STS_FAILURE, OSC_STS_UNRECOGNIZED_UUID), STS0)
      }

      Return (Arg3)
    }

    Name (CLPI, Package () {  /* LPI for Cluster, support 1 LPI state */
      0,                      // Version
      0,                      // Level Index
      1,                      // Count
      Package () {            // Power Gating state for Cluster
        2500,                 // Min residency (uS)
        1150,                 // Wake latency (uS)
        1,                    // Flags
        1,                    // Arch Context Flags
        0,                    // Residency Counter Frequency
        0,                    // No Parent State
        0x00000020,           // Integer Entry method
        ResourceTemplate () { // Null Residency Counter
          Register (SystemMemory, 0, 0, 0, 0)
        },
        ResourceTemplate () { // Null Usage Counter
          Register (SystemMemory, 0, 0, 0, 0)
        },
        "LPI2-Cluster"
      },
    })

    Name (PLPI, Package () {  /* LPI for Processor, support 2 LPI states */
      0,                      // Version
      0,                      // Level Index
      2,                      // Count
      Package () {            // WFI for CPU
        1,                    // Min residency (uS)
        1,                    // Wake latency (uS)
        1,                    // Flags
        0,                    // Arch Context lost Flags (no loss)
        0,                    // Residency Counter Frequency
        0,                    // No parent state
        ResourceTemplate () { // Register Entry method
          Register (FFixedHW,
            32,               // Bit Width
            0,                // Bit Offset
            0xFFFFFFFF,       // Address
            3,                // Access Size
          )
        },
        ResourceTemplate () { // Null Residency Counter
          Register (SystemMemory, 0, 0, 0, 0)
        },
        ResourceTemplate () { // Null Usage Counter
          Register (SystemMemory, 0, 0, 0, 0)
        },
        "LPI1-Core"
      },
      Package () {            // Power Gating state for CPU
        150,                  // Min residency (uS)
        350,                  // Wake latency (uS)
        1,                    // Flags
        1,                    // Arch Context lost Flags (Core context lost)
        0,                    // Residency Counter Frequency
        1,                    // Parent node can be in any shallower state
        ResourceTemplate () { // Register Entry method
          Register (FFixedHW,
            32,               // Bit Width
            0,                // Bit Offset
            0x40000002,       // Address (PwrLvl:core, StateTyp:PwrDn)
            3,                // Access Size
          )
        },
        ResourceTemplate () { // Null Residency Counter
          Register (SystemMemory, 0, 0, 0, 0)
        },
        ResourceTemplate () { // Null Usage Counter
          Register (SystemMemory, 0, 0, 0, 0)
        },
        "LPI3-Core"
      },
    })

    Device (CLU0) {   // Cluster 0
      Name (_HID, "ACPI0010")
      Name (_UID, 0)
      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.CLPI)
      }

      Device (CP00) { // Cluster 0, Cpu 0
        Name (_HID, "ACPI0007")
        Name (_UID, 0)
        Name (_STA, 0xF)
        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
        Device(ETM0) { // ETM on Cluster0 CPU0
          Name (_HID, "ARMHC500")
          Name (_CID, "ARMHC500")
          Name (_UID, 0)
          Name (_CRS, ResourceTemplate() {
            QWordMemory (
              ResourceProducer,
              PosDecode,
              MinFixed,
              MaxFixed,
              Cacheable,
              ReadWrite,
              0x00000000,                                   // Granularity
              FixedPcdGet64 (PcdCsEtm0Base),                // Min Base Address
              FixedPcdGet64 (PcdCsEtm0MaxBase),             // Max Base Address
              0,                                            // Translate
              FixedPcdGet32 (PcdCsComponentSize)            // Length
            )
          })
          DSD_PORTS_BEGIN(1)
          CS_OUTPUT_PORT(0, 0, \_SB_.SFN0)
          DSD_PORTS_END
        } // ETM0
      }

      Device (CP01) { // Cluster 0, Cpu 1
        Name (_HID, "ACPI0007")
        Name (_UID, 1)
        Name (_STA, 0xF)
        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
        Device(ETM1) { // ETM on Cluster0 CPU1
          Name (_HID, "ARMHC500")
          Name (_CID, "ARMHC500")
          Name (_UID, 1)
          Method (_LPI, 0, NotSerialized) {
            Return (\_SB.PLPI)
          }
          Name (_CRS, ResourceTemplate() {
            QWordMemory (
              ResourceProducer,
              PosDecode,
              MinFixed,
              MaxFixed,
              Cacheable,
              ReadWrite,
              0x00000000,                               // Granularity
              FixedPcdGet64 (PcdCsEtm1Base),            // Min Base Address
              FixedPcdGet64 (PcdCsEtm1MaxBase),         // Max Base Address
              0,                                        // Translate
              FixedPcdGet32 (PcdCsComponentSize)        // Length
            )
          })
          DSD_PORTS_BEGIN(1)
          CS_OUTPUT_PORT(0, 1, \_SB_.SFN0)
          DSD_PORTS_END
        } // ETM1
      }
    }

    Device (CLU1) {   // Cluster 1
      Name (_HID, "ACPI0010")
      Name (_UID, 1)
      Method (_LPI, 0, NotSerialized) {
        Return (\_SB.CLPI)
      }

      Device (CP02) { // Cluster 1, Cpu 0
        Name (_HID, "ACPI0007")
        Name (_UID, 2)
        Name (_STA, 0xF)
        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
        Device(ETM2) { // ETM on Cluster1 CPU0
          Name (_HID, "ARMHC500")
          Name (_CID, "ARMHC500")
          Name (_UID, 2)
          Name (_CRS, ResourceTemplate() {
            QWordMemory (
              ResourceProducer,
              PosDecode,
              MinFixed,
              MaxFixed,
              Cacheable,
              ReadWrite,
              0x00000000,                               // Granularity
              FixedPcdGet64 (PcdCsEtm2Base),            // Min Base Address
              FixedPcdGet64 (PcdCsEtm2MaxBase),         // Max Base Address
              0,                                        // Translate
              FixedPcdGet32 (PcdCsComponentSize)        // Length
            )
          })
          DSD_PORTS_BEGIN(1)
          CS_OUTPUT_PORT(0, 0, \_SB_.SFN1)
          DSD_PORTS_END
        } // ETM2
      }

      Device (CP03) { // Cluster 1, Cpu 1
        Name (_HID, "ACPI0007")
        Name (_UID, 3)
        Name (_STA, 0xF)
        Method (_LPI, 0, NotSerialized) {
          Return (\_SB.PLPI)
        }
        Device(ETM3) { // ETM on Cluster0 CPU0
          Name (_HID, "ARMHC500")
          Name (_CID, "ARMHC500")
          Name (_UID, 3)
          Name (_CRS, ResourceTemplate() {
            QWordMemory (
              ResourceProducer,
              PosDecode,
              MinFixed,
              MaxFixed,
              Cacheable,
              ReadWrite,
              0x00000000,                               // Granularity
              FixedPcdGet64 (PcdCsEtm3Base),            // Min Base Address
              FixedPcdGet64 (PcdCsEtm3MaxBase),         // Max Base Address
              0,                                        // Translate
              FixedPcdGet32 (PcdCsComponentSize)        // Length
            )
          })
          DSD_PORTS_BEGIN(1)
          CS_OUTPUT_PORT(0, 1, \_SB_.SFN1)
          DSD_PORTS_END
        } // ETM3
      }
    }

    Device(ETF0) {
      Name(_HID, "ARMHC97C")          // TMC
      Name(_CID, "ARMHC97C")          // TMC
      Name(_UID, 0)
      Name(_CRS, ResourceTemplate() {
        QWordMemory (
          ResourceProducer,
          PosDecode,
          MinFixed,
          MaxFixed,
          Cacheable,
          ReadWrite,
          0x00000000,                               // Granularity
          FixedPcdGet64 (PcdCsEtf0Base),            // Min Base Address
          FixedPcdGet64 (PcdCsEtf0MaxBase),         // Max Base Address
          0,                                        // Translate
          FixedPcdGet32 (PcdCsComponentSize)        // Length
        )
      })
      DSD_PORTS_BEGIN(2)
      CS_OUTPUT_PORT(0, 0, \_SB_.FUN),
      CS_INPUT_PORT(0, 0, \_SB_.SFN0)
      DSD_PORTS_END
    } // ETF0

    Device(ETF1) {
      Name(_HID, "ARMHC97C")          // TMC
      Name(_CID, "ARMHC97C")          // TMC
      Name(_UID, 1)
      Name(_CRS, ResourceTemplate() {
        QWordMemory (
          ResourceProducer,
          PosDecode,
          MinFixed,
          MaxFixed,
          Cacheable,
          ReadWrite,
          0x00000000,                               // Granularity
          FixedPcdGet64 (PcdCsEtf1Base),            // Min Base Address
          FixedPcdGet64 (PcdCsEtf1MaxBase),         // Max Base Address
          0,                                        // Translate
          FixedPcdGet32 (PcdCsComponentSize)        // Length
        )
      })
      DSD_PORTS_BEGIN(2)
      CS_OUTPUT_PORT(0, 1, \_SB_.FUN),
      CS_INPUT_PORT(0, 0, \_SB_.SFN1)
      DSD_PORTS_END
    } // ETF1

    Device(ETF2) {
      Name(_HID, "ARMHC97C")          // TMC
      Name(_CID, "ARMHC97C")          // TMC
      Name(_UID, 2)
      Name(_CRS, ResourceTemplate() {
        QWordMemory (
          ResourceProducer,
          PosDecode,
          MinFixed,
          MaxFixed,
          Cacheable,
          ReadWrite,
          0x00000000,                               // Granularity
          FixedPcdGet64 (PcdCsEtf2Base),            // Min Base Address
          FixedPcdGet64 (PcdCsEtf2MaxBase),         // Max Base Address
          0,                                        // Translate
          FixedPcdGet32 (PcdCsComponentSize)        // Length
        )
      })
      DSD_PORTS_BEGIN(2)
      CS_OUTPUT_PORT(0, 5, \_SB_.MFUN),
      CS_INPUT_PORT(0, 0, \_SB_.STM0)
      DSD_PORTS_END
    } // ETF2

    Device(FUN) {
      Name(_HID, "ARMHC9FF")
      Name(_CID, "ARMHC9FF")
      Name(_UID, 0)
      Name(_CRS, ResourceTemplate() {
        QWordMemory (
          ResourceProducer,
          PosDecode,
          MinFixed,
          MaxFixed,
          Cacheable,
          ReadWrite,
          0x00000000,                               // Granularity
          FixedPcdGet64 (PcdCsFunnel0Base),         // Min Base Address
          FixedPcdGet64 (PcdCsFunnel0MaxBase),      // Max Base Address
          0,                                        // Translate
          FixedPcdGet32 (PcdCsComponentSize)        // Length
        )
      })
      DSD_PORTS_BEGIN(3)
      CS_OUTPUT_PORT(0, 0, \_SB_.MFUN),
      CS_INPUT_PORT(0, 0, \_SB_.ETF0),
      CS_INPUT_PORT(1, 0, \_SB_.ETF1)
      DSD_PORTS_END
    } // FUN

    Device(STM0) {
      Name(_HID, "ARMHC502")    // STM
      Name(_CID, "ARMHC502")
      Name(_CRS, ResourceTemplate() {
        QWordMemory (
          ResourceProducer,
          PosDecode,
          MinFixed,
          MaxFixed,
          Cacheable,
          ReadWrite,
          0x00000000,                               // Granularity
          FixedPcdGet64 (PcdCsStmBase),             // Min Base Address
          FixedPcdGet64 (PcdCsStmMaxBase),          // Max Base Address
          0,                                        // Translate
          FixedPcdGet32 (PcdCsComponentSize)        // Length
        )
        Memory32Fixed(ReadWrite,
                      FixedPcdGet32 (PcdCsStmStimulusBase),
                      FixedPcdGet32 (PcdCsStmStimulusSize))
      })
      DSD_PORTS_BEGIN(1)
      CS_OUTPUT_PORT(0, 0, \_SB_.ETF2)
      DSD_PORTS_END
    }

    Device(MFUN) {              // Main Funnel
      Name(_HID, "ARMHC9FF")    // Funnel
      Name(_CID, "ARMHC9FF")
      Name(_UID, 1)
      Name(_CRS, ResourceTemplate() {
        QWordMemory (
          ResourceProducer,
          PosDecode,
          MinFixed,
          MaxFixed,
          Cacheable,
          ReadWrite,
          0x00000000,                               // Granularity
          FixedPcdGet64 (PcdCsFunnel1Base),         // Min Base Address
          FixedPcdGet64 (PcdCsFunnel1MaxBase),      // Max Base Address
          0,                                        // Translate
          FixedPcdGet32 (PcdCsComponentSize)        // Length
        )
      })

      DSD_PORTS_BEGIN(3)
      CS_OUTPUT_PORT(0, 0, \_SB_.REP),
      CS_INPUT_PORT(0, 0, \_SB_.FUN),
      CS_INPUT_PORT(5, 0, \_SB_.ETF2)
      DSD_PORTS_END
    } // MFUN

    Device(REP) {
      Name(_HID, "ARMHC98D")          // Replicator
      Name(_CID, "ARMHC98D")
      Name(_CRS, ResourceTemplate() {
        QWordMemory (
          ResourceProducer,
          PosDecode,
          MinFixed,
          MaxFixed,
          Cacheable,
          ReadWrite,
          0x00000000,                               // Granularity
          FixedPcdGet64 (PcdCsReplicatorBase),      // Min Base Address
          FixedPcdGet64 (PcdCsReplicatorMaxBase),   // Max Base Address
          0,                                        // Translate
          FixedPcdGet32 (PcdCsComponentSize)        // Length
        )
      })
      DSD_PORTS_BEGIN(3)
      CS_OUTPUT_PORT(0, 0, \_SB_.TPIU),
      CS_OUTPUT_PORT(1, 0, \_SB_.ETR),
      CS_INPUT_PORT(0, 0, \_SB_.MFUN)
      DSD_PORTS_END
    } // REP

    Device(TPIU) {
      Name(_HID, "ARMHC979")          // TPIU
      Name(_CID, "ARMHC979")
      Name(_CRS, ResourceTemplate() {
        QWordMemory (
          ResourceProducer,
          PosDecode,
          MinFixed,
          MaxFixed,
          Cacheable,
          ReadWrite,
          0x00000000,                               // Granularity
          FixedPcdGet64 (PcdCsTpiuBase),            // Min Base Address
          FixedPcdGet64 (PcdCsTpiuMaxBase),         // Max Base Address
          0,                                        // Translate
          FixedPcdGet32 (PcdCsComponentSize)        // Length
        )
      })
      DSD_PORTS_BEGIN(1)
      CS_INPUT_PORT(0, 0, \_SB_.REP)
      DSD_PORTS_END
    } // TPIU

    Device(ETR) {
      Name(_HID, "ARMHC97C")        // TMC
      Name(_CID, "ARMHC97C")
      Name(_UID, 3)
      Name(_CCA, 0)                 // The ETR on this platform is not coherent
      Name(_CRS, ResourceTemplate() {
        QWordMemory (
          ResourceProducer,
          PosDecode,
          MinFixed,
          MaxFixed,
          Cacheable,
          ReadWrite,
          0x00000000,                               // Granularity
          FixedPcdGet64 (PcdCsEtrBase),             // Min Base Address
          FixedPcdGet64 (PcdCsEtrMaxBase),          // Max Base Address
          0,                                        // Translate
          FixedPcdGet32 (PcdCsComponentSize)        // Length
        )
        // TMC_FULL (ETBUFINT)
        Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive){ 71 }
      })
      Name(_DSD, Package() {
         DSD_GRAPH_BEGIN(1)
         CS_INPUT_PORT(0, 1, \_SB_.REP)
         DSD_GRAPH_END,

         ToUUID(ACPI_DEVICE_PROP_UUID),
         Package() {
            Package(2) {"arm,scatter-gather", 1}
         }
      })
    } // ETR

    Device(SFN0) {              // Static Funnel 0
      Name(_HID, "ARMHC9FE")    // Funnel
      Name(_CID, "ARMHC9FE")
      Name(_UID, 0)
      DSD_PORTS_BEGIN(3)
      CS_OUTPUT_PORT(0, 0, \_SB_.ETF0),
      CS_INPUT_PORT(0, 0, \_SB_.CLU0.CP00.ETM0),
      CS_INPUT_PORT(1, 0, \_SB_.CLU0.CP01.ETM1)
      DSD_PORTS_END
    } // SFN0

    Device(SFN1) {              // Static Funnel 1
      Name(_HID, "ARMHC9FE")    // Funnel
      Name(_CID, "ARMHC9FE")
      Name(_UID, 1)
      DSD_PORTS_BEGIN(3)
      CS_OUTPUT_PORT(0, 0, \_SB_.ETF1),
      CS_INPUT_PORT(0, 0, \_SB_.CLU1.CP02.ETM2),
      CS_INPUT_PORT(1, 0, \_SB_.CLU1.CP03.ETM3)
      DSD_PORTS_END
    } // SFN1
  } // Scope(_SB)
}
