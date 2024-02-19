/** @file
  Source code file for Intel VTd PEI DXE library.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/IoLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/IntelVTdPeiDxeLib.h>
#include <IndustryStandard/Vtd.h>

//
// Define the maximum message length that this library supports
//
#define MAX_STRING_LENGTH (0x100)

#define VTD_64BITS_ADDRESS(Lo, Hi) (LShiftU64 (Lo, 12) | LShiftU64 (Hi, 32))

/**
  Produces a Null-terminated ASCII string in an output buffer based on a Null-terminated
  ASCII format string and  variable argument list.
  
  @param[in]     Context           Event context
  @param[in out] CallbackHandle    Callback handler
  @param[in]     ErrorLevel        The error level of the debug message.
  @param[in]     FormatString      A Null-terminated ASCII format string.
  @param[in]     ...               Variable argument list whose contents are accessed based on the format string specified by FormatString.

  @return The number of ASCII characters in the produced output buffer not including the
          Null-terminator.
**/
UINTN
EFIAPI
VtdLogEventCallback (
  IN       VOID                    *Context,
  IN OUT   EDKII_VTD_LIB_STRING_CB CallbackHandle,
  IN       UINTN                   ErrorLevel,
  IN CONST CHAR8                   *FormatString,
  ...
  )
{
  CHAR8   Buffer[MAX_STRING_LENGTH];
  VA_LIST Marker;
  UINTN   NumberOfPrinted;

  if ((CallbackHandle == NULL) || (FormatString == NULL)) {
    return 0;
  }

  VA_START (Marker, FormatString);
  NumberOfPrinted = AsciiVSPrint (Buffer, sizeof (Buffer), FormatString, Marker);
  VA_END (Marker);

  if (NumberOfPrinted > 0) {
    CallbackHandle (Context, ErrorLevel, Buffer);
  }

  return NumberOfPrinted;
}

/**
  Dump DMAR DeviceScopeEntry.

  @param[in]      Context               Event context
  @param[in out]  CallbackHandle        Callback handler
  @param[in]      DmarDeviceScopeEntry  DMAR DeviceScopeEntry
**/
VOID
VtdLibDumpDmarDeviceScopeEntry (
  IN     VOID                                         *Context,
  IN OUT EDKII_VTD_LIB_STRING_CB                      CallbackHandle,
  IN     EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER  *DmarDeviceScopeEntry
  )
{
  UINTN                   PciPathNumber;
  UINTN                   PciPathIndex;
  EFI_ACPI_DMAR_PCI_PATH  *PciPath;

  if (DmarDeviceScopeEntry == NULL) {
    return;
  }

  VTDLIB_DEBUG ((DEBUG_INFO,
    "    *************************************************************************\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO,
    "    *       DMA-Remapping Device Scope Entry Structure                      *\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "    *************************************************************************\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO,
    (sizeof(UINTN) == sizeof(UINT64)) ?
    "    DMAR Device Scope Entry address ...................... 0x%016lx\n" :
    "    DMAR Device Scope Entry address ...................... 0x%08x\n",
    DmarDeviceScopeEntry
    ));
  VTDLIB_DEBUG ((DEBUG_INFO,
    "      Device Scope Entry Type ............................ 0x%02x\n",
    DmarDeviceScopeEntry->Type
    ));
  switch (DmarDeviceScopeEntry->Type) {
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT:
    VTDLIB_DEBUG ((DEBUG_INFO,
      "        PCI Endpoint Device\n"
      ));
    break;
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE:
    VTDLIB_DEBUG ((DEBUG_INFO,
      "        PCI Sub-hierachy\n"
      ));
    break;
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_IOAPIC:
    VTDLIB_DEBUG ((DEBUG_INFO,
      "        IOAPIC\n"
      ));
    break;
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_MSI_CAPABLE_HPET:
    VTDLIB_DEBUG ((DEBUG_INFO,
      "        MSI Capable HPET\n"
      ));
    break;
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_ACPI_NAMESPACE_DEVICE:
    VTDLIB_DEBUG ((DEBUG_INFO,
      "        ACPI Namespace Device\n"
      ));
    break;
  default:
    break;
  }
  VTDLIB_DEBUG ((DEBUG_INFO,
    "      Length ............................................. 0x%02x\n",
    DmarDeviceScopeEntry->Length
    ));
  VTDLIB_DEBUG ((DEBUG_INFO,
    "      Flags .............................................. 0x%02x\n",
    DmarDeviceScopeEntry->Flags
    ));
  VTDLIB_DEBUG ((DEBUG_INFO,
    "      Enumeration ID ..................................... 0x%02x\n",
    DmarDeviceScopeEntry->EnumerationId
    ));
  VTDLIB_DEBUG ((DEBUG_INFO,
    "      Starting Bus Number ................................ 0x%02x\n",
    DmarDeviceScopeEntry->StartBusNumber
    ));

  PciPathNumber = (DmarDeviceScopeEntry->Length - sizeof(EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER)) / sizeof(EFI_ACPI_DMAR_PCI_PATH);
  PciPath = (EFI_ACPI_DMAR_PCI_PATH *)(DmarDeviceScopeEntry + 1);
  for (PciPathIndex = 0; PciPathIndex < PciPathNumber; PciPathIndex++) {
    VTDLIB_DEBUG ((DEBUG_INFO,
      "      Device ............................................. 0x%02x\n",
      PciPath[PciPathIndex].Device
      ));
    VTDLIB_DEBUG ((DEBUG_INFO,
      "      Function ........................................... 0x%02x\n",
      PciPath[PciPathIndex].Function
      ));
  }

  VTDLIB_DEBUG ((DEBUG_INFO,
    "    *************************************************************************\n\n"
    ));
}

/**
  Dump DMAR SIDP table.

  @param[in]      Context           Event context
  @param[in out]  CallbackHandle    Callback handler
  @param[in]      Sidp              DMAR SIDP table
**/
VOID
VtdLibDumpDmarSidp (
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LIB_STRING_CB    CallbackHandle,
  IN EFI_ACPI_DMAR_SIDP_HEADER      *Sidp
  )
{
  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER   *DmarDeviceScopeEntry;
  INTN                                          SidpLen;

  if (Sidp == NULL) {
    return;
  }

  VTDLIB_DEBUG ((DEBUG_INFO,
    "  ***************************************************************************\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO,
    "  *       SoC Integrated Device Property Reporting Structure                *\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO,
    "  ***************************************************************************\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO,
    (sizeof(UINTN) == sizeof(UINT64)) ?
    "  SIDP address ........................................... 0x%016lx\n" :
    "  SIDP address ........................................... 0x%08x\n",
    Sidp
    ));
  VTDLIB_DEBUG ((DEBUG_INFO,
    "    Type ................................................. 0x%04x\n",
    Sidp->Header.Type
    ));
  VTDLIB_DEBUG ((DEBUG_INFO,
    "    Length ............................................... 0x%04x\n",
    Sidp->Header.Length
    ));
  VTDLIB_DEBUG ((DEBUG_INFO,
    "    Segment Number ....................................... 0x%04x\n",
    Sidp->SegmentNumber
    ));

  SidpLen  = Sidp->Header.Length - sizeof(EFI_ACPI_DMAR_SIDP_HEADER);
  DmarDeviceScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)(Sidp + 1);
  while (SidpLen > 0) {
    VtdLibDumpDmarDeviceScopeEntry (Context, CallbackHandle, DmarDeviceScopeEntry);
    SidpLen -= DmarDeviceScopeEntry->Length;
    DmarDeviceScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)DmarDeviceScopeEntry + DmarDeviceScopeEntry->Length);
  }

  VTDLIB_DEBUG ((DEBUG_INFO,
    "  ***************************************************************************\n\n"
    ));
}

/**
  Dump DMAR SATC table.

  @param[in]      Context           Event context
  @param[in out]  CallbackHandle    Callback handler
  @param[in]      Satc              DMAR SATC table
**/
VOID
VtdLibDumpDmarSatc (
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LIB_STRING_CB    CallbackHandle,
  IN     EFI_ACPI_DMAR_SATC_HEADER  *Satc
  )
{
  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER   *DmarDeviceScopeEntry;
  INTN                                          SatcLen;

  if (Satc == NULL) {
    return;
  }

  VTDLIB_DEBUG ((DEBUG_INFO, 
    "  ***************************************************************************\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "  *       ACPI Soc Integrated Address Translation Cache reporting Structure *\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "  ***************************************************************************\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    (sizeof(UINTN) == sizeof(UINT64)) ?
    "  SATC address ........................................... 0x%016lx\n" :
    "  SATC address ........................................... 0x%08x\n",
    Satc
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "    Type ................................................. 0x%04x\n",
    Satc->Header.Type
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "    Length ............................................... 0x%04x\n",
    Satc->Header.Length
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "    Flags ................................................ 0x%02x\n",
    Satc->Flags
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "    Segment Number ....................................... 0x%04x\n",
    Satc->SegmentNumber
    ));

  SatcLen  = Satc->Header.Length - sizeof(EFI_ACPI_DMAR_SATC_HEADER);
  DmarDeviceScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)(Satc + 1);
  while (SatcLen > 0) {
    VtdLibDumpDmarDeviceScopeEntry (Context, CallbackHandle, DmarDeviceScopeEntry);
    SatcLen -= DmarDeviceScopeEntry->Length;
    DmarDeviceScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)DmarDeviceScopeEntry + DmarDeviceScopeEntry->Length);
  }

  VTDLIB_DEBUG ((DEBUG_INFO, 
    "  ***************************************************************************\n\n"
    ));
}

