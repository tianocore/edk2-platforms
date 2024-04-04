# Introduction

The Firmware Update Feature is based on
[Platform Security Firmware Update for the A-profile Specification 1.0][1] specification.
To update firmware, Firmware Update Feature uses FmpDevicePkg framework
for the firmware to be updated via capsule update framework.

Updates firmware with Capsule update framework in Arm with following steps:
  - Deliver firmware image via UpdateCapsule().
  - firmware image delivered to StandaloneMm via MmCommunication.
  - StandaloneMm which is UpdateAgent write new image in firmware update storage
    according to PSA specification.
  - To apply updated firmware, Reset.

This is slight different from other architecture which using
coalescing update firmware with following steps:
  - Deliver the firmware image via UpdateCapsule().
  - Save the firmware image in the variable storage.
  - Warm Reset, and In PEI phase coalesce the firmware image scattered in physical memory.
  - Before EndofDxe, update the firmware by calling ProcessCapsules()
  - To apply the updated firmware, Reset.
for preventing arbitrary access to firmware storage device locked after EndofDxe phase.

It's the reason Arm doesn't supports coalescing way because
    - According to platform UEFI doesn't run in ROM but
      it loaded to memory by TF-A
    - According to platform, it can skip PEICORE (See EDK2_SKIP_PEICORE)
    - Arm doesn't need to lock the firmware storage device because
      it's completely isolated in StandaloneMm (at S-EL0).
      Therefore, operating system, uefi or any other software components running in
      normal world cannot access isolated firmware storage.

By doing so, it can remove WarmReset for unlocking device and support runtime
firmware update in the future.

This document is written for platforms where firmware storage's layout
(typical platform is Base FVP platform):

    +----------------------+
    |      GPT-HEADER      |
    +----------------------+
    |    FIP_A (bank0)     |
    +----------------------+
    |    FIP_B (bank1)     |
    +----------------------+
    |    FWU-Metadata      |
    +----------------------+
    |  Bkup-FWU-Metadata   |
    +----------------------+

and uses FwsGptSystemFipLib used to access above firmware storage.

# Firmware Update Storage
For Base FVP platform, firmware update storage consists in the following:

    +----------------------+
    |      GPT-HEADER      |
    +----------------------+
    |    FIP_A (bank0)     |
    +----------------------+
    |    FIP_B (bank1)     |
    +----------------------+
    |    FWU-Metadata      |
    +----------------------+
    |  Bkup-FWU-Metadata   |
    +----------------------+

