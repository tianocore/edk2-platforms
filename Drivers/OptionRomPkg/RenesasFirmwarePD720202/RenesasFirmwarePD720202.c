/**  @file
  Implementation of driver entry point and driver binding protocol.

  Copyright (c) 2016 - 2017, Linaro Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/DebugLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <IndustryStandard/Pci22.h>

#include <Protocol/PciIo.h>

#define PCI_VENDOR_ID_RENESAS   0x1912
#define PCI_DEVICE_ID_PD720201  0x14
#define PCI_DEVICE_ID_PD720202  0x15

#define PCI_FW_CTL_STAT_REG  0xF4
#define PCI_FW_CTL_DATA_REG  0xF5
#define PCI_EXT_ROM_CTL_REG  0xF6
#define PCI_FW_DATA0         0xF8
#define PCI_FW_DATA1         0xFC

#define PCI_FW_CTL_STAT_REG_FW_DL_ENABLE    (1U << 0)
#define PCI_FW_CTL_STAT_REG_RESULT_CODE     (7U << 4)
#define PCI_FW_CTL_STAT_REG_RESULT_INVALID  (0)
#define PCI_FW_CTL_STAT_REG_RESULT_OK       (1U << 4)
#define PCI_FW_CTL_STAT_REG_SET_DATA0       (1U << 0)
#define PCI_FW_CTL_STAT_REG_SET_DATA1       (1U << 1)

#define PCI_EXT_ROM_CTL_REG_ROM_EXISTS  (1U << 15)

STATIC CONST UINT32  *mFirmwareImage;
STATIC UINTN         mFirmwareImageSize;

static
UINT8
ReadCtlStatVal (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINTN                Offset
  )
{
  UINT8       CtlStatVal;
  EFI_STATUS  Status;

  Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, Offset, 1, &CtlStatVal);
  ASSERT_EFI_ERROR (Status);

  return CtlStatVal;
}

static
VOID
WriteCtlStatVal (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINTN                Offset,
  IN UINT8                CtlStatVal
  )
{
  EFI_STATUS  Status;

  Status = PciIo->Pci.Write (PciIo, EfiPciIoWidthUint8, Offset, 1, &CtlStatVal);
  ASSERT_EFI_ERROR (Status);
}

static
VOID
WriteDataVal (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINTN                Offset,
  IN UINT32               DataVal
  )
{
  EFI_STATUS  Status;

  Status = PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, Offset, 1, &DataVal);
  ASSERT_EFI_ERROR (Status);
}

static
BOOLEAN
WaitReadCtlStatVal (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINTN                Offset,
  IN UINT8                Mask,
  IN UINT8                Val
  )
{
  UINTN  Timeout;

  for (Timeout = 0; (ReadCtlStatVal (PciIo, Offset) & Mask) != Val; Timeout++) {
    if (Timeout > 1000) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Timeout waiting for reg [+0x%x] & 0x%x to become 0x%x\n",
        __func__,
        Offset,
        Mask,
        Val
        ));
      return FALSE;
    }

    gBS->Stall (10);
  }

  return TRUE;
}

static
VOID
DownloadPD720202Firmware (
  IN EFI_PCI_IO_PROTOCOL  *PciIo
  )
{
  UINTN  Idx;

  Idx = 0;

  // 1. Set "FW Download Enable" to '1b'.
  WriteCtlStatVal (
    PciIo,
    PCI_FW_CTL_STAT_REG,
    PCI_FW_CTL_STAT_REG_FW_DL_ENABLE
    );

  // 2. Read "Set DATA0" and confirm it is '0b'.
  if (!WaitReadCtlStatVal (
         PciIo,
         PCI_FW_CTL_DATA_REG,
         PCI_FW_CTL_STAT_REG_SET_DATA0,
         0
         ))
  {
    return;
  }

  // 3. Write FW data to "DATA0".
  WriteDataVal (PciIo, PCI_FW_DATA0, mFirmwareImage[Idx++]);

  // 4. Read "Set DATA1" and confirm it is '0b'.
  if (!WaitReadCtlStatVal (
         PciIo,
         PCI_FW_CTL_DATA_REG,
         PCI_FW_CTL_STAT_REG_SET_DATA1,
         0
         ))
  {
    return;
  }

  // 5. Write FW data to "DATA1".
  WriteDataVal (PciIo, PCI_FW_DATA1, mFirmwareImage[Idx++]);

  // 6. Set "Set DATA0" & "Set DATA1" to '1b'.
  WriteCtlStatVal (
    PciIo,
    PCI_FW_CTL_DATA_REG,
    PCI_FW_CTL_STAT_REG_SET_DATA0 | PCI_FW_CTL_STAT_REG_SET_DATA1
    );

  while (Idx < mFirmwareImageSize / sizeof (UINT32)) {
    // 7. Read "Set DATA0" and confirm it is '0b'.
    if (!WaitReadCtlStatVal (
           PciIo,
           PCI_FW_CTL_DATA_REG,
           PCI_FW_CTL_STAT_REG_SET_DATA0,
           0
           ))
    {
      return;
    }

    // 8. Write FW data to"DATA0". Set "Set DATA0" to '1b'.
    WriteDataVal (PciIo, PCI_FW_DATA0, mFirmwareImage[Idx++]);
    WriteCtlStatVal (PciIo, PCI_FW_CTL_DATA_REG, PCI_FW_CTL_STAT_REG_SET_DATA0);

    // 9. Read "Set DATA1" and confirm it is '0b'.
    if (!WaitReadCtlStatVal (
           PciIo,
           PCI_FW_CTL_DATA_REG,
           PCI_FW_CTL_STAT_REG_SET_DATA1,
           0
           ))
    {
      return;
    }

    // 10. Write FW data to"DATA1". Set "Set DATA1" to '1b'.
    WriteDataVal (PciIo, PCI_FW_DATA1, mFirmwareImage[Idx++]);
    WriteCtlStatVal (PciIo, PCI_FW_CTL_DATA_REG, PCI_FW_CTL_STAT_REG_SET_DATA1);

    // 11. Return to step 7 and repeat the sequence from step 7 to step 10.
  }

  // 12. After writing the last data of FW, the System Software must set "FW Download Enable" to '0b'.
  WriteCtlStatVal (PciIo, PCI_FW_CTL_STAT_REG, 0);

  // 13. Read "Result Code" and confirm it is '001b'.
  if (WaitReadCtlStatVal (
        PciIo,
        PCI_FW_CTL_STAT_REG,
        PCI_FW_CTL_STAT_REG_RESULT_CODE,
        PCI_FW_CTL_STAT_REG_RESULT_OK
        ))
  {
    DEBUG ((
      DEBUG_INFO,
      "%a: Renesas PD720202 firmware download successful\n",
      __func__
      ));
  } else {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Renesas PD720202 firmware download FAILED\n",
      __func__
      ));
  }
}

/**
  Test to see if this driver supports ControllerHandle. This service
  is called by the EFI boot service ConnectController(). In
  order to make drivers as small as possible, there are a few calling
  restrictions for this service. ConnectController() must
  follow these calling restrictions. If any other agent wishes to call
  Supported() it must also follow these calling restrictions.

   @param  This                   Protocol instance pointer.
   @param  ControllerHandle       Handle of device to test.
   @param  RemainingDevicePath    Optional parameter use to pick a specific child
                                                         device to start.

   @retval EFI_SUCCESS            This driver supports this device.
   @retval EFI_ALREADY_STARTED    This driver is already running on this device.
   @retval other                  This driver does not support this device.

**/
static
EFI_STATUS
EFIAPI
RenesasPD720202DriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  UINT32               PciID;
  UINT8                CtlStatVal;

  //
  // Check for the PCI IO Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        PCI_VENDOR_ID_OFFSET,
                        1,
                        &PciID
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Pci->Pci.Read() of vendor/device id failed (Status == %r)\n",
      __func__,
      Status
      ));
    goto CloseProtocol;
  }

  if (((PciID & 0xffff) != PCI_VENDOR_ID_RENESAS) ||
      (((PciID >> 16) != PCI_DEVICE_ID_PD720201) &&
       ((PciID >> 16) != PCI_DEVICE_ID_PD720202)))
  {
    DEBUG ((
      DEBUG_INFO,
      "%a: ignoring unsupported PCI device 0x%04x:0x%04x\n",
      __func__,
      PciID & 0xffff,
      PciID >> 16
      ));
    goto CloseProtocol;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        PCI_FW_CTL_STAT_REG,
                        1,
                        &CtlStatVal
                        );
  if (!EFI_ERROR (Status) &&
      ((CtlStatVal & PCI_FW_CTL_STAT_REG_RESULT_CODE) == PCI_FW_CTL_STAT_REG_RESULT_INVALID))
  {
    //
    // Firmware download required
    //
    DEBUG ((DEBUG_INFO, "%a: downloading firmware\n", __func__));
    DownloadPD720202Firmware (PciIo);
  }

