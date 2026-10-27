/** @file

  Marvell CN10K ACPI platform driver.

  Reads the ACPI template tables from the platform firmware volume, patches
  the platform-specific information from the configuration device tree
  (interrupts, SPI lock ranges, RVU mailbox range, cache sizes, error records,
  and board selection), and installs the resulting tables through the ACPI
  table protocol.

  Copyright (c) 2025-2026 Marvell. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Protocol/AcpiTable.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/FdtClient.h>

#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>

#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/MemoryMappedConfigurationSpaceAccessTable.h>
#include <IndustryStandard/AcpiAml.h>

#include <Library/CN10KConfigLib.h>
#include <Library/AmlLib/AmlLib.h>

#include <../../AcpiTables/Mpam.h>

#define GET_CPU_MPID(NodeId, CoreId)  (((NodeId) << 24) | ((CoreId) << 16))
#define GIC_SPI_IRQ_BASE  32

CHAR8                     mProductName[32]; // 32 bytes should be enough to store product name
UINT64                    mOemTableId;
MRVL_FDT_CLIENT_PROTOCOL  *mFdtClient;

typedef struct {
  UINT8    Name[4];
  UINT8    Cmd[3];
  UINT8    Status;
} AML_METHOD;

#pragma pack(1)
typedef struct {
  UINT8     Id;
  UINT16    Length;
} ADDR_SPACE_DESC;

typedef struct {
  UINT8     Desc;
  UINT16    Length;
  UINT8     Type;
  UINT8     GeneralFlag;
  UINT8     SPecificFlag;
  UINT64    Granularity;
  UINT64    MinAddress;
  UINT64    MaxAddress;
  UINT64    TranslationOffset;
  UINT64    AddressLength;
} QWORD_MEM;
#pragma pack()

#define ACPI_NAME_SIZE  4
#define MAX_SPI_CTRLS   2

/**
  Log the signature and length of an ACPI table.

  @param[in]  Table  Table header to describe.
**/
STATIC
VOID
DescribeTable (
  IN EFI_ACPI_DESCRIPTION_HEADER  *Table
  )
{
  DEBUG ((
    DEBUG_INFO,
    "ACPI table signature: %c%c%c%c\n",
    (Table->Signature & 0x000000FF),
    (Table->Signature & 0x0000FF00) >> 8,
    (Table->Signature & 0x00FF0000) >> 16,
    (Table->Signature & 0xFF000000) >> 24
    ));
  DEBUG ((
    DEBUG_INFO,
    "Table length: %d\n",
    Table->Length
    ));
}

/**
  Dump the raw bytes of an ACPI table to the debug output.

  Compiled in only when EFI_DEBUG is defined.

  @param[in]  Table  Table to dump.
**/
STATIC
VOID
DumpTable (
  IN EFI_ACPI_DESCRIPTION_HEADER  *Table
  )
{
 #ifdef  EFI_DEBUG
  INTN   Index;
  UINT8  *Buffer = (VOID *)Table;

  DEBUG ((DEBUG_INFO, "\n===========================\n"));

  for (Index = 0; Index < Table->Length; Index++) {
    DEBUG ((
      DEBUG_INFO,
      "%02x%s",
      Buffer[Index],
      (Index + 1) & 0xF ? L" " : L"\r\n"
      ));
  }

  DEBUG ((DEBUG_INFO, "\n===========================\n"));
 #endif
}

/* this structure describes common header for ACPI MADT table entries */
typedef struct {
  UINT8    Type;
  UINT8    Length;
} PACKED ACPI_APIC_ENTRY_HDR;

/**
  Locate the firmware volume that contains the ACPI table storage file.

  Enumerates all firmware volume protocol instances and returns the first one
  from which the file identified by PcdAcpiTableStorageFile can be opened.

  @param[out]  Instance  Receives the matching firmware volume instance.

  @retval  EFI_SUCCESS           A firmware volume with the storage file was
                                 found.
  @retval  EFI_NOT_FOUND         No firmware volume contains the file.
  @retval  EFI_OUT_OF_RESOURCES  The handle buffer could not be allocated.

**/
STATIC
EFI_STATUS
LocateFvInstanceWithTables (
  OUT EFI_FIRMWARE_VOLUME2_PROTOCOL  **Instance
  )
{
  EFI_STATUS                     Status;
  EFI_HANDLE                     *HandleBuffer;
  UINTN                          NumberOfHandles;
  EFI_FV_FILETYPE                FileType;
  UINT32                         FvStatus;
  EFI_FV_FILE_ATTRIBUTES         Attributes;
  UINTN                          Size;
  UINTN                          Index;
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *FvInstance;

  FvStatus = 0;

  //
  // Locate protocol.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareVolume2ProtocolGuid,
                  NULL,
                  &NumberOfHandles,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    //
    // Defined errors at this time are not found and out of resources.
    //
    return Status;
  }

  //
  // Looking for FV with ACPI storage file
  //

  for (Index = 0; Index < NumberOfHandles; Index++) {
    //
    // Get the protocol on this handle
    // This should not fail because of LocateHandleBuffer
    //
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID **)&FvInstance
                    );
    ASSERT_EFI_ERROR (Status);

    //
    // See if it has the ACPI storage file
    //
    Status = FvInstance->ReadFile (
                           FvInstance,
                           (EFI_GUID *)PcdGetPtr (PcdAcpiTableStorageFile),
                           NULL,
                           &Size,
                           &FileType,
                           &Attributes,
                           &FvStatus
                           );

    //
    // If we found it, then we are done
    //
    if (Status == EFI_SUCCESS) {
      *Instance = FvInstance;
      break;
    }
  }

  //
  // Our exit status is determined by the success of the previous operations
  // If the protocol was found, Instance already points to it.
  //

  //
  // Free any allocated buffers
  //
  gBS->FreePool (HandleBuffer);

  return Status;
}

/**
  Find a byte pattern in a memory buffer.

  @param[in]   StartPoint      Start of the buffer to search.
  @param[in]   MaxLength       Size of the buffer in bytes.
  @param[in]   StringToFind    Byte pattern to look for.
  @param[in]   StringToFindLen Length of the pattern in bytes.
  @param[out]  Location        Receives the address of the first match.

  @retval  EFI_SUCCESS       The pattern was found.
  @retval  EFI_DEVICE_ERROR  The pattern was not found.
**/
STATIC
EFI_STATUS
FindStringLocation (
  IN  VOID   *StartPoint,
  IN  UINTN  MaxLength,
  IN  CHAR8  *StringToFind,
  IN  UINTN  StringToFindLen,
  OUT UINTN  *Location
  )
{
  UINTN  Index;
  UINT8  *Start;

  Start = (UINT8 *)StartPoint;

  for (Index = 0; Index < (MaxLength - StringToFindLen); Index++) {
    if (CompareMem (Start + Index, StringToFind, StringToFindLen) == 0) {
      *Location = (UINTN)Start + Index;
      return EFI_SUCCESS;
    }
  }

  return EFI_DEVICE_ERROR;
}

/**
  Find a device name in the DSDT.

  @param[in]   DeviceName  Four-character ACPI name of the device.
  @param[in]   DsdtTable   DSDT buffer to search.
  @param[in]   TableSize   Size of the DSDT buffer in bytes.
  @param[out]  Offset      Receives the address of the name in the buffer.

  @retval  EFI_SUCCESS       The device was found.
  @retval  EFI_DEVICE_ERROR  The device was not found.
**/
STATIC
EFI_STATUS
FindDevice (
  IN  CHAR8  *DeviceName,
  IN  VOID   *DsdtTable,
  IN  UINTN  TableSize,
  OUT UINTN  *Offset
  )
{
  return FindStringLocation (DsdtTable, TableSize, DeviceName, ACPI_NAME_SIZE, Offset);
}

/**
  Find a method name in the DSDT, starting at its parent device.

  @param[in]   MethodName    Four-character ACPI method name.
  @param[in]   ParentOffset  Address of the parent device in the buffer.
  @param[in]   TableSize     Size of the DSDT buffer in bytes.
  @param[out]  Offset        Receives the address of the name in the buffer.

  @retval  EFI_SUCCESS       The method was found.
  @retval  EFI_DEVICE_ERROR  The method was not found.
**/
STATIC
EFI_STATUS
FindMethod (
  IN  CHAR8  *MethodName,
  IN  UINTN  ParentOffset,
  IN  UINTN  TableSize,
  OUT UINTN  *Offset
  )
{
  return FindStringLocation ((VOID *)ParentOffset, TableSize, MethodName, ACPI_NAME_SIZE, Offset);
}

/**
  Find an object name in the DSDT, starting at its parent device.

  @param[in]   ObjectName    Four-character ACPI object name.
  @param[in]   ParentOffset  Address of the parent device in the buffer.
  @param[in]   TableSize     Size of the DSDT buffer in bytes.
  @param[out]  Offset        Receives the address of the name in the buffer.

  @retval  EFI_SUCCESS       The object was found.
  @retval  EFI_DEVICE_ERROR  The object was not found.
**/
STATIC
EFI_STATUS
FindName (
  IN  CHAR8  *MethodName,
  IN  UINTN  ParentOffset,
  IN  UINTN  TableSize,
  OUT UINTN  *Offset
  )
{
  return FindStringLocation ((VOID *)ParentOffset, TableSize, MethodName, ACPI_NAME_SIZE, Offset);
}

