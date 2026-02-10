## @file
#  GNUmakefile for building C utilities.
#
#  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

MAKEROOT = $(EDK_TOOLS_PATH)/Source/C
ifneq ($(WORKSPACE),)
  BUILDROOT=$(WORKSPACE)/BaseTools/Build/Source/C
else
  BUILDROOT=$(abspath $(MAKEROOT))
endif

ifeq (Windows, $(findstring Windows,$(OS)))
  SEP:=$(shell echo \)
else
  SEP:=/
endif

GNU_MAKE_UTILS_PY := $(PYTHON_COMMAND) $(MAKEROOT)$(SEP)Makefiles$(SEP)GnuMakeUtils.py
MD := $(GNU_MAKE_UTILS_PY) md

APPLICATIONS = \
  FitGen \

SUBDIRS := $(APPLICATIONS)

$(APPLICATIONS): $(BUILDROOT)/bin

.PHONY: subdirs $(SUBDIRS)
subdirs: $(SUBDIRS)
$(SUBDIRS):
	$(MD) $(BUILDROOT)/$@
	$(MAKE) OBJDIR=$(BUILDROOT)/$@ BUILDROOT=$(BUILDROOT) -C $@

.PHONY: $(patsubst %,%-clean,$(sort $(SUBDIRS)))
$(patsubst %,%-clean,$(sort $(SUBDIRS))):
	-$(MAKE) OBJDIR=$(BUILDROOT)/$(@:-clean=) BUILDROOT=$(BUILDROOT) -C $(@:-clean=) clean

clean: $(patsubst %,%-clean,$(sort $(SUBDIRS)))

