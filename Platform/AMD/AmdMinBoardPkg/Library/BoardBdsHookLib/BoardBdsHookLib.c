/** @file
  This library registers Bds callbacks. It is a default library
  implementation instance of the BDS hook library

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Guid/EventGroup.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/Tcg2PhysicalPresenceLib.h>
#include <Library/IpmiBaseLib.h>
#include <Library/IpmiCommandLib.h>
#include <Protocol/BlockIo.h>
#include <Protocol/UsbIo.h>
#include <Protocol/PciEnumerationComplete.h>
#include <IndustryStandard/Ipmi.h>
#include <Library/AmdBoardBdsHookLib.h>
#include "BoardBdsHook.h"

#ifdef INTERNAL_IDS
  #include <Register/Amd/Msr.h>
#endif

CHAR16                                       *mConsoleVar[] = { L"ConIn", L"ConOut" };
GLOBAL_REMOVE_IF_UNREFERENCED EFI_BOOT_MODE  gBootMode;
BOOLEAN                                      gPPRequireUIConfirm;
extern UINTN                                 mBootMenuOptionNumber;

GLOBAL_REMOVE_IF_UNREFERENCED USB_CLASS_FORMAT_DEVICE_PATH  gUsbClassKeyboardDevicePath = {
  {
    {
      MESSAGING_DEVICE_PATH,
      MSG_USB_CLASS_DP,
      {
        (UINT8)(sizeof (USB_CLASS_DEVICE_PATH)),
        (UINT8)((sizeof (USB_CLASS_DEVICE_PATH)) >> 8)
      }
    },
    0xffff,           // VendorId
    0xffff,           // ProductId
    CLASS_HID,        // DeviceClass
    SUBCLASS_BOOT,    // DeviceSubClass
    PROTOCOL_KEYBOARD // DeviceProtocol
  },
  END_ENTIRE_DEVICE_PATH
};

#ifdef INTERNAL_IDS
EFI_STATUS
PrintSocOpnInfo (
  IN  VOID
  );

#endif

//
// BDS Platform Functions
//

/**
  Checks if Mor bit is set.
  @retval TRUE    The MOR bit is set
  @retval FALSE   The MOR bit is not set
**/
BOOLEAN
IsMorBitSet (
  VOID
  )
{
  UINTN       MorControl;
  EFI_STATUS  Status;
  UINTN       DataSize;

  //
  // Check if the MOR bit is set.
  //
  DataSize = sizeof (MorControl);
  Status   = gRT->GetVariable (
                    MEMORY_OVERWRITE_REQUEST_VARIABLE_NAME,
                    &gEfiMemoryOverwriteControlDataGuid,
                    NULL,
                    &DataSize,
                    &MorControl
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, " gEfiMemoryOverwriteControlDataGuid doesn't exist!!***\n"));
    MorControl = 0;
  } else {
    DEBUG ((DEBUG_INFO, " Get the gEfiMemoryOverwriteControlDataGuid = %x!!***\n", MorControl));
  }

  return (BOOLEAN)(MorControl & 0x01);
}

/**
  Prints device paths.
  @param[in] Name           The device name.
  @param[in] DevicePath     The device path to be printed
**/
VOID
EFIAPI
DumpDevicePath (
  IN CHAR16           *Name,
  IN EFI_DEVICE_PATH  *DevicePath
  )
{
  CHAR16  *Str;

  Str = ConvertDevicePathToText (DevicePath, TRUE, TRUE);
  DEBUG ((DEBUG_INFO, "%s: %s\n", Name, Str));
  if (Str != NULL) {
    FreePool (Str);
  }
}

/**
  Return whether the device is trusted console.

  @param[in]  ConsoleType  The console type.
  @param[in]  Device       The device path to be tested.

  @retval     TRUE   The device can be trusted.
  @retval     FALSE  The device cannot be trusted.
**/
BOOLEAN
IsTrustedConsole (
  IN CONSOLE_TYPE              ConsoleType,
  IN EFI_DEVICE_PATH_PROTOCOL  *Device
  )
{
  VOID                      *TrustedConsoleDevicepath;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  UINTN                     Size;
  EFI_DEVICE_PATH_PROTOCOL  *ConsoleDevice;

  if (Device == NULL) {
    return FALSE;
  }

  ConsoleDevice = DuplicateDevicePath (Device);

  TrustedConsoleDevicepath = NULL;

  switch (ConsoleType) {
    case ConIn:
      TrustedConsoleDevicepath = DuplicateDevicePath (PcdGetPtr (PcdTrustedConsoleInputDevicePath));
      break;
    case ConOut:
      //
      // Check GOP and remove last node
      //
      TempDevicePath = ConsoleDevice;
      while (!IsDevicePathEndType (TempDevicePath)) {
        if ((DevicePathType (TempDevicePath) == ACPI_DEVICE_PATH) &&
            (DevicePathSubType (TempDevicePath) == ACPI_ADR_DP))
        {
          SetDevicePathEndNode (TempDevicePath);
          break;
        }

        TempDevicePath = NextDevicePathNode (TempDevicePath);
      }

      TrustedConsoleDevicepath = DuplicateDevicePath (PcdGetPtr (PcdTrustedConsoleOutputDevicePath));
      break;
    default:
      ASSERT (FALSE);
      break;
  }

  TempDevicePath = TrustedConsoleDevicepath;
  do {
    Instance = GetNextDevicePathInstance (&TempDevicePath, &Size);
    if (Instance == NULL) {
      break;
    }

    if (CompareMem (ConsoleDevice, Instance, Size - END_DEVICE_PATH_LENGTH) == 0) {
      FreePool (Instance);
      FreePool (ConsoleDevice);
      if (TempDevicePath != NULL) {
        FreePool (TempDevicePath);
      }

      return TRUE;
    }

    FreePool (Instance);
  } while (TempDevicePath != NULL);

  FreePool (ConsoleDevice);
  if (TempDevicePath != NULL) {
    FreePool (TempDevicePath);
  }

  return FALSE;
}

/**
  Return whether the USB device path is in a short form.

  @param[in] DevicePath  The device path to be tested.

  @retval TRUE   The device path is in short form.
  @retval FALSE  The device path is not in short form.
**/
BOOLEAN
IsUsbShortForm (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  if ((DevicePathType (DevicePath) == MESSAGING_DEVICE_PATH) &&
      ((DevicePathSubType (DevicePath) == MSG_USB_CLASS_DP) ||
       (DevicePathSubType (DevicePath) == MSG_USB_WWID_DP)))
  {
    return TRUE;
  }

  return FALSE;
}

