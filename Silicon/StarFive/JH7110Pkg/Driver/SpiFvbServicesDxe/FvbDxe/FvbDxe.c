/** @file
 *
 *  Copyright (c) 2023, StarFive Technology Co., Ltd. All rights reserved.<BR>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include "FvbDxe.h"

STATIC FVB_DEVICE  *mFvbDevice;

STATIC CONST FVB_DEVICE  mFvbFlashInstanceTemplate = {
  {
    0,    /* AddrSize ... NEED TO BE FILLED */
    NULL, /* Read from NOR FLASH ... NEED TO BE FILLED */
    0,    /* RegBase ... NEED TO BE FILLED */
    0,    /* AbhBase ... NEED TO BE FILLED */
    0,    /* FifoWidth ... NEED TO BE FILLED */
    0,    /* WriteDelay ... NEED TO BE FILLED */
  }, /* SPI_DEVICE_PARAMS */

  NULL, /* SpiFlashProtocol ... NEED TO BE FILLED */
  NULL, /* SpiMasterProtocol ... NEED TO BE FILLED */
  NULL, /* Handle ... NEED TO BE FILLED */

  FVB_FLASH_SIGNATURE, /* Signature ... NEED TO BE FILLED */

  0,     /* ShadowBufBaseAddr ... NEED TO BE FILLED */
  0,     /* ShadowBufSize ... NEED TO BE FILLED */
  0,     /* FvbFlashVarOffset ... NEED TO BE FILLED */
  0,     /* FvbFlashVarSize ... NEED TO BE FILLED */
  0,     /* BlockSize ... NEED TO BE FILLED */
  0,     /* LastBlock ... NEED TO BE FILLED */
  0,     /* StartLba */

  {
    FvbGetAttributes,       /* GetAttributes */
    FvbSetAttributes,       /* SetAttributes */
    FvbGetPhysicalAddress,  /* GetPhysicalAddress */
    FvbGetBlockSize,        /* GetBlockSize */
    FvbRead,                /* Read */
    FvbWrite,               /* Write */
    FvbEraseBlocks,         /* EraseBlocks */
    NULL,                   /* ParentHandle */
  }, /* EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL */

  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        {
          (UINT8)sizeof (VENDOR_DEVICE_PATH),
          (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
        }
      },
      { 0xfc0cb972, 0x21df, 0x44d2, { 0x92, 0xa5, 0x78, 0x98, 0x99, 0xcb, 0xf6, 0x61 }
      }
    },
    {
      END_DEVICE_PATH_TYPE,
      END_ENTIRE_DEVICE_PATH_SUBTYPE,
      { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 }
    }
  } /* FVB_DEVICE_PATH */
};

