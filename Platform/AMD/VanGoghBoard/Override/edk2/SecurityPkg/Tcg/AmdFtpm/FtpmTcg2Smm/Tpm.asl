/** @file
  The TPM2 definition block in ACPI table for TCG2 physical presence
  and MemoryClear.

Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
(c)Copyright 2016 HP Development Company, L.P.<BR>
Copyright (c) 2017, Microsoft Corporation.  All rights reserved. <BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

DefinitionBlock (
  "Tpm.aml",
  "SSDT",
  2,
  "AMD   ",
  "Tpm2Tabl",
  0x1000
  )
{
  Scope (\_SB)
  {
    Device (TPM)
    {
      //
      // TCG2
      //

      //
      //  TAG for patching TPM2.0 _HID
      //
      Name (_HID, "MSFT0101")

      Name (_CID, "MSFT0101")

      //
      // Readable name of this device, don't know if this way is correct yet
      //
      Name (_STR, Unicode ("TPM 2.0 Device"))

      //
      // Return the resource consumed by TPM device
      //
      Name (_CRS, ResourceTemplate () {
        Memory32Fixed (ReadWrite, 0xA5A5A5A5, 0x4000)  // Command Address
        Memory32Fixed (ReadWrite, 0xAAAAAAAA, 0x4000)  // Response Address
      })

      //
      // Operational region for Smi port access, FixedPcdGet16 (PcdAmdFchCfgSmiCmdPortAddr)
      //
      OperationRegion (SMIP, SystemIO, 0xB0, 1)
      Field (SMIP, ByteAcc, NoLock, Preserve)
      {
          IOB0, 8
      }

      //
      // Operational region for fTPM control area.
      // Region Offset 0xFFFF0000 and Length 0xF0 will be fixed in C code.
      //
      OperationRegion (TPMC, SystemMemory, 0xFFFF0000, 0xF0)
      Field (TPMC, DWordAcc, NoLock, Preserve)
      {
        REQS,   32,
        STAS,   32,
        CANC,   32,
        STAR,   32,
                AccessAs (QWordAcc, 0),
        INTC,   64,
                AccessAs (DWordAcc, 0),
        CMDS,   32,
                AccessAs (QWordAcc, 0),
        CMDA,   64,
                AccessAs (DWordAcc, 0),
        RSPS,   32,
                AccessAs (QWordAcc, 0),
        RSPA,   64
      }

      //
      // Operational region for TPM support, TPM Physical Presence and TPM Memory Clear
      // Region Offset 0xFFFF0000 and Length 0xF0 will be fixed in C code.
      //
      OperationRegion (TNVS, SystemMemory, 0xFFFF0000, 0xF0)
      Field (TNVS, AnyAcc, NoLock, Preserve)
      {
        PPIN,   8,  //   Software SMI for Physical Presence Interface
        PPIP,   32, //   Used for save physical presence paramter
        PPRP,   32, //   Physical Presence request operation response
        PPRQ,   32, //   Physical Presence request operation
        PPRM,   32, //   Physical Presence request operation parameter
        LPPR,   32, //   Last Physical Presence request operation
        FRET,   32, //   Physical Presence function return code
        MCIN,   8,  //   Software SMI for Memory Clear Interface
        MCIP,   32, //   Used for save the Mor paramter
        MORD,   32, //   Memory Overwrite Request Data
        MRET,   32, //   Memory Overwrite function return code
        UCRQ,   32  //   Phyical Presence request operation to Get User Confirmation Status
      }

      Method (PTS, 1, Serialized)
      {
        //
        // Detect Sx state for MOR, only S4, S5 need to handle
        //
        If (LAnd (LLess (Arg0, 6), LGreater (Arg0, 3)))
        {
          //
          // Bit4 -- DisableAutoDetect. 0 -- Firmware MAY autodetect.
          //
          If (LNot (And (MORD, 0x10)))
          {
            //
            // Trigger the SMI through ACPI _PTS method.
            //
            Store (0x02, MCIP)

            //
            // Trigger the SMI interrupt
            //
            Store (MCIN, IOB0)
          }
        }
        Return (0)
      }

      Method (_STA, 0)
      {
        Return (0x0f)
      }

      //
      // TCG Hardware Information
      //
      Method (HINF, 3, Serialized, 0, {BuffObj, PkgObj}, {UnknownObj, UnknownObj, UnknownObj}) // IntObj, IntObj, PkgObj
      {
        //
        // Switch by function index
        //
        Switch (ToInteger(Arg1))
        {
          Case (0)
          {
            //
            // Standard query
            //
            Return (Buffer () {0x03})
          }
          Case (1)
          {
            //
            // Return failure if no TPM present
            //
            Name(TPMV, Package () {0x01, Package () {0x2, 0x0}})
            if (LEqual (_STA (), 0x00))
            {
              Return (Package () {0x00})
            }

            //
            // Return TPM version
            //
            Return (TPMV)
          }
          Default {BreakPoint}
        }
        Return (Buffer () {0})
      }

      Name(TPM2, Package (0x02){
        Zero,
        Zero
      })

      Name(TPM3, Package (0x03){
        Zero,
        Zero,
        Zero
      })

      //
      // TCG Physical Presence Interface
      //
      Method (TPPI, 3, Serialized, 0, {BuffObj, PkgObj, IntObj, StrObj}, {UnknownObj, UnknownObj, UnknownObj}) // IntObj, IntObj, PkgObj
      {
        //
        // Switch by function index
        //
        Switch (ToInteger(Arg1))
        {
          Case (0)
          {
            //
            // Standard query, supports function 1-8
            //
            Return (Buffer () {0xFF, 0x01})
          }
          Case (1)
          {
            //
            // a) Get Physical Presence Interface Version
            //
            Return ("$PV")
          }
          Case (2)
          {
            //
            // b) Submit TPM Operation Request to Pre-OS Environment
            //
            Store (DerefOf (Index (Arg2, 0x00)), PPRQ)
            Store (0, PPRM)
            Store (0x02, PPIP)
            //
            // Trigger the SMI interrupt
            //
            Store (PPIN, IOB0)
            Return (FRET)


          }
          Case (3)
          {
            //
            // c) Get Pending TPM Operation Requested By the OS
            //
            Store (PPRQ, Index (TPM2, 0x01))
            Return (TPM2)
          }
          Case (4)
          {
            //
            // d) Get Platform-Specific Action to Transition to Pre-OS Environment
            //
            Return (2)
          }
          Case (5)
          {
            //
            // e) Return TPM Operation Response to OS Environment
            //
            Store (0x05, PPIP)
            //
            // Trigger the SMI interrupt
            //
            Store (PPIN, IOB0)
            Store (LPPR, Index (TPM3, 0x01))
            Store (PPRP, Index (TPM3, 0x02))

            Return (TPM3)
          }
          Case (6)
          {

            //
            // f) Submit preferred user language (Not implemented)
            //

            Return (3)

          }
          Case (7)
          {
            //
            // g) Submit TPM Operation Request to Pre-OS Environment 2
            //
            Store (7, PPIP)
            Store (DerefOf (Index (Arg2, 0x00)), PPRQ)
            Store (0, PPRM)
            If (LEqual (PPRQ, 23)) {
              Store (DerefOf (Index (Arg2, 0x01)), PPRM)
            }
            //
            // Trigger the SMI interrupt
            //
            Store (PPIN, IOB0)
            Return (FRET)
          }
          Case (8)
          {
            //
            // e) Get User Confirmation Status for Operation
            //
            Store (8, PPIP)
            Store (DerefOf (Index (Arg2, 0x00)), UCRQ)
            //
            // Trigger the SMI interrupt
            //
            Store (PPIN, IOB0)
            Return (FRET)
          }

          Default {BreakPoint}
        }
        Return (1)
      }

      Method (TMCI, 3, Serialized, 0, IntObj, {UnknownObj, UnknownObj, UnknownObj}) // IntObj, IntObj, PkgObj
      {
        //
        // Switch by function index
        //
        Switch (ToInteger (Arg1))
        {
          Case (0)
          {
            //
            // Standard query, supports function 1-1
            //
            Return (Buffer () {0x03})
          }
          Case (1)
          {
            //
            // Save the Operation Value of the Request to MORD (reserved memory)
            //
            Store (DerefOf (Index (Arg2, 0x00)), MORD)
            //
            // Trigger the SMI through ACPI _DSM method.
            //
            Store (0x01, MCIP)
            //
            // Trigger the SMI interrupt
            //
            Store (MCIN, IOB0)
            Return (MRET)
          }
          Default {BreakPoint}
        }
        Return (1)
      }

      // ACPI Start Method to permit the OS to request the firmware to execute or cancel a TPM 2.0 command.
      Method (TPMS, 3, Serialized, 0, {BuffObj, PkgObj, IntObj, StrObj}, {UnknownObj, UnknownObj, UnknownObj})
      {
        //
        // Switch by function index
        //
        Switch (ToInteger(Arg1)) {

        Case (0) {
          //
          // Standard query, supports function 1
          //
          Return (Buffer () {0x03})
        }

        Case (1) {
          //
          // Start
          //
          Return (0)
        }

        Default {BreakPoint}
        }
        Return (1)
      }

      Method (_DSM, 4, Serialized, 0, UnknownObj, {BuffObj, IntObj, IntObj, PkgObj})
      {
        //
        // TCG Hardware Information
        //
        If(LEqual(Arg0, ToUUID ("cf8e16a5-c1e8-4e25-b712-4f54a96702c8")))
        {
          Return (HINF (Arg1, Arg2, Arg3))
        }
        //
        // TCG Physical Presence Interface
        //
        If(LEqual(Arg0, ToUUID ("3dddfaa6-361b-4eb4-a424-8d10089d1653")))
        {
          Return (TPPI (Arg1, Arg2, Arg3))
        }
        //
        // TCG Memory Clear Interface
        //
        If(LEqual(Arg0, ToUUID ("376054ed-cc13-4675-901c-4756d7f2d45d")))
        {
          Return (TMCI (Arg1, Arg2, Arg3))
        }
        //
        // ACPI Start Method
        //
        If(LEqual (Arg0, ToUUID ("6bbf6cab-5463-4714-b7cd-f0203c0368d4")))
        {
          Return (TPMS (Arg1, Arg2, Arg3))
        }

        Return (Buffer () {0})
      }
    }
  }
}
