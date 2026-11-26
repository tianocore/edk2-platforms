/** @file

  Cadence xSPI controller library.

  Copyright (c) 2025-2026 Marvell

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Uefi.h>

#include <IndustryStandard/CadenceXspi.h>

typedef
EFI_STATUS
(*CADENCE_XSPI_BUS_LOCK)(
  IN VOID  *Context
  );

typedef
VOID
(*CADENCE_XSPI_BUS_UNLOCK)(
  IN VOID  *Context
  );

//
// Optional bus arbitration callbacks. Platforms that share the controller with
// other firmware components (e.g. a secure monitor) provide these to serialize
// each flash transaction. Both may be NULL if no arbitration is required.
//
typedef struct {
  CADENCE_XSPI_BUS_LOCK      Lock;
  CADENCE_XSPI_BUS_UNLOCK    Unlock;
  VOID                       *Context;
} CADENCE_XSPI_BUS_OPS;

//
// Number of JEDEC ID bytes returned by CadenceXspiFlashReadId().
//
#define CADENCE_XSPI_FLASH_ID_SIZE  3

//
// Per-controller configuration. All addresses are CPU-visible addresses of
// the mapped controller register block and direct-access window.
//
typedef struct {
  UINTN                   MpiBase;    // Controller register base address.
  UINTN                   DirectBase; // Direct-access (SDMA) window base.
  UINT32                  ChipSelect; // Chip select routed to the device.
  CADENCE_XSPI_BUS_OPS    BusOps;     // Optional bus arbitration callbacks.
} CADENCE_XSPI_CONFIG;

/**
  Initialize the library instance for a Cadence xSPI controller.

  The configuration is copied, the caller may release its copy afterwards.
  Only a single controller is supported at a time.

  @param[in]  Config  Controller configuration. Must not be NULL.

  @retval EFI_SUCCESS            The controller is ready for use.
  @retval EFI_INVALID_PARAMETER  Config is NULL or incomplete.
**/
EFI_STATUS
CadenceXspiInit (
  IN CONST CADENCE_XSPI_CONFIG  *Config
  );

/**
  Execute a single STIG command on the controller.

  @param[in]      OpCode   Instruction opcode.
  @param[in]      Address  Instruction address (up to 32 bits).
  @param[in]      Size     Number of data bytes to transfer.
  @param[in]      Data     TRUE if the command has a data phase.
  @param[in,out]  Buffer   Data buffer for the data phase.
  @param[in]      Op       Instruction phase type.

  @retval EFI_SUCCESS            The command completed successfully.
  @retval EFI_INVALID_PARAMETER  The library has not been initialized.
  @retval EFI_TIMEOUT            The controller did not become idle/ready.
  @retval EFI_DEVICE_ERROR       The controller reported a command failure.
**/
EFI_STATUS
CadenceXspiSendCommand (
  IN     UINT8            OpCode,
  IN     UINT32           Address,
  IN     UINT32           Size,
  IN     BOOLEAN          Data,
  IN OUT VOID             *Buffer,
  IN     CADENCE_XSPI_OP  Op
  );

/**
  Read the JEDEC ID of the attached NOR flash device.

  Reads CADENCE_XSPI_FLASH_ID_SIZE bytes into Id.

  @param[out]  Id  Buffer of at least CADENCE_XSPI_FLASH_ID_SIZE bytes.

  @retval EFI_SUCCESS            The ID was read.
  @retval EFI_INVALID_PARAMETER  Id is NULL or the library is not initialized.
  @retval EFI_ACCESS_DENIED      The bus lock callback rejected the request.
  @retval EFI_DEVICE_ERROR       The read failed.
**/
EFI_STATUS
CadenceXspiFlashReadId (
  OUT UINT8  *Id
  );

/**
  Read data from the attached NOR flash device.

  @param[in]   Address  Flash address.
  @param[in]   Length   Number of bytes to read.
  @param[out]  Buffer   Destination buffer.

  @retval EFI_SUCCESS            The data was read.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL or the library is not initialized.
  @retval EFI_ACCESS_DENIED      The bus lock callback rejected the request.
  @retval EFI_DEVICE_ERROR       The read failed.
**/
EFI_STATUS
CadenceXspiFlashRead (
  IN  UINT32  Address,
  IN  UINTN   Length,
  OUT VOID    *Buffer
  );

/**
  Write data to the attached NOR flash device.

  The buffer is split on page boundaries and each page is programmed and
  polled for completion.

  @param[in]  Address  Flash address.
  @param[in]  Length   Number of bytes to write.
  @param[in]  Buffer   Source buffer.

  @retval EFI_SUCCESS            The data was written.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL or the library is not initialized.
  @retval EFI_ACCESS_DENIED      The bus lock callback rejected the request.
  @retval EFI_DEVICE_ERROR       The write failed.
**/
EFI_STATUS
CadenceXspiFlashWrite (
  IN UINT32  Address,
  IN UINTN   Length,
  IN VOID    *Buffer
  );

/**
  Erase a region of the attached NOR flash device.

  @param[in]  Address  Flash address.
  @param[in]  Length   Number of bytes to erase.

  @retval EFI_SUCCESS            The region was erased.
  @retval EFI_INVALID_PARAMETER  The library is not initialized.
  @retval EFI_ACCESS_DENIED      The bus lock callback rejected the request.
  @retval EFI_DEVICE_ERROR       The erase failed.
**/
EFI_STATUS
CadenceXspiFlashErase (
  IN UINT32  Address,
  IN UINTN   Length
  );

/**
  Convert the controller addresses and bus callbacks to virtual addresses.

  Must be called from the virtual address change event handler if the
  controller is used after ExitBootServices().
**/
VOID
CadenceXspiVirtualNotify (
  VOID
  );
