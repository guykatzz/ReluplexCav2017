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

ROOT_DIR = .

SUBDIRS = \
	property1 \
	property2 \
	property3 \
	property4 \
	property5 \
	property6a \
	property6b \
	property7 \
	property8 \
	property9 \
	property10 \
	\
	adversarial \

all:
	rm -f bin/*
	cp property1/property1.elf bin
	cp property2/property2.elf bin
	cp property3/property3.elf bin
	cp property4/property4.elf bin
	cp property5/property5.elf bin
	cp property6a/property6a.elf bin
	cp property6b/property6b.elf bin
	cp property7/property7.elf bin
	cp property8/property8.elf bin
	cp property9/property9.elf bin
	cp property10/property10.elf bin
	cp adversarial/adversarial.elf bin
	@echo Done

include $(ROOT_DIR)/Rules.mk

#
# Local Variables:
# compile-command: "make -C . "
# End:
#
