"""
*******************************************************************************
 Copyright (C) 2022-2025 Advanced Micro Devices, Inc. All rights reserved.
 SPDX-License-Identifier: BSD-2-Clause-Patent

*******************************************************************************
"""

import os

def projectprebuild():
    # These will be set elsewhere with dbuild.py and can be removed when it
    # replaces dbuild.cmd
    os.environ["SOC_FAMILY"] = os.environ.get("SOC_FAMILY", "0x19")
    os.environ["SOC_SKU"] = os.environ.get("SOC_SKU", "RS")
    os.environ["SOC2"] = os.environ.get("SOC2", "STONES")
    os.environ["SOCKET"] = os.environ.get("SOCKET", "SP6")
