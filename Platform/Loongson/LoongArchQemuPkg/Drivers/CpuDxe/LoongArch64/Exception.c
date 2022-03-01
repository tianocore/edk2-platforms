/** @file

  Copyright (c) 2022 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - ESTAT     - Exception Status
    - ECFG      - Exception Configure
    - ERA       - Exception Return Address
    - BADV      - Bad Virtual Address
    - BADI      - Bad Instructions
    - Epc or EPC or epc   - Exception Program Counter
    - pc or PC or pc      - Program Counter
    - CRMD      - Current Mode
    - PRMD      - Previous Mode
    - CsrEuen      - Cpu Status Register Extern Unit Enable
    - fpu or fp or FP   - Float Point Unit
    - LOONGARCH   - Loongson Arch
    - Irq   - Interrupt ReQuest
**/

#include "Library/Cpu.h"
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include "CpuDxe.h"
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/UefiLib.h>
#include <Guid/DebugImageInfoTable.h>

EFI_EXCEPTION_CALLBACK  gInterruptHandler[MAX_LOONGARCH_INTERRUPT + 1];
EFI_EXCEPTION_CALLBACK  gDebuggerExceptionHandlers[MAX_LOONGARCH_INTERRUPT + 1];

/**
  This function registers and enables the handler specified by InterruptHandler for a processor
  interrupt or exception type specified by InteruptNum. If InterruptHandler is NULL, then the
  handler for the processor interrupt or exception type specified by InteruptNum is uninstalled.
  The installed handler is called once for each processor interrupt or exception.

  @param  InteruptNum    A number of the processor's current interrupt.
  @param  InterruptHandler A pointer to a function of type EFI_CPU_INTERRUPT_HANDLER that is called
                           when a processor interrupt occurs. If this parameter is NULL, then the handler
                           will be uninstalled.

  @retval EFI_SUCCESS           The handler for the processor interrupt was successfully installed or uninstalled.
  @retval EFI_ALREADY_STARTED   InterruptHandler is not NULL, and a handler for InteruptNum was
                                previously installed.
  @retval EFI_INVALID_PARAMETER InterruptHandler is NULL, and a handler for InteruptNum was not
                                previously installed.
  @retval EFI_UNSUPPORTED       The interrupt specified by InteruptNum is not supported.
**/
EFI_STATUS
RegisterInterruptHandler (
  IN EFI_EXCEPTION_TYPE             InteruptNum,
  IN EFI_CPU_INTERRUPT_HANDLER      InterruptHandler
  )
{
  if (InteruptNum > MAX_LOONGARCH_INTERRUPT) {
    return EFI_UNSUPPORTED;
  }

  if ((InterruptHandler != NULL)
    && (gInterruptHandler[InteruptNum] != NULL))
  {
    return EFI_ALREADY_STARTED;
  }

  gInterruptHandler[InteruptNum] = InterruptHandler;

  return EFI_SUCCESS;
}

/**
  This function calls the corresponding exception handler based on the exception type.

  @param  SystemContext  The system context at the time of the exception.

  @retval VOID
**/
STATIC VOID
EFIAPI
CommonInterruptHandler (
  IN OUT EFI_SYSTEM_CONTEXT           SystemContext
  )
{
  INT32 Pending;
  INT32 InterruptNum;
  /*Interrupt [13-0] NMI IPI TI PCOV hw IP10-IP2 soft IP1-IP0*/
  Pending = ((SystemContext.SystemContextLoongArch64->ESTAT) &
             (SystemContext.SystemContextLoongArch64->ECFG) & 0x1fff);
  for (InterruptNum = 0; InterruptNum < MAX_LOONGARCH_INTERRUPT; InterruptNum++) {
    if (Pending & (1 << InterruptNum)) {
      if (gInterruptHandler[InterruptNum] != NULL) {
        gInterruptHandler[InterruptNum] (InterruptNum, SystemContext);
      } else {
        DEBUG ((DEBUG_INFO, "Pending: 0x%0x, InterruptNum: 0x%0x\n", Pending, InterruptNum));
      }
    }
  }
}

