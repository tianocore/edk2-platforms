/** @file
  Implements CapsuleHookLib.c

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <PiDxe.h>
#include <Guid/Gpt.h>
#include <Guid/FileSystemInfo.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/BlockIo.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/FileHandleLib.h>
#include <Library/DevicePathLib.h>
#include <Library/CapsuleLib.h>
#include <Library/CapsuleHookLib.h>
#include <OtaCapsuleUpdate.h>

#define CAP_FILE_NAME  (CHAR16 *) FixedPcdGetPtr (PcdOtaCapsuleName)
#define CAP_PARTITION  (CHAR16 *) FixedPcdGetPtr (PcdOtaCapsulePartitionName)
CHAR16                    mPartitionName[36];
STATIC CONST CHAR16       *mSlotSuffixes[2] = { L"_a", L"_b" };
EFI_DEVICE_PATH_PROTOCOL  *mBootDevicePath;

#define FLASH_DEVICE_PATH_SIZE(DevPath)  (GetDevicePathSize (DevPath) - sizeof (EFI_DEVICE_PATH_PROTOCOL))

// Hidden (Not exported) function from DxeCapsuleReportLib.
extern
EFI_STATUS
// Not EFIAPI!
RecordCapsuleStatusVariable (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader,
  IN EFI_STATUS          CapsuleStatus
  );

/**
  Read GPT partition entries.

  @param[in]  BlockIo             The BlockIo of device.
  @param[out] PartitionEntries    The output buffer of partition entry.

  @retval EFI_SUCCESS             Operation completed successfully.
  @retval others                  Some error occurs when executing this routine.

**/
EFI_STATUS
EFIAPI
ReadPartitionEntries (
  IN EFI_BLOCK_IO_PROTOCOL  *BlockIo,
  OUT EFI_PARTITION_ENTRY   **PartitionEntries
  )
{
  EFI_STATUS                  Status;
  UINTN                       EntrySize;
  UINTN                       NumEntries;
  UINTN                       BufferSize;
  UINT32                      MediaId;
  EFI_PARTITION_TABLE_HEADER  *GptHeader;

  MediaId = BlockIo->Media->MediaId;

  //
  // Read size of Partition entry and number of entries from GPT header
  //
  GptHeader = AllocatePool (BlockIo->Media->BlockSize);
  if (GptHeader == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = BlockIo->ReadBlocks (
                      BlockIo,
                      MediaId,
                      PRIMARY_PART_HEADER_LBA,
                      BlockIo->Media->BlockSize,
                      (VOID *)GptHeader
                      );
  if (EFI_ERROR (Status)) {
    FreePool (GptHeader);
    return Status;
  }

  //
  // Check there is a GPT on the media
  //
  if ((GptHeader->Header.Signature != EFI_PTAB_HEADER_ID) || (GptHeader->MyLBA != PRIMARY_PART_HEADER_LBA)) {
    DEBUG ((DEBUG_ERROR, "No valid GPT found!\n"));
    FreePool (GptHeader);
    return EFI_DEVICE_ERROR;
  }

  EntrySize  = GptHeader->SizeOfPartitionEntry;
  NumEntries = GptHeader->NumberOfPartitionEntries;
  if ((EntrySize == 0) || (NumEntries == 0)) {
    DEBUG ((DEBUG_ERROR, "Invalid Entry size or number.\n"));
    return EFI_DEVICE_ERROR;
  }

  FreePool (GptHeader);

  BufferSize        = ALIGN_VALUE (EntrySize * NumEntries, BlockIo->Media->BlockSize);
  *PartitionEntries = AllocatePool (BufferSize);
  if (PartitionEntries == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = BlockIo->ReadBlocks (
                      BlockIo,
                      MediaId,
                      2,
                      BufferSize,
                      (VOID *)*PartitionEntries
                      );
  if (EFI_ERROR (Status)) {
    FreePool (*PartitionEntries);
    return Status;
  }

  return Status;
}

/**
  Get capsule partition device path by partition name.

  @param[in]  BootDevicePath      Pointer to the Device Path Protocol from variable.
  @param[in]  PartitionName       The given partition name.
  @param[out] PartitionDevicePath Pointer to the Device Path Protocol of capsule partition.

  @retval EFI_SUCCESS             Operation completed successfully.
  @retval others                  Some error occurs when executing this routine.

**/
EFI_STATUS
EFIAPI
GetDevicePathByName (
  IN  EFI_DEVICE_PATH_PROTOCOL  *BootDevicePath,
  IN  CONST CHAR16              *PartitionName,
  OUT EFI_DEVICE_PATH_PROTOCOL  **PartitionDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_BLOCK_IO_PROTOCOL     *BlockIo;
  EFI_DEVICE_PATH_PROTOCOL  *NextNode;
  HARDDRIVE_DEVICE_PATH     *PartitionNode;
  EFI_PARTITION_ENTRY       *PartitionEntries;
  UINTN                     NumHandles;
  UINTN                     LoopIndex;
  EFI_HANDLE                *AllHandles;
  EFI_HANDLE                Handle;

  //
  // Get all BlockIo handles in system
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiBlockIoProtocolGuid,
                  NULL,
                  &NumHandles,
                  &AllHandles
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Couldn't locate BlockIo protocol: %r\n", Status));
    return Status;
  }

  for (LoopIndex = 0; LoopIndex < NumHandles; LoopIndex++) {
    Status = gBS->OpenProtocol (
                    AllHandles[LoopIndex],
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&DevicePath,
                    gImageHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Couldn't locate DevicePath protocol: %r\n", Status));
      return Status;
    }

    if (!CompareMem (DevicePath, BootDevicePath, FLASH_DEVICE_PATH_SIZE (BootDevicePath))) {
      BootDevicePath = DevicePath;
      break;
    }
  }

  DevicePath = BootDevicePath;
  Status     = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &DevicePath, &Handle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Couldn't locate device status: %r\n", Status));
    return Status;
  }

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **)&BlockIo,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to get BlockIo: %r\n", Status));
    return Status;
  }

  Status = ReadPartitionEntries (BlockIo, &PartitionEntries);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to read partitions from disk device: %r\n", Status));
    return Status;
  }

  for (LoopIndex = 0; LoopIndex < NumHandles; LoopIndex++) {
    Status = gBS->OpenProtocol (
                    AllHandles[LoopIndex],
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&DevicePath,
                    gImageHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Couldn't locate DevicePath protocol: %r\n", Status));
      return Status;
    }

    if (!CompareMem (DevicePath, BootDevicePath, FLASH_DEVICE_PATH_SIZE (BootDevicePath))) {
      NextNode = NextDevicePathNode (DevicePath);

      while (!IsDevicePathEndType (NextNode)) {
        if ((NextNode->Type == MEDIA_DEVICE_PATH) &&
            (NextNode->SubType == MEDIA_HARDDRIVE_DP))
        {
          break;
        }

        NextNode = NextDevicePathNode (NextNode);
      }

      if (IsDevicePathEndType (NextNode)) {
        continue;
      }

      PartitionNode = (HARDDRIVE_DEVICE_PATH *)NextNode;

      if (PartitionNode->PartitionNumber == 0) {
        continue;
      }

      if (0 == StrCmp (PartitionEntries[PartitionNode->PartitionNumber - 1].PartitionName, PartitionName)) {
        break;
      }
    }
  }

  if (LoopIndex >= NumHandles) {
    return EFI_LOAD_ERROR;
  }

  *PartitionDevicePath = DevicePath;

  return EFI_SUCCESS;
}

