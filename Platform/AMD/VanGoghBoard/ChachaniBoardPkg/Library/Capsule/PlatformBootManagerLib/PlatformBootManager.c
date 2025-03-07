/** @file
  This file include all platform action which can be customized
  by IBV/OEM.

Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
Copyright (c) 2015 - 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
//
// PCI Vendor ID and Device ID
//
#define VENDOR_ID   0x1002
#define DEVICE_ID   0x163F
#define DEVICE_ID2  0x1435

#include "PlatformBootManager.h"
#include "PlatformConsole.h"
#include <Protocol/PlatformBootManagerOverride.h>
#include <Guid/BootManagerMenu.h>
#include <Library/HobLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/CapsuleHookLib.h>
#include <Library/CapsuleLib.h>

//
// Predefined platform root bridge
//
PLATFORM_ROOT_BRIDGE_DEVICE_PATH  gPlatformRootBridge0 = {
  gPciRootBridge,
  gEndEntire
};

EFI_DEVICE_PATH_PROTOCOL  *gPlatformRootBridges[] = {
  (EFI_DEVICE_PATH_PROTOCOL *)&gPlatformRootBridge0,
  NULL
};

extern EFI_GUID                                            gEfiEventReadyToBootGuid;
UNIVERSAL_PAYLOAD_PLATFORM_BOOT_MANAGER_OVERRIDE_PROTOCOL  *mUniversalPayloadPlatformBootManagerOverrideInstance = NULL;

EFI_STATUS
GetGopDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL  *PciDevicePath,
  OUT EFI_DEVICE_PATH_PROTOCOL  **GopDevicePath
  );

/**
  Signal EndOfDxe event and install SMM Ready to lock protocol.

**/
VOID
InstallReadyToLock (
  VOID
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                Handle;
  EFI_SMM_ACCESS2_PROTOCOL  *SmmAccess;

  DEBUG ((DEBUG_INFO, "InstallReadyToLock  entering......\n"));
  //
  // Inform the SMM infrastructure that we're entering BDS and may run 3rd party code hereafter
  // Since PI1.2.1, we need signal EndOfDxe as ExitPmAuth
  //
  EfiEventGroupSignal (&gEfiEndOfDxeEventGroupGuid);
  DEBUG ((DEBUG_INFO, "All EndOfDxe callbacks have returned successfully\n"));

  //
  // Install DxeSmmReadyToLock protocol in order to lock SMM
  //
  Status = gBS->LocateProtocol (&gEfiSmmAccess2ProtocolGuid, NULL, (VOID **)&SmmAccess);
  if (!EFI_ERROR (Status)) {
    Handle = NULL;
    Status = gBS->InstallProtocolInterface (
                    &Handle,
                    &gEfiDxeSmmReadyToLockProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  }

  DEBUG ((DEBUG_INFO, "InstallReadyToLock  end\n"));
  return;
}

/**
  Return the index of the load option in the load option array.

  The function consider two load options are equal when the
  OptionType, Attributes, Description, FilePath and OptionalData are equal.

  @param Key    Pointer to the load option to be found.
  @param Array  Pointer to the array of load options to be found.
  @param Count  Number of entries in the Array.

  @retval -1          Key wasn't found in the Array.
  @retval 0 ~ Count-1 The index of the Key in the Array.
**/
INTN
PlatformFindLoadOption (
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION  *Key,
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION  *Array,
  IN UINTN                               Count
  )
{
  UINTN  Index;

  for (Index = 0; Index < Count; Index++) {
    if ((Key->OptionType == Array[Index].OptionType) &&
        (Key->Attributes == Array[Index].Attributes) &&
        (StrCmp (Key->Description, Array[Index].Description) == 0) &&
        (CompareMem (Key->FilePath, Array[Index].FilePath, GetDevicePathSize (Key->FilePath)) == 0) &&
        (Key->OptionalDataSize == Array[Index].OptionalDataSize) &&
        (CompareMem (Key->OptionalData, Array[Index].OptionalData, Key->OptionalDataSize) == 0))
    {
      return (INTN)Index;
    }
  }

  return -1;
}

/**
  Register a boot option using a file GUID in the FV.

  @param FileGuid     The file GUID name in FV.
  @param Description  The boot option description.
  @param Attributes   The attributes used for the boot option loading.
**/
VOID
PlatformRegisterFvBootOption (
  EFI_GUID  *FileGuid,
  CHAR16    *Description,
  UINT32    Attributes
  )
{
  EFI_STATUS                         Status;
  UINTN                              OptionIndex;
  EFI_BOOT_MANAGER_LOAD_OPTION       NewOption;
  EFI_BOOT_MANAGER_LOAD_OPTION       *BootOptions;
  UINTN                              BootOptionCount;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  FileNode;
  EFI_LOADED_IMAGE_PROTOCOL          *LoadedImage;
  EFI_DEVICE_PATH_PROTOCOL           *DevicePath;

  Status = gBS->HandleProtocol (gImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **)&LoadedImage);
  ASSERT_EFI_ERROR (Status);

  EfiInitializeFwVolDevicepathNode (&FileNode, FileGuid);
  DevicePath = AppendDevicePathNode (
                 DevicePathFromHandle (LoadedImage->DeviceHandle),
                 (EFI_DEVICE_PATH_PROTOCOL *)&FileNode
                 );

  Status = EfiBootManagerInitializeLoadOption (
             &NewOption,
             LoadOptionNumberUnassigned,
             LoadOptionTypeBoot,
             Attributes,
             Description,
             DevicePath,
             NULL,
             0
             );
  if (!EFI_ERROR (Status)) {
    BootOptions = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);

    OptionIndex = PlatformFindLoadOption (&NewOption, BootOptions, BootOptionCount);

    if (OptionIndex == -1) {
      Status = EfiBootManagerAddLoadOptionVariable (&NewOption, (UINTN)-1);
      ASSERT_EFI_ERROR (Status);
    }

    EfiBootManagerFreeLoadOption (&NewOption);
    EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
  }
}

