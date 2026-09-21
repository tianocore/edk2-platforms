/*****************************************************************************
 *
 * Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 *****************************************************************************
*/

// cspell:ignore MPRAS

DefinitionBlock (
  "DSDT.aml",
  "DSDT",
  0x02,
  "AMDINC",
  "AMDCRB  ",
  0x00
)

// BEGIN OF ASL SCOPE
{
  Name (\_S0, Package(4) {
    0x00, 0x00, 0x00, 0x00 // PM1a_CNT.SLP_TYP = 0, PM1b_CNT.SLP_TYP = 0
  })
  Name (\_S5, Package(4) {
    0x05, 0x00, 0x00, 0x00 // PM1a_CNT.SLP_TYP = 5, PM1b_CNT.SLP_TYP = 0
  })

  External (POSS, FieldUnitObj)
  External (POSC, FieldUnitObj)
  External (SMIR, FieldUnitObj)
  External (DSMI, FieldUnitObj)
  External (DRPS, FieldUnitObj)
  External (DRPB, FieldUnitObj)
  External (DRPA, FieldUnitObj)
  External (DIDX, FieldUnitObj)
  External (DFIN, FieldUnitObj)
  External (DOUT, FieldUnitObj)
  External (DRPN, FieldUnitObj)
  External (OSMI, FieldUnitObj)
  External (ORPS, FieldUnitObj)
  External (ORPB, FieldUnitObj)
  External (ORPA, FieldUnitObj)
  External (OAG1, FieldUnitObj)
  External (HSMI, FieldUnitObj)
  External (HRPB, FieldUnitObj)
  External (HRPA, FieldUnitObj)
  External (HPCK, FieldUnitObj)
  External (HPHM, FieldUnitObj)
  External (AERM, FieldUnitObj)
  External (ECRC, FieldUnitObj)
  External (EDRS, FieldUnitObj)
  External (EDRM, FieldUnitObj)
  External (PCNT, FieldUnitObj)
  External (PCIS, FieldUnitObj)
  External (MCAD, FieldUnitObj)
  External (MCMD, FieldUnitObj)
  External (MAG0, FieldUnitObj)
  External (MAG1, FieldUnitObj)
  External (MAG2, FieldUnitObj)
  External (MAG3, FieldUnitObj)
  External (MAG4, FieldUnitObj)
  External (MAG5, FieldUnitObj)

  // CXL
  External (COSS, FieldUnitObj)
  External (COSC, FieldUnitObj)
  External (CXLI, FieldUnitObj)
  External (CRPN, FieldUnitObj)
  External (CFLG, FieldUnitObj)
  External (CTAG, FieldUnitObj)

  //MPRAS String Obj
  #define MPRAS_C2PMSG_1                        RS00
  #define MPRAS_C2PMSG_16                       RS01
  #define MPRAS_C2PMSG_17                       RS02
  #define MPRAS_C2PMSG_18                       RS03
  #define MPRAS_C2PMSG_22                       RS04
  #define MAX_MPRAS_COMMAND_PARAM_COUNT         RS05
  #define MPRAS_WAIT_TIMEOUT_100NS              RS06
  #define COMMAND_ID_MASK                       RS07
  #define DATA_READY_FLAG                       RS08
  #define USE_SRAM_FLAG                         RS09
  #define RESPONSE_STATUS_MASK                  RS0A
  #define RasPciSegmentInfoCache                RS0B
  #define RasRwSmnMutex                         RS0C
  #define RasMprasXactMutex                     RS10
  #define EFI_SUCCESS                           RS0D
  #define EFI_INVALID_PARAMETER                 RS0E
  #define EFI_NOT_READY                         RS0F

  //MPRAS Method Obj
  #define RasWaitForMpRasReady                  RM00
  #define RasGetPciSegmentBaseAddress           RM01
  #define RasReadSmnRegister                    RM02
  #define RasWriteSmnRegister                   RM03
  #define RasSendCommandToMpras                 RM04
  #define RasReadSmnByAddr                      RM05

  Scope (\_SB) {
    NAME (PRPB, 0xFF) //default flag for PCIe RP Bus Number
    NAME (PRPA, 0xFF) //default flag for Dev and Fun Number

    NAME (SUPP, 0) // PCI _OSC Support Field value
    NAME (CTRL, 0) // PCI _OSC Control Field value
    NAME (SUPC, 0) // CXL _OSC Support Field value
    NAME (CTRC, 0) // CXL _OSC Control Field value
    NAME (BUF, Buffer() {0x00, 0x00})

    // Reusable buffers for MPRAS command/response flows
    // EDR _DSM
    Name (MDCM, Package (6) {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000})  //MPRAS EDR _DSM Command buffer.        The set of arguments (6 DWORDs) accompanying the command.
    Name (MDRB, Package (6) {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000})  //MPRAS EDR _DSM Command return buffer. The set of values (6 DWORDs) returned by the command.
    Name (MDRS, 0x000000000)                               //MPRAS EDR _DSM Return Status.         The status returned by MPRAS firmware after executing the EDR _DSM command.
    Name (MDFR, 0x0000000000000000)                        //Status returned by RasSendCommandToMpras function after executing the EDR _DSM command.

    // EDR _OST
    Name (MOCM, Package (6) {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000})  //MPRAS EDR _OST Command buffer.        The set of arguments (6 DWORDs) accompanying the command.
    Name (MORB, Package (6) {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000})  //MPRAS EDR _OST Command return buffer. The set of values (6 DWORDs) returned by the command.
    Name (MORS, 0x000000000)                               //MPRAS EDR _OST Return Status.         The status returned by MPRAS firmware after executing the EDR _OST command.
    Name (MOFR, 0x0000000000000000)                        //Status returned by RasSendCommandToMpras function after executing the EDR _OST command.

    // CXL _OSC (BIOS_MPRAS_CMD_CXL_HIERARCHY_OSC = 0x7)
    Name (COCM, Package (6) {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000})  //MPRAS CXL _OSC Command buffer.        Arg0=Revision ID, Arg1=CXL _OSC Support Field, Arg2=CXL _OSC Control Field (OS request).
    Name (CORB, Package (6) {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000})  //MPRAS CXL _OSC Command return buffer. Resp0=Revision ID echo with bit31 = Data Ready, Resp1=Negotiated CXL _OSC Control Field.
    Name (CORS, 0x000000000)                               //MPRAS CXL _OSC Return Status.         The status returned by MPRAS firmware after executing the CXL _OSC command.
    Name (COFR, 0x0000000000000000)                        //Status returned by RasSendCommandToMpras function after executing the CXL _OSC command.

    // PCI _OSC (BIOS_MPRAS_CMD_PCI_HIERARCHY_OSC = 0x6)
    Name (POCM, Package (6) {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000})  //MPRAS PCI _OSC Command buffer.        Arg0=Revision ID, Arg1=PCIe _OSC Support Field, Arg2=PCIe _OSC Control Field (negotiated).
    Name (PORB, Package (6) {0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000})  //MPRAS PCI _OSC Command return buffer. Resp0=Revision ID echo with bit31 = Data Ready, Resp1=PCIe _OSC Control Field.
    Name (PORS, 0x000000000)                               //MPRAS PCI _OSC Return Status.         The status returned by MPRAS firmware after executing the PCI _OSC command.
    Name (POFR, 0x0000000000000000)                        //Status returned by RasSendCommandToMpras function after executing the PCI _OSC command.

    // MPRAS register and constant definitions
    Name (MPRAS_C2PMSG_1,  0x3E10904)       // MPRAS Doorbell register
    Name (MPRAS_C2PMSG_16, 0x3E10940)       // MPRAS command/status register
    Name (MPRAS_C2PMSG_17, 0x3E10944)       // MPRAS parameter register 0
    Name (MPRAS_C2PMSG_18, 0x3E10948)       // MPRAS parameter register 1
    Name (MPRAS_C2PMSG_22, 0x3E10958)       // MPRAS parameter register 5
    Name (MAX_MPRAS_COMMAND_PARAM_COUNT, 6) // Maximum parameters
    Name (MPRAS_WAIT_TIMEOUT_100NS, 50000000) // MPRAS poll timeout: 5 seconds in ACPI Timer 100ns units (5 * 10,000,000)
    Name (COMMAND_ID_MASK, 0x0000FF00)      // Command ID mask [15:8]
    Name (DATA_READY_FLAG, 0x80000000)      // Data ready flag [31]
    Name (USE_SRAM_FLAG,   0x40000000)      // Use SRAM flag [30]
    Name (RESPONSE_STATUS_MASK, 0x000000FF) // Response status mask [7:0]
    Name (EFI_SUCCESS, 0x0000000000000000)
    Name (EFI_INVALID_PARAMETER, 0x8000000000000002)
    Name (EFI_NOT_READY, 0x8000000000000006)

    //
    // Helper method to get PCI MMIO Base Address for a given Segment and Bus Number
    //  Arg0 - Segment Number
    //  Arg1 - Bus Number
    //  Return - MMIO Base Address or 0xFFFFFFFFFFFFFFFF if not found
    //
    Method (RasGetPciSegmentBaseAddress, 2, Serialized)
    {
      // Read the entire PCIS field (3072 bits = 384 bytes) into Local2 Buffer
      // 0x180 is 384 bytes (16 * 24)
      Name (PSBF, Buffer (384) {})

      // This is the correct way to copy FieldUnit data to a Buffer in ASL
      Store (ToBuffer (PCIS), PSBF)

      Store (0, Local0)  // Segment index

      // Search for matching segment in PCI_SEGMENT_INFO table
      // Use PCNT to limit search to valid entries
      While (LAnd (LLess (Local0, PCNT), LLess (Local0, 16))) {  // Max valid segments
        // Calculate byte offset for this segment entry (24 bytes per entry)
        Store (Multiply (Local0, 24), Local1)

        // Extract entry fields from the buffer
        // Structure layout (24 bytes total with manual alignment):
        // Byte  0:     UINT8 SegmentNumber
        // Bytes 1-7:   UINT8 Reserved1[7]
        // Bytes 8-15:  UINT64 BaseAddress
        // Bytes 16:    UINT8 StartBusNumber
        // Bytes 17:    UINT8 EndBusNumber
        // Bytes 18-23: UINT8 Reserved2[6]

        // Read SegmentNumber (UINT8 at offset Local1)
        Store (DerefOf (Index (PSBF, Local1)), Local2)  // SEGN

        // Read ECAM address (UINT64 at offset Local1 + 8)
        // Extract 8 bytes and convert to integer (little-endian automatically handled)
        Mid (PSBF, Add (Local1, 8), 8, Local4)
        Store (ToInteger (Local4), Local6)  // ECAM

        // Read StartBusNumber (UINT8 at offset Local1 + 16)
        Store (DerefOf (Index (PSBF, Add (Local1, 16))), Local7)  // SBUS

        // Read EndBusNumber (UINT8 at offset Local1 + 17)
        Store (DerefOf (Index (PSBF, Add (Local1, 17))), Local3)  // EBUS

        // Check if segment matches and bus is in range
        If (LAnd (LEqual (Local2, Arg0), LAnd (LGreaterEqual (Arg1, Local7), LLessEqual (Arg1, Local3)))) {
          Return (And (Local6, 0xFFFFFFFFFFFFFFFF))
        }

        Increment (Local0)
      }

      Return (0xFFFFFFFFFFFFFFFF)  // Not found
    }

    Mutex(RasRwSmnMutex, 0)

    // Platform-wide MPRAS transaction mutex. A single MPRAS micro-controller
    // (one per IOD) serves all PCIe/CXL root ports beneath it, and the MPRAS
    // command/response handshake spans multiple register accesses that must
    // not interleave. HDSM, HOST and OSCI all acquire this mutex around their
    // full MPRAS transaction (RasSendCommandToMpras + Data Ready poll + read)
    // so only one root port is served at a time. Their individual Serialized
    // implicit mutexes do not exclude one another; this shared mutex does.
    Mutex(RasMprasXactMutex, 0)

    //
    //  Read SMN register
    //  Arg0 - Segment Number
    //  Arg1 - Bus Number
    //  Arg2 - Smn Register
    //
    Method (RasReadSmnRegister, 3, Serialized)
    {
      Store (RasGetPciSegmentBaseAddress (Arg0, Arg1), Local0)
      If (LEqual (Local0, 0xFFFFFFFFFFFFFFFF)) {
        Return (0xFFFFFFFF)  // Segment/Bus not found
      }

      // Calculate MMIO address: BaseAddress + (Bus << 20) + 0xB8 (SMN index register)
      Add (Local0, ShiftLeft (Arg1, 20), Local0)
      Add (0xB8 /*IOHC::NB_SMN_INDEX_2*/, Local0, Local0)

      Acquire (RasRwSmnMutex, 0xFFFF)
      OperationRegion(VARM, SystemMemory, Local0, 0x8)
        Field(VARM, DWordAcc, NoLock, Preserve) {
          VAR1, 32,
        }
        BankField (VARM, VAR1, Arg2, DwordAcc, NoLock, Preserve) {
          Offset (0x4),
          VAR2, 32,
        }
      Store(VAR1, Local1)
      Store(VAR2, Local2)
      Store(And(Local1, 0xFFFFFFFF), VAR1)
      Release (RasRwSmnMutex)
      Return (And (Local2, 0xFFFFFFFF))
    }

    //
    //  Write SMN register
    //  Arg0 - Segment Number
    //  Arg1 - Bus Number
    //  Arg2 - Smn Register
    //  Arg3 - Write Data
    //
    Method (RasWriteSmnRegister, 4, Serialized)
    {
      Store (RasGetPciSegmentBaseAddress (Arg0, Arg1), Local0)
      If (LNotEqual (Local0, 0xFFFFFFFFFFFFFFFF)) {
        // Calculate MMIO address: BaseAddress + (Bus << 20) + 0xB8 (SMN index register)
        Add (Local0, ShiftLeft (Arg1, 20), Local0)
        Add (0xB8, Local0, Local0)

        Acquire (RasRwSmnMutex, 0xFFFF)
        OperationRegion(VARM, SystemMemory, Local0, 0x8)
          Field(VARM, DWordAcc, NoLock, Preserve) {
            VAR1, 32,
          }
          BankField (VARM, VAR1, Arg2, DwordAcc, NoLock, Preserve) {
            Offset (0x4),
            VAR2, 32,
          }
        Store(VAR1, Local1)
        Store(And(Arg3, 0xFFFFFFFF), VAR2)
        Store(And(Local1, 0xFFFFFFFF), VAR1)
        Release (RasRwSmnMutex)
      }
    }

    //
    //  Read SMN register using a precomputed MMIO index-register address.
    //  Identical access semantics to RasReadSmnRegister, but skips the PCI
    //  segment base-address lookup so the caller can hoist that lookup out of
    //  a poll loop (the lookup copies a 384-byte buffer and scans the segment
    //  table, which otherwise dominates per-iteration cost).
    //  Arg0 - Precomputed MMIO index-register address
    //         (= PCI segment base + (Bus << 20) + 0xB8)
    //  Arg1 - Smn Register
    //
    Method (RasReadSmnByAddr, 2, Serialized)
    {
      Acquire (RasRwSmnMutex, 0xFFFF)
      OperationRegion(VARR, SystemMemory, Arg0, 0x8)
        Field(VARR, DWordAcc, NoLock, Preserve) {
          VAR1, 32,
        }
        BankField (VARR, VAR1, Arg1, DwordAcc, NoLock, Preserve) {
          Offset (0x4),
          VAR2, 32,
        }
      Store(VAR1, Local1)
      Store(VAR2, Local2)
      Store(And(Local1, 0xFFFFFFFF), VAR1)
      Release (RasRwSmnMutex)
      Return (And (Local2, 0xFFFFFFFF))
    }

    // Arg0..Arg3: _OSC arguments (UUID, Revision ID, Count, Capabilities Buffer)
    // Arg4: _ADR (PCIe Root Port Device/Function encoded)
    // Arg5: _BBN (PCIe Root Port Base Bus Number)
    // Arg6: _SEG (PCI Segment Group Number) -- required for multi-segment
    //       systems to route MPRAS C2P commands to the correct die.
    Method (OSCI, 7, Serialized)
    {
      CreateDWordField (Arg3, 0, CDW1)

      // Check for proper UUID
      If (LOr(LEqual(Arg0, ToUUID("33DB4D5B-1FF7-401C-9657-7441C03DD766")),
         // The _OSC interface for a CXL Host Bridge UUID
         (LEqual(Arg0, ToUUID("68F2D50B-C469-4D8A-BD3D-941A103FD3FC")))))
      {
        // Create DWord-addressable fields from the Capabilities Buffer
        CreateDWordField (Arg3, 4, CDW2)
        CreateDWordField (Arg3, 8, CDW3)
        // Save Capabilities DWord2 & 3
        Store (CDW2, SUPP)
        Store (CDW3 ,CTRL)
        Store(0x00, Local0)
        // Support Bits:
        // 0 - extended PCI configuration operation region
        // 1 - ASPM supported
        // 2 - Clock Power Management Capability supported
        // 3 - PCI Segment Groups supported
        // 4 - MSI supported

        // Control Bits:
        // 0 - PCI Express Native Hot Plug control
        // 1 - SHPC Native Hot Plug control
        // 2 - PCI Express Native Power Management Events control
        // 3 - PCI Express Advanced Error Reporting control
        // 4 - PCI Express Capability Structure control
        // Only allow native hot plug control if OS supports:
        // \* ASPM
        // \* Clock PM
        // \* MSI/MSI-X
        If(LNotEqual(And(SUPP, HPCK), HPCK))
        {
          And (CTRL, 0x1E, CTRL) // Mask bit 0 (and undefined bits)
        }
        If(LNotEqual(And(SUPP, EDRS), EDRS))
        {
          And (CTRL, 0x7F, CTRL) // Do not allow operating system to control DPC
          If(LAnd(LNotEqual(AERM, 1), LNotEqual(AERM, 3)))
          {
            And (CTRL, 0xF7, CTRL) // Do not allow operating system to control AER in FW First modes
          }
        }

        // Allow Revision 1 and Revision 2 (and future higher revisions)
        If (LLess (Arg1, One))
        {
          // Only error out if the revision is actually 0 (invalid)
          Or (CDW1, 0x08, CDW1) // Unknown revision
        }

        // Update CTRL (DWORD 3) from Platform RASD
        // Whether the Hot Plug Handling mode and PCIe AER Reporting Mechanism are in FW First but allow OS First mode?
        If(LAnd(LEqual(HPHM, 6), LOr(LEqual(AERM, 1), LEqual(AERM, 3))))
        {
          // --- DPC/AER LOGIC ---
          If(LEqual(And(CTRL, 0x80), 0x80)) // OS request DPC?
          {
            If(LEqual(And(CTRL, 0x08), 0x08)) // OS request AER?
            {
              Or(CTRL, 0x80, CTRL) // Grant DPC
              Or(CTRL, 0x08, CTRL) // Grant AER

              // Grant LTR (Bit 5 / 0x20) if OS supports/requests it
              If (And(CTRL, 0x20)) {
                  Or(CTRL, 0x20, CTRL)
              }

              // Grant Flit Error Logging (Bit 11 / 0x0800) if OS supports/requests it
              If (And(CTRL, 0x0800)) {
                  Or(CTRL, 0x0800, CTRL)
              }

              // Trigger OSC SMI Success Path
              Store (0x0F, Local0)
              Store (Arg5, HRPB)
              Store (Arg4, HRPA)
              Store (HSMI, SMIR)
            }
            Else
            {
              // If AER is denied, DPC and Flit must also be denied
              And(CTRL, Not(0x80), CTRL)
              And(CTRL, Not(0x0800), CTRL)
              Store (0xDEADBABE, HRPA)
              Store (HSMI, SMIR)
            }
          }
          Else
          {
            // If DPC is denied, log failure
            Store (0xDEADBABE, HRPA)
            Store (HSMI, SMIR)
          }
        }

        If(LEqual(Local0,0x0F))
        {
          // Bit 11 (Flit), Bit 7 (DPC), Bit 5 (LTR), Bit 4 (PCIeCap),
          // Bit 3 (AER), Bit 2 (PME), Bit 0 (Hotplug)
          And(CTRL, 0x08BF, CTRL)

          Store(CTRL, POSC)
        } Else {
          And(CTRL, POSC, CTRL)
        }

        If (LNotEqual (CDW3, CTRL))
        {
          // Capabilities bits were masked
          Or (CDW1, 0x10, CDW1)
        }

        // Update DWORD3 in the buffer
        Store (CTRL, CDW3)
        // Update to RASD operation region.
        Store (SUPP, POSS) //Store SUPP (DWORD 2) to Platform RASD

        //
        // Notify MPRAS of the PCIe _OSC result (OS taking ownership of AER) so
        // it can reset its system-wide inband correctable-error threshold
        // counter at the OS-handoff point. Issued as a direct C2P command (like
        // EDR _DSM / CXL _OSC), NOT via SMI.
        //
        If (LGreater (PCNT, 0)) {  // PCIe AER applet loaded in MPRAS mode (PciSegmentCount > 0)
          // Serialize the full MPRAS transaction platform-wide (same mutex as
          // the EDR / CXL _OSC C2P flows); there is a single MPRAS controller.
          Acquire (RasMprasXactMutex, 0xFFFF)
          Store (Arg1, Index (POCM, 0))                 // Revision ID
          Store (SUPP, Index (POCM, 1))                 // PCIe _OSC Support Field (CDW2)
          Store (CTRL, Index (POCM, 2))                 // PCIe _OSC Control Field (negotiated)
          Store (0x00000000, Index (POCM, 3))           // Reserved
          Store (0x00000000, Index (POCM, 4))           // Reserved
          Store (0x00000000, Index (POCM, 5))           // Reserved

          Store (0x0000000000000000, POFR)
          Store (0x00000000, PORS)
          Store (0x00000000, Index (PORB, 0))
          Store (0x00000000, Index (PORB, 1))
          Store (0x00000000, Index (PORB, 2))
          Store (0x00000000, Index (PORB, 3))
          Store (0x00000000, Index (PORB, 4))
          Store (0x00000000, Index (PORB, 5))

          // Route MPRAS C2P to the die owning this PCIe root bridge using
          // (_SEG, _BBN) carried in Arg6/Arg5 of OSCI. The applet resets its
          // system-wide inband CE threshold counter and acknowledges; the
          // negotiated _OSC Control Field returned to the OS is left unchanged
          // (the DPC/AER ownership grant is decided by the existing SMI path).
          Store (
            RasSendCommandToMpras (Arg6 /*_SEG*/, Arg5 /*_BBN*/, 0x6 /*BIOS_MPRAS_CMD_PCI_HIERARCHY_OSC*/, POCM, RefOf (PORS), PORB),
            POFR)

          If (LAnd (LEqual (POFR, Zero), LEqual (PORS, 0x01))) {
            RasWaitForMpRasReady (Arg6, Arg5, MPRAS_C2PMSG_17, DATA_READY_FLAG, MPRAS_WAIT_TIMEOUT_100NS)
          }
          Release (RasMprasXactMutex)
        }

        //
        // Evaluate CXL OS capability
        //
        If (LEqual(Arg0, ToUUID("68F2D50B-C469-4D8A-BD3D-941A103FD3FC"))) {
          //
          // Reset CXL globals to prevent leakage from a previous _OSC call on
          // a different host bridge when the current OS-supplied Capabilities
          // Buffer does not include CDW4 / CDW5 (Arg2 <= 3 or Arg2 <= 4).
          //
          Store (0, SUPC)
          Store (0, CTRC)

          //
          // Fetch 4th and 5th capability DWORD only if they are available.
          //
          If (LGreater(Arg2, 3)) {
              CreateDWordField(Arg3, 12, CDW4) // CXL Support Field
              // Save CXL capabilities Support and Capabilities
              Store (CDW4, SUPC)
          }
          If (LGreater(Arg2, 4)) {
              CreateDWordField(Arg3, 16, CDW5) // CXL Control Field
              // Save CXL capabilities Support and Capabilities
              Store (CDW5, CTRC)
          }

          //
          // Hybrid CXL _OSC negotiation.
          //   1. Always run the SMI path so FW completes the full CXL _OSC
          //      handling (component error reporting, endpoint unwind, clearing
          //      unsupported bits, etc.).
          //   2. When AmdRasEdrSciCallback selected MPRAS mode (CRPN tag ==
          //      0x5341524D 'MRAS'), unconditionally issue the MPRAS CXL _OSC
          //      C2P command. Running it on every _OSC keeps the MPRAS-side
          //      isolation globals (g_cxl_timeout_iso_sup,
          //      g_cxl_iso_notif_ctrl) in sync with the latest platform-
          //      negotiated values, so the isolation event handler never
          //      consults stale defaults.
          //   3. Overlay only the two isolation bits negotiated by the MPRAS
          //      applet onto CTRC:
          //         bit1 (0x02) CxlMemIsolationEnableControl
          //         bit5 (0x20) CxlIsolationNotificationControl
          //      The overlay set is intersected with the OS-requested mask
          //      (CDW5) so the response never grants a Control bit the OS
          //      did not ask for, as required by the ACPI _OSC contract.
          //      Every other bit keeps the SMI-decided value.
          //   4. On any MPRAS failure (send error, kernel reject, Data Ready
          //      timeout) fail-deny by clearing bit1 and bit5 in CTRC.
          //

          // Preserve the OS-requested Control Field before SMI overwrites CTRC.
          Store (CTRC, Local2)

          //
          //SMI path
          //
          //FW adjusts CXL _OSC Control Field via SMI based on CXL _OSC Support Field and FW policies
          Store (SUPC, COSS)
          Store (CTRC, COSC)
          Store (CXLI, SMIR)  //Trigger SMI
          Store (COSC, CTRC)

          //
          //MPRAS override (isolation bits only)
          //
          // Always issue MPRAS CXL _OSC when MPRAS mode is selected (CRPN tag
          // == 0x5341524D 'MRAS'), regardless of whether the OS requested
          // bit1 / bit5. This guarantees the MPRAS-side isolation globals
          // (g_cxl_timeout_iso_sup, g_cxl_iso_notif_ctrl) are refreshed on
          // every _OSC so the isolation event handler always observes the
          // current platform-negotiated values rather than stale defaults.
          //
          If (LEqual (CRPN, 0x5341524D /* 'MRAS' */)) {
              // Serialize the full MPRAS transaction platform-wide (see
              // RasMprasXactMutex) so only one root port is served by the
              // single MPRAS controller at a time. There is no Return between
              // this Acquire and the matching Release at the end of the block.
              Acquire (RasMprasXactMutex, 0xFFFF)
              //
              // Build MPRAS CXL _OSC C2P command buffer with the original OS
              // request; MPRAS will return a negotiated Control Field whose
              // bit1/bit5 are authoritative for these two features.
              //
              Store (Arg1, Index (COCM, 0))                 // Revision ID
              Store (SUPC, Index (COCM, 1))                 // CXL _OSC Support Field (CDW4)
              Store (Local2, Index (COCM, 2))               // CXL _OSC Control Field (OS request, CDW5)
              Store (0x00000000, Index (COCM, 3))           // Reserved
              Store (0x00000000, Index (COCM, 4))           // Reserved
              Store (0x00000000, Index (COCM, 5))           // Reserved

              Store (0x0000000000000000, COFR)
              Store (0x00000000, CORS)
              Store (0x00000000, Index (CORB, 0))
              Store (0x00000000, Index (CORB, 1))
              Store (0x00000000, Index (CORB, 2))
              Store (0x00000000, Index (CORB, 3))
              Store (0x00000000, Index (CORB, 4))
              Store (0x00000000, Index (CORB, 5))
              // Route MPRAS C2P to the die owning this CXL root bridge using
              // (_SEG, _BBN) carried in Arg6/Arg5 of OSCI.
              Store (
                RasSendCommandToMpras (Arg6 /*_SEG*/, Arg5 /*_BBN*/, 0x7 /*BIOS_MPRAS_CMD_CXL_HIERARCHY_OSC*/, COCM, RefOf (CORS), CORB),
                COFR
              )

              If (LAnd (LEqual (COFR, Zero), LEqual (CORS, 0x01))) {
                  Store (RasWaitForMpRasReady (Arg6, Arg5, MPRAS_C2PMSG_17, DATA_READY_FLAG, MPRAS_WAIT_TIMEOUT_100NS), Local1)
                  If (LEqual (Local1, 0)) {
                      // C2PMSG_18 holds the MPRAS-negotiated CXL _OSC Control Field.
                      // Overlay only bit1 (CxlMemIsolationEnableControl) and bit5
                      // (CxlIsolationNotificationControl) onto the SMI-decided CTRC.
                      // Per ACPI _OSC spec, FW must never grant a Control bit the
                      // OS did not request in CDW5, so intersect the MPRAS-granted
                      // bits with the OS-requested mask (Local2) before OR-ing.
                      Store (RasReadSmnRegister (Arg6, Arg5, MPRAS_C2PMSG_18), Local3)
                      Store (Local3, Index (CORB, 1))
                      And (CTRC, 0xFFFFFFDD, CTRC)                              // clear bits 1 and 5
                      Or  (CTRC, And (Local3, And (Local2, 0x22)), CTRC)        // set per (MPRAS grant AND OS request)
                  } Else {
                      // Data Ready timeout -- fail-deny: MPRAS is authoritative for
                      // bits 1 and 5; if it cannot answer, do not expose OS ownership.
                      And (CTRC, 0xFFFFFFDD, CTRC)
                  }
              } Else {
                  // Command send failure / kernel rejection -- fail-deny: clear
                  // bits 1 and 5 so the OS does not infer FW grant from a stale
                  // SMI-decided value.
                  And (CTRC, 0xFFFFFFDD, CTRC)
              }
              Release (RasMprasXactMutex)
          }

          //
          // Mirror the final CXL _OSC Control Field back into the SMM-visible
          // RasAcpiSmmData->CxlOscCtrl (COSC) so that any subsequent SMM code
          // (e.g. isolation event handling) observes the same value the OS sees.
          // No-op when the MPRAS path was skipped (CTRC already equals COSC).
          //
          Store (CTRC, COSC)

          //
          // Final value returned to the OS (CTRC: SMI path, optionally with
          // isolation bits overridden by MPRAS).
          //
          // Only write back CDW5 when it actually exists in the OS-supplied
          // Capabilities Buffer (Arg2 > 4). When Arg2 <= 4 the CreateDWordField
          // for CDW5 above was skipped, so an unguarded Store here would target
          // a non-existent field and abort _OSC with AE_NOT_FOUND.
          //
          If (LGreater (Arg2, 4)) {
            Store (CTRC, CDW5)
          }
        }// End of CXL OS Capability Evaluation
        Return (Arg3)
      } Else {
        Or (CDW1, 4, CDW1) // Unrecognized UUID
        Return (Arg3)
      }
    }

    // Arg0: _DSM Arg0, UUID
    // Arg1: _DSM Arg1, Revision
    // Arg2: _DSM Arg2, Function Index
    // Arg3: _DSM Arg3, Function Arguments
    // Arg4: PCIe Root Port Segment and Bus Number
    // Arg5: PCIe Root Port Device, Function Number
    // Arg6: PCIe Root Port Device Name
    Method (HDSM, 7, Serialized) {
      CreateWordField (Arg4, 0, WBUS)
      CreateWordField (Arg4, 2, WSEG)

      // check for GUID and revision match
      If (LEqual (Arg0, ToUUID("E5C937D0-3553-4D7A-9117-EA4D19C3434D"))) {
        If (LOr(LEqual(Arg1, 0x05), LEqual(Arg1, 0x06))) { // Revision ID 5 or Revision ID 6 based on PCI FW 3.3
          Store (Arg2, DIDX) //Function Index: 00h, 0Ch or 0Dh

          Store (0x00, DFIN) //Parsing Arg3 and store it in DFIN
          If (LEqual(Arg2, 0x0C)) {
            //Function Index: 0Ch
            //Arg3: Downstream Port Containment Enable request from OS
            //      0: The OS is requesting that firmware should keep DPC disabled. In response to this request, the firmware shall disable DPC.
            //      1: The OS is requesting that firmware should enable DPC. In response, the firmware may enable DPC or choose to keep it disabled.
            Store (ObjectType(Arg3), Local0)
            If (LEqual (Local0, 4)) { // Arg3 is a package obj
              Store (DeRefOf (Index (Arg3, 0)), Local1)
            } Else {                  // Assume Arg3 is an Integer obj
              Store (Arg3, Local1)
            }
            Store (Local1, DFIN)
            Store (Arg6, DRPN)
          }
          Store (WSEG, DRPS)
          Store (WBUS, DRPB)
          Store (Arg5, DRPA)

          Store (0x00, DOUT)
          If(LEqual(0, EDRM)) {
            // SMI mode
            // Trigger EDR DSM SMI
            Store (DSMI, SMIR)

            If (LEqual(Arg2, 0x0C)) {
              Store (0, DRPN)
            }

            // Functions 0x00, 0x0C and 0x0D return from SMI
            Return(DOUT)
          } Else {
            // MPRAS mode
            If (LEqual(Arg2, 0x0C)) {
              Store (0, DRPN)
            }

            // Function 00h:
            If (LEqual (Arg2, Zero)) {
              Return (0x3001) // Support Function 0x00, 0x0C, 0x0D
            }

            // Functions 0x0C and 0x0D:
            // Prepare six DWORD command arguments using runtime values
            // Use And with 0xFFFFFFFF to ensure all values are stored as 32-bit DWORDs with high bits zeroed
            Store (And (Arg1, 0xFFFFFFFF), Index (MDCM, 0))  //Argument Register 0 - Revision ID
            Store (And (Arg2, 0xFFFFFFFF), Index (MDCM, 1))  //Argument Register 1 - Function index.
                                           //  0xC - DPC enable request from OS
                                           //  0xD - Locate the PCIe port that experienced the containment event
            Store (And (DFIN, 0xFFFFFFFF), Index (MDCM, 2))  //Argument Register 2 - Downstream Port Containment and Hotplug surprise control
                                           //If Argument Register 1 is 0xC:
                                           //  0x0 - The OS is requesting that firmware should keep DPC disabled
                                           //  0x1 - The OS is requesting that firmware should enable DPC
                                           //If Argument Register 1 is 0xD:
                                           //  Reserved - set to 0
            Store (And (Arg4, 0xFFFFFFFF), Index (MDCM, 3))  //Argument Register 3 - (Undocumented) PCIe Root Port Segment and Bus Number. (Bits 31:16 = Segment, Bits 15:0 = Bus)
            Store (And (Arg5, 0xFFFFFFFF), Index (MDCM, 4))  //Argument Register 4 - (Undocumented) PCIe Root Port Device, Function Number.(Bits 31:16 = Device,  Bits 15:0 = Function)
            Store (And (Arg6, 0xFFFFFFFF), Index (MDCM, 5))  //Argument Register 5 - (Undocumented) PCIe Root Port Device Name

            // Reset response placeholders before issuing the command
            Store (0x00000000, MDRS)             //Command Register[7:0]
            Store (0x0000000000000000, MDFR)
            Store (0x00000000, Index (MDRB, 0))  //Response Register 0 - will reflect Argument 0 with Data Ready flag status
            Store (0x00000000, Index (MDRB, 1))  //Response Register 1 - status of the operation
                                           //If Argument Register 2 is 0xC:
                                           //  0x0 - The status of the operation is success. DPC is disabled and the hot-plug surprise bit may be set to 1 for hot-plug surprise capable ports.
                                           //  0x1 - The status of the operation is success. DPC is enabled and the hot-plug surprise bit may be cleared to 0 for hot-plug surprise capable ports.
                                           //  0x2 - The status of the operation is failure. (PCI FW Rev 3.3)
                                           //If Argument Register 2 is 0xD:
                                           //  PCIe port bus, device, and function number encoded as a 16 bit quality (Bits 2:0 = Function, Bits 7:3 = Device, Bits 15:8 = Bus)
            Store (0x00000000, Index (MDRB, 2))  //Response Register 2 - Reserved
            Store (0x00000000, Index (MDRB, 3))  //Response Register 3 - Reserved
            Store (0x00000000, Index (MDRB, 4))  //Response Register 4 - Reserved
            Store (0x00000000, Index (MDRB, 5))  //Response Register 5 - Reserved

            // Serialize the full MPRAS transaction platform-wide (see
            // RasMprasXactMutex). All MPRAS I/O is done inside the lock and the
            // outcome is captured into locals; the value-returning decode below
            // runs after Release so no Return occurs while the mutex is held.
            Acquire (RasMprasXactMutex, 0xFFFF)
            Store (
              RasSendCommandToMpras (WSEG, WBUS, 0x4 /*BIOS_MPRAS_CMD_EDR_DSM*/, MDCM, RefOf (MDRS), MDRB),
              MDFR
            )
            Store (One, Local3)         // Data Ready wait result: default = timeout
            Store (0x00000000, Local4)  // MPRAS_C2PMSG_18 response holder
            // Only poll for completion when the command was accepted:
            // MDFR == 0 (send OK) and MDRS == 0x01 (MSG_Result_OK). Otherwise
            // skip the wait so a rejected/no-response command does not stall
            // for the full Data Ready timeout (matches HOST behavior).
            If (LAnd (LEqual (MDFR, Zero), LEqual (MDRS, 0x01))) {
              //RasSendCommandToMpras returns success and command accepted
              //Waitting for MPRAS PCIe applet to set Data Ready flag
              Store (RasWaitForMpRasReady (WSEG, WBUS, MPRAS_C2PMSG_17, DATA_READY_FLAG, MPRAS_WAIT_TIMEOUT_100NS), Local3)
              If (LEqual (Local3, 0)) {
                //MPRAS PCIe applet has set Data Ready flag
                Store (RasReadSmnRegister (WSEG, WBUS, MPRAS_C2PMSG_18), Local4)
              }
            }
            Release (RasMprasXactMutex)

            // Decode the captured outcome (no MPRAS I/O beyond this point)
            If (LNotEqual (MDFR, Zero)) {
              //RasSendCommandToMpras returns failure
              Switch (ToInteger (Arg2)) {
                Case (0x0C) {
                  Return (0x02)        //Function Index: 0Ch, Return 2: The status of the operation is failure. (PCI FW Rev 3.3)
                }
                Case (0x0D) {
                  Return (0x80000000)  //Function Index: 0Dh, Return Bit 31 = 1: Failure. (PCI FW Rev 3.3)
                }
                Default {
                  // For other function indices, return generic failure
                  Return (Buffer(){0})
                }
              }
            }
            If (LEqual (Local3, 0)) {
              //MPRAS PCIe applet has set Data Ready flag
              Return (Local4)  //MPRAS_C2PMSG_18
            }
            //Timeout waiting for Data Ready flag from MPRAS PCIe applet
            Switch (ToInteger (Arg2)) {
              Case (0x0C) {
                Return (0x02)        //Function Index: 0Ch, Return 2: The status of the operation is failure. (PCI FW Rev 3.3)
              }
              Case (0x0D) {
                Return (0x80000000)  //Function Index: 0Dh, Return Bit 31 = 1: Failure. (PCI FW Rev 3.3)
              }
              Default {
                // For other function indices, return generic failure
                Return (Buffer(){0})
              }
            }
          }
        } else {
          // Revision ID not 5 and 6
          Return(Buffer(){0}) //Failed
        }
      }

      //
      // The OSPM can request the firmware to determine the optimum QoS Throttling Group (QTG)
      // to which a device HDM range should be assigned, based on its performance characteristics.
      // The OSPM evaluate this _DSM Function to retrieve QTG recommendations and map the device
      // HDM range to an HPA range that is described by a CFMWS entry that follows the
      // platform recommendations (CXL Revision 3.1)
      //
      If (LEqual (Arg0, ToUUID("f365f9a6-a7de-4071-a66a-b40c0b4f8e52"))) {
        Name(MQTG, 1)                //Max supported QoS Throttling Group (QTG) ID
        Name(QTGR, Package(){0,1})   // QoS Throttling Group (QTG) Recommendations

        //
        // Revision ID: 1
        //
        If (LEqual(Arg1, 1))
        {
          //
          // Function Index: 01h
          //
          If (LEqual(Arg2, 1))
          {
              //
              // Package: Max Supported QTG ID and QTG Recommendations
              //
              Return
              (
                  Package(0x02){MQTG, QTGR}
              )
          }
        }
      }
      Return(BUF) // Failed
    } // end HDSM

    // Arg0: _OST Arg0, An Integer containing the source event
    // Arg1: _OST Arg1, An Integer containing the status code
    // Arg2: PCIe Root Port Segment Number
    // Arg3: PCIe Root Port Bus Number
    // Arg4: PCIe Root Port Device, Function Number
    // Return: None (Per ACPI specification, _OST method does not return any value)
    //         This is a notification method that informs firmware of OS completion status
    Method (HOST, 5, Serialized) {
      // OSPM calls this method after processing ErrorDisconnectRecover notification from firmware
      Switch(And(Arg0,0xFF)) { // Mask to retain low byte
        Case(0x0F) { // Error Disconnect Recover notification
          Store (Arg2, ORPS) //_SEG
          Store (Arg3, ORPB) //BRB
          Store (Arg4, ORPA) //_ADR
          Store (Arg1, OAG1)

          // Create MOFI local variable by combining ORPB, ORPA, and OAG1
          // Bit 31:24 = ORPB, Bit 23:19 = ORPA >> 16, Bit 18:16 = ORPA & 0xFFFF, Bit 15:0 = OAG1 & 0xFFFF
          // i.e.,
          //   Bit 15:0 Status of the operation
          //   Bit 18:16 Function number of the port that experienced the containment event
          //   Bit 23:19 Device number of the port that experienced the containment event
          //   Bit 31:24 Bus number of the port that experienced the containment event
          Name (MOFI, Zero)                                                       // Declare MOFI as local variable
          Store (Zero, Local0)                                                    // Initialize temporary variable
          Or (Local0, ShiftLeft (And (ORPB, 0xFF), 24), Local0)                   // Bit 31:24 = ORPB
          Or (Local0, ShiftLeft (And (ShiftRight (ORPA, 16), 0x1F), 19), Local0)  // Bit 23:19 = ORPA >> 16
          Or (Local0, ShiftLeft (And (ORPA, 0x07), 16), Local0)                   // Bit 18:16 = ORPA & 0x7 (3 bits only)
          Or (Local0, And (OAG1, 0xFFFF), Local0)                                 // Bit 15:00 = OAG1 & 0xFFFF
          Store (Local0, MOFI)                                                    // Store final result in MOFI

          If(LEqual(0, EDRM)) {
            // SMI mode
            // Trigger EDR OST SMI
            Store (OSMI, SMIR)
          } Else {
            // MPRAS mode
            // Use And with 0xFFFFFFFF to ensure all values are stored as 32-bit DWORDs
            Store (And (0x0F, 0xFFFFFFFF), Index (MOCM, 0)) //Argument Register 0 - Source Event = 0xF - Error Disconnect Recover notification
            Store (And (MOFI, 0xFFFFFFFF), Index (MOCM, 1)) //Argument Register 1 - Command Payload
            Store (And (Arg2, 0xFFFFFFFF), Index (MOCM, 2)) //Argument Register 2 - (Undocumented) PCIe Root Port Segment
            Store (0x00000000, Index (MOCM, 3)) //Reserved
            Store (0x00000000, Index (MOCM, 4)) //Reserved
            Store (0x00000000, Index (MOCM, 5)) //Reserved

            Store (0x00000000, MORS)
            Store (0x0000000000000000, MOFR)
            Store (0x00000000, Index (MORB, 0)) //Response Register 0 - will reflect Argument 0 with Data Ready flag status
            Store (0x00000000, Index (MORB, 1))
            Store (0x00000000, Index (MORB, 2))
            Store (0x00000000, Index (MORB, 3))
            Store (0x00000000, Index (MORB, 4))
            Store (0x00000000, Index (MORB, 5))

            // Serialize the full MPRAS transaction platform-wide (see
            // RasMprasXactMutex). _OST returns no value, so all paths simply
            // release the mutex and fall through to the single Return below.
            Acquire (RasMprasXactMutex, 0xFFFF)
            Store (
              RasSendCommandToMpras (Arg2, Arg3, 0x5 /*BIOS_MPRAS_CMD_EDR_OST*/, MOCM, RefOf (MORS), MORB),
              MOFR
            )

            // Only poll for completion when the command was accepted:
            // MOFR == 0 (send OK) and MORS == 0x01 (MSG_Result_OK). Otherwise
            // skip the wait, exactly as the previous early-exit paths did.
            If (LAnd (LEqual (MOFR, Zero), LEqual (MORS, 0x01))) {
              //Waitting for MPRAS PCIe applet to set Data Ready flag.
              //_OST returns no value, so the Data Ready result is not used;
              //the wait still drains the handshake before releasing the mutex.
              RasWaitForMpRasReady (Arg2, Arg3, MPRAS_C2PMSG_17, DATA_READY_FLAG, MPRAS_WAIT_TIMEOUT_100NS)
            }
            Release (RasMprasXactMutex)
          }
        } // End Case(0xF)
      } // End Switch
      Return  // Per ACPI spec, _OST does not return a value
    } // end HOST

    Method (CDSM, 4, Serialized) {
      Name(FEID, 0)
      Name(RSTV, 0)
      Name(CPUN, 1024)//Maximum CpuNum = 1024
      Name (RBUF, Buffer (8) {0})   // Return buffer

      CreateByteField (RBUF, 0,VSTV)//BIT0:VolatileMemory ST Valid, BIT1:VolatileMemory ExtendedST Valid
      CreateByteField (RBUF, 1,VMST)//VolatileMemory ST
      CreateWordField (RBUF, 2,VEST)//VolatileMemory ExtendedST
      CreateByteField (RBUF, 4,PSTV)///BIT0:PersistentMemory ST Valid, BIT1:PersistentMemory ExtendedST Valid
      CreateByteField (RBUF, 5,PMST)//PersistentMemory ST
      CreateWordField (RBUF, 6,PEST)//PersistentMemory ExtendedST

      if (LEqual(CFLG, 0)){
        Return(0)                 //CDMA not Support
      }

      // check for GUID and revision match
      If (LEqual (Arg0, ToUUID("E5C937D0-3553-4D7A-9117-EA4D19C3434D"))) {
        If (LEqual(Arg1, 0x07)) {                          //Revision: 7
          If (LEqual(Arg2, 0x0F)) {                        //Arg2, Function Index: 0Fh

            Store (DeRefOf (Index (Arg3, 0)), Local1)      //FeatureID                      // DWORD (32 bits)
            Store (DeRefOf (Index (Arg3, 1)), Local2)      //FeatureArgument1: Target UID   // DWORD (32 bits)
            Store(Local1, FEID)

            switch (ToInteger(FEID, FEID)){
              //
              // For FeatureID == 0 (Processor Cache Steering Tags)
              //
              case(0)
              {
                //Name(LEN1, 4)  //size of processor UID
                //Name(LEN2, 2)  //size of Steering Tag
                Name(IDX1, 0)
                Name(PUID, 0)
                Name(XTAG, 0)

                While(LGreater(CPUN, 0)) {
                  Mid (CTAG, IDX1, 4, PUID)   //(Extract processor UID)
                  Add (IDX1, 8, IDX1)
                  Mid (CTAG, IDX1, 2, XTAG)   //(Extract Sterring Tag value)

                  if ((PUID) == (Local2)){                      //(ProcUid == Target UID)
                    Store(XTAG, VEST)
                    Store(XTAG, PEST)
                    And(XTAG, 0x00FF, VMST)
                    And(XTAG, 0x00FF, PMST)
                    Break
                  }

                  Add (IDX1, 8, IDX1)                           //index the next CDMA_ST_MAP structure.
                  Decrement(CPUN)
                }

                Or (VSTV, 0x03, VSTV)   // VolatileMemory ST Valid
                Or (PSTV, 0x03, PSTV)   // PersistentMemory ST Valid

                Return (RBUF)
              }

              default {
                //For any other FeatureID:
                //FeatureArgument1: DWORD: Reserved.
                //FeatureArgument2: QWORD: Reserved.
                //Return Value: QWORD: 0 (Not supported).
                Return(0)
              }
            }
          }
          If (LEqual(Arg2, 0)) {
            Return(0x8001)//SDCI _DSM function index is 0xF, so bit[15] and bit[0] should be 1
          }
        }
      }

      Return(Zero) //Not Support
    } // end CDSM

// _HPX Sample code - Start
    Method (DHPX, 1, NotSerialized) {
      Name (VAR0, 0)
      if (LNotEqual(Arg0, 0)) {   // ECRC Support or not
        Store(0x00000140,VAR0)
      }

      Return (Package(3){
        Package(6){     // PCI Setting Record
          0x00,         // Type 0
          0x01,         // Revision 1
          0x08,         // CacheLineSize in DWORDs
          0x40,         // LatencyTimer in PCI clocks
          0x01,         // Enable SERR (Boolean)
          0x01          // Enable PERR (Boolean)
        },
        Package(18){    // PCI Express Setting Record (Type 2)
          0x02,         // Type 2
          0x01,         // Revision 1
          0xFFFFFFFF,   // Uncorrectable Error Mask Register AND Mask
          0x00100000,   // Uncorrectable Error Mask Register OR Mask
          0xFFFFFFFF,   // Uncorrectable Error Severity Register AND Mask
          0x00094000,   // Uncorrectable Error Severity Register OR Mask
          0xFFFFFFFF,   // Correctable Error Mask Register AND Mask
          0x00000000,   // Correctable Error Mask Register OR Mask
          0xFFFFFFFF,   // Advanced Error Capabilities and Control Register AND Mask
          VAR0,         // Advanced Error Capabilities and Control Register OR Mask
          0xFFFF,       // Device Control Register AND Mask
          0x0007,       // Device Control Register OR Mask - Set CorrErr, NonFatalErr and FatalErr enable bits
          0xFFFF,       // Link Control Register AND Mask
          0x0040,       // Link Control Register OR Mask
          0xFFFFFFFF,   // Secondary Uncorrectable Error Severity Register AND Mask
          0x00000000,   // Secondary Uncorrectable Error Severity Register OR Mask
          0xFFFFFFFF,   // Secondary Uncorrectable Error Mask Register AND Mask
          0x00000000,   // Secondary Uncorrectable Error Mask Register OR Mask
        },
        Package(17){    // PCI Express Descriptor setting Record (Type 3)
          0x03,         // Type 3
          0x01,         // Revision 1
          0x01,         // Number of Register Descriptors
          0x01FF,       // Device/Port Type - All types in PCIe 4.0
          0x03,         // Function Type - All but VFs
          0x02,         // Configuration Space Location - PCI Express Extended Capability Structure
          0x01,         // PCIe Capability ID - Advanced Error Reporting Extended Cap Struct
          0x12,         // PCIe Capability Version - Applies to rev 2 and higher
          0x0000,       // PCIe Vendor ID
          0x00,         // Vendor-Specific Extended Capability/Designated Vendor-Specific Extended Capability ID
          0x00,         // Vendor-Specific Extended Capability/Designated Vendor-Specific Extended Capability Rev
          0x18,         // Match Register Offset - Advanced Error Capabilities and Control
          0x000000A0,   // Match AND Mask - Check ECRC Generation Capable and ECRC Check Capable
          0x000000A0,   // Match Value - ECRC Generation Capable and ECRC Check Capable supported?
          0x18,         // Write Register Offset - Advanced Error Capabilities and Control
          0xFFFFFFFF,   // Write AND Mask
          VAR0          // Write OR Mask - Set ECRC Generation and ECRC Check enable bits
        }
      })
    } // end HPX
// _HPX Sample code - End

    //
    // Wait for MPRAS ready by checking specific flag
    //  Arg0 - Segment Number
    //  Arg1 - Bus Number
    //  Arg2 - Register Address
    //  Arg3 - Flag to check
    //  Arg4 - Timeout in ACPI Timer 100ns units (e.g. 50,000,000 = 5 seconds)
    //  Return - 0: Success, 1: Timeout
    //
    Method (RasWaitForMpRasReady, 5, Serialized)
    {
      // Resolve the MMIO index-register address ONCE before polling. The PCI
      // segment base-address lookup copies a 384-byte buffer and scans the
      // segment table; performing it on every poll iteration (as the previous
      // RasReadSmnRegister call did) dominated per-iteration cost and pushed
      // the loop into the OS 30s While-loop watchdog (AE_AML_LOOP_TIMEOUT).
      Store (RasGetPciSegmentBaseAddress (Arg0, Arg1), Local2)
      If (LEqual (Local2, 0xFFFFFFFFFFFFFFFF)) {
        Return (1)  // Segment/Bus not found -- cannot poll, report not-ready
      }
      Add (Local2, ShiftLeft (Arg1, 20), Local2)
      Add (0xB8 /*IOHC::NB_SMN_INDEX_2*/, Local2, Local2)

      // Initial read before entering retry loop (matches C implementation)
      Store (RasReadSmnByAddr (Local2, Arg2), Local1)
      If (LEqual (And (Local1, Arg3), Arg3)) {
        Return (0)
      }

      // Bound the poll by REAL elapsed time using the ACPI Timer operator
      // (ACPI 3.0, spec 19.6.134: a 64-bit monotonically increasing value in
      // 100ns units; the interpreter converts from the actual hardware timer
      // and tracks overrun). This is independent of CONFIG_HZ, the kernel
      // Sleep() implementation, and per-iteration interpreter overhead, so it
      // gives a deterministic timeout and stays safely under the OS 30s
      // While-loop watchdog that otherwise aborts with AE_AML_LOOP_TIMEOUT.
      // Arg4 is the timeout expressed in ACPI Timer 100ns units.
      Store (Timer, Local0)  // Start timestamp (100ns units)

      While (One) {
        // Elapsed = now - start. 64-bit monotonic, so no rollover concern for
        // multi-second waits (5s = 50,000,000 ticks).
        If (LGreaterEqual (Subtract (Timer, Local0), Arg4)) {
          Return (1)  // Timeout
        }

        //Stall (1)  // Wait ~1 microsecond for next poll
        Sleep (1)  // Wait 1 millisecond for next poll
        Store (RasReadSmnByAddr (Local2, Arg2), Local1)
        If (LEqual (And (Local1, Arg3), Arg3)) {
          Return (0)
        }
      }
      Return (1)  // Unreachable; satisfies compiler return-path check
    }

    //
    // Send Command to MPRAS
    //  Arg0 - Segment Number
    //  Arg1 - Bus Number
    //  Arg2 - Command ID
    //  Arg3 - Input Parameters Package (must be 6 DWORDs, can be NULL)
    //  Arg4 - Response status from the MPRAS kernel (output; see "Return" below for details)
    //  Arg5 - Output Response Buffer Package (must be 6 DWORDs, can be NULL)
    //  Return - EFI_SUCCESS.  Arg4 may take one of the following values:
    //                                  Arg4 = 0x00 - No Response
    //                                  Arg4 = 0x01 - Command accepted, see parameter registers for command results (Arg5).
    //                                  Arg4 = 0xFF - Command rejected due to invalid command, no further response
    //
    //           EFI_NOT_READY.         Arg4 = 0x00 - No Response, Time out.
    //
    //           EFI_INVALID_PARAMETER. Arg4 = 0xFF - Command rejected.
    //
    Method (RasSendCommandToMpras, 6, Serialized)
    {
      Store (Arg2, Local5)

      // Validate Arg5 (Output Response Buffer Package) if provided
      If (LNotEqual (ObjectType (Arg5), 0/*Uninitialized*/)) {
        If (LNotEqual (ObjectType (Arg5), 4/*Package type*/)) {
          Store (0xFF, Arg4)
          Return (EFI_INVALID_PARAMETER)
        }

        If (LLess (SizeOf (Arg5), MAX_MPRAS_COMMAND_PARAM_COUNT)) {
          Store (0xFF, Arg4)
          Return (EFI_INVALID_PARAMETER)
        }
      }

      // Validate Arg3 (Input Parameters Package) if provided
      If (LNotEqual (ObjectType (Arg3), 0/*Uninitialized*/)) {
        If (LNotEqual (ObjectType (Arg3), 4/*Package type*/)) {
          Store (0xFF, Arg4)
          Return (EFI_INVALID_PARAMETER)
        }

        If (LLess (SizeOf (Arg3), MAX_MPRAS_COMMAND_PARAM_COUNT)) {
          Store (0xFF, Arg4)
          Return (EFI_INVALID_PARAMETER)
        }
      }

      Store (RasGetPciSegmentBaseAddress (Arg0, Arg1), Local0)
      If (LEqual (Local0, 0xFFFFFFFFFFFFFFFF)) {
        Store (0xFF, Arg4)
        Return (EFI_INVALID_PARAMETER)
      }

      // Step 1: Wait for DATA_READY flag in MPRAS_C2PMSG_16[31]
      Store (RasWaitForMpRasReady (Arg0, Arg1, MPRAS_C2PMSG_16, DATA_READY_FLAG, MPRAS_WAIT_TIMEOUT_100NS), Local1)
      If (LNotEqual (Local1, 0)) {
        Store (Zero, Arg4)  // No Response
        Return (EFI_NOT_READY)  // Timeout
      }

      // Step 2: Write command parameters to MPRAS C2PMSG_17 to C2PMSG_22 registers
      If (LEqual (ObjectType (Arg3), 4/*Package type*/)) {  // If Parameters is a valid Package
        Store (Zero, Local2)  // Index
        Store (MPRAS_C2PMSG_17, Local3)  // Register address

        While (LLess (Local2, MAX_MPRAS_COMMAND_PARAM_COUNT)) {
            If (LLess (Local2, SizeOf (Arg3))) {
                Store (DerefOf (Index (Arg3, Local2)), Local4)
            } Else {
                Store (0x00000000, Local4)  // Default to 0 if parameter not provided
            }
            RasWriteSmnRegister (Arg0, Arg1, Local3, Local4)
            Add (Local3, 4, Local3)
            Increment (Local2)
        }
      }

      // Step 3: SRAM buffer handling (commented out as per original C code)
      // Note: SRAM buffer functionality is not implemented

      // Step 3.5: Clear DATA_READY flag in MPRAS_C2PMSG_17[31] to prepare for new response
      // This prevents race condition where background FW hasn't cleared the flag from previous command
      Store (RasReadSmnRegister (Arg0, Arg1, MPRAS_C2PMSG_17), Local4)
      And (Local4, Not (DATA_READY_FLAG), Local4)
      RasWriteSmnRegister (Arg0, Arg1, MPRAS_C2PMSG_17, Local4)

      // Step 4: Write command ID into MPRAS_C2PMSG_16[15:8] registers
      Store (RasReadSmnRegister (Arg0, Arg1, MPRAS_C2PMSG_16), Local4)
      And (Local4, Not (COMMAND_ID_MASK), Local4)
      Or (Local4, ShiftLeft (Local5, 8), Local4)
      RasWriteSmnRegister (Arg0, Arg1, MPRAS_C2PMSG_16, Local4)

      // Step 5: USE_SRAM_FLAG handling (commented out as per original C code)
      // Note: USE_SRAM_FLAG functionality is not implemented

      // Step 6: Clear DATA_READY flag in MPRAS_C2PMSG_16[31]
      Store (RasReadSmnRegister (Arg0, Arg1, MPRAS_C2PMSG_16), Local4)
      And (Local4, Not (DATA_READY_FLAG), Local4)
      RasWriteSmnRegister (Arg0, Arg1, MPRAS_C2PMSG_16, Local4)

      // Step 7: Write same command ID to doorbell register to trigger interrupt
      RasWriteSmnRegister (Arg0, Arg1, MPRAS_C2PMSG_1, Local5)

      // Step 8: Wait for MPRAS to consume command and respond back by setting DATA_READY flag
      Store (RasWaitForMpRasReady (Arg0, Arg1, MPRAS_C2PMSG_16, DATA_READY_FLAG, MPRAS_WAIT_TIMEOUT_100NS), Local1)
      If (LEqual (Local1, 0)) {
        Store (EFI_SUCCESS, Local1)
      } Else {
        Store (EFI_NOT_READY, Local1)
      }

      // Step 9: Read the response status from MPRAS_C2PMSG_16[7:0] register
      Store (RasReadSmnRegister (Arg0, Arg1, MPRAS_C2PMSG_16), Local4)
      And (Local4, RESPONSE_STATUS_MASK, Local2)
      Store (Local2, Arg4)  // Store response status in output parameter

      // Read MPRAS C2PMSG_17 to C2PMSG_22 registers for response information
      If (LEqual (ObjectType (Arg5), 4/*Package type*/)) {  // If ResponseBuffer is a valid Package
        Store (Zero, Local2)  // Index
        Store (MPRAS_C2PMSG_17, Local3)  // Register address

        While (LLess (Local2, MAX_MPRAS_COMMAND_PARAM_COUNT)) {
          Store (RasReadSmnRegister (Arg0, Arg1, Local3), Local4)
          Store (Local4, Index (Arg5, Local2))
          Add (Local3, 4, Local3)
          Increment (Local2)  // Next register
        }
      }

      Return (And (Local1, 0xFFFFFFFFFFFFFFFF))
    } // end RasSendCommandToMpras
  } // End of \_SB scope

}// End of ASL File
