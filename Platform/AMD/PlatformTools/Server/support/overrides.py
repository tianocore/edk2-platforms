"""

  Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

"""
import os
import sys
import shutil

excludeFolder = ["AgesaModulePkg", "AgesaPkg", "AmdCpmPkg", "AmdCbsPkg"]

def isExcluded (file):
    for folder in excludeFolder:
        if folder in os.path.dirname(file):
            return True
    return False

def _overrides(override=False):
    """!
    Override or restore Platfrom Override files

    @param overrides        True = override files
                            False = restore files
    @exception              varies by failure
    """
    if override:
        mode = "Overrides"
    else:
        mode = "Restores"
    print("\nAMD server platform tianocore source file override processing {}".format(mode))

    # Get board folder
    for arg in sys.argv[1:]:
        if "Project.dsc" in arg:
            boardName = arg.split(os.sep)[0]
    if override:
        print("Override files from " + boardName)
    else:
        print("Restore the files overrode by " + boardName)

    rel_platform_dir = os.path.join (
            "edk2-platforms",
            "Platform",
            "AMD",
            boardName,
            "Override"
        )
    if sys.platform.startswith("linux"):
        workspace ="WORKSPACE"
    elif sys.platform.startswith("win"):
        workspace ="Workspace"

    if workspace not in os.environ:
        print("edk2 \"" + workspace + "\" environment variable is not set! Build process breaks...")
        sys.exit()

    edk2Workspace = os.path.expanduser (os.environ.get(workspace))
    search_dir_tuple = [
        os.path.join(
            edk2Workspace,
            rel_platform_dir
        )]
    # Get environment variables exception if not located
    for search_dir in search_dir_tuple:
        print("\tSearching directory: " + search_dir)
        for root, dirs, files in os.walk(search_dir):
            for file in files:
                src = os.path.join(root, file)
                dst = os.path.join(edk2Workspace, src[len(search_dir) + 1 :])
                back = "{}.back".format(dst)

                if isExcluded (dst):
                    continue

                if override:
                    if os.path.exists(dst):
                        # Do not override back or dst if back already exists
                        # Leftover from failed build. A clean build will clean up
                        if not os.path.exists(back):
                            print('src: "{}"\n\tdst: "{}"\n\tback: "{}"'.format(src, dst, back))
                            shutil.copy(dst, back)
                            # src must exist, no need to check
                            shutil.copy(src, dst)
                        else:
                            print('\tNo Override: Backup already exists: "{}"'.format(back))
                    else:
                        # This is a new file, copy it to destination
                        print('src: "{}"\n\tdst: new file: "{}"\n\tback: no back file'.format(src, dst))
                        if not os.path.exists(os.path.dirname(dst)):
                            os.makedirs(os.path.dirname(dst))
                        shutil.copy(src, dst)
                else:
                    if os.path.exists(back):
                        print('Restore to dst: "{}"\n\tsrc: "{}"\n'.format(dst, back))
                        shutil.move(back, dst)
                    else:
                        if os.path.exists(dst):
                            print('No Restore: Remove the new source file:"{}"'.format(dst))
                            os.remove(dst)
                        else:
                            print('No source file to remove:"{}"'.format(dst))


def overrides():
    """!
    Override Platfrom Override files
    """
    _overrides(override=True)


def restore_overrides():
    """!
    Restore Platfrom Override files
    """
    _overrides(override=False)