/**
  Get device path of one IGPU and one DGPU.

  @param IGpuDevicePath   Return the IGPU devide path, if no, return NULL.
  @param DGpuDevicePath   Return the DGPU devide path, if no, return NULL.

  @retval EFI_SUCCSS      Get all platform active video device path.
  @retval EFI_STATUS      Return the status of gBS->LocateDevicePath (),
                          gBS->ConnectController (),
                          and gBS->LocateHandleBuffer ().
**/
EFI_STATUS
GetVgaDevicePath (
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **IGpuDevicePath,
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **DGpuDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                RootHandle;
  UINTN                     HandleCount;
  EFI_HANDLE                *HandleBuffer;
  UINTN                     Index;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  PCI_TYPE00                Pci;

  DEBUG ((DEBUG_INFO, "GetVgaDevicePath enter\n"));

  HandleCount     = 0;
  DevicePath      = NULL;
  HandleBuffer    = NULL;
  *IGpuDevicePath = NULL;
  *DGpuDevicePath = NULL;

  DEBUG ((DEBUG_INFO, "VENDOR_ID = 0x%x\n", VENDOR_ID));
  DEBUG ((DEBUG_INFO, "DEVICE_ID = 0x%x\n", DEVICE_ID));

  //
  // Make all the PCI_IO protocols on PCI Seg 0 show up
  //
  EfiBootManagerConnectDevicePath (gPlatformRootBridges[0], NULL);

  Status = gBS->LocateDevicePath (
                  &gEfiDevicePathProtocolGuid,
                  &gPlatformRootBridges[0],
                  &RootHandle
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->ConnectController (
                  RootHandle,
                  NULL,
                  NULL,
                  FALSE
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Start to check all the pci io to find all possible VGA device
  //
  HandleCount  = 0;
  HandleBuffer = NULL;
  Status       = gBS->LocateHandleBuffer (
                        ByProtocol,
                        &gEfiPciIoProtocolGuid,
                        NULL,
                        &HandleCount,
                        &HandleBuffer
                        );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiPciIoProtocolGuid,
                    (VOID **)&PciIo
                    );
    if (!EFI_ERROR (Status)) {
      //
      // Check for all VGA device
      //
      Status = PciIo->Pci.Read (
                            PciIo,
                            EfiPciIoWidthUint32,
                            0,
                            sizeof (Pci) / sizeof (UINT32),
                            &Pci
                            );
      if (EFI_ERROR (Status)) {
        continue;
      }

      //
      // Here we decide which VGA device to enable in PCI bus
      //
      // The first plugin PCI VGA card device will be present as PCI VGA
      // The onchip AGP or AGP card will be present as AGP VGA
      //
      if (!IS_PCI_DISPLAY (&Pci)) {
        // if (!IS_PCI_VGA (&Pci)) {
        continue;
      }

      //
      // Set the device as the possible console out device,
      //
      // Below code will make every VGA device to be one
      // of the possibe console out device
      //
      gBS->HandleProtocol (
             HandleBuffer[Index],
             &gEfiDevicePathProtocolGuid,
             (VOID **)&DevicePath
             );
      DEBUG ((DEBUG_INFO, "DevicePath: %S\n", ConvertDevicePathToText (DevicePath, FALSE, FALSE)));

      if ((Pci.Hdr.VendorId == VENDOR_ID) && ((Pci.Hdr.DeviceId == DEVICE_ID) || (Pci.Hdr.DeviceId == DEVICE_ID2))) {
        // IGPU
        *IGpuDevicePath = DevicePath;
      } else {
        // DGPU
        *DGpuDevicePath = DevicePath;
      }

      if ((*IGpuDevicePath != NULL) && (*DGpuDevicePath != NULL)) {
        DEBUG ((DEBUG_INFO, "IGpuDevicePath and DGpuDevicePath are not NULL\n"));
        break;
      }
    }
  }

  DEBUG ((DEBUG_INFO, "IGpuDevicePath: %S\n", ConvertDevicePathToText (*IGpuDevicePath, FALSE, FALSE)));
  DEBUG ((DEBUG_INFO, "DGpuDevicePath: %S\n", ConvertDevicePathToText (*DGpuDevicePath, FALSE, FALSE)));
  FreePool (HandleBuffer);

  return EFI_SUCCESS;
}

