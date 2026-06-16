/** @file
  This library enables Trace32 debugging on device by generating
  symbol loading scripts in DDR memory.

  Copyright (C) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cmm  - TRACE32 CMM practice script
    - Ddr  - Double Data Rate memory
    - Pkg  - Package
    - Hdr  - Header
    - Sym  - Symbol
    - Cmd  - Command
    - Arch - Processor architecture
    - Pdb  - Program Database debug symbol file
**/

#include <PiDxe.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeCoffExtraActionLib.h>
#include <Library/PeCoffLib.h>
#include <Library/PrintLib.h>

#define ADDRESS_FORMAT_STRING  "0x%016lx"

// DDR Buffer Configuration
#define CMM_DDR_BASE       FixedPcdGet64 (PcdTrace32DdrBase)
#define CMM_DDR_SIZE       FixedPcdGet64 (PcdTrace32DdrSize)
#define CMM_MAGIC          0x314D4D43U     /* 'CMM1' */
#define CMM_FLAG_OVERFLOW  BIT0            /* Script buffer overflowed */
#define SCRIPT_INIT_STR    "local &Sf\r\n\r\n"

/* ============================================================
   Memory Layout - Firmware Buffer
   ============================================================
   Region       Offset              Size        Owner / Lifecycle
   --------------------------------------------------------
   Stop Driver  0x0000 - 0x003F     64 bytes    Written by TRACE32 before boot;
                                                read by firmware to identify the
                                                module to stop at; never cleared
                                                by firmware.
   Trap Flag    0x0040 - 0x0041      2 bytes    Written as ASCII "TF" by TRACE32
                                                to make firmware spin in
                                                BreakPointFunction; cleared by
                                                TRACE32 to release the CPU; never
                                                cleared by firmware.
   [padding]    0x0042 - 0x0047      6 bytes    Reserved; 8-byte alignment for Header.
   Header       0x0048 - 0x0057     16 bytes    Zeroed by firmware on first use
                                                (CmmInitIfNeeded); holds CMM
                                                magic, version, write offset,
                                                and overflow flag.
   Script       0x0058 - 0x3757    ~13.7KB      Append-only CMM script buffer;
                                                written by firmware, read by
                                                TRACE32 to load symbols.
   Scratchpad   0x3758 - 0x3B97    1088 bytes   Temporary buffers used by
                                                firmware during script
                                                generation; not read by TRACE32.
   ============================================================ */

// ----- TRACE32-owned Region (written by TRACE32, never cleared by firmware) -----
#define STOP_DRIVER_OFFSET  0x0000
#define STOP_DRIVER_SIZE    64

#define TRAP_FLAG_OFFSET  (STOP_DRIVER_OFFSET + STOP_DRIVER_SIZE)
#define TRAP_FLAG_SIZE    2

#define PADDING_OFFSET  (TRAP_FLAG_OFFSET + TRAP_FLAG_SIZE)
#define PADDING_SIZE    6              /* 8-byte alignment for Header */

// ----- Firmware-initialized Region (zeroed by CmmInitIfNeeded on first use) -----
#define HEADER_OFFSET  (PADDING_OFFSET + PADDING_SIZE)
#define HEADER_SIZE    16

// ----- Script Buffer -----
#define SCRIPT_DATA_OFFSET  (HEADER_OFFSET + HEADER_SIZE)
#define SCRIPT_DATA_SIZE    0x3700          /* ~13.7KB */

// ----- Scratchpad Region (1088 bytes) -----
#define SCRATCH_OFFSET  (SCRIPT_DATA_OFFSET + SCRIPT_DATA_SIZE)
#define SCRATCH_SIZE    0x440               /* 1088 bytes */

#define SCRATCH_CMD_OFFSET  (SCRATCH_OFFSET)
#define SCRATCH_CMD_SIZE    512

