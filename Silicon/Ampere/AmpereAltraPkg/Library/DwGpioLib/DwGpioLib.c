/** @file

  Copyright (c) 2020 - 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/GpioLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Platform/Ac01.h>

/* Runtime needs to be 64K alignment */
#define RUNTIME_ADDRESS_MASK    (~(SIZE_64KB - 1))
#define RUNTIME_ADDRESS_LENGTH  SIZE_64KB

#define GPIO_MUX_VAL(Gpio)  (0x00000001 << (Gpio))
#define GPIO_IN   0
#define GPIO_OUT  1

/* Address GPIO_REG Registers */
#define GPIO_SWPORTA_DR_ADDR   0x00000000
#define GPIO_SWPORTA_DDR_ADDR  0x00000004
#define GPIO_EXT_PORTA_ADDR    0x00000050

STATIC UINT64     GpioBaseAddr[]                                                           = { AC01_GPIO_BASE_ADDRESS_LIST };
STATIC UINT64     GpiBaseAddr[]                                                            = { AC01_GPI_BASE_ADDRESS_LIST };
STATIC BOOLEAN    GpioRuntimeEnableArray[sizeof (GpioBaseAddr) / sizeof (GpioBaseAddr[0])] = { FALSE };
STATIC EFI_EVENT  mVirtualAddressChangeEvent                                               = NULL;

UINT64
GetBaseAddr (
  IN UINT32  Pin
  )
{
  UINT32  NumberOfControllers = sizeof (GpioBaseAddr) / sizeof (GpioBaseAddr[0]);
  UINT32  TotalPins           = AC01_GPIO_PINS_PER_CONTROLLER * NumberOfControllers;

  if ((NumberOfControllers == 0) || (Pin >= TotalPins)) {
    return 0;
  }

  return GpioBaseAddr[Pin / AC01_GPIO_PINS_PER_CONTROLLER];
}

VOID
GpioWrite (
  IN UINT64  Base,
  IN UINT32  Val
  )
{
  MmioWrite32 ((UINTN)Base, Val);
}

VOID
GpioRead (
  IN  UINT64  Base,
  OUT UINT32  *Val
  )
{
  ASSERT (Val != NULL);
  *Val = MmioRead32 (Base);
}

VOID
EFIAPI
GpioWriteBit (
  IN UINT32  Pin,
  IN UINT32  Val
  )
{
  UINT64  Reg;
  UINT32  GpioPin;
  UINT32  ReadVal;

  Reg = GetBaseAddr (Pin);
  if (Reg == 0) {
    return;
  }

  GpioPin = Pin % AC01_GPIO_PINS_PER_CONTROLLER;

  Reg += GPIO_SWPORTA_DR_ADDR;
  GpioRead (Reg, &ReadVal);

  if (Val != 0) {
    GpioWrite (Reg, ReadVal | GPIO_MUX_VAL (GpioPin));
  } else {
    GpioWrite (Reg, ReadVal & ~GPIO_MUX_VAL (GpioPin));
  }
}

UINTN
EFIAPI
GpioReadBit (
  IN UINT32  Pin
  )
{
  UINT64  Reg;
  UINT32  Val;
  UINT32  GpioPin;
  UINT8   Index;
  UINT32  MaxIndex;

  Reg = GetBaseAddr (Pin);
  if (Reg == 0) {
    return 0;
  }

  GpioPin = Pin % AC01_GPIO_PINS_PER_CONTROLLER;

  /* Check if a base address is GPI */
  MaxIndex = sizeof (GpiBaseAddr) / sizeof (GpiBaseAddr[0]);
  for (Index = 0; Index < MaxIndex; Index++) {
    if (Reg == GpiBaseAddr[Index]) {
      break;
    }
  }

  if (Index == MaxIndex) {
    /* Only GPIO has GPIO_EXT_PORTA register, not for GPI */
    Reg +=  GPIO_EXT_PORTA_ADDR;
  }

  GpioRead (Reg, &Val);

  return Val & GPIO_MUX_VAL (GpioPin) ? 1 : 0;
}

