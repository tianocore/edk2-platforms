#!/usr/bin/env python3
# -*- coding: UTF-8 -*-
#
#  Copyright (c) 2025 Intel Corporation. All rights reserved. <BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
import os
import platform
import shutil
import glob
import re

script_file = os.path.abspath(__file__)
ROOT_WORK_SPACE = os.path.abspath(os.path.join(os.path.dirname(script_file), os.path.pardir,  os.path.pardir, os.path.pardir, os.path.pardir))


CUR_WORK_SPACE = os.path.abspath(os.getcwd())
work_space_folder = os.path.basename (CUR_WORK_SPACE)
work_space_path=os.path.abspath(os.path.join(ROOT_WORK_SPACE, work_space_folder))

open_bootloader_root="Platform/Intel"

if work_space_path not in CUR_WORK_SPACE:
    print("Error: Please run this script under Edk2Platforms root folder!")
    os._exit(-1)

edk2_path = os.path.join(ROOT_WORK_SPACE,  'Edk2')
if not os.path.exists(edk2_path):
    print ("Please clone Edk2 repository under {}".format( os.path.join(ROOT_WORK_SPACE)))
    os._exit(-1)

open_bootloader_smm_ext_build_folder = os.path.join (work_space_path, "Build", "OpenBootloaderSmmPkg")

cmd_fsp_extenstion_build_win = '''
call {}
set WORKSPACE={}
pushd . && cd {}\BaseTools && nmake && popd
set PACKAGES_PATH=%WORKSPACE%;%WORKSPACE%\..\Intel;%WORKSPACE%\Silicon\Intel;%WORKSPACE%\..\Edk2;%WORKSPACE%\Platform\Intel;%WORKSPACE%\Platform\Intel\OpenBootloaderSmmPkg;%WORKSPACE%\Platform\Intel\MinPlatformPkg
build -a X64 -p  {}/OpenBootloaderSmmPkg/OpenBootloaderSmmPkg.dsc
'''.format(os.path.abspath(os.path.join(edk2_path, "edksetup.bat")), work_space_path, edk2_path, open_bootloader_root)

#
###
cmd_openbootloader_extenstion_build_linux = '''
pushd . && cd {} && . edksetup.sh && popd
export WORKSPACE={}
make -C {}/BaseTools
'''.format(edk2_path, work_space_path, edk2_path)
cmd_openbootloader_extenstion_build_linux +='''
 export PACKAGES_PATH=${WORKSPACE}:${WORKSPACE}/../Intel:${WORKSPACE}/Silicon/Intel/:${WORKSPACE}/../Edk2:${WORKSPACE}/../Edk2Platforms/Platform/Intel:${WORKSPACE}/Platform/Intel/OpenBootloaderSmmPkg:${WORKSPACE}/Platform/Intel/MinPlatformPkg
make -C ${WORKSPACE}/../Edk2/BaseTools/Source/C/
'''
cmd_openbootloader_extenstion_build_linux +=\
'build -a X64 -p {}/OpenBootloaderSmmPkg/OpenBootloaderSmmPkg.dsc -t GCC5'.format(open_bootloader_root)

##
class OpenBootloaderSmmPkg (object):
    def __init__(self):
        self.work_space_path = work_space_path
        self.build_folder = open_bootloader_smm_ext_build_folder
        if not os.path.exists (open_bootloader_smm_ext_build_folder):
            os.makedirs (open_bootloader_smm_ext_build_folder)
    def gen_build_script(self):
        if platform.system() == 'Windows':
            cmd = cmd_fsp_extenstion_build_win
            script_file = os.path.join(self.build_folder, 'OpenBootLoaderSmmExtensionBuild.bat')
        else:
            conf_path = os.path.join(ROOT_WORK_SPACE, 'Edk2', 'Conf')
            if not os.path.exists(conf_path):
                os.makedirs(conf_path)
            script_file = os.path.join(self.build_folder, 'OpenBootLoaderSmmExtensionBuild.sh')
            cmd = cmd_openbootloader_extenstion_build_linux

        with open(script_file, 'w') as f:
            f.write(cmd)
        os.chmod(script_file, 0o755)
        return script_file

    def build_open_boot_loader(self):
        script_file = self.gen_build_script()
        src_fsp_ext_fd = ""
        if platform.system() == 'Windows':
            os.system(script_file)

            build_fd_pattern = os.path.join(self.build_folder, 'DEBUG_VS*', 'FV', 'OPENBLSMM.fd')
            for f in glob.glob(build_fd_pattern, recursive=True):
                src_fsp_ext_fd = os.path.abspath(f)
                break
        else:
            os.system('bash {}'.format(script_file))
            src_fsp_ext_fd = os.path.join(work_space_path, 'Build', 'OpenBootloaderSmmPkg','DEBUG_GCC5', 'FV', 'OPENBLSMM.fd')
        dst_fsp_ext_fd = os.path.join(work_space_path, 'OpenBlSmm.fd')
        if os.path.exists(dst_fsp_ext_fd):
            os.remove(dst_fsp_ext_fd)
        if  os.path.exists(src_fsp_ext_fd):
            shutil.copyfile(src_fsp_ext_fd, dst_fsp_ext_fd)

def main():
    open_bootloader = OpenBootloaderSmmPkg()
    open_bootloader.build_open_boot_loader()

if __name__ == '__main__':
    main()
