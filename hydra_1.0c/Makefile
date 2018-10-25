#
# Makefile - simulator suite make file
#
# This file is a part of the SimpleScalar tool suite written by
# Todd M. Austin as a part of the Multiscalar Research Project.
#  
# The tool suite is currently maintained by Doug Burger and Todd M. Austin.
# 
# Copyright (C) 1994, 1995, 1996, 1997 by Todd M. Austin
#
# This source file is distributed "as is" in the hope that it will be
# useful.  It is distributed with no warranty, and no author or
# distributor accepts any responsibility for the consequences of its
# use. 
#
# Everyone is granted permission to copy, modify and redistribute
# this source file under the following conditions:
#
#    This tool set is distributed for non-commercial use only. 
#    Please contact the maintainer for restrictions applying to 
#    commercial use of these tools.
#
#    Permission is granted to anyone to make or distribute copies
#    of this source code, either as received or modified, in any
#    medium, provided that all copyright notices, permission and
#    nonwarranty notices are preserved, and that the distributor
#    grants the recipient permission for further redistribution as
#    permitted by this document.
#
#    Permission is granted to distribute this file in compiled
#    or executable form under the same conditions that apply for
#    source code, provided that either:
#
#    A. it is accompanied by the corresponding machine-readable
#	source code,
#    B. it is accompanied by a written offer, with no time limit,
#	to give anyone a machine-readable copy of the corresponding
#	source code in return for reimbursement of the cost of
#	distribution.  This written offer must permit verbatim
#	duplication by anyone, or
#    C. it is distributed by someone who received only the
#	executable form, and is accompanied by a copy of the
#	written offer of source code that they received concurrently.
#
# In other words, you are welcome to use, share and improve this
# source file.	You are forbidden to forbid anyone else to use, share
# and improve what you give them.
#
# INTERNET: dburger@cs.wisc.edu
# US Mail:  1210 W. Dayton Street, Madison, WI 53706
#
# $Id: Makefile,v 3.16 1998/10/15 22:00:17 skadron Exp $
#
# $Log: Makefile,v $
# Revision 3.16  1998/10/15 22:00:17  skadron
# Added sim-class3
#
# Revision 3.15  1998/10/14 20:27:05  skadron
# Temp fix: khyb2
#
# Revision 3.14  1998/10/14 04:51:27  skadron
# Added sim-bpred-khyb stuff
#
# Revision 3.13  1998/10/13 23:59:09  skadron
# Added sim-bpred-hyb-mas-comp
#
# Revision 3.12  1998/10/08 02:04:45  skadron
# Added sim-class2
#
# Revision 3.11  1998/10/04 00:18:22  skadron
# Fixed Makefile to accommodate NT
#
# Revision 3.10  1998/10/03 23:49:40  skadron
# Changed sim-bpred to use bpred_small
#
# Revision 3.9  1998/09/29 04:39:21  skadron
# Added bpred-hyb-small
#
# Revision 3.8  1998/09/08 03:41:07  skadron
# Added sim-bpred-small
#
# Revision 3.7  1998/09/03 02:12:14  skadron
# Added sim-class and sim-bpred-test
#
# Revision 3.6  1998/05/06 22:54:31  skadron
# *** empty log message ***
#
# Revision 3.5  1998/04/27 17:48:36  skadron
# Parameterized "make" for portability
#
# Revision 3.4  1998/04/27 17:00:32  skadron
# Minor fix in debug compiling
#
# Revision 3.3  1998/04/23 17:17:25  skadron
# Fix of previous
#
# Revision 3.2  1998/04/22 18:41:18  skadron
# 1.  Added hydra
# 2.  Added EXT command-line option to incorporate verion tags on exes
#
# Revision 3.1  1998/04/17 16:51:35  skadron
# Versions used thru Mar '98 (retstack sub, ruu_resub, ICS final manuscript)
#

##################################################################
#
# Modify the following definitions to suit your build environment,
# NOTE: most platforms should not require any changes
#
##################################################################

#
# Insert your favorite C compiler, here.  NOTE: the SimpleScalar simulators
# must be compiled with an ANSI C compatible compiler.
#
CC = gcc