EFI_STATUS
GpioConfig (
  IN UINT32  Pin,
  IN UINT32  InOut
  )
{
  INTN    GpioPin;
  UINT32  Val;
  UINT64  Reg;

  /*
   * Caculate GPIO Pin Number for Direction Register
   * GPIO_SWPORTA_DDR for GPIO[31...0]
   * GPIO_SWPORTB_DDR for GPIO[51...32]
   */

  Reg = GetBaseAddr (Pin);
  if (Reg == 0) {
    return EFI_UNSUPPORTED;
  }

  Reg    += GPIO_SWPORTA_DDR_ADDR;
  GpioPin = Pin % AC01_GPIO_PINS_PER_CONTROLLER;
  GpioRead (Reg, &Val);

  if (InOut == GPIO_OUT) {
    Val |= GPIO_MUX_VAL (GpioPin);
  } else {
    Val &= ~GPIO_MUX_VAL (GpioPin);
  }

  GpioWrite (Reg, Val);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
GpioModeConfig (
  UINT8             Pin,
  GPIO_CONFIG_MODE  Mode
  )
{
  UINT32  NumberOfControllers = sizeof (GpioBaseAddr) / sizeof (UINT64);
  UINT32  NumersOfPins        = NumberOfControllers * AC01_GPIO_PINS_PER_CONTROLLER;
  UINT32  Delay               = 10;

  if (  (Mode < GpioConfigOutLow)
     || (Mode >= MaxGpioConfigMode)
     || (Pin > NumersOfPins - 1)
     || (Pin < 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  switch (Mode) {
    case GpioConfigOutLow:
      GpioConfig (Pin, GPIO_OUT);
      GpioWriteBit (Pin, 0);
      DEBUG ((DEBUG_INFO, "GPIO pin %d configured as output low\n", Pin));
      break;

    case GpioConfigOutHigh:
      GpioConfig (Pin, GPIO_OUT);
      GpioWriteBit (Pin, 1);
      DEBUG ((DEBUG_INFO, "GPIO pin %d configured as output high\n", Pin));
      break;

    case GpioConfigOutLowToHigh:
      GpioConfig (Pin, GPIO_OUT);
      GpioWriteBit (Pin, 0);
      MicroSecondDelay (1000 * Delay);
      GpioWriteBit (Pin, 1);
      DEBUG ((DEBUG_INFO, "GPIO pin %d configured as output low->high\n", Pin));
      break;

    case GpioConfigOutHightToLow:
      GpioConfig (Pin, GPIO_OUT);
      GpioWriteBit (Pin, 1);
      MicroSecondDelay (1000 * Delay);
      GpioWriteBit (Pin, 0);
      DEBUG ((DEBUG_INFO, "GPIO pin %d configured as output high->low\n", Pin));
      break;

    case GpioConfigIn:
      GpioConfig (Pin, GPIO_IN);
      DEBUG ((DEBUG_INFO, "GPIO pin %d configured as input\n", Pin));
      break;

    default:
      break;
  }

  return EFI_SUCCESS;
}

/**
 * Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.
 *
 * This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.
 * It convers pointer to new virtual address.
 *
 * @param  Event        Event whose notification function is being invoked.
 * @param  Context      Pointer to the notification function's context.
 */
VOID
EFIAPI
GpioVirtualAddressChangeEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINTN  Count;

  EfiConvertPointer (0x0, (VOID **)&GpioBaseAddr);
  for (Count = 0; Count < sizeof (GpioBaseAddr) / sizeof (GpioBaseAddr[0]); Count++) {
    if (!GpioRuntimeEnableArray[Count]) {
      continue;
    }

    EfiConvertPointer (0x0, (VOID **)&GpioBaseAddr[Count]);
  }
}

/**
 Setup a controller that to be used in runtime service.

 @Bus:      Bus ID.
 @return:   0 for success.
            Otherwise, error code.
 **/
EFI_STATUS
EFIAPI
GpioSetupRuntime (
  IN UINT32  Pin
  )
{
  EFI_STATUS                       Status;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  Descriptor;

  if (GetBaseAddr (Pin) == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (mVirtualAddressChangeEvent == NULL) {
    /*
    * Register for the virtual address change event
    */
    Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    GpioVirtualAddressChangeEvent,
                    NULL,
                    &gEfiEventVirtualAddressChangeGuid,
                    &mVirtualAddressChangeEvent
                    );
    ASSERT_EFI_ERROR (Status);
  }

  Status = gDS->GetMemorySpaceDescriptor (
                  GetBaseAddr (Pin) & RUNTIME_ADDRESS_MASK,
                  &Descriptor
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gDS->SetMemorySpaceAttributes (
                  GetBaseAddr (Pin) & RUNTIME_ADDRESS_MASK,
                  RUNTIME_ADDRESS_LENGTH,
                  Descriptor.Attributes | EFI_MEMORY_RUNTIME
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  GpioRuntimeEnableArray[Pin / AC01_GPIO_PINS_PER_CONTROLLER] = TRUE;

  return Status;
}
