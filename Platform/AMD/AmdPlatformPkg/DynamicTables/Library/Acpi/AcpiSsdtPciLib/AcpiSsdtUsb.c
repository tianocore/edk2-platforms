/** @file

  USB Host Controller detection for ACPI SSDT generation.

  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier BSD-2-Clause-Patent
**/

#include <IndustryStandard/Pci.h>
#include <Library/AcpiHelperLib.h>
#include <Library/AmdPlatformSocLib.h>
#include <Library/AmlLib/AmlLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PciSegmentLib.h>
#include <Library/PrintLib.h>
#include <Uefi/UefiBaseType.h>

#include "AcpiSsdtPciLib.h"

/** C array containing the compiled USB AML template.
    This symbol is defined in the auto generated C file
    containing the AML bytecode array.
*/
extern CHAR8  acpissdtusbtemplate_aml_code[];

//
// Maximum number of USB host controllers that can be tracked
//
#define MAX_USB_DEVICES  16

//
// Maximum length of an ACPI device path string (e.g., "\\_SB.PC00.RP81.XHC0")
//
#define MAX_ACPI_PATH_LEN  64

//
// Global storage for USB device ACPI paths
//
STATIC CHAR8  mUsbDevicePaths[MAX_USB_DEVICES][MAX_ACPI_PATH_LEN];
STATIC UINTN  mUsbDeviceCount = 0;

/**
  Attach USB ASL template scope to root node.

  This function parses the AcpiSsdtUsb.asl template, finds the \_SB scope,
  detaches it from the template, and attaches it to the provided root node.

  @param[in,out]  RootNode  - AML root node to attach the USB scope to

  @retval     EFI_SUCCESS           USB scope successfully attached.
  @retval     EFI_NOT_FOUND         \_SB scope not found in template.
  @retval     Other                 Error from AML library.
**/
EFI_STATUS
EFIAPI
AttachUsbAslTemplate (
  IN OUT  AML_ROOT_NODE_HANDLE  RootNode
  )
{
  EFI_STATUS              Status;
  AML_ROOT_NODE_HANDLE    UsbRootNode;
  AML_OBJECT_NODE_HANDLE  UsbSbNode;
  AML_OBJECT_NODE_HANDLE  PrwNode;

  if (RootNode == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Parse USB ASL template
  //
  Status = AmlParseDefinitionBlock (
             (EFI_ACPI_DESCRIPTION_HEADER *)acpissdtusbtemplate_aml_code,
             &UsbRootNode
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-USB: Failed to Parse USB SSDT Template. Status = %r\n",
      Status
      ));
    return Status;
  }

  //
  // Find the \_SB scope node
  //
  Status = AmlFindNode (UsbRootNode, "\\_SB_", &UsbSbNode);
  if (EFI_ERROR (Status) || (UsbSbNode == NULL)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-USB: Failed to find \\_SB scope node. Status = %r\n",
      Status
      ));
    AmlDeleteTree (UsbRootNode);
    return EFI_NOT_FOUND;
  }

  //
  // Remove _PRW method from \_SB scope before detaching
  // The _PRW will be attached to individual USB device nodes instead
  // NOTE: Must be done before detaching UsbSbNode, as AmlFindNode requires valid tree
  //
  Status = AmlFindNode (UsbRootNode, "\\_SB_._PRW", &PrwNode);
  if (!EFI_ERROR (Status) && (PrwNode != NULL)) {
    Status = AmlDetachNode (PrwNode);
    if (!EFI_ERROR (Status)) {
      AmlDeleteTree (PrwNode);
      DEBUG ((
        DEBUG_VERBOSE,
        "%a: Removed _PRW method from \\_SB scope\n",
        __func__
        ));
    }
  }

  //
  // Detach \_SB scope from the template
  //
  Status = AmlDetachNode (UsbSbNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-USB: Failed to detach \\_SB scope node. Status = %r\n",
      Status
      ));
    AmlDeleteTree (UsbRootNode);
    return Status;
  }

  //
  // Attach \_SB scope to RootNode
  //
  Status = AmlAttachNode (RootNode, UsbSbNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-USB: Failed to attach \\_SB scope node. Status = %r\n",
      Status
      ));
    AmlDeleteTree (UsbSbNode);
  }

  //
  // Clean up the USB template tree
  //
  AmlDeleteTree (UsbRootNode);

  return Status;
}

