/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MM_COMMUNICATION_LIB_H_
#define MM_COMMUNICATION_LIB_H_

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
  );

#endif /* MM_COMMUNICATION_LIB_H_ */