/**
  Connect the USB short form device path.

  @param[in] DevicePath   USB short form device path

  @retval EFI_SUCCESS           Successfully connected the USB device
  @retval EFI_NOT_FOUND         Cannot connect the USB device
  @retval EFI_INVALID_PARAMETER The device path is invalid.
**/
EFI_STATUS
ConnectUsbShortFormDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_STATUS           Status;
  EFI_HANDLE           *Handles;
  UINTN                HandleCount;
  UINTN                Index;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  UINT8                Class[3];
  BOOLEAN              AtLeastOneConnected;

  //
  // Check the passed in parameters
  //
  if (DevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IsUsbShortForm (DevicePath)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Find the usb host controller firstly, then connect with the remaining device path
  //
  AtLeastOneConnected = FALSE;
  Status              = gBS->LocateHandleBuffer (
                               ByProtocol,
                               &gEfiPciIoProtocolGuid,
                               NULL,
                               &HandleCount,
                               &Handles
                               );
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    Handles[Index],
                    &gEfiPciIoProtocolGuid,
                    (VOID **)&PciIo
                    );
    if (!EFI_ERROR (Status)) {
      //
      // Check whether the Pci device is the wanted usb host controller
      //
      Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint8, 0x09, 3, &Class);
      if (!EFI_ERROR (Status) &&
          ((PCI_CLASS_SERIAL == Class[2]) && (PCI_CLASS_SERIAL_USB == Class[1])))
      {
        Status = gBS->ConnectController (
                        Handles[Index],
                        NULL,
                        DevicePath,
                        FALSE
                        );
        if (!EFI_ERROR (Status)) {
          AtLeastOneConnected = TRUE;
        }
      }
    }
  }

  return AtLeastOneConnected ? EFI_SUCCESS : EFI_NOT_FOUND;
}

/**
  Return whether the Handle is a vga handle.

  @param[in] Handle  The handle to be tested.

  @retval TRUE   The handle is a vga handle.
  @retval FALSE  The handle is not a vga handle..
**/
BOOLEAN
IsVgaHandle (
  IN EFI_HANDLE  Handle
  )
{
  EFI_PCI_IO_PROTOCOL  *PciIo;
  PCI_TYPE00           Pci;
  EFI_STATUS           Status;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo
                  );
  if (!EFI_ERROR (Status)) {
    Status = PciIo->Pci.Read (
                          PciIo,
                          EfiPciIoWidthUint32,
                          0,
                          sizeof (Pci) / sizeof (UINT32),
                          &Pci
                          );
    if (!EFI_ERROR (Status)) {
      if (IS_PCI_VGA (&Pci) || IS_PCI_OLD_VGA (&Pci)) {
        return TRUE;
      }
    }
  }

  return FALSE;
}

/**
  Return whether the device path points to a video controller.

  @param[in] DevicePath  The device path to be tested.

  @retval TRUE   The device path points to a video controller.
  @retval FALSE  The device path does not point to a video controller.
**/
EFI_HANDLE
IsVideoController (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DupDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;
  EFI_STATUS                Status;
  EFI_HANDLE                DeviceHandle;

  DupDevicePath = DuplicateDevicePath (DevicePath);
  ASSERT (DupDevicePath != NULL);
  if (DupDevicePath == NULL) {
    return NULL;
  }

  TempDevicePath = DupDevicePath;
  Status         = gBS->LocateDevicePath (
                          &gEfiDevicePathProtocolGuid,
                          &TempDevicePath,
                          &DeviceHandle
                          );
  FreePool (DupDevicePath);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  if (IsVgaHandle (DeviceHandle)) {
    return DeviceHandle;
  } else {
    return NULL;
  }
}

/**
  Return whether the device path is a GOP device path.

  @param[in] DevicePath  The device path to be tested.

  @retval TRUE   The device path is a GOP device path.
  @retval FALSE  The device on the device path is not a GOP device path.
**/
BOOLEAN
IsGopDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  while (!IsDevicePathEndType (DevicePath)) {
    if ((DevicePathType (DevicePath) == ACPI_DEVICE_PATH) &&
        (DevicePathSubType (DevicePath) == ACPI_ADR_DP))
    {
      return TRUE;
    }

    DevicePath = NextDevicePathNode (DevicePath);
  }

  return FALSE;
}

/**
  Remove all GOP device path instance from DevicePath and add the Gop to the DevicePath.

  @param[in] DevicePath  The device path to be removed
  @param[in] Gop         The device path to be added.

  @retval Return     The updated device path.
**/
EFI_DEVICE_PATH_PROTOCOL *
UpdateGopDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN EFI_DEVICE_PATH_PROTOCOL  *Gop
  )
{
  UINTN                     Size;
  UINTN                     GopSize;
  EFI_DEVICE_PATH_PROTOCOL  *Temp;
  EFI_DEVICE_PATH_PROTOCOL  *Return;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  BOOLEAN                   Exist;

  Exist   = FALSE;
  Return  = NULL;
  GopSize = GetDevicePathSize (Gop);
  do {
    Instance = GetNextDevicePathInstance (&DevicePath, &Size);
    if (Instance == NULL) {
      break;
    }

    if (!IsGopDevicePath (Instance) ||
        ((Size == GopSize) && (CompareMem (Instance, Gop, GopSize) == 0))
        )
    {
      if ((Size == GopSize) && (CompareMem (Instance, Gop, GopSize) == 0)) {
        Exist = TRUE;
      }

      Temp   = Return;
      Return = AppendDevicePathInstance (Return, Instance);
      if (Temp != NULL) {
        FreePool (Temp);
      }
    }

    FreePool (Instance);
  } while (DevicePath != NULL);

  if (!Exist) {
    Temp   = Return;
    Return = AppendDevicePathInstance (Return, Gop);
    if (Temp != NULL) {
      FreePool (Temp);
    }
  }

  return Return;
}

