@REM @file
@REM   Windows batch file to launch PSP build scripts
@REM
@REM Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
@REM SPDX-License-Identifier: BSD-2-Clause-Patent
@REM
@echo off

set BIOSNAME=ChachaniExt

if "%PROJECT_PKG%" =="" (
  set PROJECT_PKG=%PLATFORM_PATH%\ChachaniBoardPkg
)

:: set KEY_MODE to default if not set
set KEY_MODE=TK
if "%KEY_MODE%"=="" (
  set KEY_MODE=NOSIGN
)
@echo KEY_MODE %KEY_MODE%

set PSP_PLATFORM_PATH=%WORKSPACE%\edk2-non-osi\Silicon\AMD\VanGogh\Firmwares
set PSP_CONFIG_FILE_PATH=%WORKSPACE%\%PROJECT_PKG%
set PSP_FW_PATH=%WORKSPACE%\edk2-non-osi\Silicon\AMD\VanGogh\Firmwares
set PSPKIT_PATH=%WORKSPACE%\edk2-non-osi\Silicon\AMD\VanGogh\FspBlobs\AMDTools
set PSP_TEMP_PATH=%WORKSPACE%\Build\ChachaniBoardPkg\NewPspKit
set APCB_BOARD_PKG_TEMP_PATH=%WORKSPACE%\Build\ChachaniBoardPkg\ApcbToolV3\External
set CUSTOM_APCB_PATH=%APCB_BOARD_PKG_TEMP_PATH%\Release\
IF NOT EXIST %CUSTOM_APCB_PATH%\ApcbSet1Ff3DefaultRecovery.bin set CUSTOM_APCB_PATH=
IF NOT EXIST %CUSTOM_APCB_PATH%\ApcbSet1Ff3Updatable.bin set CUSTOM_APCB_PATH=

set USE_PYTHON_SCRIPT=TRUE

@for /f "tokens=3" %%a in ('find "TARGET " %WORKSPACE%\%PROJECT_PKG%\Conf\target.txt') do @set TARGET=%%a
@for /f "tokens=3" %%a in ('find "TOOL_CHAIN_TAG" %WORKSPACE%\%PROJECT_PKG%\Conf\target.txt') do @set TOOL_CHAIN_TAG=%%a
set BIOS_FV_PATH=%WORKSPACE%\Build\ChachaniBoardPkg\%TARGET%_%TOOL_CHAIN_TAG%\FV

set RTM_FILE=FV_COMBINE.Fv
copy /B /Y %BIOS_FV_PATH%\FVMAIN_COMPACT.Fv + %BIOS_FV_PATH%\RECOVERYFV.Fv %BIOS_FV_PATH%\%RTM_FILE%
set SP_FUNCTION=SIGN BIOS WITH Chachani TEST CUSTOMER KEY

if exist %PSP_TEMP_PATH% (
  rd %PSP_TEMP_PATH% /S /Q
)
echo D | xcopy %PSPKIT_PATH%\*.*                    %PSP_TEMP_PATH%\. /S /Y
copy %BIOS_FV_PATH%\%FD_NAME%.fd                    %PSP_TEMP_PATH%\. /Y
copy %BIOS_FV_PATH%\%RTM_FILE%                      %PSP_TEMP_PATH%\. /Y
copy %PSP_CONFIG_FILE_PATH%\BuildPspImage.bat                   %PSP_TEMP_PATH%\. /Y


set PSPKIT_PATH=%PSP_TEMP_PATH%
set BIOS_IMAGE_CONFIG_FILE=%PSP_CONFIG_FILE_PATH%\BIOSImageDirectory32M.xml
if "%COMPRESS_FSP_REGION%"=="FALSE" (
  Set BIOS_IMAGE_CONFIG_FILE=%PSP_CONFIG_FILE_PATH%\BIOSImageDirectory32M_no_compress_fsp.xml
)
if "%KEY_MODE%"=="NOSIGN" (
  REM remove the line that conatins RTMSignature string for NOSIGN mode
  @findstr /v "RTMSignature" %BIOS_IMAGE_CONFIG_FILE% > %PSP_CONFIG_FILE_PATH%\BIOSImageDirectory-NOSIGN.xml
  set BIOS_IMAGE_CONFIG_FILE=%PSP_CONFIG_FILE_PATH%\BIOSImageDirectory-NOSIGN.xml
)

set OUTPUT_BIOS=%BIOSNAME%UDK-temp.FD

if "%KEY_MODE%"=="NOSIGN" (
  call %PSP_TEMP_PATH%\BuildPspImage.bat %FD_NAME%.fd %RTM_FILE% %BIOS_IMAGE_CONFIG_FILE% %OUTPUT_BIOS% %KEY_MODE%
  if %ERRORLEVEL% NEQ 0 goto ERR_END
)
if "%KEY_MODE%"=="TK" (
  @REM copy TestRtmPrivateKey.pem %PSP_TEMP_PATH%\. /Y
  call %PSP_TEMP_PATH%\BuildPspImage.bat %FD_NAME%.fd %RTM_FILE% %BIOS_IMAGE_CONFIG_FILE% %OUTPUT_BIOS% %KEY_MODE%
  if %ERRORLEVEL% NEQ 0 goto ERR_END
)

copy %PSP_TEMP_PATH%\%OUTPUT_BIOS% %WORKSPACE%\ /Y /B
@echo Final BIOS @ %WORKSPACE%\%OUTPUT_BIOS%

goto END

:ERR_END
  @exit /B 2
:END
  @exit /B 0
