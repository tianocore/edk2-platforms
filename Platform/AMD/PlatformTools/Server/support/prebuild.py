"""

  Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

"""
import os
import sys
import overrides

def prebuild():
    print('PreBuild')
    print('Launched Python Version: {}.{}.{}'.format(
        sys.version_info.major,
        sys.version_info.minor,
        sys.version_info.micro))
    overrides.overrides()

def main():
    """!
    Execute PreBuild items

    Execute anything that needs to be completed before the EDKII BUILD
    """
    prebuild()

if __name__ == '__main__':
    main()
