/*
 * ptrace.c - pipeline tracing routines
 *
 * This file is based on the SimpleScalar (see below) distribution of
 * ptrace.c, but has been extensively modified by Kevin Skadron
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
 * $Id: ptrace.c,v 3.1 1998/04/17 16:51:35 skadron Exp $
 *
 * $Log: ptrace.c,v $
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
 * Revision 2.12  1997/09/11 20:43:42  skadron
 * Ptracing of new fork now includes forking branch PC plus dest PC
 *
 * Revision 2.11  1997/09/09 18:59:59  skadron
 * Made ptracing thread-aware
 *
 * Revision 2.10  1997/09/05 16:39:58  skadron
 * Changed numbers associated with PTRACE modes to allow some bit twiddling
 *
 * Revision 2.9  1997/09/02 18:26:27  skadron
 * Buglet fix: iseq numbers for PTRACE_FUNSIM are now properly sync'd
 *
 * Revision 2.8  1997/09/02 15:55:30  skadron
 * Added functional-simulation ptracing mode -- just displays
 *    non-spec-mode information from ruu_dispatch()
 *
 * Revision 2.7  1997/08/20 15:28:56  skadron
 * Finished testing and cleaning up verbose mode
 *
 * Revision 2.6  1997/08/19 19:01:30  skadron
 * Pipetraceing now supports a verbose mode that shows operand register
 *    contents and destination register result.
 *
 * Revision 2.5  1997/08/18 20:52:55  skadron
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
 * Revision 1.1  1997/03/11  01:32:15  taustin
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "misc.h"
#include "ss.h"
#include "range.h"
#include "dlite.h"
#include "ptrace.h"

/* pipetrace file */
FILE *ptrace_outfd = NULL;

/* is pipetracing active? */
int ptrace_active = 0;

/* pipetracing level if active
 *	0: off
 *	1: simple: shows only instructions' movement
 *	2: shows mem references
 * 	3: currently unused
 *	4: verbose: shows all instruction behavior, including reg operand
 * 	   values and instruction results 
 *	5: debug functional simulation
 */
int ptrace_level = 0;
int consec_seq = -1;

/* pipetracing range */
struct range_range_t ptrace_range;

/* one-shot switch for pipetracing */
int ptrace_oneshot = FALSE;

/* open pipeline trace */
void
ptrace_open(char *level,		/* pipetracing level */
	    char *fname,		/* output filename */
	    char *range)		/* trace range */
{
  char *errstr;

  /* parse the level */
  if (level[0] < '0' || level[0] >= PTRACE_N_MODES)
    fatal("ptrace level must be between 0 and %c", (PTRACE_N_MODES - 1));
  else
    ptrace_level = atoi(level);

  if (level[0] == '3')
    fatal("ptrace level 3 is currently unused");

  /* parse the output range */
  if (!range)
    {
      /* no range */
      errstr = range_parse_range(":", &ptrace_range);
      if (errstr)
	panic("cannot parse pipetrace range, use: {<start>}:{<end>}");
      ptrace_active = TRUE;
    }
  else
    {
      errstr = range_parse_range(range, &ptrace_range);
      if (errstr)
	fatal("cannot parse pipetrace range, use: {<start>}:{<end>}");
      ptrace_active = FALSE;
    }

  if (ptrace_range.start.ptype != ptrace_range.end.ptype)
    fatal("range endpoints are not of the same type");

  /* open output trace file */
  if (!fname || !strcmp(fname, "-") || !strcmp(fname, "stderr"))
    ptrace_outfd = stderr;
  else if (!strcmp(fname, "stdout"))
    ptrace_outfd = stdout;
  else
    {
      ptrace_outfd = fopen(fname, "w");
      if (!ptrace_outfd)
	fatal("cannot open pipetrace output file `%s'", fname);
    }
}

/* close pipeline trace */
void
ptrace_close(void)
{
  if (ptrace_outfd != NULL && ptrace_outfd != stderr && ptrace_outfd != stdout)
    fclose(ptrace_outfd);
}

/* declare a new instruction */
void
__ptrace_newinst(unsigned int iseq,	/* instruction sequence number */
		 SS_INST_TYPE inst,	/* new instruction */
		 SS_ADDR_TYPE pc,	/* program counter of instruction */
		 int thread,		/* thread id */
		 SS_ADDR_TYPE addr)	/* address referenced, if load/store */
{
  if (ptrace_level == PTRACE_FUNSIM || ptrace_level == PTRACE_DECODE)
    /* use different seq numbers that are more likely to correspond
     * across different configs */
    iseq = ++consec_seq;

  fprintf(ptrace_outfd, "+ %u 0x%08x t%02d 0x%08x ", iseq, pc, thread, addr);
  ss_print_insn(inst, addr, ptrace_outfd);
  fprintf(ptrace_outfd, "\n");

  if (ptrace_outfd == stderr || ptrace_outfd == stdout)
    fflush(ptrace_outfd);
}