/**

  Find the platform active vga, and base on the policy to enable the vga as
  the console out device. The policy is active dGPU, if no dGPU active iGPU.

  None.

  @param EFI_UNSUPPORTED         There is no active vga device

  @retval EFI_STATUS             Return the status of BdsLibGetVariableAndSize ()

**/
EFI_STATUS
PlatformBdsForceActiveVga (
  VOID
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathFirst;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathSecond;
  EFI_DEVICE_PATH_PROTOCOL  *GopDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *IGpuDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *DGpuDevicePath;

  DEBUG ((DEBUG_INFO, "PlatformBdsForceActiveVga enter\n"));

  Status           = EFI_SUCCESS;
  DevicePathFirst  = NULL;
  DevicePathSecond = NULL;
  GopDevicePath    = NULL;
  IGpuDevicePath   = NULL;
  DGpuDevicePath   = NULL;

  //
  // Get device path of one IGPU and one DGPU
  //
  Status = GetVgaDevicePath (&IGpuDevicePath, &DGpuDevicePath);
  ASSERT_EFI_ERROR (Status);

  if ((IGpuDevicePath == NULL) && (DGpuDevicePath == NULL)) {
    DEBUG ((DEBUG_INFO, "No valid IGPU and DGPU\n"));
    return EFI_UNSUPPORTED;
  }

  if ((IGpuDevicePath != NULL) && (DGpuDevicePath == NULL)) {
    DEBUG ((DEBUG_INFO, "Only IGPU is valid\n"));
    // DEBUG ((DEBUG_INFO,"Only IGPU is valid, Update IGPU ...\n"));
    DevicePathFirst  = IGpuDevicePath;
    DevicePathSecond = DGpuDevicePath;
    goto UpdateConOut;
  } else if ((IGpuDevicePath == NULL) && (DGpuDevicePath != NULL)) {
    DEBUG ((DEBUG_INFO, "Only DGPU is valid\n"));
    // DEBUG ((DEBUG_INFO,"Only DGPU is valid, Update DGPU ...\n"));
    DevicePathFirst  = DGpuDevicePath;
    DevicePathSecond = IGpuDevicePath;
    goto UpdateConOut;
  } else if ((IGpuDevicePath != NULL) && (DGpuDevicePath != NULL)) {
    DEBUG ((DEBUG_INFO, "DGPU and IGPU are valid, active DGPU\n"));
    // DEBUG ((DEBUG_INFO,"Only DGPU is valid, Update DGPU ...\n"));
    DevicePathFirst  = DGpuDevicePath;
    DevicePathSecond = IGpuDevicePath;
    goto UpdateConOut;
  }

UpdateConOut:
  DEBUG ((DEBUG_INFO, "Before GetGopDevicePath: ConOutDevicePath is %S\n", ConvertDevicePathToText (DevicePathFirst, FALSE, FALSE)));
  GetGopDevicePath (DevicePathFirst, &GopDevicePath);
  DevicePathFirst = GopDevicePath;
  DEBUG ((DEBUG_INFO, "After GetGopDevicePath: ConOutDevicePath is %S\n", ConvertDevicePathToText (DevicePathFirst, FALSE, FALSE)));
  DEBUG ((DEBUG_INFO, "Exclusive device path is %S\n", ConvertDevicePathToText (DevicePathSecond, FALSE, FALSE)));

  Status = EfiBootManagerUpdateConsoleVariable (
             ConOut,
             DevicePathFirst,
             DevicePathSecond
             );
  // TODO: Specify iGPU/dGPU.
  EfiBootManagerConnectVideoController (NULL);
  return Status;
}

