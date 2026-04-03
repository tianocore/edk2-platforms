## @ PatchBinFv.py
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
from   ctypes import *

class FileChecker:
    def __init__(self):
        self.SyncSectionList = ["PatchPcd"]
        self.FvName = ""
        self.target = ""
        self.sourceRoot = ""
        self.reportFile = ""
        self.InfPcdList = []

    def GetSectionName(self, line):
        splitLine = line[1:-1].split(".")
        return splitLine[0]

    def IsSyncSection(self, line):
        name = self.GetSectionName(line)
        for sectionName in self.SyncSectionList:
            if sectionName == name:
                return True
        return False

    def PrintPcdList(self, pcdList):
        for pcd in pcdList:
            print(("PCD: {0} | {1} | {2} <== {3}({4})".format(*pcd)))

    def GetInfFileGuid(self, fileName):
        guid = ""
        try :
            file = open(fileName)
        except Exception:
            print("fail to open " + fileName)
            return
        try:
            while 1:
                line = file.readline()
                if not line:
                    break

                newline = line[:-1]

                if line[:11] == "  FILE_GUID":
                    splitLine = line.split("=")
                    templine = splitLine[1]
                    guid = templine[1:1+36]
        finally:
            file.close()
        return guid

    def ParseInfFile(self, fileName):
        SyncToDest = False
        try :
            file = open(fileName)
        except Exception:
            print("fail to open " + fileName)
            return
        try:
            while 1:
                line = file.readline()
                if not line:
                    break

                newline = line[:-1]

                if line[0] == "#":
                    continue


                if line[0] == "[":
                    SyncToDest = self.IsSyncSection(line)
                    PatchOffset = False

                if (self.GetSectionName(line) == "PatchPcd"):
                    PatchOffset = True
                    continue

                if SyncToDest == True :
                    line = line.strip()
                    if line == "":
                        continue
                    if line[0] == "#":
                        continue

                    splitLine = line.split(" ")
                    line = splitLine[0]

                    splitLine = line.split("|")

                    self.InfPcdList.append([splitLine[0], splitLine[1], splitLine[2], "", ""])

        finally:
            file.close()
        return

    def ProcessFvInf(self, fvName):
        sourceFileName = os.path.join(self.sourceRoot,fvName,self.target,fvName+".inf")
        print("\nprocessing - " + sourceFileName)
        fileGuid = self.GetInfFileGuid (sourceFileName)
        print("FV NAME GUID - " + fileGuid)

        self.InfPcdList = []
        self.ParseInfFile(sourceFileName)

        self.InfPcdList.sort()

        #self.PrintPcdList(self.InfPcdList)

        try :
            file = open(self.reportFile)
        except Exception:
            print("fail to open " + self.reportFile)
            return
        try:
            for pcd in self.InfPcdList:
                file.seek(0)
                print("checking - " + pcd[0])
                ValuePair = self.GetPcdFromReport (file, pcd[0])
                pcd[3] = ValuePair[0]
                pcd[4] = ValuePair[1]
        finally:
            file.close()

        self.PrintPcdList(self.InfPcdList)

    def PatchFv(self, fvName):
        sourceFileName = os.path.join(self.sourceRoot,fvName,self.target,fvName+".Fv")
        print("patching - " + sourceFileName)

        try :
            file = open(sourceFileName, "rb")
        except Exception:
            print("fail to open " + sourceFileName)
            return
        try:
            buffer = file.read()
            data = bytearray(buffer)
            file.close()

            for pcd in self.InfPcdList:
                offset = int(pcd[2], 16)
                if (pcd[4] == "BOOLEAN") or (pcd[4] == "UINT8"):
                    b = struct.pack("B", int(pcd[3],16))
                    print("  [" + hex(offset) + "] " + bytes(data[offset:offset+1]).hex() + " <= " + bytes(b).hex())
                    data[offset:offset+1] = b
                elif (pcd[4] == "UINT16"):
                    h = struct.pack("H", int(pcd[3],16))
                    print("  [" + hex(offset) + "] " + bytes(data[offset:offset+2]).hex() + " <= " + bytes(h).hex())
                    data[offset:offset+2] = h
                elif (pcd[4] == "UINT32"):
                    l = struct.pack("I", int(pcd[3],16))
                    print("  [" + hex(offset) + "] " + bytes(data[offset:offset+4]).hex() + " <= " + bytes(l).hex())
                    data[offset:offset+4] = l
                elif (pcd[4] == "UINT64"):
                    q = struct.pack("Q", int(pcd[3],16))
                    print("  [" + hex(offset) + "] " + bytes(data[offset:offset+8]).hex() + " <= " + bytes(q).hex())
                    data[offset:offset+8] = q
            file = open(sourceFileName, "wb")
            file.write(data)
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

            if (newline == TargetPkg):
                FoundPkg = True
                continue

            if (newline == "") or ((newline[0] != " ") and (newline[0] != "0")):
                FoundPkg = False

            if (FoundPkg == True) :
                newline = newline.strip()
                splitLine = newline.split(" ", 2)
                if (splitLine[0] == "*F") or (splitLine[0] == "*P"):
                    if (splitLine[1] == TargetPcd):
                        print("found - " + TargetPkg + "." + TargetPcd)

                        splitLine = splitLine[2].strip()[1:].strip().split(" ", 1)
                        if (splitLine[0] == "FIXED") or (splitLine[0] == "PATCH"):
                            SplitLine = splitLine[1].strip()[1:].split(")", 1)
                            Type = SplitLine[0]
                            Value = SplitLine[1].strip()[1:].strip().split()[0]
                            print("  Type - (" + Type + "), Value - (" + Value + ")")
                            return [Value, Type]
        return ["", ""]

def main():
    global FileChecker

    fileChecker = FileChecker()

    if (len(sys.argv) != 5) :
        print("usage: PatchBinFv <Target> <SourceRoot> <ReportFile> <FvName>")
        return 0

    fileChecker.target = sys.argv[1]
    fileChecker.sourceRoot = sys.argv[2]
    fileChecker.reportFile = sys.argv[3]
    fileChecker.FvName = sys.argv[4]

    fileChecker.ProcessFvInf (fileChecker.FvName)
    fileChecker.PatchFv (fileChecker.FvName)

if __name__ == '__main__':
    sys.exit(main())
