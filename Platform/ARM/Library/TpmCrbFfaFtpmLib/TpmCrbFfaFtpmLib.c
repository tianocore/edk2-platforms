/** @file TpmCrbFfa Library which is software based TPM using TpmLib.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

   @par Glossary:
     - FF-A - Firmware Framework for Arm A-profile
     - CRB - Command Response Buffer

   @par Reference(s):
     - Arm Firmware Framework for Arm A-Profile [https://developer.arm.com/documentation/den0077/latest]
     - CRB over FF-A [https://developer.arm.com/documentation/den0138/latest/]

**/
#include <PiMm.h>

#include <Library/ArmFfaLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/TpmCrbFfaDeviceLib.h>
#include <Library/TpmLib.h>
#include <Library/HobLib.h>
#include <Library/PlatformTpmLib.h>
#include <Library/MmServicesTableLib.h>

#include <IndustryStandard/Tpm20.h>
#include <IndustryStandard/TpmPtp.h>
#include <IndustryStandard/UefiTcgPlatform.h>

#include <Guid/Tpm2ServiceFfa.h>

#define CRB_BUFFER_SIZE        (sizeof (PTP_CRB_REGISTERS) - OFFSET_OF (PTP_CRB_REGISTERS, CrbDataBuffer))
#define SECURE_LOCALITY_START  4

#pragma pack(1)

/** Tpm Startup Command structure
 */
typedef struct {
  /// TPM Command Header
  TPM2_COMMAND_HEADER    Header;

  /// TPM Startup command Type
  TPM_SU                 StartupType;
} TPM2_STARTUP_COMMAND;

/** Tpm Startup Response structure
 */
typedef struct {
  /// TPM Response Header
  TPM2_RESPONSE_HEADER    Header;
} TPM2_STARTUP_RESPONSE;

/** Tpm PCR Extend Command structure
 */
typedef struct {
  TPM2_COMMAND_HEADER    Header;
  TPMI_DH_PCR            PcrHandle;
  UINT32                 AuthorizationSize;
  TPMS_AUTH_COMMAND      AuthSessionPcr;
  TPML_DIGEST_VALUES     DigestValues;
} TPM2_PCR_EXTEND_COMMAND;

/** Tpm PCR Extend response structure
 */
typedef struct {
  TPM2_RESPONSE_HEADER    Header;
  UINT32                  ParameterSize;
  TPMS_AUTH_RESPONSE      AuthSessionPcr;
} TPM2_PCR_EXTEND_RESPONSE;

#pragma pack()

//
// Crb Buffer Type.
//
typedef enum {
  CRB_REGISTER = 0x00,
  CRB_COMMAND,
  CRB_RESPONSE,
} CRB_TYPE;

typedef struct {
  TPMI_ALG_HASH    HashAlgo;
  UINT32           HashMask;
  UINT16           HashSize;
  CHAR8            *HashName;
} INTERNAL_HASH_INFO;

STATIC VOID     *mTmpCommandBuffer;
STATIC VOID     *mTmpResponseBuffer;
STATIC VOID     *mEventLog;
STATIC UINTN    mEventLogSize;
STATIC BOOLEAN  mFtpmAvailable = FALSE;

STATIC INTERNAL_HASH_INFO  mHashInfo[] = {
  { TPM_ALG_SHA256, HASH_ALG_SHA256, SHA256_DIGEST_SIZE, "SHA256" },
  { TPM_ALG_SHA384, HASH_ALG_SHA384, SHA384_DIGEST_SIZE, "SHA384" },
  { TPM_ALG_SHA512, HASH_ALG_SHA512, SHA512_DIGEST_SIZE, "SHA512" },
};

/**
  Get Command Response Buffer address.

  @param [in]   Locality          Locality
  @param [in]   Type              Crb buffer type

  @retval Address                 Crb buffer address matched with Locality
                                  and Type. If error, NULL

**/
STATIC
VOID *
EFIAPI
GetCrbBuffer (
  IN UINT8     Locality,
  IN CRB_TYPE  Type
  )
{
  PTP_CRB_REGISTERS  *CrbReg;
  UINT64             BufferAddr;

  if (Locality >= NUM_LOCALITIES) {
    // Invalid locality
    ASSERT (0);
    return NULL;
  }

  if (Locality == SECURE_LOCALITY_START) {
    CrbReg = (PTP_CRB_REGISTERS *)(FixedPcdGet64 (PcdTpmSecureCrbBase) +
                                   ((Locality - SECURE_LOCALITY_START) * sizeof (PTP_CRB_REGISTERS)));
  } else {
    CrbReg = (PTP_CRB_REGISTERS *)(FixedPcdGet64 (PcdTpmBaseAddress) +
                                   (Locality * sizeof (PTP_CRB_REGISTERS)));
  }

  switch (Type) {
    case CRB_REGISTER:
      return CrbReg;
    case CRB_COMMAND:
      BufferAddr = (((UINT64)CrbReg->CrbControlCommandAddressHigh << 32) |
                    CrbReg->CrbControlCommandAddressLow);
      return (VOID *)BufferAddr;
    case CRB_RESPONSE:
      return (VOID *)CrbReg->CrbControlResponseAddrss;
  }

  return NULL;
}

