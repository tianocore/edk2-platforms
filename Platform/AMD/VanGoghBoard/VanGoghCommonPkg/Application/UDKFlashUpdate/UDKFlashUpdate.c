/** @file
  Platform Flash Access library.

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include  "UDKFlashUpdate.h"

EFI_SPI_PROTOCOL  *mSpiProtocol = NULL;
UINT32            mFlashAreaBaseAddress;
UINTN             mBiosSize;
UINTN             mBlockSize;

/**
  Input the BeginTimeValue and EndTimeValue, return the spent time(seconds).

  @param[in] BeginTimeValue      The begin time value read by AsmReadTsc().
  @param[in] EndTimeValue        The end time value read by AsmReadTsc().

  @retval  -1                    An error occurred.
  @retval  other                 The seconds value.

**/
STATIC
INT64
EFIAPI
GetSpentTime (
  IN  UINT64  EndTimeValue,
  IN  UINT64  BeginTimeValue
  )
{
  if (EndTimeValue >= BeginTimeValue) {
    return (DivU64x32 (GetTimeInNanoSecond (EndTimeValue - BeginTimeValue), 1000000000));
  } else {
    Print (L"!!!ERROR: Wrong time\n");
    return (-1);
  }
}

/**
  Read 'ReadAddress|NumBytes' of the flash chip, and saved into 'ReadFlash.bin'

  @param[in] ReadAddress    Read address base in flash chip.
  @param[in] NumBytes       Read number of bytes.

  @retval  0                Flash read exited normally.
  @retval  Other            An error occurred.

**/
UINTN
EFIAPI
FlashFdRead (
  IN  UINTN   ReadAddress,
  IN  UINTN   NumBytes,
  IN  CHAR16  *FileName
  )
{
  EFI_STATUS         Status;
  VOID               *Buffer;
  SHELL_FILE_HANDLE  FileHandle;

  Print (L"\nRead flash chip and saved into %s ...\n", FileName);

  Buffer = AllocateZeroPool (NumBytes);
  if (NULL == Buffer) {
    Print (L"!!!ERROR: Allocate pool fail ...\n");
    return (1);
  }

  Status = ShellOpenFileByName (FileName, &FileHandle, EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE, 0);
  if (EFI_ERROR (Status)) {
    Print (L"!!!ERROR: Open file %s %r\n", FileName, Status);
    FreePool (Buffer);
    return (1);
  }

  CopyMem ((UINT8 *)Buffer, (UINT8 *)(ReadAddress + mFlashAreaBaseAddress), NumBytes);
  Status = ShellWriteFile (FileHandle, &NumBytes, Buffer);
  if (EFI_ERROR (Status)) {
    Print (L"!!!ERROR: Write file %s %r\n", FileName, Status);
    FreePool (Buffer);
    ShellCloseFile (&FileHandle);
    return (1);
  }

  FreePool (Buffer);
  ShellCloseFile (&FileHandle);

  return (0);
}

/**
  Erase 'EraseAddress|NumBytes' in flash chip, and skip the block all '0xFF'.

  @param[in] EraseAddress                               Erase address base.
  @param[in] NumBytes                             Erase number of bytes.

  @retval  0                                                          Flash erase exited normally.
  @retval  Other                                                    An error occurred.

**/
UINTN
EFIAPI
FlashFd64KErase (
  IN  UINTN  EraseAddress,
  IN  UINTN  NumBytes
  )
{
  EFI_STATUS  Status;
  // UINTN           Index;
  UINT8  *Buffer;

  Print (L"\nErase flash chip ");

  Buffer = AllocateZeroPool (NumBytes);
  if (NULL == Buffer) {
    Print (L"!!!ERROR: Allocate fail ...\n");
    return (1);
  }

  CopyMem (Buffer, (UINT8 *)(EraseAddress + mFlashAreaBaseAddress), NumBytes);

  for ( ; EraseAddress < NumBytes; EraseAddress += mBlockSize) {
    Status = mSpiProtocol->Execute (
                             mSpiProtocol,
                             SPI_OPCODE_ERASE_INDEX,              // OpcodeIndex
                             0,                                   // PrefixOpcodeIndex
                             FALSE,                               // DataCycle
                             TRUE,                                // Atomic
                             TRUE,                                // ShiftOut
                             EraseAddress,                        // Address
                             0,                                   // Data Number
                             NULL,
                             EnumSpiRegionBios                    // SPI_REGION_TYPE
                             );
    Print (L"Erase address = 0x%x, Erase %r\n", EraseAddress, Status);
    if (EFI_ERROR (Status)) {
      FreePool (Buffer);
      Print (L"!!!ERROR: Erase flash %r\n", Status);
      return (1);
    }
  }

  FreePool (Buffer);
  AsmWbinvd ();

  return (0);
}

