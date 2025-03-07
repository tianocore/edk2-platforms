"""

  Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

"""
import os
import sys
import overrides

def postbuild():
    print('PostBuild')
    print('Launched Python Version: {}.{}.{}'.format(
        sys.version_info.major,
        sys.version_info.minor,
        sys.version_info.micro))
    overrides.restore_overrides()

def main():
    """!
    Execute PostBuild items

    Execute anything that needs to be completed after the EDKII BUILD
    """
    postbuild()

if __name__ == '__main__':
    main()