/**
  Get hash size based on Algo

  @param[in]     HashAlgo           Hash Algorithm Id.

  @return Size of the hash.
**/
STATIC
EFI_STATUS
EFIAPI
GetHashInfo (
  IN TPMI_ALG_HASH        HashAlgo,
  OUT INTERNAL_HASH_INFO  **HashInfo
  )
{
  UINTN  Idx;

  for (Idx = 0; Idx < ARRAY_SIZE (mHashInfo); Idx++) {
    if (mHashInfo[Idx].HashAlgo == HashAlgo) {
      *HashInfo = &mHashInfo[Idx];
      return EFI_SUCCESS;
    }
  }

  return EFI_UNSUPPORTED;
}

/**
  Set empty TPM session

  @param[out]  AuthSessionBuffer     AuthSessionBuffer

  @return     Size                   AuthSession size

 **/
STATIC
UINT32
EFIAPI
SetEmptyAuthSession (
  IN UINT8  *AuthSessionBuffer
  )
{
  UINT8  *Buffer;

  Buffer = AuthSessionBuffer;

  // sessionHandle
  WriteUnaligned32 ((UINT32 *)Buffer, SwapBytes32 (TPM_RS_PW));
  Buffer += sizeof (UINT32);

  // nonce = nullNonce
  WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (0));
  Buffer += sizeof (UINT16);

  // sessionAttributes = 0
  *Buffer = 0x00;
  Buffer++;

  // hmac = nullAuth
  WriteUnaligned16 ((UINT16 *)Buffer, SwapBytes16 (0));
  Buffer += sizeof (UINT16);

  return (UINT32)(Buffer - AuthSessionBuffer);
}

/**
  This function dump TCG_EfiSpecIDEventStruct.

  @param[in,out]  LogAddr    Event address
  @param[in,out]  LogSize    Left size of events

**/
VOID
EFIAPI
DumpTcgEfiSpecIdEvent (
  IN OUT UINT8  **LogAddr,
  IN OUT UINTN  *LogSize
  )
{
  TCG_PCR_EVENT                    *TcgPcrEvent;
  TCG_EfiSpecIDEventStruct         *TcgEfiSpecIdEventStruct;
  TCG_EfiSpecIdEventAlgorithmSize  *DigestSize;
  UINTN                            Idx;
  UINT8                            *VendorInfoSize;
  UINT8                            *VendorInfo;
  UINT32                           NumberOfAlgorithms;

  TcgPcrEvent             = (TCG_PCR_EVENT *)*LogAddr;
  TcgEfiSpecIdEventStruct = (TCG_EfiSpecIDEventStruct *)(*LogAddr + (sizeof (TCG_PCR_EVENT) - 1));

  DEBUG ((DEBUG_INFO, "  TCG_EfiSpecIDEvent:\n"));
  DEBUG ((DEBUG_INFO, "    PCRIndex  - %d\n", TcgPcrEvent->PCRIndex));
  DEBUG ((DEBUG_INFO, "    EventType - 0x%08x\n", TcgPcrEvent->EventType));

  DEBUG ((DEBUG_INFO, "    Digest: "));
  for (Idx = 0; Idx < TPM_SHA1_160_HASH_LEN; Idx++) {
    DEBUG ((DEBUG_INFO, "%02x ", TcgPcrEvent->Digest.digest[Idx]));
  }

  DEBUG ((DEBUG_INFO, "\n"));

  DEBUG ((DEBUG_INFO, "     Signature          - '"));
  for (Idx = 0; Idx < sizeof (TcgEfiSpecIdEventStruct->signature); Idx++) {
    DEBUG ((DEBUG_INFO, "%c", TcgEfiSpecIdEventStruct->signature[Idx]));
  }

  DEBUG ((DEBUG_INFO, "'\n"));

  DEBUG ((DEBUG_INFO, "     PlatformClass      - 0x%08x\n", TcgEfiSpecIdEventStruct->platformClass));
  DEBUG ((DEBUG_INFO, "     SpecVersion        - %d.%d%d\n", TcgEfiSpecIdEventStruct->specVersionMajor, TcgEfiSpecIdEventStruct->specVersionMinor, TcgEfiSpecIdEventStruct->specErrata));
  DEBUG ((DEBUG_INFO, "     UintnSize          - 0x%02x\n", TcgEfiSpecIdEventStruct->uintnSize));

  CopyMem (&NumberOfAlgorithms, TcgEfiSpecIdEventStruct + 1, sizeof (NumberOfAlgorithms));
  DEBUG ((DEBUG_INFO, "     NumberOfAlgorithms - 0x%08x\n", NumberOfAlgorithms));

  DigestSize = (TCG_EfiSpecIdEventAlgorithmSize *)((UINT8 *)TcgEfiSpecIdEventStruct + sizeof (*TcgEfiSpecIdEventStruct) + sizeof (NumberOfAlgorithms));
  for (Idx = 0; Idx < NumberOfAlgorithms; Idx++) {
    DEBUG ((DEBUG_INFO, "     Digest(%d)\n", Idx));
    DEBUG ((DEBUG_INFO, "       AlgorithmId      - 0x%04x\n", DigestSize[Idx].algorithmId));
    DEBUG ((DEBUG_INFO, "       DigestSize       - 0x%04x\n", DigestSize[Idx].digestSize));
  }

  VendorInfoSize = (UINT8 *)&DigestSize[NumberOfAlgorithms];
  DEBUG ((DEBUG_INFO, "    VendorInfoSize     - 0x%02x\n", *VendorInfoSize));
  VendorInfo = VendorInfoSize + 1;
  DEBUG ((DEBUG_INFO, "    VendorInfo         - "));
  for (Idx = 0; Idx < *VendorInfoSize; Idx++) {
    DEBUG ((DEBUG_INFO, "%02x ", VendorInfo[Idx]));
  }

  DEBUG ((DEBUG_INFO, "\n"));

  *LogSize -= (UINTN)(VendorInfo + *VendorInfoSize - *LogAddr);
  *LogAddr  = (UINT8 *)(VendorInfo + *VendorInfoSize);
}

