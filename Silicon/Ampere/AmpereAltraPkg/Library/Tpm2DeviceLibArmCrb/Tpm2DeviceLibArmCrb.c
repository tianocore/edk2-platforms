/** @file
  This implements the TPM2 Device Library based on Ampere TPM CRB model.

  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Uefi.h>

#include <Guid/PlatformInfoHob.h>
#include <IndustryStandard/Tpm20.h>
#include <Library/ArmSmcLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/TimerLib.h>
#include <Library/Tpm2DeviceLib.h>

//
// Command Response Buffer (CRB) interface definition
//
// Refer to TPM 2.0 Mobile Command Response Buffer Interface
// Level 00 Revision 12
//

//
// Set structure alignment to 1-byte
//
#pragma pack (1)

//
// Register set map as specified in Section 3
//
typedef struct {
  UINT32    CrbControlRequest;     // 00h
  ///
  /// Register used by the TPM to provide status of the CRB interface.
  ///
  UINT32    CrbControlStatus;      // 04h
  ///
  /// Register used by software to cancel command processing.
  ///
  UINT32    CrbControlCancel;      // 08h
  ///
  /// Register used to indicate presence of command or response data in the CRB buffer.
  ///
  UINT32    CrbControlStart;       // 0Ch
  ///
  /// Register used to configure and respond to interrupts.
  ///
  UINT32    CrbInterruptEnable;    // 10h
  UINT32    CrbInterruptStatus;    // 14h
  ///
  /// Size of the Command buffer.
  ///
  UINT32    CrbControlCommandSize; // 18h
  ///
  /// Command buffer start address
  ///
  UINT32    CrbControlCommandAddressLow;  // 1Ch
  UINT32    CrbControlCommandAddressHigh; // 20h
  ///
  /// Size of the Response buffer
  ///
  UINT32    CrbControlResponseSize; // 24h
  ///
  /// Address of the start of the Response buffer
  ///
  UINT64    CrbControlResponseAddress; // 28h
} PLATFORM_TPM2_CONTROL_AREA;

//
// Define bits of CRB Control Area Request Register
//

///
/// Used by Software to indicate transition the TPM to and from the Idle state
/// 1: Set by Software to indicate response has been read from the response buffer
/// and TPM can transition to Idle.
/// 0: Cleared to 0 by TPM to acknowledge the request when TPM enters Idle state.
///
#define CRB_CONTROL_AREA_REQUEST_GO_IDLE  BIT1

///
/// Used by Software to request the TPM transition to the Ready State.
/// 1: Set to 1 by Software to indicate the TPM should be ready to receive a command.
/// 0: Cleared to 0 by TPM to acknowledge the request.
///
#define CRB_CONTROL_AREA_REQUEST_COMMAND_READY  BIT0

//
// Define bits of CRB Control Area Status Register
//

///
/// Used by TPM to indicate it is in the Idle State
/// 1: Set by TPM when in the Idle State.
/// 0: Cleared by TPM when TPM transitions to the Ready State.
///
#define CRB_CONTROL_AREA_STATUS_TPM_IDLE  BIT1

///
/// Used by the TPM to indicate current status.
/// 1: Set by TPM to indicate a FATAL Error
/// 0: Indicates TPM is operational
///
#define CRB_CONTROL_AREA_STATUS_TPM_STATUS  BIT0

//
// Define bits of CRB Control Cancel Register
//

///
/// Used by software to cancel command processing Reads return correct value
/// Writes (0000 0001h): Cancel a command
/// Writes (0000 0000h): Clears field when command has been cancelled
///
#define CRB_CONTROL_CANCEL  BIT0

//
// Define bits of CRB Control Start Register
//

///
/// When set by software, indicates a command is ready for processing.
/// Writes (0000 0001h): TPM transitions to Command Execution
/// Writes (0000 0000h): TPM clears this field and transitions to Command Completion
///
#define CRB_CONTROL_START  BIT0

//
// Restore original structure alignment
//
#pragma pack ()

typedef enum {
  TPM2_NO_SUPPORT = 0,
  TPM2_CRB_INTERFACE
} PLATFORM_TPM2_INTERFACE_TYPE;

STATIC PLATFORM_TPM2_CONFIG_DATA               mPlatformTpm2Config;
STATIC PLATFORM_TPM2_CRB_INTERFACE_PARAMETERS  mPlatformTpm2InterfaceParams;

//
// Execution of the command may take from several seconds to minutes for certain
// commands, such as key generation.
//
#define CRB_TIMEOUT_MAX_US  (90000 * 1000)            // 90s
#define CRB_POLL_DELAY_US   30

/**
  Check whether the value of a TPM chip register satisfies the input BIT setting.

  @param[in]  Register     Address port of register to be checked.
  @param[in]  BitSet       Check these data bits are set.
  @param[in]  BitClear     Check these data bits are clear.
  @param[in]  TimeOut      The max wait time (unit MicroSecond) when checking register.

  @retval     EFI_SUCCESS  The register satisfies the check bit.
  @retval     EFI_TIMEOUT  The register can't run into the expected status in time.
**/
EFI_STATUS
Tpm2ArmCrbWaitRegisterBits (
  IN UINT32  *Register,
  IN UINT32  BitSet,
  IN UINT32  BitClear,
  IN UINT32  TimeOut
  )
{
  UINT32  RegRead;
  UINT32  WaitTime;

  for (WaitTime = 0; WaitTime < TimeOut; WaitTime += CRB_POLL_DELAY_US) {
    RegRead = MmioRead32 ((UINTN)Register);
    if (((RegRead & BitSet) == BitSet) && ((RegRead & BitClear) == 0)) {
      return EFI_SUCCESS;
    }

    MicroSecondDelay (CRB_POLL_DELAY_US);
  }

  return EFI_TIMEOUT;
}

