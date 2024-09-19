/** @file

  Copyright (c) 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef NV_PARAM_LIB_COMMON_H_
#define NV_PARAM_LIB_COMMON_H_

#define EFI_MM_MAX_PAYLOAD_SIZE  0x50

#define MM_NVPARAM_FUNC_READ       0x01
#define MM_NVPARAM_FUNC_WRITE      0x02
#define MM_NVPARAM_FUNC_CLEAR      0x03
#define MM_NVPARAM_FUNC_CLEAR_ALL  0x04

#define MM_NVPARAM_RES_SUCCESS  0xAABBCC00
#define MM_NVPARAM_RES_NOT_SET  0xAABBCC01
#define MM_NVPARAM_RES_NO_PERM  0xAABBCC02
#define MM_NVPARAM_RES_FAIL     0xAABBCCFF

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
} EFI_MM_COMMUNICATE_REQUEST;

typedef struct {
  UINT64    Status;
  UINT64    Value;
} EFI_MM_COMMUNICATE_NVPARAM_RESPONSE;

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
NVParamMmCommunicate (
  IN  VOID    *Request,
  IN  UINT32  RequestDataSize,
  OUT VOID    *Response,
  IN  UINT32  ResponseDataSize
  );

#endif /* NV_PARAM_LIB_COMMON_H_ */
