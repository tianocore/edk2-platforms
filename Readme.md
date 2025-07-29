This branch holds platforms and drivers actively maintained against the
[edk2](https://github.com/tianocore/edk2) default branch.
If any platform or driver is failing to build against current edk2 and (if
applicable) [edk2-non-osi](https://github.com/tianocore/edk2-non-osi), please
raise a github issue.

For generic information about the edk2-platforms repository, and the process
under which _stable_ and _devel_ branches can be added for individual platforms,
please see
[the introduction on the about branch](https://github.com/tianocore/edk2-platforms/blob/about/Readme.md).

The majority of the content in the EDK II open source project uses a
[BSD-2-Clause Plus Patent License](License.txt).  Additional details on EDK II
open source project code contributions can be found in the edk2 repository
[Readme.md](https://github.com/tianocore/edk2/blob/master/ReadMe.rst).
The EDK II Platforms open source project contains the following components that
are covered by additional licenses:

- [`Silicon/RISC-V/ProcessorPkg/Library/RiscVOpensbiLib/opensbi`](https://github.com/riscv/opensbi/blob/master/COPYING.BSD)

# INDEX
* [Overview](#overview)
* [How To Build (Linux Environment)](#how-to-build-linux-environment)
   * [Manual building](#manual-building)
   * [Using uefi-tools helper scripts](#using-uefi-tools-helper-scripts)
* [How To Build (Windows Environment)](#how-to-build-windows-environment)
* [Maintainers](#maintainers)

# Overview

Platform description files can be found under `Platform/{Vendor}/{Platform}`.

Many platforms require additional image processing beyond the EDK2 build.
Any such steps should be documented (as a Readme.md), and any necessary helper
scripts be contained, under said platform directory.

Any contributions to this repo should be submitted via GitHub Pull Request.

For details of who owns code in certain parts of the repo, see the CODEOWNERS and
REVIEWERS files. Look in CONTRIBUTORS.md to find out people's names and their
email addresses.

In general, you should not privately email the maintainer. You should
email the edk2-devel list, and Cc the area maintainers and
reviewers.

If the maintainer wants to hand over the role to other people,
they should create a PR on GitHub to update CODEOWNERS,
REVIEWERS and CONTRIBUTORS.md with new maintainer, and the new maintainer
should review the PR and approve it.

EDK II Platforms
----------------
W: https://github.com/tianocore/tianocore.github.io/wiki/EDK-II
L: https://edk2.groups.io/g/devel/
T: git - https://github.com/tianocore/edk2-platforms.git

Responsible Disclosure, Reporting Security Issues
-------------------------------------------------
W: https://github.com/tianocore/tianocore.github.io/wiki/Security

# How to build (Linux Environment)

## Prerequisites
The build tools themselves depend on Python (3) and libuuid. Most Linux systems
will come with a Python environment installed by default, but you usually need
to install uuid-dev (or uuid-devel, depending on distribution) manually.

## If cross compiling
If building EDK2 for a different archtecture than the build machine, you need to
obtain an appropriate cross-compiler. X64 (x86_64) compilers also support IA32,
but the reverse may not always be true.

Target architecture | Cross compilation prefix
--------------------|-------------------------
AARCH64             | aarch64-linux-gnu-
ARM                 | arm-linux-gnueabihf-
IA32                | i?86-linux-gnu-* _or_ x86_64-linux-gnu-
IPF                 | ia64-linux-gnu
X64                 | x86_64-linux-gnu-
RISCV64             | riscv64-unknown-elf-
LOONGARCH64         | loongarch64-unknown-linux-

\* i386, i486, i586 or i686

### GCC
Arm provides GCC toolchains for aarch64-linux-gnu and arm-linux-gnueabihf at
[GNU Toolchain for the A-profile Architecture](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-a/downloads)
compiled to run on x86_64/i686 Linux and i686 Windows. Some Linux distributions
provide their own packaged cross-toolchains.

### GCC for RISC-V
RISC-V open source community provides GCC toolchains for
[riscv64-unknown-elf](https://github.com/riscv/riscv-gnu-toolchain)
compiled to run on x86 Linux.

### GCC for LoongArch
Loonson open source community provides GCC toolchains for
[loongarch64-unknown-elf](https://github.com/loongson/build-tools)
compiled to run on x86 Linux

### clang
Clang does not require separate cross compilers, but it does need a
target-specific binutils. These are included with any prepackaged GCC toolchain
(see above), or can be installed or built separately.

## Obtaining source code
1. Create a new folder (directory) on your local development machine
   for use as your workspace. This example uses `/work/git/tianocore`, modify as
   appropriate for your needs.
   ```
   $ export WORKSPACE=/work/git/tianocore
   $ mkdir -p $WORKSPACE
   $ cd $WORKSPACE
   ```

1. Into that folder, clone:
   1. [edk2](https://github.com/tianocore/edk2)
   1. [edk2-platforms](https://github.com/tianocore/edk2-platforms)
   1. [edk2-non-osi](https://github.com/tianocore/edk2-non-osi) (if building
      platforms that need it)
   ```
   $ git clone https://github.com/tianocore/edk2.git
   $ git submodule update --init
   ...
   $ git clone https://github.com/tianocore/edk2-platforms.git
   $ git submodule update --init
   ...
   $ git clone https://github.com/tianocore/edk2-non-osi.git
   ```

1. Set up a **PACKAGES_PATH** to point to the locations of these three
   repositories:

   `$ export PACKAGES_PATH=$PWD/edk2:$PWD/edk2-platforms:$PWD/edk2-non-osi`

## Manual building

1. Set up the build environment (this will modify your environment variables)

   `$ . edk2/edksetup.sh`

   (This step _depends_ on **WORKSPACE** being set as per above.)
1. Build BaseTools

   `make -C edk2/BaseTools`

### Build options
There are a number of options that can (or must) be specified at the point of
building. Their default values are set in `edk2/Conf/target.txt`. If we are
working only on a single platform, it makes sense to just update this file.

target.txt option | command line | Description
------------------|--------------|------------
ACTIVE_PLATFORM   | `-p`         | Description file (.dsc) of platform.
TARGET            | `-b`         | One of DEBUG, RELEASE or NOOPT.
TARGET_ARCH       | `-a`         | Architecture to build for.
TOOL_CHAIN_TAG    | `-t`         | Toolchain profile to use for building.

There is also MAX_CONCURRENT_THREAD_NUMBER (`-n`), roughly equivalent to
`make -j`.

When specified on command line, `-b` can be repeated multiple times in order to
build multiple targets sequentially.

After a successful build, the resulting images can be found in
`Build/{Platform Name}/{TARGET}_{TOOL_CHAIN_TAG}/FV`.

### Build a platform
The main build process runs in parallel natively. For the toolchain tag, use
GCC for gcc version 5 or later, GCC4x for earlier versions, or
CLANGPDB/CLANGDWARF as appropriate when building with clang.
```
$ build -a AARCH64 -t GCC -p Platform/ARM/JunoPkg/ArmJuno.dsc
```
(Note that the description file gets resolved by the build command through
searching in all locations specified in **PACKAGES_PATH**.)

#### If cross-compiling
When cross-compiling, or building with a different version of the compiler than
the default `gcc` or `clang`(/binutils), we additionally need to inform the
build command which toolchain to use. We do this by setting the environment
variable `{TOOL_CHAIN_TAG}_{TARGET_ARCH}_PREFIX` - in the case above,
**GCC_AARCH64_PREFIX**.

So, referring to the cross compiler toolchain table above, we should prepend the `build` command line with `GCC_AARCH64_PREFIX=aarch64-linux-gnu-`.

## Using uefi-tools helper scripts
uefi-tools is a completely unofficial set of helper-scripts developed by Linaro.
They automate figuring out all of the manual options above, and store the paths
to platform description files in a separate configuration file. Additionally,
they simplify bulk-building large numbers of platforms.

The (best effort) intent is to keep this configuration up to date with all
platforms that exist in the edk2-platforms master branch.

The equivalent of the manual example above would be
```
$ git clone https://git.linaro.org/uefi/uefi-tools.git
...
$ ./uefi-tools/edk2-build.sh juno
...
------------------------------------------------------------
                         aarch64 Juno (AARCH64) RELEASE pass
------------------------------------------------------------
pass   1
fail   0
```
The build finishes with a summary of which platforms/targets were built, which
succeeded and which failed (and the total number of either).

Like the `build` command itself, `edk2-build.sh` it supports specifying multiple
targets on a single command line, but it also lets you specify multiple
platforms (or `all` for building all known platforms). So in order to build all
platforms described by the configuration file, for both DEBUG and RELEASE
targets:
```
$ ./uefi-tools/edk2-build.sh -b DEBUG -b RELEASE
```

# How To Build (Windows Environment)

(I genuinely have no idea. Please help!)


# Maintainers

See [CONTRIBUTORS.md](CONTRIBUTORS.md), [CODEOWNERS](CODEOWNERS) and [REVIEWERS](REVIEWERS).

# Submodules

Submodule in EDK II Platforms is allowed but submodule chain should be avoided
as possible as we can. Currently EDK II Platforms contains the following
submodules

- Silicon/RISC-V/ProcessorPkg/Library/RiscVOpensbiLib/opensbi

To get a full, buildable EDK II repository, use following steps of git command

```bash
  git clone https://github.com/tianocore/edk2-platforms.git
  cd edk2-platforms
  git submodule update --init
  cd ..
```

If there's update for submodules, use following git commands to get the latest
submodules code.

```bash
  cd edk2-platforms
  git pull
  git submodule update
```

Note: When cloning submodule repos, '--recursive' option is not recommended.
EDK II Platforms itself will not use any code/feature from submodules in above
submodules. So using '--recursive' adds a dependency on being able to reach
servers we do not actually want any code from, as well as needlessly
downloading code we will not use.