STATIC
EFI_STATUS
FvbInitFvAndVariableStoreHeaders (
  IN FVB_DEVICE  *FlashInstance
  )
{
  EFI_FIRMWARE_VOLUME_HEADER  *FirmwareVolumeHeader;
  VARIABLE_STORE_HEADER       *VariableStoreHeader;
  EFI_STATUS                  Status;
  VOID                        *Headers;
  UINTN                       HeadersLength;
  UINTN                       BlockSize;

  HeadersLength = sizeof (EFI_FIRMWARE_VOLUME_HEADER) +
                  sizeof (EFI_FV_BLOCK_MAP_ENTRY) +
                  sizeof (VARIABLE_STORE_HEADER);
  Headers = AllocateZeroPool (HeadersLength);

  BlockSize = FlashInstance->BlockSize;

  /* VariableBase -> FtwWOrkingBase -> FtwSpareBase are declared
   * consecutively in contiguous memory
   */
  ASSERT (
          PcdGet64 (PcdFlashNvStorageVariableBase64) +
          PcdGet32 (PcdFlashNvStorageVariableSize) ==
          PcdGet64 (PcdFlashNvStorageFtwWorkingBase64)
          );
  ASSERT (
          PcdGet64 (PcdFlashNvStorageFtwWorkingBase64) +
          PcdGet32 (PcdFlashNvStorageFtwWorkingSize) ==
          PcdGet64 (PcdFlashNvStorageFtwSpareBase64)
          );

  /* Ensure the size of the variable area is at least one block size */
  ASSERT (
          (PcdGet32 (PcdFlashNvStorageVariableSize) > 0) &&
          (PcdGet32 (PcdFlashNvStorageVariableSize) / BlockSize > 0)
          );
  ASSERT (
          (PcdGet32 (PcdFlashNvStorageFtwWorkingSize) > 0) &&
          (PcdGet32 (PcdFlashNvStorageFtwWorkingSize) / BlockSize > 0)
          );
  ASSERT (
          (PcdGet32 (PcdFlashNvStorageFtwSpareSize) > 0) &&
          (PcdGet32 (PcdFlashNvStorageFtwSpareSize) / BlockSize > 0)
          );

  /* Ensure the Variable areas are aligned on block size boundaries */
  ASSERT ((PcdGet64 (PcdFlashNvStorageVariableBase64) % BlockSize) == 0);
  ASSERT ((PcdGet64 (PcdFlashNvStorageFtwWorkingBase64) % BlockSize) == 0);
  ASSERT ((PcdGet64 (PcdFlashNvStorageFtwSpareBase64) % BlockSize) == 0);

  /* ---------------------------------------------
   * | Firmware Volume Header |                  |
   * --------------------------   Non-Volatile   |
   * | Variable Store Header  | Storage Variable |
   * --------------------------      Region      |
   * |       Variables        |                  |
   * ---------------------------------------------
   */

  /* Prepare Firmware Volume Header */
  FirmwareVolumeHeader = (EFI_FIRMWARE_VOLUME_HEADER *)Headers;
  CopyGuid (&FirmwareVolumeHeader->FileSystemGuid, &gEfiSystemNvDataFvGuid);
  FirmwareVolumeHeader->FvLength   = FlashInstance->FvbFlashVarSize;
  FirmwareVolumeHeader->Signature  = EFI_FVH_SIGNATURE;
  FirmwareVolumeHeader->Attributes = EFI_FVB2_READ_ENABLED_CAP | /* Reads may be enabled */
                                     EFI_FVB2_READ_STATUS |      /* Reads are currently enabled */
                                     EFI_FVB2_STICKY_WRITE |     /* A block erase is required to flip bits into EFI_FVB2_ERASE_POLARITY */
                                     EFI_FVB2_ERASE_POLARITY |   /* After erasure all bits take this value (i.e. '1') */
                                     EFI_FVB2_WRITE_STATUS |     /* Writes are currently enabled */
                                     EFI_FVB2_WRITE_ENABLED_CAP; /* Writes may be enabled */

  FirmwareVolumeHeader->HeaderLength = sizeof (EFI_FIRMWARE_VOLUME_HEADER) +
                                       sizeof (EFI_FV_BLOCK_MAP_ENTRY);
  FirmwareVolumeHeader->Revision              = EFI_FVH_REVISION;
  FirmwareVolumeHeader->BlockMap[0].NumBlocks = FlashInstance->LastBlock + 1;
  FirmwareVolumeHeader->BlockMap[0].Length    = FlashInstance->BlockSize;
  FirmwareVolumeHeader->BlockMap[1].NumBlocks = 0;
  FirmwareVolumeHeader->BlockMap[1].Length    = 0;
  FirmwareVolumeHeader->Checksum              = CalculateCheckSum16 (
                                                                     (UINT16 *)FirmwareVolumeHeader,
                                                                     FirmwareVolumeHeader->HeaderLength
                                                                     );

  /* Prepare Variable Store Header */
  VariableStoreHeader = (VOID *)((UINTN)Headers +
                                 FirmwareVolumeHeader->HeaderLength);
  CopyGuid (&VariableStoreHeader->Signature, &gEfiAuthenticatedVariableGuid);
  VariableStoreHeader->Size = PcdGet32 (PcdFlashNvStorageVariableSize) -
                              FirmwareVolumeHeader->HeaderLength;
  VariableStoreHeader->Format = VARIABLE_STORE_FORMATTED;
  VariableStoreHeader->State  = VARIABLE_STORE_HEALTHY;

  /* Write both header to the flash device on the base address of the
   * declared variable base address in the flash. CAUTIONS! This will
   * replace the existing firmware volume and variable header or possibly
   * cause data corruption. Make sure the declared base address and size
   * in the flash is only use for EFI Variable Storage.
   * Offset = 0, LastBlockAdress = 0;
   */
  Status = FvbWrite (&FlashInstance->FvbProtocol, 0, 0, &HeadersLength, Headers);

  FreePool (Headers);

  return Status;
}