/**
  Find the extended interrupt descriptor of a _CRS resource template.

  Only supports an Extended Interrupt descriptor containing a single interrupt
  number; the descriptor is matched by its three-byte prefix.

  @param[in]   ParentOffset         Address of the parent object in the DSDT.
  @param[in]   TableSize            Size of the DSDT buffer in bytes.
  @param[out]  InterruptDescOffset  Receives the address of the descriptor.

  @retval  EFI_SUCCESS       The descriptor was found.
  @retval  EFI_DEVICE_ERROR  The descriptor was not found.
**/
STATIC
EFI_STATUS
FindInterruptDescriptor (
  IN  UINTN  ParentOffset,
  IN  UINTN  TableSize,
  OUT UINTN  *InterruptDescOffset
  )
{
  CHAR8  Name[] = { ACPI_LARGE_EXTENDED_IRQ_DESCRIPTOR_NAME | (ACPI_LARGE_ITEM_FLAG << 7), 0x06, 0x00 };

  return FindStringLocation ((VOID *)ParentOffset, TableSize, Name, 3, InterruptDescOffset);
}

/**
  Replace the interrupt number in an extended interrupt descriptor.

  Only the first interrupt number is updated; the descriptor must contain a
  single 32-bit interrupt. Interrupt flags are not changed by this function.

  @param[in]  InterruptDescOffset  Address of the descriptor in the DSDT.
  @param[in]  NewInterruptNumber   Interrupt number to write.

  @retval  EFI_SUCCESS  The interrupt number was updated.
**/
STATIC
EFI_STATUS
ChangeInterruptNumber (
  IN  UINTN   InterruptDescOffset,
  IN  UINT32  NewInterruptNumber
  )
{
  EFI_ACPI_EXTENDED_INTERRUPT_DESCRIPTOR  *Intr = (EFI_ACPI_EXTENDED_INTERRUPT_DESCRIPTOR *)InterruptDescOffset;

  Intr->InterruptNumber[0] = NewInterruptNumber;
  return EFI_SUCCESS;
}

/**
  Update the trigger and polarity flags of an extended interrupt descriptor.

  The device tree interrupt mode is translated as follows: 1 = edge rising,
  2 = edge falling, 4 = level high, 8 = level low. The consumer/shared flag
  in the descriptor is preserved.

  @param[in]  InterruptDescOffset  Address of the descriptor in the DSDT.
  @param[in]  InterruptMode        Device tree interrupt mode value.

  @retval  EFI_SUCCESS  The flags were updated.
**/
STATIC
EFI_STATUS
ChangeInterruptMode (
  IN  UINTN   InterruptDescOffset,
  IN  UINT32  InterruptMode
  )
{
  EFI_ACPI_EXTENDED_INTERRUPT_DESCRIPTOR  *Intr   = (EFI_ACPI_EXTENDED_INTERRUPT_DESCRIPTOR *)InterruptDescOffset;
  UINT8                                   IntFlag = 0;

  /* InterruptMode = 1 EDGE RISING, 2 EDGE FALLING, 4 LEVEL HIGH, 8 LEVEL LOW */

  /* InterruptVectorFlags = ResourceConsumer ? BIT0 : 0
                            EdgeTriggered ? BIT1 : 0
                            ActiveLow ? BIT2 : 0
                            Shared ? BIT3 : 0 */
  switch (InterruptMode) {
    case 1:
      IntFlag = BIT1;
      break;
    case 2:
      IntFlag = BIT1 | BIT2;
      break;
    case 4:
      IntFlag = 0;
      break;
    case 8:
      IntFlag = BIT2;
      break;
  }

  Intr->InterruptVectorFlags &= ~(BIT1 | BIT2);
  Intr->InterruptVectorFlags |= IntFlag;

  return EFI_SUCCESS;
}

/**
  Replace the return value of a _STA method in the DSDT.

  @param[in]  MethodOffset  Address of the _STA method in the DSDT.
  @param[in]  NewStatus     Status value to store in the method body.

  @retval  EFI_SUCCESS  The status was updated.
**/
STATIC
EFI_STATUS
ChangeDeviceStatus (
  IN  UINTN  MethodOffset,
  IN  UINT8  NewStatus
  )
{
  AML_METHOD  *Method = (AML_METHOD *)MethodOffset;

  Method->Status = NewStatus;
  return EFI_SUCCESS;
}

/**
  Update a device interrupt in the DSDT from its device tree entry.

  Finds the device and its _CRS method, locates the single extended interrupt
  descriptor, and replaces the interrupt number and trigger/polarity flags with
  the values from the device tree interrupts property.

  @param[in]  DeviceName       Four-character ACPI device name.
  @param[in]  DsdtTableBuffer  DSDT buffer to patch.
  @param[in]  DsdtTableSize    Size of the DSDT buffer in bytes.
  @param[in]  NodeLoc          Device tree interrupts property: type, number,
                               and mode cells.

  @retval  EFI_SUCCESS  The interrupt was updated.
  @retval  Others       The device, _CRS, or interrupt descriptor was not
                        found.
**/
STATIC
EFI_STATUS
UpdateDeviceInterruptInfo (
  IN  CHAR8   *DeviceName,
  IN  VOID    *DsdtTableBuffer,
  IN  UINT32  DsdtTableSize,
  IN  UINT64  *NodeLoc
  )
{
  EFI_STATUS  Status;
  UINTN       DevLocation, CrsLocation, IntrLocation;
  UINT32      *IntInfo, NewInterruptNumber;

  Status = FindDevice (DeviceName, DsdtTableBuffer, DsdtTableSize, &DevLocation);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: %a not found in DSDT\n", __FUNCTION__, DeviceName));
    return Status;
  }

  DEBUG ((DEBUG_VERBOSE, "Found %a in DSDT @ 0x%llx\n", DeviceName, DevLocation));

  /* Locate _CRS within Device */
  // AsciiSPrint (DeviceName, sizeof (NodeName), "_CRS");
  Status = FindName ("_CRS", DevLocation, DsdtTableSize, &CrsLocation);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: %a._CRS not found\n", __FUNCTION__, DeviceName));
    return Status;
  }

  DEBUG ((DEBUG_VERBOSE, "Found %a._CRS in DSDT @ 0x%llx\n", DeviceName, CrsLocation));

  /* Locate Interrupt resource */
  Status = FindInterruptDescriptor (CrsLocation, DsdtTableSize, &IntrLocation);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: %a._CRS Interrupt not found\n", __FUNCTION__, DeviceName));
    return Status;
  }

  DEBUG ((DEBUG_VERBOSE, "Found %a._CRS Interrupt in DSDT @ 0x%llx\n", DeviceName, IntrLocation));

  /* Change Interrupt info */
  /* IntInfo[0] = Zero - SPI OR One - PPI */
  /* IntInfo[1] = GIC interrupt number */
  /* IntInfo[2] = 1 EDGE RISING, 2 EDGE FALLING, 4 LEVEL HIGH, 8 LEVEL LOW */
  IntInfo = (UINT32 *)NodeLoc;

  NewInterruptNumber = FdtToCpu32 (IntInfo[1]) + GIC_SPI_IRQ_BASE;
  Status             = ChangeInterruptNumber (IntrLocation, NewInterruptNumber);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: %a._CRS Interrupt number not changed to 0x%llx\n", __FUNCTION__, DeviceName, NewInterruptNumber));
    return Status;
  }

  Status = ChangeInterruptMode (IntrLocation, FdtToCpu32 (IntInfo[2]));
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: %a._CRS Interrupt mode not changed to 0x%llx\n", __FUNCTION__, DeviceName, IntInfo[2]));
    return Status;
  }

  return Status;
}

/* Find 'lock' QWordMemory resource in SPI _CRS and update the MIN/MAX/LEN.
 * Consult Dsdt.asl for the resources layout and adjust below resource number accordingly.
 * The resource number is the number of resources before the 'lock' resource.
 */
#define SPI_LOCK_RESOURCE_NO  4 // 'lock' resource is the 5th one: io, sdma, aux, xfer, lock