/**
  Write 'WriteAddress|NumBytes' in flash chip and skip the block all '0xFF'.

  @param[in] WriteAddress             Write address base in flash chip.
  @param[in] NumBytes                 Write number of bytes.
  @param[in] Buffer                   Point to contents going to write into flash chip.

  @retval  0                          Flash write exited normally.
  @retval  Other                      An error occurred.

**/
UINTN
EFIAPI
FlashFdWrite (
  IN  UINTN  WriteAddress,
  IN  UINTN  NumBytes,
  IN  UINT8  *Buffer
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  Print (L"\nWrite flash chip ");

  for ( ; WriteAddress < NumBytes; WriteAddress += mBlockSize) {
    for (Index = 0; Index < mBlockSize; Index++) {
      if (0xFF != *(Buffer + Index)) {
        Print (L"FlashFdWrite WriteAddress= 0x%x\n", WriteAddress);
        Status = mSpiProtocol->Execute (
                                 mSpiProtocol,
                                 SPI_OPCODE_WRITE_INDEX,    // OpcodeIndex
                                 0,                         // PrefixOpcodeIndex
                                 TRUE,                      // DataCycle
                                 TRUE,                      // Atomic
                                 TRUE,                      // ShiftOut
                                 WriteAddress,              // Address
                                 (UINT32)mBlockSize,        // Data Number
                                 Buffer,
                                 EnumSpiRegionBios
                                 );
        if (EFI_ERROR (Status)) {
          Print (L"!!!ERROR: Write flash %r\n", Status);
          return (1);
        } else {
          Print (L".");
        }

        break;
      }
    }

    Buffer += mBlockSize;
  }

  Print (L"\nWrite flash chip success\n");
  AsmWbinvd ();

  return (0);
}

/**
  Verify the binary in flash chip and the source binary use checksum.

  @param[in] BaseAddress              Write address base in memory.
  @param[in] NumBytes                 Write total number of bytes.
  @param[in] Sourcefile               Point to contents writed into flash chip.

  @retval  0                          Flash verify exited normally.
  @retval  Other                      An error occurred.

**/
UINTN
EFIAPI
FlashFdVerify (
  IN  UINTN  BaseAddress,
  IN  UINTN  NumBytes,
  IN  VOID   *Sourcefile
  )
{
  UINT8   *Buffer;
  UINT32  Index;
  UINT32  ChecksumSourceFile;
  UINT32  ChecksumFlash;

  Print (L"\n");

  ChecksumSourceFile = 0;
  ChecksumFlash      = 0;

  Buffer = AllocateZeroPool (NumBytes);
  if (NULL == Buffer) {
    Print (L"!!!ERROR: Allocate fail ...\n");
    return (1);
  }

  CopyMem (Buffer, (UINT8 *)(BaseAddress + mFlashAreaBaseAddress), NumBytes);
  for (Index = 0; Index < NumBytes; Index++) {
    ChecksumFlash      += *(UINT8 *)(Buffer + Index);
    ChecksumSourceFile += *((UINT8 *)Sourcefile + Index);
  }

  Print (L"Flash checksum: 0x%x, Source File checksum: 0x%x\n", ChecksumFlash, ChecksumSourceFile);

  if (ChecksumSourceFile == ChecksumFlash) {
    Print (L"Verify success\n");
  } else {
    Print (L"!!!ERROR: Verify fail\n");
    FreePool (Buffer);
    return (1);
  }

  FreePool (Buffer);

  return (0);
}