#define SCRATCH_PATH_OFFSET  (SCRATCH_CMD_OFFSET + SCRATCH_CMD_SIZE)
#define SCRATCH_PATH_SIZE    512

#define SCRATCH_PKG_OFFSET  (SCRATCH_PATH_OFFSET + SCRATCH_PATH_SIZE)
#define SCRATCH_PKG_SIZE    64

// Trap flag used to hold CPU inside BreakPointFunction until host clears it.
// Stored as ASCII "TF" at DDR[TRAP_FLAG_OFFSET..TRAP_FLAG_OFFSET+1].
// Located immediately after StopDriverName, before the header.

typedef struct {
  UINT32    Magic;        ///< CMM magic number identifying an initialized buffer
  UINT32    Version;      ///< Header version number
  UINT32    WriteOffset;  ///< Current write position in the script buffer
  UINT32    Flags;        ///< Status flags; bit0 = overflow
} CMM_DDR_HEADER;

/**
  Compute a pointer into the DDR buffer at the given byte offset.

  @param[in]  Offset  Byte offset from CMM_DDR_BASE.

  @return  Pointer to the DDR location at the specified offset.
**/
#define DDR_PTR(Offset)  ((VOID *)(CMM_DDR_BASE + (Offset)))

#if defined (__GNUC__)
/// Compiler hint to prevent inlining of the decorated function (GCC).
#define NOINLINE  __attribute__((noinline))
#elif defined (_MSC_VER)
/// Compiler hint to prevent inlining of the decorated function (MSVC).
#define NOINLINE  __declspec(noinline)
#else
/// Compiler hint to prevent inlining of the decorated function (fallback).
#define NOINLINE
#endif

/**
  Dummy function to serve as a breakpoint target for the debugger.
  The debugger sets a breakpoint here to catch when a specific driver loads.
**/
VOID
NOINLINE
BreakPointFunction (
  VOID
  )
{
  volatile CHAR8  *TrapFlag;

  TrapFlag = NULL;
  TrapFlag = (volatile CHAR8 *)DDR_PTR (TRAP_FLAG_OFFSET);

  // Spin while host flag is set to ASCII "TF"
  // Use barriers to ensure the CPU observes host updates on weakly-ordered ARM systems.
  while (TRUE) {
    ArmDataMemoryBarrier ();
    if (!((TrapFlag[0] == 'T') && (TrapFlag[1] == 'F'))) {
      ArmDataMemoryBarrier ();
      break;
    }
  }
}

/**
  Get pointer to CMM DDR header.

  @return  Pointer to the CMM DDR header structure.

**/
STATIC
CMM_DDR_HEADER *
CmmHdr (
  VOID
  )
{
  return (CMM_DDR_HEADER *)(CMM_DDR_BASE + HEADER_OFFSET);
}

/**
  Get pointer to CMM script data buffer.

  @return  Pointer to the script data buffer.

**/
STATIC
CHAR8 *
CmmData (
  VOID
  )
{
  return (CHAR8 *)(CMM_DDR_BASE + SCRIPT_DATA_OFFSET);
}

/**
  Initialize CMM DDR buffer if needed.

  Checks if the CMM buffer is initialized and initializes it if not.

**/
STATIC
VOID
CmmInitIfNeeded (
  VOID
  )
{
  CMM_DDR_HEADER  *Hdr;

  Hdr = CmmHdr ();

  if (Hdr->Magic != CMM_MAGIC) {
    // Initialize from Header onwards.
    // Preserve StopDriverName at offset 0x0.
    ZeroMem (Hdr, CMM_DDR_SIZE - HEADER_OFFSET);

    Hdr->Magic       = CMM_MAGIC;
    Hdr->Version     = 1;
    Hdr->WriteOffset = 0;
    Hdr->Flags       = 0;

    AsciiStrCpyS (CmmData (), SCRIPT_DATA_SIZE, SCRIPT_INIT_STR);
    Hdr->WriteOffset = (UINT32)AsciiStrLen (SCRIPT_INIT_STR);

    // Flush everything from Header onwards
    WriteBackDataCacheRange (Hdr, CMM_DDR_SIZE - HEADER_OFFSET);
  }
}