/**
  Dump DMAR ANDD table.

  @param[in]      Context           Event context
  @param[in out]  CallbackHandle    Callback handler
  @param[in]      Andd              DMAR ANDD table
**/
VOID
VtdLibDumpDmarAndd (
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LIB_STRING_CB    CallbackHandle,
  IN     EFI_ACPI_DMAR_ANDD_HEADER  *Andd
  )
{
  if (Andd == NULL) {
    return;
  }

  VTDLIB_DEBUG ((DEBUG_INFO, 
    "  ***************************************************************************\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "  *       ACPI Name-space Device Declaration Structure                      *\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "  ***************************************************************************\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    (sizeof(UINTN) == sizeof(UINT64)) ?
    "  ANDD address ........................................... 0x%016lx\n" :
    "  ANDD address ........................................... 0x%08x\n",
    Andd
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "    Type ................................................. 0x%04x\n",
    Andd->Header.Type
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "    Length ............................................... 0x%04x\n",
    Andd->Header.Length
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "    ACPI Device Number ................................... 0x%02x\n",
    Andd->AcpiDeviceNumber
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "    ACPI Object Name ..................................... '%a'\n",
    (Andd + 1)
    ));

  VTDLIB_DEBUG ((DEBUG_INFO, 
    "  ***************************************************************************\n\n"
    ));
}

/**
  Dump DMAR RHSA table.

  @param[in]      Context           Event context
  @param[in out]  CallbackHandle    Callback handler
  @param[in]      Rhsa              DMAR RHSA table
**/
VOID
VtdLibDumpDmarRhsa (
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LIB_STRING_CB    CallbackHandle,
  IN     EFI_ACPI_DMAR_RHSA_HEADER  *Rhsa
  )
{
  if (Rhsa == NULL) {
    return;
  }

  VTDLIB_DEBUG ((DEBUG_INFO, 
    "  ***************************************************************************\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "  *       Remapping Hardware Status Affinity Structure                      *\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "  ***************************************************************************\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    (sizeof(UINTN) == sizeof(UINT64)) ?
    "  RHSA address ........................................... 0x%016lx\n" :
    "  RHSA address ........................................... 0x%08x\n",
    Rhsa
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "    Type ................................................. 0x%04x\n",
    Rhsa->Header.Type
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "    Length ............................................... 0x%04x\n",
    Rhsa->Header.Length
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "    Register Base Address ................................ 0x%016lx\n",
    Rhsa->RegisterBaseAddress
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "    Proximity Domain ..................................... 0x%08x\n",
    Rhsa->ProximityDomain
    ));

  VTDLIB_DEBUG ((DEBUG_INFO, 
    "  ***************************************************************************\n\n"
    ));
}

/**
  Dump DMAR ATSR table.

  @param[in]      Context           Event context
  @param[in out]  CallbackHandle    Callback handler
  @param[in]      Atsr              DMAR ATSR table
**/
VOID
VtdLibDumpDmarAtsr (
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LIB_STRING_CB    CallbackHandle,
  IN     EFI_ACPI_DMAR_ATSR_HEADER  *Atsr
  )
{
  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER   *DmarDeviceScopeEntry;
  INTN                                          AtsrLen;

  if (Atsr == NULL) {
    return;
  }

  VTDLIB_DEBUG ((DEBUG_INFO, 
    "  ***************************************************************************\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "  *       Root Port ATS Capability Reporting Structure                      *\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "  ***************************************************************************\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    (sizeof(UINTN) == sizeof(UINT64)) ?
    "  ATSR address ........................................... 0x%016lx\n" :
    "  ATSR address ........................................... 0x%08x\n",
    Atsr
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "    Type ................................................. 0x%04x\n",
    Atsr->Header.Type
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "    Length ............................................... 0x%04x\n",
    Atsr->Header.Length
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "    Flags ................................................ 0x%02x\n",
    Atsr->Flags
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "      ALL_PORTS .......................................... 0x%02x\n",
    Atsr->Flags & EFI_ACPI_DMAR_ATSR_FLAGS_ALL_PORTS
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "    Segment Number ....................................... 0x%04x\n",
    Atsr->SegmentNumber
    ));

  AtsrLen  = Atsr->Header.Length - sizeof(EFI_ACPI_DMAR_ATSR_HEADER);
  DmarDeviceScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)(Atsr + 1);
  while (AtsrLen > 0) {
    VtdLibDumpDmarDeviceScopeEntry (Context, CallbackHandle, DmarDeviceScopeEntry);
    AtsrLen -= DmarDeviceScopeEntry->Length;
    DmarDeviceScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)DmarDeviceScopeEntry + DmarDeviceScopeEntry->Length);
  }

  VTDLIB_DEBUG ((DEBUG_INFO, 
    "  ***************************************************************************\n\n"
    ));
}

/**
  Dump DMAR RMRR table.

  @param[in]      Context           Event context
  @param[in out]  CallbackHandle    Callback handler
  @param[in]      Rmrr              DMAR RMRR table
**/
VOID
VtdLibDumpDmarRmrr (
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LIB_STRING_CB    CallbackHandle,
  IN     EFI_ACPI_DMAR_RMRR_HEADER  *Rmrr
  )
{
  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER   *DmarDeviceScopeEntry;
  INTN                                          RmrrLen;

  if (Rmrr == NULL) {
    return;
  }

  VTDLIB_DEBUG ((DEBUG_INFO, 
    "  ***************************************************************************\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "  *       Reserved Memory Region Reporting Structure                        *\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "  ***************************************************************************\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    (sizeof(UINTN) == sizeof(UINT64)) ?
    "  RMRR address ........................................... 0x%016lx\n" :
    "  RMRR address ........................................... 0x%08x\n",
    Rmrr
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "    Type ................................................. 0x%04x\n",
    Rmrr->Header.Type
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "    Length ............................................... 0x%04x\n",
    Rmrr->Header.Length
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "    Segment Number ....................................... 0x%04x\n",
    Rmrr->SegmentNumber
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "    Reserved Memory Region Base Address .................. 0x%016lx\n",
    Rmrr->ReservedMemoryRegionBaseAddress
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "    Reserved Memory Region Limit Address ................. 0x%016lx\n",
    Rmrr->ReservedMemoryRegionLimitAddress
    ));

  RmrrLen  = Rmrr->Header.Length - sizeof(EFI_ACPI_DMAR_RMRR_HEADER);
  DmarDeviceScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)(Rmrr + 1);
  while (RmrrLen > 0) {
    VtdLibDumpDmarDeviceScopeEntry (Context, CallbackHandle, DmarDeviceScopeEntry);
    RmrrLen -= DmarDeviceScopeEntry->Length;
    DmarDeviceScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)DmarDeviceScopeEntry + DmarDeviceScopeEntry->Length);
  }

  VTDLIB_DEBUG ((DEBUG_INFO, 
    "  ***************************************************************************\n\n"
    ));
}

/**
  Dump DMAR DRHD table.

  @param[in]      Context           Event context
  @param[in out]  CallbackHandle    Callback handler
  @param[in]      Drhd              DMAR DRHD table
**/
VOID
VtdLibDumpDmarDrhd (
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LIB_STRING_CB    CallbackHandle,
  IN     EFI_ACPI_DMAR_DRHD_HEADER  *Drhd
  )
{
  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER   *DmarDeviceScopeEntry;
  INTN                                          DrhdLen;

  if (Drhd == NULL) {
    return;
  }

  VTDLIB_DEBUG ((DEBUG_INFO, 
    "  ***************************************************************************\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "  *       DMA-Remapping Hardware Definition Structure                       *\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "  ***************************************************************************\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    (sizeof(UINTN) == sizeof(UINT64)) ?
    "  DRHD address ........................................... 0x%016lx\n" :
    "  DRHD address ........................................... 0x%08x\n",
    Drhd
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "    Type ................................................. 0x%04x\n",
    Drhd->Header.Type
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "    Length ............................................... 0x%04x\n",
    Drhd->Header.Length
    ));
  VTDLIB_DEBUG ((DEBUG_INFO, 
    "    Flags ................................................ 0x%02x\n",
    Drhd->Flags
    ));
  VTDLIB_DEBUG ((DEBUG_INFO,
    "      INCLUDE_PCI_ALL .................................... 0x%02x\n",
    Drhd->Flags & EFI_ACPI_DMAR_DRHD_FLAGS_INCLUDE_PCI_ALL
    ));
  VTDLIB_DEBUG ((DEBUG_INFO,
    "    Size ................................................. 0x%02x\n",
    Drhd->Size
    ));
  VTDLIB_DEBUG ((DEBUG_INFO,
    "    Segment Number ....................................... 0x%04x\n",
    Drhd->SegmentNumber
    ));
  VTDLIB_DEBUG ((DEBUG_INFO,
    "    Register Base Address ................................ 0x%016lx\n",
    Drhd->RegisterBaseAddress
    ));

  DrhdLen  = Drhd->Header.Length - sizeof(EFI_ACPI_DMAR_DRHD_HEADER);
  DmarDeviceScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)(Drhd + 1);
  while (DrhdLen > 0) {
    VtdLibDumpDmarDeviceScopeEntry (Context, CallbackHandle, DmarDeviceScopeEntry);
    DrhdLen -= DmarDeviceScopeEntry->Length;
    DmarDeviceScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)DmarDeviceScopeEntry + DmarDeviceScopeEntry->Length);
  }

  VTDLIB_DEBUG ((DEBUG_INFO,
    "  ***************************************************************************\n\n"
    ));
}

