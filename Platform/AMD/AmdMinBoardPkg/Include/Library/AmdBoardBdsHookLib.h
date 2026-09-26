/** @file
  Header file for BDS Hook Library

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2024 - 2025, Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Library/UefiLib.h>

/**
  This is the callback function for Bds Ready To Boot event.

  @param[in]  Event   Pointer to this event
  @param[in]  Context Event handler private data

  @retval None.
**/
VOID
EFIAPI
BdsReadyToBootCallback (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  );

/**
  This is the callback function for Smm Ready To Lock event.

  @param[in] Event      The Event this notify function registered to.
  @param[in] Context    Pointer to the context data registered to the Event.
**/
VOID
EFIAPI
BdsSmmReadyToLockCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  This is the callback function for PCI ENUMERATION COMPLETE.

  @param[in] Event      The Event this notify function registered to.
  @param[in] Context    Pointer to the context data registered to the Event.
**/
VOID
EFIAPI
BdsPciEnumCompleteCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Before console after trusted console event callback

  @param[in] Event      The Event this notify function registered to.
  @param[in] Context    Pointer to the context data registered to the Event.
**/
VOID
EFIAPI
BdsBeforeConsoleAfterTrustedConsoleCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Before console before end of DXE event callback

  @param[in] Event      The Event this notify function registered to.
  @param[in] Context    Pointer to the context data registered to the Event.
**/
VOID
EFIAPI
BdsBeforeConsoleBeforeEndOfDxeGuidCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  After console ready before boot option event callback

  @param[in] Event      The Event this notify function registered to.
  @param[in] Context    Pointer to the context data registered to the Event.
**/
VOID
EFIAPI
BdsAfterConsoleReadyBeforeBootOptionCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );
