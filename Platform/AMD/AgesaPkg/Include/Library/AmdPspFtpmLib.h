/** @file
  AMD Psp Ftpm Library header file

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PSP_FTPM_LIB_H_
#define PSP_FTPM_LIB_H_
#include <IndustryStandard/Acpi30.h>

/* SYSTEM_TPM_CONFIG_VAL
 *  Value range for APCB_TOKEN_UID_PSP_SYSTEM_TPM_CONFIG
 */
typedef enum {
  SYSTEM_TPM_CONFIG_DTPM     = 0x00,              ///< dTPM
  SYSTEM_TPM_CONFIG_PSP_FTPM = 0x01,              ///< PSP FTPM
  SYSTEM_TPM_CONFIG_HSP_FTPM = 0x02,              ///< HSP FTPM
  SYSTEM_TPM_CONFIG_NONE     = 0xFF,              ///< None of TPM
} SYSTEM_TPM_CONFIG_VAL;

#pragma pack (push, 1)
/// Define TPM_2_CONTROL_AREA
typedef struct {
  // Interface Identifier
  volatile UINT64         InterfaceIdentifier;    ///< Interface Identifier
  // TPM2 Control Area Extension
  volatile UINT32         Clear;                  ///< Clear
  volatile UINT32         RemainingBytes;         ///< RemainingBytes
  volatile UINT32         StatusReserved;         ///< StatusReserved
  volatile UINT32         StatusError;            ///< StatusError
  volatile UINT32         StatusCancel;           ///< StatusCancel
  volatile UINT32         StatusStart;            ///< StatusStart
  UINT64                  InterruptControl;       ///< InterruptControl
  UINT32                  CommandSize;            ///< CommandSize
  EFI_PHYSICAL_ADDRESS    CommandAddress;         ///< CommandAddress
  UINT32                  ResponseSize;           ///< ResponseSize
  EFI_PHYSICAL_ADDRESS    ResponseAddress;        ///< ResponseAddress
  // Memory Absent command/response buffer
  volatile UINT32         CmdRespHWBuffer;        ///< Cmd/Rsp HW Buffer
} TPM2_CONTROL_AREA;

#pragma pack (pop)

/**
  GET TPM related Info

  @param[in,out] FtpmStatus              Used to hold more detail info (Unused Currently)

  @return       EFI_SUCCESS              Ftpm function supported
  @return       EFI_UNSUPPORTED          Ftpm function unsupported

**/
EFI_STATUS
FtpmGetInfo (
  IN OUT UINTN  *FtpmStatus
  );

/**
  Execute a TPM command

  @param[in]    CommandBuffer              Point to the TPM command buffer
  @param[in]    CommandSize                Size of the TPM command buffer
  @param[in, out]    ResponseBuffer             Point to the TPM response buffer
  @param[in, out]    ResponseSize               Size of the TPM response buffer

  @return       EFI_SUCCESS                Command executed successfully
  @return       EFI_UNSUPPORTED            Device unsupported
  @return       EFI_TIMEOUT                Command fail due the time out
  @return       EFI_DEVICE_ERROR           Command fail due the error status set
  @return       EFI_BUFFER_TOO_SMALL       Response buffer too small to hold the response

**/
EFI_STATUS
FtpmExecuteCommand (
  IN     VOID    *CommandBuffer,
  IN     UINT32  CommandSize,
  IN OUT VOID    *ResponseBuffer,
  IN OUT UINT32  *ResponseSize
  );

#endif //PSP_FTPM_LIB_H_
