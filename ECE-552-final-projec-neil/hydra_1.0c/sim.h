/*
 * sim.h - simulator main line interfaces
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
 * $Id: sim.h,v 3.2 1998/05/26 19:02:34 skadron Exp $
 *
 * $Log: sim.h,v $
 * Revision 3.2  1998/05/26 19:02:34  skadron
 * Added "enum pipestage"
 *
 * Revision 3.1  1998/04/17 16:51:35  skadron
 * Versions used thru Mar '98 (retstack sub, ruu_resub, ICS final manuscript)
 *
 * Revision 2.101  1997/12/09 22:20:31  skadron
 * Version Pritpal used for ISCA-25 submission
 *
 * Revision 2.100  1997/12/03 20:05:46  skadron
 * Version Kevin used for ISCA-25 submission
 *
 * Revision 2.4  1997/09/15 16:37:30  skadron
 * Added -outfile option
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
 * Revision 1.3  1997/04/15  19:03:20  skadron
 * Declared 'done_priming' and 'sim_warmup'
 *
 * Revision 1.2  1997/02/17  17:27:22  skadron
 * Declared exit_now()
 *
 * Revision 1.1  1997/02/16  22:23:54  skadron
 * Initial revision
 *
 * Revision 1.3  1996/12/27  15:54:47  taustin
 * updated comments
 * integrated support for options and stats packages
 *
 * Revision 1.1  1996/12/05  18:50:23  taustin
 * Initial revision
 *
 *
 */

#ifndef SIM_H
#define SIM_H

#include <stdio.h>
#include <setjmp.h>
#include <time.h>
#include "options.h"
#include "stats.h"

enum pipestage {Fetch, Decode, Issue, Writeback, Commit};

/* file for simulator output */
FILE *outfile;

/* are we pre- or post-priming?  are we in cache-warmup-mode? */
extern int done_priming;
extern int sim_warmup;

/* set to non-zero when simulator should dump statistics */
extern int sim_dump_stats;

/* exit when this becomes non-zero */
extern int sim_exit_now;

/* longjmp here when simulation is completed */
extern jmp_buf sim_exit_buf;

/* Function that gracefully exits simulation (prints stats, etc) when called */
void exit_now(int exit_code);

/* byte/word swapping required to execute target executable on this host */
extern int sim_swap_bytes;
extern int sim_swap_words;

/* execution start/end times */
extern time_t sim_start_time;
extern time_t sim_end_time;
extern time_t sim_elapsed_time;

/* options database */
struct opt_odb_t *sim_odb;

/* stats database */
struct stat_sdb_t *sim_sdb;

/*
 * main simulator interfaces, called in the following order
 */

/* register simulator-specific options */
void sim_reg_options(struct opt_odb_t *odb);

/* main() parses options next... */

/* check simulator-specific option values */
void sim_check_options(struct opt_odb_t *odb, int argc, char **argv);

/* register simulator-specific statistics */
void sim_reg_stats(struct stat_sdb_t *sdb);

/* initialize the simulator */
void sim_init(void);

/* main() initialized architected state next... */

/* main() prints the option database values next... */

/* print simulator-specific configuration information */
void sim_aux_config(FILE *stream);

/* start simulation, program loaded, processor precise state initialized */
void sim_main(void);

/* main() prints the stats database values next... */

/* dump simulator-specific auxiliary simulator statistics */
void sim_aux_stats(FILE *stream);

/* un-initialize simulator-specific state */
void sim_uninit(void);

/* print all simulator stats */
void
sim_print_stats(FILE *fd);		/* output stream */

#endif /* SIM_H */
