# @file
# GNUmakefile for building the FitGen utility.
#
# Copyright (c) 2010-2019, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
ifeq (Windows, $(findstring Windows,$(MAKE_HOST)))
SEP:=$(shell echo \)
MAKEROOT ?= $(EDK_TOOLS_PATH)$(SEP)Source$(SEP)C
else
MAKEROOT ?= $(EDK_TOOLS_PATH)/Source/C
endif

APPNAME = FitGen

OBJECTS = FitGen.o

include $(MAKEROOT)/Makefiles/app.makefile

LIBS = -lCommon

