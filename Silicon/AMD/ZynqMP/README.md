## Introduction

These instructions explain how to use this package for porting EDK-II to a
ZynqMP-based platform, and how to build.

## Creating a new ZynqMP platform

The following steps contain brief explanation. You can refer to
[Platform/AMD/ZynqmpVirtPkg](../../../Platform/AMD/ZynqmpVirtPkg/) for an
example.

1. Create a new platform directory (e.g. Platform/AMD/<YourPlatform>).

2. Create a platform description file (.dsc), and flash description file (.fdf)
under this directory. This ZynqMP package contains header files with minimal
needed libraries and components, which can be included in your description
files.

    **ZynqMP.dsc.inc:** Platform description header with library classes and
    components.

    **ZynqMPFvMain.fdf.inc:** Flash description header with DXE components.

    **ZynqMPRules.fdf.inc:** Flash description header with build rules.

    Additionaly, you need to have:
    ```
    INF ArmPlatformPkg/PeilessSec/PeilessSec.inf
    ```
    in the flash description file.

3. Add your devicetree files and include them in the description files.

4. **ZynqMP.dec** has ZynqMP PCDs with default values. Tweak them if needed in
   your (.dsc) file.

5. If needed, you can add extra drivers or libraries for your platform.


## Building

You can refer to [ARM](../../../Platform/ARM/Readme.md) build instructions for
setting-up the environment.

Then use the following command:

```
build -a AARCH64 -t GCC5 -p Platform/AMD/<YourPlatform>/<YourPlatform>.dsc -b <DEBUG|RELEASE>
```

It's expected that you use the obtained binary as BL33 alongside Xilinx FSBL
(BL1/2), PMU firmware, TF-A (BL31), and optionally OP-TEE (BL32) to have a full
firmware image.

Check [Boot
Flow](https://docs.amd.com/r/en-US/ug1137-zynq-ultrascale-mpsoc-swdev/Boot-Flow)
section in ZynqMP Software Developer Guide. EDK-II can replace U-Boot in this
scenario.
