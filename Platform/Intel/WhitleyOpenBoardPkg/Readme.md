# **Board Porting for Intel&reg; Whitley Platform**

## Overview
There are currently three board ports:
* WilsonCityRvp
* CooperCityRvp
* JunctionCity

There are corresponding binaries in edk2-non-osi/WhitleyOpenBoardBinPkg.

And there is a template for board porting, *BoardPortWhitley*.  See below for detailed instructions on creating a new board port.

## BoardPortTemplate
This template profides basic instructions for how to customize the WhitleyOpenBoardPkg for a new system board.

## Board Naming Conventions
The naming of boards within the filesystem is only loosely affiliated with naming used in code.

The convention *TypeBoardName* shows up in code in several ways:
* EFI_PLATFORM_TYPE enum, e.g. TypeJunctionCity
* UBA Protocol, e.g. gEfiPlatformTypeJunctionCityProtocolGuid
* Sometimes in an unused UBA PPI name
* Sometimes to decorate function and variable names. Consistently in UBA PEI code to avoid name collision when multiple library class instances are supported.

"BoardPortTemplate" is used in the board porting template for both board directory name and within the consistent *TypeBoardName* code.

There is no requirement for board directory naming to match code. The most important thing is for developers to match the source code with their hardware. Consistency is desirable, but it is very common for one board port to support multiple board and system products and thus consistent naming between file system and code content is not required.

## Board Porting Steps
It is desirable to pick a fairly unique name as WhitleyOpenBoardPkg UBA feature is designed to make it easy to support many boards in a single binary.
For the purposes of this example, "MyBoard" is the board name in code and filesystem.

1. Copy WhitleyOpenBoardPkg/BoardPortTemplate to WhitleyOpenBoardPkg/MyBoard
2. Rename WhitleyOpenBoardPkg/MyBoard/Uba/TypeBoardPortTemplate to WhitleyOpenBoardPkg/MyBoard/Uba/TypeMyBoard
3. Search and replace BoardPortTemplate with MyBoard in WhitleyOpenBoardPkg/MyBoard.  Do not search and replace at a higher scope as you will break the template examples.
4. Add a new EFI_PLATFORM_TYPE enum in edk2-platforms\Silicon\Intel\WhitleySiliconPkg\Include\PlatformInfoTypes.h, e.g.
```
TypeMyBoard, // 0x80
```
Please update the comment for TypeBoardPortTemplate to match the new maximum used, e.g.
```
TypeBoardPortTemplate               // 0x81
```
5. Update the PcdBoardId for your board in the WhitleyOpenBoardPkg/MyBoard/PlatformPkg.dsc, e.g.
```
gPlatformTokenSpaceGuid.PcdBoardId|0x80 # TypeMyBoard
```
6. Update each INF in WhitleyOpenBoardPkg/MyBoard/Uba with new GUID filename
7. Add a DXE UBA protocol GUID to WhitleyOpenBoardPkg/PlatformPkg.dec, *with a new GUID*
```
gEfiPlatformTypeMyBoardProtocolGuid       = { 0xa68228c5, 0xc00f, 0x4d9a, { 0x8d, 0xed, 0xb9, 0x6b, 0x9e, 0xef, 0xab, 0xca } }
```
8. Add your board to the switch statement in BoardInitDxeDriverEntry (); in WhitleyOpenBoardPkg/Uba/BoardInit/Dxe/BoardInitDxe.c
```
   case TypeMyBoard:
      Status = gBS->InstallProtocolInterface (
        &Handle,
        &gEfiPlatformTypeMyBoardProtocolGuid,
        EFI_NATIVE_INTERFACE,
        NULL
        );
      ASSERT_EFI_ERROR (Status);
      break;
```
9. Add the gEfiPlatformTypeMyBoardProtocolGuid to the WhitleyOpenBoardPkg/Uba/BoardInit/Dxe/BoardInitDxe.inf
10. Add a build option to edk2-platforms/Platform/Intel/build.cfg.  e.g.
```
MyBoard = WhitleyOpenBoardPkg/MyBoard/build_config.cfg
```
11. Add Ifwi folder for MyBoard 
```
cd edk2-non-osi/Platform/Intel/WhitleyOpenBoardBinPkg/Ifwi/
mkdir MyBoard
cp WilsonCityRvp/* MyBoard/
```
12. At this point, you can build from edk2-platforms/Platform/Intel, e.g.
```
build_bios.py -p MyBoard -t VS2015x86 -d
```
13. At this point, customization is not scripted.  The following are common customization areas:
MyBoard/Uba/TypeBoardPortTemplate/Pei
* GPIO
* VR, IMON
* SKU info
* Board layout, sockets, memory
* Soft straps, PCH, config, USB OC
* PCI, KTI, IO port bifurcation
MyBoard/Uba/TypeBoardPortTemplate/Dxe
* IIO config update
* Slot config update
* USB overcurrent update

## Board Builds

**Building with the python script**

1. Open command window, go to the workspace directory, e.g. c:\Edk2Workspace or ~/Edk2Workspace in the case of a linux OS
2. If using a linux OS
   * Type "cd edk2"
   * Type "source edksetup.sh"
   * Type "cd ../" to go back to the workspace directory
3. Type "cd edk2-platforms/Platform/Intel
4. Type "python build_bios.py -p TARGET_BOARD"

* build_bios.py arguments:

  | Argument              | Function                            |
  | ----------------------|-------------------------------------|
  | -h, --help            | show this help message and exit     |
  | --platform, -p        | the platform to build               |
  | --toolchain, -t       | tool Chain to use in build process  |
  | --DEBUG, -d           | debug flag                          |
  | --RELEASE, -r         | release flag                        |
  | --TEST_RELEASE, -tr   | test Release flag                   |
  | --RELEASE_PDB, -rp    | release flag                        |
  | --list, -l            | lists available platforms           |
  | --cleanall            | cleans all                          |
  | --clean               | cleans specified platform           |
  | --capsule             | capsule build enabled               |
  | --silent              | silent build enabled                |
  | --performance         | performance build enabled           |
  | --fsp                 | fsp wrapper build enabled           |
  | --fspapi              | API mode fsp wrapper build enabled  |
  | --hash                | Enable hash-based caching           |
  | --binary-destination  | create cache in specified directory |
  | --binary-source       | Consume cache from directory        |
  |                                                             |

* For more information on build options
  * Type "python build_bios.py -h"
