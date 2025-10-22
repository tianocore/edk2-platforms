/** @file
  NXP TDA19988 HDMI transmitter register map

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

#ifndef MORELLO_TDA19988DXE_REGISTERMAP_H_
#define MORELLO_TDA19988DXE_REGISTERMAP_H_

#include <Uefi.h>

/// The CEC Core register description
typedef struct {
  UINT8    Address;
} CEC_REGISTER;

#define CEC_REG(addr)  (CEC_REGISTER){ addr }

#define CEC_ENAMODS              CEC_REG(0xff)
#define CEC_ENAMODS_ENA_HDMI     BIT1
#define CEC_ENAMODS_DIS_FRO      BIT6
#define CEC_RXSHPDLEV            CEC_REG(0xfeu)
#define CEC_RXSHPDLEV_HPD_LEVEL  BIT1

/// The HDMI Core register description
typedef struct {
  UINT8    Page;
  UINT8    Address;
} HDMI_REGISTER;

#define HDMI_REG(page, addr)  (HDMI_REGISTER){ page, addr }

// this register is in all pages
#define HDMI_SELECT_PAGE  0xff

#define HDMI_VERSION                     HDMI_REG(0x00, 0x00)
#define HDMI_MAIN_CNTRL0                 HDMI_REG(0x00, 0x01)
#define HDMI_MAIN_CNTRL0_SR              (1 << 0)
#define HDMI_VERSION_MSB                 HDMI_REG(0x00, 0x02)
#define HDMI_SOFTRESET                   HDMI_REG(0x00, 0x0a)
#define HDMI_SOFTRESET_AUDIO             (1 << 0)
#define HDMI_SOFTRESET_I2C               (1 << 1)
#define HDMI_DDC_CTRL                    HDMI_REG(0x00, 0x0b)
#define HDMI_DDC_ENABLE                  (0 << 0)
#define HDMI_INT_FLAGS_2                 HDMI_REG(0x00, 0x11)
#define HDMI_INT_FLAGS_2_EDID_BLK_RD     (1 << 1)
#define HDMI_VIP_CNTRL_0                 HDMI_REG(0x00, 0x20)
#define HDMI_VIP_CNTRL_1                 HDMI_REG(0x00, 0x21)
#define HDMI_VIP_CNTRL_2                 HDMI_REG(0x00, 0x22)
#define HDMI_VIP_CNTRL_3                 HDMI_REG(0x00, 0x23)
#define HDMI_VIP_CNTRL_3_H_TGL           (1 << 1)
#define HDMI_VIP_CNTRL_3_V_TGL           (1 << 2)
#define HDMI_VIP_CNTRL_3_SYNC_HS         (2 << 4)
#define HDMI_VIP_CNTRL_4                 HDMI_REG(0x00, 0x24)
#define HDMI_VIP_CNTRL_4_BLC_NONE        (0 << 0)
#define HDMI_VIP_CNTRL_4_BLC_RGB444      (1 << 0)
#define HDMI_VIP_CNTRL_4_BLC_YUV444      (2 << 0)
#define HDMI_VIP_CNTRL_4_BLC_YUV422      (3 << 0)
#define HDMI_VIP_CNTRL_4_BLANKIT_NDE     (0 << 2)
#define HDMI_VIP_CNTRL_4_BLANKIT_HS_VS   (1 << 2)
#define HDMI_VIP_CNTRL_4_BLANKIT_NHS_VS  (2 << 2)
#define HDMI_VIP_CNTRL_4_BLANKIT_HE_VE   (3 << 2)
#define HDMI_VIP_CNTRL_5                 HDMI_REG(0x00, 0x25)
#define HDMI_VIP_CNTRL_5_SP_CNT(n)  (((n) & 3) << 1)
#define HDMI_MUX_VP_VIP_OUT             HDMI_REG(0x00, 0x27)
#define HDMI_MUX_VP_VIP_OUT_R_B         (0x00)
#define HDMI_MUX_VP_VIP_OUT_R_G         (0x10)
#define HDMI_MUX_VP_VIP_OUT_R_R         (0x20)
#define HDMI_MUX_VP_VIP_OUT_G_B         (0x00)
#define HDMI_MUX_VP_VIP_OUT_G_G         (0x04)
#define HDMI_MUX_VP_VIP_OUT_G_R         (0x08)
#define HDMI_MUX_VP_VIP_OUT_B_B         (0x00)
#define HDMI_MUX_VP_VIP_OUT_B_G         (0x01)
#define HDMI_MUX_VP_VIP_OUT_B_R         (0x02)
#define HDMI_MAT_CONTRL                 HDMI_REG(0x00, 0x80)
#define HDMI_MAT_CONTRL_MAT_BP          (1 << 2)
#define HDMI_VIDFORMAT                  HDMI_REG(0x00, 0xa0)
#define HDMI_REFPIX                     HDMI_REG(0x00, 0xa1)
#define HDMI_REFLINE                    HDMI_REG(0x00, 0xa3)
#define HDMI_NPIX                       HDMI_REG(0x00, 0xa5)
#define HDMI_NLINE                      HDMI_REG(0x00, 0xa7)
#define HDMI_VS_LINE_STRT_1             HDMI_REG(0x00, 0xa9)
#define HDMI_VS_PIX_STRT_1              HDMI_REG(0x00, 0xab)
#define HDMI_VS_LINE_END_1              HDMI_REG(0x00, 0xad)
#define HDMI_VS_PIX_END_1               HDMI_REG(0x00, 0xaf)
#define HDMI_VS_LINE_STRT_2             HDMI_REG(0x00, 0xb1)
#define HDMI_VS_PIX_STRT_2              HDMI_REG(0x00, 0xb3)
#define HDMI_VS_LINE_END_2              HDMI_REG(0x00, 0xb5)
#define HDMI_VS_PIX_END_2               HDMI_REG(0x00, 0xb7)
#define HDMI_HS_PIX_START               HDMI_REG(0x00, 0xb9)
#define HDMI_HS_PIX_STOP                HDMI_REG(0x00, 0xbb)
#define HDMI_VWIN_START_1               HDMI_REG(0x00, 0xbd)
#define HDMI_VWIN_END_1                 HDMI_REG(0x00, 0xbf)
#define HDMI_VWIN_START_2               HDMI_REG(0x00, 0xc1)
#define HDMI_VWIN_END_2                 HDMI_REG(0x00, 0xc3)
#define HDMI_DE_START                   HDMI_REG(0x00, 0xc5)
#define HDMI_DE_STOP                    HDMI_REG(0x00, 0xc7)
#define HDMI_TBG_CNTRL_0                HDMI_REG(0x00, 0xca)
#define HDMI_TBG_CNTRL_0_SYNC_MTHD      (1 << 6)
#define HDMI_TBG_CNTRL_0_SYNC_ONCE      (1 << 7)
#define HDMI_TBG_CNTRL_1                HDMI_REG(0x00, 0xcb)
#define HDMI_TBG_CNTRL_1_H_TGL          (1 << 0)
#define HDMI_TBG_CNTRL_1_V_TGL          (1 << 1)
#define HDMI_TBG_CNTRL_1_TGL_EN         (1 << 2)
#define HDMI_TBG_CNTRL_1_DWIN_DIS       (1 << 6)
#define HDMI_ENABLE_SPACE               HDMI_REG(0x00, 0xd6)
#define HDMI_HVF_CNTRL_0                HDMI_REG(0x00, 0xe4)
#define HDMI_HVF_CNTRL_0_INTPOL_BYPASS  (0 << 0)
#define HDMI_HVF_CNTRL_0_PREFIL_NONE    (0 << 2)
#define HDMI_HVF_CNTRL_1                HDMI_REG(0x00, 0xe5)
#define HDMI_HVF_CNTRL_1_VQR(x)  (((x) & 3) << 2)
#define HDMI_RPT_CNTRL                HDMI_REG(0x00, 0xf0)
#define HDMI_PLL_SERIAL_1             HDMI_REG(0x02, 0x00)
#define HDMI_PLL_SERIAL_1_SRL_MAN_IP  (1 << 6)
#define HDMI_PLL_SERIAL_2             HDMI_REG(0x02, 0x01)
#define HDMI_PLL_SERIAL_2_SRL_NOSC(x)  (((x) & 0x3) << 0)
#define HDMI_PLL_SERIAL_2_SRL_PR(x)    (((x) & 0xf) << 4)
#define HDMI_PLL_SERIAL_3               HDMI_REG(0x02, 0x02)
#define HDMI_PLL_SERIAL_3_SRL_CCIR      (1 << 0)
#define HDMI_PLL_SERIAL_3_SRL_DE        (1 << 2)
#define HDMI_PLL_SERIAL_3_SRL_PXIN_SEL  (1 << 4)
#define HDMI_SERIALIZER                 HDMI_REG(0x02, 0x03)
#define HDMI_BUFFER_OUT                 HDMI_REG(0x02, 0x04)
#define HDMI_PLL_SCG1                   HDMI_REG(0x02, 0x05)
#define HDMI_PLL_SCG2                   HDMI_REG(0x02, 0x06)
#define HDMI_PLL_SCGN1                  HDMI_REG(0x02, 0x07)
#define HDMI_PLL_SCGN2                  HDMI_REG(0x02, 0x08)
#define HDMI_PLL_SCGR1                  HDMI_REG(0x02, 0x09)
#define HDMI_PLL_SCGR2                  HDMI_REG(0x02, 0x0a)
#define HDMI_SEL_CLK                    HDMI_REG(0x02, 0x11)
#define HDMI_SEL_CLK_SEL_CLK1           (1 << 0)
#define HDMI_SEL_CLK_SEL_VRF_CLK(x)  (((x) & 3) << 1)
#define HDMI_SEL_CLK_ENA_SC_CLK   (1 << 3)
#define HDMI_ANA_GENERAL          HDMI_REG(0x02, 0x12)
#define HDMI_EDID_DATA0           HDMI_REG(0x09, 0x00)
#define HDMI_EDID_CTRL            HDMI_REG(0x09, 0xfa)
#define HDMI_DDC_ADDR             HDMI_REG(0x09, 0xfb)
#define HDMI_DDC_OFFS             HDMI_REG(0x09, 0xfc)
#define HDMI_DDC_SEGM_ADDR        HDMI_REG(0x09, 0xfd)
#define HDMI_DDC_SEGM             HDMI_REG(0x09, 0xfe)
#define HDMI_ENC_CNTRL            HDMI_REG(0x11, 0x0d)
#define HDMI_ENC_CNTRL_DVI_MODE   (0 << 2)
#define HDMI_ENC_CNTRL_HDMI_MODE  (1 << 2)
#define HDMI_DIP_IF_FLAGS         HDMI_REG(0x11, 0x0f)
#define HDMI_DIP_IF_FLAGS_IF1     (1 << 1)
#define HDMI_DIP_IF_FLAGS_IF2     (1 << 2)
#define HDMI_DIP_IF_FLAGS_IF3     (1 << 3)
#define HDMI_DIP_IF_FLAGS_IF4     (1 << 4)
#define HDMI_DIP_IF_FLAGS_IF5     (1 << 5)
#define HDMI_TX3                  HDMI_REG(0x12, 0x9a)
#define HDMI_TX4                  HDMI_REG(0x12, 0x9b)
#define HDMI_TX4_PD_RAM           (1 << 1)
#define HDMI_HDCP_TX33            HDMI_REG(0x12, 0xb8)
#define HDMI_HDCP_TX33_HDMI       (1 << 1)

/// EDID related defines
#define HDMI_EDID_DEV_ADDR       0xa0
#define HDMI_EDID_SEG_PTR_ADDR   0x60
#define HDMI_EDID_REQ_READ_MASK  0x01

#endif // MORELLO_TDA19988DXE_REGISTERMAP_H_
