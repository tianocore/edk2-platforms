/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/ArmStdSmc.h>
#include <Library/ArmLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MmCommunicationLib.h>
#include <Library/PcdLib.h>
#include <Protocol/MmCommunication2.h>

//
// Address, Length of the pre-allocated buffer for communication with the secure
// world.
//
STATIC ARM_MEMORY_REGION_DESCRIPTOR  mNsCommBuffMemRegion;

EFI_STATUS
EFIAPI
MmCommunicationLibConstructor (
  VOID
  )
{
  mNsCommBuffMemRegion.PhysicalBase = PcdGet64 (PcdMmBufferBase);
  // During UEFI boot, virtual and physical address are the same
  mNsCommBuffMemRegion.VirtualBase = mNsCommBuffMemRegion.PhysicalBase;
  mNsCommBuffMemRegion.Length      = PcdGet64 (PcdMmBufferSize);

  return EFI_SUCCESS;
}

/**
  Communicates with a registered handler.

  This function provides an interface to send and receive messages to the
  Standalone MM environment in UEFI PEI phase.

  @param[in, out] CommBuffer          A pointer to the buffer to convey
                                      into MMRAM.
  @param[in, out] CommSize            The size of the data buffer being
                                      passed in. This is optional.

  @retval EFI_SUCCESS                 The message was successfully posted.
  @retval EFI_INVALID_PARAMETER       The CommBuffer was NULL.
  @retval EFI_BAD_BUFFER_SIZE         The buffer size is incorrect for the MM
                                      implementation. If this error is
                                      returned, the MessageLength field in
                                      the CommBuffer header or the integer
                                      pointed by CommSize are updated to reflect
                                      the maximum payload size the
                                      implementation can accommodate.
  @retval EFI_ACCESS_DENIED           The CommunicateBuffer parameter
                                      or CommSize parameter, if not omitted,
                                      are in address range that cannot be
                                      accessed by the MM environment
**/
EFI_STATUS
EFIAPI
MmCommunicationCommunicate (
  IN OUT VOID   *CommBuffer,
  IN OUT UINTN  *CommSize OPTIONAL
  )
{
  EFI_MM_COMMUNICATE_HEADER  *CommunicateHeader;
  ARM_SMC_ARGS               CommunicateSmcArgs;
  EFI_STATUS                 Status;
  UINTN                      BufferSize;

  Status     = EFI_ACCESS_DENIED;
  BufferSize = 0;

  ZeroMem (&CommunicateSmcArgs, sizeof (ARM_SMC_ARGS));

  //
  // Check parameters
  //
  if (CommBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CommunicateHeader = CommBuffer;
  // CommBuffer is a mandatory parameter. Hence, Rely on
  // MessageLength + Header to ascertain the
  // total size of the communication payload rather than
  // rely on optional CommSize parameter
  BufferSize = CommunicateHeader->MessageLength +
               sizeof (CommunicateHeader->HeaderGuid) +
               sizeof (CommunicateHeader->MessageLength);

  // If the length of the CommBuffer is 0 then return the expected length.
  if (CommSize != NULL) {
    // This case can be used by the consumer of this driver to find out the
    // max size that can be used for allocating CommBuffer.
    if ((*CommSize == 0) ||
        (*CommSize > mNsCommBuffMemRegion.Length))
    {
      *CommSize = mNsCommBuffMemRegion.Length;
      return EFI_BAD_BUFFER_SIZE;
    }

    //
    // CommSize must match MessageLength + sizeof (EFI_MM_COMMUNICATE_HEADER);
    //
    if (*CommSize != BufferSize) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // If the buffer size is 0 or greater than what can be tolerated by the MM
  // environment then return the expected size.
  //
  if ((BufferSize == 0) ||
      (BufferSize > mNsCommBuffMemRegion.Length))
  {
    CommunicateHeader->MessageLength = mNsCommBuffMemRegion.Length -
                                       sizeof (CommunicateHeader->HeaderGuid) -
                                       sizeof (CommunicateHeader->MessageLength);
    return EFI_BAD_BUFFER_SIZE;
  }

  // SMC Function ID
  CommunicateSmcArgs.Arg0 = ARM_SMC_ID_MM_COMMUNICATE_AARCH64;

  // Cookie
  CommunicateSmcArgs.Arg1 = 0;

  // Copy Communication Payload
  CopyMem ((VOID *)mNsCommBuffMemRegion.VirtualBase, CommBuffer, BufferSize);

  // comm_buffer_address (64-bit physical address)
  CommunicateSmcArgs.Arg2 = (UINTN)mNsCommBuffMemRegion.PhysicalBase;

  // comm_size_address (not used, indicated by setting to zero)
  CommunicateSmcArgs.Arg3 = 0;

  // Call the Standalone MM environment.
  ArmCallSmc (&CommunicateSmcArgs);

  switch (CommunicateSmcArgs.Arg0) {
    case ARM_SMC_MM_RET_SUCCESS:
      ZeroMem (CommBuffer, BufferSize);
      // On successful return, the size of data being returned is inferred from
      // MessageLength + Header.
      CommunicateHeader = (EFI_MM_COMMUNICATE_HEADER *)mNsCommBuffMemRegion.VirtualBase;
      BufferSize        = CommunicateHeader->MessageLength +
                          sizeof (CommunicateHeader->HeaderGuid) +
                          sizeof (CommunicateHeader->MessageLength);

      CopyMem (
        CommBuffer,
        (VOID *)mNsCommBuffMemRegion.VirtualBase,
        BufferSize
        );
      Status = EFI_SUCCESS;
      break;

    case ARM_SMC_MM_RET_INVALID_PARAMS:
      Status = EFI_INVALID_PARAMETER;
      break;

    case ARM_SMC_MM_RET_DENIED:
      Status = EFI_ACCESS_DENIED;
      break;

    case ARM_SMC_MM_RET_NO_MEMORY:
      // Unexpected error since the CommSize was checked for zero length
      // prior to issuing the SMC
      Status = EFI_OUT_OF_RESOURCES;
      ASSERT (0);
      break;

    default:
      Status = EFI_ACCESS_DENIED;
      ASSERT (0);
  }

  return Status;
}
