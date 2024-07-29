# RISC-V UEFI Server Reference Board
The goal of this document is to provide a generic server platform firmware solution applicable to systems built on the RISC-V architecture SOC.

## INDEX
* 1 [Overview](#1-Overview)
* 2 [Server SOC Reference Model](#2-Server-SOC-Reference-Model)
* 3 [Boot Flow](#3-Boot-Flow)
* 4 [Development Environment Setup](#4-Development-Environment-Setup)
* 5 [WIP and Pending Tasks](#6-WIP-and-Pending-Tasks)
* 6 [Known Issues](#7-Known-Issues)

## 1 Overview
### 1.1 References
* UEFI Specification v2.10: https://uefi.org/sites/default/files/resources/UEFI_Spec_2_10_Aug29.pdf
* UEFI PI Specification v1.8.0: https://uefi.org/sites/default/files/resources/UEFI_PI_Spec_1_8_March3.pdf
* ACPI Specification v6.5: https://uefi.org/sites/default/files/resources/ACPI_Spec_6_5_Aug29.pdf
* OpenSbi Specification: https://github.com/riscv-software-src/opensbi
* BRS Specification: https://github.com/riscv-non-isa/riscv-brs
* Device Tree Specification v0.4: https://www.devicetree.org/
* Smbios Specification v3.7.0: https://www.dmtf.org/sites/default/files/standards/documents/DSP0134_3.7.0.pdf
* Server Soc Specification: https://github.com/riscv-non-isa/server-soc
* Server Platform Specification: https://github.com/riscv-non-isa/riscv-server-platform

### 1.2 Target Audience
This document is intended for the following readers:
* IHVs and OSVs who actively engaged in the building of the RISC-V ecosystem, serving as a vital component in the vertical integration of systems.
* Bios developers, either those who create general-purpose BIOS and other firmware products or those who modify these products for use in various vendor architecture-based products.
* Other stakeholders who are interested in the RISC-V platform and have firmware development needs.

### 1.3 Terminology

| Term                 | Description
|:-------------------- |:-------------------------
| UEFI                 | Unified Extensible Firmware Interface
| SBI                  | Supervisor Binary Interface
| BRS                  | Boot and Runtime Services
| RVI                  | [RISC-V International](https://riscv.org/)
| RISE                 | [RISC-V Software Ecosystem](https://wiki.riseproject.dev/display/HOME/About+RISE)
| SCT                  | UEFI Self Certification Tests
| FWTS                 | Firmware Test Suite


## 2 Server SOC Reference Model
This chapter introduces the hardware-level topology of ‘rvsp-ref’, allowing users to gain insights into the device list and resource allocation under this model.

### 2.1 Requirements
This qemu virtual machine (server soc reference board) is required to be compliant with RISC-V [Server SOC Spec](https://github.com/riscv-non-isa/server-soc) as much as possible. The details of Server SOC Requrement can be found in [Table: Server SOC Requirement](https://wiki.riseproject.dev/display/HOME/EDK2_00_18+-+RISC-V+QEMU+Server+Reference+Platform#EDK2_00_18RISCVQEMUServerReferencePlatform-Table1ServerSOCRequirement).

### 2.2 High Level Design
The Implementation Choices
* Make the configuration as fixed as possible so that this new machine is easy-to-go and less confusing.
* Remove the unnecessary devices as many as possible, e.g. CLINT/PLIC are removed.
* Keep the MemMap entries as similar as RiscVVirt vm for easy adoption at the early stage.
* Keep dtb entries as small as possible.

Refer to [Table: Devices and Memory Mappings](https://wiki.riseproject.dev/display/HOME/EDK2_00_18+-+RISC-V+QEMU+Server+Reference+Platform#EDK2_00_18RISCVQEMUServerReferencePlatform-Table2DevicesandMemoryMappings) for the details.

## 3 Boot Flow
The following diagram illustrates various platform initialization scenarios. This document will not cover the detailed work of initializing on real hardware platforms, as it is beyond its scope. Our focus will be on the more general firmware initialization tasks performed on the qemu emulator. See the part of the diagram indicated by the blue color, which corresponds to QemuServerPlatform Boot Flow.

Note: _For specifics on the qemu Server SOC reference model in this document, it is essential to consult both the latest developments in the qemu source code and the definition in the server platform specifications. Relevant information for both can be obtained from Server SOC TG and Server Platform TG of RVI._

#### <caption>Figuire 1 RISC-V Platform EDK2 Firmware Enabling Philosophy

![RISC-V_Platform_EDK2_Firmware_Enabling_Philosophy](https://wiki.riseproject.dev/download/attachments/25395218/RISC-V_Platform_EDK2_Firmware_Enabling_Philosophy_v3.2.JPG?version=2&modificationDate=1722239408110&api=v2)

For a more detailed introduction to the background and specifics of Boot-Flow, please refer to [The Traditional Boot Flow](https://wiki.riseproject.dev/display/HOME/EDK2_00_18+-+RISC-V+QEMU+Server+Reference+Platform#EDK2_00_18RISCVQEMUServerReferencePlatform-3.1TheTraditionalBootFlow) and [The Alternative Boot Flow](https://wiki.riseproject.dev/display/HOME/EDK2_00_18+-+RISC-V+QEMU+Server+Reference+Platform#EDK2_00_18RISCVQEMUServerReferencePlatform-3.2AlternativeBootFlow). To understand the design concepts of other modules, refer to the remaining sections of [this chapter](https://wiki.riseproject.dev/display/HOME/EDK2_00_18+-+RISC-V+QEMU+Server+Reference+Platform#EDK2_00_18RISCVQEMUServerReferencePlatform-3BootFlow).

## 4 Development Environment Setup

### 4.1 Compiling edk2 firmware separately outside of BRS (Manually)
1. Building the RISC-V edk2 server platform

```
git clone https://github.com/tianocore/edk2.git
cd edk2
git submodule update --init
cd ..

git clone https://github.com/tianocore/edk2-platforms.git
cd edk2-platforms
git submodule update --init
cd ..

export WORKSPACE=`pwd`
export GCC5_RISCV64_PREFIX=riscv64-linux-gnu-
export PACKAGES_PATH=$WORKSPACE/edk2:$WORKSPACE/edk2-platforms

cd edk2
make -C BaseTools clean
make -C BaseTools
source edksetup.sh
./edksetup.sh

build -a RISCV64 -t GCC5 -p Platform/Qemu/RiscVQemuServerPlatform/RiscVQemuServerPlatform.dsc
```
2. Convert FD files
```
 truncate -s 32M Build/RiscVQemuServerPlatform/DEBUG_GCC5/FV/RISCV_SP_CODE.fd
 truncate -s 32M Build/RiscVQemuServerPlatform/DEBUG_GCC5/FV/RISCV_SP_VARS.fd
```
3. Boot Execution

__Command example1__:
```
 ./qemu-system-riscv64 -nographic -m 8G -smp 2 \
 -machine rvsp-ref,pflash0=pflash0,pflash1=pflash1 \
 -blockdev node-name=pflash0,driver=file,read-only=on,filename=$FW_DIR/RISCV_SP_CODE.fd \
 -blockdev node-name=pflash1,driver=file,filename=$FW_DIR/RISCV_SP_VARS.fd \
 -bios $Sbi_DIR/fw_dynamic.bin \
 -drive file=$Img_DIR/brs_live_image.img,if=ide,format=raw
```
__Note__:
* _‘rvsp-ref’ is a specified qemu-based SOC model, whose source code is still under development and will be accessible from the RVI staging repository later. (The ongoing patch can be found in the [Appendix](https://wiki.riseproject.dev/display/HOME/EDK2_00_18+-+RISC-V+QEMU+Server+Reference+Platform#EDK2_00_18RISCVQEMUServerReferencePlatform-7Appendix))_
* _The Pre-build image ‘brs_live_image.img,if’ can be downloaded in RISC-V BRS Development Suite repository, or you can build it by yourself. See section [OS Image](https://wiki.riseproject.dev/display/HOME/EDK2_00_18+-+RISC-V+QEMU+Server+Reference+Platform#EDK2_00_18RISCVQEMUServerReferencePlatform-3.6OSImage)_
* _‘-bios $Sbi_DIR/fw_dynamic.bin’ the parameter points to the opensbi path. See more in section [OpenSbi](https://wiki.riseproject.dev/display/HOME/EDK2_00_18+-+RISC-V+QEMU+Server+Reference+Platform#EDK2_00_18RISCVQEMUServerReferencePlatform-3.5OpenSbi)._
* _In case the developer aims to boot just to the UEFI shell, the parameter '-drive file=$Img_DIR/brs_live_image.img,if=ide,format=raw' in the final line is not needed._

### 4.2 Building and running based on BRS environment (**Recommended but not mandatory**)

1. Build brs test suit
```
git clone https://github.com/intel/rv-brs-test-suite.git
```
The test suite's default Qemu model(virt) is established on a virtual hardware platform that leverages Virtio services provided by the Ovmf package. Obviously, to validate the current server platfor m with the test suite, modifications are required at minimum in both Qemu and the edk2 firmware.

For more details regarding changes to the two parts, please refer to the section [Building and running based on BRS environment](https://wiki.riseproject.dev/display/HOME/EDK2_00_18+-+RISC-V+QEMU+Server+Reference+Platform#EDK2_00_18RISCVQEMUServerReferencePlatform-4.2BuildingandrunningbasedonBRSenvironment(Automatically)).

2. Boot Execution

```
 cd rv-brs-test-suite/brsi/scripts/
 ./build-scripts/start_qemu.sh
```

### 4.3 Verification
The tests covered in this document are based on the BRS spec, focusing on two primary modules: SCT and FWTS. The relevant test scripts, pre-build image and guidance can be obtained from the [RISC-V BRS Development Suite Repository](https://github.com/intel/rv-brs-test-suite):

## 5 WIP and Pending Tasks
The listed items in the table represent ongoing firmware development tasks that are still unfinished. Some specifications are in the process of refinement, and a few are yet to be drafted. Please refer to subsequent updates from the RISE community for more information.

To see [Table: Bios Requirements](https://wiki.riseproject.dev/display/HOME/EDK2_00_18+-+RISC-V+QEMU+Server+Reference+Platform#EDK2_00_18RISCVQEMUServerReferencePlatform-Table5BiosRequirements) and [Table: Upcoming UEFI Features List](https://wiki.riseproject.dev/display/HOME/EDK2_00_18+-+RISC-V+QEMU+Server+Reference+Platform#EDK2_00_18RISCVQEMUServerReferencePlatform-Table6UpcomingUEFIFeaturesList)

## 6 Known Issues

The current known issues, which will be resolved gradually during the subsequent phases of development. Please refer to [Table: Known Issues](https://wiki.riseproject.dev/display/HOME/EDK2_00_18+-+RISC-V+QEMU+Server+Reference+Platform#EDK2_00_18RISCVQEMUServerReferencePlatform-Table7KnownIssues) for more details.

## Contributors
- Evan Chai <evan.chai@linux.alibaba.com>