/* declare a new uop */
void
__ptrace_newuop(unsigned int iseq,	/* instruction sequence number */
		char *uop_desc,		/* new uop description */
		SS_ADDR_TYPE pc,	/* program counter of instruction */
		int thread,		/* thread id */
		SS_ADDR_TYPE addr)	/* address referenced, if load/store */
{
  fprintf(ptrace_outfd, "+ %u 0x%08x t%02d 0x%08x [%s]\n", 
	  iseq, pc, thread, addr, uop_desc);

  if (ptrace_outfd == stderr || ptrace_outfd == stdout)
    fflush(ptrace_outfd);
}

/* declare a new thread */
void
__ptrace_newthread(int thread, 		/* id of new thread */
		   SS_ADDR_TYPE brpc,	/* program counter of forking branch */
		   SS_ADDR_TYPE destpc) /* forked-to address */
{
  fprintf(ptrace_outfd, "f t%02d 0x%08x -> 0x%08x\n", thread, brpc, destpc);

  if (ptrace_outfd == stderr || ptrace_outfd == stdout)
    fflush(ptrace_outfd);
}

/* declare squashing of a thread (but it might continue to exist as a zombie
 * for some time */
void
__ptrace_squashthread(int thread)	/* id of new thread */
{
  fprintf(ptrace_outfd, "s t%02d\n", thread);

  if (ptrace_outfd == stderr || ptrace_outfd == stdout)
    fflush(ptrace_outfd);
}

/* declare deallocation of a thread */
void
__ptrace_killthread(int thread)		/* id of new thread */
{
  fprintf(ptrace_outfd, "k t%02d\n", thread);

  if (ptrace_outfd == stderr || ptrace_outfd == stdout)
    fflush(ptrace_outfd);
}

/* declare a resource overflow */
void
__ptrace_overflow(char *item)		/* name of overflowed resource */
{
  fprintf(ptrace_outfd, "\\/ %s\n", item);

  if (ptrace_outfd == stderr || ptrace_outfd == stdout)
    fflush(ptrace_outfd);
}

/* declare instruction retirement or squash */
void
__ptrace_endinst(unsigned int iseq,	/* instruction sequence number */
		 int thread)		/* thread id */
{
  fprintf(ptrace_outfd, "- %u t%02d\n", iseq, thread);

  if (ptrace_outfd == stderr || ptrace_outfd == stdout)
    fflush(ptrace_outfd);
}

/* declare a new cycle */
void
__ptrace_newcycle(SS_TIME_TYPE cycle,	                 /* new cycle */
		  int RUU_occ, int LSQ_occ, int IFQ_occ) /* queue occupancies*/
{
  fprintf(ptrace_outfd, "@ %.0f\t ruu: %d,  lsq: %d,  ifq: %d\n", 
	  (double)cycle, RUU_occ, LSQ_occ, IFQ_occ);

  if (ptrace_outfd == stderr || ptrace_outfd == stdout)
    fflush(ptrace_outfd);
}

/* indicate instruction transition to a new pipeline stage */
void
__ptrace_newstage(unsigned int iseq,	/* instruction sequence number */
		  int thread,		/* thread id */
		  char *pstage,		/* pipeline stage entered */
		  unsigned int pevents) /* pipeline events while in stage */
{
  fprintf(ptrace_outfd, "* %u %s t%02d 0x%08x\n", iseq, pstage, thread,
	  pevents);

  if (ptrace_outfd == stderr || ptrace_outfd == stdout)
    fflush(ptrace_outfd);
}

/* indicate instruction transition to a new pipeline stage for a mem op:
 * print mem addr and data in that word */
void
__ptrace_newstage_mem(unsigned int iseq,/* instruction sequence number */
		  int thread,		/* thread id */
		  char *pstage,		/* pipeline stage entered */
		  unsigned int pevents, /* pipeline events while in stage */
		  SS_ADDR_TYPE addr,	/* mem op's data addr */
		  unsigned int data)	/* value in that addr after mem op */
{
  /* if ptrace is in simple mode, don't print extra mem info */
  if (ptrace_level == PTRACE_SIMPLE)
    {
      __ptrace_newstage(iseq, thread, pstage, pevents);
      return;
    }

  /* otherwise, print mem address and that location's contents after a  
   * load or store has been functionally simulated */
  fprintf(ptrace_outfd, "* %u %s t%02d 0x%08x mem[0x%08x] = 0x%08x\n", 
	  iseq, pstage, thread, pevents, addr, data);
  
  if (ptrace_outfd == stderr || ptrace_outfd == stdout)
    fflush(ptrace_outfd);
}

#define DHI			(0+32+32)
#define DLO			(1+32+32)
#define DFCC			(2+32+32)

/* helper function for ptrace_newstage_verbose to produce a register name 
 * given a dependency number */
