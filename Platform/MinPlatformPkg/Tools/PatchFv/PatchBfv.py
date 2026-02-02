## @ PatchBfv.py
#
# Copyright (c) 2017 - 2023, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

import os
import re
import sys
import time
import shutil
import struct
from ctypes import *

class FileChecker:
    def __init__(self):
        self.fdName = ""
        self.reportFile = ""
        self.pcd = ["", "", ""]

    def PrintPcd(self):
        print("PCD: {0}|{1}({2})".format(*(self.pcd)))

    def ProcessReport(self):
        try :
            file = open(self.reportFile)
        except Exception:
            print("fail to open " + self.reportFile)
            return
        try:
            file.seek(0)
            print("checking - " + self.pcd[0])
            ValuePair = self.GetPcdFromReport (file, self.pcd[0])
            self.pcd[1] = ValuePair[0]
            self.pcd[2] = ValuePair[1]
        finally:
            file.close()

        self.PrintPcd()

    def PatchFd(self):
        fileName = self.fdName
        print("patching BFV - " + fileName)

        try :
            file = open(fileName, "rb")
        except Exception:
            print("fail to open " + fileName)
            return
        try:
            buffer = file.read()
            data = bytearray(buffer)
            file.close()

            offset = -4

            l = struct.pack("L", int(self.pcd[1],16))
            print("  [" + hex(offset) + "] " + bytes(data[-4:]).hex() + " <= " + bytes(l).hex())
            data[-4:] = l

            file = open(fileName, "wb")
            file.write(data[0:])
        finally:
            file.close()

    def GetPcdFromReport(self, file, pcd):
        FoundPkg = False
        pcdSplit = pcd.split(".")
        TargetPkg = pcdSplit[0]
        TargetPcd = pcdSplit[1]
        while 1:
            line = file.readline()
            if not line:
                break

            newline = line[:-1]

            if newline == TargetPkg:
                FoundPkg = True
                continue

            if newline == "" or (newline[0] != " " and newline[0] != "0"):
                FoundPkg = False

            if (FoundPkg == True) :
                newline = newline.strip()
                splitLine = newline.split(" ", 2)
                if splitLine[0] == "*F" or splitLine[0] == "*P":
                    if splitLine[1] == TargetPcd:
                        print("found - " + TargetPkg + "." + TargetPcd)

                        splitLine = splitLine[2].strip()[1:].strip().split(" ", 1)
                        if splitLine[0] == "FIXED" or splitLine[0] == "PATCH":
                            SplitLine = splitLine[1].strip()[1:].split(")", 1)
                            Type = SplitLine[0]
                            Value = SplitLine[1].strip()[1:].strip().split()[0]
                            print("  Type - (" + Type + "), Value - (" + Value + ")")
                            return [Value, Type]
        return ["", ""]

def main():
    global FileChecker

    fileChecker = FileChecker()

    if (len(sys.argv) != 4) :
        print("usage: PatchBfv <FdFile> <ReportFile> <BfvPcdName>")
        return 0

    fileChecker.fdName = sys.argv[1]
    fileChecker.reportFile = sys.argv[2]
    fileChecker.pcd[0] = sys.argv[3]

    fileChecker.ProcessReport ()
    fileChecker.PatchFd ()

if __name__ == '__main__':
    sys.exit(main())
