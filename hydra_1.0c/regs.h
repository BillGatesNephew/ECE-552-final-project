/*
 * regs.h - architected register state interfaces
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
 * $Id: regs.h,v 3.1 1998/04/17 16:51:35 skadron Exp $
 *
 * $Log: regs.h,v $
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
 * Revision 2.1  1997/07/02 19:15:41  skadron
 * Last version used for Micro-30 submission
 *
 * Revision 1.1  1997/02/16  22:23:54  skadron
 * Initial revision
 *
 * Revision 1.3  1997/01/06  16:02:48  taustin
 * \comments updated
 *
 * Revision 1.1  1996/12/05  18:50:23  taustin
 * Initial revision
 *
 *
 */

#ifndef REGS_H
#define REGS_H

#include "ss.h"

/*
 * This module implements the SimpleScalar architected register state, which
 * includes integer and floating point registers and miscellaneous registers.
 * The architected register state is as follows:
 *
 * Integer Register File:                  Miscellaneous Registers:
 * (aka general-purpose registers, GPR's)
 *
 *   +------------------+                  +------------------+
 *   | $r0 (src/sink 0) |                  | PC               | Program Counter
 *   +------------------+                  +------------------+
 *   | $r1              |                  | HI               | Mult/Div HI val
 *   +------------------+                  +------------------+
 *   |  .               |                  | LO               | Mult/Div LO val
 *   |  .               |                  +------------------+
 *   |  .               |
 *   +------------------+
 *   | $r31             |
 *   +------------------+
 *
 * Floating point Register File:
 *    single-precision:  double-precision:
 *   +------------------+------------------+  +------------------+
 *   | $f0              | $f1 (for double) |  | FCC              | FP codes
 *   +------------------+------------------+  +------------------+
 *   | $f1              |
 *   +------------------+
 *   |  .               |
 *   |  .               |
 *   |  .               |
 *   +------------------+------------------+
 *   | $f30             | $f31 (for double)|
 *   +------------------+------------------+
 *   | $f31             |
 *   +------------------+
 *
 * The floating point register file can be viewed as either 32 single-precision
 * (32-bit IEEE format) floating point values $f0 to $f31, or as 16
 * double-precision (64-bit IEEE format) floating point values $f0 to $f30.
 */

/* (signed) integer register file */
extern SS_WORD_TYPE regs_R[SS_NUM_REGS];

/* floating point register file format */
union regs_FP {
    SS_WORD_TYPE l[SS_NUM_REGS];		/* integer word view */
    SS_FLOAT_TYPE f[SS_NUM_REGS];		/* single-precision FP view */
    SS_DOUBLE_TYPE d[SS_NUM_REGS/2];		/* double-precision FP view */
};

/* floating point register file */
extern union regs_FP regs_F;

/* (signed) hi register, holds mult/div results */
extern SS_WORD_TYPE regs_HI;

/* (signed) lo register, holds mult/div results */
extern SS_WORD_TYPE regs_LO;

/* floating point condition codes */
extern int regs_FCC;

/* program counter */
extern SS_ADDR_TYPE regs_PC;

/* set/get top-level regs_PC by function call (usually this would be done
 * directly by saying "regs_PC = ...", but some simulators might want
 * to shadow the top-level regs_PC */
void set_regs_PC(SS_ADDR_TYPE new_regs_PC);
SS_ADDR_TYPE get_regs_PC(void);

/* initialize architected register state */
void regs_init(void);

/* dump all architected register state values to output stream STREAM */
void
regs_dump(FILE *stream);	/* output stream */

#endif /* REGS_H */