/**
  Get capsule paritioin information.

  @param[in]  VOID

  @retval EFI_SUCCESS             Operation completed successfully.
  @retval others                  Some error occurs when executing this routine.

**/
EFI_STATUS
EFIAPI
CapsulePartitionInfo (
  VOID
  )
{
  EFI_STATUS          Status;
  UINTN               VarSize;
  OTA_CAPSULE_UPDATE  OtaCapsuleUpdateVal;
  CHAR16              BootPath[512];

  //
  // Get Capsule A/B partition.
  //
  ZeroMem (&OtaCapsuleUpdateVal, sizeof (OTA_CAPSULE_UPDATE));
  VarSize = sizeof (OTA_CAPSULE_UPDATE);
  Status  = gRT->GetVariable (
                   OTA_CAPSULE_VAR_NAME,
                   &gOtaCapsuleUpdateGuid,
                   NULL,
                   &VarSize,
                   (VOID *)&OtaCapsuleUpdateVal
                   );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "CapsulePartitionInfo: GetVariable failed: %r\n", Status));
    return Status;
  }

  ZeroMem (mPartitionName, sizeof (mPartitionName));
  StrCpyS (mPartitionName, sizeof (mPartitionName), CAP_PARTITION);
  StrCatS (mPartitionName, sizeof (mPartitionName), mSlotSuffixes[OtaCapsuleUpdateVal.UpdateSlot]);
  DEBUG ((DEBUG_INFO, "CapsulePartitionInfo from partition: %s\n", mPartitionName));

  //
  // Get Boot device path
  //
  VarSize = 512;
  Status  = gRT->GetVariable (
                   L"AndroidBootDevice",
                   &gEfiGlobalVariableGuid,
                   NULL,
                   &VarSize,
                   BootPath
                   );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "CapsulePartitionInfo: Get BootDevice variable failed: %r\n", Status));
    return Status;
  }

  mBootDevicePath = ConvertTextToDevicePath (BootPath);
  if (mBootDevicePath == NULL) {
    DEBUG ((DEBUG_ERROR, "mBootDevicePath is NULL\n"));
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Read Capsule file information from FAT partiton.

  @param[in]  FileName              File name of Capsule binary
  @param[out] Buffer                Return buffer pointer of Capsule binary
  @param[out] BufferSize            Capsule binary size

  @retval EFI_SUCCESS               Read Capsule information successfully
  @retval EFI_OUT_OF_RESOURCES      No enough buffer to allocate
  @retval EFI_NOT_FOUND             Fail to read Capsule information
  @retval Others                    Internal error when read Capsule information

**/
EFI_STATUS
EFIAPI
ReadCapsuleInfo (
  IN CHAR16  *FileName,
  OUT VOID   **Buffer,
  OUT UINTN  *BufferSize
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *Fs;
  EFI_FILE                         *Root;
  EFI_FILE                         *FileHandle;
  UINT8                            *FileBuffer;
  UINTN                            FileSize;
  EFI_HANDLE                       Handle;
  EFI_BLOCK_IO_PROTOCOL            *BlockIo;
  EFI_DEVICE_PATH_PROTOCOL         *BootPartitionPath;

  FileBuffer = NULL;
  FileSize   = 0;

  DEBUG ((DEBUG_INFO, "ReadCapsuleInfo()...\n"));

  Status = GetDevicePathByName (mBootDevicePath, mPartitionName, &BootPartitionPath);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "GetDevicePathByName failed: %r\n", Status));
    return Status;
  }

  Status = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &BootPartitionPath, &Handle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ReadCapsuleInfo: Locate DevicePath failed: %r\n", Status));
    return Status;
  }

  //
  // Get Capsule file
  //
  Status = gBS->HandleProtocol (Handle, &gEfiBlockIoProtocolGuid, (VOID **)&BlockIo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (BlockIo->Media->RemovableMedia) {
    return Status;
  }

  Status = gBS->HandleProtocol (Handle, &gEfiSimpleFileSystemProtocolGuid, (VOID **)&Fs);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Fs->OpenVolume (Fs, &Root);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Root->Open (Root, &FileHandle, FileName, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (FileHandle == NULL) {
    return EFI_NOT_FOUND;
  }

  Status = FileHandleGetSize (FileHandle, (UINT64 *)&FileSize);
  if (EFI_ERROR (Status)) {
    FileHandleClose (FileHandle);
    return Status;
  }

  FileBuffer = AllocateZeroPool (FileSize);
  if (FileBuffer == NULL) {
    FileHandleClose (FileHandle);
    return Status;
  }

  Status = FileHandleRead (FileHandle, &FileSize, FileBuffer);
  if (EFI_ERROR (Status)) {
    FileHandleClose (FileHandle);
    FreePool (FileBuffer);
    return Status;
  }

  Status = FileHandleClose (FileHandle);
  if (EFI_ERROR (Status)) {
    FreePool (FileBuffer);
    return Status;
  }

  *Buffer     = FileBuffer;
  *BufferSize = FileSize;

  DEBUG ((DEBUG_INFO, "Capsule size: 0x%x\n", *BufferSize));

  return EFI_SUCCESS;
}

/**
  Remove capsule file from FAT partitions.

  @param[in]  FileName              File name of Capsule binary

  @retval EFI_SUCCESS               Delete capsule succesfully
  @retval Others                    Internal error of delete capsule function

**/
EFI_STATUS
EFIAPI
RemoveCapsuleFile (
  IN CHAR16  *FileName
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *Fs;
  EFI_FILE                         *Root;
  EFI_FILE                         *FileHandle;
  EFI_HANDLE                       Handle;
  EFI_BLOCK_IO_PROTOCOL            *BlockIo;
  EFI_DEVICE_PATH_PROTOCOL         *BootPartitionPath;

  DEBUG ((DEBUG_INFO, "RemoveCapsuleFile()...\n"));

  Status = GetDevicePathByName (mBootDevicePath, mPartitionName, &BootPartitionPath);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "GetDevicePathByName failed: %r\n", Status));
    return Status;
  }

  Status = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &BootPartitionPath, &Handle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ReadCapsuleInfo: Locate DevicePath failed: %r\n", Status));
    return Status;
  }

  //
  // Remove Capsule file
  //
  Status = gBS->HandleProtocol (Handle, &gEfiBlockIoProtocolGuid, (VOID **)&BlockIo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (BlockIo->Media->RemovableMedia) {
    return Status;
  }

  Status = gBS->HandleProtocol (Handle, &gEfiSimpleFileSystemProtocolGuid, (VOID **)&Fs);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Fs->OpenVolume (Fs, &Root);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Root->Open (Root, &FileHandle, FileName, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (FileHandle == NULL) {
    return EFI_NOT_FOUND;
  }

  Status = FileHandleDelete (FileHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Passes and processes the capsule file.

  @param  CapsuleHeaderArray    Virtual pointer to an array of virtual pointers to the capsules
                                being passed into update capsule.
  @param  CapsuleCount          Number of pointers to EFI_CAPSULE_HEADER in CaspuleHeaderArray.

  @retval EFI_SUCCESS           Valid capsule was passed.
  @retval others                Some error occurs when executing this routine.

**/
EFI_STATUS
EFIAPI
UpdateCapsule (
  IN EFI_CAPSULE_HEADER  **CapsuleHeaderArray,
  IN UINTN               CapsuleCount
  )
{
  UINTN               ArrayNumber;
  EFI_STATUS          Status;
  EFI_CAPSULE_HEADER  *CapsuleHeader;

  //
  // Capsule Count can't be less than one.
  //
  if (CapsuleCount < 1) {
    return EFI_INVALID_PARAMETER;
  }

  CapsuleHeader = NULL;

  for (ArrayNumber = 0; ArrayNumber < CapsuleCount; ArrayNumber++) {
    //
    // A capsule which has the CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE flag must have
    // CAPSULE_FLAGS_PERSIST_ACROSS_RESET set in its header as well.
    //
    CapsuleHeader = CapsuleHeaderArray[ArrayNumber];
    if ((CapsuleHeader->Flags & (CAPSULE_FLAGS_PERSIST_ACROSS_RESET | CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE)) == CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // A capsule which has the CAPSULE_FLAGS_INITIATE_RESET flag must have
    // CAPSULE_FLAGS_PERSIST_ACROSS_RESET set in its header as well.
    //
    if ((CapsuleHeader->Flags & (CAPSULE_FLAGS_PERSIST_ACROSS_RESET | CAPSULE_FLAGS_INITIATE_RESET)) == CAPSULE_FLAGS_INITIATE_RESET) {
      return EFI_INVALID_PARAMETER;
    }

    // FIXME: The CoD Image CANNOT BE RELOADED as Memory capsule.
    //
    // Check FMP capsule flag
    //
    if (  CompareGuid (&CapsuleHeader->CapsuleGuid, &gEfiFmpCapsuleGuid)
       && ((CapsuleHeader->Flags & CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) != 0))
    {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Check Capsule image without populate flag by firmware support capsule function
    //
    if ((CapsuleHeader->Flags & CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) == 0) {
      Status = SupportCapsuleImage (CapsuleHeader);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }

  //
  // Walk through all capsules, record whether there is a capsule needs reset
  // or initiate reset. And then process capsules which has no reset flag directly.
  //
  for (ArrayNumber = 0; ArrayNumber < CapsuleCount; ArrayNumber++) {
    CapsuleHeader = CapsuleHeaderArray[ArrayNumber];
    //
    // Here should be in the boot-time for non-reset capsule image
    // Platform specific update for the non-reset capsule image.
    //

    // FIXME: The CoD Image CANNOT BE RELOADED as Memory capsule.
    if (((CapsuleHeader->Flags & CAPSULE_FLAGS_PERSIST_ACROSS_RESET) == 0) || TRUE) {
      Status = ProcessCapsuleImage (CapsuleHeader);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Detect Capsule file from ESP partition and update capsule.

  @retval EFI_SUCCESS               Opertion is successful.
  @retval EFI_OUT_OF_RESOURCES      No enough buffer to allocate.
  @retval EFI_ERROR                 Internal error when update Capsule.

**/
EFI_STATUS
EFIAPI
CapsuleUpdateViaFileHook (
  VOID
  )
{
  EFI_STATUS          Status;
  UINT8               *CapsuleBuffer;
  UINTN               CapsuleSize;
  EFI_CAPSULE_HEADER  *CapsuleHeader;
  UINTN               CapsuleNum;

  CapsuleBuffer = NULL;
  CapsuleSize   = 0;

  DEBUG ((DEBUG_INFO, "CapsuleUpdateViaFileHook() Entry Point...\n"));

  Status = CapsulePartitionInfo ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "CapsulePartitionInfo failed: %r\n", Status));
    return Status;
  }

  Status = ReadCapsuleInfo (CAP_FILE_NAME, (VOID **)&CapsuleBuffer, &CapsuleSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Read Capsule file failed. Status: %r\n", Status));
    if (CapsuleBuffer != NULL) {
      FreePool (CapsuleBuffer);
    }

    return Status;
  }

  CapsuleHeader = (EFI_CAPSULE_HEADER *)CapsuleBuffer;
  CapsuleNum    = 1;

  Status = UpdateCapsule (&CapsuleHeader, CapsuleNum);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to update capsule. Status: %r\n", Status));
  }

  if (CapsuleBuffer != NULL) {
    FreePool (CapsuleBuffer);
  }

  DEBUG ((DEBUG_INFO, "Capsule update via file completed, reset system...\n"));

  gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);

  return EFI_SUCCESS;
}

/**
  Get capsule partition device path by partition name.

  @param[in]  BootDevicePath      Pointer to the Device Path Protocol from variable.
  @param[in]  PartitionName       The given partition name.
  @param[out] PartitionDevicePath Pointer to the Device Path Protocol of capsule partition.

  @retval EFI_SUCCESS             Operation completed successfully.
  @retval others                  Some error occurs when executing this routine.

**/
EFI_STATUS
EFIAPI
GetDevicePathByBoot (
  IN  EFI_DEVICE_PATH_PROTOCOL  *BootDevicePath,
  OUT EFI_DEVICE_PATH_PROTOCOL  **PartitionDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_BLOCK_IO_PROTOCOL     *BlockIo;
  EFI_DEVICE_PATH_PROTOCOL  *NextNode;
  UINTN                     NumHandles;
  UINTN                     LoopIndex;
  EFI_HANDLE                *AllHandles;
  EFI_HANDLE                Handle;

  //
  // Get all BlockIo handles in system
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiBlockIoProtocolGuid,
                  NULL,
                  &NumHandles,
                  &AllHandles
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Couldn't locate BlockIo protocol: %r\n", Status));
    return Status;
  }

  for (LoopIndex = 0; LoopIndex < NumHandles; LoopIndex++) {
    Status = gBS->OpenProtocol (
                    AllHandles[LoopIndex],
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&DevicePath,
                    gImageHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Couldn't locate DevicePath protocol: %r\n", Status));
      return Status;
    }

    if (!CompareMem (DevicePath, BootDevicePath, FLASH_DEVICE_PATH_SIZE (BootDevicePath))) {
      BootDevicePath = DevicePath;
      break;
    }
  }

  DevicePath = BootDevicePath;
  Status     = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &DevicePath, &Handle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Couldn't locate device status: %r\n", Status));
    return Status;
  }

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **)&BlockIo,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to get BlockIo: %r\n", Status));
    return Status;
  }

  for (LoopIndex = 0; LoopIndex < NumHandles; LoopIndex++) {
    Status = gBS->OpenProtocol (
                    AllHandles[LoopIndex],
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&DevicePath,
                    gImageHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Couldn't locate DevicePath protocol: %r\n", Status));
      return Status;
    }

    if (!CompareMem (DevicePath, BootDevicePath, FLASH_DEVICE_PATH_SIZE (BootDevicePath))) {
      NextNode = NextDevicePathNode (DevicePath);

      while (!IsDevicePathEndType (NextNode)) {
        if ((NextNode->Type == MEDIA_DEVICE_PATH) &&
            (NextNode->SubType == MEDIA_HARDDRIVE_DP))
        {
          break;
        }

        NextNode = NextDevicePathNode (NextNode);
      }

      if (IsDevicePathEndType (NextNode)) {
        continue;
      }

      break;
    }
  }

  if (LoopIndex >= NumHandles) {
    return EFI_LOAD_ERROR;
  }

  *PartitionDevicePath = DevicePath;

  return EFI_SUCCESS;
}

