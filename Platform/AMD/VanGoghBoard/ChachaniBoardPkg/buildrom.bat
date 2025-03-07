@REM @file
@REM   Windows batch file to build BIOS ROM image
@REM
@REM Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
@REM SPDX-License-Identifier: BSD-2-Clause-Patent
@REM

@REM ============================================
@REM   Run Project specific post-build process
@REM ============================================
set PROJECT_PKG=%PLATFORM_PATH%\ChachaniBoardPkg
set FD_NAME=%OemBoard%

@echo source override before build
xcopy %WORKSPACE%\edk2-platforms\Platform\AMD\VanGoghBoard\Override\edk2\UefiCpuPkg\Library\SmmCpuFeaturesLib\SmmCpuFeaturesLibCommon.c  %WORKSPACE%\edk2\UefiCpuPkg\Library\SmmCpuFeaturesLib\    /S /Y

@echo source override for AMD Smram override
xcopy %WORKSPACE%\edk2-platforms\Platform\AMD\VanGoghBoard\Override\edk2\MdePkg\Include\Register\Intel\SmramSaveStateMap.h %WORKSPACE%\edk2\MdePkg\Include\Register\Intel\SmramSaveStateMap.h    /S /Y
xcopy %WORKSPACE%\edk2-platforms\Platform\AMD\VanGoghBoard\Override\edk2\UefiCpuPkg\PiSmmCpuDxeSmm\SmramSaveState.c   %WORKSPACE%\edk2\UefiCpuPkg\PiSmmCpuDxeSmm\SmramSaveState.c /S /Y

@echo source override to support FSP PCD get method
xcopy %WORKSPACE%\edk2-platforms\Platform\AMD\VanGoghBoard\Override\edk2\MdeModulePkg\Universal\PCD\Dxe\Pcd.inf %WORKSPACE%\edk2\MdeModulePkg\Universal\PCD\Dxe\Pcd.inf  /S /Y
xcopy %WORKSPACE%\edk2-platforms\Platform\AMD\VanGoghBoard\Override\edk2\MdeModulePkg\Universal\PCD\Dxe\Pcd.c   %WORKSPACE%\edk2\MdeModulePkg\Universal\PCD\Dxe\Pcd.c    /S /Y
xcopy %WORKSPACE%\edk2-platforms\Platform\AMD\VanGoghBoard\Override\edk2\MdeModulePkg\Universal\PCD\Pei\Pcd.c   %WORKSPACE%\edk2\MdeModulePkg\Universal\PCD\Pei\Pcd.c    /S /Y
xcopy %WORKSPACE%\edk2-platforms\Platform\AMD\VanGoghBoard\Override\edk2\MdeModulePkg\Universal\PCD\Pei\Pcd.inf %WORKSPACE%\edk2\MdeModulePkg\Universal\PCD\Pei\Pcd.inf  /S /Y

@echo source override to support Capsule Update
xcopy %WORKSPACE%\edk2-platforms\Platform\AMD\VanGoghBoard\Override\edk2\BaseTools\Source\Python\GenFds\Capsule.py %WORKSPACE%\edk2\BaseTools\Source\Python\GenFds\Capsule.py /S /Y
xcopy %WORKSPACE%\edk2-platforms\Platform\AMD\VanGoghBoard\Override\edk2\MdeModulePkg\Universal\CapsuleRuntimeDxe\CapsuleRuntimeDxe.inf %WORKSPACE%\edk2\MdeModulePkg\Universal\CapsuleRuntimeDxe\CapsuleRuntimeDxe.inf  /S /Y
xcopy %WORKSPACE%\edk2-platforms\Platform\AMD\VanGoghBoard\Override\edk2\MdeModulePkg\Universal\CapsuleRuntimeDxe\CapsuleService.c %WORKSPACE%\edk2\MdeModulePkg\Universal\CapsuleRuntimeDxe\CapsuleService.c  /S /Y
xcopy %WORKSPACE%\edk2-platforms\Platform\AMD\VanGoghBoard\Override\edk2\MdeModulePkg\Universal\CapsuleRuntimeDxe\CapsuleService.h %WORKSPACE%\edk2\MdeModulePkg\Universal\CapsuleRuntimeDxe\CapsuleService.h  /S /Y
@ if "%COMPRESS_FSP_REGION%"=="TRUE" (
  @echo FSP Will be compressed with Recovery FV.
  @set COMPRESS_FSP_EXTRA_ARG=-D COMPRESS_FSP_REGION=TRUE
)

