/** @file

  Copyright (C) 2008-2025 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
/* $NoKeywords:$ */

/**
 * @file
 *
 * Collectively assign unique filecodes for assert and debug to each source file.
 *
 * Publish values for decorated filenames, which can be used for
 * ASSERT and debug support using a preprocessor define like:
 * @n <tt> _#define FILECODE MY_C_FILENAME_FILECODE </tt> @n
 * This file serves as a reference for debugging to associate the code and filename.
 *
 * @xrefitem bom "File Content Label" "Release Content"
 * @e project:      AGESA
 * @e sub-project:  Include
 * @e _$Revision: 312538 $   @e \$Date: 2015-02-09 16:53:54 +0800 (Mon, 09 Feb 2015) $
 */

#ifndef FILECODE_H_
#define FILECODE_H_

#define UNASSIGNED_FILE_FILECODE  (0xFFFF)

#endif // FILECODE_H_