/**
  Get capsule paritioin information.

  @param[in]  VOID

  @retval EFI_SUCCESS             Operation completed successfully.
  @retval others                  Some error occurs when executing this routine.

**/
EFI_STATUS
EFIAPI
CapsulePathInfo (
  VOID
  )
{
  mBootDevicePath = ConvertTextToDevicePath ((CHAR16 *)PcdGetPtr (PcdNVMeDevicePath));

  if (mBootDevicePath == NULL) {
    DEBUG ((DEBUG_ERROR, "mBootDevicePath is NULL\n"));
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Read Capsule file information from FAT partiton.

  @param[in]  FileName              File name of Capsule binary
  @param[out] Buffer                Return buffer pointer of Capsule binary
  @param[out] BufferSize            Capsule binary size

  @retval EFI_SUCCESS               Read Capsule information successfully
  @retval EFI_OUT_OF_RESOURCES      No enough buffer to allocate
  @retval EFI_NOT_FOUND             Fail to read Capsule information
  @retval Others                    Internal error when read Capsule information

**/
EFI_STATUS
EFIAPI
IterateAllCapsulesFromDisk (
  IN CHAR16    *FileBaseName,
  OUT VOID     **Buffer,
  OUT UINTN    *BufferSize,
  OUT BOOLEAN  *NoNextFile
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *Fs;
  EFI_FILE                         *Root;
  EFI_FILE                         *DirHandle;
  EFI_FILE                         *FileHandle;
  EFI_FILE_INFO                    *FileInfo;
  UINT8                            *FileBuffer;
  EFI_HANDLE                       Handle;
  EFI_BLOCK_IO_PROTOCOL            *BlockIo;
  EFI_DEVICE_PATH_PROTOCOL         *BootPartitionPath;
  BOOLEAN                          Found = FALSE;

  FileBuffer = NULL;

  DEBUG ((DEBUG_INFO, "ReadCapsuleInfo()...\n"));

  Status = GetDevicePathByBoot (mBootDevicePath, &BootPartitionPath);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "GetDevicePathByName failed: %r\n", Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "IGpuDevicePath: %S\n", ConvertDevicePathToText (BootPartitionPath, FALSE, FALSE)));

  Status = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &BootPartitionPath, &Handle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ReadCapsuleInfo: Locate DevicePath failed: %r\n", Status));
    return Status;
  }

  //
  // Get Capsule file
  //
  Status = gBS->HandleProtocol (Handle, &gEfiBlockIoProtocolGuid, (VOID **)&BlockIo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ReadCapsuleInfo: 1Locate DevicePath failed: %r\n", Status));
    return Status;
  }

  if (BlockIo->Media->RemovableMedia) {
    DEBUG ((DEBUG_ERROR, "ReadCapsuleInfo: 2Locate DevicePath failed: %r\n", Status));
    return Status;
  }

  Status = gBS->HandleProtocol (Handle, &gEfiSimpleFileSystemProtocolGuid, (VOID **)&Fs);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ReadCapsuleInfo: 3Locate DevicePath failed: %r\n", Status));
    return Status;
  }

  Status = Fs->OpenVolume (Fs, &Root);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ReadCapsuleInfo: 4Locate DevicePath failed: %r\n", Status));
    return Status;
  }

  Status = Root->Open (Root, &DirHandle, FileBaseName, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Cannot open %s. Status = %r\n", FileBaseName, Status));
    return Status;
  }

  //
  // Get file count first
  //
  Status = FileHandleFindFirstFile (DirHandle, &FileInfo);
  while (!*NoNextFile && !Found) {
    // . & ..
    Status = FileHandleFindNextFile (DirHandle, FileInfo, NoNextFile);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (FileInfo->Attribute & EFI_FILE_DIRECTORY) {
      continue;
    }

    Found = TRUE;
  }

  if (!Found) {
    *NoNextFile = TRUE;
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_INFO, "Processing Capsule %s\n", FileInfo->FileName));
  FileBuffer = AllocateZeroPool (FileInfo->FileSize);
  if (FileBuffer == NULL) {
    return EFI_BUFFER_TOO_SMALL;
  }

  Status = DirHandle->Open (DirHandle, &FileHandle, FileInfo->FileName, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ReadCapsuleInfo Cannot open file %s: %r\n", FileInfo->FileName, Status));
    FreePool (FileBuffer);
    return Status;
  }

  Status = FileHandleRead (FileHandle, &FileInfo->FileSize, FileBuffer);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ReadCapsuleInfo: 7Locate DevicePath failed: %r\n", Status));
    FileHandleClose (FileHandle);
    FreePool (FileBuffer);
    return Status;
  }

  if (!*NoNextFile) {
    Status = FileHandleClose (DirHandle);
  }

  Status = FileHandleClose (FileHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ReadCapsuleInfo: 8Locate DevicePath failed: %r\n", Status));
    FreePool (FileBuffer);
    return Status;
  }

  *Buffer     = FileBuffer;
  *BufferSize = FileInfo->FileSize;

  DEBUG ((DEBUG_INFO, "Capsule size: 0x%x\n", *BufferSize));

  return EFI_SUCCESS;
}