/**
  Get Graphics Controller Handle. VgaDevices needs to be freed by caller.

  @param[in] NeedTrustedConsole     The flag to determine if trusted console
                                    or non trusted console should be returned
  @param[out] VgaDevices            Array of handles corresponding to
                                    valid VGA devices
  @param[out] VgaDevicesCount       Number of VGA devices found and placed in
                                    VgaDevices

  @retval EFI ERROR                 No VGA capable devices found
  @retval EFI_SUCCESS               At least one VGA device found
**/
EFI_STATUS
EFIAPI
GetGraphicsController (
  IN  BOOLEAN     NeedTrustedConsole,
  OUT EFI_HANDLE  **VgaDevices,
  OUT UINTN       *VgaDevicesCount
  )
{
  EFI_STATUS                Status;
  UINTN                     Index;
  EFI_HANDLE                *PciHandles;
  UINTN                     PciHandlesSize;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINT32                    NumDevices;

  if ((VgaDevicesCount == NULL) || (VgaDevices == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  NumDevices = 0;
  Status     = gBS->LocateHandleBuffer (
                      ByProtocol,
                      &gEfiPciIoProtocolGuid,
                      NULL,
                      &PciHandlesSize,
                      &PciHandles
                      );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *VgaDevices = AllocateZeroPool (sizeof (EFI_HANDLE) * PciHandlesSize);
  if (VgaDevices == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < PciHandlesSize; Index++) {
    Status = gBS->HandleProtocol (
                    PciHandles[Index],
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&DevicePath
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (!IsVgaHandle (PciHandles[Index])) {
      continue;
    }

    if ((NeedTrustedConsole && IsTrustedConsole (ConOut, DevicePath)) ||
        ((!NeedTrustedConsole) && (!IsTrustedConsole (ConOut, DevicePath))))
    {
      VgaDevices[0][NumDevices] = PciHandles[Index];
      NumDevices++;
    }
  }

  *VgaDevicesCount = NumDevices;
  if (NumDevices > 0) {
    return EFI_SUCCESS;
  } else {
    return EFI_NOT_FOUND;
  }
}

/**
  Updates Graphic ConOut variable. This function searches for all VGA capable output devices and
  adds them to the ConOut variable.

  @param[in] NeedTrustedConsole    The flag that determines if trusted console
  or non trusted console should be returned
**/
VOID
UpdateGraphicConOut (
  IN BOOLEAN  NeedTrustedConsole
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *GopDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *ConOutDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *UpdatedConOutDevicePath;
  EFI_HANDLE                *VgaDevices;
  UINTN                     VgaDevicesCount;
  UINTN                     Count;
  EFI_STATUS                Status;

  Count           = 0;
  VgaDevicesCount = 0;

  //
  // Update ConOut variable
  //
  Status = GetGraphicsController (NeedTrustedConsole, &VgaDevices, &VgaDevicesCount);
  if (Status == EFI_SUCCESS) {
    GetEfiGlobalVariable2 (L"ConOut", (VOID **)&ConOutDevicePath, NULL);
    if (ConOutDevicePath != NULL) {
      DumpDevicePath (L"Original ConOut variable", ConOutDevicePath);
      FreePool (ConOutDevicePath);
    }

    // Add VGA devices to ConOut
    for (Count = 0; Count < VgaDevicesCount; Count++) {
      //
      // Connect the GOP driver
      //
      gBS->ConnectController (VgaDevices[Count], NULL, NULL, TRUE);
      //
      // Get the GOP device path
      // NOTE: We may get a device path that contains Controller node in it.
      //
      GopDevicePath = EfiBootManagerGetGopDevicePath (VgaDevices[Count]);
      if (GopDevicePath != NULL) {
        GetEfiGlobalVariable2 (L"ConOut", (VOID **)&ConOutDevicePath, NULL);
        if (ConOutDevicePath != NULL) {
          UpdatedConOutDevicePath = UpdateGopDevicePath (ConOutDevicePath, GopDevicePath);
          if (UpdatedConOutDevicePath != NULL) {
            gRT->SetVariable (
                   L"ConOut",
                   &gEfiGlobalVariableGuid,
                   EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                   GetDevicePathSize (UpdatedConOutDevicePath),
                   UpdatedConOutDevicePath
                   );
            FreePool (UpdatedConOutDevicePath);
          }

          FreePool (ConOutDevicePath);
        }

        FreePool (GopDevicePath);
      }
    }

    FreePool (VgaDevices);
  }

  GetEfiGlobalVariable2 (L"ConOut", (VOID **)&ConOutDevicePath, NULL);
  if (ConOutDevicePath != NULL) {
    DumpDevicePath (L"Final ConOut variable", ConOutDevicePath);
    FreePool (ConOutDevicePath);
  }
}

/**
  The function connects the trusted consoles.
**/
VOID
ConnectTrustedConsole (
  VOID
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *Consoles;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  EFI_DEVICE_PATH_PROTOCOL  *Next;
  UINTN                     Size;
  UINTN                     Index;
  EFI_HANDLE                Handle;
  EFI_STATUS                Status;
  VOID                      *TrustedConsoleDevicepath;

  TrustedConsoleDevicepath = PcdGetPtr (PcdTrustedConsoleInputDevicePath);
  DumpDevicePath (L"TrustedConsoleIn", TrustedConsoleDevicepath);
  TrustedConsoleDevicepath = PcdGetPtr (PcdTrustedConsoleOutputDevicePath);
  DumpDevicePath (L"TrustedConsoleOut", TrustedConsoleDevicepath);

  for (Index = 0; Index < sizeof (mConsoleVar) / sizeof (mConsoleVar[0]); Index++) {
    GetEfiGlobalVariable2 (mConsoleVar[Index], (VOID **)&Consoles, NULL);

    TempDevicePath = Consoles;
    do {
      Instance = GetNextDevicePathInstance (&TempDevicePath, &Size);
      if (Instance == NULL) {
        break;
      }

      if (IsTrustedConsole (Index, Instance)) {
        if (IsUsbShortForm (Instance)) {
          ConnectUsbShortFormDevicePath (Instance);
        } else {
          for (Next = Instance; !IsDevicePathEnd (Next); Next = NextDevicePathNode (Next)) {
            if ((DevicePathType (Next) == ACPI_DEVICE_PATH) && (DevicePathSubType (Next) == ACPI_ADR_DP)) {
              break;
            } else if ((DevicePathType (Next) == HARDWARE_DEVICE_PATH) &&
                       (DevicePathSubType (Next) == HW_CONTROLLER_DP) &&
                       (DevicePathType (NextDevicePathNode (Next)) == ACPI_DEVICE_PATH) &&
                       (DevicePathSubType (NextDevicePathNode (Next)) == ACPI_ADR_DP)
                       )
            {
              break;
            }
          }

          if (!IsDevicePathEnd (Next)) {
            SetDevicePathEndNode (Next);
            Status = EfiBootManagerConnectDevicePath (Instance, &Handle);
            if (!EFI_ERROR (Status)) {
              gBS->ConnectController (Handle, NULL, NULL, TRUE);
            }
          } else {
            EfiBootManagerConnectDevicePath (Instance, NULL);
          }
        }
      }

      FreePool (Instance);
    } while (TempDevicePath != NULL);

    if (Consoles != NULL) {
      FreePool (Consoles);
    }
  }
}

/**
  The function connects the trusted Storages.
**/
VOID
ConnectTrustedStorage (
  VOID
  )
{
  VOID                      *TrustedStorageDevicepath;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *Instance;
  UINTN                     Size;
  EFI_DEVICE_PATH_PROTOCOL  *TempStorageDevicePath;
  EFI_STATUS                Status;
  EFI_HANDLE                DeviceHandle;

  TrustedStorageDevicepath = DuplicateDevicePath (PcdGetPtr (PcdTrustedStorageDevicePath));
  DumpDevicePath (L"TrustedStorage", TrustedStorageDevicepath);

  TempDevicePath = TrustedStorageDevicepath;
  do {
    Instance = GetNextDevicePathInstance (&TempDevicePath, &Size);
    if (Instance == NULL) {
      break;
    }

    EfiBootManagerConnectDevicePath (Instance, NULL);

    TempStorageDevicePath = Instance;

    Status = gBS->LocateDevicePath (
                    &gEfiDevicePathProtocolGuid,
                    &TempStorageDevicePath,
                    &DeviceHandle
                    );
    if (!EFI_ERROR (Status)) {
      gBS->ConnectController (DeviceHandle, NULL, NULL, FALSE);
    }

    FreePool (Instance);
  } while (TempDevicePath != NULL);

  if (TempDevicePath != NULL) {
    FreePool (TempDevicePath);
  }
}

/**
  Check if current BootCurrent variable is internal shell boot option.

  @retval  TRUE         BootCurrent is internal shell.
  @retval  FALSE        BootCurrent is not internal shell.
**/
BOOLEAN
BootCurrentIsInternalShell (
  VOID
  )
{
  UINTN                     VarSize;
  UINT16                    BootCurrent;
  CHAR16                    BootOptionName[16];
  UINT8                     *BootOption;
  UINT8                     *Ptr;
  BOOLEAN                   Result;
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *TempDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *LastDeviceNode;
  EFI_GUID                  *GuidPoint;

  BootOption = NULL;
  Result     = FALSE;

  //
  // Get BootCurrent variable
  //
  VarSize = sizeof (UINT16);
  Status  = gRT->GetVariable (
                   L"BootCurrent",
                   &gEfiGlobalVariableGuid,
                   NULL,
                   &VarSize,
                   &BootCurrent
                   );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  //
  // Create boot option Bootxxxx from BootCurrent
  //
  UnicodeSPrint (BootOptionName, sizeof (BootOptionName), L"Boot%04X", BootCurrent);

  GetEfiGlobalVariable2 (BootOptionName, (VOID **)&BootOption, &VarSize);
  if ((BootOption == NULL) || (VarSize == 0)) {
    return FALSE;
  }

  Ptr            = BootOption;
  Ptr           += sizeof (UINT32);
  Ptr           += sizeof (UINT16);
  Ptr           += StrSize ((CHAR16 *)Ptr);
  TempDevicePath = (EFI_DEVICE_PATH_PROTOCOL *)Ptr;
  LastDeviceNode = TempDevicePath;
  while (!IsDevicePathEnd (TempDevicePath)) {
    LastDeviceNode = TempDevicePath;
    TempDevicePath = NextDevicePathNode (TempDevicePath);
  }

  GuidPoint = EfiGetNameGuidFromFwVolDevicePathNode (
                (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)LastDeviceNode
                );
  if ((GuidPoint != NULL) &&
      ((CompareGuid (GuidPoint, &gUefiShellFileGuid))))
  {
    //
    // if this option is internal shell, return TRUE
    //
    Result = TRUE;
  }

  if (BootOption != NULL) {
    FreePool (BootOption);
    BootOption = NULL;
  }

  return Result;
}

/**
  This function will change video resolution and text mode
  for internal shell when internal shell is launched.

  @param   None.

  @retval  EFI_SUCCESS  Mode is changed successfully.
  @retval  Others       Mode failed to changed.
**/
EFI_STATUS
EFIAPI
ChangeModeForInternalShell (
  VOID
  )
{
  EFI_GRAPHICS_OUTPUT_PROTOCOL          *GraphicsOutput;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL       *SimpleTextOut;
  UINTN                                 SizeOfInfo;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;
  UINT32                                MaxGopMode;
  UINT32                                MaxTextMode;
  UINT32                                ModeNumber;
  UINTN                                 HandleCount;
  EFI_HANDLE                            *HandleBuffer;
  EFI_STATUS                            Status;
  UINTN                                 Index;
  UINTN                                 CurrentColumn;
  UINTN                                 CurrentRow;

  //
  // Internal shell mode
  //
  UINT32  mShellModeColumn;
  UINT32  mShellModeRow;
  UINT32  mShellHorizontalResolution;
  UINT32  mShellVerticalResolution;

  //
  // Get user defined text mode for internal shell only once.
  //
  mShellHorizontalResolution = PcdGet32 (PcdSetupVideoHorizontalResolution);
  mShellVerticalResolution   = PcdGet32 (PcdSetupVideoVerticalResolution);
  mShellModeColumn           = PcdGet32 (PcdSetupConOutColumn);
  mShellModeRow              = PcdGet32 (PcdSetupConOutRow);

  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&GraphicsOutput
                  );
  if (EFI_ERROR (Status)) {
    GraphicsOutput = NULL;
  }

  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiSimpleTextOutProtocolGuid,
                  (VOID **)&SimpleTextOut
                  );
  if (EFI_ERROR (Status)) {
    SimpleTextOut = NULL;
  }

  if ((GraphicsOutput == NULL) || (SimpleTextOut == NULL)) {
    return EFI_UNSUPPORTED;
  }

  MaxGopMode  = GraphicsOutput->Mode->MaxMode;
  MaxTextMode = SimpleTextOut->Mode->MaxMode;

  //
  // 1. If current video resolution is same with new video resolution,
  //    video resolution need not be changed.
  //    1.1. If current text mode is same with new text mode, text mode need not be change.
  //    1.2. If current text mode is different with new text mode, text mode need be change to new text mode.
  // 2. If current video resolution is different with new video resolution, we need restart whole console drivers.
  //
  for (ModeNumber = 0; ModeNumber < MaxGopMode; ModeNumber++) {
    Status = GraphicsOutput->QueryMode (
                               GraphicsOutput,
                               ModeNumber,
                               &SizeOfInfo,
                               &Info
                               );
    if (!EFI_ERROR (Status)) {
      if ((Info->HorizontalResolution == mShellHorizontalResolution) &&
          (Info->VerticalResolution == mShellVerticalResolution))
      {
        if ((GraphicsOutput->Mode->Info->HorizontalResolution == mShellHorizontalResolution) &&
            (GraphicsOutput->Mode->Info->VerticalResolution == mShellVerticalResolution))
        {
          //
          // If current video resolution is same with new resolution,
          // then check if current text mode is same with new text mode.
          //
          Status = SimpleTextOut->QueryMode (SimpleTextOut, SimpleTextOut->Mode->Mode, &CurrentColumn, &CurrentRow);
          ASSERT_EFI_ERROR (Status);
          if ((CurrentColumn == mShellModeColumn) && (CurrentRow == mShellModeRow)) {
            //
            // Current text mode is same with new text mode, text mode need not be change.
            //
            FreePool (Info);
            return EFI_SUCCESS;
          } else {
            //
            // Current text mode is different with new text mode, text mode need be change to new text mode.
            //
            for (Index = 0; Index < MaxTextMode; Index++) {
              Status = SimpleTextOut->QueryMode (SimpleTextOut, Index, &CurrentColumn, &CurrentRow);
              if (!EFI_ERROR (Status)) {
                if ((CurrentColumn == mShellModeColumn) && (CurrentRow == mShellModeRow)) {
                  //
                  // New text mode is supported, set it.
                  //
                  Status = SimpleTextOut->SetMode (SimpleTextOut, Index);
                  ASSERT_EFI_ERROR (Status);
                  //
                  // Update text mode PCD.
                  //
                  Status = PcdSet32S (PcdConOutColumn, mShellModeColumn);
                  ASSERT_EFI_ERROR (Status);

                  Status = PcdSet32S (PcdConOutRow, mShellModeRow);
                  ASSERT_EFI_ERROR (Status);

                  FreePool (Info);
                  return EFI_SUCCESS;
                }
              }
            }

            if (Index == MaxTextMode) {
              //
              // If new text mode is not supported, return error.
              //
              FreePool (Info);
              return EFI_UNSUPPORTED;
            }
          }
        } else {
          FreePool (Info);
          //
          // If current video resolution is not same with the new one, set new video resolution.
          // In this case, the driver which produces simple text out need be restarted.
          //
          Status = GraphicsOutput->SetMode (GraphicsOutput, ModeNumber);
          if (!EFI_ERROR (Status)) {
            //
            // Set PCD to restart GraphicsConsole and Consplitter to change video resolution
            // and produce new text mode based on new resolution.
            //
            Status = PcdSet32S (PcdVideoHorizontalResolution, mShellHorizontalResolution);
            ASSERT_EFI_ERROR (Status);

            Status = PcdSet32S (PcdVideoVerticalResolution, mShellVerticalResolution);
            ASSERT_EFI_ERROR (Status);

            Status = PcdSet32S (PcdConOutColumn, mShellModeColumn);
            ASSERT_EFI_ERROR (Status);

            Status = PcdSet32S (PcdConOutRow, mShellModeRow);
            ASSERT_EFI_ERROR (Status);

            Status = gBS->LocateHandleBuffer (
                            ByProtocol,
                            &gEfiSimpleTextOutProtocolGuid,
                            NULL,
                            &HandleCount,
                            &HandleBuffer
                            );
            if (!EFI_ERROR (Status)) {
              for (Index = 0; Index < HandleCount; Index++) {
                gBS->DisconnectController (HandleBuffer[Index], NULL, NULL);
              }

              for (Index = 0; Index < HandleCount; Index++) {
                gBS->ConnectController (HandleBuffer[Index], NULL, NULL, TRUE);
              }

              if (HandleBuffer != NULL) {
                FreePool (HandleBuffer);
              }

              break;
            }
          }
        }
      }

      FreePool (Info);
    }
  }

  if (ModeNumber == MaxGopMode) {
    //
    // If the new resolution is not supported, return error.
    //
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  The function connects the trusted consoles and then call the PP processing library interface.
**/
VOID
ProcessTcgPp (
  VOID
  )
{
  gPPRequireUIConfirm |= Tcg2PhysicalPresenceLibNeedUserConfirm ();

  if (gPPRequireUIConfirm) {
    ConnectTrustedConsole ();
  }

  Tcg2PhysicalPresenceLibProcessRequest (NULL);
}

/**
  The function connects the trusted storage to perform TPerReset.
**/
VOID
ProcessTcgMor (
  VOID
  )
{
  if (IsMorBitSet ()) {
    ConnectTrustedConsole ();
    ConnectTrustedStorage ();
  }
}

/**
  Update the ConIn variable with USB Keyboard device path,if its not already exists in ConIn.
**/
VOID
EnumUsbKeyboard (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "[EnumUsbKeyboard]\n"));
  EfiBootManagerUpdateConsoleVariable (ConIn, (EFI_DEVICE_PATH_PROTOCOL *)&gUsbClassKeyboardDevicePath, NULL);
  //
  // Append Usb Keyboard short form DevicePath into "ConInDev"
  //
  EfiBootManagerUpdateConsoleVariable (ConInDev, (EFI_DEVICE_PATH_PROTOCOL *)&gUsbClassKeyboardDevicePath, NULL);
}

/**
  Connect with predeined platform connect sequence,
  the OEM/IBV can customize with their own connect sequence.

  @param[in] BootMode          Boot mode of this boot.
**/
VOID
ConnectSequence (
  IN EFI_BOOT_MODE  BootMode
  )
{
  EfiBootManagerConnectAll ();
}

/**
  Connects Root Bridge.

  @param[in] Recursive  Recursively connect controllers if TRUE
 **/
VOID
ConnectRootBridge (
  IN BOOLEAN  Recursive
  )
{
  UINTN       RootBridgeHandleCount;
  EFI_HANDLE  *RootBridgeHandleBuffer;
  UINTN       RootBridgeIndex;

  RootBridgeHandleCount = 0;
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiPciRootBridgeIoProtocolGuid,
         NULL,
         &RootBridgeHandleCount,
         &RootBridgeHandleBuffer
         );
  for (RootBridgeIndex = 0; RootBridgeIndex < RootBridgeHandleCount; RootBridgeIndex++) {
    gBS->ConnectController (RootBridgeHandleBuffer[RootBridgeIndex], NULL, NULL, Recursive);
  }
}