/**
  Check if the CMM script buffer contains a specific string of known length.

  @param[in]  Str     String to search for.
  @param[in]  StrLen  Length of Str (excluding null terminator).

  @retval TRUE   String found in script.
  @retval FALSE  String not found or StrLen is zero.

**/
STATIC
BOOLEAN
CmmScriptContainsStr (
  IN CONST CHAR8  *Str,
  IN UINTN        StrLen
  )
{
  CMM_DDR_HEADER  *Hdr;
  CHAR8           *Data;
  UINTN           Len;
  UINTN           Index;

  if (StrLen == 0) {
    return FALSE;
  }

  Hdr  = CmmHdr ();
  Data = CmmData ();
  Len  = (UINTN)Hdr->WriteOffset;

  if (StrLen > Len) {
    return FALSE;
  }

  for (Index = 0; Index <= Len - StrLen; Index++) {
    if (CompareMem (Data + Index, Str, StrLen) == 0) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Append data to CMM script buffer.

  @param[in]  Src  Source data to append.
  @param[in]  Len  Length of data to append.

  @retval TRUE   Data appended successfully.
  @retval FALSE  Failed to append (overflow or invalid input).

**/
STATIC
BOOLEAN
CmmAppend (
  IN CONST CHAR8  *Src,
  IN UINTN        Len
  )
{
  CMM_DDR_HEADER  *Hdr;
  CHAR8           *Dst;
  UINTN           Capacity;

  if (Src == NULL) {
    return FALSE;
  }

  if (Len == 0) {
    return TRUE;   // Zero-length append is a no-op success
  }

  Hdr      = CmmHdr ();
  Capacity = SCRIPT_DATA_SIZE;

  if ((Hdr->Flags & CMM_FLAG_OVERFLOW) != 0) {
    return FALSE; // Already overflowed
  }

  if ((Len > Capacity) || ((Hdr->WriteOffset + Len) > Capacity)) {
    Hdr->Flags |= CMM_FLAG_OVERFLOW;
    WriteBackDataCacheRange (Hdr, sizeof (CMM_DDR_HEADER));
    DEBUG ((DEBUG_WARN, "TRACE32: CMM script buffer overflow - symbol loading may be incomplete\n"));
    return FALSE;
  }

  Dst = CmmData () + Hdr->WriteOffset;
  CopyMem (Dst, Src, Len);
  Hdr->WriteOffset += (UINT32)Len;

  WriteBackDataCacheRange (Hdr, sizeof (CMM_DDR_HEADER));
  WriteBackDataCacheRange (Dst, Len);

  return TRUE;
}

/**
  Insert a path translation entry into the CMM script.

  Writes the global variable declarations and y.spath.translate command
  for the given package into the DDR script buffer. Skips if the entry
  is already present (deduplication via CmmContainsN).

  @param[in]  PkgName       Package name.
  @param[in]  PkgRootPath   Package root path (workspace root).
  @param[in]  BuildOutPath  Build output path (relative to workspace root).

**/
STATIC
VOID
InsertTranslationEntry (
  IN CHAR8  *PkgName,
  IN CHAR8  *PkgRootPath,
  IN CHAR8  *BuildOutPath
  )
{
  CHAR8  *TempCmdString;

  TempCmdString = (CHAR8 *)DDR_PTR (SCRATCH_CMD_OFFSET);

  // Check if this package translation is already in the script
  AsciiSPrint (
    TempCmdString,
    SCRATCH_CMD_SIZE,
    "y.spath.translate \"%a\" \"&%a\"",
    PkgRootPath,
    PkgName
    );

  if (CmmScriptContainsStr (TempCmdString, AsciiStrLen (TempCmdString))) {
    return;
  }

  // Write the full translation block
  AsciiSPrint (
    TempCmdString,
    SCRATCH_CMD_SIZE,
    "\r\nglobal &%a\r\n"
    "global &%aBinToSrcRelPath\r\n"
    "if (\"&%aBinToSrcRelPath\"==\"\")\r\n(\r\n  &%aBinToSrcRelPath=\".\"\r\n)\r\n"
    "y.spath.translate \"%a\" \"&%a\"\r\n"
    "global &%aObj\r\n"
    "&%aObj=\"&%a/&%aBinToSrcRelPath/%a\"\r\n",
    PkgName,
    PkgName,
    PkgName,
    PkgName,
    PkgRootPath,
    PkgName,
    PkgName,
    PkgName,
    PkgName,
    PkgName,
    BuildOutPath
    );

  CmmAppend (TempCmdString, AsciiStrLen (TempCmdString));
}

/**
  Get the path separator character used in a path string.

  @param[in]  ImageSymbolFilePath  Path string to analyze.

  @retval '/'   Forward slash path separator found.
  @retval '\\'  Backslash path separator found.
  @retval 0     No path separator found or input is invalid.

**/
STATIC
CHAR8
GetPathSeparator (
  IN CHAR8  *ImageSymbolFilePath
  )
{
  CHAR8  *Ptr;

  if (ImageSymbolFilePath == NULL) {
    return 0;
  }

  for (Ptr = ImageSymbolFilePath; *Ptr != '\0'; Ptr++) {
    if ((*Ptr == '/') || (*Ptr == '\\')) {
      return *Ptr;
    }
  }

  return 0;
}

/**
  Convert ASCII string to lowercase in place.

  Converts all uppercase ASCII characters in the null-terminated string
  pointed to by SrcStr to their lowercase equivalents.

  @param[in, out]  SrcStr  Null-terminated source string to convert.

  @return  Pointer to the converted string (same as SrcStr).

**/
STATIC
CHAR8 *
AsciiToLower (
  IN OUT CHAR8  *SrcStr
  )
{
  CHAR8  Char;
  CHAR8  *TempPtr;

  TempPtr = SrcStr;

  while (*TempPtr != '\0') {
    Char = *TempPtr;
    if ((Char >= 'A') && (Char <= 'Z')) {
      Char    += ('a' - 'A');
      *TempPtr = Char;
    }

    TempPtr++;
  }

  return SrcStr;
}

/**
  Get parent directory name that doesn't contain "obj".

  Scans backwards through the path from DirectoryPtr, skipping any
  directory component whose name contains "obj", and returns the first
  component that does not.

  @param[in]   FullPathPtr           Full path string. Must not be NULL.
  @param[in]   DirectoryPtr          Pointer to current directory position
                                     within FullPathPtr. Must not be NULL and
                                     must be greater than FullPathPtr.
  @param[out]  DirectoryNameSizePtr  Pointer to store directory name size.

  @return  Pointer to directory name within FullPathPtr, or NULL if not found.

**/
STATIC
CHAR8 *
GetNoObjParentDirectory (
  IN  CHAR8  *FullPathPtr,
  IN  CHAR8  *DirectoryPtr,
  OUT UINTN  *DirectoryNameSizePtr
  )
{
  CHAR8  TempDirectoryName[128];
  CHAR8  *Ptr;
  CHAR8  *SegmentEnd;
  UINTN  Size;

  if ((FullPathPtr == NULL) || (DirectoryPtr == NULL) || (DirectoryPtr <= FullPathPtr)) {
    return NULL;
  }

  SegmentEnd = DirectoryPtr;
  Ptr        = DirectoryPtr - 1;

  while (Ptr > FullPathPtr) {
    if ((*Ptr == '/') || (*Ptr == '\\')) {
      Size = (UINTN)(SegmentEnd - Ptr) - 1;

      if ((Size > 0) && (Size < sizeof (TempDirectoryName))) {
        AsciiStrnCpyS (TempDirectoryName, sizeof (TempDirectoryName), Ptr + 1, Size);
        AsciiToLower (TempDirectoryName);

        if (AsciiStrStr (TempDirectoryName, "obj") == NULL) {
          if (DirectoryNameSizePtr != NULL) {
            *DirectoryNameSizePtr = Size;
          }

          return Ptr + 1;
        }
      }

      SegmentEnd = Ptr;
    }

    Ptr--;
  }

  return NULL;
}

/**
  Extract package name and symbol path from image path, and register
  the path translation entry in the CMM script.

  Parses a PDB/ELF symbol file path of the form:
    <WorkspaceRoot>/Build/<PkgName>/<Config>_<Toolchain>/AARCH64/<SymbolPath>
  and returns the package name and the symbol file path relative to the
  architecture directory.

  @param[in]   ImageSymbolFilePath  Full path to symbol file. Must not be NULL.
  @param[out]  SymFilePathPtr       Pointer to store symbol file path within
                                    the architecture directory. May be NULL.

  @return  Package name string in DDR scratch buffer, or NULL if parsing fails.

**/
STATIC
CHAR8 *
GetPkgNameAndSymbolPath (
  IN  CHAR8  *ImageSymbolFilePath,
  OUT CHAR8  **SymFilePathPtr
  )
{
  CHAR8  *PathSep;
  CHAR8  *WorkPath;
  CHAR8  *PkgNameBuf;
  CHAR8  *BuildPtr;
  CHAR8  *ConfigPtr;
  CHAR8  *ArchPtr;
  CHAR8  *SymbolPath;
  CHAR8  *PkgNameStart;
  CHAR8  *FallbackPtr;
  CHAR8  Pattern[32];
  CHAR8  Sep;
  UINTN  PatternLen;
  UINTN  PkgNameLen;
  UINTN  Index;
  UINTN  FallbackSize;

  if (ImageSymbolFilePath == NULL) {
    return NULL;
  }

  // Determine path separator once - use string literal pointer (not stack array)
  Sep = GetPathSeparator (ImageSymbolFilePath);
  if (Sep == '/') {
    PathSep = "/";
  } else if (Sep == '\\') {
    PathSep = "\\";
  } else {
    return NULL;
  }

  WorkPath   = (CHAR8 *)DDR_PTR (SCRATCH_PATH_OFFSET);
  PkgNameBuf = (CHAR8 *)DDR_PTR (SCRATCH_PKG_OFFSET);

  // Copy path into scratch buffer for in-place modification
  AsciiStrCpyS (WorkPath, SCRATCH_PATH_SIZE, ImageSymbolFilePath);

  // Locate /Build/ directory
  AsciiSPrint (Pattern, sizeof (Pattern), "%aBuild%a", PathSep, PathSep);
  BuildPtr = AsciiStrStr (WorkPath, Pattern);
  if (BuildPtr == NULL) {
    return NULL;
  }

  // Locate /DEBUG_ or /RELEASE_ configuration directory
  AsciiSPrint (Pattern, sizeof (Pattern), "%aDEBUG_", PathSep);
  ConfigPtr = AsciiStrStr (WorkPath, Pattern);
  if (ConfigPtr == NULL) {
    AsciiSPrint (Pattern, sizeof (Pattern), "%aRELEASE_", PathSep);
    ConfigPtr = AsciiStrStr (WorkPath, Pattern);
    if (ConfigPtr == NULL) {
      return NULL;
    }
  }

  // Locate /AARCH64/ architecture directory; PatternLen used to skip past it
  PatternLen = AsciiSPrint (Pattern, sizeof (Pattern), "%aAARCH64%a", PathSep, PathSep);
  ArchPtr    = AsciiStrStr (WorkPath, Pattern);
  if (ArchPtr == NULL) {
    return NULL;
  }

  // Symbol file path starts immediately after /AARCH64/
  SymbolPath = ArchPtr + PatternLen;
  // PkgName starts after /Build (skip separator + "Build", point to trailing sep)
  PkgNameStart = BuildPtr + AsciiStrLen ("/Build");

  if (ConfigPtr <= PkgNameStart) {
    return NULL;
  }

  PkgNameLen = (UINTN)(ConfigPtr - PkgNameStart);
  if (PkgNameLen >= SCRATCH_PKG_SIZE) {
    return NULL;
  }

  if (PkgNameLen == 0) {
    // No explicit package directory - fall back to the nearest non-obj parent
    FallbackSize = 0;
    FallbackPtr  = GetNoObjParentDirectory (WorkPath, BuildPtr, &FallbackSize);
    if ((FallbackPtr == NULL) || (FallbackSize == 0)) {
      return NULL;
    }

    PkgNameStart             = FallbackPtr;
    PkgNameLen               = FallbackSize;
    PkgNameStart[PkgNameLen] = 0;
  } else {
    PkgNameStart++;
    PkgNameLen--;
  }

  // Copy package name to DDR buffer, replacing path separators with '_'
  AsciiStrnCpyS (PkgNameBuf, SCRATCH_PKG_SIZE, PkgNameStart, PkgNameLen);
  for (Index = 0; Index < PkgNameLen; Index++) {
    if ((PkgNameBuf[Index] == '/') || (PkgNameBuf[Index] == '\\')) {
      PkgNameBuf[Index] = '_';
    }
  }

  // Split WorkPath at the Build directory to isolate the workspace root
  *BuildPtr = 0;
  BuildPtr++;

  // Remove the trailing separator before the symbol file path.
  // SymbolPath must be > WorkPath; if not, the path is malformed - return NULL.
  if (SymbolPath <= WorkPath) {
    return NULL;
  }

  *(SymbolPath - 1) = 0;

  // Register the path translation entry in the CMM script
  InsertTranslationEntry (PkgNameBuf, WorkPath, BuildPtr);

  if (SymFilePathPtr != NULL) {
    *SymFilePathPtr = SymbolPath;
  }

  return PkgNameBuf;
}

/**
  Process image loading event and update the symbol loading debug script.

  If ImageContext is NULL, then return.

  @param[in]  ImageContext  Pointer to the image context structure that describes the
                            PE/COFF image that has already been loaded and relocated.

**/
VOID
ProcessImageLoad (
  IN PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  CHAR8   *ImagePathPtr;
  CHAR8   *SymFilePath;
  CHAR8   *PkgName;
  CHAR8   *CmdBuf;
  CHAR8   *PathBuf;
  CHAR8   *StopDriverName;
  CHAR8   *StopNameBuf;
  UINT64  LoadAddr;

  if ((ImageContext == NULL) || (ImageContext->PdbPointer == NULL)) {
    return;
  }

  CmmInitIfNeeded ();

  // Clear package name scratch buffer (used as null-terminated string)
  ZeroMem (DDR_PTR (SCRATCH_PKG_OFFSET), SCRATCH_PKG_SIZE);

  CmdBuf         = (CHAR8 *)DDR_PTR (SCRATCH_CMD_OFFSET);
  PathBuf        = (CHAR8 *)DDR_PTR (SCRATCH_PATH_OFFSET);
  LoadAddr       = ImageContext->ImageAddress;
  ImagePathPtr   = ImageContext->PdbPointer;
  StopDriverName = (CHAR8 *)DDR_PTR (STOP_DRIVER_OFFSET);
  SymFilePath    = NULL;

  PkgName = GetPkgNameAndSymbolPath (ImagePathPtr, &SymFilePath);

  if ((PkgName == NULL) || (SymFilePath == NULL)) {
    // Fallback: path parsing failed - load by absolute path
    AsciiStrCpyS (PathBuf, SCRATCH_PATH_SIZE, ImagePathPtr);
    AsciiSPrint (
      CmdBuf,
      SCRATCH_CMD_SIZE,
      "&Sf=\"%a\"\r\n"
      "if (os.file(\"&Sf\"))\r\n"
      "(\r\n"
      "  data.load.elf \"&Sf\" " ADDRESS_FORMAT_STRING " /NoCODE /NoClear\r\n"
                                                       ")\r\n",
      PathBuf,
      LoadAddr
      );
    CmmAppend (CmdBuf, AsciiStrLen (CmdBuf));
    return;
  }

  // Dedup check: skip if this module is already in the script
  AsciiSPrint (
    CmdBuf,
    SCRATCH_CMD_SIZE,
    "data.load.elf \"&Sf\" " ADDRESS_FORMAT_STRING,
    LoadAddr
    );
  if (CmmScriptContainsStr (CmdBuf, AsciiStrLen (CmdBuf))) {
    return;
  }

  // Emit the load command using the package-relative path
  AsciiSPrint (
    CmdBuf,
    SCRATCH_CMD_SIZE,
    "&Sf=\"&%aObj/%a\"\r\n"
    "if (os.file(\"&Sf\"))\r\n"
    "(\r\n"
    "  data.load.elf \"&Sf\" " ADDRESS_FORMAT_STRING " /nocode /noclear\r\n"
                                                     ")\r\n",
    PkgName,
    SymFilePath,
    LoadAddr
    );
  CmmAppend (CmdBuf, AsciiStrLen (CmdBuf));

  // If a stop-driver name is set, check whether this module matches and break.
  // Copy both strings into scratch buffers and lowercase the copies so the
  // comparison is case-insensitive without modifying the originals in DDR.
  if (StopDriverName[0] != 0) {
    // Copy at most STOP_DRIVER_SIZE-1 chars to guarantee null termination even
    // if TRACE32 filled all 64 bytes of StopDriverName without a null terminator.
    StopNameBuf = (CHAR8 *)DDR_PTR (SCRATCH_PKG_OFFSET);
    AsciiStrnCpyS (StopNameBuf, SCRATCH_PKG_SIZE, StopDriverName, STOP_DRIVER_SIZE - 1);
    AsciiToLower (StopNameBuf);

    AsciiStrCpyS (CmdBuf, SCRATCH_CMD_SIZE, ImagePathPtr);
    AsciiToLower (CmdBuf);

    if (AsciiStrStr (CmdBuf, StopNameBuf) != NULL) {
      BreakPointFunction ();
    }
  }
}

/**
  Process image unload event.

  This implementation uses an append-only model and does not remove entries.

  @param[in]  ImageContext  Pointer to the image context structure.

**/
VOID
ProcessImageUnload (
  IN PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  //
  // Append-only model: do not remove entries on unload.
  //
  return;
}

/**
  Performs additional actions after a PE/COFF image has been loaded and relocated.

  If ImageContext is NULL, then ASSERT().

  @param[in]  ImageContext  Pointer to the image context structure that describes the
                            PE/COFF image that has already been loaded and relocated.

**/
VOID
EFIAPI
PeCoffLoaderRelocateImageExtraAction (
  IN PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  if (ImageContext == NULL) {
    return;
  }

  ProcessImageLoad (ImageContext);
}

/**
  Performs additional actions just before a PE/COFF image is unloaded.  Any resources
  that were allocated by PeCoffLoaderRelocateImageExtraAction() must be freed.

  If ImageContext is NULL, then ASSERT().

  @param[in, out]  ImageContext  Pointer to the image context structure that describes the
                                 PE/COFF image that is being unloaded.

**/
VOID
EFIAPI
PeCoffLoaderUnloadImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  if ((ImageContext == NULL) || (ImageContext->ImageSize == 0)) {
    DEBUG ((DEBUG_ERROR, "ERROR: Encountered NULL ImageContext\n"));
    return;
  }

  ProcessImageUnload (ImageContext);
}
