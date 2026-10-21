/** @file
  Copyright (c) 2025 Intel Corporation. All rights reserved. <BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __FSP_WRAPPER_NON_VOLATILE_STORAGE_PROTOCOL__
#define __FSP_WRAPPER_NON_VOLATILE_STORAGE_PROTOCOL__

#define FSP_WRAPPER_NON_VOLATILE_STORAGE_PROTOCOL_GUID \
  { \
    0xd2e8223a, 0x7201, 0x45ad, { 0x89, 0x5c, 0x81, 0x6d, 0x88, 0x1f, 0xd9, 0x33 } \
  }

typedef struct _FSP_WRAPPER_NON_VOLATILE_STORAGE_PROTOCOL FSP_WRAPPER_NON_VOLATILE_STORAGE_PROTOCOL;

/**
  Write DataLength bytes pointed by Data to the Offset-th byte in NVS buffer.

  @param[in] Offset               Byte offset from the beginning of NVS buffer.
  @param[in] Data                 Pointer to the data to write.
  @param[in] DataLength           Number of bytes to write.

  @retval EFI_SUCCESS             Write succeeds.
  @retval EFI_DEVICE_ERROR        Write fails due to NVS device error.
  @retval EFI_INVALID_PARAMETER   Offset exceeds the total length of NVS buffer.
  @retval EFI_INVALID_PARAMETER   Offset + DataLength exceeds the total length of NVS buffer.
**/
typedef
EFI_STATUS
(EFIAPI *FSP_WRAPPER_NON_VOLATILE_STORAGE_WRITE)(
  IN UINTN  Offset,
  IN VOID   *Data,
  IN UINTN  DataLength
  );

struct _FSP_WRAPPER_NON_VOLATILE_STORAGE_PROTOCOL {
  UINT32                                    Revision;
  FSP_WRAPPER_NON_VOLATILE_STORAGE_WRITE    Write;
};

extern EFI_GUID  gFspWrapperNonVolatileStorageProtocolGuid;

#endif // __FSP_WRAPPER_NON_VOLATILE_STORAGE_PROTOCOL__
