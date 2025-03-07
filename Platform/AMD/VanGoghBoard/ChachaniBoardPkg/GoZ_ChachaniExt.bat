@REM @file
@REM   Windows batch file to launch Chachani Board BIOS build
@REM
@REM Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
@REM SPDX-License-Identifier: BSD-2-Clause-Patent
@REM

set UDK=edk2
set OemBoard=Chachani
set PLATFORM_PATH=edk2-platforms\Platform\AMD\VanGoghBoard
set BUILD_TYPE=EXTERNAL
:: TRUE / FALSE
set COMPRESS_FSP_REGION=TRUE
set BDK=FspO
@if not exist %cd%\%BDK% set BDK=%PLATFORM_PATH%\AgesaPublic
set PACKAGES_PATH=%cd%\%UDK%;%cd%\%BDK%;%cd%\%PLATFORM_PATH%;%cd%\edk2-non-osi\Silicon\AMD\VanGogh;%cd%\edk2-platforms\Silicon\AMD\VanGoghBoard;%cd%\edk2-platforms\Platform\AMD;
set IASL_PREFIX=C:\ASL\
set PYTHON_HOME=C:\Python39
set WORKSPACE=%CD%
echo DEFINE INTERNAL_IDS = NO> %PLATFORM_PATH%\ChachaniBoardPkg\Set.env
echo DEFINE BUILD_BOARD = Chachani>> %PLATFORM_PATH%\ChachaniBoardPkg\Set.env


@for /f "tokens=3" %%a in ('find "TOOL_CHAIN_TAG" %PLATFORM_PATH%\ChachaniBoardPkg\Conf\target.txt') do @set TOOL_CHAIN_TAG=%%a
:: Supported Compiler: VS2017, VS2013x86
@if "%TOOL_CHAIN_TAG%"=="VS2017"    goto SetVs2017Env
@if "%TOOL_CHAIN_TAG%"=="VS2013x86" goto SetVs2013Env
@echo Build tool chain setting is not correct!
goto err_end

:SetVs2017Env
@REM Set the MSVC compiler path for VS2017
FOR /F "delims==" %%i IN (
'powershell -Command "echo "$^(Get-CimInstance MSFT_VSInstance^).InstallLocation ^| findstr 2017""'
) DO SET VS2017_INSTALLATION=%%i
:: Bug fix for newer VS2017 installation
@if "%VS2017_INSTALLATION%"=="" (
  FOR /F "delims==" %%i IN (
    'powershell -Command "echo "$^(Get-CimInstance MSFT_VSInstance -Namespace root/cimv2/vs^).InstallLocation ^| findstr 2017""'
  ) DO SET VS2017_INSTALLATION=%%i
)
@dir /b "%VS2017_INSTALLATION%\VC\Tools\MSVC" >%TEMP%\VsDir.txt
@for /f %%z in ('type %TEMP%\VsDir.txt') do @set VS2017_PREFIX=%VS2017_INSTALLATION%\VC\Tools\MSVC\%%z\
@echo VS2017_PREFIX=%VS2017_PREFIX%
@REM Set the Windows SDK path for VS2017
FOR /F "delims==" %%i IN (
'powershell -Command "echo "$^(Get-ItemProperty -Path \"Registry::HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows Kits\Installed Roots\"^).\"KitsRoot10\"""'
) DO SET WIN10_SDK_INSTALLATION=%%i
@for /f "tokens=3 delims==" %%z in ('find "PlatformIdentity" "%WIN10_SDK_INSTALLATION%SDKManifest.xml"') do @set WinSdkVerStr=%%~z
@set WinSdkVerStr=%WinSdkVerStr: =%
@set WinSdk_PREFIX=%WIN10_SDK_INSTALLATION%Lib\%WinSdkVerStr:~0,-1%\
@set WinSdk_Inc_PREFIX=%WIN10_SDK_INSTALLATION%Include\%WinSdkVerStr:~0,-1%\
@echo WinSdk_PREFIX=%WinSdk_PREFIX%
:: Carefully process RC path.
@set WINSDK_PATH_FOR_RC_EXE=%WIN10_SDK_INSTALLATION%bin\%WinSdkVerStr:~0,-1%\x86
@if not EXIST "%WINSDK_PATH_FOR_RC_EXE%\rc.exe" (
  @echo RC not found. Did you install Windows 10 SDK?
  goto err_end
)
set path=%VS2017_PREFIX%bin\Hostx86\x86;%path%
set include=%VS2017_PREFIX%include;%WinSdk_Inc_PREFIX%ucrt;%WinSdk_Inc_PREFIX%um;%WinSdk_Inc_PREFIX%shared
set lib=%VS2017_PREFIX%lib\x86;%WinSdk_PREFIX%um\x86;%WinSdk_PREFIX%ucrt\x86
goto VsEnvSetDone

:SetVs2013Env
set VS2013_PREFIX=%ProgramFiles(x86)%\Microsoft Visual Studio 12.0\
set path=%VS2013_PREFIX%VC\bin;%path%
set include=%VS2013_PREFIX%VC\include
set lib=%ProgramFiles(x86)%\Microsoft SDKs\Windows\v7.1A\lib;%VS2013_PREFIX%VC\lib

:VsEnvSetDone
@set WINSDK10_PREFIX=%ProgramFiles(x86)%\Windows Kits\10\bin\%WinSdkVerStr:~0,-1%\
@echo WINSDK10_PREFIX=%WINSDK10_PREFIX%
call %UDK%\edksetup.bat Rebuild
cd %PLATFORM_PATH%\ChachaniBoardPkg
@cmd.exe

:err_end
@exit /B 2


