/** @file
*
*  Copyright (c) 2023-2024, Linaro Ltd. All rights reserved.<BR>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef SBSA_QEMU_SMC_H_
#define SBSA_QEMU_SMC_H_

#include <IndustryStandard/ArmStdSmc.h>

#define SIP_SVC_VERSION                SMC_SIP_FUNCTION_ID(1)
#define SIP_SVC_GET_GIC                SMC_SIP_FUNCTION_ID(100)
#define SIP_SVC_GET_GIC_ITS            SMC_SIP_FUNCTION_ID(101)
#define SIP_SVC_GET_CPU_COUNT          SMC_SIP_FUNCTION_ID(200)
#define SIP_SVC_GET_CPU_NODE           SMC_SIP_FUNCTION_ID(201)
#define SIP_SVC_GET_CPU_TOPOLOGY       SMC_SIP_FUNCTION_ID(202)
#define SIP_SVC_GET_MEMORY_NODE_COUNT  SMC_SIP_FUNCTION_ID(300)
#define SIP_SVC_GET_MEMORY_NODE        SMC_SIP_FUNCTION_ID(301)

/*
 *  SMCC does not define return codes for SiP functions.
 *  We use Architecture ones then.
 */

#define SMC_SIP_CALL_SUCCESS  SMC_ARCH_CALL_SUCCESS

#endif /* SBSA_QEMU_SMC_H_ */
