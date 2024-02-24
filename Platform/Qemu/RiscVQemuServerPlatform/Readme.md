# RISC-V UEFI PI Reference Board Port Guide
The goal of this document is to provide a generic server platform firmware solution applicable to systems built on the RISC-V architecture SOC.

## INDEX
* 1 [Overview](#1-Overview)
* 2 [Server SOC Reference Model](#2-Server-SOC-Reference-Model)
* 3 [Boot Flow](#3-Boot-Flow)
* 4 [Verification](#4-Verification)
* 5 [Pending Tasks](#5-Pending-Tasks)
* 6 [Known Issues](#6-Known-Issues)
* 7 [Appendix](#7-Appendix)


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

### 1.2	Target Audience
This document is intended for the following readers:
* IHVs and OSVs who actively engaged in the building of the RISC-V ecosystem, serving as a vital component in the vertical integration of systems.
* Bios developers, either those who create general-purpose BIOS and other firmware products or those who modify these products for use in various vendor architecture-based products.
* Other stakeholders who are interested in the RISC-V platform and have firmware development needs.

### 1.3	Terminology

| Term                 | Description
|:-------------------- |:-------------------------
| UEFI                 | Unified Extensible Firmware Interface
| SBI                  | Supervisor Binary Interface
| BRS                  | Boot and Runtime Services
| BMC                  | Baseboard Management Component BMC's main function is to automatically monitor the system platform management events, the events recorded in the system event log.
| OOB                  | Out of Band
| RVI                  | [RISC-V International](https://riscv.org/)
| RISE                 | [RISC-V Software Ecosystem](https://wiki.riseproject.dev/display/HOME/About+RISE)
| SCT                  | UEFI Self Certification Tests 
| FWTS                 | Firmware Test Suite
| SPL                  | U-Boot Second Program Loader
| SAL                  | System Abstraction Layer


## 2 Server SOC Reference Model

### 2.1 Requirements
This qemu virtual machine (server soc reference board) is required to be compliant with RISC-V [Server SOC Spec](https://github.com/riscv-non-isa/server-soc) (the commit id at writing this doc is aec20046c35b108f76a11a751f754505aaad7800) as much as possible.

#### <caption>Table 1 Server SOC Requirement

| Items          | Requirements | On qemu reference board (1st version)| Future Plan
|:-------------------- |:-------------------------|:-------------------- |:-------------------------
| 2.1. Clocks and Timers | time CSR | supported 
| 2.2. External Interrupt Controllers | AIA | supported with non-upstream patches 
| 2.3. Input-Output Memory Management Unit (IOMMU) | RISC-V IOMMU spec | no support (need qemu/linux patches) | Yes
| 2.4. PCIe Subsystem Integration | PCIe features | supported using existing qemu implementation | 
| 2.5. Reliability, Availability, and Serviceability (RAS) | The level of RAS implemented by the SoC is UNSPECIFIED | not considered
| 2.6. Quality of Service | capacity and bandwidth controller register interface (CBQRI), etc. | not considered | 
| 2.7. Manageability | BMC | no support | TBD
| 2.8. Performance Monitoring | HPM counters | supported | 
| 2.9. Security Requirements | On PCIe, off-chip DRAM and TPM | no support (follow qemu existing features) | Yes (for TPM)
| Harts features | (removed from this spec) | support the previous stated hart features | 


### 2.2 High Level Design
The Implementation Choices
* Make the configuration as fixed as possible so that this new machine is easy-to-go and less confusing.
* Remove the unnecessary devices as many as possible, e.g. CLINT/PLIC are removed.
* Keep the MemMap entries as similar as RiscVVirt vm for easy adoption at the early stage.
* Keep dtb entries as small as possible.
#### <caption>Table 2 Devices and Memory Mappings

| Devices          | Base Addr | Size | Comment 
|:-------------------- |:-------------------------|:-------------------- |:-------------------------
| MROM | 0x1000 | 0xf000 | 
| TEST | 0x100000 | 0x1000 | For reset function (name misleading?)
| RTC | 0x101000 | 0x1000 | 
| ACLINT | 0x2000000 | 0x10000 | aclint mtimer (spec definition missed)
| PCIE_PIO | 0x3000000 | 0x10000 | 
| APLIC_M | 0xc000000 | APLIC_SIZE(SSOC_CPUS_MAX) | fixed AIA
| APLIC_S | 0xd000000 | APLIC_SIZE(SSOC_CPUS_MAX) | 
| UART0 | 0x10000000 | 0x100 | 
| FW_CFG | 0x10200000 | 0x18 | depend on ACPI support on firmware
| FLASH | 0x20000000 | 0x4000000 | 
| IMSIC_M | 0x24000000 | SSOC_IMSIC_MAX_SIZE | 
| IMSIC_S | 0x28000000 | SSOC_IMSIC_MAX_SIZE | 
| PCIE_ECAM | 0x30000000 | 0x10000000 | 
| PCIE_MMIO | 0x40000000 | 0x40000000 | 
| DRAM | 0x80000000 | 0xff80000000ull (max) | still one continuous range
| VIRT64_HIGH_PCIE_MMIO | 0x10000000000ull | 0x10000000000ull | different from RiscVVirt
| AHCI | Handled as the PCIe device |   | 
| EHCI | Handled as the PCIe device |   | 
| IOMMU | to be decided |  |  patch yet to integrate (kernel/qemu/…)

### 2.3 Qemu/Guest FW Interface
#### Hardcode Addresses
It's possible qemu and guest have no explicit interface about some information, e.g. address of specific devices, but both of them hardcodes the same address to access the device.

## 3 Boot Flow
The following diagram illustrates various platform initialization scenarios. This document will not cover the detailed work of initializing on real hardware platforms, as it is beyond its scope. Our focus will be on the more general firmware initialization tasks performed on the qemu emulator. See the part of the diagram indicated by the blue color, which corresponds to QemuServerPlatform Boot Flow.

Note: _For specifics on the qemu Server SOC reference model in this document, it is essential to consult both the latest developments in the qemu source code and the definition in the server platform specifications. Relevant information for both can be obtained from Server SOC TG and Server Platform TG of RVI.c_

#### <caption>Figuire 1 RISC-V Platform EDK2 Firmware Enabling Philosophy

![RISC-V_Platform_EDK2_Firmware_Enabling_Philosophy](https://github.com/ChaiEvan/edk2-platforms/blob/RV_ServerPlatformRef_v1/Platform/Qemu/RiscVQemuServerPlatform/Documents/Media/RISC-V_Platform_EDK2_Firmware_Enabling_Philosophy.jpg)

### 3.1 The Traditional Boot Flow

[PI Architecture Firmware Phases](https://uefi.org/specs/PI/1.8/V2_Overview.html#pi-architecture-firmware-phases) shows the phases that a platform with PI Architecture firmware will execute.

#### <caption>Figuire 2 PI Architecture Firmware Phases

![PI_Boot_Phases](https://github.com/ChaiEvan/edk2-platforms/blob/RV_ServerPlatformRef_v1/Platform/Qemu/RiscVQemuServerPlatform/Documents/Media/PI_Boot_Phases.jpg)

_In a PI Architecture firmware implementation, the phase executed prior to DXE is PEI. This specification covers the transition from the PEI to the DXE phase, the DXE phase, and the DXE phase’s interaction with the BDS phase. The DXE phase does not require a PEI phase to be executed. The only requirement for the DXE phase to execute is the presence of a valid HOB list. There are many different implementations that can produce a valid HOB list for the DXE phase to execute. The PEI phase in a PI Architecture firmware implementation is just one of many possible implementations._

Based on the content quoted from the PI specification above, it is evident that the PEI phase is merely a traditional and integral part of the UEFI boot flow. In actual PI Architecture firmware implementations, it is not mandatory. Currently, the primary purpose of the PEI Phase is to perform memory initialization and pass necessary HOBs to DXE. 

For the former, the alternative solution for the former involves the implementation of OOB firmware, which is currently beyond the scope of this document. Regarding the latter, we can move more works for building Hand-Off Blocks (HOBs) to the SEC phase. All of these considerations collectively contribute to providing additional insights and flexibility for the practical initiation of hardware.

### 3.2 Alternative Boot Flow
The boot flow without the PEI phase, also known as the Pei-Less flow, is the initialization approach that will be further discussed in this document. Detailed standards for Pei-Less can be obtained from RISE's Firmware TG.(See [Boot Flow](https://wiki.riseproject.dev/display/HOME/EDK2_00_18+-+RISC-V+QEMU+Server+Reference+Platform?preview=/25395218/25395220/EDK2%20implementation%20choices%20for%20RISC-V%20platforms.pdf) in Project No. EDK2_00_18)

#### 3.2.1 SEC Phase
See [HOB Translations](https://uefi.org/specs/PI/1.8/V2_DXE_Foundation.html#hob-translations) for more information on HOB types.

#### <caption>Figuire 3 HOB List
![V2_DXE_Foundation-2](https://github.com/ChaiEvan/edk2-platforms/blob/RV_ServerPlatformRef_v1/Platform/Qemu/RiscVQemuServerPlatform/Documents/Media/Hand-Off-Block-Hob-List.jpg)

The HOBs list, as mandated by the PI specification, is illustrated in Figure 2.3. It is recommended to refer to the SecMain structure in RiscVVirt for guidance.

Figure 2.3 displays the HOBs list, a prerequisite set by the PI specification for readiness before entering the DXE phase. For additional insights, it is recommended to refer to the SecMain module in RiscVVirt.

Regarding memory initialization, specific details are intentionally omitted in this document. The recommended strategy involves the use of OOB firmware, like SPL. This provides input to the System Memory HOB generated by the UEFI firmware during the SEC phase, and the input for HOB is extracted from static data in the device tree, including entries like 'memory' by AddMemoryBaseSizeHob() and 'reserved-memory BuildMemoryAllocationHob().' 

The creation of HOBs for IO, MMIO, and other resources provides SOC vendors with the flexibility to customize according to their chip specifications. The PopulateIoResources() function provides a straightforward approach to achieve this customization.

It is essential to emphasize that RiscVVirt adopts a separate Flash Descriptor (FD) approach for firmware code and Variables. The advantage is clear as it allows for the protection of the firmware code by setting it as read-only, while conferring write permissions exclusively to the Variable FD, effectively ensuring the security of the firmware. Particularly, this approach provides convenient and flexible control, especially when dealing with firmware upgrade actions in the future. Of course, in qemu-based firmware development, virtualizing two or even more flashes is extremely straightforward. However, it's worth mentioning that in many real hardware platforms, the single-flash configuration is more common, and further elaboration on this point is not necessary here.

In the upcoming server platform solution, we will continue to adopt a similar approach as RiscVVirt. However, there is a slight difference in the treatment of the Variable FD. We prefer to establish a corresponding Memory HOB for the Variable's scope during the SEC phase, 

The advantage of this adjustment is that there is no longer a strong dependency between VariableRuntimeDxe and VirtNorFlashDxe. We no longer need to ensure that VirtNorFlashDxe is dispatched before VariableRuntimeDxe. To achieve this purpose, the following two steps must be taken: 
* Build Memory Hob for variable region by BuildResourceDescriptorHob() in SEC phase.
* Replace the version of Ovfm with a new VirtNorFlashDxe driver or remove its GCD mapping related code. To avoid compatibility issues with other platforms, the first implementation approach is adopted in this document.

#### 3.2.2 DXE Phase
The following table supplements the missing implementation details in Table 2.5 of the PI specification. The DXE Foundation is abstracted from the platform through the DXE Architectural Protocols. The DXE Architectural Protocols manifest the platform-specific components of the DXE Foundation. DXE drivers that are loaded and executed by the DXE Dispatcher component of the DXE Foundation must produce these protocols.

For implementations outside of the EmbeddedPkg solutions, please follow the guidance outlined in the table.


#### <caption>Table 3 DXE Architectural Protocols
Protocol Name|Driver Path in use|Driver Compatibility __1__|Task required
|:--------------------|:-------------------------|:--------------------|:--------------------
|Security Architectural|MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf|Platform independent|no extra work
|CPU Architectural|UefiCpuPkg/CpuDxeRiscV64/CpuDxeRiscV64.inf|Platform dependent|no extra work
|Metronome Architectural|EmbeddedPkg/MetronomeDxe/MetronomeDxe.inf (__Note__: Precision in the EmbeddedPkg version is limited to microseconds, whereas the MdeModulePkg version offers precision down to nanoseconds.)|Platform dependent|no extra work
|Timer Architectural|UefiCpuPkg/CpuTimerDxeRiscV64/CpuTimerDxeRiscV64.inf (__Note__: In S mode, utilize the TIME CSR for implementing GetTimer, and achieve SetTimer by the ‘Set Timer’ interface of SBI Timer Extension 6.1. Please note that the Runtime type of timer service isn’t presently supported yet. Runtime type of timer service will be produced by RealTimeClock.)|Platform dependent|no extra work
|BDS Architectural|MdeModulePkg/Universal/BdsDxe/BdsDxe.inf|Platform independent|no extra work
|Watchdog Timer Architectural|MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf (__Note__: It depends on TimerLib and ResetLib, the implementation of which is typically determined by the different CPU architectures.)|Platform independent|no extra work
|Runtime Architectural|MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf (__Note__: Some functionalities in the RISC-V version of CacheMaintenanceLib are still under development.)|Platform independent|need to complete the remaining functions of CacheMaintenanceLib.
|Variable Architectural|MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf (__Note__: The dependency issue on VirtNorFlashDxe in RiscVVirt has been clarified in the preceding chapters. Please refer to Section 2.10 for details.)|Platform independent|no extra work
|Variable Write ArchitecturalProtocol|MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf | Platform independent|no extra work
|Monotonic Counter Architectural|MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf|Platform independent|no extra work
|Reset Architectural|MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf (__Note__: ResetSystemLib implements warm reset, cold reset, and shutdown through the SBI interface.)|Platform independent|no extra work
|Real Time Clock Architectural|EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf (__Note__: Some features in RealTimeClockLib remain unimplemented, including GetWakeUpTime/SetWakeUpTime. The present RealTimeClockLib is specifically designed for qemu; future adaptations should implement their own RTC timer based on the platform's needs.)|Platform dependent|no extra work
|Capsule Architectural Protocol|MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf (__Note__: It depends on CacheMaintenanceLib and ResetLib.)|Platform independent|no extra work

Note:
_1.The 'Driver Compatibility, task required' column addresses platform dependencies exclusively at the DXE driver level. The platform-specificity of the consumed libraries does not influence the conclusion._

#### 3.2.3 Network Stack
The Server SOC model currently in use, 'ssoc_ref,' comes with e1000 as the default NIC device. However, as this device lacks an integrated UEFI UNDI driver, it cannot provide the foundational services to enable the network. From a testing perspective, here are two recommended steps：
* Build UndiRuntimeDxe to your firmware FD, with the following patch: edk2-platforms/Drivers/OptionRomPkg/UndiRuntimeDxe/UndiRuntimeDxe.inf
* Append NIC ‘i82557b’ to the qemu command line. See the reference command in Chapter 4.

From a product implementation perspective, it is not recommended to directly use UndiRuntimeDxe as the provider for the UNDI services. It is more appropriate to provide the UNDI services based on the NIC device integrated into the platform.

### 3.3 Extra work for Fdt-to-OS
The DXE Foundation produces the UEFI System Table, and the UEFI System Table is consumed by every DXE driver and executable image invoked by the DXE Dispatcher and BDS. It contains all the information required for these components to utilize the services provided by the DXE Foundation and the services provided by any previously loaded DXE driver. [UEFI System Table and Related Components](https://uefi.org/specs/PI/1.8/V2_DXE_Foundation.html#uefi-system-table-and-related-components) shows the various components that are available through the UEFI System Table.

#### <caption>Figuire 4 UEFI System Table and Related Components
![V2_DXE_Foundation-3](https://github.com/ChaiEvan/edk2-platforms/blob/RV_ServerPlatformRef_v1/Platform/Qemu/RiscVQemuServerPlatform/Documents/Media/V2_DXE_Foundation-3.jpg)

As shown in figure 9.2, the UEFI Configuration Tables are an extensible list of tables that describe the configuration of the platform. Today, this includes pointers to tables such as DXE Services, the HOB list, ACPI table, SMBIOS table, and the SAL System Table. This list may be expanded in the future as new table types are defined. 

Due to the broad usage of device trees in embedded firmware products, several chip manufacturers in the upstream and downstream supply chains use static data from the device tree to initialize specific peripherals. For compatibility reasons, it is essential to install the device tree pointer into the system configuration table. Two DXE drivers are available as references:

#### <caption>Table 4 The Proposal for FdtTabGuid
|Table Guid |Driver Path in use |Comments
|:-------------------- |:-------------------------|:--------------------
|gFdtTableGuid |EmbeddedPkg/Drivers/FdtClientDxe/FdtClientDxe.inf |Using in RiscVVirt
|gFdtTableGuid |Silicon/RISCV/ProcessorPkg/Universal/FdtDxe/FdtDxe.inf |Recommend using the streamlined DXE driver if no required for FdtClientProtocl

### 3.4 DT File Decoding 
The DXE driver FdtClientDxe, mentioned in the previous section, supplies a limited set of APIs for parsing DT binary, handling fundamental DT data extraction tasks. 

Users with more advanced demands can turn to another DXE service produced by [FdtBusPkg](https://github.com/intel/FdtBusPkg), which offers an extensive array of APIs to meet the requirements of more complex scenarios. Detailed information is accessible in the firmware TG of RISE community (Project No. [EDK2_00_03](https://wiki.riseproject.dev/display/HOME/Firmware+Projects)), and the source code is available from [FdtBusPkg](https://github.com/intel/FdtBusPkg) repository. It will be upstreamed to the [edk2](https://github.com/tianocore/edk2) repository in the future, and any suggestions or requirements for improvement are encouraged before this transition.

### 3.5 OpenSbi
It is recommended to align with the version of OpenSBI in the [RISC-V BRS Development Suite Repository](https://github.com/intel/rv-brs-test-suite). The binary file can be found at the following path:  ./rv-brs-test-suite/brsi/scripts/opensbi/build/platform/generic/firmware/fw_dynamic.bin

__Or build it by the command example1__:
```
cd opensbi
make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- PLATFORM=generic
```
### 3.6 OS Image 
Similarly, the BRS Repo offers an OS image based on RISC-V. To validate the boot flow, you can make use of the prebuilt images located in the test suite at /rv-brs-test-suite/brsi/prebuilt_images/. For those interested in building their own image, please refer to the repository guidance for detailed steps.

## 4 Verification
The tests covered in this document are based on the BRS spec, focusing on two primary modules: SCT and FWTS. The relevant test scripts, pre-build image and guidance can be obtained from the [RISC-V BRS Development Suite Repository](https://github.com/intel/rv-brs-test-suite):

__Command example2__:
```   
 ./qemu-system-riscv64 -nographic -m 8G -smp 2 \
 -machine ssoc_ref,pflash0=pflash0,pflash1=pflash1 \
 -blockdev node-name=pflash0,driver=file,read-only=on,filename=$FW_DIR/RISCV_SP_CODE.fd \
 -blockdev node-name=pflash1,driver=file,filename=$FW_DIR/RISCV_SP_VARS.fd \
 -bios $Sbi_DIR/fw_dynamic.bin \
 -drive file=$Img_DIR/brs_live_image.img,if=ide,format=raw
```

__Note__:
* _‘ssoc_ref’ is a specified qemu-based SOC model, whose source code is still under development and will be accessible from the RVI staging repository later._
* _The Pre-build image ‘brs_live_image.img,if’ can be downloaded in RISC-V BRS Development Suite repository, or you can build it by yourself. See 3.6_
* _‘-bios $Sbi_DIR/fw_dynamic.bin’ the parameter points to the opensbi path. See more in 3.5._

In general, a series of modules related to Network are enabled by default. However, during SCT test execution, it is noticed that most Network test items fail to pass. The most likely reason is the lack of a UNDI driver built in the current platform firmware codebase. So, if you intend to enable the edk2 network stack with QEMU in the boot flow, it is suggested to use the following command:

__Command example3__:
```
  Command example2 \
  -device i82557b,netdev=net2 \
  -netdev type=user,id=net2
```

## 5 Pending Tasks
The listed items in the table represent ongoing firmware development tasks that are still unfinished. Some specifications are in the process of refinement, and a few are yet to be drafted. Please refer to subsequent updates from the RISE community for more information.
### 5.1 Bios Requirements

#### <caption>Table 5 Bios Requirements
ID | Requirement | Comment
|:-------------------- |:-------------------------|:--------------------
|1 | ACPI enabling | The establishment of all ACPI tables is currently achieved through qemu, leveraging a series of libraries such as QemuFwCfgLib to interact with the Linux kernel. The following task is to implement the publishment of all ACPI tables solely through firmware.
|2 | Smbios | The upcoming tasks for SMBIOS should be handled exclusively through firmware publishment, replacing the existing qemu method entirely.
|4 | FWTS test | Upon the completion of tasks 1 and 2, the pass ratio for FWTS should be maintained at a specific benchmark.
|5 | Security | Not started 
|6 | TPM | Not started 

## 6 Known Issues
The following table outlines the current known issues, which will be resolved gradually during the subsequent phases of development.

#### <caption>Table 6 Known Issues
No. | Issue Description | Cause | Comment
|:--------------------|:-------------------------|:--------------------|:--------------------
|1 | Graphic device doesn’t work | ‘Ssoc_ref’ doesn’t provide a default GPU yet | Will do in next phase
|2 | ACPI/Smbios don’t work | Not firmware implementation | Will do in next phase

## 7 Appendix



### 7.1 Building and running based on BRS environment

1. Build brs test suit
```
git clone https://github.com/intel/rv-brs-test-suite.git
```

Please refer to detailed build steps from [README.md](https://github.com/intel/rv-brs-test-suite/blob/main/README.md), then you will get the following stuff:
```
 QEMU_DIR=$WORKSPACE/rv-brs-test-suite/brsi/scripts/qemu/build
 BRS_IMG_DIR=$WORKSPACE/rv-brs-test-suite/brsi/scripts/output
 OPENSBI_DIR=$WORKSPACE/rv-brs-test-suite/brsi/scripts/opensbi/build/platform/generic/firmware
 EDK2_DIR=$WORKSPACE/rv-brs-test-suite/brsi/scripts/Build/RiscVQemuServerPlatform/DEBUG_GCC5/FV
```

2. Boot Execution(See Command example2)
```
$QEMU_DIR/qemu-system-riscv64 \
 -machine ssoc_ref,pflash0=pflash0,pflash1=pflash1 \
 -blockdev node-name=pflash0,driver=file,read-only=on,filename=$EDK2_DIR/RISCV_SP_CODE.fd \
 -blockdev node-name=pflash1,driver=file,filename=$EDK2_DIR/RISCV_SP_VARS.fd \
 -bios $OPENSBI_DIR/fw_dynamic.bin \
 -drive file=$BRS_IMG_DIR/brs_live_image.img,if=ide,format=raw
```
### 7.2 Compiling edk2 firmware separately outside of BRS
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

## Contributors
- Evan Chai <evan.chai@intel.com>