/**
  Update the SPI lock QWordMemory resource in a SPI controller _CRS.

  Parses the DSDT, finds the _CRS object of the SPI controller, selects the
  QWordMemory resource that describes the lock register (the fifth resource in
  the template), replaces its minimum, maximum, and length fields, and
  serializes the modified tree back into the DSDT buffer.

  @param[in]  Idx              SPI controller index.
  @param[in]  DsdtTableBuffer  DSDT buffer to patch in place.
  @param[in]  DsdtTableSize    Size of the DSDT buffer in bytes.
  @param[in]  SpiLockBase      New lock register base address.
  @param[in]  SpiLockBaseSize  Size of the lock register region.

  @retval  EFI_SUCCESS  The resource was updated.
  @retval  Others       The node, resource, or AML operation failed.
**/
STATIC
EFI_STATUS
UpdateSpiLockInfo (
  IN  UINT8   Idx,
  IN  VOID    *DsdtTableBuffer,
  IN  UINT32  DsdtTableSize,
  IN  UINT64  SpiLockBase,
  IN  UINT64  SpiLockBaseSize
  )
{
  EFI_STATUS  Status;
  CHAR8       NodeName[] = "\\_SB_.SPI0._CRS"; // Inital value to setup correct size of NodeName

  EFI_ACPI_DESCRIPTION_HEADER  *Table         = NULL;
  AML_ROOT_NODE_HANDLE         RootNodeHandle = NULL;
  AML_DATA_NODE_HANDLE         QWordFirst     = NULL;
  AML_DATA_NODE_HANDLE         QWordNext;
  AML_OBJECT_NODE_HANDLE       Crs;
  UINT8                        Index;

  // Parse the DSDT table to get the RootNodeHandle.
  Status = AmlParseDefinitionBlock (DsdtTableBuffer, &RootNodeHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AmlParseDefinitionBlock Status = %r\n", Status));
    return Status;
  }

  // Find the _CRS object of device defined by NodeName
  AsciiSPrint (NodeName, sizeof (NodeName), "\\_SB_.SPI%d._CRS", Idx);
  Status = AmlFindNode (RootNodeHandle, NodeName, &Crs);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AmlFindNode Status = %r\n", Status));
    goto error;
  }

  // Get the first resource in the "_CRS" object.
  Status = AmlNameOpGetFirstRdNode (Crs, &QWordFirst);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AmlNameOpGetFirstRdNode Status = %r\n", Status));
    goto error;
  }

  if (QWordFirst == NULL) {
    Status = EFI_NOT_FOUND;
    goto error;
  }

 #if SPI_LOCK_RESOURCE_NO // If SPI_LOCK_RESOURCE_NO != 0, we need to loop through the resources
  for (Index = 0; Index < SPI_LOCK_RESOURCE_NO; Index++) {
    // Get the next resource in the "_CRS" object.
    // This should be next QWordMemory
    QWordNext = NULL;
    Status    = AmlNameOpGetNextRdNode (QWordFirst, &QWordNext);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "AmlNameOpGetNextRdNode Status = %r\n", Status));
      goto error;
    }

    if (QWordNext == NULL) {
      Status = EFI_NOT_FOUND;
      goto error;
    }

    QWordFirst = QWordNext;
  }

 #else // SPI_LOCK_RESOURCE_NO = 0, so we don't need to loop
  QWordNext = QWordFirst;   // QWordNext is the first resource
 #endif

  DEBUG ((DEBUG_INFO, "%a: Found 'lock' resource of %a in DSDT \n", __FUNCTION__, NodeName));

  // Update MIN/MAX/LEN
  Status = AmlUpdateRdQWord (QWordNext, SpiLockBase, SpiLockBaseSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AmlUpdateRdQWord Status = %r\n", Status));
    goto error;
  }

  // Serialize the tree to a table.
  Status = AmlSerializeDefinitionBlock (RootNodeHandle, &Table);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AmlSerializeTree Status = %r\n", Status));
  } else {
    // Update original DSDT table with updated table
    CopyMem (DsdtTableBuffer, Table, DsdtTableSize);
    FreePool (Table); // Free the table allocated by AmlSerializeDefinitionBlock
  }

error:
  // Cleanup
  if (RootNodeHandle != NULL) {
    if (AmlDeleteTree (RootNodeHandle)) {
      DEBUG ((DEBUG_ERROR, "Failed to cleanup AML tree\n"));
      // If till this point everything was successful, but code just
      // failed to delete the AML Tree, return EFI_SUCCESS anyway
    }
  }

  DEBUG ((DEBUG_INFO, "%a: Status %r\n", __FUNCTION__, Status));
  return Status;
}

/**
  Update the RVU mailbox range in the DSDT from the device tree.

  ATF allocates the RVU mailbox in DDR and publishes its address and size in
  the ranges property of the ECAM2 PCI node. The function selects the DDR
  window (bit 47 clear in the CPU address), finds the PCI2._PRS method in the
  DSDT, and rewrites the matching QWordMemory descriptor.

  @param[in]  DsdtTableBuffer  DSDT buffer to patch in place.
  @param[in]  DsdtTableSize    Size of the DSDT buffer in bytes.

  @retval  EFI_SUCCESS    The mailbox range was updated.
  @retval  EFI_NOT_FOUND  The FDT client, ECAM2 node, ranges property, PCI2
                          device, or _PRS method was not found.
**/
STATIC
EFI_STATUS
UpdateRvuMailboxRange (
  IN  VOID    *DsdtTableBuffer,
  IN  UINT32  DsdtTableSize
  )
{
  EFI_STATUS       Status;
  FDT_HANDLE       Ecam2Handle;
  UINT32           *PhyAddress;
  UINT32           Length, Idx;
  UINT32           AddressHi, AddressLow, SizeLow;
  UINTN            DevLocation, MethodLocation;
  ADDR_SPACE_DESC  *AddrSpaceDescriptor;
  UINT8            *DescPtr;
  QWORD_MEM        *QwordDesc;

  /* Find mailbox ranges from device tree */
  if (mFdtClient == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: FDT Client protocol not found\n", __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  /* Get pointer to ecam2 node */
  Status = mFdtClient->GetNode (mFdtClient, "/soc@0/pci@878020000000", &Ecam2Handle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Ecam2 node not found\n", __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  /* Get location of ranges property */
  Status = mFdtClient->GetNodeProperty (mFdtClient, Ecam2Handle, "ranges", (CONST VOID **)&PhyAddress, &Length);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Ecam2 ranges device tree property not found \n", __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  /* Extract mailbox address and size */

  /* Ranges property is made up of three values as below */
  /* ranges = <pci-address cpu-address size> */
  /* pci-address is made up of 3 cells; bdf value, address high, address low */
  /* cpu-address is made up of 2 cells; address high, address low */
  /* size is made up of 2 cells; size high, size low */
  /* each cell is 32-bits, (3+2+2) * 4 = 28 bytes for each range entry */
  #define RANGE_ENTRY_CELLS  7
  #define PCI_ADDRESS_CELLS  3
  #define CPU_ADDRESS_CELLS  2
  /* RVU mailbox is in DDR, so look for address that is not in IO range, bit 47 is clear */
  for (Idx = 0; Idx < (Length/28); Idx++) {
    AddressHi = FdtToCpu32 (
                  *(UINT32 *)(PhyAddress + RANGE_ENTRY_CELLS * Idx +
                              PCI_ADDRESS_CELLS)
                  );
    if ((AddressHi & (1 << 15)) == 0) {
      break;
    }
  }

  AddressLow = FdtToCpu32 (
                 *(UINT32 *)(PhyAddress + RANGE_ENTRY_CELLS * Idx +
                             PCI_ADDRESS_CELLS + 1)
                 );
  SizeLow = FdtToCpu32 (
              *(UINT32 *)(PhyAddress + RANGE_ENTRY_CELLS * Idx +
                          PCI_ADDRESS_CELLS + CPU_ADDRESS_CELLS + 1)
              );

  /* update DSDT */
  /* Find DEVICE(PCI2) */
  Status = FindDevice ("PCI2", DsdtTableBuffer, DsdtTableSize, &DevLocation);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: PCI2 not found\n", __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  /* Find _PRS method */
  Status = FindMethod ("_PRS", DevLocation, DsdtTableSize, &MethodLocation);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: PCI2._PRS not found\n", __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  #define QWORD_SMALL_DESC_ID  0x8A
  #define QWORD_LARGE_DESC_ID  0x0A
  /* Look at address space descriptors */
  DescPtr  = (UINT8 *)MethodLocation;
  DescPtr += 4; // skip over '_PRS' string
  DescPtr += 7; // skip over unknown bytes****
  /* Loop through descriptors, max 20 limit to avoid hang */
  for (Idx = 0; Idx < 10; Idx++) {
    AddrSpaceDescriptor = (ADDR_SPACE_DESC *)DescPtr;
    if ((AddrSpaceDescriptor->Id == QWORD_SMALL_DESC_ID) ||
        (AddrSpaceDescriptor->Id == QWORD_LARGE_DESC_ID))
    {
      QwordDesc = (QWORD_MEM *)DescPtr;
      /* Look for default values from DSDT */
      if ((QwordDesc->MinAddress == 0x1000000) &&
          (QwordDesc->MaxAddress == 0x37FFFFF))
      {
        break;
      }
    }

    DescPtr += sizeof (ADDR_SPACE_DESC) + AddrSpaceDescriptor->Length;
  }

  QwordDesc->MinAddress    = AddressHi;
  QwordDesc->MinAddress    = (QwordDesc->MinAddress << 32) | AddressLow;
  QwordDesc->MaxAddress    = QwordDesc->MinAddress + SizeLow -1;
  QwordDesc->AddressLength = SizeLow;
  DEBUG ((
    DEBUG_VERBOSE,
    "%a: QwordDesc: 0x%lx 0x%lx 0x%lx\n",
    __FUNCTION__,
    QwordDesc->MinAddress,
    QwordDesc->MaxAddress,
    QwordDesc->AddressLength
    ));

  return EFI_SUCCESS;
}

/**
  Update all SPI controller information in the DSDT from the device tree.

  For every spiX node found in the device tree, updates the controller
  interrupt number and mode in the DSDT and, when ATF published a lock range
  in the reg property, updates the lock QWordMemory resource. Missing nodes or
  properties are skipped.

  @param[in]  DsdtTableBuffer  DSDT buffer to patch in place.
  @param[in]  DsdtTableSize    Size of the DSDT buffer in bytes.

  @retval  EFI_SUCCESS  Always returned so that the table is still installed
                        when individual fixups are skipped.
**/
STATIC
EFI_STATUS
UpdateSpiControllerInfo (
  IN  VOID    *DsdtTableBuffer,
  IN  UINT32  DsdtTableSize
  )
{
  EFI_STATUS    Status;
  UINT8         Idx;
  FDT_HANDLE    SpiNorHandle;
  CHAR8         NodeName[80];
  CONST UINT64  *RegProperty;
  UINT32        RegSize;
  UINT64        SpiLockBase;
  UINT64        SpiLockBaseSize;
  UINT64        *PhyAddress;

  /* Retrieve SPI-NOR data from DT and update DSDT table node */
  ASSERT (mFdtClient != NULL);
  for (Idx = 0; Idx < MAX_SPI_CTRLS; Idx++) {
    AsciiSPrint (NodeName, sizeof (NodeName), "spi%d", Idx);
    Status = mFdtClient->GetNode (mFdtClient, NodeName, &SpiNorHandle);
    if (EFI_ERROR (Status)) {
      continue;
    }

    DEBUG ((
      DEBUG_VERBOSE,
      "%a: Found %a handle: 0x%x\n",
      __FUNCTION__,
      NodeName,
      SpiNorHandle
      ));

    Status = mFdtClient->GetNodeProperty (mFdtClient, SpiNorHandle, "interrupts", (CONST VOID **)&PhyAddress, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: SPI%d interrupts device tree property not found \n", __FUNCTION__, Idx));
      continue;
    }

    /* Update SPIx controller interrupt info in DSDT table */
    AsciiSPrint (NodeName, sizeof (NodeName), "SPI%d", Idx);

    Status = UpdateDeviceInterruptInfo (NodeName, DsdtTableBuffer, DsdtTableSize, PhyAddress);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: SPI%d._CRS Interrupt update failed\n", __FUNCTION__, Idx));
    }

    // Find SPI Lock Base and Size info in DT
    Status = mFdtClient->GetNodeProperty (mFdtClient, SpiNorHandle, "reg", (CONST VOID **)&RegProperty, &RegSize);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: SPI%d reg device tree property not found \n", __FUNCTION__, Idx));
      continue;
    }

    // Check for outdated DT which does NOT have lock base/size defined under SPI controller.
    if (RegSize < 80) {
      // 80 bytes = 20 * reg entry (each has 4 bytes)
      DEBUG ((DEBUG_INFO, "%a: Lock base not in DT, so leave defaults\n", __FUNCTION__));
      continue;
    }

    // See SPI_0_LOCK_DATA_BASE and SPI_1_LOCK_DATA_BASE written to DT by ATF.
    if (!FdtToCpu64 (RegProperty[8]) || !FdtToCpu64 (RegProperty[9])) {
      // reg[16]/[17]/[18]/[19]
      DEBUG ((DEBUG_INFO, "%a: Improper lock base/size, so leave defaults\n", __FUNCTION__));
      continue;
    }

    // Lock base and size defined in DT. RegProperty is UINT64 and reg is UINT32
    SpiLockBase     = FdtToCpu64 (RegProperty[8]);
    SpiLockBaseSize = FdtToCpu64 (RegProperty[9]);

    // Update SPI Lock Base and Size info in DSDT table
    DEBUG ((DEBUG_INFO, "%a: lock @%p size 0x%llx\n", __FUNCTION__, SpiLockBase, SpiLockBaseSize));
    Status = UpdateSpiLockInfo (Idx, DsdtTableBuffer, DsdtTableSize, SpiLockBase, SpiLockBaseSize);
  } // for

  // Regardless of previous results always return EFI_SUCCESS,
  // so that ACPI table is installed even if some values were maybe not properly updated.
  return EFI_SUCCESS;
}

