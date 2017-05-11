# \file Makefile
# \verbatim
# Top contributors (to current version):
#   Guy Katz
# This file is part of the Reluplex project.
# Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
# (in the top-level source directory) and their institutional affiliations.
# All rights reserved. See the file COPYING in the top-level source
# directory for licensing information.\endverbatim
#

ROOT_DIR = ..
PROJECT_DIR = $(ROOT_DIR)/..
GLPK_DIR = $(PROJECT_DIR)/glpk-4.60

SUBDIRS += \

LOCAL_INCLUDES += \
	$(PROJECT_DIR)/common \
	$(GLPK_DIR)/installed/include \
	$(PROJECT_DIR)/reluplex \
	$(PROJECT_DIR)/nnet \

LINK_FLAGS += \
	-L$(GLPK_DIR)/installed/lib

LOCAL_LIBRARIES += \
	glpk

SOURCES += \
	nnet.cpp \
	main.cpp \

TARGET = property6a.elf

include $(ROOT_DIR)/Rules.mk

vpath %.cpp $(PROJECT_DIR)/nnet

#
# Local Variables:
# compile-command: "make -C .. "
# End:
#