/**
  Add console variable device paths.

  @param[in] ConsoleType         ConIn or ConOut
  @param[in] ConsoleDevicePath   Device path to be added
**/
VOID
AddConsoleVariable (
  IN CONSOLE_TYPE     ConsoleType,
  IN EFI_DEVICE_PATH  *ConsoleDevicePath
  )
{
  EFI_DEVICE_PATH  *TempDevicePath;
  EFI_DEVICE_PATH  *Instance;
  UINTN            Size;
  EFI_HANDLE       GraphicsControllerHandle;
  EFI_DEVICE_PATH  *GopDevicePath;

  TempDevicePath = DuplicateDevicePath (ConsoleDevicePath);
  do {
    Instance = GetNextDevicePathInstance (&TempDevicePath, &Size);
    if (Instance == NULL) {
      break;
    }

    switch (ConsoleType) {
      case ConIn:
        if (IsUsbShortForm (Instance)) {
          //
          // Append Usb Keyboard short form DevicePath into "ConInDev"
          //
          EfiBootManagerUpdateConsoleVariable (ConInDev, Instance, NULL);
        }

        EfiBootManagerUpdateConsoleVariable (ConsoleType, Instance, NULL);
        break;
      case ConOut:
        GraphicsControllerHandle = IsVideoController (Instance);
        if (GraphicsControllerHandle == NULL) {
          EfiBootManagerUpdateConsoleVariable (ConsoleType, Instance, NULL);
        } else {
          //
          // Connect the GOP driver
          //
          gBS->ConnectController (GraphicsControllerHandle, NULL, NULL, TRUE);
          //
          // Get the GOP device path
          // NOTE: We may get a device path that contains Controller node in it.
          //
          GopDevicePath = EfiBootManagerGetGopDevicePath (GraphicsControllerHandle);
          if (GopDevicePath != NULL) {
            EfiBootManagerUpdateConsoleVariable (ConsoleType, GopDevicePath, NULL);
          }
        }

        break;
      default:
        ASSERT (FALSE);
        break;
    }

    FreePool (Instance);
  } while (TempDevicePath != NULL);

  if (TempDevicePath != NULL) {
    FreePool (TempDevicePath);
  }
}