/**
  Disable the PCI1 controller in the DSDT when requested.

  When PcdDisablePci1Bus is TRUE, locates the PCI1 device and its _STA method
  and sets the status to zero. This workaround allows Windows to boot on
  configurations where PCI1 cannot be used.

  @param[in]  DsdtTableBuffer  DSDT buffer to patch in place.
  @param[in]  DsdtTableSize    Size of the DSDT buffer in bytes.

  @retval  EFI_SUCCESS  The controller was disabled.
  @retval  Others       The PCI1 device or _STA method could not be modified.
**/
STATIC
EFI_STATUS
DisablePci1Controller (
  IN  VOID    *DsdtTableBuffer,
  IN  UINT32  DsdtTableSize
  )
{
  EFI_STATUS  Status;
  UINTN       DevLocation, MethodLocation;

  /* Disable PCI1 if set by user */
  if (PcdGetBool (PcdDisablePci1Bus) == FALSE) {
    return EFI_SUCCESS;
  }

  Status = FindDevice ("PCI1", DsdtTableBuffer, DsdtTableSize, &DevLocation);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: PCI1 not found\n", __FUNCTION__));
    goto END;
  }

  Status = FindMethod ("_STA", DevLocation, DsdtTableSize, &MethodLocation);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: PCI1._STA not found\n", __FUNCTION__));
    goto END;
  }

  Status = ChangeDeviceStatus (MethodLocation, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: PCI1._STA set to zero\n", __FUNCTION__));
    goto END;
  }

END:
  return Status;
}

/**
  Update the eMMC interrupt in the DSDT from the device tree.

  Reads the interrupts property of the CN10K SDHCI node and updates the EMMC
  device interrupt number and mode in the DSDT.

  @param[in]  DsdtTableBuffer  DSDT buffer to patch in place.
  @param[in]  DsdtTableSize    Size of the DSDT buffer in bytes.

  @retval  EFI_SUCCESS  The interrupt was updated.
  @retval  Others       The SDHCI node, its interrupts property, or the EMMC
                        DSDT device was not found.
**/
STATIC
EFI_STATUS
UpdateEmmcInterruptInfo (
  IN  VOID    *DsdtTableBuffer,
  IN  UINT32  DsdtTableSize
  )
{
  EFI_STATUS  Status;
  FDT_HANDLE  EmmcHandle;
  CHAR8       NodeName[80];
  // UINT64      EmmcInterrupt;
  UINT64  *PhyAddress;

  /* Retrieve EMMC interrupt numbers and update DSDT table node */
  ASSERT (mFdtClient != NULL);
  AsciiSPrint (NodeName, sizeof (NodeName), "/soc@0/sdhci@824000000000");
  Status = mFdtClient->GetNode (mFdtClient, NodeName, &EmmcHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: Found %a handle: 0x%x\n",
    __FUNCTION__,
    NodeName,
    EmmcHandle
    ));
  Status = mFdtClient->GetNodeProperty (mFdtClient, EmmcHandle, "interrupts", (CONST VOID **)&PhyAddress, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: EMMC interrupts device tree property not found \n", __FUNCTION__));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "%a: interrupts address:0x%llx\n", __FUNCTION__, PhyAddress));

  /* Update EMMC interrupt info in DSDT table */
  AsciiSPrint (NodeName, sizeof (NodeName), "EMMC");
  return UpdateDeviceInterruptInfo (NodeName, DsdtTableBuffer, DsdtTableSize, PhyAddress);
}

/**
  Create a patched copy of the DSDT template.

  Copies the template and applies the SPI controller, eMMC interrupt, PCI1
  disable, and RVU mailbox fixups.

  @param[in]   TemplateTable  DSDT template read from the firmware volume.
  @param[out]  NewTable       Receives the allocated patched DSDT.

  @retval  EFI_SUCCESS            The patched table was created.
  @retval  EFI_INVALID_PARAMETER  The template length is zero.
  @retval  EFI_OUT_OF_RESOURCES   The table buffer could not be allocated.
**/
STATIC
EFI_STATUS
CN10KProcessDsdt (
  IN CONST EFI_ACPI_DESCRIPTION_HEADER  *TemplateTable,
  OUT EFI_ACPI_DESCRIPTION_HEADER       **NewTable
  )
{
  UINT32  TableSize;
  VOID    *TableBuffer;

  TableSize = TemplateTable->Length;
  if (TableSize <= 0) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid DSDT table size (%d)\n", __FUNCTION__, TableSize));
    return EFI_INVALID_PARAMETER;
  }

  TableBuffer = AllocateZeroPool (TableSize);
  if (TableBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (TableBuffer, TemplateTable, TableSize);
  DEBUG ((DEBUG_VERBOSE, "%a: DSDT @0x%llx\n", __FUNCTION__, TableBuffer));

  /* Fixup SPI controller data */
  UpdateSpiControllerInfo (TableBuffer, TableSize);

  /* Fixup EMMC controller interrupt */
  UpdateEmmcInterruptInfo (TableBuffer, TableSize);

  /* Fixup PCI1 controller */
  DisablePci1Controller (TableBuffer, TableSize);

  UpdateRvuMailboxRange (TableBuffer, TableSize);

  *NewTable = TableBuffer;
  return EFI_SUCCESS;
}

