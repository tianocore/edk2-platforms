## @file
# This file contains the script to build UniversalPayload with platform payload
#
# Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import argparse
import subprocess
import os
import shutil
import sys
from   ctypes import *

sys.dont_write_bytecode = True

def RunCommand(cmd):
    print(cmd)
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,cwd=os.environ['WORKSPACE'])
    while True:
        line = p.stdout.readline()
        if not line:
            break
        print(line.strip().decode(errors='ignore'))

    p.communicate()
    if p.returncode != 0:
        print("- Failed - error happened when run command: %s"%cmd)
        raise Exception("ERROR: when run command: %s"%cmd)

def BuildUniversalPayload(Args, MacroList):
    BuildTarget  = Args.Target
    ToolChain    = Args.ToolChain
    ElfToolChain = 'CLANGDWARF'
    ObjCopyFlag  = "elf64-x86-64" if Args.Arch == 'X64' else "elf32-i386"

    #
    # Find universal UEFI payload build build script
    #
    Edk2PayloadBuildScript = os.path.normpath("UefiPayloadPkg/UniversalPayloadBuild.py")
    for package_path in os.environ['PACKAGES_PATH'].split(';'):
        if os.path.exists (os.path.join (package_path, Edk2PayloadBuildScript)):
            Edk2PayloadBuildScript = os.path.join (package_path, Edk2PayloadBuildScript)
            break
    if not os.path.exists (Edk2PayloadBuildScript):
        raise Exception("Could not find universal UEFI payload build script UniversalPayloadBuild.py")

    PlatformFvDscPath      = os.path.normpath("PlatformPayloadPkg/PlatformPayloadPkg.dsc")
    BuildDir               = os.path.join(os.environ['WORKSPACE'], os.path.normpath("Build/UefiPayloadPkgX64"))
    PlatformFvReportPath   = os.path.join(BuildDir, "PlatformPayloadReport.txt")
    UniversalUefiPld       = os.path.join(BuildDir, 'UniversalPayload.elf')
    PlatformFv             = os.path.join(os.environ['WORKSPACE'], os.path.normpath("Build/PlatformPayloadPkg"), f"{BuildTarget}_{ToolChain}", os.path.normpath("FV/PlatformPayload.Fv"))

    if "CLANG_BIN" in os.environ:
        LlvmObjcopyPath = os.path.join(os.environ["CLANG_BIN"], "llvm-objcopy")
    else:
        LlvmObjcopyPath = "llvm-objcopy"
    try:
        RunCommand('"%s" --version'%LlvmObjcopyPath)
    except:
        print("- Failed - Please check if LLVM is installed or if CLANG_BIN is set correctly")
        sys.exit(1)

    Defines = ""
    for key in MacroList:
        Defines +=" -D {0}={1}".format(key, MacroList[key])

    #
    # Building Universal Payload entry.
    #
    if not Args.Skip:
         BuildPayload = f"python {Edk2PayloadBuildScript} -b {BuildTarget} -t {ToolChain} -a {Args.Arch} {Defines}"
         RunCommand(BuildPayload)

    #
    # Building Platform Payload.
    #
    BuildPayload = f"build -p {PlatformFvDscPath} -b {BuildTarget} -a X64 -t {ToolChain} -y {PlatformFvReportPath}"
    BuildPayload += Defines
    RunCommand(BuildPayload)

    #
    # Copy the Platform Payload as a section in elf format Universal Payload.
    #
    remove_section = f'"{LlvmObjcopyPath}" -I {ObjCopyFlag} -O {ObjCopyFlag} --remove-section        .upld.platform_fv              {UniversalUefiPld}'
    add_section    = f'"{LlvmObjcopyPath}" -I {ObjCopyFlag} -O {ObjCopyFlag} --add-section           .upld.platform_fv={PlatformFv} {UniversalUefiPld}'
    set_section    = f'"{LlvmObjcopyPath}" -I {ObjCopyFlag} -O {ObjCopyFlag} --set-section-alignment .upld.platform_fv=16           {UniversalUefiPld}'
    RunCommand(remove_section)
    RunCommand(add_section)
    RunCommand(set_section)


def main():
    parser = argparse.ArgumentParser(description='Build Platform Payload FV and add it to Universal UEFI Payload')
    parser.add_argument('-t', '--ToolChain')
    parser.add_argument('-b', '--Target', default='DEBUG')
    parser.add_argument('-a', '--Arch', choices=['IA32', 'X64'], help='Specify the ARCH for payload entry module. Default build X64 image.', default ='X64')
    parser.add_argument("-D", "--Macro", action="append")
    parser.add_argument('-s', '--Skip',  action='store_true', help='Skip Universal UEFI payload build (just build platform FV and add to Universal UEFI payload.')
    MacroList = {}
    args = parser.parse_args()
    if args.Macro is not None:
        for Argument in args.Macro:
            if Argument.count('=') != 1:
                print("Unknown variable passed in: %s"%Argument)
                raise Exception("ERROR: Unknown variable passed in: %s"%Argument)
            tokens = Argument.strip().split('=')
            MacroList[tokens[0].upper()] = tokens[1]
    BuildUniversalPayload(args, MacroList)
    print ("Successfully build Universal Payload with platform FV")

if __name__ == '__main__':
    main()