/**
  This is the callback function for PCI ENUMERATION COMPLETE.

  @param[in] Event      The Event this notify function registered to.
  @param[in] Context    Pointer to the context data registered to the Event.
**/
VOID
EFIAPI
BdsPciEnumCompleteCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                Status;
  VOID                      *ProtocolPointer;
  EFI_DEVICE_PATH_PROTOCOL  *VarConOut;
  EFI_DEVICE_PATH_PROTOCOL  *VarConIn;

  Status = EFI_SUCCESS;

  //
  // Check if this is first time called by EfiCreateProtocolNotifyEvent() or not,
  // if it is, we will skip it until real event is triggered
  //
  Status = gBS->LocateProtocol (&gEfiPciEnumerationCompleteProtocolGuid, NULL, (VOID **)&ProtocolPointer);
  if (EFI_SUCCESS != Status) {
    return;
  }

  // gBS->CloseEvent (Event);

  DEBUG ((DEBUG_INFO, "Event BdsPciEnumCompleteCallback callback starts\n"));

  gBootMode = GetBootModeHob ();

  //
  // Fill ConIn/ConOut in Full Configuration boot mode
  //
  DEBUG ((DEBUG_INFO, "PlatformBootManagerInit - %x\n", gBootMode));

  if ((gBootMode == BOOT_WITH_FULL_CONFIGURATION) ||
      (gBootMode == BOOT_WITH_DEFAULT_SETTINGS) ||
      (gBootMode == BOOT_WITH_FULL_CONFIGURATION_PLUS_DIAGNOSTICS) ||
      (gBootMode == BOOT_IN_RECOVERY_MODE))
  {
    GetEfiGlobalVariable2 (L"ConOut", (VOID **)&VarConOut, NULL);
    if (VarConOut != NULL) {
      FreePool (VarConOut);
    }

    GetEfiGlobalVariable2 (L"ConIn", (VOID **)&VarConIn, NULL);
    if (VarConIn  != NULL) {
      FreePool (VarConIn);
    }

    //
    // Only fill ConIn/ConOut when ConIn/ConOut is empty because we may drop to Full Configuration boot mode in non-first boot
    //
    if ((VarConOut == NULL) || (VarConIn == NULL)) {
      if (PcdGetSize (PcdTrustedConsoleOutputDevicePath) >= sizeof (EFI_DEVICE_PATH_PROTOCOL)) {
        AddConsoleVariable (ConOut, PcdGetPtr (PcdTrustedConsoleOutputDevicePath));
      }

      if (PcdGetSize (PcdTrustedConsoleInputDevicePath) >= sizeof (EFI_DEVICE_PATH_PROTOCOL)) {
        AddConsoleVariable (ConIn, PcdGetPtr (PcdTrustedConsoleInputDevicePath));
      }
    }
  }

  //
  // Enumerate USB keyboard
  //
  EnumUsbKeyboard ();

  //
  // For trusted console it must be handled here.
  //
  UpdateGraphicConOut (TRUE);

  //
  // Register Boot Options
  //
  RegisterDefaultBootOption ();

  //
  // Register Static Hot keys
  //
  RegisterStaticHotkey ();

  //
  // Process Physical Preo
  //
  PERF_START_EX (NULL, "EventRec", NULL, AsmReadTsc (), 0x7010);
  if (PcdGetBool (PcdTpm2Enable)) {
    ProcessTcgPp ();
    ProcessTcgMor ();
  }

  PERF_END_EX (NULL, "EventRec", NULL, AsmReadTsc (), 0x7011);

  //
  // Perform memory test
  // We should make all UEFI memory and GCD information populated before ExitPmAuth.
  // SMM may consume these information.
  //
  MemoryTest ((EXTENDMEM_COVERAGE_LEVEL)PcdGet32 (PcdPlatformMemoryCheckLevel));
}