#
# Insert the name of RANLIB tool here, most machines can simply use "ranlib"
# machines that do not use RANLIB, should put "true" here
#
RANLIB = ranlib

#
# Insert your favorite make tool, here.	 Most anything should work.
#
MAKE = gmake

#
# Compilation-specific feature flags
#
# -DDEBUG	- turns on debugging features
# -DBFD_LOADER	- use libbfd.a to load programs (also required BINUTILS_INC
#		  and BINUTILS_LIB to be defined, see below)
#
FFLAGS =

#
# Choose your optimization level here
#
# for optimization:	OFLAGS = -O2 -g -finline-functions -funroll-loops
# for debug:		OFLAGS = -g -Wall
#
OPT = -O3
ifdef DEBUG
OFLAGS = -g -O0 -Wall 
else
OFLAGS = $(OPT) -g -finline-functions -funroll-loops
endif

#
# Point the Makefile to your Simplescalar-based bunutils
# Platform MUST be defined on command line; one of hp, sun, x86
# NOTE: these definitions are only required if BFD_LOADER is defined.
#
PLATFORM = 
BINUTILS_DIR = ../$(PLATFORM)/

#
# external libraries required for build
#
SIM_LIB = $(BINUTILS_DIR)/lib/libbfd.a $(BINUTILS_DIR)/lib/libiberty.a -lm

##################################################################
#
# YOU SHOULD NOT NEED TO MODIFY ANYTHING BELOW THIS COMMENT
#
##################################################################

#
# Machine-specific flags and libraries, generated by sysprobe
#
MLIBS  = `./sysprobe -libs`
ifeq ($(PLATFORM),sun)
MFLAGS = `./sysprobe -flags` -mv8
else
MFLAGS = `./sysprobe -flags`
endif

ifeq ($(PLATFORM),nt)
BINUTILS_DIR = //c/Cygnus/B19/binutils
SIM_LIB =
endif

#FIXME: I should eventually incorporate -DSS_(BIG,LITTLE) so that it's
#   determined automatically

# 
# User may add flags to CFLAGS by defining this on the command line
#
EXTRA_CFLAGS =

#
# complete flags
#
CFLAGS = $(MFLAGS) $(FFLAGS) $(OFLAGS) $(EXTRA_CFLAGS) \
 -I$(BINUTILS_DIR)/include

#
# all the sources
#
SIM_SRC = main.c sim-fast.c sim-safe.c sim-cache.c sim-profile.c \
	  sim-bpred.c sim-cheetah.c hydra.c syscall.c memory.c \
	  regs.c loader.c cache.c bpred.c bpred_small.c ptrace.c \
	  eventq.c resource.c \
	  endian.c dlite.c symbol.c eval.c options.c range.c stats.c \
	  ss.c endian.c misc.c bconf.c
SIM_HDR = syscall.h memory.h regs.h sim.h loader.h cache.h \
	  bpred.h bpred_small.h bconf.h ptrace.h \
	  eventq.h resource.h endian.h dlite.h symbol.h eval.h bitmap.h \
	  range.h version.h ss.h ss.def endian.h ecoff.h misc.h

#
# common objects
#
SIM_OBJ = main.o syscall.o memory.o regs.o loader.o ss.o endian.o dlite.o \
	  symbol.o eval.o options.o stats.o range.o misc.o

# Main target
ifdef DEBUG
target: announce hydraD
	@echo "my work is done here..."
else
target: announce hydra
	@echo "my work is done here..."
endif

#
# all targets
#
all: sim-fast sim-safe sim-profile sim-cheetah sim-bpred sim-cache sim-missr \
 sim-bmissr sim-cmissr hydra
	@echo "my work is done here..."

hydra: 

sysprobe:	sysprobe.c
	$(CC) $(FFLAGS) -o sysprobe sysprobe.c
	@echo endian probe results: `./sysprobe -s`
	@echo probe flags: `./sysprobe -flags`
	@echo probe libs: `./sysprobe -libs`

