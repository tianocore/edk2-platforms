"""

  Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

"""
import os
import sys
import subprocess
import argparse

def parseargs():
    parser = argparse.ArgumentParser(
        description = 'Launches the proper prebuild.py or postbuild.py script'
    )
    parser.add_argument(
        'command',
        choices=['prebuild', 'postbuild'],
        help='Select between prebuild and postbuild execution'
    )

    args = parser.parse_known_args()
    return args

def main():
    """!
    Find proper Python interpreter and launch post build processes
    """
    print('PreBuild-PostBuild Launcher')
    lmajor, lminor, lmicro, releaselevel, serial = sys.version_info
    print('Launched Python Version: {}.{}.{}'.format( lmajor, lminor, lmicro))

    args, unknown_args = parseargs()
    module = __import__(args.command)
    func = getattr(module, args.command)

    if 'PYTHON_HOME' in os.environ:
        python_home = os.environ['PYTHON_HOME'].strip('"').strip("'")
        python_home = os.path.normpath(python_home)
        if (
                (os.path.normpath(os.path.dirname(sys.executable))
                 == python_home)
                and
                (lmajor >= 3 and lminor >= 6)
        ):
            exit(func())
        else:
            if sys.platform.startswith("linux"):
                python_exe = "python3"
                workspace ="WORKSPACE"
            elif sys.platform.startswith("win"):
                python_exe = "python.exe"
                workspace ="Workspace"
            python = os.path.normpath(os.path.join(python_home, python_exe))

        if workspace not in os.environ:
            print("edk2 \"" + workspace + "\" environment variable is not set! Build process breaks...")
            sys.exit(1)
        else:
            edk2Workspace = os.environ.get(workspace)
            prePostScriptPath = os.path.join (
                    edk2Workspace,
                    "edk2-platforms",
                    "Platform",
                    "AMD",
                    "PlatformTools",
                    "Server",
                    "support"
                )

            sys.stdout.flush()
            version = str(subprocess.check_output([python, '--version']))
            # Split apart Python version response
            name, version = version.split()
            major, minor, micro = version.split('.')
            if int(major) >= 3 and int(minor) >= 6:
                print('Launching using "{}"'.format(python))
                sys.stdout.flush()
                command = [
                    python,
                    os.path.join(
                        prePostScriptPath,
                        '{}.py'.format(args.command),
                    )
                ]
                command.extend(sys.argv)
                exit(subprocess.check_call(command))
            else:
                print('ERROR: PYTHON_HOME not pointing to version 3.6 or later')
                exit(1)
    else:
        if lmajor >= 3 and lminor >= 6:
            print('Continuing with Launched Python')
            exit(func())
        else:
            print('Python 3.6 or greater required, exiting')
    exit(1)

if __name__ == '__main__':
    main()



