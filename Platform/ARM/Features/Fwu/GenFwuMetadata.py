## @file
# Generating Firmware Medata Data version 1 based on sepcification:
#    Platform Security Firmware Update for A-profile 1.0:
#        https://developer.arm.com/documentation/den0118/latest
#
# Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#  @par Reference(s):
#  - Platform Security Firmware Update for the A-profile Specification 1.0
#    (https://developer.arm.com/documentation/den0118/latest)
#
#  @par Glossary:
#    - FW          - Firmware
#    - FWU         - Firmware Update
#    - PSA         - Platform Security update for the A-profile specification
#    - IMG         - Image
#    - MM          - Management Mode
#    - PROP        - Property
#    - Bkup, Bkp   - Backup
#

import os
import sys
import argparse
import glob
import shutil
import struct
import uuid
import binascii
import zlib

#
# Global for help information
#
__prog__    = 'GenFwuMetadata.py'
__version__ = '0.1'
__description__ = 'Generate Firwmare update Metadata.\n'

class FwuImageBankInfoV2:
    #
    # struct fwu_image_bank_info_v2 {
    #    uuid_t img_guid;
    #    uint32_t accepted;
    #    uint8_t reserved_4[4];
    # }
    #
    _StructFormat = "<16sII"
    _StructSize = struct.calcsize(_StructFormat)

    def __init__(self, ImageGuid=None):
        self.ImageGuid = ImageGuid
        self.Accepted = 1
        self.Reserved4 = 0x00000000

    def Encode(self):
        FwuImageBankInfo = struct.pack(
                FwuImageBankInfoV2._StructFormat,
                self.ImageGuid.bytes_le,
                self.Accepted,
                self.Reserved4)

        return FwuImageBankInfo

    def Decode(self, Buffer):
        if len(Buffer) != FwuImageBankInfoV2._StructSize:
            raise ValueError

        (ImageGuid, Accepted, Reserved4) = \
                struct.unpack(
                        FwuImageBankInfoV2._StructFormat,
                        Buffer[0:FwuImageBankInfoV2._StructSize])

        if Reserved4 != 0x00000000:
            raise ValueError

        self.ImageGuid = uuid.UUID(bytes_le=ImageGuid)
        self.Accepted = Accepted

    def Dump(self):
        print("\t\t\timg_guid: {Guid}".format(Guid=str(self.ImageGuid)))
        print("\t\t\taccept: {Accpeted:d}".format(Accpeted=self.Accepted))
        print("\t\t\treserved_4: 0x{Reserved:08x}".format(Reserved=self.Reserved4))

class FwuImageEntryV2:
    #
    # struct fwu_image_entry_v2 {
    #    uuid_t img_type_guid;
    #    uuid_t location_guid;
    #    struct fwu_image_bank_info_v2 img_bank_info[];
    # }
    #
    _StructFormat = "<16s16s"
    _StructSize = struct.calcsize(_StructFormat)

    def __init__(self, ImageTypeGuid=None, LocationGuid=None, ImageBankInfoBuffer='b'):
        self.ImageTypeGuid = ImageTypeGuid
        self.LocationGuid = LocationGuid
        self.FwuImageBankInfo = ImageBankInfoBuffer

    def Encode(self):
        FwuImageEntry = struct.pack(
                FwuImageEntryV2._StructFormat,
                self.ImageTypeGuid.bytes_le,
                self.LocationGuid.bytes_le)

        return FwuImageEntry + self.FwuImageBankInfo

    def Decode(self, Buffer, NumBanks):
        EntrySize = FwuImageEntryV2._StructSize + (FwuImageBankInfoV2._StructSize * NumBanks)

        if len(Buffer) != EntrySize:
            raise ValueError

        (ImageTypeGuid, LocationGuid) = \
                struct.unpack(
                        FwuImageEntryV2._StructFormat,
                        Buffer[0:FwuImageEntryV2._StructSize])

        self.ImageTypeGuid = uuid.UUID(bytes_le=ImageTypeGuid)
        self.LocationGuid = uuid.UUID(bytes_le=LocationGuid)
        self.FwuImageBankInfo = Buffer[FwuImageEntryV2._StructSize:EntrySize]

    def Dump(self, NumBanks):
        print("\t\timg_type_guid: {Guid}".format(Guid=str(self.ImageTypeGuid)))
        print("\t\tlocation_guid: {Guid}".format(Guid=str(self.LocationGuid)))

        for Index in range(NumBanks):
            print("\t\tBank[{bank:d}]:".format(bank=Index))
            FwuImageBankInfo = FwuImageBankInfoV2()
            FwuImageBankInfo.Decode(
                    self.FwuImageBankInfo[
                        Index * FwuImageBankInfoV2._StructSize:
                        (Index + 1) * FwuImageBankInfoV2._StructSize
                        ])
            FwuImageBankInfo.Dump()

