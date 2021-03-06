/*
 * endian.h - host endian probe interfaces
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
 * $Id: endian.h,v 3.1 1998/04/17 16:51:35 skadron Exp $
 *
 * $Log: endian.h,v $
 * Revision 3.1  1998/04/17 16:51:35  skadron
 * Versions used thru Mar '98 (retstack sub, ruu_resub, ICS final manuscript)
 *
 * Revision 2.101  1997/12/09 22:20:31  skadron
 * Version Pritpal used for ISCA-25 submission
 *
 * Revision 2.100  1997/12/03 20:05:46  skadron
 * Version Kevin used for ISCA-25 submission
 *
 * Revision 2.3  1997/07/11 21:44:19  skadron
 * Updated to incorporate final changes for public 2.0 release
 *
 * Revision 2.2  1997/07/08 03:45:58  skadron
 * Updated to reflect Todd/Milo's version 2.0e; includes memory-leak fix
 *
 * Revision 1.4  1997/03/11  01:10:30  taustin
 * updated copyright
 * long/int tweaks made for ALPHA target support
 * swapping supported disabled until it can be tested further
 *
 * Revision 1.3  1997/01/06  15:58:53  taustin
 * comments updated
 *
 * Revision 1.1  1996/12/05  18:50:23  taustin
 * Initial revision
 *
 *
 */

#ifndef ENDIAN_H
#define ENDIAN_H

/* data swapping functions, from big/little to little/big endian format */
#if 0 /* FIXME: disabled until further notice... */
#define __SWAP_HALF(N)	((((N) & 0xff) << 8) | (((unsigned short)(N)) >> 8))
#define SWAP_HALF(N)	(sim_swap_bytes ? __SWAP_HALF(N) : (N))

#define __SWAP_WORD(N)	(((N) << 24) |					\
			 (((N) << 8) & 0x00ff0000) |			\
			 (((N) >> 8) & 0x0000ff00) |			\
			 (((unsigned int)(N)) >> 24))
#define SWAP_WORD(N)	(sim_swap_bytes ? __SWAP_WORD(N) : (N))
#else
#define SWAP_HALF(N)	(N)
#define SWAP_WORD(N)	(N)
#endif

/* recognized endian formats */
enum endian_t { endian_big, endian_little, endian_unknown};
/* probe host (simulator) byte endian format */
enum endian_t
endian_host_byte_order(void);

/* probe host (simulator) double word endian format */
enum endian_t
endian_host_word_order(void);

#ifndef HOST_ONLY

/* probe target (simulated program) byte endian format, only
   valid after program has been loaded */
enum endian_t
endian_target_byte_order(void);

/* probe target (simulated program) double word endian format,
   only valid after program has been loaded */
enum endian_t
endian_target_word_order(void);

#endif /* HOST_ONLY */

#endif /* ENDIAN_H */
