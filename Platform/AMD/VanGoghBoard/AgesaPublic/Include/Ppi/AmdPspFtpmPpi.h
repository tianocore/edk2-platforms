/** @file
     AMD Psp Ftpm Ppi Header
  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PSP_FTPM_PPI_H_
#define PSP_FTPM_PPI_H_
#include <Uefi.h>
typedef struct _PSP_FTPM_PPI PSP_FTPM_PPI;

/// Define function prototype: Execute a TPM command
typedef
EFI_STATUS
(EFIAPI *FTPM_EXECUTE)(
  IN     PSP_FTPM_PPI         *This,
  IN     VOID                 *CommandBuffer,
  IN     UINTN                CommandSize,
  IN OUT VOID                 *ResponseBuffer,
  IN OUT UINTN                *ResponseSize
  );

/// Define function prototype: GET TPM related Info
typedef
EFI_STATUS
(EFIAPI *FTPM_CHECK_STATUS)(
  IN     PSP_FTPM_PPI         *This,
  IN OUT UINTN                *FtpmStatus
  );

/// Define function prototype: Send a TPM command
typedef
EFI_STATUS
(EFIAPI *FTPM_SEND_COMMAND)(
  IN     PSP_FTPM_PPI         *This,
  IN     VOID                 *CommandBuffer,
  IN     UINTN                 CommandSize
  );

/// Define function prototype: Get a TPM command's response
typedef
EFI_STATUS
(EFIAPI *FTPM_GET_RESPONSE)(
  IN     PSP_FTPM_PPI          *This,
  IN OUT VOID                  *ResponseBuffer,
  IN OUT UINTN                 *ResponseSize
  );

/// Define function prototype: Get TCG Logs
typedef
EFI_STATUS
(EFIAPI *FTPM_GET_TCG_LOGS)(
  IN     PSP_FTPM_PPI          *This,
  IN OUT VOID                  *ResponseBuffer,
  IN OUT UINTN                 *ResponseSize
  );

//
// PPI prototype
//
// Defines PSP_FTPM_PPI. This PPI is used to get Ftpm info
// Send TPM command, Get TPM command's response, Execute TPM command(Include send & get response)

/// Define PSP_FTPM_PPI
typedef struct _PSP_FTPM_PPI {
  FTPM_EXECUTE         Execute;                           ///< Execute TPM command, include send & get response
  FTPM_CHECK_STATUS    CheckStatus;                       ///< Check TPM Status
  FTPM_SEND_COMMAND    SendCommand;                       ///< Send TPM command
  FTPM_GET_RESPONSE    GetResponse;                       ///< Get Last TPM command response
  FTPM_GET_TCG_LOGS    GetTcgLogs;                        ///< Get TCG Logs
} PSP_FTPM_PPI;

extern EFI_GUID  gAmdPspFtpmPpiGuid;
extern EFI_GUID  gAmdPspFtpmFactoryResetPpiGuid;
typedef struct _PSP_FTPM_FACTORY_RESET_PPI {
  UINT8    Version;        ///< PPI Version
} PSP_FTPM_FACTORY_RESET_PPI;

#endif