class FwuFwStoreDescV2:
    #
    # struct fwu_fw_store_desc_v2 {
    #     uint8_t num_banks;
    #     uint8_t reserved;
    #     uint16_t num_images;
    #     uint16_t img_entry_size;
    #     uint16_t bank_info_entry_size;
    #     struct fwu_image_entry_v2 img_entry[];
    # }
    #
    _StructFormat = "<BBHHH"
    _StructSize = struct.calcsize(_StructFormat)

    def __init__(self, NumBanks=2, ImagePerBank=1, ImageEntryBuffer='b'):
        self.NumBanks = NumBanks
        self.Reserved = 0x00
        self.NumImages = ImagePerBank
        self.ImageEntrySize = FwuImageEntryV2._StructSize +(FwuImageBankInfoV2._StructSize * NumBanks)
        self.BankInfoEntrySize = FwuImageBankInfoV2._StructSize
        self.ImageEntry = ImageEntryBuffer

    def Encode(self):
        FwuFwStoreDesc = struct.pack(
                FwuFwStoreDescV2._StructFormat,
                self.NumBanks,
                self.Reserved,
                self.NumImages,
                self.ImageEntrySize,
                self.BankInfoEntrySize)

        return FwuFwStoreDesc + self.ImageEntry

    def Decode(self, Buffer):
        if len(Buffer) < FwuFwStoreDescV2._StructSize:
            return ValueError

        (NumBanks, Reserved, NumImages, ImageEntrySize, BankInfoEntrySize) = \
                struct.unpack(
                        FwuFwStoreDescV2._StructFormat,
                        Buffer[0:FwuFwStoreDescV2._StructSize])

        DescSize = FwuFwStoreDescV2._StructSize + \
                (NumImages * (FwuImageEntryV2._StructSize + \
                (FwuImageBankInfoV2._StructSize * NumBanks)))

        if len(Buffer) != DescSize:
            raise ValueError

        self.NumBanks = NumBanks
        self.Reserved = Reserved
        self.NumImages = NumImages
        self.ImageEntrySize = ImageEntrySize
        self.BankInfoEntrySize = BankInfoEntrySize
        self.ImageEntry = Buffer[FwuFwStoreDescV2._StructSize:DescSize]

    def Dump(self):
        print("\tnum_banks: {NumBanks:d}".format(NumBanks=self.NumBanks))
        print("\treserved: 0x{Reserved:02x}".format(Reserved=self.Reserved))
        print("\tnum_images: {NumImages:d}".format(NumImages=self.NumImages))
        print("\timg_entry_size: {ImageEntrySize}".format(ImageEntrySize=str(self.ImageEntrySize)))
        print("\tbank_info_entry_size: {BankInfoEntrySize}".format(BankInfoEntrySize=str(self.BankInfoEntrySize)))

        print("\tIMAGE_ENTRIES:")
        for Index in range(self.NumImages):
            print("\tIMAGE[{image:d}]:".format(image=Index))
            FwuImageEntry = FwuImageEntryV2()
            FwuImageEntry.Decode(
                    self.ImageEntry[
                        Index * self.ImageEntrySize:
                        (Index + 1) * self.ImageEntrySize
                        ], self.NumBanks)
            FwuImageEntry.Dump(self.NumBanks)

