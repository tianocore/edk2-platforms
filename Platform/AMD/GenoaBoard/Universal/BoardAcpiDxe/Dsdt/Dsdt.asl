/*****************************************************************************
 *
 * Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 *****************************************************************************/

DefinitionBlock (
  "DSDT.aml",
  "DSDT",
  0x02,
  "AMD   ",
  "AmdTable",
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
  External (DRPB, FieldUnitObj)
  External (DRPA, FieldUnitObj)
  External (DIDX, FieldUnitObj)
  External (DFIN, FieldUnitObj)
  External (DOUT, FieldUnitObj)
  External (DRPN, FieldUnitObj)
  External (OSMI, FieldUnitObj)
  External (ORPB, FieldUnitObj)
  External (ORPA, FieldUnitObj)
  External (OAG1, FieldUnitObj)

  Scope (\_SB) {
    Name (SUPP, 0)
    Name (CTRL, 0)
    Name (SUPC, Zero)
    Name (CTRC, Zero)
    NAME (BUF, Buffer() {0x00, 0x00})
    Method (OSCI, 4, NotSerialized)
    {
      CreateDWordField (Arg3, 0, CDW1)
      // Check for proper UUID
      If (LOr(LEqual(Arg0, ToUUID("33DB4D5B-1FF7-401C-9657-7441C03DD766")),
         // The _OSC interface for a CXL Host Bridge UUID
         (LEqual(Arg0, ToUUID("68F2D50B-C469-4D8A-BD3D-941A103FD3FC")))))
      {
        // Create DWord-adressable fields from the Capabilities Buffer
        CreateDWordField (Arg3, 4, CDW2)
        CreateDWordField (Arg3, 8, CDW3)
        // Save Capabilities DWord2 & 3
        Store (CDW2, SUPP)
        Store (CDW3 ,CTRL)
        // Only allow native hot plug control if OS supports:
        // \* ASPM
        // \* Clock PM
        // \* MSI/MSI-X
        If (LNotEqual (And (SUPP, 0x16), 0x16))
        {
          And (CTRL, 0x1E, CTRL) // Mask bit 0 (and undefined bits)
        }
        If (LNotEqual (Arg1, One))
        {
          // Unknown revision
          Or (CDW1, 0x08, CDW1)
        }
        // Update CTRL (DWORD 3) from Platform RASD
        And (CTRL, POSC, CTRL)
        If (LNotEqual (CDW3, CTRL))
        {
          // Capabilities bits were masked
          Or (CDW1, 0x10, CDW1)
        }
        // Update DWORD3 in the buffer
        Store (CTRL, CDW3)
        // Update to RASD oreration region.
        Store (SUPP, POSS) //Store SUPP (DWORD 2) to Platform RASD
        // If CXL Host Bridge
        If (LEqual (Arg0, ToUUID ("68F2D50B-C469-4D8A-BD3D-941A103FD3FC"))) {
          CreateDWordField (Arg3, 12, CDW4)    // CXL Support Field:
          CreateDWordField (Arg3, 16, CDW5)    // CXL Control Field:
          Store(CDW4,SUPC)
          Store(CDW5,CTRC)
          //
          // The firmware clear bit 0 to deny control over CXL Memory CXL
          // Error Reporting if bit 0 or 1 are not set
          //
          // Check bit 0
          // RCD and RCH Port Register Access Supported
          //
          If (LNotEqual (And (SUPC, 0x01), 0x01))
          {
            And (CTRC, 0xFE, CTRC)
          }
          //
          // Check bit 1
          // CXL VH Register Access Supported
          //
          If (LNotEqual (And (SUPC, 0x02), 0x01))
          {
            And (CTRC, 0xFE, CTRC)
          }
          // Update DWORD5 in the buffer
          Store (CTRC, CDW5)
        }
        Return (Arg3)
      } Else {
        Or (CDW1, 4, CDW1) // Unrecognized UUID
        Return (Arg3)
      }
    }

    Method (HDSM, 7, Serialized) {
      CreateWordField(BUF, 0, SUPF)
      Store(0, SUPF)
      // check for GUID and revision match
      If (LEqual (Arg0, ToUUID("E5C937D0-3553-4D7A-9117-EA4D19C3434D"))) {
        If (LEqual(Arg1, 0x05)) {
          Store (Arg2, DIDX)
          Store (0x00, DFIN)
          If (LEqual(Arg2, 0x0C)) {
            Store (ObjectType(Arg3), Local0)
            If (LEqual (Local0, 4)) { // Arg3 is a package obj
              Store (DeRefOf (Index (Arg3, 0)), Local1)
            } Else {                  // Assume Arg3 is an Integer obj
              Store (Arg3, Local1)
            }
            Store (Local1, DFIN)
            Store (Arg6, DRPN)
          }
          Store (Arg4, DRPB)
          Store (Arg5, DRPA)
          // Trigger EDR DSM SMI
          Store (DSMI, SMIR)
          // Function 0
          If (LEqual(Arg2, 0)) {
            Store(DOUT, SUPF)
            Return(BUF)
          }
          // Functions 0x0C, 0x0D
          Return(DOUT)
        }
      }
      Return(BUF) // Failed
    } // end HDSM

    Method (HOST, 4, Serialized) {
      // OSPM calls this method after processing ErrorDisconnectRecover notification from firmware
      Switch(And(Arg0,0xFF)) { // Mask to retain low byte
        Case(0x0F) { // Error Disconnect Recover request
          Store (Arg2,  ORPB)
          Store (Arg3, ORPA)
          Store (Arg1, OAG1)
          // Trigger EDR OST SMI
          Store (OSMI, SMIR)
        } // End Case(0xF)
      } // End Switch
    } // end HOST

  }

  Include ("FchSongshanI2C_I3C.asl")

}// End of ASL File