sim-fast:	sysprobe sim-fast.o $(SIM_OBJ)
	$(CC) -o sim-fast $(CFLAGS) sim-fast.o $(SIM_OBJ) $(SIM_LIB) $(MLIBS)

sim-safe:	sysprobe sim-safe.o $(SIM_OBJ)
	$(CC) -o sim-safe $(CFLAGS) sim-safe.o $(SIM_OBJ) $(SIM_LIB) $(MLIBS)

sim-profile:	sysprobe sim-profile.o $(SIM_OBJ)
	$(CC) -o sim-profile $(CFLAGS) sim-profile.o $(SIM_OBJ) $(SIM_LIB) \
	$(MLIBS)

sim-bpred:	sysprobe sim-bpred.o bpred.o $(SIM_OBJ)
	$(CC) -o sim-bpred $(CFLAGS) sim-bpred.o bpred.o $(SIM_OBJ) $(SIM_LIB)\
	$(MLIBS)

sim-cheetah:	sysprobe sim-cheetah.o libcheetah/libcheetah.a $(SIM_OBJ)
	$(CC) -o sim-cheetah $(CFLAGS) sim-cheetah.o $(SIM_OBJ) \
	libcheetah/libcheetah.a $(SIM_LIB) $(MLIBS)

sim-cache:	sysprobe sim-cache.o cache.o $(SIM_OBJ)
	$(CC) -o sim-cache $(CFLAGS) sim-cache.o cache.o $(SIM_OBJ) $(SIM_LIB)\
	$(MLIBS)

sim-outorder:	sysprobe sim-outorder.o cache.o bpred.o bconf.o resource.o ptrace.o $(SIM_OBJ) warmup-cache.o
	$(CC) -o sim-outorder `./sysprobe` $(CFLAGS) sim-outorder.o cache.o bpred.o bconf.o resource.o ptrace.o $(SIM_OBJ) warmup-cache.o $(SIM_LIB) $(MLIBS)

hydra:	sysprobe hydra.o cache.o bpred.o bconf.o resource.o ptrace.o $(SIM_OBJ) warmup-cache.o
	$(CC) -o hydra$(EXT) `./sysprobe` $(CFLAGS) hydra.o cache.o bpred.o bconf.o resource.o ptrace.o $(SIM_OBJ) warmup-cache.o $(SIM_LIB) $(MLIBS)

hydraD:	hydra
	mv hydra$(EXT) hydraD

announce:
	@echo "EXTRA_CFLAGS=$(EXTRA_CFLAGS)"

# This is to facilitate compiling a suite of simulators for my ICS runs
WORK: hydra1 hydra16 

hydra1: 
	rm -f hydra.o
	$(MAKE) PLATFORM=$(PLATFORM) EXTRA_CFLAGS="$(EXTRA_CFLAGS) -DCFG1"
	mv hydra$(EXT) hydra$(EXT).1T

hydra8: 
	rm -f hydra.o
	$(MAKE) PLATFORM=$(PLATFORM) EXTRA_CFLAGS="$(EXTRA_CFLAGS) -DCFG8"
	mv hydra$(EXT) hydra$(EXT).8T

hydra16: 
	rm -f hydra.o
	$(MAKE) PLATFORM=$(PLATFORM) EXTRA_CFLAGS="$(EXTRA_CFLAGS) -DCFG16"
	mv hydra$(EXT) hydra$(EXT).16T

hydra24: 
	rm -f hydra.o
	$(MAKE) PLATFORM=$(PLATFORM) EXTRA_CFLAGS="$(EXTRA_CFLAGS) -DCFG24"
	mv hydra$(EXT) hydra$(EXT).24T

hydra40: 
	rm -f hydra.o
	$(MAKE) PLATFORM=$(PLATFORM) EXTRA_CFLAGS="$(EXTRA_CFLAGS) -DCFG40"
	mv hydra$(EXT) hydra$(EXT).40T

hydra64: 
	rm -f hydra.o
	$(MAKE) PLATFORM=$(PLATFORM) EXTRA_CFLAGS="$(EXTRA_CFLAGS) -DCFG64"
	mv hydra$(EXT) hydra$(EXT).64T