/**
  Do the platform specific action before the console is connected.

  Such as:
    Update console variable;
    Register new Driver#### or Boot####;
    Signal ReadyToLock event.
**/
VOID
EFIAPI
PlatformBootManagerBeforeConsole (
  VOID
  )
{
  EFI_INPUT_KEY                 Enter;
  EFI_INPUT_KEY                 CustomKey;
  EFI_INPUT_KEY                 Down;
  EFI_BOOT_MANAGER_LOAD_OPTION  BootOption;
  EFI_STATUS                    Status;
  UINT64                        OsIndication;
  UINTN                         DataSize;
  UINT32                        Attributes;
  BOOLEAN                       CapsuleUpdateonDisk;

  Status = gBS->LocateProtocol (&gUniversalPayloadPlatformBootManagerOverrideProtocolGuid, NULL, (VOID **)&mUniversalPayloadPlatformBootManagerOverrideInstance);
  if (EFI_ERROR (Status)) {
    mUniversalPayloadPlatformBootManagerOverrideInstance = NULL;
  }

  Status = gRT->GetVariable (
                  L"OsIndications",
                  &gEfiGlobalVariableGuid,
                  &Attributes,
                  &DataSize,
                  &OsIndication
                  );
  if (mUniversalPayloadPlatformBootManagerOverrideInstance != NULL) {
    mUniversalPayloadPlatformBootManagerOverrideInstance->BeforeConsole ();
    return;
  }

  //
  // Register ENTER as CONTINUE key
  //
  Enter.ScanCode    = SCAN_NULL;
  Enter.UnicodeChar = CHAR_CARRIAGE_RETURN;
  EfiBootManagerRegisterContinueKeyOption (0, &Enter, NULL);

  if (FixedPcdGetBool (PcdBootManagerEscape)) {
    //
    // Map Esc to Boot Manager Menu
    //
    CustomKey.ScanCode    = SCAN_ESC;
    CustomKey.UnicodeChar = CHAR_NULL;
  } else {
    //
    // Map Esc to Boot Manager Menu
    //
    CustomKey.ScanCode    = SCAN_F2;
    CustomKey.UnicodeChar = CHAR_NULL;
  }

  EfiBootManagerGetBootManagerMenu (&BootOption);
  EfiBootManagerAddKeyOptionVariable (NULL, (UINT16)BootOption.OptionNumber, 0, &CustomKey, NULL);

  //
  // Also add Down key to Boot Manager Menu since some serial terminals don't support F2 key.
  //
  Down.ScanCode    = SCAN_DOWN;
  Down.UnicodeChar = CHAR_NULL;
  EfiBootManagerGetBootManagerMenu (&BootOption);
  EfiBootManagerAddKeyOptionVariable (NULL, (UINT16)BootOption.OptionNumber, 0, &Down, NULL);
  CapsuleUpdateonDisk = (BOOLEAN)((OsIndication & EFI_OS_INDICATIONS_FILE_CAPSULE_DELIVERY_SUPPORTED) != 0);
  // Process Capsule in Memory first, before EndOfDxe.
  if ((GetBootModeHob () == BOOT_ON_FLASH_UPDATE) || CapsuleUpdateonDisk) {
    PlatformBdsForceActiveVga (); // Force enable VGA on Capsule Update.
    ASSERT_EFI_ERROR (BootLogoEnableLogo ());
    Print (
      L"\n"
      L"    Updating system BIOS.....\n"
      L"\n"
      );
    if (CapsuleUpdateonDisk) {
      EfiBootManagerConnectAll (); // Get BlockIo
      CapsuleUpdateViaFileLib ();
    } else {
      ProcessCapsules ();
    }
  }

  //
  // Install ready to lock.
  // This needs to be done before option rom dispatched.
  //
  InstallReadyToLock ();

  //
  // Dispatch deferred images after EndOfDxe event and ReadyToLock installation.
  //
  EfiBootManagerDispatchDeferredImages ();

  PlatformConsoleInit ();
}

