/** @file
  NXP TDA19988 HDMI transmitter UEFI Driver implementation

  Copyright (c) 2015, Oleksandr Tymoshenko <gonzo@freebsd.org>.<BR>
  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  SUCH DAMAGE.

**/

#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

#include "Driver.h"
#include "RegisterMap.h"

/// Revision of the TDA19988 tested and supported by this driver
#define HDMI_REV_TDA19988  0x0331

/// Marker meaning that the driver do not know the currently selected register
/// page
#define HDMI_PAGE_UNKNOWN  0xff

/**
  Reads a TDA19988 CEC 8-bit register.

  @param[in,out] Dev       Device context.
  @param[in]     Register  The TDA19988 CEC register to read.
  @param[out]    Value     Pointer to where to store the value read.

  @return EFI_SUCCESS The register was successfully read to the device.
  @return *           Other errors are possible.

**/
STATIC
EFI_STATUS
CecRead8 (
  IN OUT TDA19988_CONTEXT  *Dev,
  IN     CEC_REGISTER      Reg,
  OUT    UINT8             *Value
  )
{
  struct {
    UINTN                OperationCount;
    EFI_I2C_OPERATION    Reg;
    EFI_I2C_OPERATION    Value;
  } RequestPacket = {
    .OperationCount = 2,
    .Reg            = {
      .LengthInBytes = sizeof (Reg.Address),
      .Buffer        = &Reg.Address
    },
    .Value           = {
      .Flags         = I2C_FLAG_READ,
      .LengthInBytes = sizeof (*Value),
      .Buffer        = Value
    }
  };
  return Dev->I2cIo->QueueRequest (
                       Dev->I2cIo,
                       TDA19988_CEC_INDEX,
                       NULL,
                       (EFI_I2C_REQUEST_PACKET *)&RequestPacket,
                       NULL
                       );
}

/**
  Writes a TDA19988 CEC 8-bit register.

  @param[in,out] Dev       Device context.
  @param[in]     Register  The TDA19988 CEC register to modify.
  @param[in]     Value     The value to write to the TDA19988 CEC register.

  @return EFI_SUCCESS The register was successfully written to the device.
  @return *           Other errors are possible.

**/
STATIC
EFI_STATUS
CecWrite8 (
  IN OUT TDA19988_CONTEXT  *Dev,
  IN     CEC_REGISTER      Reg,
  IN     UINT8             Value
  )
{
  UINT8                   Buffer[2]     = { Reg.Address, Value };
  EFI_I2C_REQUEST_PACKET  RequestPacket = {
    .OperationCount = 1,
    .Operation      = {
      {
        .LengthInBytes = sizeof (Buffer),
        .Buffer        = Buffer
      }
    }
  };

  return Dev->I2cIo->QueueRequest (
                       Dev->I2cIo,
                       TDA19988_CEC_INDEX,
                       NULL,
                       &RequestPacket,
                       NULL
                       );
}

/**
  Change active the TDA19988 HDMI register page if needed.

  @param[in,out] Dev   Device context.
  @param[in]     Page  The TDA19988 HDMI register page to make active.

  @return EFI_SUCCESS The register was successfully written to the device.
  @return *           Other errors are possible.

**/
STATIC
EFI_STATUS
HdmiSetPage (
  IN OUT TDA19988_CONTEXT  *Dev,
  IN     UINT8             Page
  )
{
  if (Page == Dev->HdmiCurrentPage) {
    return EFI_SUCCESS;
  }

  EFI_STATUS              Status;
  UINT8                   Buffer[2]     = { HDMI_SELECT_PAGE, Page };
  EFI_I2C_REQUEST_PACKET  RequestPacket = {
    .OperationCount = 1,
    .Operation      = {
      {
        .LengthInBytes = sizeof (Buffer),
        .Buffer        = Buffer
      }
    }
  };

  Status = Dev->I2cIo->QueueRequest (
                         Dev->I2cIo,
                         TDA19988_HDMI_INDEX,
                         NULL,
                         &RequestPacket,
                         NULL
                         );
  if (EFI_ERROR (Status)) {
    // assume wierd state
    Dev->HdmiCurrentPage = HDMI_PAGE_UNKNOWN;
    return Status;
  }

  Dev->HdmiCurrentPage = Page;
  return EFI_SUCCESS;
}

