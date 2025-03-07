"""
*******************************************************************************
 Copyright (C) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
 SPDX-License-Identifier: BSD-2-Clause-Patent

*******************************************************************************
"""

import os

def projectpostbuild():
    # These will be set elsewhere with dbuild.py and can be removed when it
    # replaces dbuild.cmd
    os.environ["SOC_FAMILY"] = os.environ.get("SOC_FAMILY", "0x19")
    os.environ["SOC_SKU"] = os.environ.get("SOC_SKU", "RS")
    os.environ["SOC2"] = os.environ.get("SOC2", "STONES")
    os.environ["SOCKET"] = os.environ.get("SOCKET", "SP6")

    workspace = os.environ['WORKSPACE']
    build_output = os.environ['BUILD_OUTPUT']

    os.environ['APCB_TOOL_TEMP_PATH'] = os.path.normpath(os.path.join(
        workspace,
        'AGESA/AgesaPkg/Addendum/Apcb/GenoaSp5Rdimm'
    ))
    os.environ['APCB_MULTI_BOARD_SUPPORT'] = '1'
    os.environ['APCB_DATA_BOARD_DIR_LIST'] = 'GenoaCommon Shale'
    os.environ['CUSTOM_APCB_PATH'] = os.path.normpath(os.path.join(
        build_output,
        'Apcb'
    ))
