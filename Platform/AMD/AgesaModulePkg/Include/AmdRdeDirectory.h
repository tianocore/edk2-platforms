/*****************************************************************************
 * Copyright Advanced Micro Devices, Inc. All rights reserved.
 *
*****************************************************************************
*/

#ifndef _AMD_RDE_DIRECTORY_H
#define _AMD_RDE_DIRECTORY_H

#include <Uefi.h>

/**
  Get AMD SoC RDE BIOS Driectory Image

  @param[in]      ImageType        Image type.
  @param[in]      InMediaOffset    The offset to the image in the media. The offset is related to the
                                   begining address of meida, such as the SPI ROM.
  @param[in out]  Imagesize        Pointer to the image size.
                                   When zero, the size of the image is returned.
                                   Otherwise, the image is read and returned to users
                                   the user's buffer if ImagePointer is not NULL.
  @param[out]     Buffer           Buffer pointer that points to memory buffer allocated by caller
                                   to store the image.

  @retval  EFI_SUCCESS  Initial library successfully.
  @retval  Other        Return error status.

**/
EFI_STATUS
EFIAPI
AmdRdeDirectoryGetImage (
  IN      UINT16  ImageType,
  IN      UINT64  *InMediaOffset OPTIONAL,
  IN OUT  UINT32  *Imagesize,
  OUT     VOID    *Buffer
  );

/**
  Get AMD SoC RDE Response table

  @param[in out] RdeResponseSize   RDE Response table size
                                   When zero, the size of the table is returned.
                                   Otherwise, the table is read and returned to users buffer if
                                   RdeResponseTable is not NULL.

  @param[out]    RdeResponseTable  Pointer to store the response table.
  @param[out]    InMediaOffset     The offset to the image in the media. The offset is related to the
                                   begining address of meida, such as the SPI ROM.

  @retval  EFI_SUCCESS            RDE response table is sucessfully returned to the caller's buffer.
  @retval  EFI_BUFFER_TOO_SMALL   RDE response table is returned to caller. Callers has to allocate a
                                  sufficient memory buffer to receive the table.
  @retval  Other                  Return error status.

**/
EFI_STATUS
EFIAPI
AmdRdeDirectoryGetResponseTable (
  IN OUT  UINT32  *RdeResponseSize,
  OUT     VOID    *RdeResponseTable,
  OUT     UINT64  *InMediaOffset
);

/**
  Set AMD SoC RDE Response table

  @param[in]   InMediaOffset     The offset to the image in the media. The offset is related to the
                                 begining address of meida, such as the SPI ROM.
  @param[in]   RdeResponseTable  Pointer to the response table.
  @param[in]   RdeResponseSize   RDE Response table size

  @retval  EFI_SUCCESS            RDE response table is sucessfully write to SPI.
  @retval  Other                  Return error status.

**/
EFI_STATUS
EFIAPI
AmdRdeDirectorySetResponseTable (
  IN  UINT64  InMediaOffset,
  IN  VOID    *RdeResponseTable,
  IN  UINT32  RdeResponseSize
);

#endif // _AMD_RDE_DIRECTORY_H