libcheetah/libcheetah.a: libcheetah/ascbin.c libcheetah/din.c libcheetah/dmvl.c libcheetah/faclru.c libcheetah/facopt.c libcheetah/libcheetah.c libcheetah/pixie.c libcheetah/ppopt.c libcheetah/saclru.c libcheetah/sacopt.c libcheetah/util.c
	cd libcheetah; $(MAKE) "MAKE=$(MAKE)" "CC=$(CC)" "RANLIB=$(RANLIB)" "CFLAGS=$(FFLAGS) $(OFLAGS)" libcheetah.a

.c.o:
	$(CC) $(CFLAGS) -c $*.c

filelist:
	@echo $(SIM_SRC) $(SIM_HDR) Makefile

diffs:
	-rcsdiff RCS/*

sim-tests: sysprobe sim-fast sim-safe sim-cache sim-cheetah sim-bpred sim-profile hydra
	cd tests; $(MAKE) "MAKE=$(MAKE)" tests "SIM_DIR=.." "SIM_BIN=sim-fast"
	cd tests; $(MAKE) "MAKE=$(MAKE)" tests "SIM_DIR=.." "SIM_BIN=sim-safe"
	cd tests; $(MAKE) "MAKE=$(MAKE)" tests "SIM_DIR=.." "SIM_BIN=sim-cache"
	cd tests; $(MAKE) "MAKE=$(MAKE)" tests "SIM_DIR=.." "SIM_BIN=sim-cheetah"
	cd tests; $(MAKE) "MAKE=$(MAKE)" tests "SIM_DIR=.." "SIM_BIN=sim-bpred"
	cd tests; $(MAKE) "MAKE=$(MAKE)" tests "SIM_DIR=.." "SIM_BIN=sim-profile" "SIM_OPTS=-all"
	cd tests; $(MAKE) "MAKE=$(MAKE)" tests "SIM_DIR=.." "SIM_BIN=hydra"

sim-tests-nt: sysprobe sim-fast sim-safe sim-cache sim-cheetah sim-bpred sim-profile hydra
	cd tests; $(MAKE) "MAKE=$(MAKE)" tests "SIM_DIR=.." "REDIR=redir.bash" "SIM_BIN=sim-fast"
	cd tests; $(MAKE) "MAKE=$(MAKE)" tests "SIM_DIR=.." "REDIR=redir.bash" "SIM_BIN=sim-safe"
	cd tests; $(MAKE) "MAKE=$(MAKE)" tests "SIM_DIR=.." "REDIR=redir.bash" "SIM_BIN=sim-cache"
	cd tests; $(MAKE) "MAKE=$(MAKE)" tests "SIM_DIR=.." "REDIR=redir.bash" "SIM_BIN=sim-cheetah"
	cd tests; $(MAKE) "MAKE=$(MAKE)" tests "SIM_DIR=.." "REDIR=redir.bash" "SIM_BIN=sim-bpred"
	cd tests; $(MAKE) "MAKE=$(MAKE)" tests "SIM_DIR=.." "REDIR=redir.bash" "SIM_BIN=sim-profile" "SIM_OPTS=-all"
	cd tests; $(MAKE) "MAKE=$(MAKE)" tests "SIM_DIR=.." "REDIR=redir.bash" "SIM_BIN=hydra"

clean:
	rm -f *.o core *~ 

clobber: clean
	rm -f sim-fast sim-safe sim-profile sim-cheetah \
		sim-cache sim-inorder sim-outorder \
		hydra hydraD sysprobe

reallyclean: clean
	cd libcheetah; $(MAKE) clean
	cd tests; $(MAKE) clean

unpure:
	rm -f sim.pure *pure*.o sim.pure.pure_hardlink sim.pure.pure_linkinfo

depend:
	makedepend -n $(BINUTILS_INC) $(SIM_SRC)


# DO NOT DELETE THIS LINE -- make depend depends on it.

main.o: misc.h regs.h ss.h ss.def memory.h endian.h options.h stats.h eval.h
main.o: loader.h version.h dlite.h sim.h
sim-fast.o: misc.h ss.h ss.def regs.h memory.h endian.h options.h stats.h
sim-fast.o: eval.h loader.h syscall.h dlite.h sim.h
sim-safe.o: misc.h ss.h ss.def regs.h memory.h endian.h options.h stats.h
sim-safe.o: eval.h loader.h syscall.h dlite.h sim.h
sim-cache.o: misc.h ss.h ss.def regs.h memory.h endian.h options.h stats.h
sim-cache.o: eval.h cache.h loader.h syscall.h dlite.h sim.h
sim-cmissr.o: misc.h ss.h ss.def regs.h memory.h endian.h options.h stats.h
sim-cmissr.o: eval.h cache.h loader.h syscall.h dlite.h sim.h
sim-profile.o: misc.h ss.h ss.def regs.h memory.h endian.h options.h stats.h
sim-profile.o: eval.h loader.h syscall.h dlite.h symbol.h sim.h
sim-bpred.o: misc.h ss.h ss.def regs.h memory.h endian.h options.h stats.h
sim-bpred.o: eval.h loader.h syscall.h dlite.h bpred.h sim.h
sim-cheetah.o: misc.h ss.h ss.def regs.h memory.h endian.h options.h stats.h
sim-cheetah.o: eval.h loader.h syscall.h dlite.h libcheetah/libcheetah.h
sim-cheetah.o: sim.h
sim-outorder.o: misc.h ss.h ss.def regs.h memory.h endian.h options.h stats.h
sim-outorder.o: eval.h cache.h loader.h syscall.h bpred.h bconf.h resource.h bitmap.h
sim-outorder.o: ptrace.h range.h dlite.h sim.h 
hydra.o: misc.h ss.h ss.def regs.h memory.h endian.h options.h stats.h
hydra.o: eval.h cache.h loader.h syscall.h bpred.h bconf.h resource.h bitmap.h
hydra.o: ptrace.h range.h dlite.h sim.h 
syscall.o: misc.h ss.h ss.def regs.h memory.h endian.h options.h stats.h
syscall.o: eval.h loader.h sim.h syscall.h
memory.o: misc.h ss.h ss.def loader.h memory.h endian.h options.h stats.h
memory.o: eval.h regs.h
regs.o: misc.h ss.h ss.def loader.h memory.h endian.h options.h stats.h
regs.o: eval.h regs.h
loader.o: ecoff.h misc.h ss.h ss.def regs.h memory.h endian.h options.h
loader.o: stats.h eval.h sim.h loader.h
cache.o: misc.h ss.h ss.def cache.h memory.h endian.h options.h stats.h
cache.o: eval.h
bpred.o: misc.h ss.h ss.def bpred.h stats.h eval.h
bconf.o: misc.h ss.h bconf.h
ptrace.o: misc.h ss.h ss.def range.h ptrace.h
eventq.o: misc.h ss.h ss.def eventq.h bitmap.h
resource.o: misc.h resource.h
endian.o: loader.h ss.h ss.def memory.h endian.h options.h stats.h eval.h
dlite.o: misc.h version.h eval.h regs.h ss.h ss.def memory.h endian.h
dlite.o: options.h stats.h sim.h symbol.h loader.h range.h dlite.h
symbol.o: ecoff.h misc.h loader.h ss.h ss.def memory.h endian.h options.h
symbol.o: stats.h eval.h symbol.h
eval.o: misc.h eval.h
options.o: misc.h options.h
range.o: misc.h ss.h ss.def symbol.h loader.h memory.h endian.h options.h
range.o: stats.h eval.h range.h
stats.o: misc.h eval.h stats.h
ss.o: misc.h ss.h ss.def
endian.o: loader.h ss.h ss.def memory.h endian.h options.h stats.h eval.h
misc.o: misc.h
warmup-cache.o: misc.h ss.h ss.def regs.h memory.h endian.h options.h stats.h
warmup-cache.o: eval.h cache.h loader.h syscall.h dlite.h sim.h bpred.h bconf.h
