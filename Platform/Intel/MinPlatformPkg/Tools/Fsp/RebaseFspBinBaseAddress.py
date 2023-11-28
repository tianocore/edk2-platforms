## @ RebaseFspBinBaseAddress.py
#
# Copyright (c) 2019 - 2023, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

import os
import sys
import re
import subprocess
import argparse

#
# Globals for help information
#
__description__ = 'Rebase Fsp Bin Base and Split Fsp components.\n'
__prog__        = sys.argv[0]

Parser = argparse.ArgumentParser(
                        prog = __prog__,\
                        description = __description__
                        )

Parser.add_argument("-fm", "--flashMap", dest = 'flashMap', help = 'FlashMap file path')
Parser.add_argument("-p", "--fspBinPath", dest = 'fspBinPath', help = 'fsp Bin path')
Parser.add_argument("-fsp", "--fspBinFile", dest = 'fspBinFile', required = True, help = 'Fsp.fd file name')
Parser.add_argument("-po", "--padOffset", dest = 'padOffset', help = 'pad offset for Fsp-S Base Address')
Parser.add_argument("-s", "--splitFspBin", dest = 'splitFspBin', help = 'SplitFspBin.py tool path')
Parser.add_argument("-sr", "--skipRebase", dest = 'skipRebase',action="store_true", \
                    help = "Whether skip FSP rebase, the value is True or False")
Args, Remaining  = Parser.parse_known_args()

if Args.skipRebase == False:
  if Args.flashMap == None or Args.fspBinPath == None or Args.padOffset == None:
    print ("RebaseFspBinBaseAddress.py - Invalid argements.")
    print (" When skipRebase is false, It's necessary to input flashMap, fspBinPkg and padOffset to rebase.")
    exit(1)

fspBinPath = Args.fspBinFile

splitFspBinPath   = os.path.join("edk2","IntelFsp2Pkg","Tools","SplitFspBin.py")
if Args.splitFspBin != None:
  splitFspBinPath = Args.splitFspBin

fspBinPath        = Args.fspBinPath
fspBinFile        = Args.fspBinFile

#
# Make sure argument passed or valid
#fspBinFilePath = fspBinPath + os.sep + fspBinFile
if not os.path.exists(fspBinFilePath):
  print ("WARNING!  " + str(fspBinFilePath) + " is not found.")
  exit(1)
if not os.path.exists(splitFspBinPath):
  print ("WARNING!  " + str(splitFspBinPath) + " is not found.")
  exit(1)

pythontool = 'python'
if 'PYTHON_HOME' in os.environ:
    pythontool = os.environ['PYTHON_HOME'] + os.sep + 'python'
else:
    pythontool = sys.executable

if Args.skipRebase == True:
  print("SKip FSP rebase")
  #
  # Split FSP bin to FSP-S/M/T segments
  #
  splitArguments = fspBinPath + os.sep + fspBinFile + " -o " + fspBinPath + " -n Fsp_Rebased.fd"
  os.system('"' + pythontool + '"' + " " + splitFspBinPath + " split -f" + splitArguments)
  exit(0)

fspBinFileRebased = "Fsp_Rebased.fd"
flashMapName      = Args.flashMap
fvOffset          = int(Args.padOffset, 16)

#
# Make sure argument passed or valid
#
if not os.path.exists(flashMapName):
  print ("WARNING!  " + str(flashMapName) + " is not found.")
  exit(1)

#
# Get the FSP-S / FSP-M-T FV Base Address from Flash Map
#
file = open (flashMapName, "r")
data = file.read ()

# Get the Flash Base Address
flashBase = int(data.split("FLASH_BASE")[1].split("=")[1].split()[0], 16)

# Based on Build Target, select the section in the FlashMap file
flashmap = data

# Get FSP-S & FSP-M & FSP-T offset & calculate the base
for line in flashmap.split("\n"):
  if "PcdFlashFvFspSOffset" in line:
    fspSBaseOffset = int(line.split("=")[1].split()[0], 16)
  if "PcdFlashFvFspMOffset" in line:
    fspMBaseOffset = int(line.split("=")[1].split()[0], 16)
  if "PcdFlashFvFspTOffset" in line:
    fspTBaseOffset = int(line.split("=")[1].split()[0], 16)
file.close()

#
# Get FSP-M Size, in order to calculate the FSP-T Base. Used SplitFspBin.py script
# to dump the header, and get the ImageSize in FSP-M section
#

Process = subprocess.Popen([pythontool, splitFspBinPath, "info","-f",fspBinFilePath], stdout=subprocess.PIPE)
Output = Process.communicate()[0]
FsptInfo = Output.rsplit(b"FSP_M", 1)
for line in FsptInfo[1].split(b"\n"):
  if b"ImageSize" in line:
    fspMSize = int(line.split(b"=")[1], 16)
    break

# Calculate FSP-S/M/T base address, to which re-base has to be done
fspSBaseAddress = flashBase + fspSBaseOffset + fvOffset
fspMBaseAddress = flashBase + fspMBaseOffset
fspTBaseAddress = flashBase + fspTBaseOffset

#
# Re-base FSP bin file to new address and save it as fspBinFileRebased using SplitFspBin.py
#
rebaseArguments = fspBinFilePath + " -c s m t -b " + str(hex(fspSBaseAddress).rstrip("L")) + " " + str(hex(fspMBaseAddress).rstrip("L")) + " " + str(hex(fspTBaseAddress).rstrip("L")) + " -o" + fspBinPath + " -n " + fspBinFileRebased
os.system('"' + pythontool + '"' + " " + splitFspBinPath + " rebase -f" + rebaseArguments)

#
# Split FSP bin to FSP-S/M/T segments
#
splitArguments = fspBinPath + os.sep + fspBinFileRebased + " -o " + fspBinPath + " -n Fsp_Rebased.fd"
os.system('"' + pythontool + '"' + " " + splitFspBinPath + " split -f" + splitArguments)

exit(0)
