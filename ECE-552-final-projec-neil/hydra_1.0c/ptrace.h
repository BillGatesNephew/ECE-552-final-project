/*
 * ptrace.h - pipeline tracing definitions and interfaces
 *
 * This file is based on the SimpleScalar (see below) distribution of
 * ptrace.h, but has been extensively modified by Kevin Skadron
 * to be part of the HydraScalar simulator.  
 * Revisions Copyright (C) 1998, 1999.
 * skadron@cs.princeton.edu
 *
 * This file is a part of the SimpleScalar tool suite written by
 * Todd M. Austin as a part of the Multiscalar Research Project.
 * 
 * The tool suite is currently maintained by Doug Burger and Todd M. Austin.
 * 
 * Copyright (C) 1994, 1995, 1996, 1997 by Todd M. Austin
 *
 * This source file is distributed "as is" in the hope that it will be
 * useful.  The tool set comes with no warranty, and no author or
 * distributor accepts any responsibility for the consequences of its
 * use. 
 * 
 * Everyone is granted permission to copy, modify and redistribute
 * this tool set under the following conditions:
 * 
 *    This source code is distributed for non-commercial use only. 
 *    Please contact the maintainer for restrictions applying to 
 *    commercial use.
 *
 *    Permission is granted to anyone to make or distribute copies
 *    of this source code, either as received or modified, in any
 *    medium, provided that all copyright notices, permission and
 *    nonwarranty notices are preserved, and that the distributor
 *    grants the recipient permission for further redistribution as
 *    permitted by this document.
 *
 *    Permission is granted to distribute this file in compiled
 *    or executable form under the same conditions that apply for
 *    source code, provided that either:
 *
 *    A. it is accompanied by the corresponding machine-readable
 *       source code,
 *    B. it is accompanied by a written offer, with no time limit,
 *       to give anyone a machine-readable copy of the corresponding
 *       source code in return for reimbursement of the cost of
 *       distribution.  This written offer must permit verbatim
 *       duplication by anyone, or
 *    C. it is distributed by someone who received only the
 *       executable form, and is accompanied by a copy of the
 *       written offer of source code that they received concurrently.
 *
 * In other words, you are welcome to use, share and improve this
 * source file.  You are forbidden to forbid anyone else to use, share
 * and improve what you give them.
 *
 * INTERNET: dburger@cs.wisc.edu
 * US Mail:  1210 W. Dayton Street, Madison, WI 53706
 *
 * $Id: ptrace.h,v 3.1 1998/04/17 16:51:35 skadron Exp $
 *
 * $Log: ptrace.h,v $
 * Revision 3.1  1998/04/17 16:51:35  skadron
 * Versions used thru Mar '98 (retstack sub, ruu_resub, ICS final manuscript)
 *
 * Revision 2.102  1998/01/08 22:53:19  skadron
 * Augmented to print resource overflows; resource occupancies every cycle
 *
 * Revision 2.101  1997/12/09 22:20:31  skadron
 * Version Pritpal used for ISCA-25 submission
 *
 * Revision 2.100  1997/12/03 20:05:46  skadron
 * Version Kevin used for ISCA-25 submission
 *
 * Revision 2.11  1997/09/11 20:43:47  skadron
 * Ptracing of new fork now includes forking branch PC plus dest PC
 *
 * Revision 2.10  1997/09/09 18:59:50  skadron
 * Made ptracing thread-aware
 *
 * Revision 2.9  1997/09/05 16:39:52  skadron
 * Changed numbers associated with PTRACE modes to allow some bit twiddling
 *
 * Revision 2.8  1997/09/02 15:55:21  skadron
 * Added functional-simulation ptracing mode -- just displays
 *    non-spec-mode information from ruu_dispatch()
 *
 * Revision 2.7  1997/08/20 15:29:20  skadron
 * Finished testing and cleaning up verbose mode
 *
 * Revision 2.6  1997/08/19 19:01:38  skadron
 * Pipetraceing now supports a verbose mode that shows operand register
 *    contents and destination register result.
 *
 * Revision 2.5  1997/08/18 20:53:03  skadron
 * Added a ptrace level: now user can choose whether to get baseline
 *    info, mem info, or verbose
 *
 * Revision 2.4  1997/08/14 20:53:27  skadron
 * added printing of mem addr/value on mem ops.
 *
 * Revision 2.3  1997/07/11 21:44:19  skadron
 * Updated to incorporate final changes for public 2.0 release
 *
 * Revision 2.2  1997/07/08 03:45:58  skadron
 * Updated to reflect Todd/Milo's version 2.0e; includes memory-leak fix
 *
 * Revision 1.1  1997/03/11  01:32:28  taustin
 * Initial revision
 *
 *
 */

