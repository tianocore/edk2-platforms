/** @file MockBiosIdLib.h
  Google Test mocks for BiosIdLib
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_BIOS_ID_LIB_H_
#define MOCK_BIOS_ID_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Pi/PiBootMode.h>
  #include <Library/BiosIdLib.h>
}

struct MockBiosIdLib {
  MOCK_INTERFACE_DECLARATION (MockBiosIdLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetBiosId,
    (
     OUT BIOS_ID_IMAGE     *BiosIdImage OPTIONAL
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetBiosVersionDateTime,
    (
     OUT CHAR16    *BiosVersion, OPTIONAL
     OUT CHAR16    *BiosReleaseDate, OPTIONAL
     OUT CHAR16    *BiosReleaseTime OPTIONAL
    )
    );
};

#endif