/**
  Do the platform specific action after the console is connected.

  Such as:
    Dynamically switch output mode;
    Signal console ready platform customized event;
    Run diagnostics like memory testing;
    Connect certain devices;
    Dispatch additional option roms.
**/
VOID
EFIAPI
PlatformBootManagerAfterConsole (
  VOID
  )
{
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Black;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  White;
  EDKII_PLATFORM_LOGO_PROTOCOL   *PlatformLogo;
  EFI_STATUS                     Status;
  UINT64                         OsIndication;
  UINTN                          DataSize;
  UINT32                         Attributes;

  if (mUniversalPayloadPlatformBootManagerOverrideInstance != NULL) {
    mUniversalPayloadPlatformBootManagerOverrideInstance->AfterConsole ();
    return;
  }

  Black.Blue = Black.Green = Black.Red = Black.Reserved = 0;
  White.Blue = White.Green = White.Red = White.Reserved = 0xFF;

  Status = gBS->LocateProtocol (&gEdkiiPlatformLogoProtocolGuid, NULL, (VOID **)&PlatformLogo);

  if (!EFI_ERROR (Status)) {
    gST->ConOut->ClearScreen (gST->ConOut);
    BootLogoEnableLogo ();
  }

  EfiBootManagerConnectAll ();
  EfiBootManagerRefreshAllBootOption ();

  // Process Capsule in Memory again, after EndOfDxe.
  if (GetBootModeHob () == BOOT_ON_FLASH_UPDATE) {
    ProcessCapsules ();
  }

  OsIndication = 0;
  Attributes   = 0;
  DataSize     = sizeof (UINT64);
  Status       = gRT->GetVariable (
                        EFI_OS_INDICATIONS_VARIABLE_NAME,
                        &gEfiGlobalVariableGuid,
                        &Attributes,
                        &DataSize,
                        &OsIndication
                        );
  if (EFI_ERROR (Status)) {
    OsIndication = 0;
  }

  //
  // Register UEFI Shell
  //
  PlatformRegisterFvBootOption (PcdGetPtr (PcdShellFile), L"UEFI Shell", LOAD_OPTION_ACTIVE);

  {
    if (FixedPcdGetBool (PcdBootManagerEscape)) {
      Print (
        L"\n"
        L"    Esc or Down      to enter Boot Manager Menu.\n"
        L"    ENTER            to boot directly.\n"
        L"\n"
        );
    } else {
      Print (
        L"\n"
        L"    F2 or Down      to enter Boot Manager Menu.\n"
        L"    ENTER           to boot directly.\n"
        L"\n"
        );
    }
  }
}

