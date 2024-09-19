/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FLASH_LIB_COMMON_H_
#define FLASH_LIB_COMMON_H_

#define EFI_MM_MAX_TMP_BUF_SIZE  0x1000
#define EFI_MM_MAX_PAYLOAD_SIZE  0x50

#define MM_SPINOR_FUNC_GET_INFO           0x00
#define MM_SPINOR_FUNC_READ               0x01
#define MM_SPINOR_FUNC_WRITE              0x02
#define MM_SPINOR_FUNC_ERASE              0x03
#define MM_SPINOR_FUNC_GET_NVRAM_INFO     0x04
#define MM_SPINOR_FUNC_GET_NVRAM2_INFO    0x05
#define MM_SPINOR_FUNC_GET_FAILSAFE_INFO  0x06

#define MM_SPINOR_RES_SUCCESS  0xAABBCC00
#define MM_SPINOR_RES_FAIL     0xAABBCCFF

#pragma pack(1)

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
  UINT64    DeviceBase;
  UINT64    PageSize;
  UINT64    SectorSize;
  UINT64    DeviceSize;
} EFI_MM_COMMUNICATE_SPINOR_RESPONSE;

typedef struct {
  UINT64    Status;
  UINT64    FailSafeBase;
  UINT64    FailSafeSize;
} EFI_MM_COMMUNICATE_FAILSAFE_INFO_RESPONSE;

typedef struct {
  UINT64    Status;
  UINT64    NvRamBase;
  UINT64    NvRamSize;
} EFI_MM_COMMUNICATE_NVRAM_INFO_RESPONSE;

#pragma pack()

extern BOOLEAN  gFlashLibRuntime;
extern UINT8    *gFlashLibPhysicalBuffer;
extern UINT8    *gFlashLibVirtualBuffer;

/**
  Provides an interface to access the Flash services via MM interface.

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
FlashMmCommunicate (
  IN  VOID    *Request,
  IN  UINT32  RequestDataSize,
  OUT VOID    *Response,
  IN  UINT32  ResponseDataSize
  );

#endif /* FLASH_LIB_COMMON_H_ */
