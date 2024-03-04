## @file
# Generating Test Certificates for VExpressPkg's System Fip FmpDevicePkg
#
# Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

'''
GenTestCert.py
'''

import os
import sys
import argparse
import subprocess
import glob
import shutil
import struct
import datetime
import errno

#
# Globals for help information
#
__prog__        = 'GenTestCert.py'
__copyright__   = 'Copyright (c) 2025, Arm Limited. All rights reserved.'
__description__ = 'Generatiing Test Certificates for System Fip FmpDevicePkg.'

#
# Globals
#
gWorkspace = ''
gBaseToolsPath = ''
gFeatauresFwuPath = ''
gTestCertDir = ''
gTestCertsCnfFile = ''
gRootCerPcdFile = ''
gArgs      = None

#
# Command Templete
#
GenerateKeyCommand = '''
openssl genrsa
-aes256
-passout pass:
-out {KEY_FILE} 2048
'''
GenerateRootCertCommand = '''
openssl req
-config {OPENSSL_CNF_FILE}
-extensions v3_ca
-new
-x509
-days 3650
-key {KEY_FILE}
-passin pass:
-subj "/C=UK/ST=test/O=test/CN=www.{DOMAIN_NAME}.com"
-out {OUT_CERT_FILE}
'''
GenerateCertCommand = '''
openssl req
-config {OPENSSL_CNF_FILE}
-new
-key {KEY_FILE}
-passin pass:
-subj "/C=UK/ST=test/O=test/CN=www.{DOMAIN_NAME}.com"
-out {OUT_CSR_FILE}
&&
openssl ca
-config {OPENSSL_CNF_FILE}
-extensions v3_ca
-batch
-in {OUT_CSR_FILE}
-days 3650
-cert {CA_CERT_FILE}
-keyfile {CA_KEY_FILE}
-passin pass:
-out {OUT_CERT_FILE}
'''
GeneratePubPemFromCertCommand = '''
openssl x509
-in {CERT_FILE}
-out {OUT_CER_FILE}
-outform DER
&&
openssl x509
-inform DER
-in {OUT_CER_FILE}
-out {OUT_PUB_PEM_FILE}
-outform PEM
'''

GeneratePkcsPemCommand = '''
openssl pkcs12
-export
-out
{OUT_PFX_FILE}
-inkey
{KEY_FILE}
-passin pass:
-passout pass:
-in
{CERT_FILE}
&&
openssl pkcs12
-in
{OUT_PFX_FILE}
-passin pass:
-nodes
-out
{OUT_PEM_FILE}
'''

BinToPcdCommand = '''
python {BASE_TOOLS_PATH}/Scripts/BinToPcd.py
-i {ROOT_CER_FILE}
-p gFmpDevicePkgTokenSpaceGuid.PcdFmpDevicePkcs7CertBufferXdr
-x
-o {OUT_PCD_FILE}
'''
def LogAlways(Message):
    sys.stdout.write(__prog__ + ': ' + Message + '\n')
    sys.stdout.flush()

def Log(Message):
    global gArgs
    if not gArgs.Verbose:
        return
    sys.stdout.write(__prog__ + ': ' + Message + '\n')
    sys.stdout.flush()

def Error(Message, ExitValue=1):
    sys.stderr.write(__prog__ + ': ERROR: ' + Message + '\n')
    sys.exit(ExitValue)

def RelativePath(target):
    global gWorkspace
    Log('RelativePath' + target)
    return os.path.relpath(target, gWorkspace)

def NormalizePath(target):
    if isinstance(target, tuple):
        return os.path.normpath(os.path.join(*target))
    else:
        return os.path.normpath(target)

def RunCommand(command, workdir):
    Command = ' '.join(command.splitlines()).strip()

    LogAlways(Command)

    Process = subprocess.Popen(
                Command,
                cwd=workdir,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                shell=True
                )

    ProcessOutput = Process.communicate()

    return Process.returncode

def CreateDirectory(target):
    target = NormalizePath(target)
    if not os.path.exists(target):
        Log('mkdir %s' % (RelativePath(target)))
        os.makedirs(target)

def CreateTestCertDirecotry():
    if not os.path.exists(gTestCertDir):
        CreateDirectory(gTestCertDir)

    TestCADir = NormalizePath((gTestCertDir, "demoCA"))
    if not os.path.exists(TestCADir):
        CreateDirectory(TestCADir)

    TestNewCertsDir = NormalizePath((TestCADir, "newcerts"))
    if not os.path.exists(TestNewCertsDir):
        CreateDirectory(TestNewCertsDir)

    SerialFile = NormalizePath((TestCADir, "serial"))
    if not os.path.exists(SerialFile):
        with open(SerialFile, 'w') as file:
            file.write("01")

    IndexFile = NormalizePath((TestCADir, "index.txt"))
    if not os.path.exists(IndexFile):
        open(IndexFile, 'w').close()

