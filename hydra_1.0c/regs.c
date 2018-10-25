/*
 * regs.c - architected registers state routines
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
 * $Id: regs.c,v 3.1 1998/04/17 16:51:35 skadron Exp $
 *
 * $Log: regs.c,v $
 * Revision 3.1  1998/04/17 16:51:35  skadron
 * Versions used thru Mar '98 (retstack sub, ruu_resub, ICS final manuscript)
 *
 * Revision 2.101  1997/12/09 22:20:31  skadron
 * Version Pritpal used for ISCA-25 submission
 *
 * Revision 2.100  1997/12/03 20:05:46  skadron
 * Version Kevin used for ISCA-25 submission
 *
 * Revision 2.4  1997/08/14 20:55:04  skadron
 * Added set_regs_PC() get_regs_PC()
 *
 * Revision 2.3  1997/07/11 21:44:19  skadron
 * Updated to incorporate final changes for public 2.0 release
 *
 * Revision 2.2  1997/07/08 03:45:58  skadron
 * Updated to reflect Todd/Milo's version 2.0e; includes memory-leak fix
 *
 * Revision 1.4  1997/03/11  01:19:28  taustin
 * updated copyright
 * long/int tweaks made for ALPHA target support
 *
 * Revision 1.3  1997/01/06  16:02:36  taustin
 * comments updated
 *
 * Revision 1.1  1996/12/05  18:52:32  taustin
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "ss.h"
#include "loader.h"
#include "regs.h"

/* (signed) integer register file */
SS_WORD_TYPE regs_R[SS_NUM_REGS];

/* floating point register file */
union regs_FP regs_F;

/* (signed) hi register, holds mult/div results */
SS_WORD_TYPE regs_HI;
/* (signed) lo register, holds mult/div results */
SS_WORD_TYPE regs_LO;

/* floating point condition codes */
int regs_FCC;

/* program counter */
SS_ADDR_TYPE regs_PC;

/* set/get top-level regs_PC by function call (usually this would be done
 * directly by saying "regs_PC = ...", but some simulators might want
 * to shadow the top-level regs_PC */
void set_regs_PC(SS_ADDR_TYPE new_regs_PC)
{
  regs_PC = new_regs_PC;
}

SS_ADDR_TYPE get_regs_PC(void)
{
  return regs_PC;
}

/* initialize architected register state */
void
regs_init(void)
{
  int i;

  for (i=0; i<SS_NUM_REGS; i++)
    regs_F.l[i] = regs_R[i] = 0;
  regs_HI = 0;
  regs_LO = 0;
  regs_FCC = 0;
  regs_PC = 0;

  /* set up initial register state */
  regs_R[SS_STACK_REGNO] = ld_environ_base;
  regs_PC = ld_prog_entry;
}

/* dump all architected register state values to output stream STREAM */
void
regs_dump(FILE *stream)		/* output stream */
{
  int i;

  /* stderr is the default output stream */
  if (!stream)
    stream = stderr;

  /* dump processor register state */
  fprintf(stream, "Processor state:\n");
  fprintf(stream, "    PC: 0x%08x\n", regs_PC);
  for (i=0; i<SS_NUM_REGS; i += 2)
    {
      fprintf(stream, "    R[%2d]: %12d/0x%08x",
	      i, regs_R[i], regs_R[i]);
      fprintf(stream, "  R[%2d]: %12d/0x%08x\n",
	      i+1, regs_R[i+1], regs_R[i+1]);
    }
  fprintf(stream, "    HI:      %10d/0x%08x  LO:      %10d/0x%08x\n",
	  regs_HI, regs_HI, regs_LO, regs_LO);
  for (i=0; i<SS_NUM_REGS; i += 2)
    {
      fprintf(stream, "    F[%2d]: %12d/0x%08x",
	      i, regs_F.l[i], regs_F.l[i]);
      fprintf(stream, "  F[%2d]: %12d/0x%08x\n",
	      i+1, regs_F.l[i+1], regs_F.l[i+1]);
    }
  fprintf(stream, "    FCC:                0x%08x\n", regs_FCC);
}