/**
  Use the EFI Debug Image Table to lookup the FaultAddress and find which PE/COFF image
  it came from. As long as the PE/COFF image contains a debug directory entry a
  string can be returned. For ELF and Mach-O images the string points to the Mach-O or ELF
  image. Microsoft tools contain a pointer to the PDB file that contains the debug information.

  @param  FaultAddress         Address to find PE/COFF image for.
  @param  ImageBase            Return load address of found image
  @param  PeCoffSizeOfHeaders  Return the size of the PE/COFF header for the image that was found

  @retval NULL                 FaultAddress not in a loaded PE/COFF image.
  @retval                      Path and file name of PE/COFF image.
**/
CHAR8 *
GetImageName (
  IN  UINTN  FaultAddress,
  OUT UINTN  *ImageBase,
  OUT UINTN  *PeCoffSizeOfHeaders
  )
{
  EFI_STATUS                          Status;
  EFI_DEBUG_IMAGE_INFO_TABLE_HEADER   *DebugTableHeader;
  EFI_DEBUG_IMAGE_INFO                *DebugTable;
  UINTN                               Entry;
  CHAR8                               *Address;

  Status = EfiGetSystemConfigurationTable (&gEfiDebugImageInfoTableGuid, (VOID **)&DebugTableHeader);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  DebugTable = DebugTableHeader->EfiDebugImageInfoTable;
  if (DebugTable == NULL) {
    return NULL;
  }

  Address = (CHAR8 *)(UINTN)FaultAddress;
  for (Entry = 0; Entry < DebugTableHeader->TableSize; Entry++, DebugTable++) {
    if (DebugTable->NormalImage != NULL) {
      if ((DebugTable->NormalImage->ImageInfoType == EFI_DEBUG_IMAGE_INFO_TYPE_NORMAL) &&
          (DebugTable->NormalImage->LoadedImageProtocolInstance != NULL)) {
        if ((Address >= (CHAR8 *)DebugTable->NormalImage->LoadedImageProtocolInstance->ImageBase) &&
            (Address <= ((CHAR8 *)DebugTable->NormalImage->LoadedImageProtocolInstance->ImageBase + DebugTable->NormalImage->LoadedImageProtocolInstance->ImageSize))) {
          *ImageBase = (UINTN)DebugTable->NormalImage->LoadedImageProtocolInstance->ImageBase;
          *PeCoffSizeOfHeaders = PeCoffGetSizeOfHeaders ((VOID *)(UINTN)*ImageBase);
          return PeCoffLoaderGetPdbPointer (DebugTable->NormalImage->LoadedImageProtocolInstance->ImageBase);
        }
      }
    }
  }
  return NULL;
}

/**
  pass a file name string that contains the path, return file name.

  @param  FullName   Path and file name

  @retval            file name.
**/
STATIC
CONST CHAR8 *
BaseName (
  IN  CONST CHAR8 *FullName
  )
{
  CONST CHAR8 *Str;

  Str = FullName + AsciiStrLen (FullName);

  while (--Str > FullName) {
    if (*Str == '/' || *Str == '\\') {
      return Str + 1;
    }
  }
  return Str;
}

/** Default Exception Handler Function
  This function is called when an exception occurs that cannot be handled,
  and this function prints the system context information when the interrupt occurred

  @param  SystemContext  The system context at the time of the exception.

  @retval            VOID.
**/
STATIC
VOID
EFIAPI
DefaultHandler (
  IN OUT EFI_SYSTEM_CONTEXT           SystemContext
  )
{
  CHAR8  *ImageName;
  UINTN  ImageBase;
  UINTN  Epc;
  UINTN  PeCoffSizeOfHeader;

  DEBUG ((DEBUG_ERROR, "CRMD   0x%llx\n",  SystemContext.SystemContextLoongArch64->CRMD));
  DEBUG ((DEBUG_ERROR, "PRMD   0x%llx\n",  SystemContext.SystemContextLoongArch64->PRMD));
  DEBUG ((DEBUG_ERROR, "ECFG  0x%llx\n",  SystemContext.SystemContextLoongArch64->ECFG));
  DEBUG ((DEBUG_ERROR, "ESTAT   0x%llx\n",  SystemContext.SystemContextLoongArch64->ESTAT));
  DEBUG ((DEBUG_ERROR, "ERA    0x%llx\n",  SystemContext.SystemContextLoongArch64->ERA));
  DEBUG ((DEBUG_ERROR, "BADV    0x%llx\n",  SystemContext.SystemContextLoongArch64->BADV));
  DEBUG ((DEBUG_ERROR, "BADI 0x%llx\n",  SystemContext.SystemContextLoongArch64->BADI));

  Epc = SystemContext.SystemContextLoongArch64->ERA;
  ImageName = GetImageName (Epc, &ImageBase, &PeCoffSizeOfHeader);
  if (ImageName != NULL) {
    DEBUG ((DEBUG_ERROR, "PC 0x%012lx (0x%012lx+0x%08x) [ 0] %a\n",
           Epc, ImageBase,
           Epc - ImageBase, BaseName (ImageName)));
  } else {
    DEBUG ((DEBUG_ERROR, "PC 0x%012lx\n", Epc));
  }

  while (1);
}