/**
  Read Capsule file information from FAT partiton.

  @param[in]  FileBaseName              File name of Capsule binary

  @retval EFI_SUCCESS               Delete first capsule successfully
  @retval EFI_NOT_FOUND             Fail to found Capsule information

**/
EFI_STATUS
EFIAPI
DeleteFirstCapsule (
  CHAR16  *FileBaseName
  )
{
  EFI_DEVICE_PATH_PROTOCOL         *BootPartitionPath;
  EFI_HANDLE                       Handle;
  EFI_STATUS                       Status;
  EFI_BLOCK_IO_PROTOCOL            *BlockIo;
  EFI_FILE_PROTOCOL                *Root;
  EFI_FILE_HANDLE                  DirHandle;
  BOOLEAN                          NoNextFile = FALSE;
  EFI_FILE_INFO                    *FileInfo;
  BOOLEAN                          Found = FALSE;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *Fs;
  EFI_FILE_HANDLE                  FileHandle;

  Status = GetDevicePathByBoot (mBootDevicePath, &BootPartitionPath);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "GetDevicePathByName failed: %r\n", Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "IGpuDevicePath: %S\n", ConvertDevicePathToText (BootPartitionPath, FALSE, FALSE)));

  Status = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &BootPartitionPath, &Handle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ReadCapsuleInfo: Locate DevicePath failed: %r\n", Status));
    return Status;
  }

  //
  // Get Capsule file
  //
  Status = gBS->HandleProtocol (Handle, &gEfiBlockIoProtocolGuid, (VOID **)&BlockIo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ReadCapsuleInfo: 1Locate DevicePath failed: %r\n", Status));
    return Status;
  }

  if (BlockIo->Media->RemovableMedia) {
    DEBUG ((DEBUG_ERROR, "ReadCapsuleInfo: 2Locate DevicePath failed: %r\n", Status));
    return Status;
  }

  Status = gBS->HandleProtocol (Handle, &gEfiSimpleFileSystemProtocolGuid, (VOID **)&Fs);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ReadCapsuleInfo: 3Locate DevicePath failed: %r\n", Status));
    return Status;
  }

  Status = Fs->OpenVolume (Fs, &Root);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ReadCapsuleInfo: 4Locate DevicePath failed: %r\n", Status));
    return Status;
  }

  Status = Root->Open (Root, &DirHandle, FileBaseName, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Cannot open %s. Status = %r\n", FileBaseName, Status));
    return Status;
  }

  Status = FileHandleFindFirstFile (DirHandle, &FileInfo);
  while (!NoNextFile && !Found) {
    // . & ..
    FileHandleFindNextFile (DirHandle, FileInfo, &NoNextFile);
    if (FileInfo->Attribute & EFI_FILE_DIRECTORY) {
      continue;
    }

    Found = TRUE;
  }

  if (!Found) {
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_INFO, "Deleting Capsule %s\n", FileInfo->FileName));
  Status = DirHandle->Open (DirHandle, &FileHandle, FileInfo->FileName, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
  if (!EFI_ERROR (Status)) {
    Status = FileHandle->Delete (FileHandle);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "Cannot delete Capsule %s:%r\n", FileInfo->FileName, Status));
    }

    DirHandle->Close (DirHandle);
    Root->Close (Root);
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  Detect Capsule file from ESP partition and update capsule.

  @retval EFI_SUCCESS               Opertion is successful.
  @retval EFI_OUT_OF_RESOURCES      No enough buffer to allocate.
  @retval EFI_ERROR                 Internal error when update Capsule.