/**
  Attach USB host controller ACPI device (XHC0) to parent node.

  This function creates a Device node (XHC0) with _ADR, parses the
  AcpiSsdtUsb.asl template to find the _PRW method, and attaches it
  to the created device. It also saves the complete ACPI device path
  to the global array for later use in GPE methods.

  @param[in]      ParentNode      - Parent AML node to attach XHC0 device to
  @param[in]      ParentPath      - ACPI path of the parent node (e.g., "\\_SB.PC00.RP81")
  @param[in]      FunctionNum     - PCI function number of the USB controller (for _ADR)
  @param[out]     UsbDeviceNode   - Optional. Returns the created USB device node.

  @retval     EFI_SUCCESS           XHC0 device successfully attached.
  @retval     EFI_NOT_FOUND         _PRW method not found in template.
  @retval     EFI_INVALID_PARAMETER Invalid parameters provided.
**/
STATIC
EFI_STATUS
EFIAPI
AttachUsbAcpiDevice (
  IN      AML_OBJECT_NODE_HANDLE  ParentNode,
  IN      CONST CHAR8             *ParentPath,
  IN      UINT8                   FunctionNum,
  OUT     AML_OBJECT_NODE_HANDLE  *UsbDeviceNode  OPTIONAL
  )
{
  EFI_STATUS              Status;
  AML_ROOT_NODE_HANDLE    UsbRootNode;
  AML_OBJECT_NODE_HANDLE  XhcNode;
  AML_OBJECT_NODE_HANDLE  PrwNode;
  CHAR8                   FullPath[MAX_ACPI_PATH_LEN];

  if ((ParentNode == NULL) || (ParentPath == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  UsbRootNode = NULL;
  XhcNode     = NULL;
  PrwNode     = NULL;

  //
  // Create Device node (XHC0) under the parent
  //
  Status = AmlCodeGenDevice ("XHC0", ParentNode, &XhcNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to create XHC0 device node. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  //
  // Create _ADR object with function number
  //
  Status = AmlCodeGenNameInteger ("_ADR", FunctionNum, XhcNode, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to create _ADR. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  //
  // Build and save the complete ACPI device path
  //
  AsciiSPrint (FullPath, sizeof (FullPath), "%a.XHC0", ParentPath);
  if (mUsbDeviceCount < MAX_USB_DEVICES) {
    AsciiStrCpyS (
      mUsbDevicePaths[mUsbDeviceCount],
      MAX_ACPI_PATH_LEN,
      FullPath
      );
    mUsbDeviceCount++;
    DEBUG ((
      DEBUG_INFO,
      "%a: Saved USB device path [%d]: %a\n",
      __func__,
      mUsbDeviceCount - 1,
      FullPath
      ));
  } else {
    DEBUG ((
      DEBUG_WARN,
      "%a: Maximum USB device count reached, path not saved: %a\n",
      __func__,
      FullPath
      ));
  }

  //
  // Parse the USB ASL template to get _PRW method
  //
  Status = AmlParseDefinitionBlock (
             (EFI_ACPI_DESCRIPTION_HEADER *)acpissdtusbtemplate_aml_code,
             &UsbRootNode
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to parse USB SSDT template. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  //
  // Find _PRW method in the template at \_SB._PRW
  //
  Status = AmlFindNode (UsbRootNode, "\\_SB_._PRW", &PrwNode);
  if (EFI_ERROR (Status) || (PrwNode == NULL)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to find _PRW method in template. Status = %r\n",
      __func__,
      Status
      ));
    AmlDeleteTree (UsbRootNode);
    return EFI_NOT_FOUND;
  }

  //
  // Detach _PRW from the template tree
  //
  Status = AmlDetachNode (PrwNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to detach _PRW node. Status = %r\n",
      __func__,
      Status
      ));
    AmlDeleteTree (UsbRootNode);
    return Status;
  }

  //
  // Attach _PRW to the XHC0 device node
  //
  Status = AmlAttachNode (XhcNode, PrwNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to attach _PRW to XHC0. Status = %r\n",
      __func__,
      Status
      ));
    AmlDeleteTree (PrwNode);
    AmlDeleteTree (UsbRootNode);
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: XHC0 device created with _ADR=0x%02X and _PRW attached\n",
    __func__,
    FunctionNum
    ));

  //
  // Return the USB device node if requested
  //
  if (UsbDeviceNode != NULL) {
    *UsbDeviceNode = XhcNode;
  }

  //
  // Clean up the template tree (_PRW is now detached)
  //
  AmlDeleteTree (UsbRootNode);

  return EFI_SUCCESS;
}