/**
  Create a GTDT copy with the watchdog timer optionally disabled.

  Copies the GTDT template and clears PlatformTimerCount when
  PcdDisableWatchdog is TRUE, which removes the non-secure generic watchdog
  from the table.

  @param[in]   TemplateTable  GTDT template.
  @param[out]  NewTable       Receives the allocated GTDT.

  @retval  EFI_SUCCESS           The GTDT copy was created.
  @retval  EFI_OUT_OF_RESOURCES  The table buffer could not be allocated.
**/
STATIC
EFI_STATUS
CN10KProcessGtdt (
  IN CONST EFI_ACPI_DESCRIPTION_HEADER  *TemplateTable,
  OUT EFI_ACPI_DESCRIPTION_HEADER       **NewTable
  )
{
  UINTN                                         TableSize;
  EFI_ACPI_6_3_GENERIC_TIMER_DESCRIPTION_TABLE  *GtdtTable = NULL;

  TableSize = sizeof (EFI_ACPI_6_3_GENERIC_TIMER_DESCRIPTION_TABLE) +
              sizeof (EFI_ACPI_6_3_GTDT_SBSA_GENERIC_WATCHDOG_STRUCTURE);

  GtdtTable = AllocateZeroPool (TableSize);
  if (GtdtTable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (GtdtTable, TemplateTable, TableSize);

  if (PcdGetBool (PcdDisableWatchdog) == TRUE) {
    GtdtTable->PlatformTimerCount = 0;
  }

  *NewTable = (EFI_ACPI_DESCRIPTION_HEADER *)GtdtTable;
  return EFI_SUCCESS;
}

/**
  Read the last-level cache size from the device tree.

  Looks up the /l3-cache node and its cache-size property using the cached
  FDT client.

  @param[out]  CacheSize  Receives the cache size in bytes.

  @retval  EFI_SUCCESS     The cache size was found.
  @retval  EFI_NOT_FOUND   The /l3-cache node is missing.
  @retval  EFI_NO_MAPPING  The cache-size property is missing.
**/
STATIC
EFI_STATUS
GetLlcSizeOutOfDeviceTree (
  OUT UINT32  *CacheSize
  )
{
  EFI_STATUS    Status;
  FDT_HANDLE    L3CacheHandle = 0;
  CONST UINT32  *L3CacheSize  = NULL;

  DEBUG ((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

  Status = mFdtClient->GetNode (mFdtClient, "/l3-cache", &L3CacheHandle);
  if (EFI_ERROR (Status) || (L3CacheHandle == 0)) {
    DEBUG ((DEBUG_ERROR, "%a: %s node not found!\n", __FUNCTION__, L"/l3-cache"));
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_INFO, "%a: Found: %s in DT.\n", __FUNCTION__, L"/l3-cache"));
  Status = mFdtClient->GetNodeProperty (
                         mFdtClient,
                         L3CacheHandle,
                         "cache-size",
                         (CONST VOID **)&L3CacheSize,
                         NULL
                         );
  if (EFI_ERROR (Status) || (NULL == L3CacheSize)) {
    DEBUG ((DEBUG_ERROR, "%a: %s property not found!\n", __FUNCTION__, L"\"cache-size\""));
    return EFI_NO_MAPPING;
  }

  *CacheSize = FdtToCpu32 (*L3CacheSize);
  DEBUG ((
    DEBUG_INFO,
    "%a: L3CacheSize in DT = 0x%x (%d MB)\n",
    __FUNCTION__,
    *CacheSize,
    (*CacheSize)/SIZE_1MB
    ));

  return EFI_SUCCESS;
}

/**
  Build the PPTT for the configured number of cores.

  Allocates a table sized for the L3, L2, L1I, and L1D cache nodes, the CPU
  container, and one processor node per core, copies the template, and updates
  the L3 cache size and set count from the device tree.

  @param[in]   TemplateTable  PPTT template.
  @param[out]  NewTable       Receives the allocated PPTT.

  @retval  EFI_SUCCESS           The PPTT was built.
  @retval  EFI_OUT_OF_RESOURCES  The table buffer could not be allocated.
**/
STATIC
EFI_STATUS
CN10KProcessPptt (
  IN CONST EFI_ACPI_DESCRIPTION_HEADER  *TemplateTable,
  OUT EFI_ACPI_DESCRIPTION_HEADER       **NewTable
  )
{
  UINTN   TableSize;
  UINT32  L3CacheSize;

  EFI_ACPI_6_4_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_HEADER  *PpttTable  = NULL;
  EFI_ACPI_6_4_PPTT_STRUCTURE_PROCESSOR                    *CpuPointer = NULL;

  // Calculate required table size based on configured CPU count,
  // i.e. number of “/cpus/cpu-map/cluster%d/core%d” in DT which is reflected by PcdCoresPerNode.
  TableSize = sizeof (*PpttTable) +
              // L3 + L2 + L1I + L1D
              4 * sizeof (EFI_ACPI_6_4_PPTT_STRUCTURE_CACHE) +
              // One Socket node
              sizeof (*CpuPointer) + sizeof (UINT32) +
              // Each CPU has 2 private resources (UINT32 offsets to L1I and L1D nodes)
              PcdGet32 (PcdCoresPerNode) * (sizeof (*CpuPointer) + 2 * sizeof (UINT32));

  DEBUG ((DEBUG_INFO, "%a: CoresNo = %d\n", __FUNCTION__, PcdGet32 (PcdCoresPerNode)));

  PpttTable = AllocateZeroPool (TableSize);
  if (PpttTable == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Allocation of %d bytes failed \n", __FUNCTION__, TableSize));
    return EFI_OUT_OF_RESOURCES;
  }

  // Copy header + cache information + up to 'PcdCoresPerNode' CPU templates to new PPTT table:
  CopyMem (PpttTable, TemplateTable, TableSize);
  // Set new PPTT table total length
  PpttTable->Header.Length = TableSize;

  // Update L3 cache size information based on DT
  if (!EFI_ERROR (GetLlcSizeOutOfDeviceTree (&L3CacheSize))) {
    EFI_ACPI_6_4_PPTT_STRUCTURE_CACHE  *L3Cache;

    L3Cache       = (EFI_ACPI_6_4_PPTT_STRUCTURE_CACHE *)((UINT8 *)PpttTable + sizeof (*PpttTable));
    L3Cache->Size = L3CacheSize;
    // Number of sets = L3CacheSize devided by Line Size and devided by Associativity
    L3Cache->NumberOfSets = L3CacheSize/L3Cache->LineSize/L3Cache->Associativity;
  }

  // Return ouput PPTT table
  *NewTable = (EFI_ACPI_DESCRIPTION_HEADER *)PpttTable;
  DEBUG ((DEBUG_INFO, "%a: Corresponding PPTT table size = %d\n", __FUNCTION__, TableSize));

  return EFI_SUCCESS;
}

/**
  Build the MADT for the configured nodes and cores.

  Allocates a table sized for one GIC structure per CPU, one GICR and one ITS
  per node, and a single GIC distributor, copies the template, and expands the
  GIC, GICR, and ITS entries for every node. MPIDR values are generated from
  the node and core index.

  @param[in]   TemplateTable  MADT template.
  @param[out]  NewTable       Receives the allocated MADT.

  @retval  EFI_SUCCESS           The MADT was built.
  @retval  EFI_OUT_OF_RESOURCES  The table buffer could not be allocated.
**/
STATIC
EFI_STATUS
CN10KProcessMadt (
  IN CONST EFI_ACPI_DESCRIPTION_HEADER  *TemplateTable,
  OUT EFI_ACPI_DESCRIPTION_HEADER       **NewTable
  )
{
  EFI_ACPI_6_3_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER  *ApicTable = NULL;

  EFI_ACPI_6_5_GIC_STRUCTURE              *GicPointer    = NULL;
  EFI_ACPI_6_3_GIC_DISTRIBUTOR_STRUCTURE  *GicDPointer   = NULL;
  EFI_ACPI_6_3_GICR_STRUCTURE             *GicRPointer   = NULL;
  EFI_ACPI_6_3_GIC_ITS_STRUCTURE          *GicItsPointer = NULL;

  CONST UINT8  *DataStream = NULL;
  UINTN        BytesLeft;
  UINTN        Node;
  UINTN        Core;
  UINTN        CpuCount;
  UINTN        NodeCount;
  UINTN        TableSize;
  UINT32       CpuId;

  CpuCount  = PcdGet32 (PcdNodeCount) * PcdGet32 (PcdCoresPerNode);
  NodeCount = PcdGet32 (PcdNodeCount);

  // calculate required table size based on configuration
  // 1 ITS per node, 1 GICR per node and GICD always have 1 instance
  TableSize = sizeof (EFI_ACPI_6_3_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER) +
              CpuCount * sizeof (EFI_ACPI_6_5_GIC_STRUCTURE) +
              NodeCount * (sizeof (EFI_ACPI_6_3_GIC_ITS_STRUCTURE) +
                           sizeof (EFI_ACPI_6_3_GICR_STRUCTURE)) +
              // only one GICD
              sizeof (EFI_ACPI_6_3_GIC_DISTRIBUTOR_STRUCTURE);

  ApicTable = AllocateZeroPool (TableSize);
  if (ApicTable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // copy header and adjust table size
  CopyMem (
    ApicTable,
    TemplateTable,
    sizeof (EFI_ACPI_6_3_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER)
    );

  ApicTable->Header.Length = TableSize;

  // Setup output pointers
  GicPointer    = (EFI_ACPI_6_5_GIC_STRUCTURE *)(&ApicTable[1]);
  GicDPointer   = (EFI_ACPI_6_3_GIC_DISTRIBUTOR_STRUCTURE *)(&GicPointer[CpuCount]);
  GicRPointer   = (EFI_ACPI_6_3_GICR_STRUCTURE *)(&GicDPointer[1]);
  GicItsPointer = (EFI_ACPI_6_3_GIC_ITS_STRUCTURE *)(&GicRPointer[NodeCount]);

  // go over template structures and create necessary number of them
  DataStream = (UINT8 *)((UINTN)TemplateTable + sizeof (EFI_ACPI_6_3_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER));
  BytesLeft  = TemplateTable->Length - sizeof (EFI_ACPI_6_3_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER);

  while (BytesLeft > 0) {
    UINT8  Size = ((ACPI_APIC_ENTRY_HDR *)DataStream)->Length;
    UINT8  Type = ((ACPI_APIC_ENTRY_HDR *)DataStream)->Type;

    DEBUG ((DEBUG_INFO, "[ACPI] Found template Type %x Size %d\n", Type, Size));

    switch (Type) {
      case EFI_ACPI_6_3_GIC:
      {
        // we must populate CPUsPerCluster*NumberOFClustes per node
        for (Node = 0; Node < NodeCount; Node++) {
          for (Core = 0; Core < PcdGet32 (PcdCoresPerNode); Core++) {
            CpuId = GET_CPU_MPID (Node, Core);

            CopyMem (GicPointer, DataStream, sizeof (EFI_ACPI_6_5_GIC_STRUCTURE));

            GicPointer->AcpiProcessorUid   = Core;
            GicPointer->CPUInterfaceNumber = 0; // No legacy
            GicPointer->MPIDR              = CpuId;

            DEBUG ((DEBUG_VERBOSE, "[ACPI] keeping CPU 0x%x\n", CpuId));

            GicPointer++;
          }
        }

        break;
      }
      case EFI_ACPI_6_3_GICD:
      {
        CopyMem (
          GicDPointer,
          DataStream,
          sizeof (EFI_ACPI_6_3_GIC_DISTRIBUTOR_STRUCTURE)
          );
        break;
      }
      case EFI_ACPI_6_3_GICR:
      {
        for (Node = 0; Node < NodeCount; Node++) {
          CopyMem (
            GicRPointer,
            DataStream,
            sizeof (EFI_ACPI_6_3_GICR_STRUCTURE)
            );

          GicRPointer++;
        }

        break;
      }
      case EFI_ACPI_6_3_GIC_ITS:
      {
        for (Node = 0; Node < NodeCount; Node++) {
          CopyMem (
            GicItsPointer,
            DataStream,
            sizeof (EFI_ACPI_6_3_GIC_ITS_STRUCTURE)
            );
          GicItsPointer->GicItsId = Node;

          GicItsPointer++;
        }

        break;
      }
      default:
        ASSERT (FALSE);
        break;
    }

    DataStream += Size;
    BytesLeft  -= Size;
  }

  *NewTable = (EFI_ACPI_DESCRIPTION_HEADER *)ApicTable;

  return EFI_SUCCESS;
}

/**
  Build the HEST from the template and the device tree.

  Counts the GHES subnodes under /soc@0/sdei-ghes, checks that the template
  has at least that many error source entries, and adds the base address of
  the reserved GHES memory to each error status address.

  @param[in,out]  TemplateTable  HEST template.
  @param[out]  NewTable       Receives the allocated HEST.

  @retval  EFI_SUCCESS            The HEST was built.
  @retval  EFI_COMPROMISED_DATA   The template has fewer entries than the
                                  device tree requires.
  @retval  EFI_OUT_OF_RESOURCES   The table buffer could not be allocated.
  @retval  Others                 A required device tree node or property is
                                  missing.
**/
STATIC
EFI_STATUS
CN10KProcessHest (
  IN OUT EFI_ACPI_DESCRIPTION_HEADER   *TemplateTable,
  OUT EFI_ACPI_DESCRIPTION_HEADER  **NewTable
  )
{
  EFI_ACPI_6_3_HARDWARE_ERROR_SOURCE_TABLE_HEADER       *HestTable;
  EFI_ACPI_6_3_GENERIC_HARDWARE_ERROR_SOURCE_STRUCTURE  *TableEntry;
  EFI_STATUS                                            Status;
  FDT_HANDLE                                            GhesHandle;
  FDT_HANDLE                                            GhesSdei;
  FDT_HANDLE                                            Ghes, NextGhes;
  CONST UINT64                                          *PhyAddress;
  UINT64                                                BaseAddress;
  UINT32                                                Index;
  UINT32                                                NumberGhes;
  UINT32                                                TableLength;
  CHAR8                                                 PropertyName[64];

  ASSERT (mFdtClient != NULL);
  Status = mFdtClient->GetNode (mFdtClient, "/reserved-memory/ghes-hest", &GhesHandle);
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: Found %a offset: %d\n",
    __FUNCTION__,
    "/reserved-memory/ghes-hest",
    GhesHandle
    ));

  Status = mFdtClient->GetNodeProperty (mFdtClient, GhesHandle, "reg", (CONST VOID **)&PhyAddress, NULL);
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  BaseAddress = FdtToCpu64 (*PhyAddress);
  DEBUG ((
    DEBUG_INFO,
    "%a: PhyAddress: %p %llx %llx\n",
    __FUNCTION__,
    PhyAddress,
    BaseAddress,
    FdtToCpu64 (*PhyAddress)
    ));

  AsciiSPrint (PropertyName, sizeof (PropertyName), "/soc@0/sdei-ghes");

  Status = mFdtClient->GetNode (
                         mFdtClient,
                         PropertyName,
                         &GhesSdei
                         );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: cannot locate: %a\n", __FUNCTION__, PropertyName));
    return EFI_ABORTED;
  }

  NumberGhes = 0;
  Status     = mFdtClient->GetFirstSubnode (mFdtClient, GhesSdei, &Ghes);
  while (!EFI_ERROR (Status)) {
    NumberGhes++;
    Status = mFdtClient->GetNextSubnode (mFdtClient, Ghes, &NextGhes);
    Ghes   = NextGhes;
  }

  // HEST table template shall not have less entries than what's defined in DT.
  if (((EFI_ACPI_6_3_HARDWARE_ERROR_SOURCE_TABLE_HEADER *)TemplateTable)->ErrorSourceCount < NumberGhes) {
    return EFI_COMPROMISED_DATA;
  }

  TableLength = sizeof (EFI_ACPI_6_3_HARDWARE_ERROR_SOURCE_TABLE_HEADER) +
                sizeof (EFI_ACPI_6_3_GENERIC_HARDWARE_ERROR_SOURCE_STRUCTURE) * NumberGhes;
  TemplateTable->Length                                                                = TableLength;
  ((EFI_ACPI_6_3_HARDWARE_ERROR_SOURCE_TABLE_HEADER *)TemplateTable)->ErrorSourceCount = NumberGhes;
  HestTable                                                                            = AllocateCopyPool (TemplateTable->Length, TemplateTable);
  if (HestTable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  TableEntry = (EFI_ACPI_6_3_GENERIC_HARDWARE_ERROR_SOURCE_STRUCTURE *)
               ((UINT8 *)HestTable + sizeof (EFI_ACPI_6_3_HARDWARE_ERROR_SOURCE_TABLE_HEADER));

  for (Index = 0; Index < HestTable->ErrorSourceCount; Index++) {
    DEBUG ((
      DEBUG_INFO,
      "%a: Generic Hardware Error Source Address %llx\n",
      __FUNCTION__,
      TableEntry[Index].ErrorStatusAddress.Address
      ));
    TableEntry[Index].ErrorStatusAddress.Address += BaseAddress;
    DEBUG ((
      DEBUG_INFO,
      "%a: Generic Hardware Error Source Address %llx\n",
      __FUNCTION__,
      TableEntry[Index].ErrorStatusAddress.Address
      ));
  }

  *NewTable = (EFI_ACPI_DESCRIPTION_HEADER *)HestTable;

  return EFI_SUCCESS;
}

/**
  Build the BERT from the template and the device tree.

  Uses the third and fourth cells of the bed-bert reg property for the error
  region offset and length, and the ghes-bert reserved-memory base for the
  final BootErrorRegion address.

  @param[in]   TemplateTable  BERT template.
  @param[out]  NewTable       Receives the allocated BERT.

  @retval  EFI_SUCCESS            The BERT was built.
  @retval  EFI_COMPROMISED_DATA   A required reg property is missing.
  @retval  EFI_OUT_OF_RESOURCES   The table buffer could not be allocated.
  @retval  EFI_ABORTED            A required device tree node is missing.
**/
STATIC
EFI_STATUS
CN10KProcessBert (
  IN EFI_ACPI_DESCRIPTION_HEADER   *TemplateTable,
  OUT EFI_ACPI_DESCRIPTION_HEADER  **NewTable
  )
{
  EFI_ACPI_6_4_BOOT_ERROR_RECORD_TABLE_HEADER  *BertTable;
  EFI_STATUS                                   Status;
  FDT_HANDLE                                   BertHandle;
  CONST UINT64                                 *Reg64;
  UINT64                                       BaseAddress;
  UINT64                                       Length;
  CHAR8                                        PropertyName[64];

  //  Status = VerifyOemTableId(TemplateTable->OemTableId);
  // if (EFI_ERROR(Status)) {
  //  return Status;
  // }

  // NOTE: for Structera/Odyssey bed-bert node is out of soc@0. For T9x/CN10k this node is under soc@0.
  AsciiSPrint (PropertyName, sizeof (PropertyName), "/soc@0/bed-bert/bert");

  Status = mFdtClient->GetNode (mFdtClient, PropertyName, &BertHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: cannot locate: %a\n", __FUNCTION__, PropertyName));
    return EFI_ABORTED;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: Found %a offset: %d\n",
    __FUNCTION__,
    PropertyName,
    BertHandle
    ));

  Status = mFdtClient->GetNodeProperty (mFdtClient, BertHandle, "reg", (CONST VOID **)&Reg64, NULL);
  if (EFI_ERROR (Status)) {
    return EFI_COMPROMISED_DATA;
  }

  // T9x/CN10K uses third entry of bed-bert 'reg' to store BootErrorRegion Address and Length.
  BaseAddress = FdtToCpu64 (Reg64[2]); // 0x14 = ATF offset_of(otx2_ghes_err_ring.records[0])
  // Length represents number of BERT records multiplied by one record size.
  Length = FdtToCpu64 (Reg64[3]);

  // T9x/CN10K uses also ghes-bert to form final BootErrorRegion address.
  AsciiSPrint (PropertyName, sizeof (PropertyName), "/reserved-memory/ghes-bert");
  Status = mFdtClient->GetNode (mFdtClient, PropertyName, &BertHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ERROR: cannot locate: %a\n", __FUNCTION__, PropertyName));
    return EFI_ABORTED;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: Found %a offset: %d\n",
    __FUNCTION__,
    PropertyName,
    BertHandle
    ));

  Status = mFdtClient->GetNodeProperty (mFdtClient, BertHandle, "reg", (CONST VOID **)&Reg64, NULL);
  if (EFI_ERROR (Status)) {
    return EFI_COMPROMISED_DATA;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: BaseAddress: 0x%llx Length = 0x%llx\n",
    __FUNCTION__,
    BaseAddress,
    Length
    ));

  BertTable = AllocateCopyPool (TemplateTable->Length, TemplateTable);
  if (BertTable == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Update values in BERT table according to values gathered from DT:
  BertTable->BootErrorRegion       = BaseAddress + FdtToCpu64 (*Reg64);
  BertTable->BootErrorRegionLength = Length;

  DEBUG ((
    DEBUG_INFO,
    "%a: Boot Error Region: 0x%llx, Length: 0x%lx\n",
    __FUNCTION__,
    BertTable->BootErrorRegion,
    BertTable->BootErrorRegionLength
    ));

  *NewTable = (EFI_ACPI_DESCRIPTION_HEADER *)BertTable;

  return EFI_SUCCESS;
}

