/** @file
  The Morello SoC I2C HDMI bus

  Copyright (c) 2022, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MORELLO_I2CBUSHDMI_H_
#define MORELLO_I2CBUSHDMI_H_

#include <Protocol/I2cEnumerate.h>
#include <Protocol/I2cBusConfigurationManagement.h>

typedef struct _I2C_BUS_HDMI I2C_BUS_HDMI;

/**
  Allocate and install UEFI driver bindings for the controller and devices for
  the Morello SoC HDMI I2C bus.

  @param[out] I2cBusHdmi  Updated with a pointer to the allocated I2C_BUS_HDMI
                          on success.

  @retval EFI_SUCCESS  Bus successfully allocated and installed with I2cBusHdmi
                       updated and valid.
  @retval *            Other errors are possible.

**/
EFI_STATUS
I2cBusHdmiStart (
  OUT I2C_BUS_HDMI  **I2cBusHdmi
  );

/**
  Free and uninstall UEFI driver bindings for the controller and devices for
  the Morello SoC HDMI I2C bus.

  @param[in,out] I2cBusHdmi  Updated with a pointer to NULL on success.

  @retval EFI_SUCCESS  Bus successfully deallocated and uninstalled with
                       I2cBusHdmi updated and valid.
  @retval EFI_SUCCESS  *I2cBusHdmi is NULL already.
  @retval *            Other errors are possible.

**/
EFI_STATUS
I2cBusHdmiStop (
  IN OUT I2C_BUS_HDMI  **I2cBusHdmi
  );

#endif // MORELLO_I2CBUSHDMI_H_