/**
  Dump Header of DMAR ACPI table.

  @param[in]      Context           Event context
  @param[in out]  CallbackHandle    Callback handler
  @param[in]      Dmar              DMAR ACPI table
**/
VOID
VtdLibDumpAcpiDmarHeader (
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LIB_STRING_CB    CallbackHandle,
  IN     EFI_ACPI_DMAR_HEADER       *Dmar
  )
{
  //
  // Dump Dmar table
  //
  VTDLIB_DEBUG ((DEBUG_INFO,
    "*****************************************************************************\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO,
    "*         DMAR Table                                                        *\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO,
    "*****************************************************************************\n"
    ));

  VTDLIB_DEBUG ((DEBUG_INFO,
    (sizeof(UINTN) == sizeof(UINT64)) ?
    "DMAR address ............................................. 0x%016lx\n" :
    "DMAR address ............................................. 0x%08x\n",
    Dmar
    ));

  VTDLIB_DEBUG ((DEBUG_INFO,
    "  Table Contents:\n"
    ));
  VTDLIB_DEBUG ((DEBUG_INFO,
    "    Host Address Width ................................... 0x%02x\n",
    Dmar->HostAddressWidth
    ));
  VTDLIB_DEBUG ((DEBUG_INFO,
    "    Flags ................................................ 0x%02x\n",
    Dmar->Flags
    ));
  VTDLIB_DEBUG ((DEBUG_INFO,
    "      INTR_REMAP ......................................... 0x%02x\n",
    Dmar->Flags & EFI_ACPI_DMAR_FLAGS_INTR_REMAP
    ));
  VTDLIB_DEBUG ((DEBUG_INFO,
    "      X2APIC_OPT_OUT_SET ................................. 0x%02x\n",
    Dmar->Flags & EFI_ACPI_DMAR_FLAGS_X2APIC_OPT_OUT
    ));
  VTDLIB_DEBUG ((DEBUG_INFO,
    "      DMA_CTRL_PLATFORM_OPT_IN_FLAG ...................... 0x%02x\n",
    Dmar->Flags & EFI_ACPI_DMAR_FLAGS_DMA_CTRL_PLATFORM_OPT_IN_FLAG
    ));
}

/**
  Dump DMAR ACPI table.

  @param[in]      Context           Event context
  @param[in out]  CallbackHandle    Callback handler
  @param[in]      Dmar              DMAR ACPI table
**/
VOID
VtdLibDumpAcpiDmar (
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LIB_STRING_CB    CallbackHandle,
  IN     EFI_ACPI_DMAR_HEADER       *Dmar
  )
{
  EFI_ACPI_DMAR_STRUCTURE_HEADER    *DmarHeader;
  INTN                              DmarLen;

  if (Dmar == NULL) {
    return;
  }

  //
  // Dump Dmar table
  //
  VtdLibDumpAcpiDmarHeader (Context, CallbackHandle, Dmar);

  DmarLen  = Dmar->Header.Length - sizeof(EFI_ACPI_DMAR_HEADER);
  DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)(Dmar + 1);
  while (DmarLen > 0) {
    switch (DmarHeader->Type) {
    case EFI_ACPI_DMAR_TYPE_DRHD:
      VtdLibDumpDmarDrhd (Context, CallbackHandle, (EFI_ACPI_DMAR_DRHD_HEADER *)DmarHeader);
      break;
    case EFI_ACPI_DMAR_TYPE_RMRR:
      VtdLibDumpDmarRmrr (Context, CallbackHandle, (EFI_ACPI_DMAR_RMRR_HEADER *)DmarHeader);
      break;
    case EFI_ACPI_DMAR_TYPE_ATSR:
      VtdLibDumpDmarAtsr (Context, CallbackHandle, (EFI_ACPI_DMAR_ATSR_HEADER *)DmarHeader);
      break;
    case EFI_ACPI_DMAR_TYPE_RHSA:
      VtdLibDumpDmarRhsa (Context, CallbackHandle, (EFI_ACPI_DMAR_RHSA_HEADER *)DmarHeader);
      break;
    case EFI_ACPI_DMAR_TYPE_ANDD:
      VtdLibDumpDmarAndd (Context, CallbackHandle, (EFI_ACPI_DMAR_ANDD_HEADER *)DmarHeader);
      break;
    case EFI_ACPI_DMAR_TYPE_SATC:
      VtdLibDumpDmarSatc (Context, CallbackHandle, (EFI_ACPI_DMAR_SATC_HEADER *)DmarHeader);
      break;
    case EFI_ACPI_DMAR_TYPE_SIDP:
      VtdLibDumpDmarSidp (Context, CallbackHandle, (EFI_ACPI_DMAR_SIDP_HEADER *)DmarHeader);
      break;
    default:
      break;
    }
    DmarLen -= DmarHeader->Length;
    DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)DmarHeader + DmarHeader->Length);
  }

  VTDLIB_DEBUG ((DEBUG_INFO,
    "*****************************************************************************\n\n"
    ));
}

/**
  Dump DRHD DMAR ACPI table.

  @param[in]      Context           Event context
  @param[in out]  CallbackHandle    Callback handler
  @param[in]      Dmar              DMAR ACPI table
**/
VOID
VtdLibDumpAcpiDmarDrhd (
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LIB_STRING_CB    CallbackHandle,
  IN     EFI_ACPI_DMAR_HEADER       *Dmar
  )
{
  EFI_ACPI_DMAR_STRUCTURE_HEADER *DmarHeader;
  INTN                  DmarLen;

  if (Dmar == NULL) {
    return;
  }

  //
  // Dump Dmar table
  //
  VtdLibDumpAcpiDmarHeader (Context, CallbackHandle, Dmar);

  DmarLen  = Dmar->Header.Length - sizeof(EFI_ACPI_DMAR_HEADER);
  DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)(Dmar + 1);
  while (DmarLen > 0) {
    switch (DmarHeader->Type) {
    case EFI_ACPI_DMAR_TYPE_DRHD:
      VtdLibDumpDmarDrhd (Context, CallbackHandle, (EFI_ACPI_DMAR_DRHD_HEADER *)DmarHeader);
      break;
    default:
      break;
    }
    DmarLen -= DmarHeader->Length;
    DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)DmarHeader + DmarHeader->Length);
  }

  VTDLIB_DEBUG ((DEBUG_INFO,
    "*****************************************************************************\n\n"
    ));
}

/**
  Dump the PCI device information managed by this VTd engine.

  @param[in]      Context           Event context
  @param[in out]  CallbackHandle    Callback handler
  @param[in]      PciDeviceInfo     VTd Unit Information
**/
VOID
VtdLibDumpPciDeviceInfo (
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LIB_STRING_CB    CallbackHandle,
  IN     PCI_DEVICE_INFORMATION     *PciDeviceInfo
  )
{
  UINTN  Index;

  if (PciDeviceInfo != NULL) {
    VTDLIB_DEBUG ((DEBUG_INFO, "PCI Device Information (Number 0x%x, IncludeAll - %d):\n",
      PciDeviceInfo->PciDeviceDataNumber,
      PciDeviceInfo->IncludeAllFlag
      ));
    for (Index = 0; Index < PciDeviceInfo->PciDeviceDataNumber; Index++) {
      VTDLIB_DEBUG ((DEBUG_INFO, "  S%04x B%02x D%02x F%02x\n",
        PciDeviceInfo->Segment,
        PciDeviceInfo->PciDeviceData[Index].PciSourceId.Bits.Bus,
        PciDeviceInfo->PciDeviceData[Index].PciSourceId.Bits.Device,
        PciDeviceInfo->PciDeviceData[Index].PciSourceId.Bits.Function
        ));
    }
  }
}