/**
  Check if a USB host controller is connected to the specified root port.

  This function reads the secondary bus number from the root port and checks
  if the device at Device 0 on that bus is a USB host controller (Class Code 0x0C03).
  In PCIe topology, a device directly connected to a root port is always at Device 0.

  @param[in]      RootBridge  - Root Bridge instance containing the root port
  @param[in]      RpIndex     - Index of the root port to check
  @param[in,out]  DeviceNode  - AML device node for the root port (for creating child devices)

  @retval     EFI_SUCCESS           USB host controller found.
  @retval     EFI_NOT_FOUND         No USB host controller found on this root port.
  @retval     EFI_INVALID_PARAMETER Invalid parameters provided.
**/
EFI_STATUS
EFIAPI
DetectUsbHostController (
  IN      AMD_PCI_ROOT_BRIDGE_OBJECT_INSTANCE  *RootBridge,
  IN      UINTN                                RpIndex,
  IN OUT  AML_OBJECT_NODE_HANDLE               DeviceNode
  )
{
  EFI_STATUS  Status;
  UINT8       SecondaryBus;
  UINT8       FuncNum;
  UINT8       MaxFunc;
  UINT32      ClassCode;
  UINT16      VendorId;
  BOOLEAN     UsbFound;
  CHAR8       DevicePath[MAX_ACPI_PATH_LEN];

  if ((RootBridge == NULL) || (RootBridge->Object == NULL) ||
      (RpIndex == 0) || (RpIndex > RootBridge->RootPortCount) ||
      (RootBridge->RootPort[RpIndex] == NULL) || (DeviceNode == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  UsbFound = FALSE;

  //
  // Construct the ACPI device path for the root port (e.g., "\_SB.PC00.RP81")
  //
  if (RootBridge->Uid > 0xF) {
    AsciiSPrint (
      DevicePath,
      sizeof (DevicePath),
      "\\_SB.PC%c%c.RP%c%c",
      AsciiFromHex ((UINT8)((RootBridge->Uid >> 4) & 0xF)),
      AsciiFromHex ((UINT8)(RootBridge->Uid & 0xF)),
      AsciiFromHex ((UINT8)(RootBridge->RootPort[RpIndex]->Device & 0xF)),
      AsciiFromHex ((UINT8)(RootBridge->RootPort[RpIndex]->Function & 0xF))
      );
  } else {
    AsciiSPrint (
      DevicePath,
      sizeof (DevicePath),
      "\\_SB.PC0%c.RP%c%c",
      AsciiFromHex ((UINT8)(RootBridge->Uid & 0xF)),
      AsciiFromHex ((UINT8)(RootBridge->RootPort[RpIndex]->Device & 0xF)),
      AsciiFromHex ((UINT8)(RootBridge->RootPort[RpIndex]->Function & 0xF))
      );
  }

  //
  // Read secondary bus number (offset 0x19) from the root port
  //
  SecondaryBus = PciSegmentRead8 (
                   PCI_SEGMENT_LIB_ADDRESS (
                     RootBridge->Object->Segment,
                     RootBridge->Object->BaseBusNumber,
                     RootBridge->RootPort[RpIndex]->Device,
                     RootBridge->RootPort[RpIndex]->Function,
                     PCI_BRIDGE_SECONDARY_BUS_REGISTER_OFFSET  // Secondary Bus Number register
                     )
                   );

  if (SecondaryBus == 0) {
    return EFI_NOT_FOUND;
  }

  //
  // In PCIe, device connected to root port is always at Device 0 on Secondary Bus.
  // Check if device exists at Device 0, Function 0.
  //
  VendorId = PciSegmentRead16 (
               PCI_SEGMENT_LIB_ADDRESS (
                 RootBridge->Object->Segment,
                 SecondaryBus,
                 0,                     // Device 0
                 0,                     // Function 0
                 PCI_VENDOR_ID_OFFSET   // Vendor ID register
                 )
               );

  if (VendorId == 0xFFFF) {
    return EFI_NOT_FOUND;  // No device connected
  }

  //
  // Determine if multi-function device
  //
  MaxFunc = ((PciSegmentRead8 (
                PCI_SEGMENT_LIB_ADDRESS (
                  RootBridge->Object->Segment,
                  SecondaryBus,
                  0,
                  0,
                  PCI_HEADER_TYPE_OFFSET // Header Type register
                  )
                ) & 0x80) != 0) ? 8 : 1;

  //
  // Check all functions for USB host controller
  //
  for (FuncNum = 0; FuncNum < MaxFunc; FuncNum++) {
    if (FuncNum > 0) {
      VendorId = PciSegmentRead16 (
                   PCI_SEGMENT_LIB_ADDRESS (
                     RootBridge->Object->Segment,
                     SecondaryBus,
                     0,
                     FuncNum,
                     PCI_VENDOR_ID_OFFSET
                     )
                   );

      if (VendorId == 0xFFFF) {
        continue;
      }
    }

    //
    // Read class code: USB Host Controller has BaseClass=PCI_CLASS_SERIAL (0x0C),
    // SubClass=PCI_CLASS_SERIAL_USB (0x03)
    //
    ClassCode = PciSegmentRead32 (
                  PCI_SEGMENT_LIB_ADDRESS (
                    RootBridge->Object->Segment,
                    SecondaryBus,
                    0,
                    FuncNum,
                    PCI_REVISION_ID_OFFSET  // Revision ID / Class Code register
                    )
                  ) >> 8;

    if (((ClassCode >> 16) == PCI_CLASS_SERIAL) &&
        (((ClassCode >> 8) & 0xFF) == PCI_CLASS_SERIAL_USB))
    {
      DEBUG ((
        DEBUG_INFO,
        "USB Host Controller found on RP%X%X: SecBus=0x%02X Dev=0 Func=0x%X ProgIf=0x%02X\n",
        RootBridge->RootPort[RpIndex]->Device,
        RootBridge->RootPort[RpIndex]->Function,
        SecondaryBus,
        FuncNum,
        ClassCode & 0xFF
        ));

      //
      // Attach USB ACPI device (XHC0) to the root port device node
      // Pass the device path so it can be saved for GPE notification
      //
      Status = AttachUsbAcpiDevice (DeviceNode, DevicePath, FuncNum, NULL);
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_WARN,
          "%a: Failed to attach USB ACPI device. Status = %r\n",
          __func__,
          Status
          ));
      }

      UsbFound = TRUE;
    }
  }

  return UsbFound ? EFI_SUCCESS : EFI_NOT_FOUND;
}