Cf. **[TF-A](https://www.trustedfirmware.org/projects/tf-a/)**.

Suppose, firmware is booted with the image in FIP_A (bank0).

Let's assume the firmware boots from the FIP_A (bank0) image.
StandaloneMm (the UpdateAgent) writes the new firmware image at
__update_index_bank__, i.e. in FIP_B (bank1) in this case.
After updating the firmware, the new image (saved in __update_index_bank__)
is booted.

For more details, please see the [PSA][1] specification, **Part A. A/B Firmware Store design guidance**.

# Overview
Here is an overview of Firmware Update Feature.

    UEFI (Normal world)          |        StandaloneMm (Secure world)
---------------------------------|--------------------------------------
                                 |                             +-------+
                                 |                    ---------|  Fws  |
                                 |                    |        +-------+
+------------------+             |                    |       (Gpt parted)
|   FmpDevicePkg   |             |  Read /Write Image |
+------------------+             |                    |
  |                              |        +-------------------+
  |  SetTheImage and etc         |        |  FwsPlatformLib   |
  |  progress via FmpDeviceLib   |        +-------------------+
  |                              |                  |
  |                              |  Parsing Request | Access Fws via
  |                              |                  | FwsPlatformLib
  |                              |                  |
  |                              |                  |
  -> +---------------------+   PSA ABI (FF-A)    +-------------------+
     |  FmpDevicePsaFwuLib |<------------------> |     FwuSmm.c      |
     |   (FmpDeviceLib)    |   PSA Error code    +-------------------+
     +---------------------+     |

When UEFI calls UpdateCapsule(), FmpDevicePkg->SetTheImage() is called.
Through FmpDeviceLib, FmpDevicePkg requests a firmware update to StandaloneMm
according to Firmware Store Update ABI defined in [PSA][1] spec via PsaFwuLib.
Then FwuStMm StandaloneMm driver parses requests from PsaFwuLib and access to firmware
storage via FwsPlatformLib which is platform specific library.

# Quick Start

## How to Generate Certificates.
Before starting, a certificate chain should be generated.
Below example generates the chain for testing with TestCerts.cnf

NOTE:
  For test purpose, you can run **GenTestCert.py** which generates
  all certificate chain for it.

```
0) Preparation
 cd {edk2-platforms}/Platform/ARM/Features/Fwu
 export OPENSSL_CONF={edk2-platforms}/Platform/ARM/Features/Fwu/TestCerts.cnf
 mkdir -p demoCA/newcerts
 touch demoCA/index.txt
 echo 01 > demoCA/serial

1) Generate the Root Pair:

 Generate a root key:
    openssl genrsa -aes256 -out TestRoot.key 2048

 Generate a self-signed root certificate:
    openssl req -extensions v3_ca -new -x509 -days 3650 -key TestRoot.key -out TestRoot.crt
    openssl x509 -in TestRoot.crt -out TestRoot.cer -outform DER
    openssl x509 -inform DER -in TestRoot.cer -outform PEM -out TestRoot.pub.pem

2) Generate the Intermediate Pair:

  Generate the intermediate key:
    openssl genrsa -aes256 -out TestSub.key 2048

  Generate the intermediate certificate:
    openssl req -new -days 3650 -key TestSub.key -out TestSub.csr
    openssl ca -extensions v3_ca -in TestSub.csr -days 3650 -out TestSub.crt -cert TestRoot.crt -keyfile TestRoot.key
    openssl x509 -in TestSub.crt -out TestSub.cer -outform DER
    openssl x509 -inform DER -in TestSub.cer -outform PEM -out TestSub.pub.pem


3) Generate User Key Pair for Data Signing:

  Generate User key:
    openssl genrsa -aes256 -out TestCert.key 2048

  Generate User certificate:
    openssl req -new -days 3650 -key TestCert.key -out TestCert.csr
    openssl ca -extensions usr_cert -in TestCert.csr -days 3650 -out TestCert.crt -cert TestSub.crt -keyfile TestSub.key
    openssl x509 -in TestCert.crt -out TestCert.cer -outform DER
    openssl x509 -inform DER -in TestCert.cer -outform PEM -out TestCert.pub.pem


4) Generate key & certificate bundle for end certificate:
    openssl pkcs12 -export -out TestCert.pfx -inkey TestCert.key -in TestCert.crt
    openssl pkcs12 -in TestCert.pfx -nodes -out TestCert.pem

5) Test
  -  Sign a Binary File to generate a detached PKCS7 signature:
    openssl smime -sign -binary -signer TestCert.pem -outform DER -md sha256 -certfile TestSub.pub.pem -out test.bin.p7 -in {some_file}
  -  Verify PKCS7 Signature of a Binary File:
    openssl smime -verify -inform DER -in test.bin.p7 -content {some_file} -CAfile TestRoot.pub.pem

6) Generate certificate in Pcd format which used in FmpDevicePkg for authentification.
  # Running this command after edk2 build setup.
  BinToPcd.py -i TestRoot.cer -p gFmpDevicePkgTokenSpaceGuid.PcdFmpDevicePkcs7CertBufferXdr -x -o TestRoot.cer.gFmpDevicePkgTokenSpaceGuid.PcdFmpDevicePkcs7CertBufferXdr.inc
```

## Use your own certificate chain
If you have your own certificate chain, you can make the Pcd file used in
FmpDevicePkg for authentification with your root certificate:

   openssl x509 -in {ROOT_CERT_FILE} -out Root.cer -outform DER
   BinToPcd.py -i Root.cer gFmpDevicePkgTokenSpaceGuid.PcdFmpDevicePkcs7CertBufferXdr -x -o {OUTPUT_FILE_PATH}

And build with
   -D FMP_SYSTEM_FIP_CERT_PCD_FILE = {OUTPUT_FILE_PATH}

## Build TF-A
To support the firmware update feature, TF-A must be built using the following flags:

Support firmware update feature:
```
ARM_GPT_SUPPORT=1 PSA_FWU_SUPPORT=1
```
e.g.
```
// Build TF-A with SPMC_AT_EL3 (with StandaloneMm)
cd tf-a

make all PLAT=fvp CROSS_COMPILE={cross_compile_prefix} DEBUG=1 V=1 CSS_NON_SECURE_UART=1 EXTRA_EL2_INIT=1 \
     EL3_EXCEPTION_HANDLING=1 ENABLE_SME2_FOR_NS=0 ENABLE_SME_FOR_NS=0 ENABLE_SVE_FOR_NS=0 CTX_INCLUDE_AARCH32_REGS=0 \
     SPD=spmd SPMD_SPM_AT_SEL2=0 SPMC_AT_EL3=1 SPMC_AT_EL3_SEL0_SP=1 CTX_INCLUDE_EL2_REGS=0 NS_TIMER_SWITCH=1 HOB_LIST=1 \
     FVP_TRUSTED_SRAM_SIZE=512 ARM_SPMC_MANIFEST_DTS=${TF_A_DIR}/plat/arm/board/fvp/fdts/fvp_stmm_manifest.dts \
     BL32={StandaloneMm Binary} ARM_GPT_SUPPORT=1 PSA_FWU_SUPPORT=1
```

## Build StandaloneMm
To support firmware update feature, StandaloneMm should be built with below options.
```
build -t GCC5 -a AARCH64 -p Platform/ARM/VExpressPkg/PlatformStandaloneMm.dsc -D ENABLE_UEFI_SECURE_VARIABLE=TRUE -D ENABLE_FIRMWARE_UPDATE=TRUE
```

## Build UEFI
To support firmware update feature, UEFI should be built with below options.

Note:
   Before build, Please check Root certificate generate properly with Pcd format
   using **BinToPcd.py**

   for test/development purpose you can generate all required files with

       python3 Features/Fwu/GenTestCert.py

```
For Test:
    build -t GCC5 -a AARCH64 -p Platform/ARM/VExpressPkg/ArmVExpress-FVP-AArch64.dsc -D ENABLE_UEFI_SECURE_VARIABLE=TRUE -D ENABLE_FIRMWARE_UPDATE=TRUE -D FMP_SYSTEM_FIP_CERT_PCD_FILE=Platform/ARM/Features/Fwu/Root.cer.gFmpDevicePkgTokenSpaceGuid.PcdFmpDevicePkcs7CertBufferXdr.inc

With your certificate Pcd files:
    build -t GCC5 -a AARCH64 -p Platform/ARM/VExpressPkg/ArmVExpress-FVP-AArch64.dsc -D ENABLE_UEFI_SECURE_VARIABLE=TRUE -D ENABLE_FIRMWARE_UPDATE=TRUE -D FMP_SYSTEM_FIP_CERT_PCD_FILE={YOUR_CERT_PCD_FILE}
```

## Generate Fip Image
To generate Fip image with UEFI & StandaloneMm, please see TF-A documents[2].

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

## How to Generate Capsule Image
Before generating Capsule Image, please do edk2 build setup
and generate certificates following **How to generate certificates**.

To generate capsule image used for updating firmware, use the below commands:
```
GenerateCapsule --encode --verbose --guid "49757d90-6c22-11ee-a556-1757eba0420c" --fw-version={fw_version} --lsv={least_support_version} --signer-private-cert={TestCert.pem} --other-public-cert={TestSub.pub.pem} --trusted-public-cert={TestRoot.pub.pem} -o {capsule_name} {fip_image}
```

NOTE: --guid should be matched with FMP_SYSTEM_FIP_IMAGE_GUID so let
      this capsule know that it is built for FMP_SYSTEM_FIP_IMAGE_GUID.

## Generate Firmware Storage.
To generate firmware storage partitioned image easily with fip image,
Please do following steps.

```
# Generate firmware update metadata
python3 GenFwuMetadata.py -g -b 2 -p 1 -i 49757d90-6c22-11ee-a556-1757eba0420c -o fwu_metadata.data

# Generate gpt partition
gen_gpt_flash.sh -m fwu_metadata.data -f ${WORKSPACE}/Artifacts/fvp_debug/fip_fvp.bin
```

You can get flash_gpt.img and with this image, run the Model with below option
```
-C bp.flashloader0.fname="{...}/flash_gpt.img" \
-C secure_only_flash0=1 \    # To prevent flash0 from being accessed from NS world.
-C secure_only_flash1=1      # To prevent flash1 from being accessed from NS world.
```

e.g.
```
dd if=/dev/zero of=flash1.data bs=1M count=64

FVP_Base_RevC-2xAEMvA -R -I \
  -C bp.secure_memory=1 \
  -C pctl.startup=0.0.0.0 \
  -C cluster0.cpu0.enable_crc32=1 \
  -C cluster0.cpu1.enable_crc32=1 \
  -C cluster0.cpu2.enable_crc32=1 \
  -C cluster0.cpu3.enable_crc32=1 \
  -C cluster0.cpu4.enable_crc32=1 \
  -C cluster0.cpu5.enable_crc32=1 \
  -C cluster0.cpu6.enable_crc32=1 \
  -C cluster0.cpu7.enable_crc32=1 \
  -C cluster0.has_rndr=1          \
  -C cluster0.has_arm_v9-6=1 \
  -C cluster1.cpu0.enable_crc32=1 \
  -C cluster1.cpu1.enable_crc32=1 \
  -C cluster1.cpu2.enable_crc32=1 \
  -C cluster1.cpu3.enable_crc32=1 \
  -C cluster1.cpu4.enable_crc32=1 \
  -C cluster1.cpu5.enable_crc32=1 \
  -C cluster1.cpu6.enable_crc32=1 \
  -C cluster1.cpu7.enable_crc32=1 \
  -C cluster1.has_rndr=1          \
  -C cluster1.has_arm_v9-6=1 \
  -C bp.dram_metadata.is_enabled=1 \
  -C bp.secureflashloader.fname="bl1.bin" \
  -C bp.flashloader0.fname="flash_gpt.img" \
  -C bp.flashloader1.fname="flash1.data" \
  -C bp.flashloader1.fnameWrite="flash1.data" \
  -C bp.pl011_uart0.out_file="./logs/terminal_uart0.log" \
  -C bp.pl011_uart0.enable_dc4=0 \
  -C bp.pl011_uart1.out_file="./logs/terminal_uart1.log" \
  -C bp.pl011_uart1.enable_dc4=0 \
  -C bp.pl011_uart2.out_file="./logs/terminal_uart2.log" \
  -C bp.pl011_uart2.enable_dc4=0 \
  -C bp.pl011_uart3.out_file="./logs/terminal_uart3.log" \
  -C bp.pl011_uart3.enable_dc4=0 \
  -C bp.terminal_3.mode="telnet" \
  -C bp.terminal_3.start_telnet=0 \
  -C bp.terminal_3.start_port=5003 \
  -C bp.terminal_2.mode="telnet" \
  -C bp.terminal_2.start_telnet=0 \
  -C bp.terminal_2.start_port=5002 \
  -C bp.terminal_1.mode="telnet" \
  -C bp.terminal_1.start_telnet=0 \
  -C bp.terminal_1.start_port=5001 \
  -C bp.terminal_0.mode="telnet" \
  -C bp.terminal_0.start_telnet=0 \
  -C bp.terminal_0.start_port=5000 \
  -C cache_state_modelled=0 \
  -C bp.vis.rate_limit-enable=0 \
  -C bp.dram_size=0x8 \
  -C bp.pl011_uart0.flow_ctrl_mask_en=1 \
  -C bp.pl011_uart1.flow_ctrl_mask_en=1 \
  -C bp.pl011_uart2.flow_ctrl_mask_en=1 \
  -C bp.pl011_uart3.flow_ctrl_mask_en=1 \
  -C bp.virtio_net.enabled=0 \
  -C bp.smsc_91c111.enabled=1 \
  -C bp.hostbridge.userNetworking=1 \
  -C bp.hostbridge.userNetPorts="2222=22" \
  -C bp.secure_only_flash1=0 \
  -C pci.mem0_end="0x5fffFFFF" \
  -C cluster0.NUM_CORES=4 \
  -C cluster1.NUM_CORES=4 \
  -C bp.secure_sram_size=1 \
  -C bp.virtioblockdevice.image_path="{rootfs.img}" \
  -C gic_distributor.ITS-count=1 \
  -C gic_distributor.print-memory-map=1 \
  -C gic_distributor.has_nmi=0 \
  -C gic_distributor.ITS-device-bits=16
```

## Test
You can test using Capsule Image generated in **How to generate Capsule Image** with:

  1) CapsuleApp
    In UEFI shell with the following command, It can update firmware.
```
    CapsuleApp {capsule_name}
```

  2) fwupd
    In Linux, It can update firmware with the following command:
```
  sudo fwupdate --apply 49757d90-6c22-11ee-a556-1757eba0420c fip_fvp.cap
```

# References
[1] https://developer.arm.com/products/system-design/fixed-virtual-platforms
[2] https://trustedfirmware-a.readthedocs.io/en/latest/getting_started/tools-build.html
