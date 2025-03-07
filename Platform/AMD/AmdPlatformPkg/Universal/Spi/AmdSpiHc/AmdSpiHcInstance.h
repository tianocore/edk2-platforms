/** @file

  FV block I/O protocol driver for SPI flash libary.

  Copyright (C) 2023-2025 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef AMD_SPI_HC_INSTANCE_H_
#define AMD_SPI_HC_INSTANCE_H_

#include <Library/AmdPspRomArmorLib.h>
#include <Library/DevicePathLib.h>
#include <Uefi/UefiBaseType.h>
#include <Protocol/SpiHc.h>
#include <Protocol/SpiConfiguration.h>
#include "AmdSpiHc.h"

#define SPI_HOST_CONTROLLER_SIGNATURE SIGNATURE_32 ('s', 'h', 'c', 'd')

typedef struct {
  UINTN                           Signature;
  EFI_HANDLE                      Handle;
  EFI_EVENT                       Event;
  BOOLEAN                         PspMailboxSpiMode;
  SPI_COMMUNICATION_BUFFER        SpiCommunicationBuffer;
  VOID                            *Registration;
  EFI_PHYSICAL_ADDRESS            HcAddress;
  EFI_SPI_HC_PROTOCOL             Protocol;
} SPI_HOST_CONTROLLER_INSTANCE;

#define SPI_HOST_CONTROLLER_FROM_THIS(a) \
  CR (a, SPI_HOST_CONTROLLER_INSTANCE, Protocol, \
      SPI_HOST_CONTROLLER_SIGNATURE)

#define SPI_HOST_CONTROLLER_FROM_CONFIG_ACCESS(a) \
  CR (a, SPI_HOST_CONTROLLER_INSTANCE, ConfigAccess, \
      SPI_HOST_CONTROLLER_SIGNATURE)

extern UINT8  AmdSpiHcFormBin[];
extern UINT8  AmdSpiHcProtocolDxeStrings[];

#endif // AMD_SPI_HC_SMM_PROTOCOL_H_