class FwuMetadataV2:
    #
    # struct fwu_metadata_v2 {
    #    uint32_t crc_32;
    #    uint32_t version;
    #    uint32_t active_index;
    #    uint32_t previous_active_index;
    #    uint32_t metadata_size;
    #    uint16_t descriptor_offset;
    #    uint16_t reserved_2;
    #    uint8_t  bank_state[4];
    #    uint32_t reserved_4;
    #    struct fwu_fw_store_desc_v2 fw_store_desc;
    # }
    #
    _StructFormat = "<IIIIIHH4sI"
    _StructSize = struct.calcsize(_StructFormat)

    def __init__(self, FwuFwStoreDescBuffer=b''):
        self.Crc32 = 0x00
        self.Version = 0x00000002
        self.MetadataSize = 0x00;
        self.ActiveIndex = 0x00
        self.PreviousActiveIndex = 0x01
        self.DescriptorOffset = 0x20;
        self.Reserved2 = 0x0000;
        self.BankState = [ 0xFF, 0xFF, 0xFF, 0xFF ]
        self.Reserved4 = 0x00000000
        self.FwuFwStoreDesc = FwuFwStoreDescBuffer

    def Encode(self, NumBanks=2):
        for Index in range(NumBanks):
            self.BankState[Index] = 0xFC;

        self.MetadataSize = FwuMetadataV2._StructSize + len(self.FwuFwStoreDesc)
        BankState = struct.pack(
                "<BBBB",
                self.BankState[0],
                self.BankState[1],
                self.BankState[2],
                self.BankState[3])

        FwuMetadata = struct.pack(
                FwuMetadataV2._StructFormat,
                self.Crc32,
                self.Version,
                self.ActiveIndex,
                self.PreviousActiveIndex,
                self.MetadataSize,
                self.DescriptorOffset,
                self.Reserved2,
                BankState,
                self.Reserved4)

        TmpBuf = FwuMetadata + self.FwuFwStoreDesc
        TmpBuf = TmpBuf[4:]

        self.Crc32 = (zlib.crc32(TmpBuf))

        FwuMetadata = struct.pack(
                FwuMetadataV2._StructFormat,
                self.Crc32,
                self.Version,
                self.ActiveIndex,
                self.PreviousActiveIndex,
                self.MetadataSize,
                self.DescriptorOffset,
                self.Reserved2,
                BankState,
                self.Reserved4)

        return FwuMetadata + self.FwuFwStoreDesc

    def Decode(self, Buffer, ImagePerBanks, NumBanks):
        TotalSize = FwuMetadataV2._StructSize + \
                FwuFwStoreDescV2._StructSize + \
                (ImagePerBanks * \
                (FwuImageEntryV2._StructSize + \
                (FwuImageBankInfoV2._StructSize * NumBanks)))

        if len(Buffer) != TotalSize:
            raise ValueError

        (Crc32, Version, ActiveIndex, PreviousActiveIndex, \
                MetadataSize, DescriptorOffset, Reserved2, \
                BankState, Reserved4) = struct.unpack(
                        FwuMetadataV2._StructFormat,
                        Buffer[0:FwuMetadataV2._StructSize])

        self.Crc32 = Crc32
        self.Version = Version
        self.ActiveIndex = ActiveIndex
        self.PreviousActiveIndex = PreviousActiveIndex
        self.MetadataSize = MetadataSize
        self.DescriptorOffset = DescriptorOffset
        self.Reserved2 = Reserved2
        self.BankState = BankState
        self.Reserved4 = Reserved4
        self.FwuFwStoreDesc = Buffer[FwuMetadataV2._StructSize:]

    def Dump(self):
        print("FWU METADATA HEADER:")
        print("\tcrc32: 0x{Crc32:04x}".format(Crc32=self.Crc32))
        print("\tversion: 0x{Version:04x}".format(Version=self.Version))
        print("\tactive_index: {ActiveIndex:d}".format(ActiveIndex=self.ActiveIndex))
        print("\tprevious_active_index: {PIndex:d}".format(PIndex=self.PreviousActiveIndex))
        print("\tmetadata_size: {MetadataSize:d}".format(MetadataSize=self.MetadataSize))
        print("\tdescriptor_offset: {DescOff:d}".format(DescOff=self.DescriptorOffset))
        print("\treserved: 0x{Reserved2:04x}".format(Reserved2=self.Reserved2))
        for Index in range (4):
            print("\tbank_state[{Idx:d}]: 0x{State:02x}".format(
                Idx=Index, State=self.BankState[Index]))
        print("\treserved: 0x{Reserved4:08x}".format(Reserved4=self.Reserved4))

        print("FIRMWARE STORE DESCRIPTION:")
        FwuFwStoreDesc = FwuFwStoreDescV2()
        FwuFwStoreDesc.Decode(self.FwuFwStoreDesc)
        FwuFwStoreDesc.Dump()