/**
  Dump DMAR second level paging entry.

  @param[in]  Context                   Event context
  @param[in]  CallbackHandle            Callback handler
  @param[in]  SecondLevelPagingEntry    The second level paging entry.
  @param[in]  Is5LevelPaging            If it is the 5 level paging.
**/
VOID
VtdLibDumpSecondLevelPagingEntry (
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LIB_STRING_CB    CallbackHandle,
  IN     VOID                       *SecondLevelPagingEntry,
  IN     BOOLEAN                    Is5LevelPaging
  )
{
  UINTN                          Index5;
  UINTN                          Index4;
  UINTN                          Index3;
  UINTN                          Index2;
  UINTN                          Index1;
  UINTN                          Lvl5IndexEnd;
  VTD_SECOND_LEVEL_PAGING_ENTRY  *Lvl5PtEntry;
  VTD_SECOND_LEVEL_PAGING_ENTRY  *Lvl4PtEntry;
  VTD_SECOND_LEVEL_PAGING_ENTRY  *Lvl3PtEntry;
  VTD_SECOND_LEVEL_PAGING_ENTRY  *Lvl2PtEntry;
  VTD_SECOND_LEVEL_PAGING_ENTRY  *Lvl1PtEntry;

  VTDLIB_DEBUG ((DEBUG_VERBOSE, "================\n"));
  VTDLIB_DEBUG ((DEBUG_VERBOSE, "DMAR Second Level Page Table:\n"));
  VTDLIB_DEBUG ((DEBUG_VERBOSE, "SecondLevelPagingEntry Base - 0x%x, Is5LevelPaging - %d\n", SecondLevelPagingEntry, Is5LevelPaging));

  Lvl5IndexEnd = Is5LevelPaging ? SIZE_4KB/sizeof(VTD_SECOND_LEVEL_PAGING_ENTRY) : 1;
  Lvl4PtEntry = (VTD_SECOND_LEVEL_PAGING_ENTRY *)SecondLevelPagingEntry;
  Lvl5PtEntry = (VTD_SECOND_LEVEL_PAGING_ENTRY *)SecondLevelPagingEntry;

  for (Index5 = 0; Index5 < Lvl5IndexEnd; Index5++) {
    if (Is5LevelPaging) {
      if (Lvl5PtEntry[Index5].Uint64 != 0) {
        VTDLIB_DEBUG ((DEBUG_VERBOSE, "  Lvl5Pt Entry(0x%03x) - 0x%016lx\n", Index5, Lvl5PtEntry[Index5].Uint64));
      }
      if (Lvl5PtEntry[Index5].Uint64 == 0) {
        continue;
      }
      Lvl4PtEntry = (VTD_SECOND_LEVEL_PAGING_ENTRY *)(UINTN)VTD_64BITS_ADDRESS(Lvl5PtEntry[Index5].Bits.AddressLo, Lvl5PtEntry[Index5].Bits.AddressHi);
    }

    for (Index4 = 0; Index4 < SIZE_4KB/sizeof(VTD_SECOND_LEVEL_PAGING_ENTRY); Index4++) {
      if (Lvl4PtEntry[Index4].Uint64 != 0) {
        VTDLIB_DEBUG ((DEBUG_VERBOSE, "  Lvl4Pt Entry(0x%03x) - 0x%016lx\n", Index4, Lvl4PtEntry[Index4].Uint64));
      }
      if (Lvl4PtEntry[Index4].Uint64 == 0) {
        continue;
      }
      Lvl3PtEntry = (VTD_SECOND_LEVEL_PAGING_ENTRY *)(UINTN)VTD_64BITS_ADDRESS(Lvl4PtEntry[Index4].Bits.AddressLo, Lvl4PtEntry[Index4].Bits.AddressHi);
      for (Index3 = 0; Index3 < SIZE_4KB/sizeof(VTD_SECOND_LEVEL_PAGING_ENTRY); Index3++) {
        if (Lvl3PtEntry[Index3].Uint64 != 0) {
          VTDLIB_DEBUG ((DEBUG_VERBOSE, "   Lvl3Pt Entry(0x%03x) - 0x%016lx\n", Index3, Lvl3PtEntry[Index3].Uint64));
        }
        if (Lvl3PtEntry[Index3].Uint64 == 0) {
          continue;
        }

        Lvl2PtEntry = (VTD_SECOND_LEVEL_PAGING_ENTRY *)(UINTN)VTD_64BITS_ADDRESS(Lvl3PtEntry[Index3].Bits.AddressLo, Lvl3PtEntry[Index3].Bits.AddressHi);
        for (Index2 = 0; Index2 < SIZE_4KB/sizeof(VTD_SECOND_LEVEL_PAGING_ENTRY); Index2++) {
          if (Lvl2PtEntry[Index2].Uint64 != 0) {
            VTDLIB_DEBUG ((DEBUG_VERBOSE, "    Lvl2Pt Entry(0x%03x) - 0x%016lx\n", Index2, Lvl2PtEntry[Index2].Uint64));
          }
          if (Lvl2PtEntry[Index2].Uint64 == 0) {
            continue;
          }
          if (Lvl2PtEntry[Index2].Bits.PageSize == 0) {
            Lvl1PtEntry = (VTD_SECOND_LEVEL_PAGING_ENTRY *)(UINTN)VTD_64BITS_ADDRESS(Lvl2PtEntry[Index2].Bits.AddressLo, Lvl2PtEntry[Index2].Bits.AddressHi);
            for (Index1 = 0; Index1 < SIZE_4KB/sizeof(VTD_SECOND_LEVEL_PAGING_ENTRY); Index1++) {
              if (Lvl1PtEntry[Index1].Uint64 != 0) {
                VTDLIB_DEBUG ((DEBUG_VERBOSE, "      Lvl1Pt Entry(0x%03x) - 0x%016lx\n", Index1, Lvl1PtEntry[Index1].Uint64));
              }
            }
          }
        }
      }
    }
  }
  VTDLIB_DEBUG ((DEBUG_VERBOSE, "================\n"));
}

/**
  Dump DMAR context entry table.

  @param[in]      Context           Event context
  @param[in out]  CallbackHandle    Callback handler
  @param[in]      RootEntry         DMAR root entry.
  @param[in]      Is5LevelPaging    If it is the 5 level paging.
**/
VOID
VtdLibDumpDmarContextEntryTable (
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LIB_STRING_CB    CallbackHandle,
  IN     VTD_ROOT_ENTRY             *RootEntry,
  IN     BOOLEAN                    Is5LevelPaging
  )
{
  UINTN                 Index;
  UINTN                 Index2;
  VTD_CONTEXT_ENTRY     *ContextEntry;

  VTDLIB_DEBUG ((DEBUG_INFO, "=========================\n"));
  VTDLIB_DEBUG ((DEBUG_INFO, "DMAR Context Entry Table:\n"));

  VTDLIB_DEBUG ((DEBUG_INFO, "RootEntry Address - 0x%x\n", RootEntry));

  for (Index = 0; Index < VTD_ROOT_ENTRY_NUMBER; Index++) {
    if ((RootEntry[Index].Uint128.Uint64Lo != 0) || (RootEntry[Index].Uint128.Uint64Hi != 0)) {
      VTDLIB_DEBUG ((DEBUG_INFO, "  RootEntry(0x%02x) B%02x - 0x%016lx %016lx\n",
        Index, Index, RootEntry[Index].Uint128.Uint64Hi, RootEntry[Index].Uint128.Uint64Lo));
    }
    if (RootEntry[Index].Bits.Present == 0) {
      continue;
    }
    ContextEntry = (VTD_CONTEXT_ENTRY *) (UINTN) VTD_64BITS_ADDRESS (RootEntry[Index].Bits.ContextTablePointerLo, RootEntry[Index].Bits.ContextTablePointerHi);
    for (Index2 = 0; Index2 < VTD_CONTEXT_ENTRY_NUMBER; Index2++) {
      if ((ContextEntry[Index2].Uint128.Uint64Lo != 0) || (ContextEntry[Index2].Uint128.Uint64Hi != 0)) {
        VTDLIB_DEBUG ((DEBUG_INFO, "    ContextEntry(0x%02x) D%02xF%02x - 0x%016lx %016lx\n",
          Index2, Index2 >> 3, Index2 & 0x7, ContextEntry[Index2].Uint128.Uint64Hi, ContextEntry[Index2].Uint128.Uint64Lo));
      }
      if (ContextEntry[Index2].Bits.Present == 0) {
        continue;
      }
      VtdLibDumpSecondLevelPagingEntry (Context, CallbackHandle, (VOID *) (UINTN) VTD_64BITS_ADDRESS (ContextEntry[Index2].Bits.SecondLevelPageTranslationPointerLo, ContextEntry[Index2].Bits.SecondLevelPageTranslationPointerHi), Is5LevelPaging);
    }
  }
  VTDLIB_DEBUG ((DEBUG_INFO, "=========================\n"));
}

/**
  Dump DMAR extended context entry table.

  @param[in]      Context           Event context
  @param[in out]  CallbackHandle    Callback handler
  @param[in]      ExtRootEntry      DMAR extended root entry.
  @param[in]      Is5LevelPaging    If it is the 5 level paging.
**/
VOID
VtdLibDumpDmarExtContextEntryTable (
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LIB_STRING_CB    CallbackHandle,
  IN     VTD_EXT_ROOT_ENTRY         *ExtRootEntry,
  IN     BOOLEAN                    Is5LevelPaging
  )
{
  UINTN                 Index;
  UINTN                 Index2;
  VTD_EXT_CONTEXT_ENTRY *ExtContextEntry;

  VTDLIB_DEBUG ((DEBUG_INFO, "=========================\n"));
  VTDLIB_DEBUG ((DEBUG_INFO, "DMAR ExtContext Entry Table:\n"));

  VTDLIB_DEBUG ((DEBUG_INFO, "ExtRootEntry Address - 0x%x\n", ExtRootEntry));

  for (Index = 0; Index < VTD_ROOT_ENTRY_NUMBER; Index++) {
    if ((ExtRootEntry[Index].Uint128.Uint64Lo != 0) || (ExtRootEntry[Index].Uint128.Uint64Hi != 0)) {
      VTDLIB_DEBUG ((DEBUG_INFO, "  ExtRootEntry(0x%02x) B%02x - 0x%016lx %016lx\n",
        Index, Index, ExtRootEntry[Index].Uint128.Uint64Hi, ExtRootEntry[Index].Uint128.Uint64Lo));
    }
    if (ExtRootEntry[Index].Bits.LowerPresent == 0) {
      continue;
    }
    ExtContextEntry = (VTD_EXT_CONTEXT_ENTRY *) (UINTN) VTD_64BITS_ADDRESS (ExtRootEntry[Index].Bits.LowerContextTablePointerLo, ExtRootEntry[Index].Bits.LowerContextTablePointerHi);
    for (Index2 = 0; Index2 < VTD_CONTEXT_ENTRY_NUMBER/2; Index2++) {
      if ((ExtContextEntry[Index2].Uint256.Uint64_1 != 0) || (ExtContextEntry[Index2].Uint256.Uint64_2 != 0) ||
          (ExtContextEntry[Index2].Uint256.Uint64_3 != 0) || (ExtContextEntry[Index2].Uint256.Uint64_4 != 0)) {
        VTDLIB_DEBUG ((DEBUG_INFO, "    ExtContextEntryLower(0x%02x) D%02xF%02x - 0x%016lx %016lx %016lx %016lx\n",
          Index2, Index2 >> 3, Index2 & 0x7, ExtContextEntry[Index2].Uint256.Uint64_4, ExtContextEntry[Index2].Uint256.Uint64_3, ExtContextEntry[Index2].Uint256.Uint64_2, ExtContextEntry[Index2].Uint256.Uint64_1));
      }
      if (ExtContextEntry[Index2].Bits.Present == 0) {
        continue;
      }
      VtdLibDumpSecondLevelPagingEntry (Context, CallbackHandle, (VOID *) (UINTN) VTD_64BITS_ADDRESS (ExtContextEntry[Index2].Bits.SecondLevelPageTranslationPointerLo, ExtContextEntry[Index2].Bits.SecondLevelPageTranslationPointerHi), Is5LevelPaging);
    }

    if (ExtRootEntry[Index].Bits.UpperPresent == 0) {
      continue;
    }
    ExtContextEntry = (VTD_EXT_CONTEXT_ENTRY *) (UINTN) VTD_64BITS_ADDRESS (ExtRootEntry[Index].Bits.UpperContextTablePointerLo, ExtRootEntry[Index].Bits.UpperContextTablePointerHi);
    for (Index2 = 0; Index2 < VTD_CONTEXT_ENTRY_NUMBER/2; Index2++) {
      if ((ExtContextEntry[Index2].Uint256.Uint64_1 != 0) || (ExtContextEntry[Index2].Uint256.Uint64_2 != 0) ||
          (ExtContextEntry[Index2].Uint256.Uint64_3 != 0) || (ExtContextEntry[Index2].Uint256.Uint64_4 != 0)) {
        VTDLIB_DEBUG ((DEBUG_INFO, "    ExtContextEntryUpper(0x%02x) D%02xF%02x - 0x%016lx %016lx %016lx %016lx\n",
          Index2, (Index2 + 128) >> 3, (Index2 + 128) & 0x7, ExtContextEntry[Index2].Uint256.Uint64_4, ExtContextEntry[Index2].Uint256.Uint64_3, ExtContextEntry[Index2].Uint256.Uint64_2, ExtContextEntry[Index2].Uint256.Uint64_1));
      }
      if (ExtContextEntry[Index2].Bits.Present == 0) {
        continue;
      }
    }
  }
  VTDLIB_DEBUG ((DEBUG_INFO, "=========================\n"));
}

