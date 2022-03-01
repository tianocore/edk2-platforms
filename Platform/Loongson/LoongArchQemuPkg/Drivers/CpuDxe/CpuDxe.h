/** @file
  CPU DXE Module to produce CPU ARCH Protocol and CPU MP Protocol

  Copyright (c) 2022 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef CPU_DXE_H_
#define CPU_DXE_H_

#include <Protocol/Cpu.h>

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
  );

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
RegisterDebuggerInterruptHandler (
  IN EFI_EXCEPTION_TYPE             InteruptNum,
  IN EFI_CPU_INTERRUPT_HANDLER      InterruptHandler
  );

/**
  This function modifies the attributes for the memory region specified by BaseAddress and
  Length from their current attributes to the attributes specified by Attributes.

  @param  This             The EFI_CPU_ARCH_PROTOCOL instance.
  @param  BaseAddress      The physical address that is the start address of a memory region.
  @param  Length           The size in bytes of the memory region.
  @param  Attributes       The bit mask of attributes to set for the memory region.

  @retval EFI_SUCCESS           The attributes were set for the memory region.
  @retval EFI_ACCESS_DENIED     The attributes for the memory resource range specified by
                                BaseAddress and Length cannot be modified.
  @retval EFI_INVALID_PARAMETER Length is zero.
  @retval EFI_OUT_OF_RESOURCES  There are not enough system resources to modify the attributes of
                                the memory resource range.
  @retval EFI_UNSUPPORTED       The processor does not support one or more bytes of the memory
                                resource range specified by BaseAddress and Length.
                                The bit mask of attributes is not support for the memory resource
                                range specified by BaseAddress and Length.
**/
EFI_STATUS
EFIAPI
CpuSetMemoryAttributes (
  IN EFI_CPU_ARCH_PROTOCOL     *This,
  IN EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN UINT64                    Length,
  IN UINT64                    Attributes
  );

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
  );

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
  );

extern CHAR8 LoongArchException[], LoongArchExceptionEnd[];
/** Set Exception Base Address

  @param  addr Exception Base Address.

  @retval      The Old Exception Base Address.
**/
extern
UINT64
SetEbase (
  EFI_PHYSICAL_ADDRESS addr
  );
/*
  Load the FPU with signalling NANS.  This bit pattern we're using has
  the property that no matter whether considered as single or as double
  precision represents signaling NANS.

  @param  fcsr  The value to initialize FCSR0

  @retval      The Old Exception Base Address.
 */
extern
VOID
InitFpu (
  UINT32 fcsr
  );

/*
  Read Csr EUEN register.

  @param  CsrEuen Pointer to the variable used to store the EUEN register value

  @retval  none
 */
extern
VOID
LoongArchReadqCsrEuen (
  UINT64   *CsrEuen
  );

/*
  Write Csr EUEN register.

  @param   The value used to write to the EUEN register

  @retval  none
 */
extern
VOID
LoongArchWriteqCsrEuen (
  UINT64   CsrEuen
  );

/*
  Enables  floating-point unit

  @param  VOID

  @retval  VOID
 */
extern
VOID
LoongArchEnableFpu (
  VOID
  );

/*
  Disable  floating-point unit

  @param  VOID

  @retval  VOID
 */
extern
VOID
LoongArchDisableFpu (
  VOID
  );

#endif // CPU_DXE_H_