if __name__ == "__main__":
    def convert_arg_line_to_args(arg_line):
        for arg in arg_line.split():
            if not arg.split():
                continue
            yield arg

    def ValidateUint(args):
        try:
            Value = int (args, 0)
        except:
            Msg = "{Arg} isn't integer format".format(Arg=args)
            raise argparse.ArgumentTypeError(Msg)

        if Value < 0:
            Msg = "{Arg} is negative ".format(Arg=args)
            raise argparse.ArgumentTypeError(Msg)

        return Value


    def ValidateGuidList(args):
        GuidList = []

        for arg in args.split(','):
            try:
                Value = uuid.UUID(arg)
            except:
                Msg = "{Arg} isn't valid GUID format".format(Arg=arg)
                raise argparse.ArgumentTypeError(Msg)

            GuidList.append(Value)

        return GuidList


    # create command line argument parser object.
    parser = argparse.ArgumentParser (
                        prog = __prog__,
                        description = __description__,
                        conflict_handler = 'resolve',
                        fromfile_prefix_chars = '@'
                        )
    parser.convert_arg_line_to_args = convert_arg_line_to_args

    #
    # add input and output file arguments
    #
    parser.add_argument("InputFile", type = argparse.FileType('rb'), nargs='?',
                        metavar="filename",
                        help="input filename. used only when dump fwu_metadata.")
    parser.add_argument("-o", "--output", dest="OutputFile", type = argparse.FileType('wb'),
                        metavar="filename",
                        help="output filename.")

    #
    # add group for -c and -d options taht are mutually exclusive and required.
    #
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("-d", "--dump", dest="DumpMode", action="store_true",
                        help="Dump firmware metadata given by [InputFile]")
    group.add_argument("-g", "--generate", dest="GenMode", action="store_true",
                        help="Generate firmware metadata")

    #
    # add optional arguments.
    #
    parser.add_argument("-v", "--version", dest="Version", type=ValidateUint,
                        default=2, metavar="number",
                        help="Metadata Version (default: 2).")
    parser.add_argument("-b", "--banks", dest="NumBanks", type=ValidateUint,
                        default=2, metavar="number",
                        help="Number of banks (default: 2).")
    parser.add_argument("-p", "--img_per_bank", dest="ImagePerBank", type=ValidateUint,
                        default=1, metavar="number",
                        help="Number of image per bank (default: 1).")
    parser.add_argument("-i", "--img_type_guid", dest="ImgTypeGuidList", type=ValidateGuidList,
                        default=[], metavar="guid_list",
                        help="Image Type Guid list separated by ','. if not, Generate based on image per bank.")
    parser.add_argument("-l", "--location_guid", dest="LocationGuidList", type=ValidateGuidList,
                        default=[], metavar="guid_list",
                        help="Location Guid list separated by ','. if not, Generate based on image per bank.")

    args = parser.parse_args()

    if args.DumpMode:
        if args.InputFile is None:
            print("ERROR: No Input file.");
            sys.exit(2)

        Buffer = args.InputFile.read()
        args.InputFile.close()

        if (args.Version == 1):
            print("ERROR: Metadata v1 isn't supported.");
            sys.exit(2)
        else:
            FwuMetadata = FwuMetadataV2()
            FwuMetadata.Decode(Buffer, args.ImagePerBank, args.NumBanks)
            FwuMetadata.Dump()

        sys.exit(0)

    #
    # generate firmware update metadata.
    #
    if len(args.ImgTypeGuidList) == 0:
        for Index in range(args.ImagePerBank):
            args.ImgTypeGuidList.append(uuid.uuid4())

    if len(args.LocationGuidList) == 0:
        for Index in range(args.ImagePerBank):
            args.LocationGuidList.append(uuid.uuid4())

    if (len(args.ImgTypeGuidList) < args.ImagePerBank or
            len(args.LocationGuidList) < args.ImagePerBank):
        print("ERROR: Please input proper list: img_type_guid or location_guid")
        sys.exit(22)


    if (args.Version == 1):
        print("ERROR: Metadata v1 isn't supported.");
        sys.exit(2)
    else:
        ImgEntryBuffer = b''

        for EntryIndex in range(args.ImagePerBank):
            ImageTypeGuid = args.ImgTypeGuidList[EntryIndex]
            LocationGuid = args.LocationGuidList[EntryIndex]
            ImageBankInfoBuffer = b''

            for InfoIndex in range(args.NumBanks):
                FwuImageBankInfo = FwuImageBankInfoV2(ImageTypeGuid)
                ImageBankInfoBuffer += FwuImageBankInfo.Encode()

            FwuImageEntry = FwuImageEntryV2(ImageTypeGuid, LocationGuid, ImageBankInfoBuffer)
            ImgEntryBuffer += FwuImageEntry.Encode()

        FwuFwStoreDesc = FwuFwStoreDescV2(args.NumBanks, args.ImagePerBank, ImgEntryBuffer)
        FwStoreDescBuffer = FwuFwStoreDesc.Encode()

        FwuMetadata = FwuMetadataV2(FwStoreDescBuffer)
        MetadataBuffer = FwuMetadata.Encode(args.NumBanks)

        print("Generated Firmware Metadata:")
        FwuMetadata.Dump()

    if args.OutputFile is not None:
        args.OutputFile.write(MetadataBuffer)
        args.OutputFile.close()

    sys.exit(0)
