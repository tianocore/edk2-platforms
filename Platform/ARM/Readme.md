# Introduction

These instructions explain how to get an edk2/edk2-platforms build running
on the Arm Base FVP and a Juno. The Arm Base FVP is a software model provided by ARM (for free)
, which models a Cortex A core with various peripherals. More information
can be found [here](https://developer.arm.com/products/system-design/fixed-virtual-platforms).

## Build environment setup on Linux or Windows

### Initial steps

The first step towards building an EDKII firmware image is to create a working directory.

1. Launch a terminal window.
2. Create a directory on your development machine (we willl name it 'source' in this example).
3. Set the WORKSPACE environment variable to point to this directory.

#### Example:
In a Linux bash shell:
```
cd <Directory where you want to work>
mkdir source
cd source
export WORKSPACE=$PWD
```

OR

In a Windows command prompt:

```
cd <Directory where you want to work>
mkdir source
cd source
set WORKSPACE=%CD%
```

### Cloning the source code repositories

Note: To clone the repositories you need 'git' to be installed on your development PC (see Development Tools).

In the terminal window, change directory to your workspace ('source') folder and run the following commands. Install Git if necessary.

```
git clone https://github.com/tianocore/edk2.git
git clone https://github.com/tianocore/edk2-platforms.git
git clone https://github.com/acpica/acpica.git
```

Then go to the edk2 folder and update the submodules.

```
cd edk2
git submodule update --init
cd ..
```

# Building firmware on a Linux host

## Prerequisites

- A 64-bit development machine.
- Ubuntu 20.04 desktop.
- At least 10GB of free disk space.

Check the Ubuntu version by typing the following in the terminal window.

```
$ uname -srvmpio
Linux 5.4.0-131-generic #147-Ubuntu SMP Fri Oct 14 17:07:22 UTC 2022 x86_64 x86_64 x86_64 GNU/Linux
```

### Development Tools

The following tools must be installed on the development PC.


| Sr. No.   | Tool                | Description                                                  | Install instructions                                     |
|-----------|---------------------|--------------------------------------------------------------|----------------------------------------------------------|
| 1         | Python 3            | Python interpreter                                           | $ sudo apt install python3 python3-distutils             |
| 2         | Git                 | Git source control tool                                      | $ sudo apt install git                                   |
| 3         | uuid-dev            | Required for including uuid/uuid.h                           | $ sudo apt install uuid-dev                              |
| 4         | build-essential     | Installs make, gcc, g++, etc                                 | $ sudo apt install build-essential <br> $ make -v <br> GNU Make 4.2.1 <br> gcc --version <br> gcc (Ubuntu 9.4.0-1ubuntu1~20.04.1) 9.4.0 <br> $ g++ --version <br> g++ (Ubuntu 9.4.0-1ubuntu1\~20.04.1) 9.4.0 |
| 5         | bison               | A parser generator required by acpica tools.                 | $ sudo apt install bison                                 |
| 6         | flex                | A fast lexical analyzer generator required by acpica tools   | $ sudo apt get install flex                              |

### Setting up the development tools

Install the required development tools by running the following commands in the terminal window.

```
$ sudo apt install bison build-essential flex git uuid-dev
 ```

```
$ sudo apt install python3 python3-distutils
 ```

### Arm cross compiler toolchain

The Arm toolchain to cross compile from x86_64-linux to aarch64-elf is available [here](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads).

Select the latest toolchain to match the development PC architecture. Select the little-endian 'AArch64 ELF bare-metal target (aarch64-elf)' GCC cross compiler.

Example: For a x86_64 development PC, download arm-gnu-toolchain-12.2.rel1-x86_64-aarch64-none-elf.tar.xz

Create a directory called 'toolchain' under the workspace folder. For example source\toolchain and extract the toolchain to this directory.

```
$ mkdir $WORKSPACE/toolchain
$ cd $WORKSPACE/toolchain
$ wget https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu/12.2.rel1/binrel/arm-gnu-toolchain-12.2.rel1-x86_64-aarch64-none-elf.tar.xz
$ tar xf arm-gnu-toolchain-12.2.rel1-x86_64-aarch64-none-elf.tar.xz
$ cd $WORKSPACE
```

### Build the acpica tools

The acpica tools implement the latest iasl compiler. To build the acpica tools, run the following commands in the terminal window.

```
$ make -C $WORKSPACE/acpica
```

## Building EDKII firmware

1. To build the firmware image, follow the steps below and run the commands in the terminal window.

2. Set up the environment variables.

```
$ export GCC_AARCH64_PREFIX=$WORKSPACE/toolchain/arm-gnu-toolchain-12.2.rel1-x86_64-aarch64-none-elf/bin/aarch64-none-elf-
$ export PACKAGES_PATH=$WORKSPACE/edk2:$WORKSPACE/edk2-platforms
$ export IASL_PREFIX=$WORKSPACE/acpica/generate/unix/bin/
$ export PYTHON_COMMAND=/usr/bin/python3
```

3. Configure the EDKII development environment by running the edk2setup bash script.
```
$ source edk2/edksetup.sh
```

4. Build the BaseTools.
```
$ make -C edk2/BaseTools
```

### Build the firmware for Arm FVP Base Model platform

Run the following command to build the firmware for FVP Base platform.
```
$ build -a AARCH64 -t GCC -p Platform/ARM/VExpressPkg/ArmVExpress-FVP-AArch64.dsc -b < DEBUG | RELEASE >
```

The firmware binaries can be found at the following location:
```
$WORKSPACE/Build/ArmVExpress-FVP-AArch64/<DEBUG|RELEASE>_GCC/FV/FVP_AARCH64_EFI.fd
```

Note: The same firmware binary can be used with Arm FVP Base AEMvA-AEMvA and
Armv-A Base RevC AEM FVP models.

### Build the firmware for Arm Juno platform

Run the following command to build the firmware for Arm Juno platform.
```
$ build -a AARCH64 -t GCC -p Platform/ARM/JunoPkg/ArmJuno.dsc -b < DEBUG | RELEASE >
```

The firmware binaries can be found at the following location:
```
$WORKSPACE/Build/ArmJuno/<DEBUG|RELEASE>_GCC/FV/BL33_AP_UEFI.fd
```

# Building firmware on a Windows host using Windows Subsystem for Linux (WSL)

The instructions for building the firmware using WSL are similar to that for a Linux host.
The prerequisites for setting up the Windows Subsystem for Linux environment are listed below.

## Prerequisites

- A x64 development machine with Windows 10 (Version 21H2 - OS Build 19044.2486).
- At least 10GB of free disk space.
- Install the Windows Subsystem for Linux. Select Ubuntu 20.04 LTS from the Microsoft Store.

Check the Ubuntu version by typing the following on the console.
```
$ uname -srvmpio
Linux 4.4.0-19041-Microsoft #2311-Microsoft Tue Nov 08 17:09:00 PST 2022 x86_64 x86_64 x86_64 GNU/Linux
```

The remaining instructions for installing the development tools, configuring the development environment and building firmware are exactly the same as those for a Linux host.

# Building firmware on a x64 Windows host

#### Prerequisites

- A 64-bit development machine
- Windows 10 desktop (Version 21H2 - OS Build 19044.2486)
- At least 10GB of free disk space.

#### Development Tools

The following tools must be installed on the development machine.

| Sr. No.   | Tool                                         | Description                                                  | Install instructions                                     |
|-----------|----------------------------------------------|--------------------------------------------------------------|----------------------------------------------------------|
| 1         | Python 3                                     | Python interpreter                                           | Go [here](https://www.python.org/downloads/windows/) <br> <br> Choose the latest Python 3.X release. <br> <br> Download and run the Windows x86_64 MSI installer <br> <br> If needed, add the python executable to your path by executing the following command: <br> > set PATH=<Path_to_the_python_executable>;%PATH%  |
| 2         | Git                                          | Git source control tool                                      | Go [here](https://git-scm.com/download/win) <br> <br> Download and run the 64-bit Git for Windows Setup |
| 3         | ASL tools                                    | iasl compiler and other tools for the ASL language           | Go [here](https://www.acpica.org/downloads/binary-tools)  <br> <br> Download the iASL Compiler and Windows ACPI Tools <br> <br> Extract the content and place it at C:\ASL\ <br> <br> Check that the compiler is at the right place by executing: <br> > C:\ASL\iasl.exe -v |
| 4         | Microsoft Visual Studio 2019 Professional    | Microsoft IDE and compiler toolchain.                        | Go [here](https://visualstudio.microsoft.com/downloads/) <br> <br> Download and install Visual Studio 2019 Professional |
| 5         | echo tool                                    | Echo                                                         | See Workaround for echo command below. |


## Setting up the development tools

Install the required development tools listed above by running the appropriate installer applications.

### Arm cross compiler toolchain
The Arm toolchain Windows (i686-mingw32) hosted cross compilers are available [here](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads).

Select the latest toolchain for 'AArch64 bare-metal target (aarch64-none-elf)' GCC cross compiler.

Example: Download arm-gnu-toolchain-12.2.rel1-mingw-w64-i686-aarch64-none-elf

Create a directory called 'toolchain' under the workspace directory and extract the toolchain to this directory using the downloaded installer.

The toolchain folder tree should look as below:

```
toolchain
+---arm-gnu-toolchain-12.2.rel1-mingw-w64-i686-aarch64-none-elf
|   +---aarch64-none-elf
|   +---bin
|   +---include
|   +---lib
|   +---libexec
|   +---share
```

### Workaround for the echo command

EDKII needs a workaround related to the echo command. A script replacing the Windows echo executable must be created, with the name "echo.BAT?:

- Create a file named "echo.BAT" in the folder of your choice.
- Paste the following lines inside the file:

```
rem %~f0  echo.BAT  %*
rem This file exists to overcome a problem in the EDKII build where
rem build_rule.template invokes a command as:
rem     "$(OBJCOPY)" $(OBJCOPY_FLAGS) ${dst}
rem When OBJCOPY is set to echo, this results in the following error:
rem     "echo" objcopy not needed for m:\...\PCD\Dxe\Pcd\DEBUG\PcdDxe.dll
rem And CMD.EXE fails to find the DOS echo command because of the quotes
@echo %*
@goto :EOF
```

- Add the file to your PATH by executing:
```
> set PATH=<Path_to_the_echo_file>;%PATH%
```

## Building EDKII firmware

1. To build the firmware image, follow the steps below and run the commands in the terminal window.
2. Set up the environment variables.

```
 set GCC_AARCH64_PREFIX=%WORKSPACE%\toolchain\arm-gnu-toolchain-12.2.rel1-mingw-w64-i686-aarch64-none-elf\bin\aarch64-none-elf-
 set PACKAGES_PATH=%WORKSPACE%\edk2;%WORKSPACE%\edk2-platforms
 set EDK_TOOLS_PATH=%WORKSPACE%\edk2\BaseTools
 set GCC_HOST_BIN=n
```

Select the python version you wish to use and set the PYTHON_COMMAND environment variable to your Python executable.

Set the PYTHON_COMMAND to point to the Python3 executable.

Check that your path is set up as it is stated in the development tools table above. It should give access to:

- The make executable
- The echo.BAT script

3. Configure the EDKII development environment by running the edksetup.bat script.
The Rebuild option can be skipped if the BaseTools have already been built.

The ForceRebuild option can be used to do a clean build of the Base tools.
```
> call %WORKSPACE%\edk2\edksetup.bat [Rebuild | ForceRebuild]
```

### Build the firmware for Arm FVP Base Model platform

Run the following command to build the firmware for FVP Base platform.

```
> build -a AARCH64 -t GCC -p Platform\ARM\VExpressPkg\ArmVExpress-FVP-AArch64.dsc -b < DEBUG | RELEASE >
```

The firmware binaries can be found at the following location:
```
%WORKSPACE%\Build\ArmVExpress-FVP-AArch64\<DEBUG|RELEASE>_GCC\FV\FVP_AARCH64_EFI.fd
```
Note: The same firmware binary can be used with Arm FVP Base AEMvA-AEMvA and
Armv-A Base RevC AEM FVP models.

### Build the firmware for Arm Juno platform

Run the following command to build the firmware for Arm Juno platform.

```
> build -a AARCH64 -t GCC -p Platform\ARM\JunoPkg\ArmJuno.dsc -b < DEBUG | RELEASE >
```
The firmware binaries are at the following location:

```
%WORKSPACE%\Build\ArmJuno\<DEBUG|RELEASE>_GCC\FV\BL33_AP_UEFI.fd
```

## Standalone MM support

Arm FVP Base Model Platform support Standalone MM. To integrate Standalone MM
setup, the TF-A build must include the options to specify the SPMC to be used
as well as the Standalone MM binary.

Standalone MM can host multiple services that the EDKII firmware can utilise:
e.g. Secure Variable Service can be hosted by Standalone MM and is enabled
by the ENABLE_UEFI_SECURE_VARIABLE build option.

Note: The ENABLE_UEFI_SECURE_VARIABLE build option must be specified when
building both the Standalone MM and EDKII firmware binary.

### Building Standalone MM for Arm FVP Base Model platform
To build Standalone MM binary, run following command:

```
> build -a AARCH64 -t GCC -p Platform\ARM\VExpressPkg\PlatformStandaloneMm.dsc -b < DEBUG | RELEASE > { -D option1, -D option2, ... }
```

e.g. to enable Secure Variable Service pass the following option to the build
command line.

   '-D ENABLE_UEFI_SECURE_VARIABLE=1'

The Standalone MM binary is generated at the following location:
```
%WORKSPACE%\Build\ArmVExpress-FVP-AArch64\<DEBUG|RELEASE>_GCC\FV\BL32_AP_MM.fd
```

### Building TF-A for Standalone MM

TF-A supports the following ABIs to communicate with Standalone MM:

   - FF-A (>= v1.2)

#### Building TF-A with FF-A as the communication ABI
TF-A should be built with the following additional build flags:
```
    SPD=spmd SPMD_SPM_AT_SEL2=0 SPMC_AT_EL3=1 SPMC_AT_EL3_SEL0_SP=1 CTX_INCLUDE_EL2_REGS=0 NS_TIMER_SWITCH=1 HOB_LIST=1 \
    FVP_TRUSTED_SRAM_SIZE=512 ARM_SPMC_MANIFEST_DTS=${TF_A_DIR}/plat/arm/board/fvp/fdts/fvp_stmm_manifest.dts \
    BL32={StandaloneMm Binary}
```
e.g.
```
cd tf-a
make all PLAT=fvp CROSS_COMPILE={cross_compile_prefix} DEBUG=1 V=1 CSS_NON_SECURE_UART=1 EXTRA_EL2_INIT=1 \
     EL3_EXCEPTION_HANDLING=1 ENABLE_SME2_FOR_NS=0 ENABLE_SME_FOR_NS=0 ENABLE_SVE_FOR_NS=0 CTX_INCLUDE_AARCH32_REGS=0 \
     SPD=spmd SPMD_SPM_AT_SEL2=0 SPMC_AT_EL3=1 SPMC_AT_EL3_SEL0_SP=1 CTX_INCLUDE_EL2_REGS=0 NS_TIMER_SWITCH=1 HOB_LIST=1 \
     FVP_TRUSTED_SRAM_SIZE=512 ARM_SPMC_MANIFEST_DTS=${TF_A_DIR}/plat/arm/board/fvp/fdts/fvp_stmm_manifest.dts \
     BL32={StandaloneMm Binary}
```

#### Building the FIP image
The FIP image should be generated with the following additional option with FF-A:
```
   --tos-fw-config  $TF_A_DIR/build/fvp/<debug|release>/fdts/fvp_stmm_manifest.dtb"
```

e.g.
```
cd tf-a
./tools/fiptool/fiptool --verbose update \
   --tb-fw $TF_A_DIR/build/fvp/debug/bl2.bin \
   --soc-fw $TF_A_DIR/build/fvp/debug/bl31.bin \
   --tos-fw ${WORKSPACE}/Build/ArmVExpress-FVP-AArch64/DEBUG_GCC/FV/BL32_AP_MM.fd \
   --nt-fw ${WORKSPACE}/Build/ArmVExpress-FVP-AArch64/DEBUG_GCC/FV/FVP_AARCH64_EFI.fd \
   --hw-config  $TF_A_DIR/build/fvp/debug/fdts/fvp-base-gicv3-psci.dtb \
   --tos-fw-config  $TF_A_DIR/build/fvp/debug/fdts/fvp_stmm_manifest.dtb \
   fip_fvp.bin
```


### Running the FVP RevC model with Standalone MM support

The following additional command line options should be specified to run the
FVP RevC model with Standalone MM.

```
  -C bp.secure_memory=1
  -C bp.secure_only_flash1=1
```

## GICv5 support with FVP RevC model.

Arm FVP Base Model Platform supports boot with GICv5 with some limitations:

  - LPI idle state is not supported.
  - StandaloneMm is not supported since SPD is not supported with
    GICv5 option in TF-A.

However, the same binary built for FVP RevC with GICv5 works for
FVP RevC with GICv3 and FVP AEM.

To build firmware for Arm FVP Base Model platform, please follow the same
step "Build the firmware for Arm FVP Base Model platform".

### Download FVP RevC model with GICv5

You can download FVP RevC model with GICv5 in [here](https://developer.arm.com/Tools%20and%20Software/Fixed%20Virtual%20Platforms/Arm%20Architecture%20FVPs).

### Building TF-A with GICv5

TF-A should be built with the following additional build flags:
```
  FVP_USE_GIC_DRIVER=FVP_GICV5
```
e.g.
```
cd tf-a
make all PLAT=fvp CROSS_COMPILE={cross_compile_prefix} DEBUG=1 V=1 \
         CSS_NON_SECURE_UART=1 EXTRA_EL2_INIT=0 FVP_FAKE_TRNG_SUPPORT=1 \
         FVP_USE_GIC_DRIVER=FVP_GICV5 ENABLE_SME2_FOR_NS=0 ENABLE_SME_FOR_NS=0 \
         ENABLE_SVE_FOR_NS=0 ARM_BL31_IN_DRAM=1 CTX_INCLUDE_AARCH32_REGS=0
```

Please check [TF-A documents for GICv5 for FVP platform](https://github.com/ARM-software/arm-trusted-firmware/blob/master/docs/plat/arm/fvp/fvp-specific-configs.rst#gicv5-support).


#### Building the FIP image with GICv5
The FIP image should be generated with the following additional for GICv5:
```
   --hw-config  $TF_A_DIR/build/fvp/<debug|release>/fdts/fvp-base-gicv5-psci.dtb
```

e.g.
```
cd tf-a
./tools/fiptool/fiptool --verbose update \
   --tb-fw $TF_A_DIR/build/fvp/debug/bl2.bin \
   --soc-fw $TF_A_DIR/build/fvp/debug/bl31.bin \
   --tos-fw ${WORKSPACE}/Build/ArmVExpress-FVP-AArch64/DEBUG_GCC/FV/BL32_AP_MM.fd \
   --nt-fw ${WORKSPACE}/Build/ArmVExpress-FVP-AArch64/DEBUG_GCC/FV/FVP_AARCH64_EFI.fd \
   --hw-config  $TF_A_DIR/build/fvp/debug/fdts/fvp-base-gicv3-psci.dtb \
   --tos-fw-config  $TF_A_DIR/build/fvp/debug/fdts/fvp_stmm_manifest.dtb \
   fip_fvp.bin

   --tb-fw $TF_A_DIR/build/fvp/debug/bl2.bin \
   --soc-fw $TF_A_DIR/build/fvp/debug/bl31.bin \
   --nt-fw ${WORKSPACE}/Build/ArmVExpress-FVP-AArch64/DEBUG_GCC/FV/FVP_AARCH64_EFI.fd \
   --hw-config $TF_A_DIR/build/fvp/debug/fdts/fvp-base-gicv5-psci.dtb \
   --fw-config $TF_A_DIR/build/fvp/debug/fdts/fvp_fw_config.dtb \
   --nt-fw-config $TF_A_DIR/build/fvp/debug/fdts/fvp_nt_fw_config.dtb \
   --soc-fw-config $TF_A_DIR/build/fvp/debug/fdts/fvp_soc_fw_config.dtb \
   --tb-fw-config  $TF_A_DIR/build/fvp/debug/fdts/fvp_tb_fw_config.dtb
   fip_fvp.bin
```

### How to run FVP RevC model with GICv5

#### Command to run Model and parameters
```
TFA=/gicv5/tfa-repo
FS=/gicv5/fs
CONF=/gicv5/conf
LINUX=/gicv5/linux

FVP_Base_RevC-2xAEMvA_GICV5	\
		-C pctl.startup=0.0.0.0 \
		-C bp.virtio_rng.enabled=1 \
		-C cluster0.NUM_CORES=4 \
		-C cluster0.has_delayed_sysreg=0 \
		-C cluster1.NUM_CORES=4 \
		-C cluster1.has_delayed_sysreg=0 \
		-C cache_state_modelled=0 \
		-C bp.secure_memory=0 \
		-C bp.pl011_uart0.uart_enable=1 \
		-C bp.pl011_uart0.untimed_fifos=1 \
		-C bp.pl011_uart0.unbuffered_output=1 \
		-C bp.secureflashloader.fname=$TFA/build/fvp/debug/bl1.bin \
		-C bp.flashloader0.fname=$TFA/build/fvp/debug/fip.bin \
		--data cluster0.cpu0=$LINUX/Image@0x84000000 \
		-C pci.pcie_rc.ahci0.ahci.image_path=$FS/rootfs.ext3 \
		-C bp.virtioblockdevice.image_path=$FS/diskvio.ext3 \
		-C gicv5_config_file=$CONF/gicv5.yaml
```

#### gicv5.yaml
```
---
  name: gicv5_config
  version: 1

  GIC_TOP:
    - pa_range: 6 # 0b0110   52 bits, 4PB (should match the 'System' PA size)

      IWB:
        - name: "iwb0"
          config_frame_base_address: 0x2F000000
          target_itsid: 0
          num_wires: 64        # 16 bit value
          device_id: 64        # 16 bit value
          domains: 7

      ITS:
        - name: "its0"
          itsid: 0
          target_irsid: 0
          device_id_bits: 20                          # The maximum permitted value of this field is 32 (dec).
          event_id_bits: 0x10                         # The maximum permitted value of this field is 32 (dec).
          device_table_levels: 0x01                   # 0b00 - linear DT only. 0b01 linear and 2-level DT supported.
          interrupt_translation_table_levels: 0x01    # 0b00 - linear ITT only. 0b01 linear and 2-level ITT supported.
          has_swerr_reporting: false
          domains:
              - type: Non_Secure
                config_frame_base_address: 0x2F120000
                translate_frame_base_addresses: [0x2F130000]
              - type: Secure
                config_frame_base_address: 0x2F100000
                translate_frame_base_addresses: [0x2F110000]
              - type: EL3
                config_frame_base_address: 0x2F140000
                translate_frame_base_addresses: [0x2F150000]

      IRS:
        COMMON:
          spi_range: 256                        # SPI range supported across all the IRSs.
          support_setlpi_frame: true            # Implement set LPI register frame.
          min_lpi_id_bits: 0                    # The minimum number of LPI ID Bits supported. (The maximum value supported for this field is 14.)
          max_lpi_id_bits: 24                   # The maximum number of LPI ID Bits supported. (The maximum value supported for this field is 24.)
          ist_levels: 2                         # Levels supported for the IST, possible values [1 - 2], '1' is the default, '2' means 2-level structure is supported.
          istmd: false                          # Reports whether the IRS stores metadata in the level 2 ISTEs, default is 'false' which means that IST entries don't require storage for metadata.
          ist_splits: 7                         # Supported split values when a 2-level IST structure is used. possible values are from 1 to 7, '1' is default means Level 2 IST sizes supported:4KB
          istmd_sz: 0                           # Minimum number of LPI ID bits which requires a level 2 ISTE size of 16 bytes to store metadata.
        INSTANCES:
          - name: "irs0"
            irsid: 0
            spi_irs_range: 256                  # SPI range supported for this IRS instance.
            spi_base: 0                         # The minimum SPI ID implemented for this IRS instance.
            domains:
              - config_frame_base_address: 0x2F1A0000
                lpi_frame_base_address: 0x2F1B0000
                type: Non_Secure
              - config_frame_base_address: 0x2F180000
                lpi_frame_base_address: 0x2F190000
                type: Secure
              - config_frame_base_address: 0x2F1C0000
                lpi_frame_base_address: 0x2F1D0000
                type: EL3
            # The affinities of the PEs connected to this IRS instance [ the order should be matching the platform connections in the LISA file].
            processing_element_affinities: [0, 1, 2, 3, 4, 5, 6, 7]


  CPU_INTERFACE:
    - core_id: 0  # Core ID of the Core implementing the CPUIF (starting from 0)
      has_gicv5_legacy: false
      supported_int_id_bits: 16
      number_of_non_arch_ppis_implemented: 0    # Number of non-architected PPIs to be implemented starting from the PPI ID 64.
    - core_id: 1
      has_gicv5_legacy: false
      supported_int_id_bits: 16
      number_of_non_arch_ppis_implemented: 0
    - core_id: 2
      has_gicv5_legacy: false
      supported_int_id_bits: 16
      number_of_non_arch_ppis_implemented: 0
    - core_id: 3
      has_gicv5_legacy: false
      supported_int_id_bits: 16
      number_of_non_arch_ppis_implemented: 0
    - core_id: 4
      has_gicv5_legacy: false
      supported_int_id_bits: 16
      number_of_non_arch_ppis_implemented: 0
    - core_id: 5
      has_gicv5_legacy: false
      supported_int_id_bits: 16
      number_of_non_arch_ppis_implemented: 0
    - core_id: 6
      has_gicv5_legacy: false
      supported_int_id_bits: 16
      number_of_non_arch_ppis_implemented: 0
    - core_id: 7
      has_gicv5_legacy: false
      supported_int_id_bits: 16
      number_of_non_arch_ppis_implemented: 0
```