/**
  Dump VTd FRCD register.

  @param[in]      Context           Event context
  @param[in out]  CallbackHandle    Callback handler
  @param[in]      FrcdRegNum        FRCD Register Number
  @param[in]      FrcdRegTab        FRCD Register Table
**/
VOID
VtdLibDumpVtdFrcdRegs (
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LIB_STRING_CB    CallbackHandle,
  IN     UINT16                     FrcdRegNum,
  IN     VTD_UINT128                *FrcdRegTab
  )
{
  UINT16        Index;
  VTD_FRCD_REG  FrcdReg;
  VTD_SOURCE_ID SourceId;

  for (Index = 0; Index < FrcdRegNum; Index++) {
    FrcdReg.Uint64[0] = FrcdRegTab[Index].Uint64Lo;
    FrcdReg.Uint64[1] = FrcdRegTab[Index].Uint64Hi;
    VTDLIB_DEBUG ((DEBUG_INFO, "  FRCD_REG[%d] - 0x%016lx %016lx\n", Index, FrcdReg.Uint64[1], FrcdReg.Uint64[0]));
    if (FrcdReg.Uint64[1] != 0 || FrcdReg.Uint64[0] != 0) {
      VTDLIB_DEBUG ((DEBUG_INFO, "    Fault Info - 0x%016lx\n", VTD_64BITS_ADDRESS(FrcdReg.Bits.FILo, FrcdReg.Bits.FIHi)));
      VTDLIB_DEBUG ((DEBUG_INFO, "    Fault Bit - %d\n", FrcdReg.Bits.F));
      SourceId.Uint16 = (UINT16)FrcdReg.Bits.SID;
      VTDLIB_DEBUG ((DEBUG_INFO, "    Source - B%02x D%02x F%02x\n", SourceId.Bits.Bus, SourceId.Bits.Device, SourceId.Bits.Function));
      VTDLIB_DEBUG ((DEBUG_INFO, "    Type - 0x%02x\n", (FrcdReg.Bits.T1 << 1) | FrcdReg.Bits.T2));
      VTDLIB_DEBUG ((DEBUG_INFO, "    Reason - %x (Refer to VTd Spec, Appendix A)\n", FrcdReg.Bits.FR));
    }
  }
}

/**
  Dump VTd registers.

  @param[in]      Context           Event context
  @param[in out]  CallbackHandle    Callback handler
  @param[in]      VtdRegInfo        Registers information
**/
VOID
VtdLibDumpVtdRegsAll (
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LIB_STRING_CB    CallbackHandle,
  IN     VTD_REGESTER_INFO          *VtdRegInfo
  )
{
  if (VtdRegInfo != NULL) {
    VTDLIB_DEBUG ((DEBUG_INFO, "VTd Engine: [0x%016lx]\n", VtdRegInfo->BaseAddress));
    VTDLIB_DEBUG ((DEBUG_INFO, "  VER_REG     - 0x%08x\n",   VtdRegInfo->VerReg));
    VTDLIB_DEBUG ((DEBUG_INFO, "  CAP_REG     - 0x%016lx\n", VtdRegInfo->CapReg));
    VTDLIB_DEBUG ((DEBUG_INFO, "  ECAP_REG    - 0x%016lx\n", VtdRegInfo->EcapReg));
    VTDLIB_DEBUG ((DEBUG_INFO, "  GSTS_REG    - 0x%08x \n",  VtdRegInfo->GstsReg));
    VTDLIB_DEBUG ((DEBUG_INFO, "  RTADDR_REG  - 0x%016lx\n", VtdRegInfo->RtaddrReg));
    VTDLIB_DEBUG ((DEBUG_INFO, "  CCMD_REG    - 0x%016lx\n", VtdRegInfo->CcmdReg));
    VTDLIB_DEBUG ((DEBUG_INFO, "  FSTS_REG    - 0x%08x\n",   VtdRegInfo->FstsReg));
    VTDLIB_DEBUG ((DEBUG_INFO, "  FECTL_REG   - 0x%08x\n",   VtdRegInfo->FectlReg));
    VTDLIB_DEBUG ((DEBUG_INFO, "  FEDATA_REG  - 0x%08x\n",   VtdRegInfo->FedataReg));
    VTDLIB_DEBUG ((DEBUG_INFO, "  FEADDR_REG  - 0x%08x\n",   VtdRegInfo->FeaddrReg));
    VTDLIB_DEBUG ((DEBUG_INFO, "  FEUADDR_REG - 0x%08x\n",   VtdRegInfo->FeuaddrReg));
    VTDLIB_DEBUG ((DEBUG_INFO, "  IQERCD_REG  - 0x%016lx\n", VtdRegInfo->IqercdReg));

    VtdLibDumpVtdFrcdRegs (Context, CallbackHandle, VtdRegInfo->FrcdRegNum, VtdRegInfo->FrcdReg);

    VTDLIB_DEBUG ((DEBUG_INFO, "  IVA_REG     - 0x%016lx\n", VtdRegInfo->IvaReg));
    VTDLIB_DEBUG ((DEBUG_INFO, "  IOTLB_REG   - 0x%016lx\n", VtdRegInfo->IotlbReg));
  }
}

/**
  Dump VTd registers.

  @param[in]      Context           Event context
  @param[in out]  CallbackHandle    Callback handler
  @param[in]      VtdRegInfo        Registers information
**/
VOID
VtdLibDumpVtdRegsThin (
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LIB_STRING_CB    CallbackHandle,
  IN     VTD_REGESTER_THIN_INFO     *VtdRegInfo
  )
{
  if (VtdRegInfo != NULL) {
    VTDLIB_DEBUG ((DEBUG_INFO, "VTd Engine: [0x%016lx]\n", VtdRegInfo->BaseAddress));
    VTDLIB_DEBUG ((DEBUG_INFO, "  GSTS_REG    - 0x%08x \n",  VtdRegInfo->GstsReg));
    VTDLIB_DEBUG ((DEBUG_INFO, "  RTADDR_REG  - 0x%016lx\n", VtdRegInfo->RtaddrReg));
    VTDLIB_DEBUG ((DEBUG_INFO, "  FSTS_REG    - 0x%08x\n",   VtdRegInfo->FstsReg));
    VTDLIB_DEBUG ((DEBUG_INFO, "  FECTL_REG   - 0x%08x\n",   VtdRegInfo->FectlReg));
    VTDLIB_DEBUG ((DEBUG_INFO, "  IQERCD_REG  - 0x%016lx\n", VtdRegInfo->IqercdReg));

    VtdLibDumpVtdFrcdRegs (Context, CallbackHandle, VtdRegInfo->FrcdRegNum, VtdRegInfo->FrcdReg);
  }
}

/**
  Dump VTd registers.

  @param[in]      Context           Event context
  @param[in out]  CallbackHandle    Callback handler
  @param[in]      VtdRegInfo        Registers information
**/
VOID
VtdLibDumpVtdRegsQi (
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LIB_STRING_CB    CallbackHandle,
  IN     VTD_REGESTER_QI_INFO       *VtdRegInfo
  )
{
  if (VtdRegInfo != NULL) {
    VTDLIB_DEBUG ((DEBUG_INFO, "VTd Engine: [0x%016lx]\n", VtdRegInfo->BaseAddress));
    VTDLIB_DEBUG ((DEBUG_INFO, "  FSTS_REG    - 0x%08x\n",   VtdRegInfo->FstsReg));
    VTDLIB_DEBUG ((DEBUG_INFO, "  IQERCD_REG  - 0x%016lx\n", VtdRegInfo->IqercdReg));
  }
}

