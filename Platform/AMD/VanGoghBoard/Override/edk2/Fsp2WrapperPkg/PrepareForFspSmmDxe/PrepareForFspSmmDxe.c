/** @file
  Implements PrepareForFspSmmDxe.c

  Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <FspsUpd.h>
#include <MultiPhaseSiPhases.h>
#include <FspSmmDataExchangeBuffer.h>
#include <Pi/PiHob.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>
#include <Library/FspWrapperApiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/FspWrapperApiLib.h>
#include <Protocol/MmCommunication2.h>

#ifdef TPM_ENABLE
#define TOTAL_DEPENDENCY_COUNT  2
#else
#define TOTAL_DEPENDENCY_COUNT  1// No TCG2.
#endif

STATIC FSPS_UPD *volatile                      FspsUpd;
STATIC FSP_SMM_DATA_EXCHANGE_BUFFER *volatile  ExchangeBuffer;
STATIC volatile UINTN                          DependencyCount = 0;

extern EFI_GUID  gFspsUpdDataPointerAddressGuid;
extern EFI_GUID  gExchangeBufferUpdateNotifyGuid;
extern EFI_GUID  gFspSmmDependencyReadyProtocolGuid;

STATIC
EFI_STATUS
CallFspAfterSmmConditionsMet (
  VOID
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;
  EFI_HANDLE  Handle = NULL;

  gST->BootServices->InstallProtocolInterface (
                       &Handle,
                       &gFspSmmDependencyReadyProtocolGuid,
                       EFI_NATIVE_INTERFACE,
                       NULL
                       );
  // }
  return Status;
}

VOID
EFIAPI
OnRequiredProtocolReady (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  VOID  *Interface;

  gBS->CloseEvent (Event);
  gBS->LocateProtocol (Context, NULL, &Interface);
  DEBUG ((DEBUG_INFO, "%a:located %g at %011p\n", __FILE__, Context, Interface));
  if (CompareGuid (Context, &gEfiGlobalNvsAreaProtocolGuid)) {
    ExchangeBuffer->NvsAreaProtocol = Interface;
    DEBUG ((DEBUG_INFO, "%a:gEfiGlobalNvsAreaProtocolGuid\n", __FILE__));
    DependencyCount++;
    goto check_dependencies_count;
  }

  if (CompareGuid (Context, &gEfiTcg2ProtocolGuid)) {
    ExchangeBuffer->EfiTcg2Protocol = Interface;
    DEBUG ((DEBUG_INFO, "%a:gEfiTcg2ProtocolGuid\n", __FILE__));
    DependencyCount++;
    goto check_dependencies_count;
  }

check_dependencies_count:
  if (DependencyCount == TOTAL_DEPENDENCY_COUNT) {
    DEBUG ((DEBUG_INFO, "All Dependencies are ready!\n"));
    CallFspAfterSmmConditionsMet ();
  }
}

EFI_STATUS
EFIAPI
PrepareForFSPSmmDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  VOID        *FspsUpdHob = GetFirstGuidHob (&gFspsUpdDataPointerAddressGuid);
  VOID        *Registration;

  if ( FspsUpdHob != NULL ) {
    FspsUpd        = ((FSPS_UPD *)(UINTN)(*(UINT32 *)GET_GUID_HOB_DATA (FspsUpdHob)));
    ExchangeBuffer = AllocateZeroPool (sizeof (FSP_SMM_DATA_EXCHANGE_BUFFER));
    if ( ExchangeBuffer == NULL ) {
      DEBUG ((DEBUG_ERROR, "Cannot Allocate memory for SMM data exchange!\n"));
      return EFI_ABORTED;
    }

    FspsUpd->FspsConfig.smm_data_buffer_address = (UINT64)(UINTN)ExchangeBuffer;
    DEBUG ((DEBUG_ERROR, "Exchange Buffer is at %011p\n", ExchangeBuffer));
    // Create callbacks to acquire protocol base address.
    Status = gBS->LocateProtocol (&gEfiGlobalNvsAreaProtocolGuid, NULL, &(ExchangeBuffer->NvsAreaProtocol));
    if (EFI_ERROR (Status)) {
      EfiNamedEventListen (
        &gEfiGlobalNvsAreaProtocolGuid,
        TPL_NOTIFY,
        OnRequiredProtocolReady,
        &gEfiGlobalNvsAreaProtocolGuid,
        &Registration
        );
    } else {
      DEBUG ((DEBUG_INFO, "%a:gEfiGlobalNvsAreaProtocolGuid is installed already\n", __FILE__));
      DependencyCount++;
    }

    Status = gBS->LocateProtocol (&gEfiTcg2ProtocolGuid, NULL, &(ExchangeBuffer->EfiTcg2Protocol));
    if (EFI_ERROR (Status)) {
      EfiNamedEventListen (
        &gEfiTcg2ProtocolGuid,
        TPL_NOTIFY,
        OnRequiredProtocolReady,
        &gEfiTcg2ProtocolGuid,
        &Registration
        );
    } else {
      DEBUG ((DEBUG_INFO, "%a:gEfiTcg2ProtocolGuid is installed already\n", __FILE__));
      DependencyCount++;
    }

    if (DependencyCount == 5) {
      DEBUG ((DEBUG_INFO, "All Dependencies are ready!\n"));
      CallFspAfterSmmConditionsMet ();
    }
  } else {
    DEBUG ((DEBUG_ERROR, "Cannot locate FSP-S UPD!\n"));
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}