def GenerateRootKey():
    RootKeyFile = NormalizePath((gTestCertDir, "TestRoot.key"))

    if not os.path.exists(RootKeyFile):
        Command = GenerateKeyCommand.format(KEY_FILE = RootKeyFile)
        ret = RunCommand(Command, gTestCertDir)
        if ret != 0:
            Error ("Failed to generate TestRoot.key...", ret)

def GenerateRootCert():
    RootKeyFile = NormalizePath((gTestCertDir, "TestRoot.key"))
    RootCertFile = NormalizePath((gTestCertDir, "TestRoot.crt"))
    RootCerFile = NormalizePath((gTestCertDir, "TestRoot.cer"))
    RootPubPemFile = NormalizePath((gTestCertDir, "TestRoot.pub.pem"))
    DomainName = "testroot"

    if not os.path.exists(RootCerFile):
        Command = GenerateRootCertCommand.format(
                    OPENSSL_CNF_FILE = gTestCertsCnfFile,
                    KEY_FILE = RootKeyFile,
                    DOMAIN_NAME=DomainName,
                    OUT_CERT_FILE = RootCertFile
                    )
        ret = RunCommand(Command, gTestCertDir)
        if ret != 0:
            Error ("Failed to generate TestRoot.crt...", ret)

    if not os.path.exists(RootCerFile):
        Command = GeneratePubPemFromCertCommand.format(
                    CERT_FILE = RootCertFile,
                    OUT_CER_FILE = RootCerFile,
                    OUT_PUB_PEM_FILE = RootPubPemFile
                    )
        ret = RunCommand(Command, gTestCertDir)
        if ret != 0:
            Error ("Failed to generate TestRoot.pub.pem...", ret)

def GenerateCaKey():
    CaKeyFile = NormalizePath((gTestCertDir, "TestSub.key"))

    if not os.path.exists(CaKeyFile):
        Command = GenerateKeyCommand.format(KEY_FILE = CaKeyFile)
        ret = RunCommand(Command, gTestCertDir)
        if ret != 0:
            Error ("Failed to generate TestSub.key...", ret)

def GenerateCaCert():
    RootKeyFile = NormalizePath((gTestCertDir, "TestRoot.key"))
    RootCertFile = NormalizePath((gTestCertDir, "TestRoot.crt"))
    CaKeyFile = NormalizePath((gTestCertDir, "TestSub.key"))
    CaCsrFile = NormalizePath((gTestCertDir, "TestSub.csr"))
    CaCertFile = NormalizePath((gTestCertDir, "TestSub.crt"))
    CaCerFile = NormalizePath((gTestCertDir, "TestSub.cer"))
    CaPubPemFile = NormalizePath((gTestCertDir, "TestSub.pub.pem"))
    DomainName = "testsub"

    if not os.path.exists(CaCertFile):
        Command = GenerateCertCommand.format(
                    OPENSSL_CNF_FILE = gTestCertsCnfFile,
                    KEY_FILE = CaKeyFile,
                    DOMAIN_NAME=DomainName,
                    CA_KEY_FILE = RootKeyFile,
                    CA_CERT_FILE = RootCertFile,
                    OUT_CSR_FILE = CaCsrFile,
                    OUT_CERT_FILE = CaCertFile
                    )
        ret = RunCommand(Command, gTestCertDir)
        if ret != 0:
            Error ("Failed to generate TestSub.crt...", ret)

    if not os.path.exists(CaCerFile):
        Command = GeneratePubPemFromCertCommand.format(
                    CERT_FILE = CaCertFile,
                    OUT_CER_FILE = CaCerFile,
                    OUT_PUB_PEM_FILE = CaPubPemFile
                    )
        ret = RunCommand(Command, gTestCertDir)
        if ret != 0:
            Error ("Failed to generate TestSub.pub.pem...", ret)

def GenerateUserKey():
    UserKeyFile = NormalizePath((gTestCertDir, "TestUser.key"))

    if not os.path.exists(UserKeyFile):
        Command = GenerateKeyCommand.format(KEY_FILE = UserKeyFile)
        ret = RunCommand(Command, gTestCertDir)
        if ret != 0:
            Error ("Failed to generate TestUser.key...", ret)

def GenerateUserCert():
    CaKeyFile = NormalizePath((gTestCertDir, "TestSub.key"))
    CaCertFile = NormalizePath((gTestCertDir, "TestSub.crt"))
    UserKeyFile = NormalizePath((gTestCertDir, "TestUser.key"))
    UserCsrFile = NormalizePath((gTestCertDir, "TestUser.csr"))
    UserCertFile = NormalizePath((gTestCertDir, "TestUser.crt"))
    UserCerFile = NormalizePath((gTestCertDir, "TestUser.cer"))
    UserPubPemFile = NormalizePath((gTestCertDir, "TestUser.pub.pem"))
    DomainName = "testuser"

    if not os.path.exists(UserCertFile):
        Command = GenerateCertCommand.format(
                    OPENSSL_CNF_FILE = gTestCertsCnfFile,
                    KEY_FILE = UserKeyFile,
                    DOMAIN_NAME=DomainName,
                    CA_KEY_FILE = CaKeyFile,
                    CA_CERT_FILE = CaCertFile,
                    OUT_CSR_FILE = UserCsrFile,
                    OUT_CERT_FILE = UserCertFile
                    )
        ret = RunCommand(Command, gTestCertDir)
        if ret != 0:
            Error ("Failed to generate TestUser.crt...", ret)

    if not os.path.exists(UserCerFile):
        Command = GeneratePubPemFromCertCommand.format(
                    CERT_FILE = UserCertFile,
                    OUT_CER_FILE = UserCerFile,
                    OUT_PUB_PEM_FILE = UserPubPemFile
                    )
        ret = RunCommand(Command, gTestCertDir)
        if ret != 0:
            Error ("Failed to generate TestUser.pub.pem...", ret)