/**
  This is the callback function for Smm Ready To Lock.

  @param[in] Event      The Event this notify function registered to.
  @param[in] Context    Pointer to the context data registered to the Event.
**/
VOID
EFIAPI
BdsSmmReadyToLockCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  VOID        *ProtocolPointer;
  EFI_STATUS  Status;

  //
  // Check if this is first time called by EfiCreateProtocolNotifyEvent() or not,
  // if it is, we will skip it until real event is triggered
  //
  Status = gBS->LocateProtocol (&gEfiDxeSmmReadyToLockProtocolGuid, NULL, (VOID **)&ProtocolPointer);
  if (EFI_SUCCESS != Status) {
    return;
  }

  DEBUG ((DEBUG_INFO, "Event gEfiDxeSmmReadyToLockProtocolGuid callback starts\n"));

  //
  // Dispatch the deferred 3rd party images.
  //
  EfiBootManagerDispatchDeferredImages ();

  //
  // For non-trusted console it must be handled here.
  //
  UpdateGraphicConOut (FALSE);
}

/**
  ReadyToBoot callback to set video and text mode for internal shell boot.
  That will not connect USB controller while CSM and FastBoot are disabled, we need to connect them
  before booting to Shell for showing USB devices in Shell.

  When FastBoot is enabled and Windows Console is the chosen Console behavior, input devices will not be connected
  by default. Hence, when booting to EFI shell, connecting input consoles are required.

  @param[in]  Event   Pointer to this event
  @param[in]  Context Event handler private data
**/
VOID
EFIAPI
BdsReadyToBootCallback (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  DEBUG ((DEBUG_INFO, "BdsReadyToBootCallback\n"));

  if (BootCurrentIsInternalShell ()) {
    ChangeModeForInternalShell ();
    EfiBootManagerConnectAllDefaultConsoles ();
    gDS->Dispatch ();
  }
}

/**
  Before console after trusted console event callback

  @param[in] Event      The Event this notify function registered to.
  @param[in] Context    Pointer to the context data registered to the Event.
**/
VOID
EFIAPI
BdsBeforeConsoleAfterTrustedConsoleCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  DEBUG ((DEBUG_INFO, "Event gBdsEventBeforeConsoleBeforeEndOfDxeGuid callback starts\n"));

  //
  // Connect Root Bridge to make PCI BAR resource allocated and all PciIo created
  //
  ConnectRootBridge (FALSE);
}

/**
  Before console before end of DXE event callback

  @param[in] Event      The Event this notify function registered to.
  @param[in] Context    Pointer to the context data registered to the Event.
**/
VOID
EFIAPI
BdsBeforeConsoleBeforeEndOfDxeGuidCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  DEBUG ((DEBUG_INFO, "Event gBdsBeforeConsoleBeforeEndOfDxeGuid callback starts\n"));
}

