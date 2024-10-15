/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <IndustryStandard/Nvme.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/BlockIo.h>
#include <Protocol/NvmExpressPassthru.h>
#include <Protocol/PartitionInfo.h>
#include <Protocol/PlatformBootManager.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/UsbIo.h>

#pragma pack (1)
typedef struct {
  CHAR16    *OSLoaderPath;
  CHAR16    *OSLoaderName;
} OS_LOADER_DESC;
#pragma pack ()

CONST OS_LOADER_DESC  KnownOsLoaderList[] =
{
  {
    L"\\EFI\\Microsoft\\Boot\\bootmgfw.efi",
    L"Windows Boot Manager"
  },
  {
    L"\\EFI\\sles\\shim.efi",
    L"SUSE"
  },
  {
    L"\\EFI\\fedora\\shimaa64.efi",
    L"Fedora"
  },
  {
    L"\\EFI\\centos\\shimaa64.efi",
    L"CentOS"
  },
  {
    L"\\EFI\\redhat\\shimaa64.efi",
    L"Redhat"
  },
  {
    L"\\EFI\\ubuntu\\shimaa64.efi",
    L"Ubuntu"
  },
  {
    L"\\EFI\\FreeBSD\\loader.efi",
    L"FreeBSD"
  },
  {
    NULL,
    NULL
  }
};

CONST OS_LOADER_DESC  DefaultUefiLoader =
{
  L"\\EFI\\BOOT\\bootaa64.efi",
  L"UEFI OS"
};

CONST UINT16  UsbEnglishLang = 0x0409;

extern EFI_GUID  mBmAutoCreateBootOptionGuid;

/**
  Append a Boot Option to a Boot Options list. If the description
  and the device path are null, this function will copy data from
  BootOptionToCopy. If BootOptionToCopy is also null, then return
  EFI_INVALID_PARAMETER.

  Note that, if BootOptionNumber is smaller than maximum order in
  Boot Options List, the object is in this BootOptionNumber order
  replaced .And the object which have higher order will be removed.

  @param BootOptionsList  Pointer to List which Boot Option is added.
  @param BootOptionNumber Boot Option order in list.
  @param BootOptionToCopy Pointer to Boot Option copied when Desscription or Device not specific.
  @param Description      Description of Boot Option.
  @param DevicePath       Device path of Boot Option.
  @param OptionalData     Optional Data of Boot Option.
  @param OptionalDataSize Optional data size of Boot Option.

  @retval EFI_SUCCESS     Success to append Boot Option to list.
  @retval other           Some error occurs when executing this function.
 **/