#ifndef PTRACE_H
#define PTRACE_H

#include "ss.h"
#include "range.h"
#include "dlite.h"

/*
 * pipeline events:
 *
 *	+ <iseq> <pc> t<thread> <addr> <inst>	- new instruction def
 *	- <iseq> t<thread>			- instruction squashed/retired
 *	@ <cycle>				- new cycle def
 *	* <iseq> t<thread> <stage> <events>	- instruction stage transition
 *                                                (may include reg/mem values)
 *	f t<thread> <pc> <dest>			- forked thread to PC 'dest'
 *	s t<thread>				- squashed thread
 *	k t<thread>				- deallocated thread
 *	\/ <item>				- resource overflow
 *
 */

/*
	[IF]   [DA]   [EX]   [WB]   [CT]
         aa     dd     jj     ll     nn
         bb     ee     kk     mm     oo
         cc                          pp
 */

/* pipeline stages */
#define PST_IFETCH		"IF"
#define PST_DISPATCH		"DA"
#define PST_EXECUTE		"EX"
#define PST_WRITEBACK		"WB"
#define PST_COMMIT		"CT"

/* PIPELINE EVENTS .  Note misfetch recovery (except indirect jumps)
 * occurs in dispatch, so PEV_MPDETECT won't appear if direction was ok;
 * misfetch on an indir. jump is like a full mispredict. */
#define PEV_CACHEMISS		0x00000001	/* I/D-cache miss */
#define PEV_TLBMISS		0x00000002	/* I/D-tlb miss */
#define PEV_AGEN		0x00000010	/* address generation */
#define PEV_FORKED		0x00000100	/* branch forked */
#define PEV_MFOCCURED		0x00000200	/* misfetch (target bad) */
#define PEV_MPOCCURED		0x00000400	/* mis-pred branch occurred */
#define PEV_MPDETECT		0x00000800	/* mis-pred branch detected
						 * or forked branch resolved */
#define PEV_SPEC_MODE		0x10000000	/* this inst is on a
						 * misspeculated path */

/* pipetrace file */
extern FILE *ptrace_outfd;

/* is pipetracing active? */
extern int ptrace_active;

/* pipetracing level if active */
/* WARNING: Any modes with bit 0x4 == 1 are reserved for modes that 
 * use register operand and result values */
extern int ptrace_level;
#define PTRACE_OFF	0
#define PTRACE_SIMPLE	1
#define PTRACE_MEM	2
#define PTRACE_INVALID  3
#define PTRACE_VERBOSE 	4
#define PTRACE_FUNSIM	5
#define PTRACE_DECODE   6
#define PTRACE_N_MODES	'7'

#define PTRACE_VERBOSE_MASK 0x4

/* pipetracing range */
extern struct range_range_t ptrace_range;

/* one-shot switch for pipetracing */
extern int ptrace_oneshot;

/* open pipeline trace */
void
ptrace_open(char *level,		/* pipetracing level */
	    char *range,		/* trace range */
	    char *fname);		/* output filename */

/* close pipeline trace */
void
ptrace_close(void);

/* NOTE: pipetracing is a one-shot switch, since turning on a trace more than
   once will mess up the pipetrace viewer */
#define ptrace_check_active(PC, ICNT, CYCLE)				\
  ((ptrace_outfd != NULL						\
    && !range_cmp_range1(&ptrace_range, (PC), (ICNT), (CYCLE)))		\
   ? (!ptrace_oneshot ? (ptrace_active = ptrace_oneshot = ptrace_level) \
                      : FALSE)                                          \
   : (ptrace_active = FALSE))

/* main interfaces, with fast checks */
#define ptrace_newinst(A,B,C,D,E)					\
  if (ptrace_active) __ptrace_newinst((A),(B),(C),(D),(E))
#define ptrace_newuop(A,B,C,D,E)					\
  if (ptrace_active) __ptrace_newuop((A),(B),(C),(D),(E))
#define ptrace_newthread(A,B,C)						\
  if (ptrace_active) __ptrace_newthread((A),(B),(C))
