# @file
#
# Copyright (c) Microsoft Corporation.
# Copyright (c) 2020, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
# Copyright (c) 2020 - 2021, ARM Limited. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import os
import logging
import io

from edk2toolext.environment import shell_environment
from edk2toolext.invocables.edk2_ci_build import CiBuildSettingsManager
from edk2toolext.invocables.edk2_setup import SetupSettingsManager, RequiredSubmodule
from edk2toolext.invocables.edk2_update import UpdateSettingsManager
from edk2toolext.invocables.edk2_pr_eval import PrEvalSettingsManager
from edk2toollib.utility_functions import GetHostInfo
from edk2toollib.utility_functions import RunCmd

class Settings(CiBuildSettingsManager, UpdateSettingsManager, SetupSettingsManager, PrEvalSettingsManager):

    def __init__(self):
        self.ActualPackages = []
        self.ActualTargets = []
        self.ActualArchitectures = []
        self.ActualToolChainTag = ""

    # ####################################################################################### #
    #                             Extra CmdLine configuration                                 #
    # ####################################################################################### #

    def AddCommandLineOptions(self, parserObj):
        pass

    def RetrieveCommandLineOptions(self, args):
        pass

    # ####################################################################################### #
    #                        Default Support for this Ci Build                                #
    # ####################################################################################### #

    def GetPackagesSupported(self):
        ''' return iterable of edk2 packages supported by this build.
        These should be edk2 workspace relative paths '''

        return (
                ["VExpressPkg"]
                )

    def GetArchitecturesSupported(self):
        ''' return iterable of edk2 architectures supported by this build '''
        return (
                "IA32",
                "X64",
                "ARM",
                "AARCH64",
                "RISCV64")

    def GetTargetsSupported(self):
        ''' return iterable of edk2 target tags supported by this build '''
        return ("DEBUG", "RELEASE", "NO-TARGET", "NOOPT")

    # ####################################################################################### #
    #                     Verify and Save requested Ci Build Config                           #
    # ####################################################################################### #

    def SetPackages(self, list_of_requested_packages):
        ''' Confirm the requested package list is valid and configure SettingsManager
        to build the requested packages.

        Raise UnsupportedException if a requested_package is not supported
        '''
        unsupported = set(list_of_requested_packages) - \
            set(self.GetPackagesSupported())
        if(len(unsupported) > 0):
            logging.critical(
                "Unsupported Package Requested: " + " ".join(unsupported))
            raise Exception("Unsupported Package Requested: " +
                            " ".join(unsupported))
        self.ActualPackages = list_of_requested_packages

    def SetArchitectures(self, list_of_requested_architectures):
        ''' Confirm the requests architecture list is valid and configure SettingsManager
        to run only the requested architectures.

        Raise Exception if a list_of_requested_architectures is not supported
        '''
        unsupported = set(list_of_requested_architectures) - \
            set(self.GetArchitecturesSupported())
        if(len(unsupported) > 0):
            logging.critical(
                "Unsupported Architecture Requested: " + " ".join(unsupported))
            raise Exception(
                "Unsupported Architecture Requested: " + " ".join(unsupported))
        self.ActualArchitectures = list_of_requested_architectures

    def SetTargets(self, list_of_requested_target):
        ''' Confirm the request target list is valid and configure SettingsManager
        to run only the requested targets.

        Raise UnsupportedException if a requested_target is not supported
        '''
        unsupported = set(list_of_requested_target) - \
            set(self.GetTargetsSupported())
        if(len(unsupported) > 0):
            logging.critical(
                "Unsupported Targets Requested: " + " ".join(unsupported))
            raise Exception("Unsupported Targets Requested: " +
                            " ".join(unsupported))
        self.ActualTargets = list_of_requested_target

    # ####################################################################################### #
    #                         Actual Configuration for Ci Build                               #
    # ####################################################################################### #

    def GetActiveScopes(self):
        ''' return tuple containing scopes that should be active for this process '''
        scopes = ("cibuild", "edk2-build", "host-based-test")

        self.ActualToolChainTag = shell_environment.GetBuildVars().GetValue("TOOL_CHAIN_TAG", "")

        if GetHostInfo().os.upper() == "LINUX" and self.ActualToolChainTag.upper().startswith("GCC"):
            if "AARCH64" in self.ActualArchitectures:
                scopes += ("gcc_aarch64_linux",)
            if "ARM" in self.ActualArchitectures:
                scopes += ("gcc_arm_linux",)
            if "RISCV64" in self.ActualArchitectures:
                scopes += ("gcc_riscv64_unknown",)

        return scopes

    def GetRequiredSubmodules(self):
        ''' return iterable containing RequiredSubmodule objects.
        If no RequiredSubmodules return an empty iterable
        '''
        rs = []

        # To avoid maintenance of this file for every new submodule
        # lets just parse the .gitmodules and add each if not already in list.
        # The GetRequiredSubmodules is designed to allow a build to optimize
        # the desired submodules but it isn't necessary for this repository.
        result = io.StringIO()
        ret = RunCmd("git", "config --file .gitmodules --get-regexp path",
                     workingdir=self.GetWorkspaceRoot(), outstream=result)
        # Cmd output is expected to look like:
        # submodule.CryptoPkg/Library/OpensslLib/openssl.path CryptoPkg/Library/OpensslLib/openssl
        # submodule.SoftFloat.path ArmPkg/Library/ArmSoftFloatLib/berkeley-softfloat-3
        if ret == 0:
            for line in result.getvalue().splitlines():
                _, _, path = line.partition(" ")
                if path is not None:
                    if path not in [x.path for x in rs]:
                        # add it with recursive since we don't know
                        rs.append(RequiredSubmodule(path, True))
        return rs

    def GetName(self):
        return "Edk2-platforms"

    def GetDependencies(self):
        return [
        ]

    def GetPackagesPath(self):
        ''' Return a list of workspace relative paths that should be mapped as edk2 PackagesPath '''
        result = [os.path.join("Platform", "ARM"), "edk2"]
        return result

    def GetWorkspaceRoot(self):
        ''' get WorkspacePath '''
        return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    def FilterPackagesToTest(self, changedFilesList: list, potentialPackagesList: list) -> list:
        ''' Filter potential packages to test based on changed files. '''
        build_these_packages = []
        possible_packages = potentialPackagesList.copy()
        for f in changedFilesList:
            # split each part of path for comparison later
            nodes = f.split("/")

            # python file change in .pytool folder causes building all
            if f.endswith(".py") and ".pytool" in nodes:
                build_these_packages = possible_packages
                break

            # BaseTools files that might change the build
            if "BaseTools" in nodes:
                if os.path.splitext(f) not in [".txt", ".md"]:
                    build_these_packages = possible_packages
                    break
        return build_these_packages
