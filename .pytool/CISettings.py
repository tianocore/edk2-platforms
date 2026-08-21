# @file
# CI configuration settings for edk2-platforms.
# Used to run CI on selected packages.
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
import glob
import logging
import os

from edk2toolext import codeql as codeql_helpers
from edk2toolext.environment import shell_environment
from edk2toolext.invocables.edk2_ci_build import CiBuildSettingsManager
from edk2toolext.invocables.edk2_ci_setup import CiSetupSettingsManager
from edk2toolext.invocables.edk2_pr_eval import PrEvalSettingsManager
from edk2toolext.invocables.edk2_setup import SetupSettingsManager
from edk2toolext.invocables.edk2_update import UpdateSettingsManager
from edk2toollib.utility_functions import GetHostInfo

LOGGER = logging.getLogger(__name__)


class Settings(
    CiSetupSettingsManager,
    CiBuildSettingsManager,
    UpdateSettingsManager,
    SetupSettingsManager,
    PrEvalSettingsManager,
):

    def __init__(self):
        self.ActualPackages = []
        self.ActualTargets = []
        self.ActualArchitectures = []
        self.ActualToolChainTag = ""
        self.ActualScopes = None
        self.PackagePaths = ["Silicon/Intel", "Platform/Intel", "Platform"]
        self.SupportedPackages = ["BoardModulePkg", "MinPlatformPkg", "IntelSiliconPkg"]

    # ####################################################################################### #
    #                             Extra CmdLine configuration                                 #
    # ####################################################################################### #

    def AddCommandLineOptions(self, parserObj):
        try:
            codeql_helpers.add_command_line_option(parserObj)
        except NameError:
            pass

    def RetrieveCommandLineOptions(self, args):
        try:
            self.codeql = codeql_helpers.is_codeql_enabled_on_command_line(args)
        except NameError:
            pass

    # ####################################################################################### #
    #                        Default Support for this Ci Build                                #
    # ####################################################################################### #

    def GetPackagesSupported(self):
        """return iterable of edk2 packages supported by this build.
        These should be edk2 workspace relative paths"""

        return (self.SupportedPackages)

    def GetArchitecturesSupported(self):
        """return iterable of edk2 architectures supported by this build"""
        return ("IA32", "X64", "AARCH64")

    def GetTargetsSupported(self):
        """return iterable of edk2 target tags supported by this build"""
        return ("DEBUG", "RELEASE", "NOOPT")

    # ####################################################################################### #
    #                     Verify and Save requested Ci Build Config                           #
    # ####################################################################################### #

    def SetPackages(self, list_of_requested_packages):
        """Confirm the requested package list is valid and configure SettingsManager
        to build the requested packages.

        Raise RuntimeException if a requested_package is not supported
        """
        unsupported = set(list_of_requested_packages) - set(self.GetPackagesSupported())
        if len(unsupported) > 0:
            LOGGER.critical("Unsupported Package Requested: " + " ".join(unsupported))
            raise RuntimeException(
                "Unsupported Package Requested: " + " ".join(unsupported)
            )
        self.ActualPackages = list_of_requested_packages

    def SetArchitectures(self, list_of_requested_architectures):
        """Confirm the requests architecture list is valid and configure SettingsManager
        to run only the requested architectures.

        Raise UnsupportedException if a list_of_requested_architectures is not supported
        """
        unsupported = set(list_of_requested_architectures) - set(
            self.GetArchitecturesSupported()
        )
        if len(unsupported) > 0:
            LOGGER.critical(
                "Unsupported Architecture Requested: " + " ".join(unsupported)
            )
            raise UnsupportedException(
                "Unsupported Architecture Requested: " + " ".join(unsupported)
            )
        self.ActualArchitectures = list_of_requested_architectures

    def SetTargets(self, list_of_requested_target):
        """Confirm the request target list is valid and configure SettingsManager
        to run only the requested targets.

        Raise RuntimeException if a requested_target is not supported
        """
        unsupported = set(list_of_requested_target) - set(self.GetTargetsSupported())
        if len(unsupported) > 0:
            LOGGER.critical("Unsupported Targets Requested: " + " ".join(unsupported))
            raise RuntimeException(
                "Unsupported Targets Requested: " + " ".join(unsupported)
            )
        self.ActualTargets = list_of_requested_target

    # ####################################################################################### #
    #                         Actual Configuration for Ci Build                               #
    # ####################################################################################### #

    def GetActiveScopes(self):
        """return tuple containing scopes that should be active for this process"""
        if self.ActualScopes is None:
            # cibuild enables CI plugins
            # edk2-build enables nasm ext_dep
            # host-based-test runs unit tests and ensures test dsc contains all unit tests
            scopes = ("cibuild", "edk2-build", "host-based-test")

            self.ActualToolChainTag = shell_environment.GetBuildVars().GetValue(
                "TOOL_CHAIN_TAG", ""
            )

            is_linux = GetHostInfo().os.upper() == "LINUX"

            if is_linux and self.ActualToolChainTag.upper().startswith("GCC"):
                if "AARCH64" in self.ActualArchitectures:
                    scopes += ("gcc_aarch64_linux",)
                if "RISCV64" in self.ActualArchitectures:
                    scopes += ("gcc_riscv64_unknown",)

            try:
                scopes += codeql_helpers.get_scopes(self.codeql)

                if self.codeql:
                    shell_environment.GetBuildVars().SetValue(
                        "STUART_CODEQL_AUDIT_ONLY", "TRUE", "Set in CISettings.py"
                    )
                    codeql_filter_files = [
                        str(n)
                        for n in glob.glob(
                            os.path.join(
                                self.GetWorkspaceRoot(), "**/CodeQlFilters.yml"
                            ),
                            recursive=True,
                        )
                    ]
                    shell_environment.GetBuildVars().SetValue(
                        "STUART_CODEQL_FILTER_FILES",
                        ",".join(codeql_filter_files),
                        "Set in CISettings.py",
                    )
            except NameError:
                pass

            self.ActualScopes = scopes
        return self.ActualScopes

    def GetRequiredSubmodules(self):
        """return iterable containing RequiredSubmodule objects.
        If no RequiredSubmodules return an empty iterable
        """
        rs = []
        return rs

    def GetName(self):
        return "edk2platforms"

    def GetDependencies(self):
        """Return Git Repository Dependencies

        Return an iterable of dictionary objects with the following fields
        {
            Path: <required> Workspace relative path
            Url: <required> Url of git repo
            Commit: <optional> Commit to checkout of repo
            Branch: <optional> Branch to checkout (will checkout most recent commit in branch)
            Full: <optional> Boolean to do shallow or Full checkout.  (default is False)
            ReferencePath: <optional> Workspace relative path to git repo to use as "reference"
            Recurse: <optional> Specifies if Dependency should be recursively cloned.
        }
        """
        return [
            {
                "Path": "EDK2",
                "Url": "https://github.com/tianocore/edk2.git",
                "Branch": "master",
                "Recurse": {"CIFile": ".pytool/CISettings.py"},
            }
        ]

    def GetPackagesPath(self):
        """Return a list of workspace relative paths that should be mapped as edk2 PackagesPath"""
        result = self.PackagePaths
        for a in self.GetDependencies():
            result.append(a["Path"])
        return result

    def GetWorkspaceRoot(self):
        """get WorkspacePath"""
        return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    def FilterPackagesToTest(
        self, changedFilesList: list, potentialPackagesList: list
    ) -> list:
        """Filter potential packages to test based on changed files."""
        return []
