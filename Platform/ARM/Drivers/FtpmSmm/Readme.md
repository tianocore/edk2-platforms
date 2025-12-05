# FtpmSmm Driver

FtpmSmm driver is a software-based TPM implementation built using the
[TPM 2.0 Reference Implementation Library][1] and the
[TPM Service Command Response Buffer Interface over FF-A][2] specification.

[The TPM 2.0 Reference Implementation Library][1] provides
the TPM functionality itself,
while [the TPM Service Command Response Buffer Interface over FF-A][2] defines
the communication mechanism between
the TPM and the system using FF-A (Firmware Framework for Arm A-profile).

With FtpmSmm driver, the platform where StandaloneMm is used
can use TPM functionality for:
    - Block device encryption with LUKS using PCR.
    - End-to-end measurement boot.

## Overview
Here is an overview how FtpmSmm works.

1) with UEFI
```
         UEFI (Normal world)       |         Secure World
    -------------------------------|------------------------------
                                   |
       +--------------+            | +-----------+      +----------+
       |    Tcg2Dxe   |            | |  FtpmSmm  |<---->|  TpmLib  |
       +--------------+            | +-----------+      +----------+
               |                   |       |
               |                   |       ----------
               |                   |                |
               |                   |                |
               |                   |       +------------------+
               |                   |       | StandaloneMmCpu  |
               |                   |       +------------------+
               |                   |                |
               |                   |                |
               |                   |                |
       +----------------------+    |       +----------------------------+
       |  Tpm2InstanceFfaLib  |<---------->| StandaloneMmCoreEntryPoint |
       +----------------------+    .       |      (Misc Service)        |
                                   .       +----------------------------+
                                   .
                               Communicate via CRB over FF-A [0]
```
2) with linux-kernel
```
         linux (Normal world)      |         Secure World
    -------------------------------|------------------------------
                                   |
       +----------------------+    | +-----------+      +----------+
       |  TPM infra-structure |    | |  FtpmSmm  |<---->|  TpmLib  |
       +----------------------+    | +-----------+      +----------+
               |                   |       |
               |                   |       ----------
               |                   |                |
               |                   |                |
               |                   |       +------------------+
               |                   |       | StandaloneMmCpu  |
               |                   |       +------------------+
               |                   |                |
               |                   |                |
               |                   |                |
       +----------------------+    |       +----------------------------+
       |  tpm_crb_ffa driver  |<---------->| StandaloneMmCoreEntryPoint |
       +----------------------+    .       |      (Misc Service)        |
                                   .       +----------------------------+
                                   .
                     Communicate via CRB over FF-A [0]
```
When a TPM command is initiated by Tpm2InstanceFfaLib or
tpm_crb_ffa driver according to
[the TPM Service Command Response Buffer Interface over FF-A][2] specitication,
FtpmSmm receives the command request and it calls the TpmLib which
is wrapper library of [The TPM 2.0 Reference Implementation Library][1]
to handle the command properly.

After TpmLib handles the TPM command via
[The TPM 2.0 Reference Implementation Library][1].
It delivers the result according to
[the TPM Service Command Response Buffer Interface over FF-A][2] specitication.


## Quick Start

All of example build commands are based on FVP RevC.

### Build TF-A
For a measurement boot, TF-A must be built using the following flags
to get eventlog from TF-A:

Support firmware update feature:
```
MEASURED_BOOT=1 HOB_LIST=1 TRANSFER_LIST=1 MBEDTLS_DIR=${MBEDTLS_DIR}
```

e.x)
    make BUILD_PARAM=V=1 CSS_NON_SECURE_UART=1 EXTRA_EL2_INIT=0 \
         FVP_FAKE_TRNG_SUPPORT=1 FVP_TRUSTED_SRAM_SIZE=512 \
         ARM_BL31_IN_DRAM=1 SPD=spmd SPMD_SPM_AT_SEL2=0 SPMC_AT_EL3=1 SPMC_AT_EL3_SEL0_SP=1 \
         CTX_INCLUDE_EL2_REGS=0 NS_TIMER_SWITCH=1 HOB_LIST=1  CTX_INCLUDE_AARCH32_REGS=0 \
         ARM_SPMC_MANIFEST_DTS=${TF_A_DIR}/plat/arm/board/fvp/fdts/fvp_stmm_spmc_at_el3_manifest.dts \
         MEASURED_BOOT=1 TRANSFER_LIST=1 MBEDTLS_DIR=${MBEDTLS_DIR}

### Build StandaloneMm
To use FtpmSmm, StandaloneMm should be built with below options.
```
build -t GCC5 -a AARCH64 -p Platform/ARM/VExpressPkg/PlatformStandaloneMm.dsc -D ENABLE_UEFI_SECURE_VARIABLE=TRUE -D ENABLE_TPM=TRUE --pcd PcdTpmUniqueValue={UINT64 unique value}
```

