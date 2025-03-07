## @file
# Python script update PSP L1 directory checksum
#
# Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import os
import sys
import struct

class Image(object):
    def __init__(self):
        self.if1 = sys.argv[1]

    def CalculateChecksum(self):
        fsize = os.path.getsize(self.if1) # binary file size in byte
        Sum1 = 0xFFFF
        Sum2 = 0xFFFF

        with open(self.if1, 'rb') as f_r:
            # except Signature and Checksum areas
            NumWords = int((fsize - 8) /2)
            f_r.seek(8, 0)

            while (NumWords):
                if (NumWords > 360):
                    CurrentBlockSize = 360
                else:
                    CurrentBlockSize = NumWords

                NumWords -= CurrentBlockSize

                for i in range (CurrentBlockSize):
                    data = f_r.read(2)
                    val = struct.unpack('H', data)[0] # revert to HEX for calculate
                    Sum1 += val
                    Sum2 += Sum1

                Sum1 = (Sum1 & 0xffff) + (Sum1 >> 16)
                Sum2 = (Sum2 & 0xffff) + (Sum2 >> 16)

            Sum1 = (Sum1 & 0xffff) + (Sum1 >> 16)
            Sum2 = (Sum2 & 0xffff) + (Sum2 >> 16)

        return (Sum2 << 16 | Sum1)

    def UpdateCksm(self):
        try:
            # Calculate File's checksum
            Checksum = self.CalculateChecksum()
            print([self.if1] ,'Checksum is: ', hex(Checksum))

            # Update checksum area in file
            f_w = open(self.if1, 'rb+') # open file as read and write, and point at beginning
            f_w.seek(4, 0) # pointer to Checksum
            f_w.write(struct.pack('I', Checksum)) # revert to INTEGER for write
            f_w.close()

        except Exception as e:
            print(e)

def main():
    image = Image()
    image.UpdateCksm()

if __name__ == '__main__':
    main()