EFI_STATUS
Tpm2ArmCrbInvokeTpmService (
  VOID
  )
{
  ARM_SMC_ARGS  ArmSmcArgs;

  ZeroMem (&ArmSmcArgs, sizeof (ARM_SMC_ARGS));
  ArmSmcArgs.Arg0 = mPlatformTpm2InterfaceParams.SmcFunctionId;
  ArmCallSmc (&ArmSmcArgs);
  if (ArmSmcArgs.Arg0 != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%d Failed to invoke TPM Service Handler in Trusted Firmware EL3.\n",
      __func__,
      __LINE__
      ));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  This service enables the sending of commands to the TPM2.

  @param[in]      InputParameterBlockSize  Size of the TPM2 input parameter block.
  @param[in]      InputParameterBlock      Pointer to the TPM2 input parameter block.
  @param[in,out]  OutputParameterBlockSize Size of the TPM2 output parameter block.
  @param[in]      OutputParameterBlock     Pointer to the TPM2 output parameter block.

  @retval EFI_SUCCESS            The command byte stream was successfully sent to
                                 the device and a response was successfully received.
  @retval EFI_DEVICE_ERROR       The command was not successfully sent to the device
                                 or a response was not successfully received from the device.
  @retval EFI_BUFFER_TOO_SMALL   The output parameter block is too small.