/**
  Install a copy of the MPAM template.

  @param[in]   TemplateTable  MPAM template.
  @param[out]  NewTable       Receives the allocated MPAM table.

  @retval  EFI_SUCCESS           The table was copied.
  @retval  EFI_OUT_OF_RESOURCES  The table buffer could not be allocated.
**/
STATIC
EFI_STATUS
InstallMpamTable (
  IN EFI_ACPI_DESCRIPTION_HEADER   *TemplateTable,
  OUT EFI_ACPI_DESCRIPTION_HEADER  **NewTable
  )
{
  EFI_ACPI_DESCRIPTION_HEADER  *MpamTable = NULL;

  MpamTable = AllocateCopyPool (TemplateTable->Length, TemplateTable);
  if (NULL == MpamTable) {
    DEBUG ((DEBUG_ERROR, "%a: couldn't allocate memory!\n", __FUNCTION__));
    return EFI_OUT_OF_RESOURCES;
  }

  *NewTable = (EFI_ACPI_DESCRIPTION_HEADER *)MpamTable;
  DEBUG ((DEBUG_INFO, "%a: install MPAM table for %a\n", __FUNCTION__, mProductName));

  return EFI_SUCCESS;
}

/**
  Install the MPAM table only when it matches the detected platform.

  The table is installed when the board OEM table ID and the MPAM table OEM
  table ID identify the same silicon family (CN103 or CN106).

  @param[in]   TemplateTable  MPAM template.
  @param[out]  NewTable       Receives the allocated MPAM table.

  @retval  EFI_SUCCESS            The table matches and was copied.
  @retval  EFI_UNSUPPORTED        The table is not meant for this platform.
  @retval  EFI_OUT_OF_RESOURCES   The table buffer could not be allocated.
**/
STATIC
EFI_STATUS
CN10KProcessMpam (
  IN EFI_ACPI_DESCRIPTION_HEADER   *TemplateTable,
  OUT EFI_ACPI_DESCRIPTION_HEADER  **NewTable
  )
{
  EFI_STATUS  Status;

  DEBUG ((
    DEBUG_ERROR,
    "%a: mProductName = %a, mOemTableId = 0x%lx, OemTableId = 0x%lx\n",
    __FUNCTION__,
    mProductName,
    mOemTableId,
    TemplateTable->OemTableId
    ));

  if (((CompareMem (&mOemTableId, "ebb-103", sizeof (mOemTableId)) == 0) ||
       (CompareMem (&mOemTableId, "crb-102", sizeof (mOemTableId)) == 0)) &&
      (CompareMem (&(TemplateTable->OemTableId), "CN103", sizeof ("CN103")) == 0))
  {
    Status = InstallMpamTable (TemplateTable, NewTable);
  } else if (((CompareMem (&mOemTableId, "ebb-106", sizeof (mOemTableId)) == 0) ||
              (CompareMem (&mOemTableId, "ebb-106s", sizeof (mOemTableId)) == 0) ||
              (CompareMem (&mOemTableId, "crb-106", sizeof (mOemTableId)) == 0)) &&
             (CompareMem (&(TemplateTable->OemTableId), "CN106", sizeof ("CN106")) == 0))
  {
    Status = InstallMpamTable (TemplateTable, NewTable);
  } else {
    // OemTableId does not match with underlying system. Skip installing this table.
    DEBUG ((DEBUG_ERROR, "%a: this MPAM table is not meant for given system. Skip it.\n", __FUNCTION__));
    Status = EFI_UNSUPPORTED;
  }

  return Status;
}