/**
  Dump Vtd PEI pre-mem event.

  @param[in]      Context           Event context
  @param[in out]  CallbackHandle    Callback handler
  @param[in]      Event             VTDLOG_EVENT_2PARAM event

**/
VOID
VtdLibDumpPeiPreMemInfo (
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LIB_STRING_CB    CallbackHandle,
  IN     VTDLOG_EVENT_2PARAM        *Event
  )
{
  UINT64                            VtdBarAddress;
  UINT64                            Mode;
  UINT64                            Status;

  VtdBarAddress = Event->Data1;
  Mode = Event->Data2 & 0xFF;
  Status = (Event->Data2>>8) & 0xFF;

  switch (Mode) {
  case VTD_LOG_PEI_PRE_MEM_DISABLE:
    VTDLIB_DEBUG ((DEBUG_INFO, "PEI (pre-memory): Disabled [0x%016lx] 0x%x\n", VtdBarAddress, Status));
    break;
  case VTD_LOG_PEI_PRE_MEM_ADM:
    VTDLIB_DEBUG ((DEBUG_INFO, "PEI (pre-memory): Enable Abort DMA Mode [0x%016lx] 0x%x\n", VtdBarAddress, Status));
    break;
  case VTD_LOG_PEI_PRE_MEM_TE:
    VTDLIB_DEBUG ((DEBUG_INFO, "PEI (pre-memory): Enable NULL Root Entry Table [0x%016lx] 0x%x\n", VtdBarAddress, Status));
    break;
  case VTD_LOG_PEI_PRE_MEM_PMR:
    VTDLIB_DEBUG ((DEBUG_INFO, "PEI (pre-memory): Enable PMR [0x%016lx] 0x%x\n", VtdBarAddress, Status));
    break;
  case VTD_LOG_PEI_PRE_MEM_NOT_USED:
    //
    // Not used
    //
    break;
  default:
    VTDLIB_DEBUG ((DEBUG_INFO, "PEI (pre-memory): Unknown [0x%016lx] 0x%x\n", VtdBarAddress, Status));
    break;
  }
}

/**
  Dump Vtd Queued Invaildation event.

  @param[in]      Context           Event context
  @param[in out]  CallbackHandle    Callback handler
  @param[in]      Event             VTDLOG_EVENT_2PARAM event

**/
VOID
VtdLibDumpQueuedInvaildation (
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LIB_STRING_CB    CallbackHandle,
  IN     VTDLOG_EVENT_2PARAM        *Event
  )
{
  switch (Event->Data1) {
  case VTD_LOG_QI_DISABLE:
    VTDLIB_DEBUG ((DEBUG_INFO, " [0x%016lx] Disable\n", Event->Data2));
    break;
  case VTD_LOG_QI_ENABLE:
    VTDLIB_DEBUG ((DEBUG_INFO, " [0x%016lx] Enable\n", Event->Data2));
    break;
  case VTD_LOG_QI_ERROR_OUT_OF_RESOURCES:
    VTDLIB_DEBUG ((DEBUG_INFO, " [0x%016lx] error - Out of resources\n", Event->Data2));
    break;
  default:
    VTDLIB_DEBUG ((DEBUG_INFO, " [0x%016lx] error - (0x%x)\n", Event->Data2, Event->Data1));
    break;
  }
}

/**
  Dump Vtd registers event.

  @param[in]      Context           Event context
  @param[in out]  CallbackHandle    Callback handler
  @param[in]      Event             VTDLOG_EVENT_CONTEXT event

**/
VOID
VtdLibDumpRegisters (
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LIB_STRING_CB    CallbackHandle,
  IN     VTDLOG_EVENT_CONTEXT       *Event
  )
{
  switch (Event->Param) {
  case VTDLOG_REGISTER_ALL:
    VtdLibDumpVtdRegsAll (Context, CallbackHandle, (VTD_REGESTER_INFO *) Event->Data);
    break;
  case VTDLOG_REGISTER_THIN:
    VtdLibDumpVtdRegsThin (Context, CallbackHandle, (VTD_REGESTER_THIN_INFO *) Event->Data);
    break;
  case VTDLOG_REGISTER_QI:
    VtdLibDumpVtdRegsQi (Context, CallbackHandle, (VTD_REGESTER_QI_INFO *) Event->Data);
    break;
  default:
    VTDLIB_DEBUG ((DEBUG_INFO, "  Unknown format (%d)\n", Event->Param));
    break;
  }
}

/**
  Dump Vtd PEI Error event.

  @param[in]      Context           Event context
  @param[in out]  CallbackHandle    Callback handler
  @param[in]      Event             VTDLOG_EVENT_2PARAM event

**/
VOID
VtdLibDumpPeiError (
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LIB_STRING_CB    CallbackHandle,
  IN     VTDLOG_EVENT_2PARAM        *Event
  )
{
  UINT64                            Timestamp;

  Timestamp = Event->Header.Timestamp;

  switch (Event->Data1) {
  case VTD_LOG_PEI_VTD_ERROR_PPI_ALLOC:
    VTDLIB_DEBUG ((DEBUG_INFO, "PEI [%ld]: Error - PPI alloc length [0x%016lx]\n", Timestamp, Event->Data2));
    break;
  case VTD_LOG_PEI_VTD_ERROR_PPI_MAP:
    VTDLIB_DEBUG ((DEBUG_INFO, "PEI [%ld]: Error - PPI map length [0x%016lx]\n", Timestamp, Event->Data2));
    break;
  default:
    VTDLIB_DEBUG ((DEBUG_INFO, "PEI [%ld]: Error - Unknown (%d) 0x%x\n", Timestamp, Event->Data1, Event->Data2));
    break;
  }
}

/**
  Dump Vtd registers event.

  @param[in]      Context           Event context
  @param[in out]  CallbackHandle    Callback handler
  @param[in]      Event             VTDLOG_EVENT_CONTEXT event

**/
VOID
VtdLibDumpSetAttribute (
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LIB_STRING_CB    CallbackHandle,
  IN     VTDLOG_EVENT_CONTEXT       *Event
  )
{
  VTD_PROTOCOL_SET_ATTRIBUTE * SetAttributeInfo;

  SetAttributeInfo = (VTD_PROTOCOL_SET_ATTRIBUTE *) Event->Data;

  VTDLIB_DEBUG ((DEBUG_INFO, "DXE [%ld]: SetAttribute SourceId = 0x%04x, Address = 0x%lx, Length = 0x%lx, IoMmuAccess = 0x%lx, %r\n", 
                 Event->Header.Timestamp,
                 SetAttributeInfo->SourceId.Uint16,
                 SetAttributeInfo->DeviceAddress,
                 SetAttributeInfo->Length,
                 SetAttributeInfo->IoMmuAccess,
                 SetAttributeInfo->Status));
}



/**
  Dump Vtd Root Table event.

  @param[in]      Context           Event context
  @param[in out]  CallbackHandle    Callback handler
  @param[in]      Event             VTDLOG_EVENT_CONTEXT event

**/
VOID
VtdLibDumpRootTable (
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LIB_STRING_CB    CallbackHandle,
  IN     VTDLOG_EVENT_CONTEXT       *Event
  )
{
  VTD_ROOT_TABLE_INFO *RootTableInfo;

  RootTableInfo = (VTD_ROOT_TABLE_INFO *) Event->Data;
  if (Event->Param == 0) {
    VTDLIB_DEBUG ((DEBUG_INFO, "DXE [%ld]: Root Entry Table [0x%016lx]\n", Event->Header.Timestamp, RootTableInfo->BaseAddress));
    VtdLibDumpDmarContextEntryTable (Context, CallbackHandle, (VTD_ROOT_ENTRY *) (UINTN) RootTableInfo->TableAddress, RootTableInfo->Is5LevelPaging);

  } else if (Event->Param == 1) {
    VTDLIB_DEBUG ((DEBUG_INFO, "DXE [%ld]: Ext Root Entry Table [0x%016lx]\n", Event->Header.Timestamp, RootTableInfo->BaseAddress));
    VtdLibDumpDmarExtContextEntryTable (Context, CallbackHandle, (VTD_EXT_ROOT_ENTRY *) (UINTN)  RootTableInfo->TableAddress, RootTableInfo->Is5LevelPaging);

  } else {
    VTDLIB_DEBUG ((DEBUG_INFO, "DXE [%ld]: Unknown Root Table Type (%d)\n", Event->Header.Timestamp, Event->Param));
  }
}