**/
EFI_STATUS
EFIAPI
Tpm2SubmitCommand (
  IN     UINT32  InputParameterBlockSize,
  IN     UINT8   *InputParameterBlock,
  IN OUT UINT32  *OutputParameterBlockSize,
  IN     UINT8   *OutputParameterBlock
  )
{
  EFI_STATUS                  Status;
  PLATFORM_TPM2_CONTROL_AREA  *Tpm2ControlArea;
  UINTN                       CommandBuffer;
  UINTN                       ResponseBuffer;
  UINT32                      Index;
  UINT32                      TpmOutSize;
  UINT16                      Data16;
  UINT32                      Data32;

  DEBUG_CODE (
    UINTN DebugSize;

    DEBUG ((DEBUG_VERBOSE, "ArmCrbTpmCommand Send - "));
    if (InputParameterBlockSize > 0x100) {
      DebugSize = 0x40;
    } else {
      DebugSize = InputParameterBlockSize;
    }

    for (Index = 0; Index < DebugSize; Index++) {
      DEBUG ((DEBUG_VERBOSE, "%02x ", InputParameterBlock[Index]));
    }

    if (DebugSize != InputParameterBlockSize) {
      DEBUG ((DEBUG_VERBOSE, "...... "));
      for (Index = InputParameterBlockSize - 0x20; Index < InputParameterBlockSize; Index++) {
        DEBUG ((DEBUG_VERBOSE, "%02x ", InputParameterBlock[Index]));
      }
    }

    DEBUG ((DEBUG_VERBOSE, "\n"));
  );

  TpmOutSize = 0;

  Tpm2ControlArea =
    (PLATFORM_TPM2_CONTROL_AREA *)(UINTN)(mPlatformTpm2InterfaceParams.AddressOfControlArea);

  //
  // Write CRB Command Buffer
  //
  CommandBuffer = (UINTN)((UINTN)(Tpm2ControlArea->CrbControlCommandAddressHigh) << 32
                          | Tpm2ControlArea->CrbControlCommandAddressLow);
  MmioWriteBuffer8 (CommandBuffer, InputParameterBlockSize, InputParameterBlock);

  //
  // Set Start bit
  //
  MmioWrite32 ((UINTN)&Tpm2ControlArea->CrbControlStart, CRB_CONTROL_START);

  //
  // The UEFI needs to make a SMC Service Call to invoke TPM Service Handler
  // in Arm Trusted Firmware EL3.
  //
  Status = Tpm2ArmCrbInvokeTpmService ();
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto GoIdle_Exit;
  }

  Status = Tpm2ArmCrbWaitRegisterBits (
             &Tpm2ControlArea->CrbControlStart,
             0,
             CRB_CONTROL_START,
             CRB_TIMEOUT_MAX_US
             );
  if (EFI_ERROR (Status)) {
    //
    // Try to goIdle, the behavior is agnostic.
    //
    Status = EFI_DEVICE_ERROR;
    goto GoIdle_Exit;
  }

  //
  // Get response data header
  //
  ResponseBuffer = (UINTN)(Tpm2ControlArea->CrbControlResponseAddress);
  MmioReadBuffer8 (ResponseBuffer, sizeof (TPM2_RESPONSE_HEADER), OutputParameterBlock);
  DEBUG_CODE (
    DEBUG ((DEBUG_VERBOSE, "ArmCrbTpmCommand ReceiveHeader - "));
    for (Index = 0; Index < sizeof (TPM2_RESPONSE_HEADER); Index++) {
    DEBUG ((DEBUG_VERBOSE, "%02x ", OutputParameterBlock[Index]));
  }

    DEBUG ((DEBUG_VERBOSE, "\n"));
    );

  //
  // Check the response data header (tag, parasize and returncode)
  //
  CopyMem (&Data16, OutputParameterBlock, sizeof (UINT16));
  // TPM2 should not use this RSP_COMMAND
  if (SwapBytes16 (Data16) == TPM_ST_RSP_COMMAND) {
    DEBUG ((DEBUG_ERROR, "TPM2: TPM_ST_RSP error - %x\n", TPM_ST_RSP_COMMAND));
    Status = EFI_UNSUPPORTED;
    goto GoIdle_Exit;
  }

  CopyMem (&Data32, (OutputParameterBlock + 2), sizeof (UINT32));
  TpmOutSize = SwapBytes32 (Data32);
  if (*OutputParameterBlockSize < TpmOutSize) {
    //
    // Command completed, but buffer is not enough
    //
    Status = EFI_BUFFER_TOO_SMALL;
    goto GoIdle_Exit;
  }

  *OutputParameterBlockSize = TpmOutSize;

  //
  // Continue reading the remaining data
  //
  MmioReadBuffer8 (ResponseBuffer, TpmOutSize, OutputParameterBlock);

  DEBUG_CODE (
    DEBUG ((DEBUG_VERBOSE, "ArmCrbTpmCommand Receive - "));
    for (Index = 0; Index < TpmOutSize; Index++) {
    DEBUG ((DEBUG_VERBOSE, "%02x ", OutputParameterBlock[Index]));
  }

    DEBUG ((DEBUG_VERBOSE, "\n"));
    );

GoIdle_Exit:
  //
  //  Always go Idle state.
  //
  MmioWrite32 ((UINTN)&Tpm2ControlArea->CrbControlRequest, CRB_CONTROL_AREA_REQUEST_GO_IDLE);

  return Status;
}

/**
  This service requests use TPM2.

  @retval EFI_SUCCESS      Get the control of TPM2 chip.
  @retval EFI_NOT_FOUND    TPM2 not found.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm2RequestUseTpm (
  VOID
  )
{
  PLATFORM_TPM2_CONTROL_AREA  *ControlAreaPtr;

  if (mPlatformTpm2Config.InterfaceType != TPM2_CRB_INTERFACE) {
    return EFI_NOT_FOUND;
  }

  ControlAreaPtr =
    (PLATFORM_TPM2_CONTROL_AREA *)(UINTN)(mPlatformTpm2InterfaceParams.AddressOfControlArea);

  if (  (ControlAreaPtr->CrbControlCommandAddressLow == 0)
     || (ControlAreaPtr->CrbControlCommandAddressLow == 0xFFFFFFFF)
     || ((ControlAreaPtr->CrbControlStatus & CRB_CONTROL_AREA_STATUS_TPM_STATUS) == 1))
  {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  The function caches current active TPM interface type.

  @retval EFI_SUCCESS   TPM2.0 instance is registered, or system
                        does not support register TPM2.0 instance
**/
EFI_STATUS
EFIAPI
Tpm2DeviceLibConstructor (
  VOID
  )
{
  VOID               *GuidHob;
  PLATFORM_INFO_HOB  *PlatformHob;

  GuidHob = GetFirstGuidHob (&gPlatformInfoHobGuid);
  if (GuidHob == NULL) {
    return EFI_DEVICE_ERROR;
  }

  PlatformHob                  = (PLATFORM_INFO_HOB *)GET_GUID_HOB_DATA (GuidHob);
  mPlatformTpm2Config          = PlatformHob->Tpm2Info.Tpm2ConfigData;
  mPlatformTpm2InterfaceParams = PlatformHob->Tpm2Info.Tpm2CrbInterfaceParams;

  return EFI_SUCCESS;
}
