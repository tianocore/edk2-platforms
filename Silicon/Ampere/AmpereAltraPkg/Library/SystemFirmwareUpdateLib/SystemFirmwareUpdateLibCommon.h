/** @file

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SYSTEM_FIRMWARE_UPDATE_LIB_COMMON_H_
#define SYSTEM_FIRMWARE_UPDATE_LIB_COMMON_H_

#define EFI_MM_MAX_PAYLOAD_U64_E  10
#define EFI_MM_MAX_PAYLOAD_SIZE   (EFI_MM_MAX_PAYLOAD_U64_E * sizeof (UINT64))

#define FWU_MM_RES_SUCCESS             0xAABBCC00
#define FWU_MM_RES_IN_PROGRESS         0xAABBCC01
#define FWU_MM_RES_SECURITY_VIOLATION  0xAABBCC02
#define FWU_MM_RES_OUT_OF_RESOURCES    0xAABBCC03
#define FWU_MM_RES_IO_ERROR            0xAABBCC04
#define FWU_MM_RES_FAIL                0xAABBCCFF

#pragma pack (1)

typedef struct {
  //
  // Allows for disambiguation of the message format.
  //
  EFI_GUID    HeaderGuid;

  //
  // Describes the size of Data (in bytes) and does not include the size of the header.
  //
  UINTN       MessageLength;

  //
  // Designates an array of bytes that is MessageLength in size.
  //
  UINT8       Data[EFI_MM_MAX_PAYLOAD_SIZE];
} EFI_MM_COMMUNICATE_FWU_REQUEST;

typedef struct {
  UINT64    Status;
  UINT64    Progress;
} EFI_MM_COMMUNICATE_FWU_RESPONSE;

#pragma pack ()

/**
  Provides an interface to access the NVParam services via MM interface.

  @param[in]  Request             Pointer to the request buffer
  @param[in]  RequestDataSize     Size of the request buffer.
  @param[out] Response            Pointer to the response buffer
  @param[in]  ResponseDataSize    Size of the response buffer.

  @retval EFI_SUCCESS             Operation succeeded.
  @retval EFI_INVALID_PARAMETER   An invalid data parameter or an invalid
                                  combination of data parameters.
  @retval Others                  An error has occurred.
**/
EFI_STATUS
FirmwareUpdateMmCommunicate (
  IN  VOID    *Request,
  IN  UINT32  RequestDataSize,
  OUT VOID    *Response,
  IN  UINT32  ResponseDataSize
  );

#endif // SYSTEM_FIRMWARE_UPDATE_LIB_COMMON_H_