/**
  Initialize.

  @retval  0                          Flash erase exited normally.
  @retval  Other                      An error occurred.

**/
UINTN
EFIAPI
Initialize (
  IN  UINT8  *Index
  )
{
  EFI_STATUS    Status;
  UINT8         FlashIndex;
  UINT8         FlashID[3];
  SPI_INSTANCE  *SpiInstance;

  mSpiProtocol = NULL;

  Status = gBS->LocateProtocol (&gEfiSpiProtocolGuid, NULL, (VOID **)&mSpiProtocol);
  if (EFI_ERROR (Status)) {
    Print (L"!!!ERROR: Locate SpiProtocol %r\n", Status);
    FreePool (mSpiProtocol);
    return (1);
  }

  //
  // attempt to identify flash part and initialize spi table
  //
  for (FlashIndex = 0; FlashIndex < EnumSpiFlashMax; FlashIndex++) {
    Status = mSpiProtocol->Init (
                             mSpiProtocol
                             );
    if (!EFI_ERROR (Status)) {
      //
      // read vendor/device IDs to check if flash device is supported
      //
      Status = mSpiProtocol->Execute (
                               mSpiProtocol,
                               SPI_OPCODE_JEDEC_ID_INDEX,
                               SPI_WREN_INDEX,
                               TRUE,
                               FALSE,
                               FALSE,
                               0,
                               3,
                               FlashID,
                               EnumSpiRegionAll
                               );
      if (EFI_ERROR (Status)) {
        return (1);
      } else {
        if ((FlashID[0] == mSpiInitTable[FlashIndex].VendorId) &&
            (FlashID[1] == mSpiInitTable[FlashIndex].DeviceId0) &&
            (FlashID[2] == mSpiInitTable[FlashIndex].DeviceId1))
        {
          Print (
            L"Supported SPI Flash device found, Vendor Id: 0x%02x, Device ID: 0x%02x%02x\n",
            FlashID[0],
            FlashID[1],
            FlashID[2]
            );
          *Index = FlashIndex;
          break;
        }
      }
    }
  }

  SpiInstance           = SPI_INSTANCE_FROM_SPIPROTOCOL (mSpiProtocol);
  mBiosSize             = SpiInstance->SpiInitTable.BiosSize;
  mFlashAreaBaseAddress = (UINT32)(0x100000000 - mBiosSize);
  mBlockSize            = SpiInstance->SpiInitTable.OpcodeMenu[SPI_OPCODE_ERASE_INDEX].Operation;
  Print (L"BiosSize :0x%x, FlashAreaBaseAddress: 0x%x, Blocksize :0x%x\n", mBiosSize, mFlashAreaBaseAddress, mBlockSize);

  return (0);
}

/**
  Print out help information.

**/
STATIC
VOID
PrintHelpInfo (
  VOID
  )
{
  Print (L"Application to update flash chip. Depends on SpiProtocol.\nSupport flash chip: W25Q64FV/JV, W25Q64FW, MX25U6435F, MX25U12835F.\n\n");
  Print (L"Usage: FLASHUPDATE option [filename]\n\n");
  Print (L"Option:\n");
  Print (L"  -help -h           This help message\n");
  Print (L"  <file>             Specifies the name of the file to write into flash chip\n");
  Print (L"  -v                 Display version information\n");
  Print (L"  -r                 Read flash chip and saved into file\n\n");
  Print (L"Filename:\n");
  Print (L"                     Specifies the name of the file to save the contents read\n                     \
from flash chip, just need when read flash chip.\n\n");
}