/**
  Decode log event.

  @param[in]      Context           Event context
  @param[in out]  PciDeviceId       Callback handler
  @param[in]      Event             Event struct

  @retval         TRUE              Decode event success
  @retval         FALSE             Unknown event
**/
BOOLEAN
VtdLibDecodeEvent (
  IN     VOID                       *Context,
  IN OUT EDKII_VTD_LIB_STRING_CB    CallbackHandle,
  IN     VTDLOG_EVENT               *Event
  )
{
  BOOLEAN                           Result;
  UINT64                            Timestamp;
  UINT64                            Data1;
  UINT64                            Data2;

  Result    = TRUE;
  Timestamp = Event->EventHeader.Timestamp;
  Data1     = Event->CommenEvent.Data1;
  Data2     = Event->CommenEvent.Data2;

  switch (Event->EventHeader.LogType) {
  case VTDLOG_LOG_TYPE (VTDLOG_PEI_BASIC):
    if (Data1 & VTD_LOG_ERROR_BUFFER_FULL) {
      VTDLIB_DEBUG ((DEBUG_INFO, "PEI [%ld]: Info : Log Buffer Full\n", Timestamp));
      Data1 &= ~VTD_LOG_ERROR_BUFFER_FULL;
    }
    if (Data1 != 0) {
      VTDLIB_DEBUG ((DEBUG_INFO, "PEI [%ld]: Info : 0x%x, 0x%x\n", Timestamp, Data1, Data2));
    }
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_PEI_PRE_MEM_DMA_PROTECT):
    VtdLibDumpPeiPreMemInfo (Context, CallbackHandle, &(Event->CommenEvent));
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_PEI_PMR_LOW_MEMORY_RANGE):
    VTDLIB_DEBUG ((DEBUG_INFO, "PEI [%ld]: PMR Low Memory Range [0x%x, 0x%x]\n", Timestamp, Data1, Data2));
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_PEI_PMR_HIGH_MEMORY_RANGE):
    VTDLIB_DEBUG ((DEBUG_INFO, "PEI [%ld]: PMR High Memory Range [0x%016lx, 0x%016lx]\n", Timestamp, Data1, Data2));
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_PEI_PROTECT_MEMORY_RANGE):
    VTDLIB_DEBUG ((DEBUG_INFO, "PEI [%ld]: Protected DMA Memory Range [0x%016lx, 0x%016lx]\n", Timestamp, Data1, Data2));
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_PEI_POST_MEM_ENABLE_DMA_PROTECT):
    VTDLIB_DEBUG ((DEBUG_INFO, "PEI [%ld]: Enable DMA protection [0x%016lx] %r\n", Timestamp, Data1, Data2));
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_PEI_POST_MEM_DISABLE_DMA_PROTECT):
    VTDLIB_DEBUG ((DEBUG_INFO, "PEI [%ld]: Disable DMA protection [0x%016lx]\n", Timestamp, Data1));
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_PEI_QUEUED_INVALIDATION):
    VTDLIB_DEBUG ((DEBUG_INFO, "PEI [%ld]: Queued Invalidation", Timestamp));
    VtdLibDumpQueuedInvaildation (Context, CallbackHandle, &(Event->CommenEvent));
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_PEI_REGISTER):
    VTDLIB_DEBUG ((DEBUG_INFO, "PEI [%ld]: Dump Registers\n", Timestamp));
    VtdLibDumpRegisters (Context, CallbackHandle, &(Event->ContextEvent));
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_PEI_VTD_ERROR):
    VtdLibDumpPeiError (Context, CallbackHandle, &(Event->CommenEvent));
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_PEI_PPI_ALLOC_BUFFER):
    VTDLIB_DEBUG ((DEBUG_INFO, "PEI [%ld]: PPI AllocateBuffer 0x%x, Length = 0x%x\n", Timestamp, Data1, Data2));
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_PEI_PPI_MAP):
    VTDLIB_DEBUG ((DEBUG_INFO, "PEI [%ld]: PPI Map 0x%x, Length = 0x%x\n", Timestamp, Data1, Data2));
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_DXE_BASIC):
    if (Data1 & VTD_LOG_ERROR_BUFFER_FULL) {
      VTDLIB_DEBUG ((DEBUG_INFO, "DXE [%ld]: Info : Log Buffer Full\n", Timestamp));
      Data1 &= ~VTD_LOG_ERROR_BUFFER_FULL;
    }
    if (Data1 != 0) {
      VTDLIB_DEBUG ((DEBUG_INFO, "DXE [%ld]: Info : 0x%x, 0x%x\n", Timestamp, Data1, Data2));
    }
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_DXE_DMAR_TABLE):
    VTDLIB_DEBUG ((DEBUG_INFO, "DXE [%ld]: DMAR Table\n", Timestamp));
    VtdLibDumpAcpiDmar (Context, CallbackHandle, (EFI_ACPI_DMAR_HEADER *) Event->ContextEvent.Data);
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_DXE_SETUP_VTD):
    VTDLIB_DEBUG ((DEBUG_INFO, "DXE [%ld]: Setup VTd Below/Above 4G Memory Limit = [0x%016lx, 0x%016lx]\n", Timestamp, Data1, Data2));
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_DXE_PCI_DEVICE):
    VTDLIB_DEBUG ((DEBUG_INFO, "DXE [%ld]: PCI Devices [0x%016lx]\n", Timestamp, Event->ContextEvent.Param));
    VtdLibDumpPciDeviceInfo (Context, CallbackHandle, (PCI_DEVICE_INFORMATION *) Event->ContextEvent.Data);
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_DXE_REGISTER):
    VTDLIB_DEBUG ((DEBUG_INFO, "DXE [%ld]: Dump Registers\n", Timestamp));
    VtdLibDumpRegisters (Context, CallbackHandle, &(Event->ContextEvent));
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_DXE_ENABLE_DMAR):
    VTDLIB_DEBUG ((DEBUG_INFO, "DXE [%ld]: Enable DMAR [0x%016lx]\n", Timestamp, Data1));
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_DXE_DISABLE_DMAR):
    VTDLIB_DEBUG ((DEBUG_INFO, "DXE [%ld]: Disable DMAR [0x%016lx]\n", Timestamp, Data1));
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_DXE_DISABLE_PMR):
    VTDLIB_DEBUG ((DEBUG_INFO, "DXE [%ld]: Disable PMR [0x%016lx] %r\n", Timestamp, Data1, Data2));
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_DXE_INSTALL_IOMMU_PROTOCOL):
    VTDLIB_DEBUG ((DEBUG_INFO, "DXE [%ld]: Install IOMMU Protocol %r\n", Timestamp, Data1));
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_DXE_QUEUED_INVALIDATION):
    VTDLIB_DEBUG ((DEBUG_INFO, "DXE [%ld]: Queued Invalidation", Timestamp));
    VtdLibDumpQueuedInvaildation (Context, CallbackHandle, &(Event->CommenEvent));
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_DXE_ROOT_TABLE):
    VtdLibDumpRootTable (Context, CallbackHandle, &(Event->ContextEvent));
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_DXE_IOMMU_ALLOC_BUFFER):
    VTDLIB_DEBUG ((DEBUG_INFO, "DXE [%ld]: AllocateBuffer 0x%x, Page = 0x%x\n", Timestamp, Data2, Data1));
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_DXE_IOMMU_FREE_BUFFER):
    VTDLIB_DEBUG ((DEBUG_INFO, "DXE [%ld]: FreeBuffer 0x%x, Page = 0x%x\n", Timestamp, Data2, Data1));
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_DXE_IOMMU_MAP):
    VTDLIB_DEBUG ((DEBUG_INFO, "DXE [%ld]: Map 0x%x, Operation = 0x%x\n", Timestamp, Data1, Data2));
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_DXE_IOMMU_UNMAP):
    VTDLIB_DEBUG ((DEBUG_INFO, "DXE [%ld]: Unmap 0x%x, NumberOfBytes = 0x%x\n", Timestamp, Data2, Data1));
    break;
  case VTDLOG_LOG_TYPE (VTDLOG_DXE_IOMMU_SET_ATTRIBUTE):
    VtdLibDumpSetAttribute (Context, CallbackHandle, &(Event->ContextEvent));
    break;
  default:
    VTDLIB_DEBUG ((DEBUG_INFO, "## Unknown VTd Event Type=%d Timestamp=%ld Size=%d\n", Event->EventHeader.LogType, Event->EventHeader.Timestamp, Event->EventHeader.DataSize));
    Result = FALSE;
    break;
  }

  return Result;
}

/**
  Flush VTd engine write buffer.

  @param[in] VtdUnitBaseAddress     The base address of the VTd engine.
**/
VOID
VtdLibFlushWriteBuffer (
  IN UINTN                      VtdUnitBaseAddress
  )
{
  UINT32        Reg32;
  VTD_CAP_REG   CapReg;

  CapReg.Uint64 = MmioRead64 (VtdUnitBaseAddress + R_CAP_REG);

  if (CapReg.Bits.RWBF != 0) {
    Reg32 = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
    Reg32 = (Reg32 & 0x96FFFFFF);       // Reset the one-shot bits
    MmioWrite32 (VtdUnitBaseAddress + R_GCMD_REG, Reg32 | B_GMCD_REG_WBF);
    do {
      Reg32 = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
    } while ((Reg32 & B_GSTS_REG_WBF) != 0);
  }
}

/**
  Clear Global Command Register Bits

  @param[in] VtdUnitBaseAddress     The base address of the VTd engine.
  @param[in] BitMask                Bit mask
**/
VOID
VtdLibClearGlobalCommandRegisterBits (
  IN UINTN                      VtdUnitBaseAddress,
  IN UINT32                     BitMask
  )
{
  UINT32    Reg32;
  UINT32    Status;
  UINT32    Command;

  Reg32 = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
  Status = (Reg32 & 0x96FFFFFF);       // Reset the one-shot bits
  Command = (Status & (~BitMask));
  MmioWrite32 (VtdUnitBaseAddress + R_GCMD_REG, Command);

  DEBUG ((DEBUG_INFO, "Clear GCMD_REG bits 0x%x.\n", BitMask));

  //
  // Poll on Status bit of Global status register to become zero
  //
  do {
    Reg32 = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
  } while ((Reg32 & BitMask) == BitMask);
  DEBUG ((DEBUG_INFO, "GSTS_REG : 0x%08x \n", Reg32));
}

/**
  Set Global Command Register Bits

  @param[in] VtdUnitBaseAddress     The base address of the VTd engine.
  @param[in] BitMask                Bit mask
**/
VOID
VtdLibSetGlobalCommandRegisterBits (
  IN UINTN                      VtdUnitBaseAddress,
  IN UINT32                     BitMask
  )
{
  UINT32    Reg32;
  UINT32    Status;
  UINT32    Command;

  Reg32 = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
  Status = (Reg32 & 0x96FFFFFF);       // Reset the one-shot bits
  Command = (Status | BitMask);
  MmioWrite32 (VtdUnitBaseAddress + R_GCMD_REG, Command);

  DEBUG ((DEBUG_INFO, "Set GCMD_REG bits 0x%x.\n", BitMask));

  //
  // Poll on Status bit of Global status register to become not zero
  //
  do {
    Reg32 = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
  } while ((Reg32 & BitMask) == 0);
  DEBUG ((DEBUG_INFO, "GSTS_REG : 0x%08x \n", Reg32));
}