::
:: Pre Build UDK BIOS
::
IF NOT EXIST %WORKSPACE%\%PROJECT_PKG%\Binary\EC\ChachaniEC.bin (
  echo  empty file for build > empty.bin
  echo f |xcopy empty.bin %WORKSPACE%\%PROJECT_PKG%\Binary\EC\ChachaniEC.bin /Y
  echo f |xcopy empty.bin %WORKSPACE%\%PROJECT_PKG%\Binary\EC\EcSig.bin /Y
  echo f |xcopy empty.bin %WORKSPACE%\%PROJECT_PKG%\Binary\PD\TIPD.bin /Y
  del empty.bin
)
IF NOT EXIST %WORKSPACE%\edk2-non-osi\Silicon\AMD\VanGogh\FspBlobs\FSPO\FSP-O_DXE.Fv (
  echo  empty file for build > empty.bin
  echo f |xcopy empty.bin %WORKSPACE%\edk2-non-osi\Silicon\AMD\VanGogh\FspBlobs\FSPO\FSP-O_DXE.Fv /Y
  echo f |xcopy empty.bin %WORKSPACE%\edk2-non-osi\Silicon\AMD\VanGogh\FspBlobs\FSPO\FSP-O_PEI.Fv /Y
  del empty.bin
)

::
:: Build UDK BIOS
::
call build -p %WORKSPACE%\%PROJECT_PKG%/project.dsc --conf=%WORKSPACE%/%PROJECT_PKG%/Conf %COMPRESS_FSP_EXTRA_ARG%
@if %errorlevel% NEQ 0 goto ERR_END

@for /f "tokens=3" %%a in ('find "TARGET " %WORKSPACE%\%PROJECT_PKG%\Conf\target.txt') do @set TARGET=%%a
@for /f "tokens=3" %%a in ('find "TOOL_CHAIN_TAG" %WORKSPACE%\%PROJECT_PKG%\Conf\target.txt') do @set TOOL_CHAIN_TAG=%%a
set BIOS_FV_PATH=%WORKSPACE%\Build\ChachaniBoardPkg\%TARGET%_%TOOL_CHAIN_TAG%\FV
set COMPRESS_TOOL_PATH=%WORKSPACE%\edk2-non-osi\Silicon\AMD\VanGogh\FspBlobs\AmdTools\CompressBios
Set PEI_COMPRESS_SIZE=120000

if "%COMPRESS_FSP_REGION%"=="FALSE" (
  Set PEI_COMPRESS_SIZE=100000
)
::
:: Generate BIOS version in binary
::
%PYTHON_HOME%\python.exe py-GenerateBiosVersion.py
@if %errorlevel% NEQ 0 goto ERR_END

@echo ###Compress PEI FV ######
@echo %COMPRESS_TOOL_PATH%\CompressBios.exe %BIOS_FV_PATH%\ChachaniSPHPei.fd %BIOS_FV_PATH%\ChachaniSPHPeiCOMPRESS.fd %PEI_COMPRESS_SIZE%
%COMPRESS_TOOL_PATH%\CompressBios.exe %BIOS_FV_PATH%\ChachaniSPHPei.fd %BIOS_FV_PATH%\ChachaniSPHPeiCOMPRESS.fd %PEI_COMPRESS_SIZE%
@if %errorlevel% NEQ 0 goto ERR_END
copy /b /y %BIOS_FV_PATH%\CHACHANI.fd+%BIOS_FV_PATH%\ChachaniSPHPeiCOMPRESS.fd  %BIOS_FV_PATH%\CHACHANI.fd
::
:: Generate ACPB Binary
::
echo D | xcopy %WORKSPACE%\edk2-non-osi\Silicon\AMD\VanGogh\FspBlobs\APCB\*.*  %WORKSPACE%\Build\ChachaniBoardPkg\ApcbToolV3\%BUILD_TYPE%\Release\/S /Y
::
:: Intergrate PSP/BIOS Directory FWs
::
call PspBuild.bat
if %ERRORLEVEL% NEQ 0 goto ERR_END

::
:: Generate Final Flash A/B Image
::
call GenFlashABImage.bat
if %ERRORLEVEL% NEQ 0 goto ERR_END

::
:: Generate Capsule Image
::
call GenCapsule.bat
if %ERRORLEVEL% NEQ 0 goto ERR_END
goto DONE

:ERR_END
  @exit /B 2
:DONE