/**
  This function Dump PCR event 2.

  @param[in,out]  LogAddr    Event address
  @param[in,out]  LogSize    Left size of events

**/
STATIC
VOID
EFIAPI
DumpEvent (
  IN OUT UINT8  **LogAddr,
  IN OUT UINTN  *LogSize
  )
{
  EFI_STATUS                   Status;
  UINTN                        Idx;
  UINT32                       DigestIdx;
  UINT32                       DigestCount;
  TPMI_ALG_HASH                HashAlgo;
  INTERNAL_HASH_INFO           *HashInfo;
  UINT8                        *DigestBuffer;
  UINT32                       EventSize;
  UINT8                        *EventBuffer;
  TCG_EfiStartupLocalityEvent  *StartupLocalityEvent;
  TCG_PCR_EVENT2               *TcgPcrEvent2;
  UINT16                       DigestSize;

  TcgPcrEvent2 = ((TCG_PCR_EVENT2 *)*LogAddr);

  DEBUG ((DEBUG_INFO, "  Event:\n"));
  DEBUG ((DEBUG_INFO, "    PCRIndex  - %d\n", TcgPcrEvent2->PCRIndex));
  DEBUG ((DEBUG_INFO, "    EventType - 0x%08x\n", TcgPcrEvent2->EventType));

  DEBUG ((DEBUG_INFO, "    DigestCount: 0x%08x\n", TcgPcrEvent2->Digest.count));

  DigestCount  = TcgPcrEvent2->Digest.count;
  HashAlgo     = TcgPcrEvent2->Digest.digests[0].hashAlg;
  DigestBuffer = (UINT8 *)&TcgPcrEvent2->Digest.digests[0].digest;

  for (DigestIdx = 0; DigestIdx < DigestCount; DigestIdx++) {
    Status = GetHashInfo (HashAlgo, &HashInfo);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "      HashAlgo : Unknown(0x%04x)\n", HashAlgo));
      return;
    } else {
      DEBUG ((DEBUG_INFO, "      HashAlgo : %a(0x%04x)\n", HashInfo->HashName, HashAlgo));
      DigestSize = HashInfo->HashSize;
    }

    DEBUG ((DEBUG_INFO, "      Digest(%d): ", DigestIdx));
    for (Idx = 0; Idx < DigestSize; Idx++) {
      DEBUG ((DEBUG_INFO, "%02x ", DigestBuffer[Idx]));
    }

    DEBUG ((DEBUG_INFO, "\n"));

    //
    // Prepare next
    //
    CopyMem (&HashAlgo, DigestBuffer + DigestSize, sizeof (TPMI_ALG_HASH));
    DigestBuffer = DigestBuffer + DigestSize + sizeof (TPMI_ALG_HASH);
  }

  DEBUG ((DEBUG_INFO, "\n"));

  DigestBuffer = DigestBuffer - sizeof (TPMI_ALG_HASH);
  CopyMem (&EventSize, DigestBuffer, sizeof (TcgPcrEvent2->EventSize));
  DEBUG ((DEBUG_INFO, "    EventSize - 0x%08x\n", EventSize));
  EventBuffer = DigestBuffer + sizeof (TcgPcrEvent2->EventSize);

  if ((EventSize == sizeof (TCG_EfiStartupLocalityEvent)) &&
      (CompareMem (EventBuffer, TCG_EfiStartupLocalityEvent_SIGNATURE, 16) == 0))
  {
    StartupLocalityEvent = (TCG_EfiStartupLocalityEvent *)EventBuffer;
    DEBUG ((DEBUG_INFO, "    Signature - %a\n", StartupLocalityEvent->Signature));
    DEBUG ((DEBUG_INFO, "    StartupLocality - 0x%08x\n", StartupLocalityEvent->StartupLocality));
  } else {
    DEBUG ((DEBUG_INFO, "    Event     - %a\n", EventBuffer));
  }

  *LogSize -= (UINTN)(EventBuffer + EventSize - *LogAddr);
  *LogAddr  = (UINT8 *)(EventBuffer + EventSize);
}

/**
  Dump passed tpm event log from TF-A.

 **/
STATIC
VOID
EFIAPI
DumpTpmEventLog (
  IN VOID
  )
{
  UINT8  *EventLog;
  UINTN  EventLogSize;

  EventLog     = (UINT8 *)mEventLog;
  EventLogSize = mEventLogSize;

  DumpTcgEfiSpecIdEvent (&EventLog, &EventLogSize);

  while (EventLogSize != 0) {
    DumpEvent (&EventLog, &EventLogSize);
  }
}