STATIC
EFI_STATUS
FvbValidateFvHeader (
  IN  FVB_DEVICE  *FlashInstance
  )
{
  UINT16                      Checksum;
  EFI_FIRMWARE_VOLUME_HEADER  *FirmwareVolumeHeader;
  VARIABLE_STORE_HEADER       *VariableStoreHeader;
  UINTN                       VariableStoreLength;

  FirmwareVolumeHeader = (EFI_FIRMWARE_VOLUME_HEADER *)FlashInstance->ShadowBufBaseAddr;

  /* Verify the header revision, header signature, length from the flash */
  if ((FirmwareVolumeHeader->Revision  != EFI_FVH_REVISION) ||
      (FirmwareVolumeHeader->Signature != EFI_FVH_SIGNATURE) ||
      (FirmwareVolumeHeader->FvLength  != FlashInstance->FvbFlashVarSize))
  {
    DEBUG (
           (DEBUG_ERROR,
            "%a(): No Firmware Volume header present\n",
            __func__)
           );
    return EFI_NOT_FOUND;
  }

  /* Verify the Firmware Volume Guid from the flash */
  if (!CompareGuid (&FirmwareVolumeHeader->FileSystemGuid, &gEfiSystemNvDataFvGuid)) {
    DEBUG (
           (DEBUG_ERROR,
            "%a(): Firmware Volume Guid non-compatible\n",
            __func__)
           );
    return EFI_NOT_FOUND;
  }

  /* Verify the header checksum from the flash */
  Checksum = CalculateSum16 ((UINT16 *)FirmwareVolumeHeader, FirmwareVolumeHeader->HeaderLength);
  if (Checksum != 0) {
    DEBUG (
           (DEBUG_ERROR,
            "%a(): FV checksum is invalid (Checksum:0x%x)\n",
            __func__,
            Checksum)
           );
    return EFI_NOT_FOUND;
  }

  VariableStoreHeader = (VOID *)((UINTN)FirmwareVolumeHeader + FirmwareVolumeHeader->HeaderLength);

  /* Verify the Variable Store Guid */
  if (!CompareGuid (&VariableStoreHeader->Signature, &gEfiVariableGuid) &&
      !CompareGuid (
                    &VariableStoreHeader->Signature,
                    &gEfiAuthenticatedVariableGuid
                    ))
  {
    DEBUG (
           (DEBUG_ERROR,
            "%a(): Variable Store Guid non-compatible\n",
            __func__)
           );
    return EFI_NOT_FOUND;
  }

  /* Verify the actual Variable Store length with declare size in header*/
  VariableStoreLength = PcdGet32 (PcdFlashNvStorageVariableSize) -
                        FirmwareVolumeHeader->HeaderLength;
  if (VariableStoreHeader->Size != VariableStoreLength) {
    DEBUG (
           (DEBUG_ERROR,
            "%a(): Variable Store Length does not match\n",
            __func__)
           );
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
FvbGetAttributes (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  OUT       EFI_FVB_ATTRIBUTES_2                 *Attributes
  )
{
  EFI_FIRMWARE_VOLUME_HEADER  *FirmwareVolumeHeader;
  EFI_FVB_ATTRIBUTES_2        *FlashFvbAttributes;
  FVB_DEVICE                  *FlashInstance;

  FlashInstance = INSTANCE_FROM_FVB_THIS (This);

  FirmwareVolumeHeader        = (EFI_FIRMWARE_VOLUME_HEADER *)FlashInstance->ShadowBufBaseAddr;
  FlashFvbAttributes = (EFI_FVB_ATTRIBUTES_2 *)&(FirmwareVolumeHeader->Attributes);

  *Attributes = *FlashFvbAttributes;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
FvbSetAttributes (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN OUT    EFI_FVB_ATTRIBUTES_2                 *Attributes
  )
{
  EFI_FVB_ATTRIBUTES_2  OldAttributes;
  EFI_FVB_ATTRIBUTES_2  FlashFvbAttributes;
  EFI_FVB_ATTRIBUTES_2  UnchangedAttributes;
  FVB_DEVICE            *FlashInstance;
  UINT32                Capabilities;
  UINT32                OldStatus;
  UINT32                NewStatus;

  FlashInstance = INSTANCE_FROM_FVB_THIS (This);

  /* Read current attribute from the Frimware Volume Header */
  FvbGetAttributes (This, &FlashFvbAttributes);

  OldAttributes = FlashFvbAttributes;
  Capabilities  = OldAttributes & EFI_FVB2_CAPABILITIES;
  OldStatus     = OldAttributes & EFI_FVB2_STATUS;
  NewStatus     = *Attributes & EFI_FVB2_STATUS;

  UnchangedAttributes = EFI_FVB2_READ_DISABLED_CAP  | \
                        EFI_FVB2_READ_ENABLED_CAP   | \
                        EFI_FVB2_WRITE_DISABLED_CAP | \
                        EFI_FVB2_WRITE_ENABLED_CAP  | \
                        EFI_FVB2_LOCK_CAP           | \
                        EFI_FVB2_STICKY_WRITE       | \
                        EFI_FVB2_ERASE_POLARITY     | \
                        EFI_FVB2_READ_LOCK_CAP      | \
                        EFI_FVB2_WRITE_LOCK_CAP     | \
                        EFI_FVB2_ALIGNMENT;

  /* Some attributes of FV is read only can *not* be set */
  if ((OldAttributes & UnchangedAttributes) ^
      (*Attributes & UnchangedAttributes))
  {
    return EFI_INVALID_PARAMETER;
  }

  /* If firmware volume is locked, no status bit can be updated */
  if (OldAttributes & EFI_FVB2_LOCK_STATUS) {
    if (OldStatus ^ NewStatus) {
      return EFI_ACCESS_DENIED;
    }
  }

  /*  Test read disable */
  if ((Capabilities & EFI_FVB2_READ_DISABLED_CAP) == 0) {
    if ((NewStatus & EFI_FVB2_READ_STATUS) == 0) {
      return EFI_INVALID_PARAMETER;
    }
  }

  /* Test read enable */
  if ((Capabilities & EFI_FVB2_READ_ENABLED_CAP) == 0) {
    if (NewStatus & EFI_FVB2_READ_STATUS) {
      return EFI_INVALID_PARAMETER;
    }
  }

  /* Test write disable */
  if ((Capabilities & EFI_FVB2_WRITE_DISABLED_CAP) == 0) {
    if ((NewStatus & EFI_FVB2_WRITE_STATUS) == 0) {
      return EFI_INVALID_PARAMETER;
    }
  }

  /* Test write enable */
  if ((Capabilities & EFI_FVB2_WRITE_ENABLED_CAP) == 0) {
    if (NewStatus & EFI_FVB2_WRITE_STATUS) {
      return EFI_INVALID_PARAMETER;
    }
  }

  /* Test lock */
  if ((Capabilities & EFI_FVB2_LOCK_CAP) == 0) {
    if (NewStatus & EFI_FVB2_LOCK_STATUS) {
      return EFI_INVALID_PARAMETER;
    }
  }

  FlashFvbAttributes = FlashFvbAttributes & (0xFFFFFFFF & (~EFI_FVB2_STATUS));
  FlashFvbAttributes = FlashFvbAttributes | NewStatus;
  *Attributes        = FlashFvbAttributes;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
FvbGetPhysicalAddress (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  OUT       EFI_PHYSICAL_ADDRESS                 *Address
  )
{
  FVB_DEVICE  *FlashInstance;

  ASSERT (Address != NULL);

  FlashInstance = INSTANCE_FROM_FVB_THIS (This);

  /* Do not support MMIO, return the shadow buffer instead */
  *Address = FlashInstance->ShadowBufBaseAddr;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
FvbGetBlockSize (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN        EFI_LBA                              Lba,
  OUT       UINTN                                *BlockSize,
  OUT       UINTN                                *NumberOfBlocks
  )
{
  FVB_DEVICE  *FlashInstance;

  FlashInstance = INSTANCE_FROM_FVB_THIS (This);

  if (Lba > FlashInstance->LastBlock) {
    DEBUG (
           (DEBUG_ERROR,
            "%a(): Error: Requested LBA %ld is beyond the last available LBA (%ld).\n",
            __func__,
            Lba,
            FlashInstance->LastBlock)
           );
    return EFI_INVALID_PARAMETER;
  } else {
    /* Assume equal sized blocks in all flash devices */
    *BlockSize      = (UINTN)FlashInstance->BlockSize;
    *NumberOfBlocks = (UINTN)(FlashInstance->LastBlock - Lba + 1);

    return EFI_SUCCESS;
  }
}

EFI_STATUS
EFIAPI
FvbRead (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN        EFI_LBA                              Lba,
  IN        UINTN                                Offset,
  IN OUT    UINTN                                *NumBytes,
  IN OUT    UINT8                                *Buffer
  )
{
  FVB_DEVICE  *FlashInstance;
  UINTN       BlockSize;
  UINTN       DataOffset;

  FlashInstance = INSTANCE_FROM_FVB_THIS (This);

  /* Cache the block size to avoid de-referencing pointers all the time */
  BlockSize = FlashInstance->BlockSize;

  /* Offset + byte have to be within the define block size.
   * The read must not span block boundaries. We need to
   * check each variable individually because adding two
   * large values together migght cause overflows.
   */
  if ((Offset               >= BlockSize) ||
      (*NumBytes            >  BlockSize) ||
      ((Offset + *NumBytes) >  BlockSize))
  {
    DEBUG (
           (DEBUG_ERROR,
            "%a(): Wrong buffer size: (Offset=0x%x + NumBytes=0x%x) > BlockSize=0x%x\n",
            __func__,
            Offset,
            *NumBytes,
            BlockSize)
           );
    return EFI_BAD_BUFFER_SIZE;
  }

  /* No bytes to read */
  if (*NumBytes == 0) {
    return EFI_SUCCESS;
  }

  DataOffset = GET_DATA_OFFSET (
                                FlashInstance->ShadowBufBaseAddr + Offset,
                                FlashInstance->StartLba + Lba,
                                FlashInstance->BlockSize
                                );

  /* Copy variable from the shadow buffer */
  CopyMem (Buffer, (UINTN *)DataOffset, *NumBytes);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
FvbWrite (
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  IN        EFI_LBA                              Lba,
  IN        UINTN                                Offset,
  IN OUT    UINTN                                *NumBytes,
  IN        UINT8                                *Buffer
  )
{
  EFI_STATUS  Status;
  FVB_DEVICE  *FlashInstance;
  UINTN       DataOffset;

  FlashInstance = INSTANCE_FROM_FVB_THIS (This);

  DataOffset = GET_DATA_OFFSET (
                                FlashInstance->FvbFlashVarOffset + Offset,
                                FlashInstance->StartLba + Lba,
                                FlashInstance->BlockSize
                                );

  Status = FlashInstance->SpiFlashProtocol->Write (
                                                   &FlashInstance->SpiDevice,
                                                   DataOffset,
                                                   *NumBytes,
                                                   Buffer
                                                   );
  if (EFI_ERROR (Status)) {
    DEBUG (
           (DEBUG_ERROR,
            "%a(): Failed to write to Spi device\n",
            __func__)
           );
    return Status;
  }

  DataOffset = GET_DATA_OFFSET (
                                FlashInstance->ShadowBufBaseAddr + Offset,
                                FlashInstance->StartLba + Lba,
                                FlashInstance->BlockSize
                                );

  /* Update shadow buffer */
  CopyMem ((UINTN *)DataOffset, Buffer, *NumBytes);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
FvbEraseBlocks (
  IN CONST EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *This,
  ...
  )
{
  EFI_FVB_ATTRIBUTES_2  FlashFvbAttributes;
  FVB_DEVICE            *FlashInstance;
  EFI_STATUS            Status;
  VA_LIST               Args;
  UINTN                 BlockAddress;  /* Physical address of Lba to erase */
  EFI_LBA               StartingLba;   /* Lba from which we start erasing */
  UINTN                 NumOfLba;      /* Number of Lba blocks to erase */

  FlashInstance = INSTANCE_FROM_FVB_THIS (This);

  Status = EFI_SUCCESS;

  /* Detect WriteDisabled state */
  FvbGetAttributes (This, &FlashFvbAttributes);
  if ((FlashFvbAttributes & EFI_FVB2_WRITE_STATUS) == 0) {
    DEBUG (
           (DEBUG_ERROR,
            "%a(): Device is in WriteDisabled state.\n",
            __func__)
           );
    return EFI_ACCESS_DENIED;
  }

  /*
   * Before erasing, check the entire list of parameters to ensure
   * all specified blocks are valid.
  */
  VA_START (Args, This);
  do {
    /* Get the Lba from which we start erasing */
    StartingLba = VA_ARG (Args, EFI_LBA);

    /* Have we reached the end of the list? */
    if (StartingLba == EFI_LBA_LIST_TERMINATOR) {
      break;
    }

    /* How many Lba blocks are we requested to erase? */
    NumOfLba = VA_ARG (Args, UINT32);

    /* All blocks must be within range */
    if ((NumOfLba == 0) ||
        ((FlashInstance->StartLba + StartingLba + NumOfLba - 1) >
         FlashInstance->LastBlock))
    {
      DEBUG (
             (DEBUG_ERROR,
              "%a(): Error: Requested LBA are beyond the last available LBA (%ld).\n",
              __func__,
              FlashInstance->LastBlock)
             );

      VA_END (Args);

      return EFI_INVALID_PARAMETER;
    }
  } while (TRUE);

  VA_END (Args);

  /* Start erasing */
  VA_START (Args, This);
  do {
    /* Get the Lba from which we start erasing */
    StartingLba = VA_ARG (Args, EFI_LBA);

    /* Have we reached the end of the list? */
    if (StartingLba == EFI_LBA_LIST_TERMINATOR) {
      break;
    }

    /* How many Lba blocks are we requested to erase? */
    NumOfLba = VA_ARG (Args, UINT32);

    /* Go through each requested block and erase it */
    while (NumOfLba > 0) {
      /* Get the offset address of Lba to erase */
      BlockAddress = GET_DATA_OFFSET (
                                      FlashInstance->FvbFlashVarOffset,
                                      FlashInstance->StartLba + StartingLba,
                                      FlashInstance->BlockSize
                                      );

      /* Erase single block */
      Status = FlashInstance->SpiFlashProtocol->Erase (
                                                       &FlashInstance->SpiDevice,
                                                       BlockAddress,
                                                       FlashInstance->BlockSize
                                                       );
      if (EFI_ERROR (Status)) {
        VA_END (Args);
        return EFI_DEVICE_ERROR;
      }

      StartingLba++;
      NumOfLba--;
    }
  } while (TRUE);

  VA_END (Args);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
FvbFlashProbe (
  IN FVB_DEVICE  *FlashInstance
  )
{
  SPI_FLASH_PROTOCOL  *SpiFlashProtocol;
  EFI_STATUS          Status;

  SpiFlashProtocol = FlashInstance->SpiFlashProtocol;

  /* Read SPI flash ID */
  Status = SpiFlashProtocol->ReadId (&FlashInstance->SpiDevice, TRUE);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  Status = SpiFlashProtocol->Init (SpiFlashProtocol, &FlashInstance->SpiDevice);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a(): Cannot initialize flash device\n", __func__));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
FvbPrepareFvHeader (
  IN FVB_DEVICE  *FlashInstance
  )
{
  EFI_BOOT_MODE  BootMode;
  EFI_STATUS     Status;

  /* Check if it is required to use default environment */
  BootMode = GetBootModeHob ();
  if (BootMode == BOOT_WITH_DEFAULT_SETTINGS) {
    Status = EFI_INVALID_PARAMETER;
  } else {
    /* Validate header at the beginning of FV region */
    Status = FvbValidateFvHeader (FlashInstance);
  }

  /* Install the default FVB header if required */
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a(): The FVB Header is not valid.\n", __func__));
    DEBUG (
           (DEBUG_ERROR,
            "%a(): Installing a correct one for this volume.\n",
            __func__)
           );

    /* Erase entire region that is reserved for variable storage in flash */
    Status = FlashInstance->SpiFlashProtocol->Erase (
                                                     &FlashInstance->SpiDevice,
                                                     FlashInstance->FvbFlashVarOffset,
                                                     FlashInstance->FvbFlashVarSize
                                                     );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    /* Write a new firmware volume and varaible storage headers */
    Status = FvbInitFvAndVariableStoreHeaders (FlashInstance);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
FvbConfigureFlashInstance (
  IN OUT FVB_DEVICE  *FlashInstance
  )
{
  EFI_STATUS  Status;
  UINTN       DataOffset;
  UINTN       VariableSize, FtwWorkingSize, FtwSpareSize, MemorySize;

  Status = gBS->LocateProtocol (
                                &gJH7110SpiFlashProtocolGuid,
                                NULL,
                                (VOID **)&FlashInstance->SpiFlashProtocol
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a(): Cannot locate SpiFlash protocol\n", __func__));
    return Status;
  }

  Status = gBS->LocateProtocol (
                                &gJH7110SpiMasterProtocolGuid,
                                NULL,
                                (VOID **)&FlashInstance->SpiMasterProtocol
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a(): Cannot locate SpiMaster protocol\n", __func__));
    return Status;
  }

  /* Setup and probe SPI flash */
  FlashInstance->SpiMasterProtocol->SetupDevice (
                                                 FlashInstance->SpiMasterProtocol,
                                                 &FlashInstance->SpiDevice
                                                 );
  Status = FvbFlashProbe (FlashInstance);
  if (EFI_ERROR (Status)) {
    DEBUG (
           (DEBUG_ERROR,
            "%a(): Error while performing SPI flash probe\n",
            __func__)
           );
    return Status;
  }

  /* Fill remaining flash description */
  VariableSize   = PcdGet32 (PcdFlashNvStorageVariableSize);
  FtwWorkingSize = PcdGet32 (PcdFlashNvStorageFtwWorkingSize);
  FtwSpareSize   = PcdGet32 (PcdFlashNvStorageFtwSpareSize);

  FlashInstance->FvbFlashVarSize    = VariableSize + FtwWorkingSize + FtwSpareSize;
  FlashInstance->FvbFlashVarOffset  = PcdGet32(PcdJH7110FlashVarOffset);
  FlashInstance->BlockSize = FlashInstance->SpiDevice.Info->SectorSize;
  FlashInstance->LastBlock = (FlashInstance->FvbFlashVarSize /
                                   FlashInstance->BlockSize) - 1;
  FlashInstance->ShadowBufSize = FlashInstance->FvbFlashVarSize;

  /* Allocate memory for shadow buffer */
  MemorySize = EFI_SIZE_TO_PAGES (FlashInstance->FvbFlashVarSize);

  /* FaultTolerantWriteDxe requires memory to be aligned to FtwWorkingSize */
  FlashInstance->ShadowBufBaseAddr = (UINTN)AllocateAlignedRuntimePages (
                                                                          MemorySize,
                                                                          SIZE_64KB
                                                                          );
  if (FlashInstance->ShadowBufBaseAddr == (UINTN)NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  /* Update PCDs value according to allocated memory */
  Status = PcdSet64S (
                      PcdFlashNvStorageVariableBase64,
                      (UINT64)FlashInstance->ShadowBufBaseAddr
                      );
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet64S (
                      PcdFlashNvStorageFtwWorkingBase64,
                      (UINT64)FlashInstance->ShadowBufBaseAddr
                      + VariableSize
                      );
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet64S (
                      PcdFlashNvStorageFtwSpareBase64,
                      (UINT64)FlashInstance->ShadowBufBaseAddr
                      + VariableSize
                      + FtwWorkingSize
                      );
  ASSERT_EFI_ERROR (Status);

  /* ill the shadow buffer with data from flash */
  DataOffset = GET_DATA_OFFSET (
                                FlashInstance->FvbFlashVarOffset,
                                FlashInstance->StartLba,
                                FlashInstance->BlockSize
                                );
  Status = FlashInstance->SpiFlashProtocol->Read (
                                                  &FlashInstance->SpiDevice,
                                                  DataOffset,
                                                  FlashInstance->FvbFlashVarSize,
                                                  (VOID *)FlashInstance->ShadowBufBaseAddr
                                                  );
  if (EFI_ERROR (Status)) {
    goto ErrorFreeAllocatedPages;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                                                   &FlashInstance->Handle,
                                                   &gEfiDevicePathProtocolGuid,
                                                   &FlashInstance->DevicePath,
                                                   &gEfiFirmwareVolumeBlockProtocolGuid,
                                                   &FlashInstance->FvbProtocol,
                                                   NULL
                                                   );
  if (EFI_ERROR (Status)) {
    goto ErrorFreeAllocatedPages;
  }

  Status = FvbPrepareFvHeader (FlashInstance);
  if (EFI_ERROR (Status)) {
    goto ErrorPrepareFvbHeader;
  }

  return EFI_SUCCESS;

ErrorPrepareFvbHeader:
  gBS->UninstallMultipleProtocolInterfaces (
                                            &FlashInstance->Handle,
                                            &gEfiDevicePathProtocolGuid,
                                            &gEfiFirmwareVolumeBlockProtocolGuid,
                                            NULL
                                            );

ErrorFreeAllocatedPages:
    FreeAlignedPages (
                      (VOID *)FlashInstance->ShadowBufBaseAddr,
                      MemorySize
                      );

  return Status;
}

EFI_STATUS
EFIAPI
FvbEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  /* Allocate memory for FVB flash device*/
  mFvbDevice = AllocateRuntimeCopyPool (
                                        sizeof (mFvbFlashInstanceTemplate),
                                        &mFvbFlashInstanceTemplate
                                        );
  if (mFvbDevice == NULL) {
    DEBUG ((DEBUG_ERROR, "%a(): Cannot allocate memory\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  /* Detect and configure flash device */
  Status = FvbConfigureFlashInstance (mFvbDevice);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a(): Fail to configure Fvb SPI device\n", __func__));
    goto ErrorConfigureFlash;
  }

  /* The driver implementing the variable read service can now be dispatched;
   * the varstore headers are in place.
   */
  Status = gBS->InstallProtocolInterface (
                                          &gImageHandle,
                                          &gEdkiiNvVarStoreFormattedGuid,
                                          EFI_NATIVE_INTERFACE,
                                          NULL
                                          );
  if (EFI_ERROR (Status)) {
    DEBUG (
           (DEBUG_ERROR,
            "%a(): Failed to install gEdkiiNvVarStoreFormattedGuid\n",
            __func__)
           );
    goto ErrorInstallNvVarStoreFormatted;
  }

  return Status;

ErrorInstallNvVarStoreFormatted:
  gBS->UninstallMultipleProtocolInterfaces (
                                            &mFvbDevice->Handle,
                                            &gEfiDevicePathProtocolGuid,
                                            &gEfiFirmwareVolumeBlockProtocolGuid,
                                            NULL
                                            );

ErrorConfigureFlash:
  FreePool (mFvbDevice);

  return Status;
}