def GenerateUserPkcsPem():
    UserKeyFile = NormalizePath((gTestCertDir, "TestUser.key"))
    UserCertFile = NormalizePath((gTestCertDir, "TestUser.crt"))
    UserPfxFile = NormalizePath((gTestCertDir, "TestUser.pfx"))
    UserPemFile = NormalizePath((gTestCertDir, "TestUser.pem"))

    if not os.path.exists(UserPemFile):
        Command = GeneratePkcsPemCommand.format(
                    OUT_PFX_FILE = UserPfxFile,
                    KEY_FILE = UserKeyFile,
                    CERT_FILE = UserCertFile,
                    OUT_PEM_FILE = UserPemFile
                    )
        ret = RunCommand(Command, gTestCertDir)
        if ret != 0:
            Error ("Failed to generate TestUser.pem...", ret)

def GenerateRootCerPcdFile():
    RootCerFile = NormalizePath((gTestCertDir, "TestRoot.cer"))
    Command = BinToPcdCommand.format(
                BASE_TOOLS_PATH = gBaseToolsPath,
                ROOT_CER_FILE = RootCerFile,
                OUT_PCD_FILE = gRootCerPcdFile
                )
    ret = RunCommand(Command, None)
    if ret != 0:
        Error ("Failed to generate Root.cer.gFmpDevicePkgTokenSpaceGuid.PcdFmpDevicePkcs7CertBufferXdr.inc", ret)

    LogAlways("Root.cer.gFmpDevicePkgTokenSpaceGuid.PcdFmpDevicePkcs7CertBufferXdr.inc is gererated.")

if __name__ == '__main__':
    def convert_arg_line_to_args(arg_line):
        for arg in arg_line.split():
            if not arg.split():
                continue
            yield arg

    #
    # Create command line argument parser object
    #
    parser = argparse.ArgumentParser(
                        prog = __prog__,
                        description = __description__ + __copyright__,
                        conflict_handler = 'resolve'
                        )
    parser.convert_arg_line_to_args = convert_arg_line_to_args

    parser = argparse.ArgumentParser(
                        prog = __prog__,
                        description = __description__ + __copyright__,
                        conflict_handler = 'resolve'
                        )

    parser.add_argument(
             '-v', '--verbose', dest = 'Verbose', action = 'store_true',
             help = '''Turn on verbose output with informational messages printed'''
             )
    #
    # Parse command line arguments
    #
    gArgs, extra = parser.parse_known_args()

    #
    # Get WORKSPACE environment variable
    #
    try:
        gWorkspace = os.environ['WORKSPACE']
    except:
        Error ('WORKSPACE environment variable not set')

    #
    # Get PACKAGES_PATH and generate prioritized list of paths
    #
    PathList = [gWorkspace]
    try:
        PathList += os.environ['PACKAGES_PATH'].split(os.pathsep)
    except:
        pass

    try:
        gBaseToolsPath = os.environ['EDK_TOOLS_PATH']
    except:
        Error ('EDK_TOOLS_PATH enviroment variable not set')

    #
    # Determine full path of VExpressPkg
    #
    gFeaturesFwuPath = ''
    for Path in PathList:
        if gFeaturesFwuPath == '':
            if os.path.exists (os.path.join(Path, 'Platform/ARM/Features/Fwu')):
                gFeaturesFwuPath = os.path.join(Path, 'Platform/ARM/Features/Fwu')

    if gFeaturesFwuPath == '':
        Error ('Can not find VExpressPkg in WORKSPACE or PACKAGES_PATH')

    gTestCertDir = NormalizePath((gFeaturesFwuPath, "TestCert"))
    gTestCertsCnfFile = NormalizePath((gFeaturesFwuPath, "TestCerts.cnf"))
    gRootCerPcdFile = NormalizePath((
                        gFeaturesFwuPath,
                        "Root.cer.gFmpDevicePkgTokenSpaceGuid.PcdFmpDevicePkcs7CertBufferXdr.inc"
                        ))

    if not os.path.exists(gRootCerPcdFile) or os.stat(gRootCerPcdFile).st_size == 0:
        CreateTestCertDirecotry()
        GenerateRootKey()
        GenerateRootCert()
        GenerateCaKey()
        GenerateCaCert()
        GenerateUserKey()
        GenerateUserCert()
        GenerateUserPkcsPem()
        GenerateRootCerPcdFile()