/**
  Strip SpecId Event which comes from TF-A

  @param[out]  StrippedLogAddr   Header stripped event log addr
  @param[out]  StrippedLogSize   Size of stripped event log size

  @retval EFI_SUCECSS            event log is valid.
  @retval EFI_INVALID_PARAMETER  event log is invalid.

 **/
STATIC
EFI_STATUS
EFIAPI
ValidateAndStripSpecIdEvent (
  OUT UINT8  **StrippedLogAddr,
  OUT UINTN  *StrippedLogSize
  )
{
  EFI_STATUS                       Status;
  VOID                             *EventLog;
  TCG_PCR_EVENT                    *TcgPcrEvent;
  TCG_EfiSpecIDEventStruct         *TcgEfiSpecIdEventStruct;
  UINTN                            Idx;
  TCG_EfiSpecIdEventAlgorithmSize  *DigestSize;
  UINT32                           NumberOfAlgorithms;
  INTERNAL_HASH_INFO               *HashInfo;

  *StrippedLogAddr = NULL;
  *StrippedLogSize = 0;
  EventLog         = mEventLog;

  TcgPcrEvent = (TCG_PCR_EVENT *)mEventLog;

  if ((mEventLogSize < OFFSET_OF (TCG_PCR_EVENT, Event)) ||
      (mEventLogSize < OFFSET_OF (TCG_PCR_EVENT, Event) + TcgPcrEvent->EventSize))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid event log size...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  TcgEfiSpecIdEventStruct = (TCG_EfiSpecIDEventStruct *)
                            (mEventLog + OFFSET_OF (TCG_PCR_EVENT, Event));

  /*
   * Strip SpecId Event. This will be generated again by Tcg2Dxe.
   */
  if (!((TcgPcrEvent->EventType == EV_NO_ACTION) &&
        ((CompareMem (TcgEfiSpecIdEventStruct->signature, TCG_EfiSpecIDEventStruct_SIGNATURE_02, 16) == 0) ||
         (CompareMem (TcgEfiSpecIdEventStruct->signature, TCG_EfiSpecIDEventStruct_SIGNATURE_03, 16) == 0))))
  {
    DEBUG ((DEBUG_ERROR, "%a: Event log passed from TF-A doesn't have SpecIdEvent...\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (&NumberOfAlgorithms, TcgEfiSpecIdEventStruct + 1, sizeof (NumberOfAlgorithms));
  if (NumberOfAlgorithms == 0) {
    DEBUG ((DEBUG_ERROR, "%a: No algorithm present for event log!\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  DigestSize = (TCG_EfiSpecIdEventAlgorithmSize *)((UINT8 *)TcgEfiSpecIdEventStruct + sizeof (*TcgEfiSpecIdEventStruct) + sizeof (NumberOfAlgorithms));

  for (Idx = 0; Idx < NumberOfAlgorithms; Idx++) {
    Status = GetHashInfo (DigestSize[Idx].algorithmId, &HashInfo);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Unsupported algorithm found in event log. id: 0x%04x\n",
        __func__,
        DigestSize[Idx].algorithmId
        ));
      return Status;
    }
  }

  EventLog += OFFSET_OF (TCG_PCR_EVENT, Event) + TcgPcrEvent->EventSize;

  *StrippedLogAddr = EventLog;
  *StrippedLogSize = mEventLogSize - (UINTN)(EventLog - mEventLog);

  return EFI_SUCCESS;
}

/**
  Extend one event Log.

  @param[in,out]  EventAddr           Tpm event log address
  @param[in,out]  TotalEventSize      Left event log size

  @return EFI_SUCCESS
  @return EFI_UNSUPPORTED

 **/
STATIC
EFI_STATUS
EFIAPI
ExtendOneEventLog (
  IN OUT UINT8  **EventAddr,
  IN OUT UINTN  *TotalEventSize
  )
{
  EFI_STATUS                Status;
  TCG_PCR_EVENT2            *TcgPcrEvent2;
  TPM2_PCR_EXTEND_COMMAND   ExtendCmd;
  UINT32                    ExtendCmdSize;
  TPM2_PCR_EXTEND_RESPONSE  ExtendRes;
  UINT32                    ExtendResSize;
  TPM2_PCR_EXTEND_RESPONSE  *Response;
  UINTN                     Idx;
  UINT32                    DigestCount;
  TPMI_ALG_HASH             HashAlgo;
  INTERNAL_HASH_INFO        *HashInfo;
  UINT8                     *NextEvent;
  UINT32                    EventSize;
  UINT8                     *DigestBuffer;
  UINT8                     *CommandBuffer;
  UINT32                    SessionSize;
  TPM_RC                    ResponseCode;

  TcgPcrEvent2 = ((TCG_PCR_EVENT2 *)*EventAddr);
  DigestCount  = TcgPcrEvent2->Digest.count;

  if (DigestCount > ARRAY_SIZE (mHashInfo)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Wrong digest count(%d), max: %d\n",
      __func__,
      DigestCount,
      ARRAY_SIZE (mHashInfo)
      ));
    return EFI_INVALID_PARAMETER;
  }

  // Prepare response
  ExtendResSize = sizeof (ExtendRes);
  Response      = &ExtendRes;

  // Prepare TPM PCR Extend command
  ExtendCmd.Header.tag         = SwapBytes16 (TPM_ST_SESSIONS);
  ExtendCmd.Header.commandCode = SwapBytes32 (TPM_CC_PCR_Extend);
  ExtendCmd.PcrHandle          = SwapBytes32 (TcgPcrEvent2->PCRIndex);

  // Set empty session
  CommandBuffer               = (UINT8 *)&ExtendCmd.AuthSessionPcr;
  SessionSize                 = SetEmptyAuthSession (CommandBuffer);
  CommandBuffer              += SessionSize;
  ExtendCmd.AuthorizationSize = SwapBytes32 (SessionSize);

  // Copy digest count
  WriteUnaligned32 ((UINT32 *)CommandBuffer, SwapBytes32 (DigestCount));
  CommandBuffer += sizeof (UINT32);

  HashAlgo     = TcgPcrEvent2->Digest.digests[0].hashAlg;
  DigestBuffer = (UINT8 *)&TcgPcrEvent2->Digest.digests[0].digest;

  // Setting Digest into CommandBuffer
  for (Idx = 0; Idx < DigestCount; Idx++) {
    Status = GetHashInfo (HashAlgo, &HashInfo);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Unsupported TPM hash algo: 0x%x\n", __func__, HashAlgo));
      return Status;
    }

    WriteUnaligned16 ((UINT16 *)CommandBuffer, SwapBytes16 (HashAlgo));
    CommandBuffer += sizeof (TPMI_ALG_HASH);

    CopyMem (CommandBuffer, DigestBuffer, HashInfo->HashSize);
    CommandBuffer += HashInfo->HashSize;

    DigestBuffer += HashInfo->HashSize;
    CopyMem (&HashAlgo, DigestBuffer, sizeof (TPMI_ALG_HASH));
    DigestBuffer += sizeof (TPMI_ALG_HASH);
  }

  // Setting Command Buffer Size
  ExtendCmdSize              = (UINT32)((UINTN)CommandBuffer - (UINTN)&ExtendCmd);
  ExtendCmd.Header.paramSize = SwapBytes32 (ExtendCmdSize);

  // Cursor to NextEvent
  NextEvent = DigestBuffer - sizeof (TPMI_ALG_HASH);
  CopyMem (&EventSize, NextEvent, sizeof (TcgPcrEvent2->EventSize));
  NextEvent += sizeof (TcgPcrEvent2->EventSize);
  NextEvent += EventSize;

  /*
   * All of event log from TF-A's EventType is EV_POST_CODE.
   */
  if (TcgPcrEvent2->EventType == EV_POST_CODE) {
    TpmLibExecuteCommand (
      ExtendCmdSize,
      (UINT8 *)&ExtendCmd,
      &ExtendResSize,
      (UINT8 **)&Response
      );

    /* TPM response's recevied with Big endian format */
    ResponseCode = SwapBytes32 (Response->Header.responseCode);
    if (ResponseCode != TPM_RC_SUCCESS) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to extend event... TPM_RC:0x%x\n", __func__, ResponseCode));
      return EFI_DEVICE_ERROR;
    }
  } else {
    DEBUG ((DEBUG_INFO, "%a: Skip to Extend Event (Type :%d)\n", __func__, TcgPcrEvent2->EventType));
  }

  *TotalEventSize -= (UINTN)(NextEvent - *EventAddr);
  *EventAddr       = NextEvent;

  return EFI_SUCCESS;
}

