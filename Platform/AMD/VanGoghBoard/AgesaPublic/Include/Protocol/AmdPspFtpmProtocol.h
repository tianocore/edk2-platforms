/** @file
     AMD Psp Ftpm Protocol Header
  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FTPM_PROTOCOL_H_
#define FTPM_PROTOCOL_H_

#include <Uefi.h>
//
// GUID definition
//
extern EFI_GUID  gAmdPspFtpmProtocolGuid;

typedef struct {
  // C2H_TPM_L0
  UINT64    TPM_L0_Address;         // Mailbox address
  UINT64    TPM_L0_C2H_MSG_Address; // Doorbell address CPU->HSP
  UINT64    TPM_L0_H2C_MSG_Address; // Doorbell address HSP->CPU

  // C2H_HSP_L0(VLT0)
  UINT64    VLT0_Address;           // Mailbox address
  UINT64    VLT0_C2H_MSG_Address;   // Doorbell address CPU->HSP
  UINT64    VLT0_H2C_MSG_Address;   // Doorbell address HSP->CPU

  // C2H_HSP_L1(VLT1)
  UINT64    VLT1_Address;           // Mailbox address
  UINT64    VLT1_C2H_MSG_Address;   // Doorbell address CPU->HSP
  UINT64    VLT1_HSC_MSG_Address;   // Doorbell address HSP->CPU

  // Interrupt Information
  UINT8     Gsi[4];                 // Gsi[0] is for HSP Channel 0 TPM
                                    // Gsi[1] is for HSP Channel 1 VTL0
                                    // Gsi[2] is for HSP Channel 2 VTL1
                                    // Gsi[3] is reserved
} HSP_MAILBOX_ADDRESS, *PHSP_MAILBOX_ADDRESS;

typedef union {
  HSP_MAILBOX_ADDRESS    HSP_info;
} FTPM_INFO;

typedef struct _PSP_FTPM_PROTOCOL PSP_FTPM_PROTOCOL;

/// Define function prototype: Execute a TPM command
typedef
EFI_STATUS
(EFIAPI *FTPM_EXECUTE)(
  IN     PSP_FTPM_PROTOCOL    *This,
  IN     VOID                 *CommandBuffer,
  IN     UINT32                CommandSize,
  IN OUT VOID                 *ResponseBuffer,
  IN OUT UINT32               *ResponseSize
  );

/// Define function prototype: GET TPM related Info
typedef
EFI_STATUS
(EFIAPI *FTPM_CHECK_STATUS)(
  IN     PSP_FTPM_PROTOCOL    *This,
  IN OUT UINTN                *FtpmStatus
  );

/// Define function prototype: Send a TPM command
typedef
EFI_STATUS
(EFIAPI *FTPM_SEND_COMMAND)(
  IN     PSP_FTPM_PROTOCOL    *This,
  IN     VOID                 *CommandBuffer,
  IN     UINT32                CommandSize
  );

/// Define function prototype: Get a TPM command's response
typedef
EFI_STATUS
(EFIAPI *FTPM_GET_RESPONSE)(
  IN     PSP_FTPM_PROTOCOL     *This,
  IN OUT VOID                  *ResponseBuffer,
  IN OUT UINT32                *ResponseSize
  );

/// Define function prototype: Get TCG Logs
typedef
EFI_STATUS
(EFIAPI *FTPM_GET_TCG_LOGS)(
  IN     PSP_FTPM_PROTOCOL     *This,
  IN OUT VOID                  *ResponseBuffer,
  IN OUT UINTN                 *ResponseSize
  );

/// Define function prototype: Get TPM info
typedef
EFI_STATUS
(EFIAPI *FTPM_GET_TPM_INFO)(
  IN     PSP_FTPM_PROTOCOL     *This,
  IN OUT VOID                  *FtpmInfo
  );

/// Defines PSP_FTPM_PROTOCOL. This protocol is used to get Ftpm info
/// Send TPM command, Get TPM command's response, Execute TPM command(Include send & get response)

typedef struct _PSP_FTPM_PROTOCOL {
  FTPM_EXECUTE         Execute;                           ///< Execute TPM command, include send & get response
  FTPM_CHECK_STATUS    CheckStatus;                       ///< Check TPM Status
  FTPM_SEND_COMMAND    SendCommand;                       ///< Send TPM command
  FTPM_GET_RESPONSE    GetResponse;                       ///< Get Last TPM command response
  FTPM_GET_TCG_LOGS    GetTcgLogs;                        ///< Get TCG Logs
  FTPM_GET_TPM_INFO    GetInfo;                           ///< Get TPM info
} PSP_FTPM_PROTOCOL;

#endif //_FTPM_PROTOCOL_H_