### Build UEFI
To use FtpmSmm, UEFI should be built with below options.
```
    build -t GCC5 -a AARCH64 -p Platform/ARM/VExpressPkg/ArmVExpress-FVP-AArch64.dsc -D ENABLE_UEFI_SECURE_VARIABLE=TRUE -D ENABLE_TPM=TRUE
```

### Test
You can check the TPM funtionality with FtpmSmm driver with ubuntu ditro:

1) Measurement boot
```
root@localhost# cat /sys/kernel/security/tpm0/binary_bios_measurements > eventlog.bin
root@localhost# tpm2_eventlog --eventlog-version=2 eventlog.bin > eventlog.txt

# confirmed TF-A passed eventlogs.
root@localhost# cat eventlog.txt

version: 2
events:
- EventNum: 0
  PCRIndex: 0
  EventType: EV_NO_ACTION
  Digest: "0000000000000000000000000000000000000000"
  EventSize: 33
  SpecID:
  - Signature: Spec ID Event03
    platformClass: 0
    specVersionMinor: 0
    specVersionMajor: 2
    specErrata: 0
    uintnSize: 2
    numberOfAlgorithms: 1
    Algorithms:
    - Algorithm[0]:
      algorithmId: sha256
      digestSize: 32
    vendorInfoSize: 0
- EventNum: 1
  PCRIndex: 0
  EventType: EV_POST_CODE
  DigestCount: 1
  Digests:
  - AlgorithmId: sha256
    Digest: "358d75e22d356aa37af47a13af1c9f731a333271c7976482ced81d4518ac6330"
  EventSize: 5
  Event: |-
    BL_2
- EventNum: 2
  PCRIndex: 0
  EventType: EV_POST_CODE
  DigestCount: 1
  Digests:
  - AlgorithmId: sha256
    Digest: "d589d37f025bd8b5159cd69380097e481959357f2055777a2942ca55a5a81873"
  EventSize: 11
  Event: |-
    SYS_CTRL_2
...
pcrs:
  sha256:
    0  : 0x2da951fe7db67ef07bf5093513e10ccd6017ffccf094fb30510bb2ee10bf751f // this is expected PCR0 value replayed based on eventlogs
    1  : 0x6045238ac4ded8d16f980f6ef42ad033a50aac53f6cd8bdd8c0aa3d00f210750
    2  : 0x3d458cfe55cc03ea1f443f1562beec8df51c75e14a9fcf9a7234a13f198e7969
    3  : 0x3d458cfe55cc03ea1f443f1562beec8df51c75e14a9fcf9a7234a13f198e7969
    4  : 0xd5cc491b88fa59c746f323dae5e4a9eebd2edc217985a40a512b28b06a9c2a29
    5  : 0x4b916b1cbb76dede7faa1ab2047875e7c72d0fad808355c61d85dc606aa16015
    6  : 0x3d458cfe55cc03ea1f443f1562beec8df51c75e14a9fcf9a7234a13f198e7969
    7  : 0x127c18eba2300e30767fafe71f4e5975776f665d22c7ca9017c7c24846b96fa1
    8  : 0xb86f03b031a00b75fb75171d6e968fdfb6206c8331d5fc33b6d2d7df38758c00
    9  : 0x4c726b31ba14bb07380f9aa4e36bc9f81c298944d559aff386cc4e3c6b19b910
    14 : 0x306f9d8b94f17d93dc6e7cf8f5c79d652eb4c6c4d13de2dddc24af416e13ecaf

# confirm current PCR values.
root@localhost# cat /sys/class/tpm/tpm0/pcr-sha256/0
2DA951FE7DB67EF07BF5093513E10CCD6017FFCCF094FB30510BB2EE10BF751F
```

2) Block device encryption with LUKS (Linux Unified Key System)

2.1) Manual Setup

2.1.1) Generate key and sealing it for luks

```
# create directory
root@localhost# mkdir -p /etc/tpm-luks

# Generate key used for block device encryption
root@localhost# echo "MySecretData" > /etc/tpm-luks/secret.data

# Create the key for sealing
root@localhost# tpm2_createprimary -C o -c /etc/tpm-luks/test_primary.ctx

# Sealing the secret.data with PCR-256 index 0.
root@localhost# tpm2_createpolicy --policy-pcr -l sha256:0 -L /etc/tpm-luks/policy.digest
root@localhost# tpm2_create -C /etc/tpm-luks/test_primary.ctx -u /etc/tpm-luks/test_seal.pub -r /etc/tpm-luks/test_seal.priv -L policy.digest -i secret.data
```