/**
  Disable DMAR translation.

  @param[in] VtdUnitBaseAddress     The base address of the VTd engine.

  @retval EFI_SUCCESS               DMAR translation is disabled.
**/
EFI_STATUS
VtdLibDisableDmar (
  IN UINTN                      VtdUnitBaseAddress
  )
{
  UINT32                        Reg32;

  DEBUG ((DEBUG_INFO, ">>>>>>DisableDmar() for engine [%x]\n", VtdUnitBaseAddress));

  //
  // Write Buffer Flush before invalidation
  //
  VtdLibFlushWriteBuffer (VtdUnitBaseAddress);

  //
  // Disable Dmar
  //
  //
  // Set TE (Translation Enable: BIT31) of Global command register to zero
  //
  VtdLibClearGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_TE);

  //
  // Set SRTP (Set Root Table Pointer: BIT30) of Global command register in order to update the root table pointerDisable VTd
  //
  VtdLibSetGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_SRTP);

  Reg32 = MmioRead32 (VtdUnitBaseAddress + R_GSTS_REG);
  DEBUG ((DEBUG_INFO, "DisableDmar: GSTS_REG - 0x%08x\n", Reg32));

  MmioWrite64 (VtdUnitBaseAddress + R_RTADDR_REG, 0);

  DEBUG ((DEBUG_INFO,"VTD () Disabled!<<<<<<\n"));

  return EFI_SUCCESS;
}

/**
  Disable PMR.

  @param[in] VtdUnitBaseAddress     The base address of the VTd engine.

  @retval EFI_SUCCESS               PMR is disabled.
  @retval EFI_UNSUPPORTED           PMR is not supported.
  @retval EFI_NOT_STARTED           PMR was not enabled.
**/
EFI_STATUS
VtdLibDisablePmr (
  IN UINTN                      VtdUnitBaseAddress
  )
{
  UINT32        Reg32;
  VTD_CAP_REG   CapReg;
  EFI_STATUS    Status;

  CapReg.Uint64 = MmioRead64 (VtdUnitBaseAddress + R_CAP_REG);
  if (CapReg.Bits.PLMR == 0 || CapReg.Bits.PHMR == 0) {
    //
    // PMR is not supported
    //
    return EFI_UNSUPPORTED;
  }

  Reg32 = MmioRead32 (VtdUnitBaseAddress + R_PMEN_ENABLE_REG);
  if ((Reg32 & BIT0) != 0) {
    MmioWrite32 (VtdUnitBaseAddress + R_PMEN_ENABLE_REG, 0x0);
    do {
      Reg32 = MmioRead32 (VtdUnitBaseAddress + R_PMEN_ENABLE_REG);
    } while((Reg32 & BIT0) != 0);

    DEBUG ((DEBUG_INFO,"Pmr [0x%016lx] disabled\n", VtdUnitBaseAddress));
    Status = EFI_SUCCESS;
  } else {
    DEBUG ((DEBUG_INFO,"Pmr [0x%016lx] not enabled\n", VtdUnitBaseAddress));
    Status = EFI_NOT_STARTED;
  }
  return Status;
}

/**
  Disable queued invalidation interface.

  @param[in] VtdUnitBaseAddress     The base address of the VTd engine.
**/
VOID
VtdLibDisableQueuedInvalidationInterface (
  IN UINTN                      VtdUnitBaseAddress
  )
{
  QI_256_DESC    QiDesc;

  QiDesc.Uint64[0] = QI_IWD_TYPE;
  QiDesc.Uint64[1] = 0;
  QiDesc.Uint64[2] = 0;
  QiDesc.Uint64[3] = 0;

  VtdLibSubmitQueuedInvalidationDescriptor (VtdUnitBaseAddress, &QiDesc, TRUE);

  DEBUG ((DEBUG_INFO, "Disable Queued Invalidation Interface. [%x]\n", VtdUnitBaseAddress));
  VtdLibClearGlobalCommandRegisterBits (VtdUnitBaseAddress, B_GMCD_REG_QIE);

  MmioWrite64 (VtdUnitBaseAddress + R_IQA_REG, 0);
}

/**
  Submit the queued invalidation descriptor to the remapping
   hardware unit and wait for its completion.

  @param[in] VtdUnitBaseAddress     The base address of the VTd engine.
  @param[in] Desc                   The invalidate descriptor
  @param[in] ClearFaultBits         Clear Error bits

  @retval EFI_SUCCESS               The operation was successful.
  @retval EFI_INVALID_PARAMETER     Parameter is invalid.
  @retval EFI_NOT_READY             Queued invalidation is not inited.
  @retval EFI_DEVICE_ERROR          Detect fault, need to clear fault bits if ClearFaultBits is FALSE

**/
EFI_STATUS
VtdLibSubmitQueuedInvalidationDescriptor (
  IN UINTN                      VtdUnitBaseAddress,
  IN VOID                       *Desc,
  IN BOOLEAN                    ClearFaultBits
  )
{
  UINTN          QueueSize;
  UINTN          QueueTail;
  UINTN          QueueHead;
  QI_DESC        *Qi128Desc;
  QI_256_DESC    *Qi256Desc;
  VTD_IQA_REG    IqaReg;
  VTD_IQT_REG    IqtReg;
  VTD_IQH_REG    IqhReg;
  UINT32         FaultReg;
  UINT64         IqercdReg;
  UINT64         IQBassAddress;

  if (Desc == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  IqaReg.Uint64 = MmioRead64 (VtdUnitBaseAddress + R_IQA_REG);
  //
  // Get IQA_REG.IQA (Invalidation Queue Base Address)
  //
  IQBassAddress = RShiftU64 (IqaReg.Uint64, 12);
  if (IQBassAddress == 0) {
    DEBUG ((DEBUG_ERROR,"Invalidation Queue Buffer not ready [0x%lx]\n", IqaReg.Uint64));
    return EFI_NOT_READY;
  }
  IqtReg.Uint64 = MmioRead64 (VtdUnitBaseAddress + R_IQT_REG);

  //
  // Check IQA_REG.DW (Descriptor Width)
  //
  if ((IqaReg.Uint64 & BIT11) == 0) {
    //
    // 128-bit descriptor
    //
    QueueSize = (UINTN) (1 << (IqaReg.Bits.QS + 8));
    Qi128Desc = (QI_DESC *) (UINTN) LShiftU64 (IQBassAddress, VTD_PAGE_SHIFT);
    //
    // Get IQT_REG.QT for 128-bit descriptors
    //
    QueueTail = (UINTN) (RShiftU64 (IqtReg.Uint64, 4) & 0x7FFF);
    Qi128Desc += QueueTail;
    CopyMem (Qi128Desc, Desc, sizeof (QI_DESC));
    QueueTail = (QueueTail + 1) % QueueSize;

    DEBUG ((DEBUG_VERBOSE, "[0x%x] Submit QI Descriptor 0x%x [0x%016lx, 0x%016lx]\n",
            VtdUnitBaseAddress,
            QueueTail,
            Qi128Desc->Low,
            Qi128Desc->High));

    IqtReg.Uint64 &= ~(0x7FFF << 4);
    IqtReg.Uint64 |= LShiftU64 (QueueTail, 4);
  } else {
    //
    // 256-bit descriptor
    //
    QueueSize = (UINTN) (1 << (IqaReg.Bits.QS + 7));
    Qi256Desc = (QI_256_DESC *) (UINTN) LShiftU64 (IQBassAddress, VTD_PAGE_SHIFT);
    //
    // Get IQT_REG.QT for 256-bit descriptors
    //
    QueueTail = (UINTN) (RShiftU64 (IqtReg.Uint64, 5) & 0x3FFF);
    Qi256Desc += QueueTail;
    CopyMem (Qi256Desc, Desc, sizeof (QI_256_DESC));
    QueueTail = (QueueTail + 1) % QueueSize;

    DEBUG ((DEBUG_VERBOSE, "[0x%x] Submit QI Descriptor 0x%x [0x%016lx, 0x%016lx, 0x%016lx, 0x%016lx]\n",
            VtdUnitBaseAddress,
            QueueTail,
            Qi256Desc->Uint64[0],
            Qi256Desc->Uint64[1],
            Qi256Desc->Uint64[2],
            Qi256Desc->Uint64[3]));

    IqtReg.Uint64 &= ~(0x3FFF << 5);
    IqtReg.Uint64 |= LShiftU64 (QueueTail, 5);
  }

  //
  // Update the HW tail register indicating the presence of new descriptors.
  //
  MmioWrite64 (VtdUnitBaseAddress + R_IQT_REG, IqtReg.Uint64);

  do {
    FaultReg = MmioRead32 (VtdUnitBaseAddress + R_FSTS_REG);
    if (FaultReg & (B_FSTS_REG_IQE | B_FSTS_REG_ITE | B_FSTS_REG_ICE)) {
      IqercdReg = MmioRead64 (VtdUnitBaseAddress + R_IQERCD_REG);
      DEBUG((DEBUG_ERROR, "BAR [0x%016lx] Detect Queue Invalidation Fault [0x%08x] - IQERCD [0x%016lx]\n", VtdUnitBaseAddress, FaultReg, IqercdReg));
      if (ClearFaultBits) {
        FaultReg &= (B_FSTS_REG_IQE | B_FSTS_REG_ITE | B_FSTS_REG_ICE);
        MmioWrite32 (VtdUnitBaseAddress + R_FSTS_REG, FaultReg);
      }
      return EFI_DEVICE_ERROR;
    }

    IqhReg.Uint64 = MmioRead64 (VtdUnitBaseAddress + R_IQH_REG);
    //
    // Check IQA_REG.DW (Descriptor Width) and get IQH_REG.QH
    //
    if ((IqaReg.Uint64 & BIT11) == 0) {
      QueueHead = (UINTN) (RShiftU64 (IqhReg.Uint64, 4) & 0x7FFF);
    } else {
      QueueHead = (UINTN) (RShiftU64 (IqhReg.Uint64, 5) & 0x3FFF);
    }
  } while (QueueTail != QueueHead);

  return EFI_SUCCESS;
}
