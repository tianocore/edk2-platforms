/** @file
  Discover TPM resources and install gOvmfTpmDiscoveredPpiGuid.

  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>

#include <Guid/RiscVSecHobData.h>

#include <Library/DebugLib.h>
#include <Library/FdtLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/PeiServicesLib.h>

CONST EFI_PEI_PPI_DESCRIPTOR  mTpm2DiscoveredPpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gOvmfTpmDiscoveredPpiGuid,
  NULL
};

/**
  Discover and set up TPM2 MMIO resources from FDT.

  @param   FdtBase         Fdt base address
  @retval  EFI_SUCCESS     TPM2 MMIO device found and resources set up
  @retval  EFI_NOT_FOUND   No TPM2 MMIO device found in FDT

**/
STATIC
EFI_STATUS
DiscoverTPM2MmioDevice (
  VOID  *FdtBase
  )
{
  INT32         Node, Prev;
  INT32         Parent, Depth;
  CONST CHAR8   *Compatible;
  CONST CHAR8   *CompItem;
  INT32         Len;
  INT32         RangesLen;
  CONST UINT8   *RegProp;
  CONST UINT32  *RangesProp;
  UINT64        TpmBase;
  UINT64        TpmBaseSize;

  //
  // Empty TpmBase and TpmBaseSize indicates no TPM found.
  //
  TpmBase     = 0;
  TpmBaseSize = 0;

  //
  // Set Parent to suppress incorrect compiler/analyzer warnings.
  //
  Parent = 0;

  for (Prev = Depth = 0; ; Prev = Node) {
    Node = FdtNextNode (FdtBase, Prev, &Depth);
    if (Node < 0) {
      break;
    }

    if (Depth == 1) {
      Parent = Node;
    }

    Compatible = FdtGetProp (FdtBase, Node, "compatible", &Len);

    //
    // Iterate over the NULL-separated items in the compatible string
    //
    for (CompItem = Compatible; CompItem != NULL && CompItem < Compatible + Len;
         CompItem += 1 + AsciiStrLen (CompItem))
    {
      if (AsciiStrCmp (CompItem, "tcg,tpm-tis-mmio") == 0) {
        RegProp = FdtGetProp (FdtBase, Node, "reg", &Len);
        ASSERT (Len == 8 || Len == 16);
        if (Len == 8) {
          TpmBase     = Fdt32ToCpu (*(UINT32 *)RegProp);
          TpmBaseSize = Fdt32ToCpu (*(UINT32 *)((UINT8 *)RegProp + 4));
        } else if (Len == 16) {
          TpmBase     = Fdt64ToCpu (ReadUnaligned64 ((UINT64 *)RegProp));
          TpmBaseSize = Fdt64ToCpu (ReadUnaligned64 ((UINT64 *)((UINT8 *)RegProp + 8)));
        }

        if (Depth > 1) {
          //
          // QEMU may put the TPM on the platform bus, in which case
          // we have to take its 'ranges' property into account to translate the
          // MMIO address. This consists of a <child base, parent base, size>
          // tuple, where the child base and the size use the same number of
          // cells as the 'reg' property above, and the parent base uses 2 cells
          //
          RangesProp = FdtGetProp (FdtBase, Parent, "ranges", &RangesLen);
          ASSERT (RangesProp != NULL);

          //
          // a plain 'ranges' attribute without a value implies a 1:1 mapping
          //
          if (RangesLen != 0) {
            //
            // assume a single translated range with 2 cells for the parent base
            //
            if (RangesLen != Len + 2 * sizeof (UINT32)) {
              DEBUG ((
                DEBUG_WARN,
                "%a: 'ranges' property has unexpected size %d\n",
                __func__,
                RangesLen
                ));
              TpmBaseSize = 0;
              break;
            }

            if (Len == 8) {
              TpmBase -= Fdt32ToCpu (RangesProp[0]);
            } else {
              TpmBase -= Fdt64ToCpu (ReadUnaligned64 ((UINT64 *)RangesProp));
            }

            //
            // advance RangesProp to the parent bus address
            //
            RangesProp = (UINT32 *)((UINT8 *)RangesProp + Len / 2);
            TpmBase   += Fdt64ToCpu (ReadUnaligned64 ((UINT64 *)RangesProp));
          }
        }

        break;
      }
    }
  }

  if (TpmBaseSize == 0) {
    DEBUG ((DEBUG_ERROR, "TPM2 MMIO device not found in FDT\n"));
    return EFI_NOT_FOUND;
  }

  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_MAPPED_IO,
    EFI_RESOURCE_ATTRIBUTE_PRESENT     |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
    EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_TESTED,
    TpmBase,
    ALIGN_VALUE (TpmBaseSize, EFI_PAGE_SIZE)
    );

  ASSERT_EFI_ERROR ((EFI_STATUS)PcdSet64S (PcdTpmBaseAddress, TpmBase));
  PeiServicesInstallPpi (&mTpm2DiscoveredPpi);

  return EFI_SUCCESS;
}

/**
  The entry point for TPM discovery driver.

  @param[in]  FileHandle   Handle of the file being invoked.
  @param[in]  PeiServices  Describes the list of possible PEI Services.

  @retval  EFI_ABORTED     No need to keep this PEIM resident
**/
EFI_STATUS
EFIAPI
TpmDiscoverPeimEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS              Status;
  VOID                    *Hob;
  RISCV_SEC_HANDOFF_DATA  *SecData;
  EFI_GUID                SecHobDataGuid = RISCV_SEC_HANDOFF_HOB_GUID;

  DEBUG ((DEBUG_LOAD | DEBUG_INFO, "TPM Discovery PEIM is Loaded\n"));

  Hob = GetFirstGuidHob (&SecHobDataGuid);
  if (Hob == NULL) {
    DEBUG ((DEBUG_ERROR, "TPM Discovery PEIM: SecHobDataGuid not found\n"));
    ASSERT (FALSE);
    return EFI_NOT_FOUND;
  }

  SecData = GET_GUID_HOB_DATA (Hob);

  Status = DiscoverTPM2MmioDevice (SecData->FdtPointer);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}