2.1.2) Create unsealing scripts
```
root@localhost# cat <<'EOF' > /etc/tpm-luks/test_unseal.sh

tpm2_load -C /etc/tpm-luks/test_primary.ctx -u /etc/tpm-luks/test_seal.pub -r /etc/tpm-luks/test_seal.priv -c /run/test_seal.ctx 2>&1 > /dev/null
tpm2_startauthsession --policy-session -S /run/test_session.ctx 2>&1 > /dev/null
tpm2_policypcr -S /run/test_session.ctx -l sha256:0 2>&1 > /dev/null
tpm2_unseal -c /run/test_seal.ctx -p session:/run/test_session.ctx
tpm2_flushcontext /run/test_session.ctx 2>&1 > /dev/null
EOF

root@localhost# chmod +x /etc/tpm-luks/test_unseal.sh
```

2.1.3) Prepare device
```
# Prepare 512MB block device
root@localhost# dd if=/dev/zero of=manual.img bs=1M count=512

# Set keyfile for block device (or diskfile)
root@localhost# cryptsetup luksFormat manuel.img --type luks2 --key-file /etc/tpm-luks/secret.data

# confirm
root@localhost# cryptsetup luksDump manual.img
```

2.1.4) Setting /etc/crypttab
```
# add entry for prepared device
root@localhost# cat /etc/crypttab

# <target name>   <source device>       <key file>    <options>
...
testdev0 /home/levi/work/test/tpm/disk/manual.img none luks,keyscript=/etc/tpm-luks/test_unseal.sh
```

2.1.5) Map encrypted block device
```

# add entry for prepared device
root@localhost# cryptdisks_start testdev0
 * Starting crypto disk...
 * testdev0 (starting)...
 * testdev0 (started)...                     [ OK ]

root@localhost# lsblk
NAME                      MAJ:MIN RM   SIZE RO TYPE  MOUNTPOINTS
...
loop0                       7:0    0   512M  0 loop
└─testdev0                252:1    0   496M  0 crypt
...

# testdev0 can be used for fdisk, mkfs, mount or etc...
```

2.1.6) Remove encrypted block device
```
# add entry for prepared device
root@localhost# cryptdisks_start testdev0
 * Starting crypto disk...
 * testdev0 (starting)...
 * testdev0 (started)...                     [ OK ]

root@localhost# lsblk
NAME                      MAJ:MIN RM   SIZE RO TYPE  MOUNTPOINTS
...
loop0                       7:0    0   512M  0 loop
└─testdev0                252:1    0   496M  0 crypt
...

# testdev0 can be used for fdisk, mkfs, mount or etc...
```

2.2) Using Clevis
As you see setup manually quite complicate and we couldn't handle properly if
the system status changed and user forgot to update properly.
the clevis is automated this progress, supply auto-unlocking feature and
give a chance to decrypt block device via user-input password.

2.2.1) Install clevis
```
root@localhost# apt install clevis clevis-luks clevis-dracut clevis-udisks2 clevis-systemd clevis-tpm2 clevis-initramfs
```

2.2.2) Prepare device
```

# Prepare 512MB block device
root@localhost# dd if=/dev/zero of=clevis.img bs=1M count=512

# Set keyfile for block device (or diskfile)
root@localhost# cryptsetup luksFormat clevis2.img

WARNING!
========
This will overwrite data on clevis2.img irrevocably.

Are you sure? (Type 'yes' in capital letters): YES
Enter passphrase for clevis2.img:
Verify passphrase:
```

2.2.3) Bind device with TPM PCR-256 index 0
```
root@localhost# clevis luks bind -d clevis.img tpm2 '{"pcr_bank":"sha256", "pcr_ids":"0"}'

# confimred
root@localhost# clevis luks list -d clevis.img
1: tpm2 '{"hash":"sha256","key":"ecc","pcr_bank":"sha256","pcr_ids":"0"}'

# for rootfs mount, update initramfs for auto-unlock (ubuntu)
root@localhost# update-initramfs -u

# for rootfs mount, update initramfs for auto-unlock (fedora)
root@localhost# dracut -f
```

2.2.4) Unlocking the device
```
root@localhost# clevis luks unlock -d clevis.img -n testdev1
lsblk
NAME                      MAJ:MIN RM   SIZE RO TYPE  MOUNTPOINTS
...
loop1                       7:1    0   512M  0 loop
└─testdev1                252:2    0   496M  0 crypt
...
# testdev0 can be used for fdisk, mkfs, mount and etc
```

2.2.5) Remove mapped device
```
root@localhost# cryptsetup close testdev1
```

## References
[1] https://github.com/TrustedComputingGroup/TPM
[2] https://developer.arm.com/documentation/den0138/latest/