#define ptrace_squashthread(A)						\
  if (ptrace_active) __ptrace_squashthread((A))
#define ptrace_killthread(A)						\
  if (ptrace_active) __ptrace_killthread((A))
#define ptrace_overflow(A)						\
  if (ptrace_active) __ptrace_overflow((A))
#define ptrace_endinst(A,B)						\
  if (ptrace_active) __ptrace_endinst((A),(B))
#define ptrace_newcycle(A,B,C,D)					\
  if (ptrace_active) __ptrace_newcycle((A),(B),(C),(D))
#define ptrace_newstage(A,B,C,D)					\
  if (ptrace_active) __ptrace_newstage((A),(B),(C),(D))
#define ptrace_newstage_mem(A,B,C,D,E,F)				\
  if (ptrace_active) __ptrace_newstage_mem((A),(B),(C),(D),(E),(F))
#define ptrace_newstage_verbose(A,B,C,D,E,F,G,H,I,J,K,L)                \
  if (ptrace_active)                                                    \
    __ptrace_newstage_verbose((A),(B),(C),(D),(E),(F),(G),(H),(I),(J),(K),(L))
#define ptrace_active(A,I,C)						\
  (ptrace_outfd != NULL	&& !range_cmp_range(&ptrace_range, (A), (I), (C)))

/* declare a new instruction */
void
__ptrace_newinst(unsigned int iseq,	/* instruction sequence number */
		 SS_INST_TYPE inst,	/* new instruction */
		 SS_ADDR_TYPE pc,	/* program counter of instruction */
		 int thread,		/* thread id */
		 SS_ADDR_TYPE addr);	/* address referenced, if load/store */

/* declare a new uop */
void
__ptrace_newuop(unsigned int iseq,	/* instruction sequence number */
		char *uop_desc,		/* new uop description */
		SS_ADDR_TYPE pc,	/* program counter of instruction */
		int thread,		/* thread id */
		SS_ADDR_TYPE addr);	/* address referenced, if load/store */

/* declare a new thread */
void
__ptrace_newthread(int thread, 		/* id of new thread */
		   SS_ADDR_TYPE brpc,	/* program counter of forking branch */
		   SS_ADDR_TYPE destpc);/* forked-to address */

/* declare squashing of a thread (but it might continue to exist as a zombie
 * for some time */
void
__ptrace_squashthread(int thread);	/* id of new thread */

/* declare deallocation of a thread */
void
__ptrace_killthread(int thread);	/* id of new thread */

/* declare a resource overflow */
void
__ptrace_overflow(char *item);		/* name of overflowed resource */

/* declare instruction retirement or squash */
void
__ptrace_endinst(unsigned int iseq,	/* instruction sequence number */
		 int thread);		/* thread id */

/* declare a new cycle */
void
__ptrace_newcycle(SS_TIME_TYPE cycle,	                 /* new cycle */
		  int RUU_occ, int LSQ_occ, int IFQ_occ);/* queue occupancies*/

/* indicate instruction transition to a new pipeline stage */
void
__ptrace_newstage(unsigned int iseq,	/* instruction sequence number */
		  int thread,		/* thread id */
		  char *pstage,		/* pipeline stage entered */
		  unsigned int pevents);/* pipeline events while in stage */

/* indicate instruction transition to a new pipeline stage */
void
__ptrace_newstage_mem(unsigned int iseq,/* instruction sequence number */
 		  int thread,		/* thread id */
		  char *pstage,		/* pipeline stage entered */
		  unsigned int pevents, /* pipeline events while in stage */
		  SS_ADDR_TYPE addr,	/* mem op's data addr */
		  unsigned int data);	/* value in that addr after mem op */

/* indicate instruction transition to a new pipeline stage */
void
__ptrace_newstage_verbose(unsigned int iseq, /* instruction sequence number */
		  int thread,		/* thread id */
		  char *pstage,		  /* pipeline stage entered */
		  unsigned int pevents,   /* pipeline events while in stage */
		  enum ss_opcode op,      /* op of instruction being printed */
		  int in1, int in2, int in3, /* input dependency numbers */
		  union dlite_reg_val_t *in_vals, /* input operand values */
		  int out1, int out2,        /* output dependency numbers */
		  union dlite_reg_val_t *out_vals);/* output values */

#endif /* PTRACE_H */
