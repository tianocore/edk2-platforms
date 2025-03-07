## @file
# Python script generate bios version
#
# Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import os
import time

project_dsc = 'Project.dsc'
recovery_fv = 'RECOVERYFV.Fv'
firmware_version_string_PCD = 'gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareVersionString'
bios_version_signature = b'BIVS'
bios_version_offset = 0x40 # offset to end of file
build_date = time.strftime('%Y%m%d',time.localtime(time.time()))
build_time = time.strftime('%H%M%S',time.localtime(time.time()))

def get_BIOS_version(filename):
    version_str = ''

    if not os.access(filename, os.R_OK):
        print("Could not read file:{}".format(filename))
        return ''

    with open(filename) as myfile:
        for each in myfile.readlines():
            if firmware_version_string_PCD in each:
                version_str = each
                break

    if version_str == '':
        print("Could not find {} in file:{}".format(firmware_version_string_PCD, filename))
        return ''

    _, _, version_str = version_str.partition('|')
    version_str, _, _ = version_str.partition('|')
    _, version_str, _ = version_str.split('"')
    return version_str


def update_ROM_BIVS(filename, version_string:str):
    if not os.access(filename, os.R_OK + os.W_OK):
        print("Could not read/write files: {}".format(filename))
        status = 1
        return status

    with open(filename, 'r+b') as rom_file:
        rom_file.seek(0, os.SEEK_END)
        file_size = rom_file.tell()
        print('%s size %.1f MB' % (filename, file_size/(1024*1024.0)))
        offset = file_size - bios_version_offset

        location = 0
        rom_file.seek(offset, os.SEEK_SET)
        if (rom_file.read(1) == bios_version_signature[0].to_bytes(1,'little')) and (rom_file.read(1) == bios_version_signature[1].to_bytes(1,'little')) and \
           (rom_file.read(1) == bios_version_signature[2].to_bytes(1,'little')) and (rom_file.read(1) == bios_version_signature[3].to_bytes(1,'little')):
            location = rom_file.tell()
        else:
            offset = 0
            rom_file.seek(offset, os.SEEK_SET)
            while (not ((rom_file.read(1) == bios_version_signature[0].to_bytes(1,'little')) and (rom_file.read(1) == bios_version_signature[1].to_bytes(1,'little')) and \
                        (rom_file.read(1) == bios_version_signature[2].to_bytes(1,'little')) and (rom_file.read(1) == bios_version_signature[3].to_bytes(1,'little')))):
                offset = offset + 16
            else:
                location = rom_file.tell()

        if location == 0:
            print("Signaure is not found")
            status = 2
            return status

        rom_file.seek(location, os.SEEK_SET)
        rom_file.seek(1, os.SEEK_CUR)
        rom_file.write(version_string.encode('ascii'))
        rom_file.seek(1, os.SEEK_CUR)
        rom_file.write(build_date.encode('ascii'))
        rom_file.seek(1, os.SEEK_CUR)
        rom_file.write(build_time.encode('ascii'))
        return 0

def main():
    status = -1
    if ('WORKSPACE' in os.environ) and ('PROJECT_PKG' in os.environ) and ('FD_NAME' in os.environ) and ('BIOS_FV_PATH' in os.environ):
        project_dsc_file = os.environ['WORKSPACE'] + os.sep + os.environ['PROJECT_PKG'] + os.sep + project_dsc
        bios_rom = os.environ['BIOS_FV_PATH'] + os.sep + (os.environ['FD_NAME'] + 'SPHPei').upper()+'.fd'
        recovery_fv_file = os.environ['BIOS_FV_PATH'] + os.sep + recovery_fv

        FirmwareVersionString = get_BIOS_version(project_dsc_file)
        if FirmwareVersionString == '':
            print("Could not find {} in file:{}".format(firmware_version_string_PCD, project_dsc_file))
            status = -4
            return status
        else:
            print('FirmwareVersionString is {}'.format(FirmwareVersionString))

        status = update_ROM_BIVS(recovery_fv_file, FirmwareVersionString)
        if  status != 0:
            return status
        else:
            print('BIVS updated for {}'.format(recovery_fv_file))

        status = update_ROM_BIVS(bios_rom, FirmwareVersionString)
        if  status != 0:
            return status
        else:
            print('BIVS updated for {}'.format(bios_rom))

    else:
        print("Unknow environment variables: WORKSPACE, PROJECT_PKG, FD_NAME and BIOS_FV_PATH!")
        status = 1
        return status

if __name__ == '__main__':
    main()