/** Common exception entry
  Exception handling is the entry point for the C environment,
  This function does different things depending on the exception type.

  @param  SystemContext  The system context at the time of the exception.

  @retval            VOID.
**/
VOID
EFIAPI
CommonExceptionEntry (
  IN OUT EFI_SYSTEM_CONTEXT           SystemContext
  )
{
  INT32    ExceptionType;
  UINT64   CsrEuen;
  UINT64   FpuStatus;

  ExceptionType = SystemContext.SystemContextLoongArch64->ESTAT & CSR_ESTAT_EXC;
  ExceptionType = ExceptionType >> CSR_ESTAT_EXC_SHIFT;

  LoongArchReadqCsrEuen (&CsrEuen);
  FpuStatus = CsrEuen & CSR_EUEN_FPEN;
  switch (ExceptionType) {
    case EXC_INT:
      /*
       * handle interrupt exception
       */
      CommonInterruptHandler (SystemContext);
      if (!FpuStatus) {
        LoongArchReadqCsrEuen (&CsrEuen);
        if (CsrEuen & CSR_EUEN_FPEN) {
          /*
           * Since Hw FP is enabled during interrupt handler,
           * disable FP
           */
           CsrEuen &= ~CSR_EUEN_FPEN;
           LoongArchWriteqCsrEuen (CsrEuen);
        }
      }
      break;
    case EXC_FPDIS:
      /*
       * Hardware FP disabled exception,
       * Enable and init FP registers here
       */
      LoongArchEnableFpu ();
      InitFpu(FPU_CSR_RN);
      break;
    default:
      DefaultHandler(SystemContext);
      break;
  }
}

/** Exception module initialization
  This function sets the exception base address.

  @param  Cpu   A pointer to the CPU architecture protocol structure.

  @retval EFI_SUCCESS           Initialization succeeded
  @retval EFI_NOT_FOUND          Could not Found resources.
  @retval EFI_OUT_OF_RESOURCES   No enough resources.
**/
EFI_STATUS
InitializeExceptions (
  IN EFI_CPU_ARCH_PROTOCOL    *Cpu
  )
{
  EFI_STATUS           Status;
  BOOLEAN              IrqEnabled;
  EFI_PHYSICAL_ADDRESS Address;

  ZeroMem (gInterruptHandler, sizeof (*gInterruptHandler));

  //
  // Disable interrupts
  //
  Cpu->GetInterruptState (Cpu, &IrqEnabled);
  Cpu->DisableInterrupt (Cpu);

  //
  // EFI does not use the FIQ, but a debugger might so we must disable
  // as we take over the exception vectors.
  //
  Status = gBS->AllocatePages (
                 AllocateAnyPages,
                 EfiRuntimeServicesData,
                 1,
                 &Address
                 );
  if (EFI_ERROR (Status)) {
         return Status;
  }

  DEBUG ((DEBUG_INFO, "Set Exception Base Address\n"));
  CopyMem ((char *)Address, LoongArchException, (LoongArchExceptionEnd - LoongArchException));
  InvalidateInstructionCacheRange ((char *)Address, (LoongArchExceptionEnd - LoongArchException));

  SetEbase (Address);
  DEBUG ((DEBUG_INFO, "LoongArchException address: 0x%p\n", Address));
  DEBUG ((DEBUG_INFO, "LoongArchExceptionEnd address: 0x%p\n", Address + (LoongArchExceptionEnd - LoongArchException)));

  DEBUG ((DEBUG_INFO, "InitializeExceptions, IrqEnabled = %x\n", IrqEnabled));
  if (IrqEnabled) {
    //
    // Restore interrupt state
    //
    Status = Cpu->EnableInterrupt (Cpu);
  }
  return Status;
}
