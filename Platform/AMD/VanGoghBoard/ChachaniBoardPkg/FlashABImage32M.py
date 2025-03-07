## @file
# Generate Final Flash A/B Image
#
# Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

# Generate Final Flash A/B Image
from __future__ import print_function
import os
import sys
import argparse

class Image(object):
    """
    class
    """
    def __init__(self):
        """
        init class
        """
        print ([sys.argv[1]])
        print ([sys.argv[2]])
        print ([sys.argv[3]])
        print ([sys.argv[4]])
        print ([sys.argv[5]])
        print ([sys.argv[6]])
        print ([sys.argv[7]])
        print ([sys.argv[8]])
        print ([sys.argv[9]])
        print ([sys.argv[10]])

        self.if1 = [sys.argv[1], 0x00000000] # binary 1: ECSIG                 0x00_0000 (Fixed)
        self.if2 = [sys.argv[2], 0x00000200] # binary 2: EC                    0x00_0200 (Fixed)
        self.if3 = [sys.argv[3], 0x00020000] # binary 3: EFS                   0x02_0000 (Fixed)
        self.if4 = [sys.argv[4], 0x00021000] # binary 4: PSP_L1_DIRECTORY      0x02_1000
        self.if5 = [sys.argv[5], 0x00022000] # binary 5: PD                    0x02_2000 (Fixed)
        self.if6 = [sys.argv[6], 0x00060000] # binary 6: IMAGE_SLOT_HEADER_1   0x06_0000
        self.if7 = [sys.argv[7], 0x00070000] # binary 7: IMAGE_SLOT_HEADER_2   0x07_0000
        self.if8 = [sys.argv[8], 0x00080000] # binary 8: IMAGE_SLOT_1          0x08_0000
        self.if9 = [sys.argv[9], 0x01080000] # binary 9: IMAGE_SLOT_2          0x108_0000
        self.of  = [sys.argv[10], 0x02000000] # 32MB

    def combine(self):
        if os.path.exists(self.of[0]):
            print (self.of[0])
            os.remove(self.of[0])

        try:

            f_w = open(self.of[0], 'wb+')
            for _ in range(int(self.of[1])):
                f_w.write(b'\xFF')

            f_w.seek(self.if1[1], 0)
            with open(self.if1[0], 'rb') as f_read_1:
                f_w.write(f_read_1.read())

            f_w.seek(self.if2[1], 0)
            with open(self.if2[0], 'rb') as f_read_2:
                f_w.write(f_read_2.read())

            f_w.seek(self.if3[1], 0)
            with open(self.if3[0], 'rb') as f_read_3:
                f_w.write(f_read_3.read())

            f_w.seek(self.if4[1], 0)
            with open(self.if4[0], 'rb') as f_read_4:
                f_w.write(f_read_4.read())

            f_w.seek(self.if5[1], 0)
            with open(self.if5[0], 'rb') as f_read_5:
                f_w.write(f_read_5.read())

            f_w.seek(self.if6[1], 0)
            with open(self.if6[0], 'rb') as f_read_6:
                f_w.write(f_read_6.read())

            f_w.seek(self.if7[1], 0)
            with open(self.if7[0], 'rb') as f_read_7:
                f_w.write(f_read_7.read())

            f_w.seek(self.if8[1], 0)
            with open(self.if8[0], 'rb') as f_read_8:
                f_w.write(f_read_8.read())

            f_w.seek(self.if9[1], 0)
            with open(self.if9[0], 'rb') as f_read_9:
                f_w.write(f_read_9.read())

            f_w.close()

        except Exception as e:
            print (e)

def main():
    image = Image()
    image.combine()


if __name__ == '__main__':
    main()