/**
  Parse command in shell.

  @param[in] Num           The number of items in Str.
  @param[in] Str           Array of pointers to strings.

  @retval  0               The application exited normally.
  @retval  2               Read flash chip and save into file.
  @retval  Other           An error occurred.

**/
STATIC
UINTN
EFIAPI
ShellCommandParse (
  IN  UINTN   Num,
  IN  CHAR16  **Str
  )
{
  EFI_STATUS  Status;

  if (Num < 2) {
    Print (L"FlashUpdate: Too few argument\n\n");
    PrintHelpInfo ();
    return (1);
  } else if (2 == Num) {
    if (StrLen (Str[1]) == 0) {
      Print (L"FlashUpdate: Too few argument\n\n");
      PrintHelpInfo ();
      return (1);
    }

    if ((Str[1])[0] == L'-') {
      //
      // Parse the arguments.
      //
      if (StrCmp (Str[1], L"-v") == 0) {
        Print (L"FlashUpdate: Version 20230527\n\n");
        return (1);
      }

      if (StrCmp (Str[1], L"-r") == 0) {
        Print (L"FlashUpdate: Too few argument\n\n");
        PrintHelpInfo ();
        return (1);
      }

      if ((StrCmp (Str[1], L"-help") == 0) || (StrCmp (Str[1], L"-h") == 0)) {
        PrintHelpInfo ();
        return (1);
      } else {
        Print (L"FlashUpdate: Illegal option: '%s'\n\n", Str[1]);
        PrintHelpInfo ();
        return (1);
      }
    }

    Status = ShellIsFile (Str[1]);
    if (EFI_ERROR (Status)) {
      Print (L"FlashUpdate: %s is not a file\n\n", Str[1]);
      PrintHelpInfo ();
      return (1);
    }
  } else if (3 == Num) {
    if ((Str[1])[0] == L'-') {
      if (StrCmp (Str[1], L"-r") == 0) {
        Print (L"Read flash chip\n");
        return (2);
      }
    }

    Print (L"FlashUpdate: Illegal argument: '%s %s'\n\n", Str[1], Str[2]);
    PrintHelpInfo ();
    return (1);
  } else if (Num > 3) {
    Print (L"FlashUpdate: Too many argument\n\n");
    PrintHelpInfo ();
    return (1);
  }

  return (0);
}