/**
  This function is called each second during the boot manager waits the timeout.

  @param TimeoutRemain  The remaining timeout.
**/
VOID
EFIAPI
PlatformBootManagerWaitCallback (
  UINT16  TimeoutRemain
  )
{
  if (mUniversalPayloadPlatformBootManagerOverrideInstance != NULL) {
    mUniversalPayloadPlatformBootManagerOverrideInstance->WaitCallback (TimeoutRemain);
  }

  return;
}

/**
  The function is called when no boot option could be launched,
  including platform recovery options and options pointing to applications
  built into firmware volumes.

  If this function returns, BDS attempts to enter an infinite loop.
**/
VOID
EFIAPI
PlatformBootManagerUnableToBoot (
  VOID
  )
{
  if (mUniversalPayloadPlatformBootManagerOverrideInstance != NULL) {
    mUniversalPayloadPlatformBootManagerOverrideInstance->UnableToBoot ();
  }

  return;
}

typedef struct {
  UINTN                   Signature;
  LIST_ENTRY              Link;
  EFI_PHYSICAL_ADDRESS    StartAddress;
  UINT64                  Length;
  UINT64                  Capabilities;
} NONTESTED_MEMORY_RANGE;

//
// This structure records every nontested memory range parsed through GCD
// service.
//
#define EFI_NONTESTED_MEMORY_RANGE_SIGNATURE  SIGNATURE_32 ('N', 'T', 'M', 'E')

//
// attributes for reserved memory before it is promoted to system memory
//
#define EFI_MEMORY_PRESENT      0x0100000000000000ULL
#define EFI_MEMORY_INITIALIZED  0x0200000000000000ULL
#define EFI_MEMORY_TESTED       0x0400000000000000ULL

