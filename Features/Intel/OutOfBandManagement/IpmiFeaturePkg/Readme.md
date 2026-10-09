# Overview

* **Feature Name:** Intelligent Platform Management Interface (IPMI)
* **PI Phase(s) Supported:** PEI, DXE, SMM
* **SMM Required?** Yes

More Information:

* [IPMI Specification 2nd Generation v2.0](https://www.intel.com/content/dam/www/public/us/en/documents/product-briefs/ipmi-second-gen-interface-spec-v2-rev1-1.pdf)

## Purpose

The IPMI feature provides firmware functionality that implements behavior described in the IPMI specification. IPMI
enables out-of-band and monitoring capabilities independent of the host system's CPU, firmware, and operating system.

## High-Level Theory of Operation

The feature is organized around a phase-specific IPMI transport. During PEI,
DXE, and MM, a GenericIpmi module initializes the interfaces enabled by the
platform and publishes legacy and Transport2 IPMI PPIs or protocols. Clients
submit a network function, command, and payload through those interfaces
without depending directly on the physical BMC connection.

The transport selects KCS, BT, SSIF, or IPMB according to the platform PCD
configuration. KCS and BT use configured I/O or MMIO resources, SSIF uses an
SMBus controller, and IPMB uses an I2C controller. Transport2 clients can use
the configured default interface or explicitly select an available interface.
The transport serializes access, formats the request, waits for the BMC,
validates the response, and returns the completion status and response data to
the caller.

Services above the transport provide BMC initialization, FRB and OS watchdog
management, SEL-backed event logging, FRU access, Serial over LAN status, ACPI
table publication, and ACPI power-state notification. Generic event-log and
FRU protocols separate consumers from their BMC-backed redirection providers,
allowing another provider to implement the same service without changing the
consumer.

## Firmware Volumes

The package supplies FDF fragments rather than naming platform firmware
volumes. A platform includes each fragment in the appropriate firmware volume:

* `Include/PreMemory.fdf` belongs in a firmware volume available before
  permanent memory. It contains `PeiGenericIpmi`, `PeiIpmiInit`, `FrbPei`, and
  `PeiBmcElog`.
* `Include/PostMemory.fdf` belongs in a post-memory/DXE firmware volume. It
  contains the DXE transport and initialization modules, ACPI support, DXE and
  traditional MM event logging, FRB, FRU, OS watchdog, and SOL modules.
* `BmcAcpi` requires the platform's `DRIVER_ACPITABLE` FDF rule so its SSDT is
  packaged with the driver.
* Standalone MM modules are built by `Include/IpmiFeature.dsc` but are not in
  the supplied FDF fragments. Platforms using Standalone MM must place the
  Standalone MM GenericIpmi, BmcElog, GenericElog, and BmcAcpiSwChild modules
  in the platform's Standalone MM firmware volume.

## Modules

The package contains the following modules. The type shown for each module is
the `MODULE_TYPE` declared by its INF file.

### BmcAcpi

* **INF:** `BmcAcpi/BmcAcpi.inf`
* **Module type:** `DXE_DRIVER`

Installs the BMC ACPI SSDT from the firmware volume. Before installing the
table, the driver updates its OEM identifiers and patches the IPMI operation
region with the configured base address and address-space type so that the
operating system can discover the BMC system interface.

### BmcAcpiState

* **INF:** `BmcAcpiState/BmcAcpiState.inf`
* **Module type:** `DXE_DRIVER`

Registers an Exit Boot Services notification and uses the IPMI transport to
tell the BMC that the system and device power states have transitioned to
S0/D0. This replaces the earlier ACPI-enable SMM notification path for the
boot-to-runtime transition.

### BmcAcpiSwChild

* **INF:** `BmcAcpiSwChild/BmcAcpiSwChild.inf`
* **Module type:** `DXE_SMM_DRIVER`

Provides the traditional MM implementation of the BMC ACPI software-child
policy protocol. The protocol allows an ACPI MM platform driver to report
system power-state transitions to the BMC and includes support for
synchronizing the BMC SEL clock.

### BmcAcpiSwChildStandaloneMm

* **INF:** `BmcAcpiSwChild/BmcAcpiSwChildStandaloneMm.inf`
* **Module type:** `MM_STANDALONE`

Provides the Standalone MM implementation of the same BMC ACPI software-child
policy protocol and power-state reporting behavior as `BmcAcpiSwChild`. Its
entry point and service bindings use the Standalone MM environment.

### DxeBmcElog

* **INF:** `BmcElog/DxeBmcElog.inf`
* **Module type:** `DXE_DRIVER`

Publishes the DXE event-log redirection protocol backed by the BMC System Event
Log (SEL). It translates generic set, get, erase, and activate operations into
IPMI platform-event and SEL commands.

### PeiBmcElog

* **INF:** `BmcElog/PeiBmcElog.inf`
* **Module type:** `PEIM`

Publishes the PEI event-log redirection PPI backed by the BMC SEL. It makes the
same IPMI event-log storage operations available to PEI clients through the
PEI IPMI transport PPI.

### SmmBmcElog

* **INF:** `BmcElog/SmmBmcElog.inf`
* **Module type:** `DXE_SMM_DRIVER`

Publishes the traditional MM event-log redirection protocol backed by the BMC
SEL. It enables MM clients and the generic MM event-log layer to add, retrieve,
clear, or enable BMC event logging.

### StandaloneMmBmcElog

* **INF:** `BmcElog/StandaloneMmBmcElog.inf`
* **Module type:** `MM_STANDALONE`

Provides the Standalone MM form of the BMC SEL redirection protocol. It uses
the common BMC event-log implementation with Standalone MM entry-point and
service bindings.

### FrbDxe

* **INF:** `Frb/FrbDxe.inf`
* **Module type:** `DXE_DRIVER`

Manages the Fault Resilient Boot (FRB2) watchdog during DXE. It checks and
clears watchdog expiration state and registers a Ready To Boot notification
that disables the firmware FRB2 timer once firmware boot processing has
completed.

### FrbPei

* **INF:** `Frb/FrbPei.inf`
* **Module type:** `PEIM`

Configures and starts the BMC FRB2 watchdog during PEI. The watchdog provides a
recovery action if firmware does not progress from the early boot phase within
the platform-configured timeout.

### DxeGenericElog

* **INF:** `GenericElog/Dxe/GenericElog.inf`
* **Module type:** `DXE_DRIVER`

Publishes the generic DXE event-log protocol and discovers event-log
redirection providers such as `DxeBmcElog`. Calls to set, get, erase, or
activate an event log are routed to the first provider that supports the
requested event-log data type.

### SmmGenericElog

* **INF:** `GenericElog/Smm/GenericElog.inf`
* **Module type:** `DXE_SMM_DRIVER`

Provides the traditional MM generic event-log aggregation protocol. It
discovers MM event-log redirection providers and routes event-log operations
to the provider that supports the requested data type.

### StandaloneMmGenericElog

* **INF:** `GenericElog/Smm/GenericElogStandaloneMm.inf`
* **Module type:** `MM_STANDALONE`

Provides the Standalone MM implementation of the generic event-log aggregation
protocol. It offers the same provider discovery and event routing as the
traditional MM driver using Standalone MM services.

### GenericFru

* **INF:** `GenericFru/GenericFru.inf`
* **Module type:** `DXE_RUNTIME_DRIVER`

Publishes a runtime-capable generic Field Replaceable Unit (FRU) protocol. It
discovers FRU redirection providers and routes FRU information, read, and write
requests to the provider matching the requested FRU type, including after the
virtual-address transition.

### DxeGenericIpmi

* **INF:** `GenericIpmi/Dxe/GenericIpmi.inf`
* **Module type:** `DXE_DRIVER`

Initializes the DXE IPMI physical transport and publishes the IPMI transport
protocols used by DXE clients. The transport provides the legacy command path
and the multi-interface Transport2 path, with interface selection controlled
by package PCDs.

### PeiGenericIpmi

* **INF:** `GenericIpmi/Pei/PeiGenericIpmi.inf`
* **Module type:** `PEIM`

Initializes IPMI communication during PEI and publishes both legacy and
Transport2 PPIs. It configures enabled KCS, BT, SSIF, and IPMB interfaces,
tracks the transport instance in a GUID HOB, and refreshes interfaces that
become usable after permanent memory is available.

### SmmGenericIpmi

* **INF:** `GenericIpmi/Smm/SmmGenericIpmi.inf`
* **Module type:** `DXE_SMM_DRIVER`

Publishes the traditional MM legacy and Transport2 IPMI protocols. It obtains
the DXE transport configuration through MM communication and exposes command
submission to MM clients using the configured BMC interfaces.

### StandaloneMmGenericIpmi

* **INF:** `GenericIpmi/StandaloneMm/StandaloneMmGenericIpmi.inf`
* **Module type:** `MM_STANDALONE`

Publishes the Standalone MM legacy and Transport2 IPMI protocols. It provides
the Standalone MM equivalent of the SMM transport service so MM components can
submit commands to the BMC.

### DxeIpmiInit

* **INF:** `IpmiInit/DxeIpmiInit.inf`
* **Module type:** `DXE_DRIVER`

Validates BMC availability during DXE by retrieving the device ID with retries
and checking whether the controller is in firmware-update mode. When the BMC
is operational, it also retrieves and reports the BMC self-test result.

### PeiIpmiInit

* **INF:** `IpmiInit/PeiIpmiInit.inf`
* **Module type:** `PEIM`

Performs the early-boot BMC device-ID and self-test checks through the PEI IPMI
transport. It establishes the BMC state needed by later PEI server-management
features and avoids normal initialization when the controller reports
firmware-update mode.

### IpmiRedirFru

* **INF:** `IpmiRedirFru/IpmiRedirFru.inf`
* **Module type:** `DXE_DRIVER`

Discovers IPMI FRU devices and publishes a FRU redirection protocol for the
generic FRU layer. It supports fragmented BMC FRU reads and writes and adds
selected FRU information to SMBIOS after the console-ready boot event.

### OsWdt

* **INF:** `OsWdt/OsWdt.inf`
* **Module type:** `DXE_DRIVER`

Registers an Exit Boot Services handler for the operating-system watchdog. If
the platform enables the feature, the handler programs the BMC watchdog for
the OS-loader use case with a hard-reset expiration action.

### SolStatus

* **INF:** `SolStatus/SolStatus.inf`
* **Module type:** `DXE_DRIVER`

Queries the BMC Serial over LAN (SOL) configuration for each configured
channel and reports whether SOL is enabled. The implementation also contains
the retrying helpers used to read and update SOL configuration parameters.

## Libraries

The package contains the following library instances. **Module type** is the
library instance's own `MODULE_TYPE`; **target consumers** lists the module
types declared after the library class in `LIBRARY_CLASS`. An unrestricted
target means the INF does not constrain the consuming module type.

### BmcCommonInterfaceLib

* **INF:** `Library/BmcInterfaceCommonAccess/BmcCommonInterfaceLib.inf`
* **Library class:** `BmcCommonInterfaceLib`
* **Module type:** `BASE`
* **Target consumers:** Unrestricted

Provides functionality shared by the BT, SSIF, and IPMB transports. This
includes I/O and MMIO register access, transport locking, soft-error tracking,
BMC self-test processing, and common request and response handling used by the
individual physical-interface libraries.

### BtInterfaceLib

* **INF:** `Library/BmcInterfaceCommonAccess/BtInterfaceLib/BtInterfaceLib.inf`
* **Library class:** `BtInterfaceLib`
* **Module type:** `BASE`
* **Target consumers:** Unrestricted

Implements the IPMI Block Transfer (BT) physical interface. It manages BT
control and data ports, host-to-BMC and BMC-to-host handshakes, buffering,
timeouts, retries, and Transport2 command submission over either I/O or MMIO
access.

### DxeIpmbInterfaceLib

* **INF:** `Library/BmcInterfaceCommonAccess/IpmbInterfaceLib/DxeIpmbInterfaceLib.inf`
* **Library class:** `IpmbInterfaceLib`
* **Module type:** `DXE_DRIVER`
* **Target consumers:** `DXE_DRIVER`, `DXE_RUNTIME_DRIVER`, `UEFI_DRIVER`, `UEFI_APPLICATION`

Implements IPMB command framing, checksums, and response handling for DXE and
UEFI consumers. This instance locates the DXE I2C Master protocol, binds it to
the Transport2 interface, performs BMC self-test validation, and sends IPMB
transactions through that protocol.

### PeiIpmbInterfaceLib

* **INF:** `Library/BmcInterfaceCommonAccess/IpmbInterfaceLib/PeiIpmbInterfaceLib.inf`
* **Library class:** `IpmbInterfaceLib`
* **Module type:** `PEIM`
* **Target consumers:** `PEIM`

Provides the PEI implementation of the IPMB transport. It combines the common
IPMB packet and checksum logic with the PEI I2C Master PPI so PEIMs can
initialize the interface and exchange IPMI messages over I2C.

### SmmIpmbInterfaceLib

* **INF:** `Library/BmcInterfaceCommonAccess/IpmbInterfaceLib/SmmIpmbInterfaceLib.inf`
* **Library class:** `IpmbInterfaceLib`
* **Module type:** `DXE_SMM_DRIVER`
* **Target consumers:** `DXE_SMM_DRIVER`, `MM_STANDALONE`

Provides the MM implementation of the IPMB transport. It combines the common
IPMB packet and checksum logic with the MM I2C Master protocol to initialize
and operate an IPMB interface for both traditional and Standalone MM clients.

### DxeSsifInterfaceLib

* **INF:** `Library/BmcInterfaceCommonAccess/SsifInterfaceLib/DxeSsifInterfaceLib.inf`
* **Library class:** `SsifInterfaceLib`
* **Module type:** `DXE_DRIVER`
* **Target consumers:** `DXE_DRIVER`, `DXE_RUNTIME_DRIVER`, `UEFI_DRIVER`, `UEFI_APPLICATION`

Implements the SMBus System Interface (SSIF) for DXE and UEFI consumers. It
uses the DXE SMBus Host Controller protocol and the common SSIF multipart,
retry, PEC, alert, capability, and response handling to expose SSIF through
Transport2.

### PeiSsifInterfaceLib

* **INF:** `Library/BmcInterfaceCommonAccess/SsifInterfaceLib/PeiSsifInterfaceLib.inf`
* **Library class:** `SsifInterfaceLib`
* **Module type:** `PEIM`
* **Target consumers:** `PEIM`

Provides the PEI implementation of SSIF using the SMBus Host Controller PPI.
It supplies PEIMs with the common SSIF single-part and multipart command flow,
retry handling, and BMC interface initialization.

### SmmSsifInterfaceLib

* **INF:** `Library/BmcInterfaceCommonAccess/SsifInterfaceLib/SmmSsifInterfaceLib.inf`
* **Library class:** `SsifInterfaceLib`
* **Module type:** `DXE_SMM_DRIVER`
* **Target consumers:** `DXE_SMM_DRIVER`, `MM_STANDALONE`

Provides SSIF services to traditional and Standalone MM consumers through the
MM SMBus Host Controller protocol. It reuses the common SSIF transaction,
multipart transfer, retry, alert, and response-validation logic.

### IpmiBaseLib

* **INF:** `Library/IpmiBaseLib/IpmiBaseLib.inf`
* **Library class:** `IpmiBaseLib`
* **Module type:** `UEFI_DRIVER`
* **Target consumers:** `DXE_RUNTIME_DRIVER`, `DXE_DRIVER`, `UEFI_APPLICATION`, `UEFI_DRIVER`

Provides the standard IPMI command API to DXE and UEFI modules. It locates the
DXE IPMI transport protocol, forwards command submissions, and exposes BMC
status and communication-address queries without requiring clients to manage
the protocol directly.

### IpmiBaseLibNull

* **INF:** `Library/IpmiBaseLibNull/IpmiBaseLibNull.inf`
* **Library class:** `IpmiBaseLib`
* **Module type:** `DXE_SMM_DRIVER`
* **Target consumers:** `DXE_CORE`, `DXE_DRIVER`, `DXE_RUNTIME_DRIVER`, `DXE_SMM_DRIVER`, `UEFI_APPLICATION`, `UEFI_DRIVER`, `SMM_CORE`

Provides a null implementation of the `IpmiBaseLib` API for modules that must
link without a functional IPMI transport. Initialization, command submission,
and status queries return success without communicating with a BMC.

### IpmiPlatformHookLibNull

* **INF:** `Library/IpmiPlatformHookLibNull/IpmiPlatformHookLibNull.inf`
* **Library class:** `IpmiPlatformHookLib`
* **Module type:** `BASE`
* **Target consumers:** Unrestricted

Provides the default no-op platform hook used when enabling an IPMI I/O range.
Platforms that require chipset-specific decode programming can replace this
instance with their own `IpmiPlatformHookLib` implementation.

### PeiIpmiBaseLib

* **INF:** `Library/PeiIpmiBaseLib/PeiIpmiBaseLib.inf`
* **Library class:** `IpmiBaseLib`
* **Module type:** `PEIM`
* **Target consumers:** `PEIM`, `PEI_CORE`

Provides the standard IPMI command API during PEI. It locates the PEI IPMI
transport PPI and forwards command and BMC-status requests for PEIM and PEI
core consumers.

### ServerManagementLib

* **INF:** `Library/ServerManagementLib/ServerManagementLib.inf`
* **Library class:** `ServerManagementLib`
* **Module type:** `BASE`
* **Target consumers:** Unrestricted

Provides server-management utility APIs for event logging and time conversion.
It locates the generic DXE event-log protocol, wraps event-log set, get, erase,
and activation operations, and converts RTC date and time values to the
32-bit Unix timestamp format used by BMC commands.

### StandaloneMmServerManagementLib

* **INF:** `Library/ServerManagementLib/StandaloneMmServerManagementLib.inf`
* **Library class:** `ServerManagementLib`
* **Module type:** `MM_STANDALONE`
* **Target consumers:** Unrestricted

Provides the Standalone MM build of the server-management time utilities. It
supplies the timestamp conversion needed by Standalone MM BMC event-log
components without depending on DXE boot services or the DXE generic event-log
protocol.

### ServerManagementLibNull

* **INF:** `Library/ServerManagementLibNull/ServerManagementLibNull.inf`
* **Library class:** `ServerManagementLib`
* **Module type:** `BASE`
* **Target consumers:** Unrestricted

Provides a nonfunctional server-management library instance for configurations
without event-log or RTC support. Event-log and timestamp operations return
unsupported or not-found results, allowing dependent modules to link while
making the unavailable service explicit.

### SmmIpmiBaseLib

* **INF:** `Library/SmmIpmiBaseLib/SmmIpmiBaseLib.inf`
* **Library class:** `IpmiBaseLib`
* **Module type:** `DXE_SMM_DRIVER`
* **Target consumers:** `DXE_SMM_DRIVER`, `SMM_CORE`, `MM_STANDALONE`, `MM_CORE_STANDALONE`

Provides the standard IPMI command API to traditional and Standalone MM
modules. It locates or waits for the MM IPMI transport protocol and forwards
command submission and BMC-status requests through the MM services table.

## Key Functions

* `IpmiSubmitCommand` sends an IPMI command through the legacy PEI, DXE, or MM
  transport interface.
* `GetBmcStatus` returns the current BMC status and communication address from
  the legacy transport.
* `IpmiSubmitCommand2` sends a command through the interface selected by
  `PcdDefaultSystemInterface`.
* `IpmiSubmitCommand2Ex` sends a command through a caller-selected KCS, BT,
  SSIF, or IPMB interface.
* `SetEventLogData`, `GetEventLogData`, `EraseEventlogData`, and
  `ActivateEventLog` expose event-log operations through the generic ELOG PPI
  or protocol. The BMC redirection provider maps these operations to the SEL.
* `GetFruInfo`, `GetFruData`, and `SetFruData` expose runtime-capable FRU
  discovery and data access through the generic FRU protocol.
* `SetACPIPowerStateInBMC` reports ACPI system and device power-state changes
  through the BMC ACPI software-child policy protocol.

## Configuration

`PcdIpmiFeatureEnable` is the package feature flag. The package DSC builds the
feature unconditionally, so an integrating platform is responsible for using
the flag when selecting DSC and FDF includes.

The transport configuration must describe at least one interface. Enable the
supported physical interfaces with `PcdKcsInterfaceSupport`,
`PcdBtInterfaceSupport`, `PcdSsifInterfaceSupport`, and
`PcdIpmbInterfaceSupport`. Set `PcdDefaultSystemInterface` to an enabled
interface; otherwise Transport2 initialization or default command submission
can return `EFI_UNSUPPORTED`.

| Configuration Area | PCDs | Purpose |
| --- | --- | --- |
| KCS | `PcdIpmiIoBaseAddress`, `PcdIpmiSmmIoBaseAddress` | Selects the KCS base address used outside and inside MM. |
| Register access | `PcdIpmiDefaultAccessType`, `PcdMmioBaseAddress`, `PcdBaseAddressRange` | Selects I/O or MMIO access and describes the MMIO window. |
| BT | `PcdBtControlPort`, `PcdBtBufferPort`, `PcdBtInterruptMaskPort`, `PcdBtBufferSize`, `PcdBtCommandRetryCounter`, `PcdBtDelayPerRetry` | Describes BT registers, buffer capacity, and retry timing. |
| SSIF | `PcdSsifSlaveAddress`, `PcdSsifCommandtRetryCounter`, `PcdSsifRequestRetriesDelay` | Describes the BMC SMBus address and SSIF retry behavior. |
| IPMB | `PcdBmcSlaveAddress` | Selects the BMC I2C slave address used for IPMB. |
| BMC readiness | `PcdIpmiBmcReadyDelayTimer` | Limits how long initialization retries while waiting for the BMC. |
| Pre-boot signal | `PcdSignalPreBootToBmc`, `PcdSioMailboxBaseAddress` | Enables and locates the optional pre-boot mailbox signal. |
| FRB2 | `PcdFRB2EnabledFlag`, `PcdFRBTimeoutValue` | Enables the firmware watchdog and selects its timeout. |
| SOL | `PcdMaxSOLChannels` | Selects how many BMC channels are queried for SOL status. |

## Data Flows

### IPMI Command

1. A PEI, DXE, or MM client calls the legacy transport, Transport2 interface,
   or `IpmiBaseLib`.
2. GenericIpmi selects the default interface or the interface requested by an
   `IpmiSubmitCommand2Ex` caller.
3. The interface library formats and transfers the request using KCS/BT
   registers, the SMBus host-controller interface for SSIF, or the I2C master
   interface for IPMB.
4. The BMC returns a completion code and response payload. The interface
   library validates framing and buffer sizes, and the transport returns the
   resulting EFI status and data to the client.

### Event Log

1. A client calls the generic ELOG PPI or protocol.
2. GenericElog selects a redirection provider supporting the requested log
   type.
3. BmcElog converts the operation to IPMI platform-event or SEL commands and
   submits them through the phase-appropriate IPMI transport.
4. Record identifiers, log data, and errors return through the generic ELOG
   interface.

### FRU

1. A client calls the generic FRU runtime protocol.
2. GenericFru selects the redirection provider matching the FRU type.
3. IpmiRedirFru performs fragmented IPMI FRU reads or writes and returns the
   data through GenericFru. Selected FRU information is also published through
   SMBIOS during boot.

## Control Flows

1. `PeiGenericIpmi` initializes enabled interfaces, publishes the PEI transport
   PPIs, and stores its instance in a GUID HOB. SSIF and IPMB can be refreshed
   after permanent memory is discovered.
2. `PeiIpmiInit` checks the BMC device ID and self-test state.
   `FrbPei` optionally starts the FRB2 watchdog, and `PeiBmcElog` publishes
   early event logging.
3. `DxeGenericIpmi` publishes the DXE transport protocols. DXE consumers load
   according to their dependency expressions, including BMC initialization,
   event logging, FRU, ACPI, watchdog, and SOL services.
4. Traditional or Standalone MM GenericIpmi publishes the MM transport.
   MM BmcElog and GenericElog then expose event logging inside MM.
5. At Ready To Boot, `FrbDxe` disables the firmware FRB2 watchdog. At Exit Boot
   Services, `BmcAcpiState` reports S0/D0 and `OsWdt` can arm the OS-loader
   watchdog.

## Build Flows

Include `Include/IpmiFeature.dsc` from the platform DSC after defining
`PEI_ARCH` and `DXE_ARCH`. Include `Include/PreMemory.fdf` and
`Include/PostMemory.fdf` in suitable platform firmware volumes, and add the
Standalone MM modules separately when that execution environment is used.

The package-level build can be used as a compile check:

```text
build -p IpmiFeaturePkg/IpmiFeaturePkg.dsc -a IA32 -a X64 -t <TOOL_CHAIN_TAG>
```

Supported toolchain tags:

* VS2019
* CLANGPDB
* GCC5

## Test Point Results

The package does not contain a dedicated automated test suite. A package build
verifies compilation and library resolution, but transport and BMC behavior
must be validated on the target platform.

The minimum hardware test point is an IPMI Get Device ID command through the
configured default interface in PEI and DXE. A passing result returns
`EFI_SUCCESS`, a valid device ID and firmware revision, and no transport
timeout or BMC hard-fail status. Platforms that include MM support must repeat
the command through the MM transport.

When the corresponding modules are enabled, integration testing should also
add and retrieve a SEL record, read a known FRU area, verify the BMC ACPI table
in the operating system, confirm FRB2 is disabled at Ready To Boot, and verify
the configured OS watchdog behavior at Exit Boot Services.

## Functional Exit Criteria

1. The package builds for every architecture and toolchain used by the
   platform without unresolved library classes.
2. Every enabled physical interface initializes, and the configured default
   interface is one of those enabled interfaces.
3. Get Device ID and Get Self Test Results complete successfully in each
   enabled execution phase.
4. The BMC remains responsive across the PEI-to-DXE transition and, when used,
   across the DXE-to-MM transport handoff.
5. SEL set, get, erase, and activation operations work when event logging is
   included.
6. FRU information and data can be read, and writable FRU data can be updated,
   when FRU support is included.
7. FRB2, OS watchdog, SOL status, ACPI table publication, and ACPI power-state
   reporting behave according to the platform configuration.
8. A normal boot produces no unexpected IPMI timeout, BMC hard-fail, or
   unsupported-interface errors.

## Feature Enabling Checklist

1. Include `Include/IpmiFeature.dsc` and define the platform's `PEI_ARCH` and
   `DXE_ARCH`.
2. Include the pre-memory and post-memory FDF fragments in firmware volumes
   available during the matching boot phases.
3. If using Standalone MM, add the Standalone MM modules to the platform FDF
   and provide the required MM I2C/SMBus protocols.
4. Enable each physical BMC interface present on the board and select one of
   them with `PcdDefaultSystemInterface`.
5. Configure KCS/BT I/O or MMIO resources, the SSIF SMBus address, or the IPMB
   I2C address to match the board and BMC firmware.
6. Replace `IpmiPlatformHookLibNull` if chipset-specific I/O decode programming
   is required.
7. Configure BMC readiness, FRB2, OS watchdog, SOL, ACPI, event-log, and FRU
   behavior required by the platform.
8. Ensure the platform supplies the ACPI table FDF rule and all protocols
   required by the selected modules.
9. Build the firmware and complete the hardware test points and functional
   exit criteria above.

## Performance Impact

Successful IPMI commands normally add BMC transaction latency to the phase in
which they execute. The largest boot-time impact occurs when the BMC is not
ready or an enabled interface is absent, because initialization and interface
libraries can consume their configured retry counts and delays.

Measure the feature by comparing phase timestamps with IPMI enabled and
disabled, and by tracing the duration of BMC initialization, self-test, SEL,
FRU, SOL, and watchdog commands. Include failure-path measurements with the BMC
unavailable to verify that configured timeouts remain within platform boot
requirements.

Enable only interfaces and optional modules present on the platform. Retry and
BMC-ready timers may be reduced only after confirming that the target BMC
reliably initializes and responds within the new limits.

## Common Optimizations

* Disable unused KCS, BT, SSIF, and IPMB interfaces to avoid initialization and
  probing of hardware that is not present.
* Include only the event-log, FRU, ACPI, watchdog, and SOL modules required by
  the platform.
* Select the lowest reliable BMC-ready timeout and interface retry values for
  the target BMC firmware.
* Limit `PcdMaxSOLChannels` to the channels implemented by the BMC.
* Use the null library instances only for modules that intentionally do not
  require functional IPMI or server-management services.

## Component Summary

| Component | INF | Module Type |
| --- | --- | --- |
| BmcAcpi | `BmcAcpi/BmcAcpi.inf` | `DXE_DRIVER` |
| BmcAcpiState | `BmcAcpiState/BmcAcpiState.inf` | `DXE_DRIVER` |
| BmcAcpiSwChild | `BmcAcpiSwChild/BmcAcpiSwChild.inf` | `DXE_SMM_DRIVER` |
| BmcAcpiSwChildStandaloneMm | `BmcAcpiSwChild/BmcAcpiSwChildStandaloneMm.inf` | `MM_STANDALONE` |
| DxeBmcElog | `BmcElog/DxeBmcElog.inf` | `DXE_DRIVER` |
| PeiBmcElog | `BmcElog/PeiBmcElog.inf` | `PEIM` |
| SmmBmcElog | `BmcElog/SmmBmcElog.inf` | `DXE_SMM_DRIVER` |
| StandaloneMmBmcElog | `BmcElog/StandaloneMmBmcElog.inf` | `MM_STANDALONE` |
| FrbDxe | `Frb/FrbDxe.inf` | `DXE_DRIVER` |
| FrbPei | `Frb/FrbPei.inf` | `PEIM` |
| DxeGenericElog | `GenericElog/Dxe/GenericElog.inf` | `DXE_DRIVER` |
| SmmGenericElog | `GenericElog/Smm/GenericElog.inf` | `DXE_SMM_DRIVER` |
| StandaloneMmGenericElog | `GenericElog/Smm/GenericElogStandaloneMm.inf` | `MM_STANDALONE` |
| GenericFru | `GenericFru/GenericFru.inf` | `DXE_RUNTIME_DRIVER` |
| DxeGenericIpmi | `GenericIpmi/Dxe/GenericIpmi.inf` | `DXE_DRIVER` |
| PeiGenericIpmi | `GenericIpmi/Pei/PeiGenericIpmi.inf` | `PEIM` |
| SmmGenericIpmi | `GenericIpmi/Smm/SmmGenericIpmi.inf` | `DXE_SMM_DRIVER` |
| StandaloneMmGenericIpmi | `GenericIpmi/StandaloneMm/StandaloneMmGenericIpmi.inf` | `MM_STANDALONE` |
| DxeIpmiInit | `IpmiInit/DxeIpmiInit.inf` | `DXE_DRIVER` |
| PeiIpmiInit | `IpmiInit/PeiIpmiInit.inf` | `PEIM` |
| IpmiRedirFru | `IpmiRedirFru/IpmiRedirFru.inf` | `DXE_DRIVER` |
| OsWdt | `OsWdt/OsWdt.inf` | `DXE_DRIVER` |
| SolStatus | `SolStatus/SolStatus.inf` | `DXE_DRIVER` |

## Library Summary

| Library Instance | INF | Library Class | Module Type | Target Consumers |
| --- | --- | --- | --- | --- |
| BmcCommonInterfaceLib | `Library/BmcInterfaceCommonAccess/BmcCommonInterfaceLib.inf` | `BmcCommonInterfaceLib` | `BASE` | Unrestricted |
| BtInterfaceLib | `Library/BmcInterfaceCommonAccess/BtInterfaceLib/BtInterfaceLib.inf` | `BtInterfaceLib` | `BASE` | Unrestricted |
| DxeIpmbInterfaceLib | `Library/BmcInterfaceCommonAccess/IpmbInterfaceLib/DxeIpmbInterfaceLib.inf` | `IpmbInterfaceLib` | `DXE_DRIVER` | `DXE_DRIVER`, `DXE_RUNTIME_DRIVER`, `UEFI_DRIVER`, `UEFI_APPLICATION` |
| PeiIpmbInterfaceLib | `Library/BmcInterfaceCommonAccess/IpmbInterfaceLib/PeiIpmbInterfaceLib.inf` | `IpmbInterfaceLib` | `PEIM` | `PEIM` |
| SmmIpmbInterfaceLib | `Library/BmcInterfaceCommonAccess/IpmbInterfaceLib/SmmIpmbInterfaceLib.inf` | `IpmbInterfaceLib` | `DXE_SMM_DRIVER` | `DXE_SMM_DRIVER`, `MM_STANDALONE` |
| DxeSsifInterfaceLib | `Library/BmcInterfaceCommonAccess/SsifInterfaceLib/DxeSsifInterfaceLib.inf` | `SsifInterfaceLib` | `DXE_DRIVER` | `DXE_DRIVER`, `DXE_RUNTIME_DRIVER`, `UEFI_DRIVER`, `UEFI_APPLICATION` |
| PeiSsifInterfaceLib | `Library/BmcInterfaceCommonAccess/SsifInterfaceLib/PeiSsifInterfaceLib.inf` | `SsifInterfaceLib` | `PEIM` | `PEIM` |
| SmmSsifInterfaceLib | `Library/BmcInterfaceCommonAccess/SsifInterfaceLib/SmmSsifInterfaceLib.inf` | `SsifInterfaceLib` | `DXE_SMM_DRIVER` | `DXE_SMM_DRIVER`, `MM_STANDALONE` |
| IpmiBaseLib | `Library/IpmiBaseLib/IpmiBaseLib.inf` | `IpmiBaseLib` | `UEFI_DRIVER` | `DXE_RUNTIME_DRIVER`, `DXE_DRIVER`, `UEFI_APPLICATION`, `UEFI_DRIVER` |
| IpmiBaseLibNull | `Library/IpmiBaseLibNull/IpmiBaseLibNull.inf` | `IpmiBaseLib` | `DXE_SMM_DRIVER` | `DXE_CORE`, `DXE_DRIVER`, `DXE_RUNTIME_DRIVER`, `DXE_SMM_DRIVER`, `UEFI_APPLICATION`, `UEFI_DRIVER`, `SMM_CORE` |
| IpmiPlatformHookLibNull | `Library/IpmiPlatformHookLibNull/IpmiPlatformHookLibNull.inf` | `IpmiPlatformHookLib` | `BASE` | Unrestricted |
| PeiIpmiBaseLib | `Library/PeiIpmiBaseLib/PeiIpmiBaseLib.inf` | `IpmiBaseLib` | `PEIM` | `PEIM`, `PEI_CORE` |
| ServerManagementLib | `Library/ServerManagementLib/ServerManagementLib.inf` | `ServerManagementLib` | `BASE` | Unrestricted |
| StandaloneMmServerManagementLib | `Library/ServerManagementLib/StandaloneMmServerManagementLib.inf` | `ServerManagementLib` | `MM_STANDALONE` | Unrestricted |
| ServerManagementLibNull | `Library/ServerManagementLibNull/ServerManagementLibNull.inf` | `ServerManagementLib` | `BASE` | Unrestricted |
| SmmIpmiBaseLib | `Library/SmmIpmiBaseLib/SmmIpmiBaseLib.inf` | `IpmiBaseLib` | `DXE_SMM_DRIVER` | `DXE_SMM_DRIVER`, `SMM_CORE`, `MM_STANDALONE`, `MM_CORE_STANDALONE` |
