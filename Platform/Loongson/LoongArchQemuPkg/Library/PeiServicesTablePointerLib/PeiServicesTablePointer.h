/** @file
  PeiServicesTablePointer

  Copyright (c) 2022 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PEISERVICESTABLEPOINTER_H_
#define PEISERVICESTABLEPOINTER_H_

/**
  Write Csr KS0 register.

 @param A0 The value used to write to the KS0 register

  @retval none
**/
extern
VOID
LoongarchWriteqKs0 (
  IN UINT64  Val
  );

/**
  Read Csr KS0 register.

 @param  Val Pointer to the variable used to store the KS0 register value

  @retval none
**/
extern
VOID
LoongarchReadqKs0 (
  IN UINT64  *Val
  );

#endif // PEISERVICESTABLEPOINTER_H_
