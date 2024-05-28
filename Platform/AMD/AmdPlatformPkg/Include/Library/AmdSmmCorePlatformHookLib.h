/** @file
  AMD Smm Core Platform Hook Library

  Copyright (C) 2023 - 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef AMD_SMM_CORE_PLATFORM_HOOK_LIB_
#define AMD_SMM_CORE_PLATFORM_HOOK_LIB_

/**
  This is the prototype of SMM Dispatcher hook before.

  @retval EFI_STATUS

**/
typedef
EFI_STATUS
(EFIAPI *AMD_SMM_DISPATCH_HOOK_BEFORE)(
  VOID
  );

/**
  This is the prototype of SMM Dispatcher after before.

  @retval EFI_STATUS

**/
typedef
EFI_STATUS
(EFIAPI *AMD_SMM_DISPATCH_HOOK_AFTER)(
  VOID
  );

typedef  UINT32 AMD_SMM_DISPATCH_HOOK_PRIORITY;

///
/// This is the structure of the SMM Dispatcher hook record
///
typedef struct {
  LIST_ENTRY                        NextList;                 ///< Point to next AMD_SMM_DISPATCH_HOOK.
  AMD_SMM_DISPATCH_HOOK_BEFORE      AmdSmmDispatchHookBefore; ///< The hook before function.
  AMD_SMM_DISPATCH_HOOK_AFTER       AmdSmmDispatchHookAfter;  ///< The hook after function.
  AMD_SMM_DISPATCH_HOOK_PRIORITY    Priority;                 ///< The priority of this hook instance.
                                                              ///< Priority could be implemented for
                                                              ///< future usage.
} AMD_SMM_DISPATCH_HOOK;

/**
  Register a SMM dispatcher hook.

  @param[in] SmmDispatchHookBefore  Function hook to SMM Dispatch before.
  @param[in] SmmDispatchHookAfter   Function hook to SMM Dispatch after.
  @param[in] Priority               The priority to execute the hook.

  @retval EFI_SUCCESS       The hook is registered successfully.

**/
EFI_STATUS
RegisterSmmDispatcherHook (
  IN  AMD_SMM_DISPATCH_HOOK_BEFORE    SmmDispatchHookBefore OPTIONAL,
  IN  AMD_SMM_DISPATCH_HOOK_AFTER     SmmDispatchHookAfter OPTIONAL,
  IN  AMD_SMM_DISPATCH_HOOK_PRIORITY  Priority
  );

#endif // AMD_SMM_CORE_PLATFORM_HOOK_LIB_