/**
  Handles possible IPMI boot overrides by modifying the LoadOptions variable.
  Uses sorting function installed in BootOptionPriorityProtocol if the protocol
  is installed and a valid IPMI override is detected.

  @retval  EFI_SUCCESS    Boot override successful, or not necessary.
  @retval  EFI_NOT_FOUND  Attempted to override boot to an unsupported boot option.
**/
EFI_STATUS
HandleIpmiBootOverride (
  VOID
  )
{
  UINT8                                        NvIpmiBootOverride;
  UINT8                                        Index;
  UINT8                                        CurrentIpmiBootOverride;
  UINT8                                        *GetBootOptionsBuffer;
  UINT8                                        *SetBootOptionsBuffer;
  UINTN                                        BootOptionCount;
  UINTN                                        DataSize;
  IPMI_GET_BOOT_OPTIONS_REQUEST                BootOptionsRequest;
  IPMI_GET_BOOT_OPTIONS_RESPONSE               *BootOptionsResponse;
  IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_5       *BootOptionsParameterData;
  IPMI_SET_BOOT_OPTIONS_REQUEST                *SetBootOptionsRequest;
  IPMI_SET_BOOT_OPTIONS_RESPONSE               SetBootOptionsResponse;
  IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_5       *SetBootOptionsParameterData;
  EFI_BOOT_MANAGER_LOAD_OPTION                 *LoadOptionToManipulate;
  EFI_BOOT_MANAGER_LOAD_OPTION                 *BootOptions;
  EFI_STATUS                                   Status;
  AMD_BOARD_BDS_BOOT_OPTION_PRIORITY_PROTOCOL  *BootOptionPriorityProtocol;
  EFI_HANDLE                                   *BootPriorityHandles;
  UINTN                                        BootPriorityCount;
  UINTN                                        BootPriorityIndex;
  BOOLEAN                                      ValidBootPriorityOverride;

  ZeroMem (&BootOptionsRequest, sizeof (IPMI_GET_BOOT_OPTIONS_REQUEST));
  ZeroMem (&SetBootOptionsResponse, sizeof (IPMI_SET_BOOT_OPTIONS_RESPONSE));

  LoadOptionToManipulate    = NULL;
  Status                    = EFI_SUCCESS;
  ValidBootPriorityOverride = FALSE;
  // setup buffers
  GetBootOptionsBuffer = (UINT8 *)AllocateZeroPool (sizeof (BootOptionsResponse) + sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_5));
  SetBootOptionsBuffer = (UINT8 *)AllocateZeroPool (sizeof (SetBootOptionsRequest) + sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_5));

  // setup parameter data
  BootOptionsRequest.ParameterSelector.Bits.ParameterSelector = IPMI_BOOT_OPTIONS_PARAMETER_BOOT_FLAGS;
  BootOptionsResponse                                         = (IPMI_GET_BOOT_OPTIONS_RESPONSE *)&GetBootOptionsBuffer[0];
  Status                                                      = IpmiGetSystemBootOptions (&BootOptionsRequest, BootOptionsResponse);
  BootOptionsParameterData                                    = (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_5 *)BootOptionsResponse->ParameterData;

  if (EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;
    goto end;
  }

  // setup SetBootOptions parameter data
  SetBootOptionsRequest       = (IPMI_SET_BOOT_OPTIONS_REQUEST *)&SetBootOptionsBuffer[0];
  SetBootOptionsParameterData = (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_5 *)SetBootOptionsRequest->ParameterData;

  BootOptions = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);

  // if received valid IPMI data, then override boot option
  if (!BootOptionsResponse->ParameterValid.Bits.ParameterValid && BootOptionsParameterData->Data1.Bits.BootFlagValid) {
    // get non volatile IpmiBootOverride variable
    DataSize = sizeof (UINT8);
    Status   = gRT->GetVariable (
                      IPMI_BOOT_OVERRIDE_VAR_NAME,
                      &gEfiCallerIdGuid,
                      NULL,
                      &DataSize,
                      &NvIpmiBootOverride
                      );
    if (EFI_ERROR (Status)) {
      NvIpmiBootOverride = IPMI_BOOT_DEVICE_SELECTOR_NO_OVERRIDE;
    }

    CurrentIpmiBootOverride = BootOptionsParameterData->Data2.Bits.BootDeviceSelector;

    // Handle platform specific boot priority override
    Status = gBS->LocateHandleBuffer (
                    ByProtocol,
                    &gAmdBoardBdsBootOptionPriorityProtocolGuid,
                    NULL,
                    &BootPriorityCount,
                    &BootPriorityHandles
                    );

    if (!EFI_ERROR (Status)) {
      for (BootPriorityIndex = 0; BootPriorityIndex < BootPriorityCount; BootPriorityIndex++) {
        Status = gBS->HandleProtocol (
                        BootPriorityHandles[BootPriorityIndex],
                        &gAmdBoardBdsBootOptionPriorityProtocolGuid,
                        (VOID **)&BootOptionPriorityProtocol
                        );
        if (!EFI_ERROR (Status) &&
            (BootOptionPriorityProtocol->IpmiBootDeviceSelectorType == CurrentIpmiBootOverride))
        {
          DEBUG ((DEBUG_INFO, "Valid BootOptionPriority Override detected\n"));
          ValidBootPriorityOverride = TRUE;
          break;
        }
      }
    }

    if (CurrentIpmiBootOverride == IPMI_BOOT_DEVICE_SELECTOR_BIOS_SETUP) {
      DEBUG ((DEBUG_INFO, "[Bds]BiosSetup option override detected via IPMI\n"));
      // need to find boot option corresponding to BiosSetup
      for (Index = 0; Index < BootOptionCount; Index++) {
        if ((StrCmp (BootOptions[Index].Description, L"Enter Setup") == 0) && (BootOptions[Index].Attributes == (LOAD_OPTION_CATEGORY_APP | LOAD_OPTION_ACTIVE | LOAD_OPTION_HIDDEN))) {
          LoadOptionToManipulate = &BootOptions[Index];
        } else if (StrCmp (BootOptions[Index].Description, L"Enter Setup") == 0) {
          // delete duplicate BiosSetup Menu option
          EfiBootManagerDeleteLoadOptionVariable (BootOptions[Index].OptionNumber, LoadOptionTypeBoot);
        }
      }

      if (LoadOptionToManipulate == NULL) {
        Status = EFI_UNSUPPORTED;
        goto end;
      }

      // have Load option for bios setup, now update loadoptions
      Status                              = EfiBootManagerDeleteLoadOptionVariable (LoadOptionToManipulate->OptionNumber, LoadOptionTypeBoot);
      LoadOptionToManipulate->Attributes &= LOAD_OPTION_CATEGORY_BOOT;
      LoadOptionToManipulate->Attributes |= (LOAD_OPTION_ACTIVE | LOAD_OPTION_HIDDEN);
      Status                              = EfiBootManagerAddLoadOptionVariable (LoadOptionToManipulate, 0); // add back in loadoptions in 0th index (first option)
    } else if (CurrentIpmiBootOverride == IPMI_BOOT_DEVICE_SELECTOR_PXE) {
      DEBUG ((DEBUG_INFO, "[Bds]PXE option override detected via IPMI\n"));

      if (ValidBootPriorityOverride) {
        EfiBootManagerSortLoadOptionVariable (LoadOptionTypeBoot, BootOptionPriorityProtocol->Compare);
      } else {
        EfiBootManagerSortLoadOptionVariable (LoadOptionTypeBoot, CompareBootOptionPxePriority);
      }
    } else if (CurrentIpmiBootOverride == IPMI_BOOT_DEVICE_SELECTOR_HARDDRIVE) {
      DEBUG ((DEBUG_INFO, "[Bds]HDD option override detected via IPMI\n"));
      if (ValidBootPriorityOverride) {
        EfiBootManagerSortLoadOptionVariable (LoadOptionTypeBoot, BootOptionPriorityProtocol->Compare);
      } else {
        EfiBootManagerSortLoadOptionVariable (LoadOptionTypeBoot, CompareBootOptionHddPriority);
      }
    } else if ((CurrentIpmiBootOverride == IPMI_BOOT_DEVICE_SELECTOR_NO_OVERRIDE) && (CurrentIpmiBootOverride != NvIpmiBootOverride)) {
      // delete BiosSetup option corresponding to the override
      Status = EfiBootManagerDeleteLoadOptionVariable (BootOptions[0].OptionNumber, LoadOptionTypeBoot);
      // re sort boot options
      EfiBootManagerSortLoadOptionVariable (LoadOptionTypeBoot, CompareBootOption);
    }

    // if Ipmi override not persistent, reset boot option to None and persistent to true
    if (!BootOptionsParameterData->Data1.Bits.PersistentOptions) {
      SetBootOptionsRequest->ParameterValid.Bits.ParameterSelector = IPMI_BOOT_OPTIONS_PARAMETER_BOOT_FLAGS;
      CopyMem (SetBootOptionsParameterData, BootOptionsParameterData, sizeof (IPMI_BOOT_OPTIONS_RESPONSE_PARAMETER_5));
      SetBootOptionsParameterData->Data1.Bits.PersistentOptions  = 1;                                     // persistent
      SetBootOptionsParameterData->Data2.Bits.BootDeviceSelector = IPMI_BOOT_DEVICE_SELECTOR_NO_OVERRIDE; // revert to no override
      Status                                                     = IpmiSetSystemBootOptions (SetBootOptionsRequest, &SetBootOptionsResponse);
    }

    Status = gRT->SetVariable (
                    IPMI_BOOT_OVERRIDE_VAR_NAME,
                    &gEfiCallerIdGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    sizeof (UINT8),
                    &CurrentIpmiBootOverride
                    );
  }

