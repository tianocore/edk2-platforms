/** @file
  Try to run Linux kernel.

  Copyright (c) 2022 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - mem     - Memory
    - Bpi    - Boot Parameter Interface
    - FwCfg    - FirmWare Configure
**/

#include <Library/QemuLoadImageLib.h>
#include <Library/ReportStatusCodeLib.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

/**
  Download the kernel, the initial ramdisk, and the kernel command line from
  QEMU's fw_cfg. Construct a minimal SimpleFileSystem that contains the two
  image files, and load and start the kernel from it.

  The kernel will be instructed via its command line to load the initrd from
  the same Simple FileSystem.

  @retval EFI_NOT_FOUND         Kernel image was not found.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
  @retval EFI_PROTOCOL_ERROR    Unterminated kernel command line.

  @return                       Error codes from any of the underlying
                                functions. On success, the function doesn't
                                return.
**/
EFI_STATUS
TryRunningQemuKernel (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  KernelImageHandle;

  Status = QemuLoadKernelImage (&KernelImageHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Signal the EFI_EVENT_GROUP_READY_TO_BOOT event.
  //
  EfiSignalEventReadyToBoot ();

  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_PC_READY_TO_BOOT_EVENT)
    );

  //
  // Start the image.
  //
  Status = QemuStartKernelImage (&KernelImageHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: QemuStartKernelImage(): %r\n",
      __func__,
      Status
      ));
  }

  QemuUnloadKernelImage (KernelImageHandle);

  return Status;
}