/**
  Callback function for event group EFI_EVENT_GROUP_READY_TO_BOOT.
  This is used to expose the 4G above memory into system memory table.

  @param[in]  Event      The Event that is being processed.
  @param[in]  Context    The Event Context.

**/
VOID
EFIAPI
ExposeOver4GMemoryEventCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  NONTESTED_MEMORY_RANGE           *Range;
  BOOLEAN                          NoFound;
  UINTN                            NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemorySpaceMap;
  UINTN                            Index;
  LIST_ENTRY                       *TmpLink;
  LIST_ENTRY                       NonTestedMemRanList;

  DEBUG ((DEBUG_INFO, "ExposeOver4GMemoryEventCallback\n"));

  TmpLink = NULL;
  NoFound = TRUE;

  InitializeListHead (&NonTestedMemRanList);

  gDS->GetMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);
  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    if ((MemorySpaceMap[Index].GcdMemoryType == EfiGcdMemoryTypeReserved) &&
        ((MemorySpaceMap[Index].Capabilities & (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED | EFI_MEMORY_TESTED)) ==
         (EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED))
        )
    {
      NoFound = FALSE;

      gBS->AllocatePool (EfiBootServicesData, sizeof (NONTESTED_MEMORY_RANGE), (VOID **)&Range);

      Range->Signature    = EFI_NONTESTED_MEMORY_RANGE_SIGNATURE;
      Range->StartAddress = MemorySpaceMap[Index].BaseAddress;
      Range->Length       = MemorySpaceMap[Index].Length;
      Range->Capabilities = MemorySpaceMap[Index].Capabilities;

      InsertTailList (&NonTestedMemRanList, &Range->Link);
    }
  }

  if (!NoFound) {
    TmpLink = NonTestedMemRanList.ForwardLink;

    while (TmpLink != &NonTestedMemRanList) {
      Range = CR (TmpLink, NONTESTED_MEMORY_RANGE, Link, EFI_NONTESTED_MEMORY_RANGE_SIGNATURE);
      gDS->RemoveMemorySpace (Range->StartAddress, Range->Length);
      gDS->AddMemorySpace (
             EfiGcdMemoryTypeSystemMemory,
             Range->StartAddress,
             Range->Length,
             Range->Capabilities &~(EFI_MEMORY_PRESENT | EFI_MEMORY_INITIALIZED | EFI_MEMORY_TESTED | EFI_MEMORY_RUNTIME)
             );

      TmpLink = TmpLink->ForwardLink;
    }
  }

  //
  // Make sure the hook ONLY called once.
  //
  if (Event != NULL) {
    gBS->CloseEvent (Event);
  }
}

/**
  Get/update PcdBootManagerMenuFile from GUID HOB which will be assigned in bootloader.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs.

**/
EFI_STATUS
EFIAPI
PlatformBootManagerLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                           Status;
  UINTN                                Size;
  VOID                                 *GuidHob;
  UNIVERSAL_PAYLOAD_GENERIC_HEADER     *GenericHeader;
  UNIVERSAL_PAYLOAD_BOOT_MANAGER_MENU  *BootManagerMenuFile;

  EFI_EVENT  ExposeOver4GMemoryEvent;

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  ExposeOver4GMemoryEventCallback,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &ExposeOver4GMemoryEvent
                  );
  ASSERT_EFI_ERROR (Status);

  GuidHob = GetFirstGuidHob (&gEdkiiBootManagerMenuFileGuid);

  if (GuidHob == NULL) {
    //
    // If the HOB is not create, the default value of PcdBootManagerMenuFile will be used.
    //
    return EFI_SUCCESS;
  }

  GenericHeader = (UNIVERSAL_PAYLOAD_GENERIC_HEADER *)GET_GUID_HOB_DATA (GuidHob);
  if ((sizeof (UNIVERSAL_PAYLOAD_GENERIC_HEADER) > GET_GUID_HOB_DATA_SIZE (GuidHob)) || (GenericHeader->Length > GET_GUID_HOB_DATA_SIZE (GuidHob))) {
    return EFI_NOT_FOUND;
  }

  if (GenericHeader->Revision == UNIVERSAL_PAYLOAD_BOOT_MANAGER_MENU_REVISION) {
    BootManagerMenuFile = (UNIVERSAL_PAYLOAD_BOOT_MANAGER_MENU *)GET_GUID_HOB_DATA (GuidHob);
    if (BootManagerMenuFile->Header.Length < UNIVERSAL_PAYLOAD_SIZEOF_THROUGH_FIELD (UNIVERSAL_PAYLOAD_BOOT_MANAGER_MENU, FileName)) {
      return EFI_NOT_FOUND;
    }

    Size   = sizeof (BootManagerMenuFile->FileName);
    Status = PcdSetPtrS (PcdBootManagerMenuFile, &Size, &BootManagerMenuFile->FileName);
    ASSERT_EFI_ERROR (Status);
  } else {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}