/**
  Initilize pseudo crb buffer for software based TPM.

  @param  [in]     Locality         Locality

  @return EFI_SUCCESS
  @return EFI_INVALID_PARAMETER     Invalid locality.

**/
STATIC
EFI_STATUS
EFIAPI
FtpmPseudoCrbInit (
  IN UINT8  Locality
  )
{
  PTP_CRB_REGISTERS             *CrbReg;
  UINT64                        BufferAddr;
  PTP_CRB_INTERFACE_IDENTIFIER  *InterfaceId;

  if (Locality >= NUM_LOCALITIES) {
    return EFI_INVALID_PARAMETER;
  }

  CrbReg = GetCrbBuffer (Locality, CRB_REGISTER);
  ZeroMem (CrbReg, sizeof (PTP_CRB_REGISTERS));
  BufferAddr  = (UINT64)CrbReg + OFFSET_OF (PTP_CRB_REGISTERS, CrbDataBuffer);
  InterfaceId = (PTP_CRB_INTERFACE_IDENTIFIER *)&CrbReg->InterfaceId;

  CrbReg->LocalityState = ((Locality << 2) | PTP_CRB_LOCALITY_STATE_TPM_REG_VALID_STATUS);
  if (Locality == 0) {
    /*
     * default locality is 0, always assigned unless disabled.
     * this is for SPM_MM using with ARM_SMC method.
     */
    CrbReg->LocalityState  |= PTP_CRB_LOCALITY_STATE_LOCALITY_ASSIGNED;
    CrbReg->LocalityStatus |= PTP_CRB_LOCALITY_STATUS_GRANTED;
  }

  InterfaceId->Bits.InterfaceType          = PTP_INTERFACE_IDENTIFIER_INTERFACE_TYPE_CRB;
  InterfaceId->Bits.InterfaceVersion       = PTP_INTERFACE_IDENTIFIER_INTERFACE_VERSION_CRB;
  InterfaceId->Bits.CapLocality            = PTP_CAP_LOCALITY_FIVE;             /**< Support localities 0-4 */
  InterfaceId->Bits.CapDataXferSizeSupport = PTP_CAP_DATA_XFER_SIZE_64_BYTES;   /**< supports 64-bytes transfer size */
  InterfaceId->Bits.CapCRB                 = PTP_CAP_CRB_INTEREFACE_SUPPORTED;
  InterfaceId->Bits.InterfaceSelector      = PTP_INTERFACE_IDENTIFIER_INTERFACE_TYPE_CRB;

  CrbReg->CrbControlCommandSize        = CRB_BUFFER_SIZE;
  CrbReg->CrbControlCommandAddressLow  = (BufferAddr & 0xffffffff);
  CrbReg->CrbControlCommandAddressHigh = (BufferAddr >> 32);

  CrbReg->CrbControlResponseSize   = CRB_BUFFER_SIZE;
  CrbReg->CrbControlResponseAddrss = BufferAddr;

  return EFI_SUCCESS;
}