void get_reg_name(char *regname, int regnum)
{
  if (regnum < 0)
    panic("Invalid reg num in get_regname(): %d", regnum);
  else if (regnum < SS_NUM_REGS)
    {
      if (regnum <= 9)
	sprintf(regname, " r%d", regnum);
      else
	sprintf(regname, "r%d", regnum);
    }
  else if (regnum < 2*SS_NUM_REGS)
    {
      int fregnum = regnum - SS_NUM_REGS;
      if (fregnum <= 9)
	sprintf(regname, " f%d", fregnum);
      else
	sprintf(regname, "f%d", fregnum);
    }
  else if (regnum == DHI)
    sprintf(regname, "%s", " hi");
  else if (regnum == DLO)
    sprintf(regname, "%s", " lo");
  else if (regnum == DFCC)
    sprintf(regname, "%s", "Fcc");
  else 
    panic("Invalid reg num in get_regname(): %d", regnum);
}

#define IS_FLOAT_NAME(NAME) ((NAME)[0] == 'f' || (NAME)[1] == 'f')

/* indicate instruction transition to a new pipeline stage and info
 * about that instruction's operands and result */
void
__ptrace_newstage_verbose(unsigned int iseq, /* instruction sequence number */
		  int thread,		/* thread id */
		  char *pstage,		  /* pipeline stage entered */
		  unsigned int pevents,   /* pipeline events while in stage */
		  enum ss_opcode op,      /* op of instruction being printed */
		  int in1, int in2, int in3, /* input dependency numbers */
		  union dlite_reg_val_t *in_vals, /* input operand values */
		  int out1, int out2,        /* output dependency numbers */
		  union dlite_reg_val_t *out_vals) /* output values */
{
  char reg_name[8];
  char reg_info_str[64];
  char fmtstr[256] = "* %u %s t%02d 0x%08x";
  char blank_str[32];
  int as_double = (SS_OP_FLAGS(op) & F_DDEP);

  sprintf(blank_str, " %17s", "");

  /* if ptrace is in simple mode, don't print extra info */
  if (ptrace_level == PTRACE_SIMPLE)
    {
      __ptrace_newstage(iseq, thread, pstage, pevents);
      return;
    }

  if (ptrace_level == PTRACE_FUNSIM || ptrace_level == PTRACE_DECODE)
    /* use different seq numbers that are more likely to correspond
     * across different configs.  Also don't print events, as they
     * can differ across different configs */
    {
      iseq = consec_seq;
      pevents = 0x0;
    }

  if (in1)
    {
      get_reg_name(reg_name, in1);
      if (IS_FLOAT_NAME(reg_name))
	sprintf(reg_info_str, " %s=%13.6E", reg_name, 
		(as_double 
		 ? in_vals[0].as_double : (double)in_vals[0].as_float));
      else
	sprintf(reg_info_str, " %s= 0x%08x  ", reg_name, in_vals[0].as_word);
      strcat(fmtstr, reg_info_str);
    }
  else if (!in3)
    strcat(fmtstr, blank_str);
  if (in2)
    {
      get_reg_name(reg_name, in2);
      if (IS_FLOAT_NAME(reg_name))
	sprintf(reg_info_str, " %s=%13.6E", reg_name, 
		(as_double 
		 ? in_vals[1].as_double : (double)in_vals[1].as_float));
      else
	sprintf(reg_info_str, " %s= 0x%08x  ", reg_name, in_vals[1].as_word);
      strcat(fmtstr, reg_info_str);
    }
  else
    strcat(fmtstr, blank_str);
  if (in3)
    {
      get_reg_name(reg_name, in3);
      if (IS_FLOAT_NAME(reg_name))
	sprintf(reg_info_str, " %s=%13.6E", reg_name, 
		(as_double 
		 ? in_vals[2].as_double : (double)in_vals[2].as_float));
      else
	sprintf(reg_info_str, " %s= 0x%08x  ", reg_name, in_vals[2].as_word);
      strcat(fmtstr, reg_info_str);
    }

  strcat(fmtstr, " -> ");

  if (out1)
    {
      get_reg_name(reg_name, out1);
      if (IS_FLOAT_NAME(reg_name))
	sprintf(reg_info_str, "%s=%13.6E", reg_name,
		(as_double 
		 ? out_vals[0].as_double : (double)out_vals[0].as_float));
      else
	sprintf(reg_info_str, "%s= 0x%08x  ", reg_name, out_vals[0].as_word);
      strcat(fmtstr, reg_info_str);
    }
  else
    strcat(fmtstr, blank_str);
  if (out2)
    {
      get_reg_name(reg_name, out2);
      if (IS_FLOAT_NAME(reg_name))
	sprintf(reg_info_str, " %s=%13.6E", reg_name,
		(as_double 
		 ? out_vals[1].as_double : (double)out_vals[1].as_float));
      else
	sprintf(reg_info_str, " %s= 0x%08x  ", reg_name, out_vals[1].as_word);
      strcat(fmtstr, reg_info_str);
    }

  strcat(fmtstr, "\n");
  fprintf(ptrace_outfd, fmtstr, iseq, pstage, thread, pevents);
      
  if (ptrace_outfd == stderr || ptrace_outfd == stdout)
    fflush(ptrace_outfd);
}
