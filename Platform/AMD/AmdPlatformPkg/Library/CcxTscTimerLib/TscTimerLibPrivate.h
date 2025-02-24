/** @file
  Header file internal to CCX TSC TimerLib.

  Copyright (C) 2008 - 2025, Advanced Micro Devices, Inc. All rights reserved
  Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TSC_TIMER_LIB_PRIVATE_H_
#define TSC_TIMER_LIB_PRIVATE_H_

#include <Library/TimerLib.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>

#pragma pack (push, 1)

///
/// P-state Status Register 0xC0010063
///
#define MSR_PSTATE_STS  0xC0010063ul

///
/// Pstate Status MSR Register
///
typedef union {
  struct {
    ///< Bitfields of Pstate Status MSR Register
    UINT64    CurPstate : 3;           ///< Current Pstate
    UINT64    Reserved  : 61;          ///< Reserved
  } Field;
  UINT64    Value;
} PSTATE_STS_MSR;

///
/// P-state Registers 0xC00100[6B:64]
///
#define MSR_PSTATE_0  0xC0010064ul
#define NM_PS_REG     8

///
/// P-state MSR
///
typedef union {
  struct {
    ///< Bitfields of P-state MSR
    struct {
      UINT32    CpuFid_7_0 : 8;          ///< CpuFid[7:0]
      UINT32    CpuDfsId   : 6;          ///< CpuDfsId
      UINT32    CpuVid     : 8;          ///< CpuVid
      UINT32    IddValue   : 8;          ///< IddValue
      UINT32    IddDiv     : 2;          ///< IddDiv
    } Field_Lo;
    struct {
      UINT32    Reserved : 31;           ///< Reserved
      UINT32    PstateEn : 1;            ///< Pstate Enable
    } Field_Hi;
  } Field;
  UINT64    Value;
} PSTATE_MSR;

///
/// P-state MSR for Zen5 and after
///
typedef union {
  struct {
    ///< Bitfields of P-state MSR
    UINT64    CpuFid_11_0 : 12;        ///< Specifies the core frequency multiplier.
    UINT64    Reserved    : 2;         ///< Reserved
    UINT64    CpuVid_7_0  : 8;         ///< CpuVid[7:0]
    UINT64    IddValue    : 8;         ///< IddDiv and IddValue combine to specify the expected maximum current dissipation of a single core
    UINT64    IddDiv      : 2;         ///< See IddValue.
    UINT64    CpuVid_8    : 1;         ///< CpuVid[8]
    UINT64    Reserved1   : 30;        ///< Reserved
    UINT64    PstateEn    : 1;         ///< 1=The P-state specified by this MSR is valid.
  } Field;
  UINT64    Value;
} PSTATE_MSR_ZEN5;

///
/// Advanced Power Management Information
///
#define APMINFOEDX_CPUID_FN  0x80000007

///
/// Advanced Power Management Information
///
typedef union {
  struct {
    UINT32    Ts                    : 1;    ///< Temperature sensor.
    UINT32    Reserved              : 2;    ///< Reserved
    UINT32    Ttp                   : 1;    ///< THERMTRIP.
    UINT32    Tm                    : 1;    ///< Hardware thermal control (HTC)
    UINT32    Reserved1             : 1;    ///< Reserved
    UINT32    OneHundredMHzSteps    : 1;    ///< 100 MHz multiplier Control.
    UINT32    HwPstate              : 1;    ///< Core::X86::Msr::PStateCurLim, Core::X86::Msr::PStateCtl and Core::X86::Msr::PStateStat exist.
    UINT32    TscInvariant          : 1;    ///< The TSC rate is invariant.
    UINT32    Cpb                   : 1;
    UINT32    EffFreqRO             : 1;    ///< Indicates presence of Core::X86::Msr::MPerfReadOnly and Core::X86::Msr::APerfReadOnly.
    UINT32    ProcFeedbackInterface : 1;
    UINT32    ProcPowerReporting    : 1;    ///< Core power reporting interface supported.
    UINT32    ConnectedStandby      : 1;    ///< Connected Standby.
    UINT32    Rapl                  : 1;    ///< Running average power limit.
    UINT32    Reserved2             : 17;   ///< Reserved
  } Field;
  UINT32    Value;
} APM_INFO_EDX;

#pragma pack (pop)

///
/// TSC Frequency Information.
///
typedef struct {
  UINT64          TscFrequency;
  UINT64          CurrentPStateReg;
  UINT32          CurrentPState;
  APM_INFO_EDX    ApmInfoEdx;
} AMD_TSC_FREQUENCY_INFORMATION;

/**
  Get TSC frequency.

  @return The number of TSC counts per second.

**/
UINT64
InternalGetTscFrequency (
  VOID
  );

/**
  Get TSC frequency.information

  This function will evaluate TSC counting frequency and record related
  MSR values.

  @param[out]  Info  Pointer to information buffer

**/
VOID
InternalGetTscFrequencyInformation (
  OUT  AMD_TSC_FREQUENCY_INFORMATION  *Info
  );

#endif