/**
  Install the SFP SSDT when the board supports it.

  Supported only on the ebb103 board. The table is installed when the I2C
  controller node has the twsi-in-kernel property.

  @param[in]   TemplateTable  SFP SSDT template.
  @param[out]  NewTable       Receives the allocated SSDT.

  @retval  EFI_SUCCESS           The table was copied.
  @retval  EFI_UNSUPPORTED       The board is not supported.
  @retval  EFI_NOT_FOUND         The I2C node or property is missing.
  @retval  EFI_OUT_OF_RESOURCES  The table buffer could not be allocated.
**/
STATIC
EFI_STATUS
ProcessSfpTable (
  IN EFI_ACPI_DESCRIPTION_HEADER   *TemplateTable,
  OUT EFI_ACPI_DESCRIPTION_HEADER  **NewTable
  )
{
  EFI_STATUS                   Status;
  FDT_HANDLE                   I2cHandle;
  CHAR8                        NodeName[100];
  UINT32                       *Flag;
  EFI_ACPI_DESCRIPTION_HEADER  *SsdtTable = NULL;

  if (mFdtClient == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: FDT Client protocol not found\n", __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  // SFP for EBB102, EBB103 boards
  if (CompareMem (&mOemTableId, "ebb-103", sizeof (mOemTableId)) == 0) {
    AsciiSPrint (NodeName, sizeof (NodeName), "/soc@0/pci@878000000000/mrml-bridge0@1,0/i2c@24,1");
  } else {
    // SFP in linux kernel not supported
    return EFI_UNSUPPORTED;
  }

  /* Get pointer to I2C node */
  Status = mFdtClient->GetNode (mFdtClient, NodeName, &I2cHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: I2C node not found\n", __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  Status = mFdtClient->GetNodeProperty (mFdtClient, I2cHandle, "twsi-in-kernel", (CONST VOID **)&Flag, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: twsi-in-kernel device tree property not found \n", __FUNCTION__));
    return EFI_NOT_FOUND;
  }

  // twsi-in-kernel present, keep SFP devices
  SsdtTable = AllocateCopyPool (TemplateTable->Length, TemplateTable);
  if (NULL == SsdtTable) {
    DEBUG ((DEBUG_ERROR, "%a: couldn't allocate memory!\n", __FUNCTION__));
    return EFI_OUT_OF_RESOURCES;
  }

  *NewTable = (EFI_ACPI_DESCRIPTION_HEADER *)SsdtTable;
  DEBUG ((DEBUG_INFO, "%a: install SFP SSDT table for %a\n", __FUNCTION__, mProductName));

  return EFI_SUCCESS;
}

/**
  Install an SSDT template when it is applicable to this platform.

  SFP-prefixed tables are handled by ProcessSfpTable(). Other tables are
  installed when their OEM table ID is empty (universal table) or matches the
  detected board.

  @param[in]   TemplateTable  SSDT template.
  @param[out]  NewTable       Receives the allocated SSDT.

  @retval  EFI_SUCCESS           The table was copied.
  @retval  EFI_UNSUPPORTED       The table is not meant for this platform.
  @retval  EFI_OUT_OF_RESOURCES  The table buffer could not be allocated.
**/
STATIC
EFI_STATUS
CN10KProcessSsdt (
  IN EFI_ACPI_DESCRIPTION_HEADER   *TemplateTable,
  OUT EFI_ACPI_DESCRIPTION_HEADER  **NewTable
  )
{
  EFI_ACPI_DESCRIPTION_HEADER  *SsdtTable = NULL;

  DEBUG ((
    DEBUG_INFO,
    "%a: mProductName = %a, mOemTableId = 0x%lx, OemTableId = 0x%lx\n",
    __FUNCTION__,
    mProductName,
    mOemTableId,
    TemplateTable->OemTableId
    ));

  // Check for SFP SSDT table first
  // First three id letters are sfp
  if (TemplateTable->OemTableId &&
      !CompareMem (&(TemplateTable->OemTableId), "sfp", 3))
  {
    return ProcessSfpTable (TemplateTable, NewTable);
  }

  // Check if table is for this platform. If OemTableId in SSDT table is empty (0 value),
  // it means this table is universal and shall be installed regardless of board model.
  if (TemplateTable->OemTableId &&
      CompareMem (
        &(TemplateTable->OemTableId),
        &mOemTableId,
        sizeof (TemplateTable->OemTableId)
        ))
  {
    // OemTableId does not match with underlying system. Skip installing this table.
    DEBUG ((DEBUG_INFO, "%a: this SSDT is not meant for given system. Skip it.\n", __FUNCTION__));
    return EFI_UNSUPPORTED;
  }

  SsdtTable = AllocateCopyPool (TemplateTable->Length, TemplateTable);
  if (NULL == SsdtTable) {
    DEBUG ((DEBUG_ERROR, "%a: couldn't allocate memory!\n", __FUNCTION__));
    return EFI_OUT_OF_RESOURCES;
  }

  *NewTable = (EFI_ACPI_DESCRIPTION_HEADER *)SsdtTable;
  DEBUG ((DEBUG_INFO, "%a: install SSDT table for %a\n", __FUNCTION__, mProductName));

  return EFI_SUCCESS;
}

