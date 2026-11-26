/** @file
  SMBIOS Type 8 (Port Connector Information) tables for the Marvell CN10K SoC.

  NOTE: These tables are valid only for the ebb106s evaluation board and are
  installed only when that board is detected. Treat them as a reference
  implementation for other boards.

  Copyright (c) 2015, Cavium Inc.
  Copyright (c) 2012, Apple Inc. All rights reserved.
  Copyright (c) 2026, Marvell. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Protocol/Smbios.h>

#include "SmbiosPlatformDxe.h"

STATIC SMBIOS_TABLE_TYPE8  mSmbiosType8SerialTemplate = {
  { EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION, sizeof (SMBIOS_TABLE_TYPE8), 0 },
  1,                              // InternalReferenceDesignator String
  PortConnectorTypeNone,          // InternalConnectorType; Value from MISC_PORT_CONNECTOR_TYPE.
  2,                              // ExternalReferenceDesignator String
  PortConnectorTypeDB9Male,       // ExternalConnectorType; Value from MISC_PORT_CONNECTOR_TYPE.
  PortTypeSerial16550ACompatible, // PortType; Value from MISC_PORT_TYPE.
};

STATIC SMBIOS_TABLE_TYPE8  mSmbiosType8UsbTemplate = {
  { EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION, sizeof (SMBIOS_TABLE_TYPE8), 0 },
  1,                     // InternalReferenceDesignator String
  PortConnectorTypeNone, // InternalConnectorType; Value from MISC_PORT_CONNECTOR_TYPE.
  2,                     // ExternalReferenceDesignator String
  PortConnectorTypeUsb,  // ExternalConnectorType; Value from MISC_PORT_CONNECTOR_TYPE.
  PortTypeUsb,           // PortType; Value from MISC_PORT_TYPE.
};

STATIC SMBIOS_TABLE_TYPE8  mSmbiosType8Rj45Template = {
  { EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION, sizeof (SMBIOS_TABLE_TYPE8), 0 },
  1,                     // InternalReferenceDesignator String
  PortConnectorTypeNone, // InternalConnectorType; Value from MISC_PORT_CONNECTOR_TYPE.
  2,                     // ExternalReferenceDesignator String
  PortConnectorTypeRJ45, // ExternalConnectorType; Value from MISC_PORT_CONNECTOR_TYPE.
  PortTypeNetworkPort,   // PortType; Value from MISC_PORT_TYPE.
};

STATIC SMBIOS_TABLE_TYPE8  mSmbiosType8NetworkTemplate = {
  { EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION, sizeof (SMBIOS_TABLE_TYPE8), 0 },
  1,                      // InternalReferenceDesignator String
  PortConnectorTypeNone,  // InternalConnectorType; Value from MISC_PORT_CONNECTOR_TYPE.
  2,                      // ExternalReferenceDesignator String
  PortConnectorTypeOther, // ExternalConnectorType; Value from MISC_PORT_CONNECTOR_TYPE.
  PortTypeNetworkPort,    // PortType; Value from MISC_PORT_TYPE.
};

STATIC CHAR8  *mSmbiosType8Strings1[]  = { "J79", "CPU_UART 0", NULL };
STATIC CHAR8  *mSmbiosType8Strings2[]  = { "J79", "CPU_UART 1", NULL };
STATIC CHAR8  *mSmbiosType8Strings3[]  = { "J92", "MCU_UART", NULL };
STATIC CHAR8  *mSmbiosType8Strings4[]  = { "J66", "USB3.1 Port 0", NULL };
STATIC CHAR8  *mSmbiosType8Strings5[]  = { "J66", "USB3.1 Port 1", NULL };
STATIC CHAR8  *mSmbiosType8Strings6[]  = { "J91", "MCU_USB 2.0 Mini-B", NULL };
STATIC CHAR8  *mSmbiosType8Strings7[]  = { "J9", "MCU_LAN", NULL };
STATIC CHAR8  *mSmbiosType8Strings8[]  = { "J5", "SFP+", NULL };
STATIC CHAR8  *mSmbiosType8Strings9[]  = { "J2", "ZQSFP+", NULL };
STATIC CHAR8  *mSmbiosType8Strings10[] = { "J1", "ZQSFP+", NULL };
STATIC CHAR8  *mSmbiosType8Strings11[] = { "J7", "ZQSFP+", NULL };
STATIC CHAR8  *mSmbiosType8Strings12[] = { "J8", "ZQSFP+", NULL };

typedef struct {
  SMBIOS_TABLE_TYPE8    *Template;
  CHAR8                 **Strings;
} TYPE8_ENTRY;

STATIC TYPE8_ENTRY  mSmbiosType8Entries[] = {
  { &mSmbiosType8SerialTemplate,  mSmbiosType8Strings1  },
  { &mSmbiosType8SerialTemplate,  mSmbiosType8Strings2  },
  { &mSmbiosType8SerialTemplate,  mSmbiosType8Strings3  },
  { &mSmbiosType8UsbTemplate,     mSmbiosType8Strings4  },
  { &mSmbiosType8UsbTemplate,     mSmbiosType8Strings5  },
  { &mSmbiosType8UsbTemplate,     mSmbiosType8Strings6  },
  { &mSmbiosType8Rj45Template,    mSmbiosType8Strings7  },
  { &mSmbiosType8NetworkTemplate, mSmbiosType8Strings8  },
  { &mSmbiosType8NetworkTemplate, mSmbiosType8Strings9  },
  { &mSmbiosType8NetworkTemplate, mSmbiosType8Strings10 },
  { &mSmbiosType8NetworkTemplate, mSmbiosType8Strings11 },
  { &mSmbiosType8NetworkTemplate, mSmbiosType8Strings12 },
};

/**
  Install the SMBIOS Type 8 (Port Connector Information) records.

  Adds one record for each entry in the ebb106s port table: two CPU UARTs, the
  MCU UART, three USB ports, the MCU LAN port, and the SFP/ZQSFP+ cages.

  @param[in]      Smbios   SMBIOS protocol instance.
  @param[in,out]  Context  Driver context. Unused.

  @retval  EFI_SUCCESS  All port records were installed.
  @retval  Others       A record could not be added.
**/
EFI_STATUS
InstallType8PortConnectorInformation (
  IN     EFI_SMBIOS_PROTOCOL   *Smbios,
  IN OUT CN10K_SMBIOS_CONTEXT  *Context
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  for (Index = 0; Index < ARRAY_SIZE (mSmbiosType8Entries); Index++) {
    Status = SmbiosPlatformAddEntry (
               Smbios,
               (SMBIOS_STRUCTURE *)mSmbiosType8Entries[Index].Template,
               mSmbiosType8Entries[Index].Strings,
               NULL
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}
