/*****************************************************************************
 * Copyright Advanced Micro Devices, Inc. All rights reserved.
 *
*****************************************************************************
*/
/* $NoKeywords:$ */
/**
 * @file
 *
 * AMD Emulation environment auto detection
 *
 *
 * @xrefitem bom "File Content Label" "Release Content"
 * @e project:      AGESA
 * @e sub-project:  IDS
 * @e \$Revision: 309090 $   @e \$Date: 2014-12-10 02:28:05 +0800 (Wed, 10 Dec 2014) $
 */

#ifndef AMD_IDS_EMULATION_AUTO_DETEC_H
#define AMD_IDS_EMULATION_AUTO_DETEC_H

/**
 *      Detect the system is emulation or real platform.
 *
 *
 *  @retval       TRUE    The system is emulation
 *  @retval       FALSE   The system is real platform
 *
 **/
BOOLEAN
AmdIdsEmulationAutoDetect (
  VOID
  );

#endif // AMD_IDS_EMULATION_AUTO_DETEC_H