/**
  UEFI application entry point which has an interface similar to a
  standard C main function.

  The ShellCEntryLib library instance wrappers the actual UEFI application
  entry point and calls this ShellAppMain function.

  @param[in] Argc          The number of items in Argv.
  @param[in] Argv          Array of pointers to strings.

  @retval  0               The application exited normally.
  @retval  Other           An error occurred.

**/
INTN
EFIAPI
ShellAppMain (
  IN  UINTN   Argc,
  IN  CHAR16  **Argv
  )
{
  VOID               *Buffer;
  EFI_STATUS         Status;
  SHELL_FILE_HANDLE  SourceHandle;
  UINTN              SourceFileSize;
  UINTN              BeginTimeValue;
  UINTN              InitTimeValue;
  UINTN              EraseTimeValue;
  UINTN              WriteTimeValue;
  UINTN              VerifyTimeValue;
  UINTN              BaseAddress;
  UINTN              NumBytes;
  UINT32             Index;
  UINT8              FlashIndex;

  BeginTimeValue = AsmReadTsc ();
  SourceHandle   = NULL;
  Buffer         = NULL;

  Status = Initialize (&FlashIndex);
  if (0 != Status) {
    Print (L"!!!ERROR: Initialize fail\n");
    return (1);
  }

  BaseAddress = 0;
  NumBytes    = mBiosSize; // Assign after mBiosSize init in Initialize

  //
  // Parse the command line.
  //
  Status = ShellCommandParse (Argc, Argv);
  if (1 == Status) {
    return (1);
  } else if (2 == Status) {
    Status = FlashFdRead (BaseAddress, NumBytes, Argv[2]);
    if (0 != Status) {
      Print (L"!!!ERROR: Read flash chip fail");
      return (1);
    }

    Print (L"Read flash chip and saved into %s success\n", Argv[2]);
    return (0);
  }

  //
  // open source file
  //
  Status = ShellOpenFileByName (Argv[1], &SourceHandle, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR (Status)) {
    Print (L"!!!ERROR: Open file %s %r\n", Argv[1], Status);
    return (1);
  }

  //
  // get file size of source file
  //
  Status = ShellGetFileSize (SourceHandle, &SourceFileSize);
  if (EFI_ERROR (Status)) {
    Print (L"!!!ERROR: Read file %s size %r\n", Argv[1], Status);
    if (NULL != SourceHandle) {
      ShellCloseFile (&SourceHandle);
    }

    return (1);
  }

  Buffer = AllocateZeroPool (SourceFileSize);
  if (NULL == Buffer) {
    Print (L"!!!ERROR: Allocate pool fail ...\n");
    if (NULL != SourceHandle) {
      ShellCloseFile (&SourceHandle);
    }

    return (1);
  }

  Status = ShellReadFile (SourceHandle, &SourceFileSize, Buffer);
  if (EFI_ERROR (Status)) {
    Print (L"!!!ERROR: Read file %s %r\n", Argv[1], Status);
    if (NULL != SourceHandle) {
      ShellCloseFile (&SourceHandle);
    }

    if (NULL != Buffer) {
      FreePool (Buffer);
    }

    return (1);
  }

  Print (
    L"Supported SPI Flash device found, Vendor Id: 0x%02x, Device ID: 0x%02x%02x\n",
    mSpiInitTable[FlashIndex].VendorId,
    mSpiInitTable[FlashIndex].DeviceId0,
    mSpiInitTable[FlashIndex].DeviceId1
    );

  InitTimeValue = AsmReadTsc ();
  Print (L"Init spent time: %d seconds\n", GetSpentTime (InitTimeValue, BeginTimeValue));

  Print (L"Size of %s: 0x%x bytes, Flash size: 0x%x bytes\n", Argv[1], SourceFileSize, mBiosSize);
  if (mBiosSize != SourceFileSize) {
    Print (L"!!!ERROR: Bios size is not correct\n");
    if (NULL != SourceHandle) {
      ShellCloseFile (&SourceHandle);
    }

    if (NULL != Buffer) {
      FreePool (Buffer);
    }

    return (1);
  }

  Status = FlashFd64KErase (BaseAddress, NumBytes);
  if (0 != Status) {
    Print (L"!!!ERROR: Erase falsh chip fail\n");
    if (NULL != SourceHandle) {
      ShellCloseFile (&SourceHandle);
    }

    if (NULL != Buffer) {
      FreePool (Buffer);
    }

    return (1);
  }

  if (0 != Status) {
    Print (L"!!!ERROR: Erase falsh chip fail\n");
    if (NULL != SourceHandle) {
      ShellCloseFile (&SourceHandle);
    }

    if (NULL != Buffer) {
      FreePool (Buffer);
    }

    return (1);
  }

  EraseTimeValue = AsmReadTsc ();
  Print (L"Erase spent time: %d seconds\n", GetSpentTime (EraseTimeValue, InitTimeValue));

  Status = FlashFdWrite (BaseAddress, NumBytes, Buffer);
  if (0 != Status) {
    Print (L"!!!ERROR: Write falsh chip fail\n");
    if (NULL != SourceHandle) {
      ShellCloseFile (&SourceHandle);
    }

    if (NULL != Buffer) {
      FreePool (Buffer);
    }

    return (1);
  }

  WriteTimeValue = AsmReadTsc ();
  Print (L"\nWrite spent time: %d seconds\n", GetSpentTime (WriteTimeValue, EraseTimeValue));

  Status = FlashFdVerify (BaseAddress, NumBytes, (UINT8 *)Buffer);
  if (0 != Status) {
    Print (L"!!!ERROR: Verify falsh chip fail\n");
    if (NULL != SourceHandle) {
      ShellCloseFile (&SourceHandle);
    }

    if (NULL != Buffer) {
      FreePool (Buffer);
    }

    return (1);
  }

  VerifyTimeValue = AsmReadTsc ();
  Print (L"\nWrite flash chip success!\n");
  Print (L"--------------------------------------------------\n");
  Print (L"Total spent time: %d seconds\n", GetSpentTime (VerifyTimeValue, BeginTimeValue));

  if (NULL != SourceHandle) {
    ShellCloseFile (&SourceHandle);
  }

  if (NULL != Buffer) {
    FreePool (Buffer);
  }

  Print (L"\nReady to restart ");
  for (Index = 0; Index < 4; Index++) {
    MicroSecondDelay (1000000); // delay 1 second
    Print (L".");
  }

  gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);

  return (0);
}