/**
  Add GPE _Lxx notify methods for USB wake support.

  This function creates the _GPE scope and adds _L0B and _L10 methods
  that send wake notifications to USB host controllers. It uses the
  device paths that were saved when USB controllers were detected.

  @param[in]  ScopeNode  - Parent scope node (typically root) to attach _GPE scope

  @retval     EFI_SUCCESS           GPE methods added successfully.
  @retval     EFI_NOT_FOUND         No USB devices were detected.
  @retval     EFI_INVALID_PARAMETER Invalid parameters provided.
  @retval     Other                 Error from AML library.
**/
EFI_STATUS
EFIAPI
AddGpeLxxNotifyMethod (
  IN  AML_OBJECT_NODE_HANDLE  ScopeNode
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  GpeNode;
  AML_NOTIFY_PARAM        NotifyParams[MAX_USB_DEVICES];
  UINTN                   Index;

  if (ScopeNode == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check if any USB devices were detected
  //
  if (mUsbDeviceCount == 0) {
    DEBUG ((
      DEBUG_INFO,
      "%a: No USB devices detected, skipping GPE method creation\n",
      __func__
      ));
    return EFI_NOT_FOUND;
  }

  //
  // Create _GPE scope node
  //
  Status = AmlCodeGenScope ("\\_GPE", ScopeNode, &GpeNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to create _GPE scope. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  //
  // Populate NotifyParams using the saved USB device paths
  //
  ZeroMem (NotifyParams, sizeof (NotifyParams));
  for (Index = 0; Index < mUsbDeviceCount; Index++) {
    NotifyParams[Index].NotifyObject.Type        = AmlMethodParamTypeString;
    NotifyParams[Index].NotifyObject.Data.Buffer = mUsbDevicePaths[Index];
    NotifyParams[Index].NotifyObject.DataSize    = AsciiStrLen (mUsbDevicePaths[Index]);
    NotifyParams[Index].NotifyValue              = 0x2;  // Device Wake

    DEBUG ((
      DEBUG_VERBOSE,
      "%a: NotifyParams[%d] = %a\n",
      __func__,
      Index,
      mUsbDevicePaths[Index]
      ));
  }

  //
  // Create Method (_L0B, 0, NotSerialized) - GPE 0x0B handler
  //
  Status = AmlCodeGenMethodNotifyList (
             "_L0B",
             TRUE,
             0,
             (UINT32)mUsbDeviceCount,
             NotifyParams,
             GpeNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to add AML _L0B method. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  //
  // Create Method (_L10, 0, NotSerialized) - GPE 0x10 handler
  //
  Status = AmlCodeGenMethodNotifyList (
             "_L10",
             TRUE,
             0,
             (UINT32)mUsbDeviceCount,
             NotifyParams,
             GpeNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to add AML _L10 method. Status = %r\n",
      __func__,
      Status
      ));
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: GPE _L0B and _L10 methods added for %d USB devices\n",
    __func__,
    mUsbDeviceCount
    ));

  return EFI_SUCCESS;
}
