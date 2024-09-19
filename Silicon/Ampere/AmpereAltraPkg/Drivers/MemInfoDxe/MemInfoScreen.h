/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MEM_INFO_SCREEN_H_
#define MEM_INFO_SCREEN_H_

#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiConfigKeyword.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiString.h>

#include "MemInfoScreenNVDataStruct.h"

//
// This is the generated IFR binary data for each formset defined in VFR.
// This data array is ready to be used as input of HiiAddPackages() to
// create a packagelist (which contains Form packages, String packages, etc).
//
extern UINT8  MemInfoScreenVfrBin[];

//
// This is the generated String package data for all .UNI files.
// This data array is ready to be used as input of HiiAddPackages() to
// create a packagelist (which contains Form packages, String packages, etc).
//
extern UINT8  MemInfoDxeStrings[];

typedef enum {
  EccDisabled = 0,
  EccSecded,
  EccSymbol,
  EccAuto,
  EccMax
} DDR_ECC_MODE;

typedef enum {
  ErrCtlrDeDisable = 0,
  ErrCtlrDeEnable,
  ErrCtlrDeMax
} DDR_ERROR_CTRL_MODE_DE;

typedef enum {
  ErrCtlrFiDisable = 0,
  ErrCtlrFiEnable,
  ErrCtlrFiMax
} DDR_ERROR_CTRL_MODE_FI;

#define MEM_INFO_DDR_SPEED_SEL_OFFSET         OFFSET_OF (MEM_INFO_VARSTORE_DATA, DDRSpeedSel)
#define MEM_INFO_ECC_MODE_SEL_OFFSET          OFFSET_OF (MEM_INFO_VARSTORE_DATA, EccMode)
#define MEM_INFO_ERR_CTRL_DE_MODE_SEL_OFFSET  OFFSET_OF (MEM_INFO_VARSTORE_DATA, ErrCtrl_DE)
#define MEM_INFO_ERR_CTRL_FI_MODE_SEL_OFFSET  OFFSET_OF (MEM_INFO_VARSTORE_DATA, ErrCtrl_FI)
#define MEM_INFO_ERR_SLAVE_32BIT_OFFSET       OFFSET_OF (MEM_INFO_VARSTORE_DATA, Slave32bit)
#define MEM_INFO_DDR_SCRUB_OFFSET             OFFSET_OF (MEM_INFO_VARSTORE_DATA, ScrubPatrol)
#define MEM_INFO_DDR_DEMAND_SCRUB_OFFSET      OFFSET_OF (MEM_INFO_VARSTORE_DATA, DemandScrub)
#define MEM_INFO_DDR_WRITE_CRC_OFFSET         OFFSET_OF (MEM_INFO_VARSTORE_DATA, WriteCrc)
#define MEM_INFO_FGR_MODE_OFFSET              OFFSET_OF (MEM_INFO_VARSTORE_DATA, FGRMode)
#define MEM_INFO_REFRESH2X_MODE_OFFSET        OFFSET_OF (MEM_INFO_VARSTORE_DATA, Refresh2x)
#define MEM_INFO_NVDIMM_MODE_SEL_OFFSET       OFFSET_OF (MEM_INFO_VARSTORE_DATA, NvdimmModeSel)

#define MEM_INFO_SCREEN_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('M', 'E', 'M', 'i')

#define MEM_INFO_DDR_SPEED_SEL_QUESTION_ID                 0x8001
#define MEM_INFO_FORM_PERFORMANCE_QUESTION_ID              0x8002
#define MEM_INFO_FORM_PERFORMANCE_ECC_QUESTION_ID          0x8003
#define MEM_INFO_FORM_PERFORMANCE_ERR_CTRL_DE_QUESTION_ID  0x8004
#define MEM_INFO_FORM_PERFORMANCE_ERR_CTRL_FI_QUESTION_ID  0x8005
#define MEM_INFO_DDR_SLAVE_32BIT_QUESTION_ID               0x8006
#define MEM_INFO_DDR_SCRUB_PATROL_QUESTION_ID              0x8007
#define MEM_INFO_DDR_DEMAND_SCRUB_QUESTION_ID              0x8008
#define MEM_INFO_DDR_WRITE_CRC_QUESTION_ID                 0x8009
#define MEM_INFO_FGR_MODE_QUESTION_ID                      0x800A
#define MEM_INFO_REFRESH2X_MODE_QUESTION_ID                0x800B
#define MEM_INFO_FORM_NVDIMM_QUESTION_ID                   0x800C
#define MEM_INFO_FORM_NVDIMM_MODE_SEL_QUESTION_ID          0x800D

#define MAX_NUMBER_OF_HOURS_IN_A_DAY  24

#define DDR_DEFAULT_SCRUB_PATROL_DURATION  24
#define DDR_DEFAULT_DEMAND_SCRUB           1
#define DDR_DEFAULT_WRITE_CRC              0
#define DDR_DEFAULT_FGR_MODE               0
#define DDR_DEFAULT_REFRESH2X_MODE         0
#define DDR_DEFAULT_NVDIMM_MODE_SEL        3

#define DDR_FGR_MODE_GET(Value)     ((Value) & 0x3)       /* Bit 0, 1 */
#define DDR_FGR_MODE_SET(Dst, Src)  do { Dst = (((Dst) & ~0x3) | ((Src) & 0x3)); } while (0)

#define DDR_REFRESH_2X_GET(Value)     ((Value) & 0x10000) >> 16     /* Bit 16 only */
#define DDR_REFRESH_2X_SET(Dst, Src)  do { Dst = (((Dst) & ~0x10000) | ((Src) & 0x1) << 16); } while (0)

#define DDR_NVDIMM_MODE_SEL_MASK       0x7FFFFFFF
#define DDR_NVDIMM_MODE_SEL_VALID_BIT  BIT31

typedef struct {
  UINTN                                  Signature;

  EFI_HANDLE                             DriverHandle;
  EFI_HII_HANDLE                         HiiHandle;
  MEM_INFO_VARSTORE_DATA                 VarStoreConfig;

  //
  // Consumed protocol
  //
  EFI_HII_DATABASE_PROTOCOL              *HiiDatabase;
  EFI_HII_STRING_PROTOCOL                *HiiString;
  EFI_HII_CONFIG_ROUTING_PROTOCOL        *HiiConfigRouting;
  EFI_CONFIG_KEYWORD_HANDLER_PROTOCOL    *HiiKeywordHandler;
  EFI_FORM_BROWSER2_PROTOCOL             *FormBrowser2;

  //
  // Produced protocol
  //
  EFI_HII_CONFIG_ACCESS_PROTOCOL         ConfigAccess;
} MEM_INFO_SCREEN_PRIVATE_DATA;

#define MEM_INFO_SCREEN_PRIVATE_FROM_THIS(a)  CR (a, MEM_INFO_SCREEN_PRIVATE_DATA, ConfigAccess, MEM_INFO_SCREEN_PRIVATE_DATA_SIGNATURE)

#pragma pack(1)

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;

#pragma pack()

EFI_STATUS
MemInfoScreenInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

EFI_STATUS
MemInfoScreenUnload (
  IN EFI_HANDLE  ImageHandle
  );

EFI_STATUS
MemInfoNvparamGet (
  OUT MEM_INFO_VARSTORE_DATA  *VarStoreConfig
  );

EFI_STATUS
MemInfoNvparamSet (
  IN MEM_INFO_VARSTORE_DATA  *VarStoreConfig
  );

#endif /* MEM_INFO_SCREEN_H_ */