/**
  Initialize software based TPM device.

  @return EFI_SUCCESS
  @return EFI_DEVICE_ERROR  Failed to initialize fTPM device.

**/
STATIC
EFI_STATUS
EFIAPI
FtpmDeviceStartup (
  IN VOID
  )
{
  INT32                  Ret;
  TPM2_STARTUP_COMMAND   StartupCmd;
  TPM2_STARTUP_RESPONSE  StartupRes;
  TPM2_STARTUP_RESPONSE  *Response;
  UINT32                 ResponseSize;
  TPM_RC                 ResponseCode;

  Ret = PlatformTpmLibNVEnable (NULL, 0);
  if (Ret != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to initialize Nv Stroage... ret: %d\n",
      __func__,
      Ret
      ));
    return EFI_DEVICE_ERROR;
  }

  if (PlatformTpmLibNVNeedsManufacture ()) {
    Ret = TpmLibTpmManufacture ();
    if (Ret < 0) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to manufacture Nv Storage... ret: %d\n",
        __func__,
        Ret
        ));
      return EFI_DEVICE_ERROR;
    }
  }

  TpmLibTpmInit ();

  ///
  /// All TPM request should be sent with Big endian format.
  ///
  StartupCmd.Header.tag         = SwapBytes16 (TPM_ST_NO_SESSIONS);
  StartupCmd.Header.paramSize   = SwapBytes32 (sizeof (TPM2_STARTUP_COMMAND));
  StartupCmd.Header.commandCode = SwapBytes32 (TPM_CC_Startup);
  StartupCmd.StartupType        = TPM_SU_CLEAR;

  Response     = &StartupRes;
  ResponseSize = sizeof (StartupRes);

  TpmLibExecuteCommand (
    sizeof (TPM2_STARTUP_COMMAND),
    (UINT8 *)&StartupCmd,
    &ResponseSize,
    (UINT8 **)&Response
    );

  ///
  /// TPM response's recevied with Big endian format.
  ///
  ResponseCode = SwapBytes32 (Response->Header.responseCode);
  if ((ResponseCode != TPM_RC_SUCCESS) && (ResponseCode != TPM_RC_INITIALIZE)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to call Startup Command... rc: 0x%x\n",
      __func__,
      ResponseCode
      ));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Notifies the TPM service that a TPM localirty request
  is ready to be processed, and allows the TPM service to process it.

  See the CRB over FF-A spec 6.3.
  all of return values based on the specification.

  @param [in]  Locality      Locality

**/
STATIC
VOID
EFIAPI
FtpmHandleLocalityRequest (
  IN UINT8  Locality
  )
{
  PTP_CRB_REGISTERS  *CrbReg;
  BOOLEAN            RestoreFormerLoc;
  INT8               Idx;

  CrbReg = GetCrbBuffer (Locality, CRB_REGISTER);

  if ((CrbReg->LocalityControl & PTP_CRB_LOCALITY_CONTROL_REQUEST_ACCESS) != 0x00) {
    CrbReg->LocalityState  |= PTP_CRB_LOCALITY_STATE_LOCALITY_ASSIGNED;
    CrbReg->LocalityStatus |= PTP_CRB_LOCALITY_STATUS_GRANTED;

    for (Idx = Locality - 1; Idx >= 0; Idx--) {
      CrbReg                  = GetCrbBuffer (Idx, CRB_REGISTER);
      CrbReg->LocalityStatus |= (PTP_CRB_LOCALITY_STATUS_BEEN_SEIZED);
    }

    TpmLibSetLocality (Locality);
  } else {
    RestoreFormerLoc        = FALSE;
    CrbReg->LocalityState  &= ~(PTP_CRB_LOCALITY_STATE_LOCALITY_ASSIGNED);
    CrbReg->LocalityStatus &= ~(PTP_CRB_LOCALITY_STATUS_GRANTED);

    for (Idx = Locality -1; Idx >= 0; Idx--) {
      CrbReg                  = GetCrbBuffer (Idx, CRB_REGISTER);
      CrbReg->LocalityStatus &= ~(PTP_CRB_LOCALITY_STATUS_BEEN_SEIZED);
      if ((!RestoreFormerLoc) &&
          (CrbReg->LocalityStatus & PTP_CRB_LOCALITY_STATUS_GRANTED) &&
          (CrbReg->LocalityState & PTP_CRB_LOCALITY_STATE_LOCALITY_ASSIGNED))
      {
        TpmLibSetLocality (Idx);
        RestoreFormerLoc = TRUE;
      }
    }

    if (!RestoreFormerLoc) {
      TpmLibSetLocality (0);
    }
  }

  CrbReg->LocalityControl = 0x00;
}