CloseProtocol:
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  //
  // Always return unsupported: we are not interested in driving the device,
  // only in having the opportunity to install the firmware before the real
  // driver attaches to it.
  //
  return EFI_UNSUPPORTED;
}

/**
  Start this driver on Controller. Not used.

  @param [in] This                    Protocol instance pointer.
  @param [in] Controller              Handle of device to work with.
  @param [in] RemainingDevicePath     Not used, always produce all possible children.

  @retval EFI_SUCCESS                 This driver is added to Controller.
  @retval other                       This driver does not support this device.

**/
static
EFI_STATUS
EFIAPI
RenesasPD720202DriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  //
  // We are not interested in driving the device, we only poke the firmware
  // in the .Supported() callback.
  //
  ASSERT (FALSE);
  return EFI_INVALID_PARAMETER;
}

/**
  Stop this driver on Controller. Not used.

  @param [in] This                    Protocol instance pointer.
  @param [in] Controller              Handle of device to stop driver on.
  @param [in] NumberOfChildren        How many children need to be stopped.
  @param [in] ChildHandleBuffer       Not used.

  @retval EFI_SUCCESS                 This driver is removed Controller.
  @retval EFI_DEVICE_ERROR            The device could not be stopped due to a device error.
  @retval other                       This driver was not removed from this device.

**/
static
EFI_STATUS
EFIAPI
RenesasPD720202DriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  ASSERT (FALSE);
  return EFI_SUCCESS;
}

