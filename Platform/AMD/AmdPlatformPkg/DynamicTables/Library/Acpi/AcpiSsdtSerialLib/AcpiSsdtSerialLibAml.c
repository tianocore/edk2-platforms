/** @file

  Generate ACPI SSDT Serial table using AML library.

  Copyright (c) 2019 - 2024, Arm Limited. All rights reserved.<BR>
  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/AmlLib/AmlLib.h>
#include <Library/AcpiHelperLib.h>
#include "AcpiSsdtSerialLib.h"

/**
  Generate an Legacy UART device in the AML tree.

  @param [in]  SerialPortInfo   Pointer to the serial port information structure.
  @param [in]  RootNode         AML root node handle.
  @param [in]  Index            Index of the UART device.

  @retval EFI_SUCCESS           The device was generated successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval Others                Failed to generate the device.
**/
EFI_STATUS
EFIAPI
GenerateLegacyUartDevice (
  IN SERIAL_PORT_CONFIG    *SerialPortInfo,
  IN AML_ROOT_NODE_HANDLE  RootNode,
  IN UINT8                 Index
  )
{
  AML_OBJECT_NODE_HANDLE  CrsNode;
  AML_OBJECT_NODE_HANDLE  DeviceNode;
  AML_OBJECT_NODE_HANDLE  ScopeNode;
  CHAR8                   Name[AML_NAME_SEG_SIZE + 1];
  EFI_STATUS              Status;
  EFI_STATUS              Status1;
  UINT32                  EisaId;
  UINT8                   IrqList[1];

  if ((SerialPortInfo == NULL) || (RootNode == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Name[0] = 'C';
  Name[1] = 'O';
  Name[2] = 'M';
  Name[3] = '1';
  Name[4] = '\0';

  /// Windows expects the legacy UART devices to be under LPC0 device
  Status = AmlCodeGenScope ("\\_SB_.PC00.LPC0", NULL, &ScopeNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL: Failed to create AML Scope Node."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  Name[3] = AsciiFromHex ((UINT8)(Index + 1));
  Status  = AmlCodeGenDevice (Name, ScopeNode, &DeviceNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL: Failed to create AML Device Node."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  Status = AmlGetEisaIdFromString ("PNP0501", &EisaId);
  if (EFI_ERROR (Status)) {
    goto exit_handler;
  }

  Status = AmlCodeGenNameInteger ("_HID", EisaId, DeviceNode, NULL);
  if (EFI_ERROR (Status)) {
    goto exit_handler;
  }

  Status = AmlCodeGenNameInteger ("_UID", Index + 1, DeviceNode, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL: Failed to create AML _UID Node."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  // _DDN
  Status = AmlCodeGenNameString ("_DDN", Name, DeviceNode, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL: Failed to create AML _DDN Node."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  // _STA
  Status = AmlCodeGenMethodRetInteger (
             "_STA",
             0x0F,
             0,
             FALSE,
             0,
             DeviceNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL: Failed to create AML _STA Node."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  Status = AmlCodeGenNameResourceTemplate ("_CRS", DeviceNode, &CrsNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL: Failed to create AML _CRS Node."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  IrqList[0] = SerialPortInfo->Interrupt & MAX_UINT8;

  Status = AmlCodeGenRdIo (
             TRUE,
             SerialPortInfo->BaseAddress & MAX_UINT16,
             SerialPortInfo->BaseAddress & MAX_UINT16,
             1,
             0x8,
             CrsNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL: Failed to generate IO RD node."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  //
  // Generate the IRQ() ASL macro.
  // This is used for legacy X86/X64/PC-AT compatible systems.
  //
  Status = AmlCodeGenRdIrq (
             TRUE,   // Edge Triggered
             FALSE,  // Active High
             TRUE,
             IrqList,
             ARRAY_SIZE (IrqList),
             CrsNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL: Failed to generate IRQ RD node."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  //
  //  Generate the UARTSerialBusV2() ASL macro.
  //  This describes legacy COM port resources for X86/X64/PC-AT compatible systems.
  //
  Status = AmlCodeGenRdUartSerialBusV2 (
             SerialPortInfo->BaudRate & MAX_UINT32,  // BaudRate
             NULL,                                   // Default 8 Bits Per Byte
             NULL,                                   // Default 1 Stop Bit
             0,                                      // Lines in Use
             NULL,                                   // Default is little endian
             NULL,                                   // Default is no parity
             NULL,                                   // Default is no flow control
             0x1,                                    // ReceiveBufferSize
             0x1,                                    // TransmitBufferSize
             (CHAR8 *)Name,                          // Serial Port Name
             (AsciiStrLen (Name) + 1) & MAX_UINT16,  // Serial Port Name Length
             NULL,                                   // Default resource index is zero
             NULL,                                   // Default is consumer
             NULL,                                   // Default is exclusive
             NULL,                                   // vendor defined data
             0,                                      // VendorDefinedDataLength
             CrsNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL: Failed to generate UartSerialBus RD node."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  Status = AmlAttachNode (RootNode, ScopeNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL: Failed to attach Scope Node to Root Node."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  return Status;
exit_handler:
  // Cleanup
  if (ScopeNode != NULL) {
    Status1 = AmlDeleteTree (ScopeNode);
    if (EFI_ERROR (Status1)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-SERIAL-PORT-FIXUP: Failed to cleanup AML tree."
        " Status = %r\n",
        Status1
        ));
    }
  }

  return Status;
}

/**
  Generate an MMIO UART device in the AML tree.

  @param [in]  SerialPortInfo   Pointer to the serial port information structure.
  @param [in]  RootNode         AML root node handle.
  @param [in]  Index            Index of the UART device.

  @retval EFI_SUCCESS           The device was generated successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval Others                Failed to generate the device.
**/
EFI_STATUS
EFIAPI
GenerateMmioUartDevice (
  IN SERIAL_PORT_CONFIG    *SerialPortInfo,
  IN AML_ROOT_NODE_HANDLE  RootNode,
  IN UINT8                 Index
  )
{
  AML_OBJECT_NODE_HANDLE  CrsNode;
  AML_OBJECT_NODE_HANDLE  DeviceNode;
  AML_OBJECT_NODE_HANDLE  ScopeNode;
  CHAR8                   Name[AML_NAME_SEG_SIZE + 1];
  CHAR8                   Uid[AML_NAME_SEG_SIZE + 1];
  EFI_STATUS              Status;
  EFI_STATUS              Status1;
  UINT8                   IrqList[1];

  if ((SerialPortInfo == NULL) || (RootNode == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Name[0] = 'F';
  Name[1] = 'U';
  Name[2] = 'R';
  Name[3] = '0';
  Name[4] = '\0';

  Uid[0] = 'I';
  Uid[1] = 'D';
  Uid[2] = '0';
  Uid[3] = '0';
  Uid[4] = '\0';

  Status = AmlCodeGenScope ("\\_SB_", NULL, &ScopeNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL: Failed to create AML Scope Node."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  Name[3] = AsciiFromHex ((UINT8)(Index));
  Status  = AmlCodeGenDevice (Name, ScopeNode, &DeviceNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL: Failed to create AML Device Node."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  Status = AmlCodeGenNameString ("_HID", "AMDI0020", DeviceNode, NULL);
  if (EFI_ERROR (Status)) {
    goto exit_handler;
  }

  Uid[3] = AsciiFromHex ((UINT8)(Index));

  Status = AmlCodeGenNameString ("_UID", Uid, DeviceNode, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL: Failed to create AML Device Node."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  // _STA
  Status = AmlCodeGenMethodRetInteger (
             "_STA",
             0x0F,
             0,
             FALSE,
             0,
             DeviceNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL: Failed to create AML _STA Node."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  // Create the _CRS for the Serial Port
  Status = AmlCodeGenNameResourceTemplate ("_CRS", DeviceNode, &CrsNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL: Failed to create AML _CRS Node."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  Status = AmlCodeGenRdMemory32Fixed (
             TRUE,
             SerialPortInfo->BaseAddress & MAX_UINT32,
             MIN_UART_ADDRESS_LENGTH & MAX_UINT32,
             CrsNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL: Failed to generate MMIO RD node."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  Status = AmlCodeGenRdMemory32Fixed (
             TRUE,
             (SerialPortInfo->BaseAddress - (2 * MIN_UART_ADDRESS_LENGTH)) & MAX_UINT32,
             MIN_UART_ADDRESS_LENGTH & MAX_UINT32,
             CrsNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL: Failed to generate MMIO RD node."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  IrqList[0] = SerialPortInfo->Interrupt & MAX_UINT8;

  Status = AmlCodeGenRdIrq (
             TRUE,   // Edge Triggered
             FALSE,  // Active High
             TRUE,
             IrqList,
             1,
             CrsNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL: Failed to generate IRQ RD node."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  Status = AmlAttachNode (RootNode, ScopeNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL: Failed to attach Scope Node to Root Node."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  return Status;

exit_handler:
  // Cleanup
  if (ScopeNode != NULL) {
    Status1 = AmlDeleteTree (ScopeNode);
    if (EFI_ERROR (Status1)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-SERIAL-PORT-FIXUP: Failed to cleanup AML tree."
        " Status = %r\n",
        Status1
        ));
    }
  }

  return Status;
}

/**
  Build the ACPI SSDT Serial Port table.

  @param [in]  SerialPortInfo   Pointer to the serial port information structure.
  @param [in]  SerialPortNum    Number of serial ports.
  @param [out] Table            Pointer to the built ACPI table.

  @retval EFI_SUCCESS           The table was built successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval Others                Failed to build the table.
**/
EFI_STATUS
EFIAPI
AmdBuildSsdtSerialPortTable (
  IN SERIAL_PORT_CONFIG                  *SerialPortInfo,
  IN UINT8                               SerialPortCount,
  OUT       EFI_ACPI_DESCRIPTION_HEADER  **Table
  )
{
  AML_ROOT_NODE_HANDLE  RootNode;
  EFI_STATUS            Status;
  EFI_STATUS            Status1;
  UINT8                 Index;

  if ((Table == NULL) || (SerialPortInfo == NULL) || (SerialPortCount == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = AmlCodeGenDefinitionBlock (
             "SSDT",
             "AMDINC",
             "SERIAL",
             0x01,
             &RootNode
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL: Failed to create AML Definition Block."
      " Status = %r\n",
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  for (Index = 0; Index < SerialPortCount; Index++) {
    if (SerialPortInfo[Index].BaseAddress > 0x3F8) {
      Status = GenerateMmioUartDevice (
                 &SerialPortInfo[Index],
                 RootNode,
                 Index
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SSDT-SERIAL: Failed to generate MMIO UART device."
          " Status = %r\n",
          Status
          ));
        goto exit_handler;
      }
    } else if ((SerialPortInfo[Index].BaseAddress >= 0x2E8) &&
               (SerialPortInfo[Index].BaseAddress <= 0x3F8))
    {
      Status = GenerateLegacyUartDevice (
                 &SerialPortInfo[Index],
                 RootNode,
                 Index
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SSDT-SERIAL: Failed to generate Legacy UART device."
          " Status = %r\n",
          Status
          ));
        goto exit_handler;
      }
    } else {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-SERIAL: Invalid Base Address 0x%X for Serial Port Index %d.\n",
        SerialPortInfo[Index].BaseAddress,
        Index
        ));
      goto exit_handler;
    }
  } // for

  // Serialize the tree.
  Status = AmlSerializeDefinitionBlock (
             RootNode,
             Table
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL: Failed to Serialize SSDT Table Data."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  // Cleanup the AML tree after successful serialization
  Status = AmlDeleteTree (RootNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL: Failed to cleanup AML tree."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  return EFI_SUCCESS;
exit_handler:
  // Cleanup
  if (RootNode != NULL) {
    Status1 = AmlDeleteTree (RootNode);
    if (EFI_ERROR (Status1)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-SERIAL: Failed to cleanup AML tree."
        " Status = %r\n",
        Status1
        ));
    }
  }

  return Status;
}