/**
  Notifies the TPM service that a TPM command request
  is ready to be processed, and allows the TPM service to process it.

  See the CRB over FF-A spec 6.3.
  all of return values based on the specification.

  @param [in]  Locality      Locality

  @retval EFI_SUCCESS          Success to execute command request
  @retval EFI_ACCESS_DENIDED   Locality isn't available
  @retval EFI_DEVICE_ERROR     Invalid CRB for locality.

**/
STATIC
EFI_STATUS
EFIAPI
FtpmHandleCommandRequest (
  IN UINT8  Locality
  )
{
  TPM2_COMMAND_HEADER   *Command;
  TPM_CC                CommandCode;
  UINT32                CommandSize;
  TPM2_RESPONSE_HEADER  *Response;
  TPM_RC                ResponseCode;
  UINT32                ResponseSize;
  PTP_CRB_REGISTERS     *CrbReg;

  CrbReg = GetCrbBuffer (Locality, CRB_REGISTER);

  if ((CrbReg->LocalityState &
       (PTP_CRB_LOCALITY_STATE_LOCALITY_ASSIGNED | PTP_CRB_LOCALITY_STATE_TPM_REG_VALID_STATUS)) !=
      (PTP_CRB_LOCALITY_STATE_LOCALITY_ASSIGNED | PTP_CRB_LOCALITY_STATE_TPM_REG_VALID_STATUS))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Locality(%d) isn't validated...\n",
      __func__,
      Locality
      ));
    return EFI_ACCESS_DENIED;
  }

  if (CrbReg->LocalityStatus != PTP_CRB_LOCALITY_STATUS_GRANTED) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Locality(%d) isn't granted...\n",
      __func__,
      Locality
      ));
    return EFI_ACCESS_DENIED;
  }

  /*
   * Ignore request via CrbControlRequest for fTPM
   */
  if (CrbReg->CrbControlRequest != 0x00) {
    CrbReg->CrbControlRequest = 0x00;
    return EFI_SUCCESS;
  }

  if (CrbReg->CrbControlStart != PTP_CRB_CONTROL_START) {
    DEBUG ((DEBUG_ERROR, "%a: CRB_CONTROL_START isn't set...\n", __func__));
    return EFI_DEVICE_ERROR;
  }

  Command     = GetCrbBuffer (Locality, CRB_COMMAND);
  CommandCode = SwapBytes32 (Command->commandCode);
  CommandSize = SwapBytes32 (Command->paramSize);

  ResponseSize = CRB_BUFFER_SIZE;
  CopyMem (mTmpCommandBuffer, Command, CommandSize);

  DEBUG ((
    DEBUG_INFO,
    "%a: TpmLibExecuteCommand: 0x%lx\n",
    __func__,
    CommandCode
    ));

  TpmLibExecuteCommand (
    CommandSize,
    mTmpCommandBuffer,
    &ResponseSize,
    (UINT8 **)&mTmpResponseBuffer
    );

  Response = GetCrbBuffer (Locality, CRB_RESPONSE);
  CopyMem (Response, mTmpResponseBuffer, ResponseSize);
  ResponseCode = SwapBytes32 (Response->responseCode);

  DEBUG ((
    DEBUG_INFO,
    "%a: Tpm ResponseCode: 0x%lx\n",
    __func__,
    ResponseCode
    ));

  if (ResponseCode != TPM_RC_SUCCESS) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to run CommandCode(0x%x)... RC: 0x%x \n",
      __func__,
      CommandCode,
      ResponseCode
      ));
  }

  CrbReg->CrbControlStart = 0x00;

  return EFI_SUCCESS;
}

/**
  Get tpm event log.

**/
STATIC
VOID
EFIAPI
FtpmGetTpmEventLog (
  IN VOID
  )
{
  VOID                  *HobList;
  EFI_HOB_GUID_TYPE     *GuidHob;
  EFI_MMRAM_DESCRIPTOR  *EventLogDesc;

  HobList = GetHobList ();
  if (HobList == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to find out gHobList.\n", __func__));
    return;
  }

  GuidHob = GetNextGuidHob (&gEdkiiTpmEventLogDescHobGuid, HobList);
  if (GuidHob == NULL) {
    DEBUG ((DEBUG_INFO, "%a: [SKIP] TPM event log doesn't present.\n", __func__));
    mEventLog     = NULL;
    mEventLogSize = 0;
    return;
  }

  EventLogDesc  = GET_GUID_HOB_DATA (GuidHob);
  mEventLog     = (VOID *)EventLogDesc->PhysicalStart;
  mEventLogSize = (UINTN)EventLogDesc->PhysicalSize;

  DumpTpmEventLog ();
}

