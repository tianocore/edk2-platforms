/** @file
  AMD Psp Ftpm Ppi Header

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PSP_FTPM_PPI_H_
#define PSP_FTPM_PPI_H_
#include <Uefi.h>

typedef struct _PSP_FTPM_PPI PSP_FTPM_PPI;

/**
  Define function prototype: Execute a TPM command

  @param[in]       This                       Point to PSP_FTPM_PPI itself
  @param[in]       CommandBuffer              Point to the TPM command buffer
  @param[in]       CommandSize                Size of the TPM command buffer
  @param[in,out]   ResponseBuffer             Point to the TPM response buffer
  @param[in,out]   ResponseSize               Size of the TPM response buffer

  @return          EFI_SUCCESS                Command executed successfully
  @return          EFI_UNSUPPORTED            Device unsupported
  @return          EFI_TIMEOUT                Command fail due the time out
  @return          EFI_DEVICE_ERROR           Command fail due the error status set
  @return          EFI_BUFFER_TOO_SMALL       Response buffer too small to hold the response
**/
typedef
EFI_STATUS
(EFIAPI *FTPM_EXECUTE)(
  IN     PSP_FTPM_PPI         *This,
  IN     VOID                 *CommandBuffer,
  IN     UINTN                CommandSize,
  IN OUT VOID                 *ResponseBuffer,
  IN OUT UINTN                *ResponseSize
  );

/**
  Define function prototype: GET TPM related Info

  @param[in]       This                     Point to PSP_FTPM_PPI itself
  @param[in,out]   FtpmStatus               Used to hold more detail info (Unused Currently)

  @return          EFI_SUCCESS              Ftpm function supported
  @return          EFI_UNSUPPORTED          Ftpm function unsupported
**/
typedef
EFI_STATUS
(EFIAPI *FTPM_CHECK_STATUS)(
  IN     PSP_FTPM_PPI         *This,
  IN OUT UINTN                *FtpmStatus
  );

/**
  Define function prototype: Send a TPM command

  @param[in]    This                       Point to PSP_FTPM_PPI itself
  @param[in]    CommandBuffer              Point to the TPM command buffer
  @param[in]    CommandSize                Size of the TPM command buffer

  @return       EFI_SUCCESS                Command executed successfully
  @return       EFI_UNSUPPORTED            Device unsupported
  @return       EFI_TIMEOUT                Command fail due the time out
  @return       EFI_DEVICE_ERROR           Command fail due the error status set
  @return       EFI_BUFFER_TOO_SMALL       Response buffer too small to hold the response
**/
typedef
EFI_STATUS
(EFIAPI *FTPM_SEND_COMMAND)(
  IN     PSP_FTPM_PPI         *This,
  IN     VOID                 *CommandBuffer,
  IN     UINTN                 CommandSize
  );

/**
  Define function prototype: Get a TPM command's response

  @param[in]       This                       Point to PSP_FTPM_PPI itself
  @param[in,out]   ResponseBuffer             Point to the TPM response buffer
  @param[in,out]   ResponseSize               Size of the TPM response buffer

  @return          EFI_SUCCESS                Command executed successfully
  @return          EFI_UNSUPPORTED            Device unsupported
  @return          EFI_TIMEOUT                Command fail due the time out
  @return          EFI_DEVICE_ERROR           Command fail due the error status set
  @return          EFI_BUFFER_TOO_SMALL       Response buffer too small to hold the response
 **/
typedef
EFI_STATUS
(EFIAPI *FTPM_GET_RESPONSE)(
  IN     PSP_FTPM_PPI          *This,
  IN OUT VOID                  *ResponseBuffer,
  IN OUT UINTN                 *ResponseSize
  );

/**
  Define function prototype: Get TCG Logs.

  This function only implemented on Pluton-fTPM

  @param[in]     This                       Point to PSP_FTPM_PPI itself
  @param[in,out] ResponseBuffer             Point to the TPM response buffer
  @param[in,out] ResponseSize               Size of the TPM response buffer

  @retval EFI_STATUS  0: Success, Non-Zero Error
**/
typedef
EFI_STATUS
(EFIAPI *FTPM_GET_TCG_LOGS)(
  IN     PSP_FTPM_PPI          *This,
  IN OUT VOID                  *ResponseBuffer,
  IN OUT UINTN                 *ResponseSize
  );

/**
  PSP_FTPM_PPI prototype

  Defines PSP_FTPM_PPI. This PPI is used to get Ftpm info.
  Send TPM command, Get TPM command's response, Execute TPM command(Include send & get response)
**/
typedef struct _PSP_FTPM_PPI {
  FTPM_EXECUTE         Execute;                           ///< Execute TPM command, include send & get response
  FTPM_CHECK_STATUS    CheckStatus;                       ///< Check TPM Status
  FTPM_SEND_COMMAND    SendCommand;                       ///< Send TPM command
  FTPM_GET_RESPONSE    GetResponse;                       ///< Get Last TPM command response
  FTPM_GET_TCG_LOGS    GetTcgLogs;                        ///< Get TCG Logs
} PSP_FTPM_PPI;

extern EFI_GUID  gAmdPspFtpmPpiGuid;
extern EFI_GUID  gAmdPspFtpmFactoryResetPpiGuid;

#endif
