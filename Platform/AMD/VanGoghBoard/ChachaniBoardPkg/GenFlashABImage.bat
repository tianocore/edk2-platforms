@REM @file
@REM   Generate final 16MB Flash A/B Image
@REM
@REM Copyright (C) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>
@REM SPDX-License-Identifier: BSD-2-Clause-Patent
@REM

@echo off
@set BINARY_BUILD_PATH=%WORKSPACE%\Build\ChachaniBoardPkg\%TARGET%_%TOOL_CHAIN_TAG%\IA32\VanGoghCommonPkg\Flash_AB

@set F1_ECSIG=%WORKSPACE%\%PROJECT_PKG%\Binary\EC\EcSig.bin
@set F2_EC=%WORKSPACE%\%PROJECT_PKG%\Binary\EC\ChachaniEC.bin
@set F3_EFS=%BINARY_BUILD_PATH%\NewEFS\NewEFS\OUTPUT\NewEFS.bin
@set F4_PSP_L1_DIRECTORY=%BINARY_BUILD_PATH%\PspL1Directory\PspL1Directory\OUTPUT\PspL1Directory.bin
@set F5_PD=%WORKSPACE%\%PROJECT_PKG%\Binary\PD\TIPD.bin
@set F6_SLOT_HEADER_1=%BINARY_BUILD_PATH%\ImageSlotHeader\ImageSlotHeader_1\OUTPUT\ImageSlotHeader_1.bin
@set F7_SLOT_HEADER_2=%BINARY_BUILD_PATH%\ImageSlotHeader\ImageSlotHeader_2\OUTPUT\ImageSlotHeader_2.bin
@set F8_SLOT_A=%WORKSPACE%\%OUTPUT_BIOS%
@set F9_SLOT_B=%WORKSPACE%\%OUTPUT_BIOS%
@set F10_OUT_IMAGE=%WORKSPACE%\%BIOSNAME%UDK.FD


IF EXIST "%PYTHON_HOME%\python.exe" (
  :: Update Checksum field for PSP_L1_DIRECTORY binary
  %PYTHON_HOME%\python.exe py-UpdatePspL1DirCksm.py %F4_PSP_L1_DIRECTORY%
  :: Generate final 32MB A image
  %PYTHON_HOME%\python.exe FlashABImage32M.py %F1_ECSIG% %F2_EC% %F3_EFS% %F4_PSP_L1_DIRECTORY% %F5_PD% %F6_SLOT_HEADER_1% %F7_SLOT_HEADER_2% %F8_SLOT_A% %F9_SLOT_B% %F10_OUT_IMAGE%
  del %F8_SLOT_A%
  GOTO END
)ELSE (
  @echo !!!PYTHON_HOME system variable is NOT set correctly!!
  goto ERROR
)

:ERROR
@echo **** Build Flash A/B Image Failed ****
@exit /B 2

:END