**/
EFI_STATUS
EFIAPI
CapsuleUpdateViaFileLib (
  VOID
  )
{
  EFI_STATUS          Status;
  UINT8               *CapsuleBuffer;
  UINTN               CapsuleSize;
  EFI_CAPSULE_HEADER  *CapsuleHeader;
  UINTN               CapsuleNum;
  UINTN               CurrentCapsuleFileNo = 0;
  BOOLEAN             NoNextFile           = FALSE;

  CapsuleBuffer = NULL;
  CapsuleSize   = 0;

  DEBUG ((DEBUG_INFO, "CapsuleUpdateViaFileHook() Entry Point...\n"));

  Status = CapsulePathInfo ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "CapsulePathInfo failed: %r\n", Status));
    return Status;
  }

  while (!NoNextFile) {
    Status = IterateAllCapsulesFromDisk (EFI_CAPSULE_FILE_DIRECTORY, (VOID **)&CapsuleBuffer, &CapsuleSize, &NoNextFile);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to update capsule:%r\n", Status));
      break;
    }

    CapsuleHeader = (EFI_CAPSULE_HEADER *)CapsuleBuffer;
    CapsuleNum    = 1;
    Status        = UpdateCapsule (&CapsuleHeader, CapsuleNum);
    RecordCapsuleStatusVariable (CapsuleHeader, Status);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to update capsule.\n"));
      break;
    }

    Status = DeleteFirstCapsule (EFI_CAPSULE_FILE_DIRECTORY);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Cannot delete Capsule.\n"));
      break;
    }
  }

  if (CapsuleBuffer != NULL) {
    FreePool (CapsuleBuffer);
  }

  if (!CurrentCapsuleFileNo && (Status == EFI_NOT_FOUND)) {
    Status = EFI_SUCCESS;
  }

  DEBUG ((DEBUG_INFO, "Capsule update via file completed, Status=%r\n", Status));
  // RecordFmpCapsuleStatus();
  gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);

  // Unreachable.
  return EFI_SUCCESS;
}
