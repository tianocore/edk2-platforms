/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/NVParamLib.h>

#include "MemInfoScreen.h"
#include "NVParamDef.h"

#define DDR_NVPARAM_ERRCTRL_DE_FIELD_SHIFT  0
#define DDR_NVPARAM_ERRCTRL_DE_FIELD_MASK   0x1

#define DDR_NVPARAM_ERRCTRL_FI_FIELD_SHIFT  1
#define DDR_NVPARAM_ERRCTRL_FI_FIELD_MASK   0x2

/**
  This is function collects meminfo from NVParam

  @param  Data                  The buffer to return the contents.

  @retval EFI_SUCCESS           Get response data successfully.
  @retval Other value           Failed to get meminfo from NVParam
**/
EFI_STATUS
MemInfoNvparamGet (
  OUT MEM_INFO_VARSTORE_DATA  *VarStoreConfig
  )
{
  UINT32      Value;
  EFI_STATUS  Status;

  ASSERT (VarStoreConfig != NULL);

  Status = NVParamGet (
             NV_SI_DDR_SPEED,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             &Value
             );
  if (EFI_ERROR (Status)) {
    VarStoreConfig->DDRSpeedSel = 0; /* Default auto mode */
  } else {
    VarStoreConfig->DDRSpeedSel = Value;
  }

  Status = NVParamGet (
             NV_SI_DDR_ECC_MODE,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             &Value
             );
  if (EFI_ERROR (Status)) {
    VarStoreConfig->EccMode = EccAuto;
  } else {
    VarStoreConfig->EccMode = Value;
  }

  Status = NVParamGet (
             NV_SI_DDR_ERRCTRL,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             &Value
             );
  if (EFI_ERROR (Status)) {
    VarStoreConfig->ErrCtrl_DE = ErrCtlrDeEnable;
    VarStoreConfig->ErrCtrl_FI = ErrCtlrFiEnable;
  } else {
    VarStoreConfig->ErrCtrl_DE = (Value & DDR_NVPARAM_ERRCTRL_DE_FIELD_MASK) >> DDR_NVPARAM_ERRCTRL_DE_FIELD_SHIFT;
    VarStoreConfig->ErrCtrl_FI = (Value & DDR_NVPARAM_ERRCTRL_FI_FIELD_MASK) >> DDR_NVPARAM_ERRCTRL_FI_FIELD_SHIFT;
  }

  Status = NVParamGet (
             NV_SI_DDR_SLAVE_32BIT_MEM_EN,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             &Value
             );
  if (EFI_ERROR (Status)) {
    VarStoreConfig->Slave32bit = 0; /* Default disabled */
  } else {
    VarStoreConfig->Slave32bit = Value;
  }

  Status = NVParamGet (
             NV_SI_DDR_SCRUB_EN,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             &Value
             );
  if (EFI_ERROR (Status)) {
    VarStoreConfig->ScrubPatrol = DDR_DEFAULT_SCRUB_PATROL_DURATION;
  } else {
    VarStoreConfig->ScrubPatrol = Value;
  }

  Status = NVParamGet (
             NV_SI_DDR_WR_BACK_EN,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             &Value
             );
  if (EFI_ERROR (Status)) {
    VarStoreConfig->DemandScrub = DDR_DEFAULT_DEMAND_SCRUB;
  } else {
    VarStoreConfig->DemandScrub = Value;
  }

  Status = NVParamGet (
             NV_SI_DDR_CRC_MODE,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             &Value
             );
  if (EFI_ERROR (Status)) {
    VarStoreConfig->WriteCrc = DDR_DEFAULT_WRITE_CRC;
  } else {
    VarStoreConfig->WriteCrc = Value;
  }

  Status = NVParamGet (
             NV_SI_DDR_REFRESH_GRANULARITY,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             &Value
             );
  if (EFI_ERROR (Status)) {
    VarStoreConfig->FGRMode   = DDR_DEFAULT_FGR_MODE;
    VarStoreConfig->Refresh2x = DDR_DEFAULT_REFRESH2X_MODE;
  } else {
    VarStoreConfig->FGRMode   = DDR_FGR_MODE_GET (Value);
    VarStoreConfig->Refresh2x = DDR_REFRESH_2X_GET (Value);
  }

  Status = NVParamGet (
             NV_SI_NVDIMM_MODE,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
             &Value
             );
  if (EFI_ERROR (Status)) {
    VarStoreConfig->NvdimmModeSel = DDR_DEFAULT_NVDIMM_MODE_SEL;
  } else {
    VarStoreConfig->NvdimmModeSel = Value & DDR_NVDIMM_MODE_SEL_MASK; /* Mask out valid bit */
  }

  return EFI_SUCCESS;
}