/**
  Reads a TDA19988 HDMI 8-bit register.

  @param[in,out] Dev       Device context.
  @param[in]     Register  The TDA19988 HDMI register to read.
  @param[out]    Value     Pointer to where to store the value read.

  @return EFI_SUCCESS The register was successfully read to the device.
  @return *           Other errors are possible.

**/
STATIC
EFI_STATUS
HdmiRead8 (
  IN OUT TDA19988_CONTEXT  *Dev,
  IN     HDMI_REGISTER     Reg,
  OUT    UINT8             *Value
  )
{
  EFI_STATUS  Status;

  Status = HdmiSetPage (Dev, Reg.Page);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  struct {
    UINTN                OperationCount;
    EFI_I2C_OPERATION    Reg;
    EFI_I2C_OPERATION    Value;
  } RequestPacket = {
    .OperationCount = 2,
    .Reg            = {
      .LengthInBytes = sizeof (Reg.Address),
      .Buffer        = &Reg.Address
    },
    .Value           = {
      .Flags         = I2C_FLAG_READ,
      .LengthInBytes = sizeof (*Value),
      .Buffer        = Value
    }
  };
  return Dev->I2cIo->QueueRequest (
                       Dev->I2cIo,
                       TDA19988_HDMI_INDEX,
                       NULL,
                       (EFI_I2C_REQUEST_PACKET *)&RequestPacket,
                       NULL
                       );
}

/**
  Writes a TDA19988 HDMI 8-bit register.

  Writes the TDA19988 HDMI 8-bit register specified by Register with the value
  specified by Value.

  @param[in,out] Dev       Device context.
  @param[in]     Register  The TDA19988 HDMI register to modify.
  @param[in]     Value     The value to write to the TDA19988 HDMI register.

  @return EFI_SUCCESS The register was successfully written to the device.
  @return *           Other errors are possible.

**/
STATIC
EFI_STATUS
HdmiWrite8 (
  IN OUT TDA19988_CONTEXT  *Dev,
  IN     HDMI_REGISTER     Register,
  IN     UINT8             Value
  )
{
  EFI_STATUS  Status;

  Status = HdmiSetPage (Dev, Register.Page);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  UINT8                   Payload[] = {
    Register.Address,
    Value
  };
  EFI_I2C_REQUEST_PACKET  RequestPacket = {
    .OperationCount = 1,
    .Operation      = {
      {
        .LengthInBytes = sizeof (Payload),
        .Buffer        = Payload
      }
    }
  };

  return Dev->I2cIo->QueueRequest (
                       Dev->I2cIo,
                       TDA19988_HDMI_INDEX,
                       NULL,
                       &RequestPacket,
                       NULL
                       );
}

/**
  Writes a TDA19988 HDMI 16-bit register.

  Writes the TDA19988 HDMI 16-bit register specified by Register with the value
  specified by Value.

  @param[in,out] Dev       Device context.
  @param[in]     Register  The TDA19988 HDMI register to modify.
  @param[in]     Value     The value to write to the TDA19988 HDMI register.

  @return EFI_SUCCESS The register was successfully written to the device.
  @return *           Other errors are possible.

**/
STATIC
EFI_STATUS
HdmiWrite16 (
  IN OUT TDA19988_CONTEXT  *Dev,
  IN     HDMI_REGISTER     Reg,
  IN     UINT16            Value
  )
{
  EFI_STATUS  Status;

  Status = HdmiSetPage (Dev, Reg.Page);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  UINT8                   Payload[] = {
    Reg.Address,
    Value >> 8,
    Value >> 0
  };
  EFI_I2C_REQUEST_PACKET  RequestPacket = {
    .OperationCount = 1,
    .Operation      = {
      {
        .LengthInBytes = sizeof (Payload),
        .Buffer        = Payload
      }
    }
  };

  return Dev->I2cIo->QueueRequest (
                       Dev->I2cIo,
                       TDA19988_HDMI_INDEX,
                       NULL,
                       &RequestPacket,
                       NULL
                       );
}

