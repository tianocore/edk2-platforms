//*****************************************************************************
//
// Copyright (C) 2020-2025 Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
//*****************************************************************************

// Add override tokens here

#ifdef ESPI_UART
#ifdef APCB_TOKEN_UID_FCH_CONSOLE_OUT_SERIAL_PORT_VALUE
  #undef APCB_TOKEN_UID_FCH_CONSOLE_OUT_SERIAL_PORT_VALUE
#endif
#define APCB_TOKEN_UID_FCH_CONSOLE_OUT_SERIAL_PORT_VALUE  0

#ifdef APCB_TOKEN_UID_FCH_CONSOLE_OUT_SERIAL_PORT_IO_VALUE
  #undef APCB_TOKEN_UID_FCH_CONSOLE_OUT_SERIAL_PORT_IO_VALUE
#endif
#define APCB_TOKEN_UID_FCH_CONSOLE_OUT_SERIAL_PORT_IO_VALUE 0
#endif /// end of ESPI_UART

#ifdef PCIE_MULTI_SEGMENT
    #define APCB_TOKEN_UID_DF_PCI_MMIO_BASE_VALUE     0x0
    #define APCB_TOKEN_UID_DF_PCI_MMIO_HI_BASE_VALUE  0x3FFB
#endif /// end of PCIE_MULTI_SEGMENT