/**
  Extend event logs.

  @retval EFI_SUCCESS
  @retval EFI_UNSUPPORTED                      Unsupported Eventlog.
  @retval EFI_INVALID_PARAMETER                Invalid arguments

**/
STATIC
EFI_STATUS
EFIAPI
FtpmExtendEventLogs (
  IN VOID
  )
{
  EFI_STATUS  Status;
  UINT8       *EventLog;
  UINTN       EventLogSize;

  EventLog     = (UINT8 *)mEventLog;
  EventLogSize = mEventLogSize;

  if ((mEventLog == NULL) || (mEventLogSize == 0)) {
    DEBUG ((DEBUG_INFO, "%a: [SKIP] Extend eventlog...\n", __func__));
    return EFI_SUCCESS;
  }

  Status = ValidateAndStripSpecIdEvent (&EventLog, &EventLogSize);
  if (EFI_ERROR (Status) || (EventLogSize == 0)) {
    return Status;
  }

  while (EventLogSize != 0) {
    Status = ExtendOneEventLog (&EventLog, &EventLogSize);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Issue TPM command or locality request to FF-A CRB device.

  @param[in]      Qualifier                 Qualifier.
  @param[in]      Locality                  Locality.

  @retval EFI_SUCCESS               The command byte stream was successfully sent to the device and a response was successfully received.
  @retval EFI_INVALID_PARAMETER     Invalid arguments.
  @retval EFI_DEVICE_ERROR          CRB control data or locality conrol data is not valid.
  @retval EFI_ACCESS_DENIED         locality requests or command processing at given locality is disabled.

**/
EFI_STATUS
EFIAPI
TpmCrbFfaDeviceStart (
  IN UINT8  Qualifier,
  IN UINT8  Locality
  )
{
  EFI_STATUS  Status;
  UINTN       CommandType;
  UINT8       CurLocality;

  CommandType = (Qualifier & TPM2_FFA_START_FUNC_COMMAND_TYPE_MASK);

  if (Locality >= NUM_LOCALITIES) {
    return EFI_INVALID_PARAMETER;
  }

  CurLocality = TpmLibGetLocality ();

  /**
   * Currently, don't use locality 4.
   */
  if ((Locality == 4) || (Locality > CurLocality)) {
    return EFI_ACCESS_DENIED;
  }

  if (CommandType == TPM2_FFA_START_FUNC_QUALIFIER_LOCALITY) {
    FtpmHandleLocalityRequest (Locality);
    Status = EFI_SUCCESS;
  } else {
    if (CurLocality != Locality) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Locality unmatched... cur:%d, req:%d\n",
        __func__,
        CurLocality,
        Locality
        ));
      return EFI_ACCESS_DENIED;
    }

    Status = FtpmHandleCommandRequest (Locality);
  }

  return Status;
}

/**
  Get CRB information.

  @param[in]      Locality                     Locality.
  @param[out]     BaseAddress                  CRB base address.
  @param[out]     Size                         CRB region size.

  @retval EFI_SUCCESS
  @retval EFI_INVALID_PARAMETER                Invalid arguments

**/
EFI_STATUS
EFIAPI
TpmCrbFfaDeviceGetCrbInfo (
  IN UINT8                  Locality,
  OUT EFI_PHYSICAL_ADDRESS  *BaseAddress,
  OUT UINTN                 *Size
  )
{
  PTP_CRB_REGISTERS  *CrbReg;

  if ((Locality >= NUM_LOCALITIES) || (BaseAddress == NULL) || (Size == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  CrbReg = GetCrbBuffer (Locality, CRB_REGISTER);
  if (CrbReg == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *BaseAddress = (EFI_PHYSICAL_ADDRESS)CrbReg;
  *Size        = EFI_PAGE_SIZE;

  return EFI_SUCCESS;
}

/**
  Check whether FF-A CRB device is available

  @retval TRUE    Available.
  @retval FALSE   Unavailable.

**/
BOOLEAN
EFIAPI
TpmCrbFfaDeviceIsAvailable (
  VOID
  )
{
  return mFtpmAvailable;
}

/**
  The constructor of TpmCrbFfaFtpmLib.

  @param  [in] ImageHandle    The image handle of the Standalone MM Driver.
  @param  [in] MmSystemTable  A pointer to the MM System Table.

  @return EFI_SUCCESS
  @return Others              Error.
**/
EFI_STATUS
EFIAPI
TpmCrbFfaFtpmLibConstuctor (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  )
{
  EFI_STATUS  Status;
  UINT8       Locality;

  mTmpCommandBuffer = AllocateRuntimePool (CRB_BUFFER_SIZE * 2);
  if (mTmpCommandBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate mTmpCommandBuffer...\n", __func__));
    goto ErrorHandler;
  }

  mTmpResponseBuffer = ((VOID *)((UINTN)mTmpCommandBuffer) + CRB_BUFFER_SIZE);

  for (Locality = 0; Locality < NUM_LOCALITIES; Locality++) {
    Status = FtpmPseudoCrbInit (Locality);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to init PseudoCrb for Locality(%d)... Status: %r\n",
        __func__,
        Locality,
        Status
        ));
      goto ErrorHandler;
    }
  }

  Status = FtpmDeviceStartup ();
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to Ftpm Device Startup... Status: %r\n",
      __func__,
      Status
      ));
    goto ErrorHandler;
  }

  FtpmGetTpmEventLog ();

  Status = FtpmExtendEventLogs ();
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to Ftpm Device Extend Tpm event log from TF-A... Status: %r\n",
      __func__,
      Status
      ));
    goto ErrorHandler;
  }

  mFtpmAvailable = TRUE;

  return EFI_SUCCESS;

ErrorHandler:
  if (mTmpCommandBuffer != NULL) {
    FreePool (mTmpCommandBuffer);
    mTmpResponseBuffer = NULL;
    mTmpCommandBuffer  = NULL;
  }

  return Status;
}
