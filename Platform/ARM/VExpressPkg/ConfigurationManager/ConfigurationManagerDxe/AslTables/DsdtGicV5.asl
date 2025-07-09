/** @file                                                                                         // [CODE_FIRST] 11148
  Differentiated System Description Table Fields (DSDT)                                           // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
  Copyright (c) 2014-2025, ARM Ltd. All rights reserved.<BR>                                      // [CODE_FIRST] 11148
  Copyright (c) 2013, Al Stone <al.stone@linaro.org>                                              // [CODE_FIRST] 11148
  All rights reserved.                                                                            // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
  SPDX-License-Identifier: BSD-2-Clause-Patent                                                    // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
**/                                                                                               // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
DefinitionBlock("DsdtTable.aml", "DSDT", 2, "ARMLTD", "ARM-VEXP", 1) {                            // [CODE_FIRST] 11148
  Scope(_SB) {                                                                                    // [CODE_FIRST] 11148
    //                                                                                            // [CODE_FIRST] 11148
    // Processor declaration                                                                      // [CODE_FIRST] 11148
    //                                                                                            // [CODE_FIRST] 11148
    Method (_OSC, 4, Serialized)  { // _OSC: Operating System Capabilities                        // [CODE_FIRST] 11148
      CreateDWordField (Arg3, 0x00, STS0)                                                         // [CODE_FIRST] 11148
      CreateDWordField (Arg3, 0x04, CAP0)                                                         // [CODE_FIRST] 11148
      If ((Arg0 == ToUUID ("0811b06e-4a27-44f9-8d60-3cbbc22e7b48") /* Platform-wide Capabilities */)) {  // [CODE_FIRST] 11148
        If (!(Arg1 == One)) {                                                                     // [CODE_FIRST] 11148
          STS0 &= ~0x1F                                                                           // [CODE_FIRST] 11148
          STS0 |= 0x0A                                                                            // [CODE_FIRST] 11148
        } Else {                                                                                  // [CODE_FIRST] 11148
          If ((CAP0 & 0x100)) {                                                                   // [CODE_FIRST] 11148
            CAP0 &= ~0x100 /* No support for OS Initiated LPI */                                  // [CODE_FIRST] 11148
            STS0 &= ~0x1F                                                                         // [CODE_FIRST] 11148
            STS0 |= 0x12                                                                          // [CODE_FIRST] 11148
          }                                                                                       // [CODE_FIRST] 11148
        }                                                                                         // [CODE_FIRST] 11148
      } Else {                                                                                    // [CODE_FIRST] 11148
        STS0 &= ~0x1F                                                                             // [CODE_FIRST] 11148
        STS0 |= 0x06                                                                              // [CODE_FIRST] 11148
      }                                                                                           // [CODE_FIRST] 11148
      Return (Arg3)                                                                               // [CODE_FIRST] 11148
    }                                                                                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
    // SMC91X                                                                                     // [CODE_FIRST] 11148
    Device (NET0) {                                                                               // [CODE_FIRST] 11148
      Name (_HID, "LNRO0003")                                                                     // [CODE_FIRST] 11148
      Name (_UID, 0)                                                                              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
      Name (_CRS, ResourceTemplate () {                                                           // [CODE_FIRST] 11148
        Memory32Fixed (ReadWrite, 0x1a000000, 0x00010000)                                         // [CODE_FIRST] 11148
        Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) {0x6000000F}                   // [CODE_FIRST] 11148
      })                                                                                          // [CODE_FIRST] 11148
    }                                                                                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
    // VIRTIO block device                                                                        // [CODE_FIRST] 11148
    Device (VIRT) {                                                                               // [CODE_FIRST] 11148
      Name (_HID, "LNRO0005")                                                                     // [CODE_FIRST] 11148
      Name (_UID, 0)                                                                              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
      Name (_CRS, ResourceTemplate() {                                                            // [CODE_FIRST] 11148
        Memory32Fixed (ReadWrite, 0x1c130000, 0x10000)                                            // [CODE_FIRST] 11148
        Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive) {0xE000002A}                   // [CODE_FIRST] 11148
      })                                                                                          // [CODE_FIRST] 11148
    }                                                                                             // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
    // Auxiliar IWB device                                                                        // [CODE_FIRST] 11148
    // This device is associated to the MADT IWB entry via the UID value                          // [CODE_FIRST] 11148
    Device (IWB0) {                                                                               // [CODE_FIRST] 11148
      Name (_HID, "ARMH0003")                                                                     // [CODE_FIRST] 11148
      Name (_UID, 0)                                                                              // [CODE_FIRST] 11148
                                                                                                  // [CODE_FIRST] 11148
      Name(_CRS, ResourceTemplate ()                                                              // [CODE_FIRST] 11148
      {                                                                                           // [CODE_FIRST] 11148
         QWordMemory (                                                                            // [CODE_FIRST] 11148
           ,                                                                                      // [CODE_FIRST] 11148
           ,                                                                                      // [CODE_FIRST] 11148
           MinFixed,                                                                              // [CODE_FIRST] 11148
           MaxFixed,                                                                              // [CODE_FIRST] 11148
           NonCacheable,                                                                          // [CODE_FIRST] 11148
           ReadWrite,                                                                             // [CODE_FIRST] 11148
           0x0,                                                                                   // [CODE_FIRST] 11148
           0x2F000000, // IWB config frame base                                                   // [CODE_FIRST] 11148
           0x2F00FFFF, // IWB Config frame last byte                                              // [CODE_FIRST] 11148
           0,                                                                                     // [CODE_FIRST] 11148
           0x10000, // IWB Config frame length                                                    // [CODE_FIRST] 11148
         )                                                                                        // [CODE_FIRST] 11148
      })                                                                                          // [CODE_FIRST] 11148
    }                                                                                             // [CODE_FIRST] 11148
  } // Scope(_SB)                                                                                 // [CODE_FIRST] 11148
}                                                                                                 // [CODE_FIRST] 11148
