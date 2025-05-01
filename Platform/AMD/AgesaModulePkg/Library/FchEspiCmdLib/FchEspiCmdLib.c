/** @file

  Copyright (C) 2008-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Filecode.h>
#include <Uefi.h>
/*----------------------------------------------------------------------------------------*/

/**
 * @param[in]  EspiBase       Espi MMIO base
 * @param[in]  RegAddr        Slave register address
 *
 * @retval    Register Value
 */
UINT32
FchEspiCmd_GetConfiguration  (
  IN  UINT32  EspiBase,
  IN  UINT32  RegAddr
  )
{
  return 0;
}

/*----------------------------------------------------------------------------------------*/

/**
 *
 * @param[in]  EspiBase       Espi MMIO base
 * @param[in]  Address        Address to read
 * @param[in]  Length         Length in byte to read
 * @param[in]  Buffer         Pointer to the data read to
 *
 */
EFI_STATUS
FchEspiCmd_SafsFlashRead  (
  IN  UINT32  EspiBase,
  IN  UINT32  Address,
  IN  UINT32  Length,
  OUT UINT8   *Buffer
  )
{
  return EFI_SUCCESS;
}

/*----------------------------------------------------------------------------------------*/

/**
 *
 * @param[in]  EspiBase       Espi MMIO base
 * @param[in]  Address        Address to write
 * @param[in]  Length         Length in byte to write
 * @param[in]  Value          Pointer to the data to write
 *
 */
EFI_STATUS
FchEspiCmd_SafsFlashWrite  (
  IN  UINT32  EspiBase,
  IN  UINT32  Address,
  IN  UINT32  Length,
  IN  UINT8   *Value
  )
{
  return EFI_SUCCESS;
}

/*----------------------------------------------------------------------------------------*/

/**
 *
 * @param[in]  EspiBase       Espi MMIO base
 * @param[in]  Address        Address to erase
 * @param[in]  Length         Block Size to erase
 *
 *
 */
EFI_STATUS
FchEspiCmd_SafsFlashErase  (
  IN  UINT32  EspiBase,
  IN  UINT32  Address,
  IN  UINT32  Length
  )
{
  return EFI_SUCCESS;
}
