/** @file
*  High memory node enumeration DXE driver for SbsaQemu
*  Virtual Machines
*
*  Copyright (c) 2023, Linaro Ltd. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/HardwareInfoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/Cpu.h>

EFI_STATUS
EFIAPI
InitializeHighMemDxe (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_CPU_ARCH_PROTOCOL            *Cpu;
  EFI_STATUS                       Status;
  UINT32                           NumMemNodes;
  UINT32                           Index;
  UINT64                           CurBase;
  UINT64                           CurSize;
  UINT64                           Attributes;
  MemoryInfo                       MemInfo;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  GcdDescriptor;

  Status = gBS->LocateProtocol (
                  &gEfiCpuArchProtocolGuid,
                  NULL,
                  (VOID **)&Cpu
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Check for memory node and add the memory spaces except the lowest one
  //
  NumMemNodes = GetMemNodeCount ();
  for (Index = 0; Index < NumMemNodes; Index++) {
    GetMemInfo (Index, &MemInfo);
    CurBase = MemInfo.AddressBase;
    CurSize = MemInfo.AddressSize;

    if (CurBase > PcdGet64 (PcdSystemMemoryBase)) {
      Status = gDS->GetMemorySpaceDescriptor (CurBase, &GcdDescriptor);
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_WARN,
          "%a: Region 0x%lx - 0x%lx not found in the GCD memory space map\n",
          __func__,
          CurBase,
          CurBase + CurSize - 1
          ));
        continue;
      }

      if (GcdDescriptor.GcdMemoryType == EfiGcdMemoryTypeNonExistent) {
        Status = gDS->AddMemorySpace (
                        EfiGcdMemoryTypeSystemMemory,
                        CurBase,
                        CurSize,
                        EFI_MEMORY_WB
                        );

        if (EFI_ERROR (Status)) {
          DEBUG ((
            DEBUG_ERROR,
            "%a: Failed to add System RAM @ 0x%lx - 0x%lx (%r)\n",
            __func__,
            CurBase,
            CurBase + CurSize - 1,
            Status
            ));
          continue;
        }

        Status = gDS->SetMemorySpaceAttributes (
                        CurBase,
                        CurSize,
                        EFI_MEMORY_WB
                        );
        if (EFI_ERROR (Status)) {
          DEBUG ((
            DEBUG_WARN,
            "%a: gDS->SetMemorySpaceAttributes() failed on region 0x%lx - 0x%lx (%r)\n",
            __func__,
            CurBase,
            CurBase + CurSize - 1,
            Status
            ));
        }

        Attributes = EFI_MEMORY_WB;
        if ((PcdGet64 (PcdDxeNxMemoryProtectionPolicy) &
             (1U << (UINT32)EfiConventionalMemory)) != 0)
        {
          Attributes |= EFI_MEMORY_XP;
        }

        Status = Cpu->SetMemoryAttributes (Cpu, CurBase, CurSize, Attributes);

        if (EFI_ERROR (Status)) {
          DEBUG ((
            DEBUG_ERROR,
            "%a: Failed to set System RAM @ 0x%lx - 0x%lx attribute (%r)\n",
            __func__,
            CurBase,
            CurBase + CurSize - 1,
            Status
            ));
        } else {
          DEBUG ((
            DEBUG_INFO,
            "%a: Add System RAM @ 0x%lx - 0x%lx\n",
            __func__,
            CurBase,
            CurBase + CurSize - 1
            ));
        }
      }
    }
  }

  return EFI_SUCCESS;
}