/**
  Determine the board model and its ACPI OEM table ID.

  Reads BOARD-MODEL from the configuration device tree and maps the known
  board names to the eight-character OEM table ID used to select the matching
  SSDT and MPAM tables. Updates mProductName and mOemTableId. A lookup failure
  only produces a warning because the generic tables remain installable.

**/
STATIC
VOID
DetermineBoardModel (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = CN10KGetConfigPropertyAscii ("BOARD-MODEL", mProductName, sizeof (mProductName));

  if (EFI_ERROR (Status)) {
    // Just warn user, but continue as this affects only SSDT handling
    DEBUG ((DEBUG_WARN, "ACPI: WARNING: Cannot get \"BOARD-MODEL\" from DT\n"));
  } else {
    /* 'OEM Table ID' in ACPI tables has only 8 bytes, but mProductName might have more chars.
        If one want to compare mProductName agains 'OEM Table ID' defined in given ACPI table,
        then one need to shrink gathered mProductName to 8 bytes or less.
        It allows to load only those tables which 'OEM Table ID' matches with mOemTableId.
    */

    // -------------------------- CN10K --------------------------
    // NOTE: case insensitive check against mProductName
    if ((AsciiStriCmp (mProductName, "asim-cn10ka")  == 0) ||
        (AsciiStriCmp (mProductName, "asim-cn10kas") == 0))
    {
      CopyMem (&mOemTableId, "a-cn10ka", sizeof (mOemTableId));
    } else if ((AsciiStriCmp (mProductName, "cb106")  == 0) ||
               (AsciiStriCmp (mProductName, "ebb106") == 0))
    {
      CopyMem (&mOemTableId, "ebb-106", sizeof (mOemTableId));
    } else if ((AsciiStriCmp (mProductName, "cb106s")  == 0) ||
               (AsciiStriCmp (mProductName, "ebb106s") == 0))
    {
      CopyMem (&mOemTableId, "ebb-106s", sizeof (mOemTableId));
    }
    // NOTE: crb106s-r1p1 and crb106s-r1p2 have same Board Model: crb106s,
    // but different Board Revision: r1p1 or r1p2.
    else if ((AsciiStriCmp (mProductName, "crb106")      == 0) ||
             (AsciiStriCmp (mProductName, "crb106-1u")     == 0) ||
             (AsciiStriCmp (mProductName, "crb106s")     == 0) ||
             (AsciiStriCmp (mProductName, "crb106-pcie") == 0))
    {
      CopyMem (&mOemTableId, "crb-106", sizeof (mOemTableId));
    }
    // cn10kb (CN102, CN103)
    else if (AsciiStriCmp (mProductName, "asim-cn10kb") == 0) {
      // Use same file as for cn10ka as configuration is the same
      CopyMem (&mOemTableId, "a-cn10ka", sizeof (mOemTableId));
    } else if ((AsciiStriCmp (mProductName, "cb102")  == 0) ||
               (AsciiStriCmp (mProductName, "ebb102") == 0) ||
               (AsciiStriCmp (mProductName, "cb103")  == 0) ||
               (AsciiStriCmp (mProductName, "ebb103") == 0))
    {
      CopyMem (&mOemTableId, "ebb-103", sizeof (mOemTableId));
    }
    // CRB102-COMX
    else if (AsciiStriCmp (mProductName, "crb102-comx") == 0) {
      CopyMem (&mOemTableId, "crb-102", sizeof (mOemTableId));
    }

    // --------------------------- END ---------------------------

    DEBUG ((
      DEBUG_INFO,
      "%a: mProductName = %a, mOemTableId = 0x%lx\n",
      __FUNCTION__,
      mProductName,
      mOemTableId
      ));
  }
}

/**
  Entry point of the ACPI platform driver.

  Determines the board model, locates the ACPI table and MRVL FDT client
  protocols and the firmware volume that holds the table storage file, then
  reads, patches, and installs every ACPI table. Tables that cannot be
  processed are skipped so the remaining tables are still installed.

  DSDT, SSDT, MCFG, and IORT are assumed to agree on PCI segment numbering,
  and the segment numbers are assumed to be sequential.

  @param[in]  ImageHandle  Image handle for this driver.
  @param[in]  SystemTable  Pointer to the system table.

  @return  EFI_SUCCESS           All applicable tables were processed.
  @return  EFI_ABORTED           A required protocol or firmware volume was
                                 not found.
  @return  EFI_OUT_OF_RESOURCES  A table buffer could not be allocated.

**/
EFI_STATUS
EFIAPI
AcpiPlatformEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                     Status;
  EFI_ACPI_TABLE_PROTOCOL        *AcpiTable;
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *FwVol;
  INTN                           Instance = 0;
  EFI_ACPI_DESCRIPTION_HEADER    *TemplateTable;
  EFI_ACPI_DESCRIPTION_HEADER    *CurrentTable;
  UINTN                          TableHandle;
  UINT32                         FvStatus;
  UINTN                          TableSize;
  UINTN                          Size;

  // Determine underlying board (e.g. "ebb106" etc.) based on DT node:
  DetermineBoardModel ();

  //
  // Find the AcpiTable protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID **)&AcpiTable
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ACPI: ERROR: cannot locate: gEfiAcpiTableProtocolGuid\n"));
    return EFI_ABORTED;
  }

  Status = gBS->LocateProtocol (
                  &gMrvlFdtClientProtocolGuid,
                  NULL,
                  (VOID **)&mFdtClient
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ACPI: ERROR: cannot locate: gMrvlFdtClientProtocolGuid\n"));
    return EFI_ABORTED;
  }

  //
  // Locate the firmware volume protocol
  //
  Status = LocateFvInstanceWithTables (&FwVol);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ACPI: ERROR: Cannot get VF volume\n"));
    return EFI_ABORTED;
  }

  // Read tables from the storage file.
  //
  while (Status == EFI_SUCCESS) {
    TemplateTable = NULL;

    Status = FwVol->ReadSection (
                      FwVol,
                      (EFI_GUID *)PcdGetPtr (PcdAcpiTableStorageFile),
                      EFI_SECTION_RAW,
                      Instance,
                      (VOID **)&TemplateTable,
                      &Size,
                      &FvStatus
                      );

    if (!EFI_ERROR (Status)) {
      TableSize = TemplateTable->Length;
      ASSERT (Size >= TableSize);

      DescribeTable (TemplateTable);

      CurrentTable = NULL;

      switch (TemplateTable->Signature) {
        case EFI_ACPI_6_3_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_STRUCTURE_SIGNATURE:
        {
          // If modified, PPTT will be replaced and and the one is read from the file will be freed
          Status = CN10KProcessPptt (TemplateTable, &CurrentTable);
          break;
        }
        case EFI_ACPI_6_3_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE:
        {
          // If modified, MADT will be replaced and and the one is read from the file will be freed
          Status = CN10KProcessMadt (TemplateTable, &CurrentTable);
          break;
        }
        case EFI_ACPI_6_4_BOOT_ERROR_RECORD_TABLE_SIGNATURE:
        {
          Status = CN10KProcessBert (TemplateTable, &CurrentTable);
          break;
        }
        case EFI_ACPI_6_3_HARDWARE_ERROR_SOURCE_TABLE_SIGNATURE:
        {
          Status = CN10KProcessHest (TemplateTable, &CurrentTable);
          break;
        }
        case EFI_ACPI_6_3_GENERIC_TIMER_DESCRIPTION_TABLE_SIGNATURE:
        {
          Status = CN10KProcessGtdt (TemplateTable, &CurrentTable);
          break;
        }
        case EFI_ACPI_6_3_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE:
        {
          Status = CN10KProcessDsdt (TemplateTable, &CurrentTable);
          break;
        }
        case EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE:
        {
          Status = CN10KProcessSsdt (TemplateTable, &CurrentTable);
          break;
        }
        case EFI_ACPI_6_4_MEMORY_SYSTEM_RESOURCE_PARTITIONING_MONITORING_TABLE_STRUCTURE_SIGNATURE:
        {
          Status = CN10KProcessMpam (TemplateTable, &CurrentTable);
          break;
        }
        default:
          // Allocate CurrentTable and fill it with original TemplateTable.
          CurrentTable = AllocateCopyPool (TemplateTable->Length, TemplateTable);
          if (!CurrentTable) {
            FreePool (TemplateTable);
            return EFI_OUT_OF_RESOURCES;
          }
      }

      if (!EFI_ERROR (Status) &&
          (CurrentTable != NULL) &&
          (CurrentTable->Signature == TemplateTable->Signature))
      {
        DumpTable (CurrentTable);

        // Install ACPI table
        Status = AcpiTable->InstallAcpiTable (
                              AcpiTable,
                              CurrentTable,
                              CurrentTable->Length,
                              &TableHandle
                              );

        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_INFO, "Status: %r, CurrentTable: %p\n", Status, CurrentTable));
        }
      }

      // Was there any error while processing current table?
      if (EFI_ERROR (Status)) {
        // Clear status, so that while loop can at least proceed with next tables
        Status = EFI_SUCCESS;
      }

      // Clean up
      FreePool (TemplateTable);
      if (CurrentTable) {
        FreePool (CurrentTable);
      }

      // Increment the instance
      Instance++;
    }
  }

  return EFI_SUCCESS;
}
