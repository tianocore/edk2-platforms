/** @file MockBiosIdLib.cpp
  Google Test mocks for BiosIdLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockBiosIdLib.h>

MOCK_INTERFACE_DEFINITION (MockBiosIdLib);
MOCK_FUNCTION_DEFINITION (MockBiosIdLib, GetBiosId, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockBiosIdLib, GetBiosVersionDateTime, 3, EFIAPI);
