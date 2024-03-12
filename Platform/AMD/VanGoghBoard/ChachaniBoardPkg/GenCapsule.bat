@REM @file
@REM   Generate final 16MB Flash A/B Image
@REM
@REM Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>
@REM SPDX-License-Identifier: BSD-2-Clause-Patent
@REM

@REM ============================================
@REM          Run Capsule build process
@REM ============================================
@echo off

if "%PROJECT_PKG%" == "" (
  set PROJECT_PKG=%PLATFORM_PATH%\ChachaniBoardPkg
)

if "%BOARD_PKG%" == "" (
  set BOARD_PKG=ChachaniBoardPkg
)

:: Set BIOS File for Capsule Build
set BIOS_FILE_NAME=ChachaniExtUDK.FD

if %BUILD_TYPE% == INTERNAL (
  echo Capsule Internal build
  set BIOS_FILE_NAME=ChachaniIntUDK.FD
) else if %BUILD_TYPE% == EXTERNAL (
  echo Capsule External build
  set BIOS_FILE_NAME=ChachaniExtUDK.FD
)

echo %BOARD_PKG%

if not exist %WORKSPACE%\%BIOS_FILE_NAME% (
  echo No BIOS file found!
  goto ERROR
)

echo Setup OpenSSL Command Line Environment
if not "%OPENSSL_PATH%" == "" (
  set OPENSSL_PATH_TEMP=%OPENSSL_PATH%
)
set OPENSSL_PATH=%WORKSPACE%\%PROJECT_PKG%\Tools\OpenSSL-Win32\bin
if not exist %OPENSSL_PATH%\openssl.exe (
  echo OPENSSL_PATH variable incorrectly set!
  goto ERROR
)

@for /f "tokens=3" %%a in ('find "TARGET " %WORKSPACE%\%PROJECT_PKG%\Conf\target.txt') do @set TARGET=%%a
@for /f "tokens=3" %%a in ('find "TOOL_CHAIN_TAG" %WORKSPACE%\%PROJECT_PKG%\Conf\target.txt') do @set TOOL_CHAIN_TAG=%%a

set OUTPUT_DIR=Build\%BOARD_PKG%\%TARGET%_%TOOL_CHAIN_TAG%
:: Set Output Capsule File Name
set OTA_CAPSULE_NAME=OtaCapsule.cap

@if exist %WORKSPACE%\%OUTPUT_DIR%\FV\SYSTEMFIRMWAREUPDATECARGO*.* (
  pushd %WORKSPACE%\%OUTPUT_DIR%\FV
  del SYSTEMFIRMWAREUPDATECARGO*.*
  popd
)

echo > %WORKSPACE%\%OUTPUT_DIR%\FV\SYSTEMFIRMWAREUPDATECARGO.Fv
call build -p %WORKSPACE%\%PROJECT_PKG%\PlatformCapsule.dsc --conf=%WORKSPACE%/%PROJECT_PKG%/Conf -D BIOS_FILE=%BIOS_FILE_NAME%
@if %ERRORLEVEL% NEQ 0 goto ERROR

copy /b %WORKSPACE%\%OUTPUT_DIR%\FV\%OTA_CAPSULE_NAME% %WORKSPACE%\%OTA_CAPSULE_NAME% /y

echo Build capsule completed.
goto DONE

:ERROR
  if not "%OPENSSL_PATH_TEMP%" == "" (
    set OPENSSL_PATH=%OPENSSL_PATH_TEMP%
  )
  @exit /B 2

:DONE
  if not "%OPENSSL_PATH_TEMP%" == "" (
      set OPENSSL_PATH=%OPENSSL_PATH_TEMP%
    )
  @exit /B 0