//
// UEFI Driver Model entry point
//
static EFI_DRIVER_BINDING_PROTOCOL  RenesasPD720202DriverBinding = {
  RenesasPD720202DriverSupported,
  RenesasPD720202DriverStart,
  RenesasPD720202DriverStop,

  // Version values of 0xfffffff0-0xffffffff are reserved for platform/OEM
  // specific drivers. Protocol instances with higher 'Version' properties
  // will be used before lower 'Version' ones. XhciDxe uses version 0x30,
  // so this driver will be called in preference, and XhciDxe will be invoked
  // after RenesasPD720202DriverSupported returns EFI_UNSUPPORTED.
  0xfffffff0,
  NULL,
  NULL
};

EFI_STATUS
EFIAPI
InitializeRenesasPD720202Driver (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // First, try to locate the firmware image. If it is missing, there is no
  // point in proceeding.
  //
  Status = GetSectionFromAnyFv (
             &gRenesasFirmwarePD720202ImageId,
             EFI_SECTION_RAW,
             0,
             (VOID **)(VOID **)&mFirmwareImage,
             &mFirmwareImageSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: could not locate PD720202 firmware image\n",
      __func__
      ));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  return EfiLibInstallDriverBinding (
           ImageHandle,
           SystemTable,
           &RenesasPD720202DriverBinding,
           NULL
           );
}
