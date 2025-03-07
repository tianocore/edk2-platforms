@REM @file
@REM   Windows batch file to build AMD PSP image
@REM
@REM Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
@REM SPDX-License-Identifier: BSD-2-Clause-Patent
@REM

:: BuildPspImage InputBiosFile rtmfile PspDirectoryCfg OutputBiosFile KeyMode

@echo off
::Flag to control use python script or executable file generate from python
::TRUE:   Script file
::FALSE:  Executable file
IF "%USE_PYTHON_SCRIPT%" == "" (
  @set USE_PYTHON_SCRIPT=FALSE
)

::Input parameter check
IF /I "%5" == "" GOTO HELP
::System variable Check
SETLOCAL EnableDelayedExpansion

@set BUILDPSPDIRECTORY=BuildPspDirectory.exe
IF "%PSPKIT_PATH%" == "" (
  SET PSPKIT_PATH=%cd%
)
::CHECK_PATH
IF NOT EXIST %PSPKIT_PATH%\BuildPspImage.bat (
  @echo !!!PSPKIT_PATH system variable is NOT set correctly!!
  goto ERROR
)

IF "%PSP_FW_PATH%" == "" (
  SET PSP_FW_PATH=%cd%
)

IF "%TEMP_PATH%" == "" (
  @SET TEMP_PATH=%PSPKIT_PATH%\Temp
)

:START
::BIOS images
@set InputBiosFile=%1
@set RTM_FILE=%2
@set PspDirectoryCfg=%3
@set FINAL_BIOS=%4

::Related PATH
@SET PSP_DIR_TOOL_PATH=%PSPKIT_PATH%\PspDirectoryTool

::Create Temporary folder, and copy all files to it
@echo rd    %TEMP_PATH% /S /Q
IF EXIST %TEMP_PATH% rd    %TEMP_PATH% /S /Q
@echo mkdir %TEMP_PATH%
mkdir %TEMP_PATH%
@echo copy  %PSP_DIR_TOOL_PATH%\*.*  %TEMP_PATH% /Y
copy  %PSP_DIR_TOOL_PATH%\*.*  %TEMP_PATH% /Y
@echo copy  %PSP_FW_PATH%\*.*        %TEMP_PATH% /Y
copy  %PSP_FW_PATH%\*.*        %TEMP_PATH% /Y
@echo copy  %PSPKIT_PATH%\*.*       %TEMP_PATH% /Y
copy  %PSPKIT_PATH%\*.*       %TEMP_PATH% /Y
IF NOT "%CUSTOM_APCB_PATH%" == "" (
  @echo copy  %CUSTOM_APCB_PATH%\*.*   %TEMP_PATH% /Y
  copy  %CUSTOM_APCB_PATH%\*.*   %TEMP_PATH% /Y
)

if /I NOT "%5" == "NOSIGN" (
@echo copy  %RTM_FILE%               %TEMP_PATH% /Y
  IF EXIST %RTM_FILE% copy  %RTM_FILE%               %TEMP_PATH% /Y
)

::BINARYS
@set RTM_PRIVATE_KEY=TestRtmPrivateKey.pem
@set BIOS_L1_DIR_FILE=Output\BiosDirHeaderL1.bin
@set BIOS_L2_DIR_FILE=Output\BiosDirHeaderL2.bin
@set RTM_BIOSDIRL1_COMBINE_FILE=BiosRtmBiosL1Combine.bin
@set RTM_BIOSDIRL1_L2_COMBINE_FILE=BiosRtmBiosL1L2Combine.bin
@set RTM_FILE_L1_SIGNATURE=RTMSignature.bin
@set RTM_FILE_L1_L2_SIGNATURE=RTMSignatureL1L2.bin

pushd %TEMP_PATH%
::delete pyd & python27.dll which may cause the compatible issue with python installed on the machine when USE_PYTHON_SCRIPT=TRUE
IF /I "%USE_PYTHON_SCRIPT%" == "TRUE" (
  IF EXIST *.pyd del  *.pyd /F /Q
  IF EXIST python27.dll del  python27.dll /F /Q
)

@echo.
@echo [Generate PSP ^& BIOS Directory]
::Build without embed RTMSignature
if /I "%5" == "NOSIGN" goto BLDBIOS

:BLDBIOS


@echo.
@echo [Finally build the Final BiosImage]
@echo %BUILDPSPDIRECTORY% bb %InputBiosFile% %PspDirectoryCfg% %FINAL_BIOS%
%BUILDPSPDIRECTORY% bb %InputBiosFile% %PspDirectoryCfg% %FINAL_BIOS%

if %ERRORLEVEL% NEQ 0 goto ERROR
@echo copy Output\%FINAL_BIOS% ..\
copy Output\%FINAL_BIOS% ..\
popd
::rd %TEMP_PATH% /S /Q
@echo.
@echo PSP contents have been embed to %FINAL_BIOS%

GOTO END

:ERROR
@echo **** Build PSP Image Fail ****
@exit /B 2
GOTO END

:HELP
@echo Embed PSP contents to Specific BIOS image
@echo.
@echo BuildPspImage.bat rtmfile PspDirectoryCfg OutputBiosFile
@echo   Positional parameters:
@echo     rtmfile           The Root trunk module of BIOS, commonly it is the SEC,PEI combined FV image
@echo     PspDirectoryCfg   Config file which describe PSP directory layout, and input bios informations
@echo     OutputBiosFile    The output bios image after embed the PSP contents
@echo     KeyMode           Only "NOSIGN" allowed
goto END
:END