end:
  // Free buffers
  FreePool (GetBootOptionsBuffer);
  FreePool (SetBootOptionsBuffer);
  return Status;
}

/**
  After console ready before boot option event callback.

  @param[in] Event      The Event this notify function registered to.
  @param[in] Context    Pointer to the context data registered to the Event.
**/
VOID
EFIAPI
BdsAfterConsoleReadyBeforeBootOptionCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_BOOT_MODE  LocalBootMode;
  EFI_STATUS     Status;
  BOOLEAN        IsFirstBoot;
  UINTN          DataSize;

  DEBUG ((DEBUG_INFO, "Event gBdsAfterConsoleReadyBeforeBootOptionEvent callback starts\n"));
  //
  // Get current Boot Mode
  //
  LocalBootMode = gBootMode;
  DEBUG ((DEBUG_INFO, "Current local bootmode - %x\n", LocalBootMode));

  //
  // Go the different platform policy with different boot mode
  // Notes: this part code can be change with the table policy
  //
  switch (LocalBootMode) {
    case BOOT_ASSUMING_NO_CONFIGURATION_CHANGES:
    case BOOT_WITH_MINIMAL_CONFIGURATION:
    case BOOT_ON_S4_RESUME:
      //
      // Perform some platform specific connect sequence
      //
      PERF_START_EX (NULL, "EventRec", NULL, AsmReadTsc (), 0x7050);
      ConnectSequence (LocalBootMode);
      PERF_END_EX (NULL, "EventRec", NULL, AsmReadTsc (), 0x7051);

      break;

    case BOOT_WITH_FULL_CONFIGURATION:
    case BOOT_WITH_FULL_CONFIGURATION_PLUS_DIAGNOSTICS:
    case BOOT_WITH_DEFAULT_SETTINGS:
    default:
      //
      // Perform some platform specific connect sequence
      //
      ConnectSequence (LocalBootMode);

      //
      // Only in Full Configuration boot mode we do the enumeration of boot device
      //
      //
      // Dispatch all but Storage Oprom explicitly, because we assume Int13Thunk driver is there.
      //

      //
      // PXE boot option may appear after boot option enumeration
      //

      EfiBootManagerRefreshAllBootOption ();
      DataSize = sizeof (BOOLEAN);
      Status   = gRT->GetVariable (
                        IS_FIRST_BOOT_VAR_NAME,
                        &gEfiCallerIdGuid,
                        NULL,
                        &DataSize,
                        &IsFirstBoot
                        );
      if (EFI_ERROR (Status)) {
        //
        // If can't find the variable, see it as the first boot
        //
        IsFirstBoot = TRUE;
      }

      if (IsFirstBoot) {
        //
        // In the first boot, sort the boot option
        //
        EfiBootManagerSortLoadOptionVariable (LoadOptionTypeBoot, CompareBootOption);
        IsFirstBoot = FALSE;
        Status      = gRT->SetVariable (
                             IS_FIRST_BOOT_VAR_NAME,
                             &gEfiCallerIdGuid,
                             EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                             sizeof (BOOLEAN),
                             &IsFirstBoot
                             );
      }

      break;
  }

  HandleIpmiBootOverride ();

  Print (L"Press F2 for Setup, or F7 for BootMenu!\n");

 #ifdef INTERNAL_IDS
  PrintSocOpnInfo ();
 #endif
}

#ifdef INTERNAL_IDS

/**
  Display CPU OPN on Splash screen After console ready.
  @retval EFI_SUCCESS
**/
EFI_STATUS
PrintSocOpnInfo (
  IN  VOID
  )
{
  CHAR16  OpnChar[(sizeof (UINT64) / sizeof (UINT8)) * (MSR_CPUID_NAME_STRING5 - MSR_CPUID_NAME_STRING0 + 1) + 1];
  UINT32  Msr;
  UINT64  Value;
  UINTN   CharIndex;
  UINTN   ByteIndex;

  CharIndex = 0;
  for (Msr = MSR_CPUID_NAME_STRING0; Msr <= MSR_CPUID_NAME_STRING5; Msr++) {
    Value = AsmReadMsr64 (Msr);
    for (ByteIndex = 0; ByteIndex < (sizeof (UINT64) / sizeof (UINT8)); ByteIndex++) {
      OpnChar[CharIndex] = (Value & 0xFF);
      CharIndex++;
      Value = RShiftU64 (Value, 8);
    }
  }

  OpnChar[CharIndex] = L'\0';

  DEBUG ((DEBUG_INFO, "\nProcessor OPN: "));
  DEBUG ((DEBUG_INFO, "%s\n", OpnChar));

  Print (L"Processor OPN: ");
  Print (OpnChar);
  Print (L"\n\r");

  return EFI_SUCCESS;
}

#endif