/**
  This is function stores meminfo to corresponding NVParam

  @param  VarStoreConfig         The contents for the variable.

  @retval EFI_SUCCESS            Set data successfully.
  @retval Other value            Failed to set meminfo to NVParam

**/
EFI_STATUS
MemInfoNvparamSet (
  IN MEM_INFO_VARSTORE_DATA  *VarStoreConfig
  )
{
  EFI_STATUS  Status;
  UINT32      Value, TmpValue, Value2, Update;

  ASSERT (VarStoreConfig != NULL);

  /* Set DDR speed */
  Status = NVParamGet (
             NV_SI_DDR_SPEED,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU |NV_PERM_BMC,
             &Value
             );
  if (EFI_ERROR (Status) || (Value != VarStoreConfig->DDRSpeedSel)) {
    Status = NVParamSet (
               NV_SI_DDR_SPEED,
               NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU |NV_PERM_BMC,
               NV_PERM_BIOS | NV_PERM_MANU,
               VarStoreConfig->DDRSpeedSel
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  /* Set ECC mode */
  Status = NVParamGet (
             NV_SI_DDR_ECC_MODE,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU |NV_PERM_BMC,
             &Value
             );

  TmpValue = VarStoreConfig->EccMode;
  if (EFI_ERROR (Status) || (Value != TmpValue)) {
    if (TmpValue == EccAuto) {
      Status = NVParamClr (
                 NV_SI_DDR_ECC_MODE,
                 NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC
                 );
    } else {
      Status = NVParamSet (
                 NV_SI_DDR_ECC_MODE,
                 NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU |NV_PERM_BMC,
                 NV_PERM_BIOS | NV_PERM_MANU,
                 VarStoreConfig->EccMode
                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }

  /* Set ErrCtrl */
  TmpValue = (VarStoreConfig->ErrCtrl_DE << DDR_NVPARAM_ERRCTRL_DE_FIELD_SHIFT) |
             (VarStoreConfig->ErrCtrl_FI << DDR_NVPARAM_ERRCTRL_FI_FIELD_SHIFT);
  Status = NVParamGet (
             NV_SI_DDR_ERRCTRL,
             NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU |NV_PERM_BMC,
             &Value
             );
  if (EFI_ERROR (Status) || (Value != TmpValue)) {
    Status = NVParamSet (
               NV_SI_DDR_ERRCTRL,
               NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU |NV_PERM_BMC,
               NV_PERM_BIOS | NV_PERM_MANU,
               TmpValue
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  /* Set slave's 32bit region */
  TmpValue = VarStoreConfig->Slave32bit;
  Status   = NVParamGet (
               NV_SI_DDR_SLAVE_32BIT_MEM_EN,
               NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU |NV_PERM_BMC,
               &Value
               );
  if (EFI_ERROR (Status) || (Value != TmpValue)) {
    if (TmpValue == 0) {
      /* Default is disabled so just clear nvparam */
      Status = NVParamClr (
                 NV_SI_DDR_SLAVE_32BIT_MEM_EN,
                 NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU |NV_PERM_BMC
                 );
    } else {
      Status = NVParamSet (
                 NV_SI_DDR_SLAVE_32BIT_MEM_EN,
                 NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU |NV_PERM_BMC,
                 NV_PERM_BIOS | NV_PERM_MANU,
                 TmpValue
                 );
    }

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  /* Set Scrub patrol */
  TmpValue = VarStoreConfig->ScrubPatrol;
  Status   = NVParamGet (
               NV_SI_DDR_SCRUB_EN,
               NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU |NV_PERM_BMC,
               &Value
               );
  if (EFI_ERROR (Status) || (Value != TmpValue)) {
    if (TmpValue == DDR_DEFAULT_SCRUB_PATROL_DURATION) {
      Status = NVParamClr (
                 NV_SI_DDR_SCRUB_EN,
                 NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU |NV_PERM_BMC
                 );
    } else {
      Status = NVParamSet (
                 NV_SI_DDR_SCRUB_EN,
                 NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU |NV_PERM_BMC,
                 NV_PERM_BIOS | NV_PERM_MANU,
                 TmpValue
                 );
    }

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  /* Demand Scrub */
  TmpValue = VarStoreConfig->DemandScrub;
  Status   = NVParamGet (
               NV_SI_DDR_WR_BACK_EN,
               NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU |NV_PERM_BMC,
               &Value
               );
  if (EFI_ERROR (Status) || (Value != TmpValue)) {
    if (TmpValue == DDR_DEFAULT_DEMAND_SCRUB) {
      Status = NVParamClr (
                 NV_SI_DDR_WR_BACK_EN,
                 NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU |NV_PERM_BMC
                 );
    } else {
      Status = NVParamSet (
                 NV_SI_DDR_WR_BACK_EN,
                 NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU |NV_PERM_BMC,
                 NV_PERM_BIOS | NV_PERM_MANU,
                 TmpValue
                 );
    }

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  /* Write CRC */
  TmpValue = VarStoreConfig->WriteCrc;
  Status   = NVParamGet (
               NV_SI_DDR_CRC_MODE,
               NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
               &Value
               );
  if (EFI_ERROR (Status) || (Value != TmpValue)) {
    if (TmpValue == DDR_DEFAULT_WRITE_CRC) {
      Status = NVParamClr (
                 NV_SI_DDR_CRC_MODE,
                 NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC
                 );
    } else {
      Status = NVParamSet (
                 NV_SI_DDR_CRC_MODE,
                 NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
                 NV_PERM_BIOS | NV_PERM_MANU,
                 TmpValue
                 );
    }

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  /* Write FGR/Refresh2X */
  Value    = 0;
  Update   = 0;
  TmpValue = VarStoreConfig->FGRMode;
  Status   = NVParamGet (
               NV_SI_DDR_REFRESH_GRANULARITY,
               NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
               &Value
               );
  Value2 = DDR_FGR_MODE_GET (Value);
  if (  (EFI_ERROR (Status) && (TmpValue != DDR_DEFAULT_FGR_MODE))
     || (Value2 != TmpValue))
  {
    DDR_FGR_MODE_SET (Value, TmpValue);
    Update = 1;
  }

  Value2   = DDR_REFRESH_2X_GET (Value);
  TmpValue = VarStoreConfig->Refresh2x;
  if (  (EFI_ERROR (Status) && (TmpValue != DDR_DEFAULT_REFRESH2X_MODE))
     || (Value2 != TmpValue))
  {
    DDR_REFRESH_2X_SET (Value, TmpValue);
    Update = 1;
  }

  if (Update == 1) {
    Status = NVParamSet (
               NV_SI_DDR_REFRESH_GRANULARITY,
               NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
               NV_PERM_BIOS | NV_PERM_MANU,
               Value
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  /* Write NVDIMM-N Mode selection */
  Value    = 0;
  TmpValue = VarStoreConfig->NvdimmModeSel;
  Status   = NVParamGet (
               NV_SI_NVDIMM_MODE,
               NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
               &Value
               );
  Value2 = Value & DDR_NVDIMM_MODE_SEL_MASK; /* Mask out valid bit */
  if (EFI_ERROR (Status) || (Value2 != TmpValue)) {
    if (TmpValue == DDR_DEFAULT_NVDIMM_MODE_SEL) {
      Status = NVParamClr (
                 NV_SI_NVDIMM_MODE,
                 NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC
                 );
    } else {
      Value  = TmpValue | DDR_NVDIMM_MODE_SEL_VALID_BIT; /* Add valid bit */
      Status = NVParamSet (
                 NV_SI_NVDIMM_MODE,
                 NV_PERM_ATF | NV_PERM_BIOS | NV_PERM_MANU | NV_PERM_BMC,
                 NV_PERM_BIOS | NV_PERM_MANU,
                 Value
                 );
    }

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}
