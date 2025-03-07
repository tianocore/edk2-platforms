### <a name="Server-boards">Server boards</a>
### Turin

| SoC       | SoC Family | SoC SKU | SoC Name  | Board    | Package                     |
|-----------|------------|---------|-----------|----------|-----------------------------|
| Turin SP5 | 0x1A       | BRH     | Breithorn | Chalupa  | TurinBoard/ChalupaBoardPkg  |
|           |            |         |           | Galena   | TurinBoard/GalenaBoardPkg   |
|           |            |         |           | Onyx     | TurinBoard/OnyxBoardPkg     |
|           |            |         |           | Purico   | TurinBoard/PuricoBoardPkg   |
|           |            |         |           | Quartz   | TurinBoard/QuartzBoardPkg   |
|           |            |         |           | Ruby     | TurinBoard/RubyBoardPkg     |
|           |            |         |           | Titanite | TurinBoard/TitaniteBoardPkg |
|           |            |         |           | Volcano  | TurinBoard/VolcanoBoardPkg  |

#### <a name="tianocore-code-base-table">tianocore Code Base</a>
| Code base      | Revision                                           |
|----------------|----------------------------------------------------|
| edk2           | edk2-stable202402 stable release                   |
| edk2-platforms | Commid ID 103c88ba5b0c6259fc674e6358c68a85e882e41b |
| edk2-non-osi   | Commid ID f0bb00937ad6bfdf92d9c7fea9f7277c160d82e9 |

#### Server boards edk2 build
AMD server SoC platform firmware reference code can be built using edk2 native build system. As of now the AGESA source code is released to customer in a different way, the AGESA source files under edk2-platforms/Platform/AMD are published to make sure the platform firmware reference code can be built without errors. Those AGESA modules are considered as the NULL instance of AGESA. Customers can request the release version of
AGESA from AMD, replace the NULL instance AGESA modules.
The open-sourced AGESA modules is still under development and will be upstream to edk2-platforms as the replacement of NULL instance AGESA.

#### edk2 build steps
- Create an workspace (**[WorkSpace]**), e.g., "~/Turin" for Linux or "c:\Turin" for Windows.
- Clone the below repositories (DO NOT update submodule for edk2, [here is the reason](#)) from tianocore to under the **[WorkSpace]**.

  - edk2
  - edk2-platforms
  - edk2-non-osi

#### <a name="Check-out-the-specific-revision">Check out the specific revision</a>
Refer to [this table](#tianocore-code-base-table) for the tag or commit to check out.<br>
When checkout edk2 repository to edk2-stable202402, the git URL to subhook repository points to https://github.com/Zeex/subhook.git which is no longer exist. This issue has been resolved by using git URL to https://github.com/tianocore/edk2-subhook.git. That says when we checkout edk2-stable202402 tag on edk2 repository, we have to manually update the git URL of subhook in .gitmodules to the one on tianocore. Then git submodule update --recursive.


#### Configure edk2 **PACKAGE_PATH** for consuming the modules under particular edk2 packages under edk2,edk2-platform and edk2-non-osi , replace **[WorkSpace]** with yours.

For Linux,
```
export PACKAGES_PATH=[WorkSpace]:[WorkSpace]/edk2:[WorkSpace]/edk2-platforms:[WorkSpace]/edk2-platforms/Platform/AMD:[WorkSpace]/edk2-platforms/Platform/AMD/TurinBoard:[WorkSpace]/edk2-platforms/Features:[WorkSpace]/edk2-platforms/Platform/Intel:[WorkSpace]/edk2-platforms/Features/Intel:[WorkSpace]/edk2-platforms/Features/Intel/Network:[WorkSpace]/edk2-platforms/Features/Intel/OutOfBandManagement:[WorkSpace]/edk2-platforms/Features/Intel/Debugging:[WorkSpace]/edk2-platforms/Features/Intel/SystemInformation:[WorkSpace]/edk2-platforms/Features/Intel/PowerManagement:[WorkSpace]/edk2-platforms/Features/Intel/UserInterface:[WorkSpace]/edk2-platforms/Silicon/Intel:[WorkSpace]/edk2-non-osi
```

##### For Windows,
```
set PACKAGES_PATH=[WorkSpace];[WorkSpace]\edk2;[WorkSpace]\edk2-platforms;[WorkSpace]\edk2-platforms\Platform\AMD;[WorkSpace]\edk2-platforms\Platform\AMD\TurinBoard;[WorkSpace]\edk2-platforms\Features;[WorkSpace]\edk2-platforms\Platform\Intel;[WorkSpace]\edk2-platforms\Features\Intel;[WorkSpace]\edk2-platforms\Features\Intel\Network;[WorkSpace]\edk2-platforms\Features\Intel\OutOfBandManagement;[WorkSpace]\edk2-platforms\Features\Intel\Debugging;[WorkSpace]\edk2-platforms\Features\Intel\SystemInformation;[WorkSpace]\edk2-platforms\Features\Intel\PowerManagement;[WorkSpace]\edk2-platforms\Features\Intel\UserInterface;[WorkSpace]\edk2-platforms\Silicon\Intel;[WorkSpace]\edk2-non-osi
```

#### Configure edk2 tools path.

##### For Linux,
```
export EDK_TOOLS_PATH=[WorkSpace]/edk2/BaseTools
```

##### For Windows,

```
set EDK_TOOLS_PATH=[WorkSpace]\edk2\BaseTools
```

#### Build edk2 base tools under **[WorkSpace]**.

##### For Linux,
```
make -C edk2/BaseTools
```
##### For Windows,
```
edksetup.bat Rebuild
```

#### Configure edk2 build environment, change the current working directory to **[WorkSpace]**, then execute edk2setup bash file.

##### For Linux,
```
. edksetup.sh
```
##### For Windows,
```
edksetup.bat
```

#### Change edk2 WORKSPACE to **[WorkSpace]**
##### For Linux,
```
 export WORKSPACE=[WorkSpace]
```

##### For Linux,
```
 set WORKSPACE=[WorkSpace]
```

#### Build server board Project.dsc<br>
The additional build environment variables are required for the build process, refer to [server board supporting table](#Server-boards) for the values.

|Environment Variable    |Value         |
|------------------------|--------------|
|**SOC_FAMILY**          | [SoC Family] |
|**SOC_SKU**             | [SoC SKU]    |
|**SOC2**                | [SoC Name]   |

##### Building Turin SP5 Chalupa board on Linux,
```
cd [WorkSpace]
build -a X64 -a IA32 -t GCC -DSOC_FAMILY=0x1A -DSOC_SKU=BRH -DSOC2=Breithorn -p TurinBoard/ChalupaBoardPkg/Project.dsc
```

##### Building Turin SP5 Chalupa board on Windows,
```
cd [WorkSpace]
build -a X64 -a IA32 -t VS2017 -DSOC_FAMILY=0x1A -DSOC_SKU=BRH -DSOC2=Breithorn -p TurinBoard\ChalupaBoardPkg\Project.dsc
```