EFI_STATUS
AppendBootOption (
  IN OUT          EFI_BOOT_MANAGER_LOAD_OPTION  **BootOptionsList,
  IN              UINTN                         BootOptionNumber,
  IN     OPTIONAL EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptionToCopy,
  IN              CHAR16                        *Description,
  IN              EFI_DEVICE_PATH_PROTOCOL      *DevicePath,
  IN     OPTIONAL UINT8                         *OptionalData,
  IN     OPTIONAL UINT32                        OptionalDataSize
  )
{
  EFI_STATUS  Status;

  if ((BootOptionToCopy == NULL) && ((Description == NULL) || (DevicePath == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  *BootOptionsList = ReallocatePool (
                       sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (BootOptionNumber),
                       sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (BootOptionNumber + sizeof (CHAR16)),
                       (VOID *)*BootOptionsList
                       );
  if (*BootOptionsList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if ((Description == NULL) || (DevicePath == NULL)) {
    Status = EfiBootManagerInitializeLoadOption (
               *BootOptionsList + BootOptionNumber,
               LoadOptionNumberUnassigned,
               BootOptionToCopy->OptionType,
               BootOptionToCopy->Attributes,
               BootOptionToCopy->Description,
               BootOptionToCopy->FilePath,
               BootOptionToCopy->OptionalData,
               BootOptionToCopy->OptionalDataSize
               );
  } else {
    Status = EfiBootManagerInitializeLoadOption (
               *BootOptionsList + BootOptionNumber,
               LoadOptionNumberUnassigned,
               LoadOptionTypeBoot,
               LOAD_OPTION_ACTIVE,
               Description,
               DevicePath,
               OptionalData,
               OptionalDataSize
               );
  }

  return Status;
}

/**
  Queries a Device path to determine if it supports the block io protocol.

  @param DevicePath  Pointer to Device Path.

  @retval TRUE       It supports the block io protocol.
  @retval FALSE      It does not support the block io protocol.
 **/
BOOLEAN
IsBlockIoProtocolInstalled  (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  EFI_STATUS             Status;
  EFI_HANDLE             Handle;
  EFI_BLOCK_IO_PROTOCOL  *BlkIo;

  ASSERT (DevicePath != NULL);

  Status = gBS->LocateDevicePath (
                  &gEfiBlockIoProtocolGuid,
                  &DevicePath,
                  &Handle
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **)&BlkIo
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  return TRUE;
}

VOID
RemoveUnnecessarySpaces (
  IN OUT CHAR16  *String,
  IN     UINTN   StringLength
  )
{
  UINTN  Index;
  UINTN  ActualIndex;

  for (Index = 0, ActualIndex = 0; Index < StringLength; Index++) {
    if (  (String[Index] != L' ')
       || ((ActualIndex > 0) && (String[ActualIndex - 1] != L' ')))
    {
      String[ActualIndex++] = String[Index];
    }
  }

  if (String[ActualIndex - 1] == L' ') {
    String[ActualIndex - 1] = L'\0';
  } else {
    String[ActualIndex] = L'\0';
  }
}

/**
  Return the boot description for NVME boot device.

  @param Handle Controller handle.

  @return       The description string.
**/
CHAR16 *
GetNvmeDescription (
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS                                Status;
  CHAR16                                    *Description;
  CHAR16                                    *Char;
  UINTN                                     Index;
  EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL        *NvmePassthru;
  EFI_DEV_PATH_PTR                          DevicePath;
  EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET  CommandPacket;
  EFI_NVM_EXPRESS_COMMAND                   Command;
  EFI_NVM_EXPRESS_COMPLETION                Completion;
  NVME_ADMIN_CONTROLLER_DATA                ControllerData;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePath.DevPath
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Status = gBS->LocateDevicePath (
                  &gEfiNvmExpressPassThruProtocolGuid,
                  &DevicePath.DevPath,
                  &Handle
                  );
  if (EFI_ERROR (Status) ||
      (DevicePathType (DevicePath.DevPath) != MESSAGING_DEVICE_PATH) ||
      (DevicePathSubType (DevicePath.DevPath) != MSG_NVME_NAMESPACE_DP))
  {
    //
    // Do not return description when the Handle is not a child of NVME controller.
    //
    return NULL;
  }

  //
  // Send ADMIN_IDENTIFY command to NVME controller to get the model and serial number.
  //
  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiNvmExpressPassThruProtocolGuid,
                  (VOID **)&NvmePassthru
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  ZeroMem (&CommandPacket, sizeof (EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof (EFI_NVM_EXPRESS_COMMAND));
  ZeroMem (&Completion, sizeof (EFI_NVM_EXPRESS_COMPLETION));

  Command.Cdw0.Opcode = NVME_ADMIN_IDENTIFY_CMD;
  //
  // According to Nvm Express 1.1 spec Figure 38, When not used, the field shall be cleared to 0h.
  // For the Identify command, the Namespace Identifier is only used for the Namespace data structure.
  //
  Command.Nsid                 = 0;
  CommandPacket.NvmeCmd        = &Command;
  CommandPacket.NvmeCompletion = &Completion;
  CommandPacket.TransferBuffer = &ControllerData;
  CommandPacket.TransferLength = sizeof (ControllerData);
  CommandPacket.CommandTimeout = EFI_TIMER_PERIOD_SECONDS (5);
  CommandPacket.QueueType      = NVME_ADMIN_QUEUE;
  //
  // Set bit 0 (Cns bit) to 1 to identify a controller
  //
  Command.Cdw10 = 1;
  Command.Flags = CDW10_VALID;

  Status = NvmePassthru->PassThru (
                           NvmePassthru,
                           0,
                           &CommandPacket,
                           NULL
                           );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Description = AllocateZeroPool ((ARRAY_SIZE (ControllerData.Mn) + sizeof (CHAR16)) * sizeof (CHAR16));
  if (Description != NULL) {
    Char = Description;
    for (Index = 0; Index < ARRAY_SIZE (ControllerData.Mn); Index++) {
      *(Char++) = (CHAR16)ControllerData.Mn[Index];
    }

    RemoveUnnecessarySpaces (Description, ARRAY_SIZE (ControllerData.Mn));
  }

  return Description;
}

/**
  Try to get the controller's USB description.

  @param Handle Controller handle.

  @retval       The description string.
**/
CHAR16 *
GetUsbDescription (
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS                 Status;
  CHAR16                     NullChar;
  CHAR16                     *Manufacturer;
  CHAR16                     *Product;
  CHAR16                     *Description;
  UINTN                      DescMaxSize;
  EFI_USB_DEVICE_DESCRIPTOR  DevDesc;
  EFI_USB_IO_PROTOCOL        *UsbIo;
  EFI_DEVICE_PATH_PROTOCOL   *UsbDevicePath;
  EFI_HANDLE                 UsbHandle;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&UsbDevicePath
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Status = gBS->LocateDevicePath (
                  &gEfiUsbIoProtocolGuid,
                  &UsbDevicePath,
                  &UsbHandle
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Status = gBS->HandleProtocol (
                  UsbHandle,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **)&UsbIo
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  NullChar = L'\0';

  Status = UsbIo->UsbGetDeviceDescriptor (UsbIo, &DevDesc);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Status = UsbIo->UsbGetStringDescriptor (
                    UsbIo,
                    UsbEnglishLang,
                    DevDesc.StrManufacturer,
                    &Manufacturer
                    );
  if (EFI_ERROR (Status)) {
    Manufacturer = &NullChar;
  }

  Status = UsbIo->UsbGetStringDescriptor (
                    UsbIo,
                    UsbEnglishLang,
                    DevDesc.StrProduct,
                    &Product
                    );
  if (EFI_ERROR (Status)) {
    Product = &NullChar;
  }

  if ((Manufacturer == &NullChar) &&
      (Product == &NullChar))
  {
    return NULL;
  }

  DescMaxSize = StrSize (Manufacturer) + StrSize (Product) + sizeof (CHAR16);
  Description = AllocateZeroPool (DescMaxSize);
  if (Description == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: %d: Out of Resources\n", __func__, __LINE__));
    return NULL;
  }

  StrCatS (Description, DescMaxSize / sizeof (CHAR16), Manufacturer);
  StrCatS (Description, DescMaxSize / sizeof (CHAR16), L" ");
  StrCatS (Description, DescMaxSize / sizeof (CHAR16), Product);

  if (Manufacturer != &NullChar) {
    FreePool (Manufacturer);
  }

  if (Product != &NullChar) {
    FreePool (Product);
  }

  RemoveUnnecessarySpaces (Description, DescMaxSize / sizeof (CHAR16));

  return Description;
}

/**
  Check if Bootable Image exists or not based on file path.

  The SimpleFileSystem protocol is the programmatic access to the FAT (12,16,32)
  file system specified in UEFI 2.8. It can also be used to abstract a file
  system other than FAT.

  SimpleFileSystem protocol is used to open a device volume and return a
  File protocol that provides interfaces to access files on a device volume.

  Use File_protocol.Open() with read mode, if it returns EFI_SUCCESS
  it means file existing; return another, refer to UEFI 2.8 section 13.5.

  @param Handle  The handle for Block IO
  @param Loader  OS loader description

  @retval TRUE   File exists
  @retval FALSE  File does not exists
**/
BOOLEAN
UefiBootableImageExists (
  IN       EFI_HANDLE      Handle,
  IN CONST OS_LOADER_DESC  Loader
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileFs;
  EFI_FILE_PROTOCOL                *Dir;
  EFI_FILE_PROTOCOL                *File;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID *)&FileFs
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  Status = FileFs->OpenVolume (
                     FileFs,
                     &Dir
                     );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  Status = Dir->Open (
                  Dir,
                  &File,
                  Loader.OSLoaderPath,
                  EFI_FILE_MODE_READ,
                  EFI_FILE_RESERVED
                  );
  if (!EFI_ERROR (Status)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Compare two device path nodes from the end node.

  @param  DevicePath1   Pointer to Device path protocol 1
  @param  DevicePath2   Pointer to Device path protocol 2

  @retval TRUE  They are identical
  @retval FALSE They are not identical.
**/
BOOLEAN
CompareDevicePath (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath1,
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath2
  )
{
  UINTN                     Size1;
  UINTN                     Size2;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathList1[3];
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathList2[3];

  DevicePathList1[1] = DevicePath1;
  do {
    DevicePathList1[0] = DevicePathList1[1];
    DevicePathList1[1] = NextDevicePathNode (DevicePathList1[0]);
    DevicePathList1[2] = IsDevicePathEnd (DevicePathList1[1]) ? DevicePathList1[1] : NextDevicePathNode (DevicePathList1[1]);
  } while (!IsDevicePathEnd (DevicePathList1[2]));

  DevicePathList2[1] = DevicePath2;
  do {
    DevicePathList2[0] = DevicePathList2[1];
    DevicePathList2[1] = NextDevicePathNode (DevicePathList2[0]);
    DevicePathList2[2] = IsDevicePathEnd (DevicePathList2[1]) ? DevicePathList2[1] : NextDevicePathNode (DevicePathList2[1]);
  } while (!IsDevicePathEnd (DevicePathList2[2]));

  Size1 = GetDevicePathSize (DevicePathList1[0]);
  Size2 = GetDevicePathSize (DevicePathList2[0]);

  if (Size1 != Size2) {
    return FALSE;
  }

  if (CompareMem (DevicePathList1[0], DevicePathList2[0], Size1) != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Check Boot Option, which has a specific Device path, whether it exists.

  @param  DevicePath Pointer to Device Path Protocol of Boot Option

  @retval TRUE       Boot Option for device path exists
  @retval FALSE      Boot Option for device path does not exists
 **/
BOOLEAN
IsBootOptionAvailable (
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  UINTN                         Index;
  UINTN                         NvBootOptionCount;
  EFI_BOOT_MANAGER_LOAD_OPTION  *NvBootOptions;

  NvBootOptions = EfiBootManagerGetLoadOptions (&NvBootOptionCount, LoadOptionTypeBoot);

  for (Index = 0; Index < NvBootOptionCount; Index++) {
    //
    // Compare device path
    //
    if (CompareDevicePath (DevicePath, NvBootOptions[Index].FilePath)) {
      //
      // With Boot Option which is not automatically created,
      // mark it as already existing, do not need to create more.
      //
      if (NvBootOptions[Index].OptionalData == NULL) {
        return TRUE;
      }

      //
      // Boot Manager will eliminate all Boot Options
      // which are automatically created if in the next cycle,
      // they are not created again. So we need to create
      // them again.
      //
      if (CompareGuid (
            (CONST GUID *)&mBmAutoCreateBootOptionGuid,
            (CONST GUID *)NvBootOptions[Index].OptionalData
            ))
      {
        return FALSE;
      }

      return TRUE;
    }
  }

  return FALSE;
}

EFI_STATUS
CreateNewBootOption (
  IN  EFI_HANDLE                    Handle,
  IN  OS_LOADER_DESC                OsDescription,
  OUT EFI_BOOT_MANAGER_LOAD_OPTION  **BootOptionList,
  OUT UINTN                         *BootOptionCount
  )
{
  EFI_STATUS                Status;
  CHAR16                    BootOptionDescription[128];
  CHAR16                    *DiskName;
  EFI_DEVICE_PATH_PROTOCOL  *BootOptionDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *Node;

  DiskName = NULL;

  if (!UefiBootableImageExists (Handle, OsDescription)) {
    return EFI_NOT_FOUND;
  }

  BootOptionDevicePath = FileDevicePath (
                           Handle,
                           OsDescription.OSLoaderPath
                           );
  if (BootOptionDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Skip if Boot Option already exists
  //
  if (IsBootOptionAvailable (BootOptionDevicePath)) {
    return EFI_SUCCESS;
  }

  //
  // Create boot option name into format of "OS Name (Disk Name)"
  //
  Node = BootOptionDevicePath;
  while (Node->Type != END_DEVICE_PATH_TYPE) {
    if (Node->Type == MESSAGING_DEVICE_PATH) {
      switch (Node->SubType) {
        case MSG_NVME_NAMESPACE_DP:
          DiskName = GetNvmeDescription (Handle);
          break;

        case MSG_USB_DP:
          DiskName = GetUsbDescription (Handle);
          break;

        default:
          DiskName = NULL;
          break;
      }

      break;
    }

    Node = NextDevicePathNode (Node);
  }

  if (DiskName != NULL) {
    UnicodeSPrint (
      BootOptionDescription,
      sizeof (BootOptionDescription),
      L"%s (%s)",
      OsDescription.OSLoaderName,
      DiskName
      );
    FreePool (DiskName);
  } else {
    UnicodeSPrint (
      BootOptionDescription,
      sizeof (BootOptionDescription),
      L"%s",
      OsDescription.OSLoaderName
      );
  }

  Status = AppendBootOption (
             BootOptionList,
             *BootOptionCount,
             NULL,
             BootOptionDescription,
             BootOptionDevicePath,
             NULL,
             0
             );
  if (!EFI_ERROR (Status)) {
    (*BootOptionCount)++;
  }

  if (BootOptionDevicePath != NULL) {
    FreePool (BootOptionDevicePath);
  }

  return Status;
}

EFI_STATUS
AddCustomBootOptionToList (
  OUT EFI_BOOT_MANAGER_LOAD_OPTION  **BootOptionList,
  OUT UINTN                         *BootOptionCount
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  UINTN       OrderInList;
  UINTN       HandleCount;
  BOOLEAN     NeedDefaultBoot;
  EFI_HANDLE  *Handle;

  Handle = NULL;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &HandleCount,
                  &Handle
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate handle buffer for Simple FS Protocol - %r\n", Status));
    return Status;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    for (OrderInList = 0, NeedDefaultBoot = TRUE;
         KnownOsLoaderList[OrderInList].OSLoaderPath != NULL;
         OrderInList++)
    {
      Status = CreateNewBootOption (
                 Handle[Index],
                 KnownOsLoaderList[OrderInList],
                 BootOptionList,
                 BootOptionCount
                 );
      if (!EFI_ERROR (Status)) {
        NeedDefaultBoot = FALSE;
      }
    }

    if (NeedDefaultBoot) {
      CreateNewBootOption (
        Handle[Index],
        DefaultUefiLoader,
        BootOptionList,
        BootOptionCount
        );
    }
  }

  if (Handle != NULL) {
    FreePool (Handle);
  }

  return EFI_SUCCESS;
}

/**
  Caching the Boot Options valid enumerated by default.

  @param LegacyBootOptionList The list of Boot Options enumerated by default
  @param LegacyBootOptionListCount The number of Boot Option in legacy list
  @param BootOptionList            The list that legacy Boot Options valid stored
  @param BootOptionCount      The number of Boot Option in new list.

  @retval VOID
**/
VOID
CacheLegacyBootOptions (
  IN  CONST EFI_BOOT_MANAGER_LOAD_OPTION  *LegacyBootOptionList,
  IN  CONST UINTN                         LegacyBootOptionListCount,
  OUT       EFI_BOOT_MANAGER_LOAD_OPTION  **BootOptionList,
  IN OUT    UINTN                         *BootOptionListCount
  )
{
  EFI_STATUS                    Status;
  UINTN                         Index;
  UINTN                         NVBootOptionCount;
  UINTN                         CacheBootOptionCount;
  EFI_BOOT_MANAGER_LOAD_OPTION  *NVBootOptions;

  NVBootOptionCount    = 0;
  NVBootOptions        = NULL;
  CacheBootOptionCount = *BootOptionListCount;

  //
  // Load Boot Options have been stored in the NVRAM
  //
  NVBootOptions = EfiBootManagerGetLoadOptions (&NVBootOptionCount, LoadOptionTypeBoot);

  for (Index = 0; Index < LegacyBootOptionListCount; Index++) {
    if (  (EfiBootManagerFindLoadOption (
             &LegacyBootOptionList[Index],
             (CONST EFI_BOOT_MANAGER_LOAD_OPTION *)NVBootOptions,
             NVBootOptionCount
             ) != -1)
       && (EfiBootManagerFindLoadOption (
             &LegacyBootOptionList[Index],
             (CONST EFI_BOOT_MANAGER_LOAD_OPTION *)*BootOptionList,
             CacheBootOptionCount
             ) == -1)
          )
    {
      Status = AppendBootOption (
                 BootOptionList,
                 *BootOptionListCount,
                 (EFI_BOOT_MANAGER_LOAD_OPTION *)&LegacyBootOptionList[Index],
                 NULL,
                 NULL,
                 NULL,
                 0
                 );
      if (!EFI_ERROR (Status)) {
        (*BootOptionListCount)++;
      }
    }
  }
}

EFI_STATUS
RefreshAllBootOptions (
  IN  CONST EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptions,
  IN  CONST UINTN                         BootOptionsCount,
  OUT       EFI_BOOT_MANAGER_LOAD_OPTION  **UpdatedBootOptions,
  OUT       UINTN                         *UpdatedBootOptionsCount
  )
{
  EFI_STATUS                    Status;
  BOOLEAN                       DoCustomBootOptionsAdd;
  UINTN                         Index;
  EFI_BOOT_MANAGER_LOAD_OPTION  *BootOptionTemp;

  *UpdatedBootOptions      = NULL;
  *UpdatedBootOptionsCount = 0;
  DoCustomBootOptionsAdd   = TRUE;

  if ((BootOptionsCount == 0) || (BootOptions == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Add custom Boot Options
  //
  Status = AddCustomBootOptionToList (
             UpdatedBootOptions,
             UpdatedBootOptionsCount
             );
  if (EFI_ERROR (Status)) {
    if (*UpdatedBootOptions != NULL) {
      FreePool (*UpdatedBootOptions);
    }

    *UpdatedBootOptions      = NULL;
    *UpdatedBootOptionsCount = 0;
    DoCustomBootOptionsAdd   = FALSE;
  }

  //
  // Collect Boot Options that are enumerated by Boot Manager
  //
  for (Index = 0; Index < BootOptionsCount; Index++) {
    //
    // If custom boot options are successfully added, bypass those options
    // based on Block IO Protocol
    //
    if (IsBlockIoProtocolInstalled (BootOptions[Index].FilePath) && DoCustomBootOptionsAdd) {
      continue;
    }

    Status = AppendBootOption (
               UpdatedBootOptions,
               *UpdatedBootOptionsCount,
               (EFI_BOOT_MANAGER_LOAD_OPTION *)&BootOptions[Index],
               NULL,
               NULL,
               NULL,
               0
               );
    if (!EFI_ERROR (Status)) {
      (*UpdatedBootOptionsCount)++;
    }
  }

  CacheLegacyBootOptions (
    BootOptions,
    BootOptionsCount,
    UpdatedBootOptions,
    UpdatedBootOptionsCount
    );

  for (Index = 0, BootOptionTemp = *UpdatedBootOptions;
       Index < *UpdatedBootOptionsCount;
       Index++)
  {
    if (BootOptionTemp[Index].OptionalData != NULL) {
      FreePool (BootOptionTemp[Index].OptionalData);
    }

    BootOptionTemp[Index].OptionalData     = AllocateCopyPool (sizeof (EFI_GUID), &mBmAutoCreateBootOptionGuid);
    BootOptionTemp[Index].OptionalDataSize = sizeof (EFI_GUID);
  }

  return Status;
}

EDKII_PLATFORM_BOOT_MANAGER_PROTOCOL  mPlatformBootManager = {
  EDKII_PLATFORM_BOOT_MANAGER_PROTOCOL_REVISION,
  RefreshAllBootOptions
};

EFI_STATUS
PlatformBootManagerInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                            Status;
  UINTN                                 Index;
  UINTN                                 HandleCount;
  EDKII_PLATFORM_BOOT_MANAGER_PROTOCOL  *Interface;
  EFI_HANDLE                            *Buffer, Handle;

  Buffer = NULL;
  Handle = NULL;
  //
  // Check existing protocol
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEdkiiPlatformBootManagerProtocolGuid,
                  NULL,
                  &HandleCount,
                  &Buffer
                  );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < HandleCount; Index++) {
      Status = gBS->HandleProtocol (
                      Buffer[Index],
                      &gEdkiiPlatformBootManagerProtocolGuid,
                      (VOID **)&Interface
                      );
      if (EFI_ERROR (Status)) {
        continue;
      }

      Status = gBS->UninstallProtocolInterface (
                      Buffer[Index],
                      &gEdkiiPlatformBootManagerProtocolGuid,
                      (VOID *)Interface
                      );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Failed to uninstall existing PlatformBootManagerProtocol - %r\n", Status));
        goto Done;
      }
    }
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEdkiiPlatformBootManagerProtocolGuid,
                  &mPlatformBootManager,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to install PlatformBootManager Protocol - %r\n", Status));
  }

Done:
  if (Buffer != NULL) {
    FreePool (Buffer);
  }

  return Status;
}