/**
  Reads a TDA19988 HDMI 8-bit register, performs a bitwise OR, and writes the
  result back to the TDA19988 HDMI 8-bit register.

  Reads the TDA19988 HDMI 8-bit register specified by Register, performs a
  bitwise OR between the read result and the value specified by OrData, and
  writes the result to the TDA19988 HDMI register specified by Register.

  @param[in,out] Dev       Device context.
  @param[in]     Register  The TDA19988 HDMI register to modify.
  @param[in]     OrData    The value to OR with the read value from the
                           TDA19988 HDMI register.

  @return EFI_SUCCESS The register was successfully written back to the device.
  @return *           Other errors are possible.

**/
STATIC
EFI_STATUS
HdmiOr8 (
  IN OUT TDA19988_CONTEXT  *Dev,
  IN     HDMI_REGISTER     Register,
  IN     UINT8             OrData
  )
{
  EFI_STATUS  Status;
  UINT8       OldValue;

  Status = HdmiRead8 (Dev, Register, &OldValue);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return HdmiWrite8 (Dev, Register, OldValue | OrData);
}

/**
  Reads a TDA19988 HDMI 8-bit register, clear requested bits, and writes the
  result back to the TDA19988 HDMI 8-bit register.

  @param[in,out] Dev       Device context.
  @param[in]     Register  The TDA19988 HDMI register to modify.
  @param[in]     ClearMask Mask of bits to clear.

  @return EFI_SUCCESS The register was successfully written back to the device.
  @return *           Other errors are possible.

**/
STATIC
EFI_STATUS
HdmiClear8 (
  IN OUT TDA19988_CONTEXT  *Dev,
  IN     HDMI_REGISTER     Register,
  IN     UINT8             ClearMask
  )
{
  EFI_STATUS  Status;
  UINT8       OldValue;

  Status = HdmiRead8 (Dev, Register, &OldValue);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return HdmiWrite8 (Dev, Register, OldValue & ~ClearMask);
}

/**
  Read the TDA19988 HDMI revision.

  @param[in,out] Dev       Device context.
  @param[out]    Revision  Where to store the revision.

  @return EFI_SUCCESS The register was successfully written back to the device.
  @return *           Other errors are possible.

**/
STATIC
EFI_STATUS
GetRevision (
  IN OUT TDA19988_CONTEXT  *Dev,
  OUT    UINT16            *Revision
  )
{
  EFI_STATUS  Status;
  UINT8       RevisionLsb;
  UINT8       RevisionMsb;

  Status = HdmiRead8 (Dev, HDMI_VERSION, &RevisionLsb);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiRead8 (Dev, HDMI_VERSION_MSB, &RevisionMsb);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Revision = RevisionMsb << 8 | RevisionLsb;
  return EFI_SUCCESS;
}

