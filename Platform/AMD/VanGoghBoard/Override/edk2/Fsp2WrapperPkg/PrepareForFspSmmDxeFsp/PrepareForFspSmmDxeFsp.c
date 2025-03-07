/** @file
  Implements PrepareForFspSmmDxeFsp.c

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <FspsUpd.h>
#include <FspSmmDataExchangeBuffer.h>
#include <Pi/PiHob.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SmmServicesTableLib.h>

extern EFI_GUID  gExchangeBufferUpdateNotifyGuid;

STATIC FSPS_UPD *volatile                      FspsUpd;
STATIC FSP_SMM_DATA_EXCHANGE_BUFFER *volatile  ExchangeBuffer;
STATIC EFI_HANDLE                              ExchangeBufferHandle;

STATIC EFI_GUID  *MonitoredGuids[] = {
  &gEfiGlobalNvsAreaProtocolGuid,
  &gEfiTcg2ProtocolGuid
};
STATIC BOOLEAN   ProtocolInstalled[sizeof (MonitoredGuids)/sizeof (VOID *)];

extern EFI_GUID  gFspsUpdDataPointerAddressGuid;

EFI_STATUS
EFIAPI
DetectAndInstallNewProtocol (
  VOID
  )
{
  if ((ExchangeBuffer->NvsAreaProtocol != 0) && (ProtocolInstalled[0] == FALSE)) {
    VOID  *Handle = NULL;
    gBS->InstallMultipleProtocolInterfaces (
           &Handle,
           &gEfiGlobalNvsAreaProtocolGuid,
           ExchangeBuffer->NvsAreaProtocol,
           NULL
           );
    ProtocolInstalled[0] = TRUE;
  }

  if ((ExchangeBuffer->EfiTcg2Protocol != 0) && (ProtocolInstalled[4] == FALSE)) {
    VOID  *Handle = NULL;
    gBS->InstallMultipleProtocolInterfaces (
           &Handle,
           &gEfiTcg2ProtocolGuid,
           ExchangeBuffer->EfiTcg2Protocol,
           NULL
           );
    ProtocolInstalled[1] = TRUE;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PrepareForFSPSmmDxeFspEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  VOID  *FspsUpdHob = GetFirstGuidHob (&gFspsUpdDataPointerAddressGuid);

  if ( FspsUpdHob != NULL ) {
    FspsUpd        = ((FSPS_UPD *)(UINTN)(*(UINT32 *)GET_GUID_HOB_DATA (FspsUpdHob)));
    ExchangeBuffer = (FSP_SMM_DATA_EXCHANGE_BUFFER *)(UINTN)FspsUpd->FspsConfig.smm_data_buffer_address;
    DEBUG ((DEBUG_ERROR, "Exchange Buffer is at %011p\n", ExchangeBuffer));
    DetectAndInstallNewProtocol ();
  } else {
    DEBUG ((DEBUG_ERROR, "Cannot locate FSP-S UPD!\n"));
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}