/**
  Check if a sink is present by checking the hot-plug detection level.

  @param[in,out] Dev          Device context.
  @param[out]    SinkPresent  Write TRUE if sink is detected, otherwise FALSE.

  @retval EFI_SUCCESS           *SinkPresent updated and valid.
  @retval *                     Other errors are possible.

**/
EFI_STATUS
DriverGetSinkPresent (
  IN OUT TDA19988_CONTEXT  *Dev,
  OUT    BOOLEAN           *SinkPresent
  )
{
  EFI_STATUS  Status;
  UINT8       data;

  Status = CecRead8 (Dev, CEC_RXSHPDLEV, &data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *SinkPresent = data & CEC_RXSHPDLEV_HPD_LEVEL ? TRUE : FALSE;
  return EFI_SUCCESS;
}

/**
  Configure the specified mode.

  @param[in,out] Dev   Device context.
  @param[in]     Mode  Describe the mode to configure.

  @retval EFI_SUCCESS           Mode configured.
  @retval *                     Other errors are possible.

**/
EFI_STATUS
DriverSetMode (
  IN OUT TDA19988_CONTEXT         *Dev,
  IN     CONST TDA19988_MODEINFO  *Mode
  )
{
  CONST UINT32  PllNoScMagicDiv = 148500;

  EFI_STATUS  Status;
  UINT16      ref_pix, ref_line;
  UINT16      hs_pix_start, hs_pix_stop;
  UINT16      vs1_pix_start, vs1_pix_stop;
  UINT16      vs1_line_start, vs1_line_end;
  UINT16      vwin1_line_start, vwin1_line_end;
  UINT16      de_start, de_stop;
  UINT8       reg, div;
  UINT16      HorizontalTotal;
  UINT16      VerticalTotal;

  HorizontalTotal = Mode->Horizontal.Active
                    + Mode->Horizontal.FrontPorch
                    + Mode->Horizontal.Sync
                    + Mode->Horizontal.BackPorch;
  VerticalTotal = Mode->Vertical.Active
                  + Mode->Vertical.FrontPorch
                  + Mode->Vertical.Sync
                  + Mode->Vertical.BackPorch;

  hs_pix_stop  = Mode->Horizontal.FrontPorch + Mode->Horizontal.Sync;
  hs_pix_start = Mode->Horizontal.FrontPorch;

  de_stop  = HorizontalTotal;
  de_start = HorizontalTotal - Mode->Horizontal.Active;
  ref_pix  = hs_pix_start + 3;

  ref_line         = 1 + Mode->Vertical.FrontPorch;
  vwin1_line_start = VerticalTotal - Mode->Vertical.Active - 1;
  vwin1_line_end   = vwin1_line_start + Mode->Vertical.Active;

  vs1_pix_start  = vs1_pix_stop = hs_pix_start;
  vs1_line_start = Mode->Vertical.FrontPorch;
  vs1_line_end   = vs1_line_start
                   +  Mode->Vertical.FrontPorch
                   + Mode->Vertical.Sync
                   + Mode->Vertical.FrontPorch;

  // set HDMI HDCP mode off
  Status = HdmiOr8 (Dev, HDMI_TBG_CNTRL_1, HDMI_TBG_CNTRL_1_DWIN_DIS);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiClear8 (Dev, HDMI_HDCP_TX33, HDMI_HDCP_TX33_HDMI);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (Dev, HDMI_ENC_CNTRL, HDMI_ENC_CNTRL_DVI_MODE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // no pre-filter or interpolator
  Status = HdmiWrite8 (
             Dev,
             HDMI_HVF_CNTRL_0,
             HDMI_HVF_CNTRL_0_INTPOL_BYPASS | HDMI_HVF_CNTRL_0_PREFIL_NONE
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (Dev, HDMI_VIP_CNTRL_5, HDMI_VIP_CNTRL_5_SP_CNT (0));
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (
             Dev,
             HDMI_VIP_CNTRL_4,
             HDMI_VIP_CNTRL_4_BLANKIT_NDE | HDMI_VIP_CNTRL_4_BLC_NONE
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiClear8 (Dev, HDMI_PLL_SERIAL_3, HDMI_PLL_SERIAL_3_SRL_CCIR);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiClear8 (Dev, HDMI_PLL_SERIAL_1, HDMI_PLL_SERIAL_1_SRL_MAN_IP);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiClear8 (Dev, HDMI_PLL_SERIAL_3, HDMI_PLL_SERIAL_3_SRL_DE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (Dev, HDMI_SERIALIZER, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (Dev, HDMI_HVF_CNTRL_1, HDMI_HVF_CNTRL_1_VQR (0));
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (Dev, HDMI_RPT_CNTRL, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (
             Dev,
             HDMI_SEL_CLK,
             HDMI_SEL_CLK_SEL_VRF_CLK (0) |
             HDMI_SEL_CLK_SEL_CLK1 | HDMI_SEL_CLK_ENA_SC_CLK
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  div = PllNoScMagicDiv / Mode->PixelFreq;
  if (div != 0) {
    div--;
    if (div > 3) {
      div = 3;
    }
  }

  Status = HdmiWrite8 (
             Dev,
             HDMI_PLL_SERIAL_2,
             HDMI_PLL_SERIAL_2_SRL_NOSC (div) |
             HDMI_PLL_SERIAL_2_SRL_PR (0)
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiOr8 (Dev, HDMI_MAT_CONTRL, HDMI_MAT_CONTRL_MAT_BP);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (Dev, HDMI_ANA_GENERAL, 0x09);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiClear8 (Dev, HDMI_TBG_CNTRL_0, HDMI_TBG_CNTRL_0_SYNC_MTHD);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  reg = HDMI_VIP_CNTRL_3_SYNC_HS;
  if (Mode->Horizontal.SyncPolarityIsNegative) {
    reg |= HDMI_VIP_CNTRL_3_H_TGL;
  }

  if (Mode->Vertical.SyncPolarityIsNegative) {
    reg |= HDMI_VIP_CNTRL_3_V_TGL;
  }

  Status = HdmiWrite8 (Dev, HDMI_VIP_CNTRL_3, reg);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  reg = HDMI_TBG_CNTRL_1_TGL_EN;
  if (Mode->Horizontal.SyncPolarityIsNegative) {
    reg |= HDMI_TBG_CNTRL_1_H_TGL;
  }

  if (Mode->Vertical.SyncPolarityIsNegative) {
    reg |= HDMI_TBG_CNTRL_1_V_TGL;
  }

  Status = HdmiWrite8 (Dev, HDMI_TBG_CNTRL_1, reg);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // timing
  Status = HdmiWrite8 (Dev, HDMI_VIDFORMAT, 0x00);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite16 (Dev, HDMI_REFPIX, ref_pix);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite16 (Dev, HDMI_REFLINE, ref_line);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite16 (Dev, HDMI_NPIX, HorizontalTotal);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite16 (Dev, HDMI_NLINE, VerticalTotal);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite16 (Dev, HDMI_VS_LINE_STRT_1, vs1_line_start);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite16 (Dev, HDMI_VS_LINE_END_1, vs1_line_end);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite16 (Dev, HDMI_VS_PIX_STRT_1, vs1_pix_start);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite16 (Dev, HDMI_VS_PIX_END_1, vs1_pix_stop);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite16 (Dev, HDMI_VS_LINE_STRT_2, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite16 (Dev, HDMI_VS_PIX_STRT_2, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite16 (Dev, HDMI_VS_LINE_END_2, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite16 (Dev, HDMI_VS_PIX_END_2, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite16 (Dev, HDMI_HS_PIX_START, hs_pix_start);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite16 (Dev, HDMI_HS_PIX_STOP, hs_pix_stop);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite16 (Dev, HDMI_VWIN_START_1, vwin1_line_start);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite16 (Dev, HDMI_VWIN_END_1, vwin1_line_end);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite16 (Dev, HDMI_VWIN_START_2, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite16 (Dev, HDMI_VWIN_END_2, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite16 (Dev, HDMI_DE_START, de_start);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite16 (Dev, HDMI_DE_STOP, de_stop);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (Dev, HDMI_ENABLE_SPACE, 0x00);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // commit the changes
  Status = HdmiClear8 (Dev, HDMI_TBG_CNTRL_0, HDMI_TBG_CNTRL_0_SYNC_ONCE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Retrieve the EDID.

  Retrieve the EDID and copy it into an allocated buffer of type
  EfiBootServicesData.
  On success the EdidData is updated to point to the allocated buffer and
  EdidSize is updated with the size of the EDID.

  @param[in,out] Dev      Device context.
  @param[out]    EdidData Updated with pointer to an allocated buffer containing
                          the EDID.
  @param[out]    EdidSize Size of the EDID that EdidData is pointing to.

  @retval EFI_SUCCESS           *EdidData and *EdidSize updated and valid.
  @retval *                     Other errors are possible.

**/
EFI_STATUS
DriverGetEdid (
  IN OUT TDA19988_CONTEXT  *Dev,
  OUT    VOID              **EdidData,
  OUT    UINTN             *EdidSize
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Initialize a NXP TDA19988 hardware and connect it to the device context
  instance Dev.

  @param[in,out] Dev       Device context.

  @retval EFI_SUCCESS      The device was started.
  @retval EFI_UNSUPPORTED  The hardware is not compatible with this driver.
  @retval *                Other errors are possible.

**/
EFI_STATUS
DriverStart (
  IN OUT TDA19988_CONTEXT  *Dev
  )
{
  EFI_STATUS  Status;
  UINT16      Revision;

  Dev->HdmiCurrentPage = HDMI_PAGE_UNKNOWN;

  Status = CecWrite8 (Dev, CEC_ENAMODS, CEC_ENAMODS_ENA_HDMI);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetRevision (Dev, &Revision);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a@Index=%u]: failed to get revision - %r\n",
      gEfiCallerBaseName,
      Dev->I2cIo->DeviceIndex,
      Status
      ));
    return Status;
  }

  if (HDMI_REV_TDA19988 != Revision) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a@Index=%u]: unsupported revision: 0x%04x\n",
      gEfiCallerBaseName,
      Dev->I2cIo->DeviceIndex,
      Revision
      ));
    return EFI_UNSUPPORTED;
  }

  DEBUG ((
    DEBUG_INFO,
    "[%a@Index=%u]: detected revision: 0x%04x\n",
    gEfiCallerBaseName,
    Dev->I2cIo->DeviceIndex,
    Revision
    ));

  Status = HdmiOr8 (
             Dev,
             HDMI_SOFTRESET,
             HDMI_SOFTRESET_I2C | HDMI_SOFTRESET_AUDIO
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->Stall (100);

  Status = HdmiClear8 (
             Dev,
             HDMI_SOFTRESET,
             HDMI_SOFTRESET_I2C | HDMI_SOFTRESET_AUDIO
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->Stall (100);

  // reset transmitter
  Status = HdmiOr8 (Dev, HDMI_MAIN_CNTRL0, HDMI_MAIN_CNTRL0_SR);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiClear8 (Dev, HDMI_MAIN_CNTRL0, HDMI_MAIN_CNTRL0_SR);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (Dev, HDMI_PLL_SERIAL_1, 0x00);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (Dev, HDMI_PLL_SERIAL_2, HDMI_PLL_SERIAL_2_SRL_NOSC (1));
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (Dev, HDMI_PLL_SERIAL_3, 0x00);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (Dev, HDMI_SERIALIZER, 0x00);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (Dev, HDMI_BUFFER_OUT, 0x00);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (Dev, HDMI_PLL_SCG1, 0x00);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (
             Dev,
             HDMI_SEL_CLK,
             HDMI_SEL_CLK_SEL_CLK1 | HDMI_SEL_CLK_ENA_SC_CLK
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (Dev, HDMI_PLL_SCGN1, 0xfa);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (Dev, HDMI_PLL_SCGN2, 0x00);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (Dev, HDMI_PLL_SCGR1, 0x5b);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (Dev, HDMI_PLL_SCGR2, 0x00);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (Dev, HDMI_PLL_SCG2, 0x10);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (
             Dev,
             HDMI_MUX_VP_VIP_OUT,
             HDMI_MUX_VP_VIP_OUT_R_R |
             HDMI_MUX_VP_VIP_OUT_G_G |
             HDMI_MUX_VP_VIP_OUT_B_B
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (Dev, HDMI_DDC_CTRL, HDMI_DDC_ENABLE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (Dev, HDMI_TX3, 39);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (Dev, HDMI_VIP_CNTRL_0, 0x0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (Dev, HDMI_VIP_CNTRL_1, 0x0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (Dev, HDMI_VIP_CNTRL_2, 0x0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // RGB external synchronization input (rising edge) mapping with
  //  Blue[MSB..LSB]  -> Video Port A[MSB..LSB]
  //  Green[MSB..LSB] -> Video Port B[MSB..LSB]
  //  Red[MSB..LSB]   -> Video Port C[MSB..LSB]
  Status = HdmiWrite8 (Dev, HDMI_VIP_CNTRL_0, 0x23);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (Dev, HDMI_VIP_CNTRL_1, 0x45);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HdmiWrite8 (Dev, HDMI_VIP_CNTRL_2, 0x01);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Disconnect the hardware from the driver instance.

  @param[in,out] Dev  Device context.
**/
VOID
DriverStop (
  IN OUT TDA19988_CONTEXT  *Dev OPTIONAL
  )
{
}
