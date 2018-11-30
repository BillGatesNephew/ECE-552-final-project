/*
 * main.c - main line routines
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
 * $Id: main.c,v 3.3 1998/08/07 17:55:43 skadron Exp $
 *
 * $Log: main.c,v $
 * Revision 3.3  1998/08/07 17:55:43  skadron
 * cosmetic change
 *
 * Revision 3.2  1998/04/24 18:18:29  skadron
 * Changed copyright notice to use my name
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
 * Revision 2.5  1997/09/17 16:48:48  skadron
 * outfile needs to be initialized
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
 * Revision 1.3  1997/04/15  19:59:27  skadron
 * signal_sim_stats() needs to reset the signal handler, which reverts to
 *    the default when called
 *
 * Revision 1.2  1997/02/17  17:27:04  skadron
 * Made exit_now() externally visible
 *
 * Revision 1.1  1997/02/16  22:23:54  skadron
 * Initial revision
 *
 * Revision 1.3  1996/12/27  15:52:20  taustin
 * updated comments
 * integrated support for options and stats packages
 *
 * Revision 1.1  1996/12/05  18:52:32  taustin
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#ifdef BFD_LOADER
#include <bfd.h>
#endif /* BFD_LOADER */

#include "misc.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "ss.h"
#include "endian.h"
#include "version.h"
#include "dlite.h"
#include "options.h"
#include "stats.h"
#include "sim.h"

/* output file -- all simulator output (but not target program output)
 * goes here */
char *outfile_name;
FILE *outfile;

/* stats signal handler */
static void
signal_sim_stats(int sigtype)
{
  sim_dump_stats = TRUE;
  signal(SIGUSR1, signal_sim_stats);
}

/* exit signal handler */
static void
signal_exit_now(int sigtype)
{
  sim_exit_now = TRUE;
}

/* execution start/end times */
time_t sim_start_time;
time_t sim_end_time;
time_t sim_elapsed_time;

/* byte/word swapping required to execute target executable on this host */
int sim_swap_bytes;
int sim_swap_words;

/* exit when this becomes non-zero */
int sim_exit_now = FALSE;

/* longjmp here when simulation is completed */
jmp_buf sim_exit_buf;

/* set to non-zero when simulator should dump statistics */
int sim_dump_stats = FALSE;

/* options database */
struct opt_odb_t *sim_odb;

/* stats database */
struct stat_sdb_t *sim_sdb;

/* track first argument orphan, this is the program to execute */
static int exec_index = -1;

/* dump help information */
static int help_me;

/* random number generator seed */
static int rand_seed;

/* initialize and quit immediately */
static int init_quit;

static int
orphan_fn(int i, int argc, char **argv)
{
  exec_index = i;
  return /* done */FALSE;
}

static void
usage(FILE *fd, int argc, char **argv)
{
  fprintf(fd, "Usage: %s {-options} executable {arguments}\n", argv[0]);
  opt_print_help(sim_odb, fd);
}

static int running = FALSE;

/* print all simulator stats */
void
sim_print_stats(FILE *fd)		/* output stream */
{

  if (!running)
    return;

  /* get stats time */
  sim_end_time = time((time_t *)NULL);
  sim_elapsed_time = MAX(sim_end_time - sim_start_time, 1);

  /* print simulation stats */
  fprintf(fd, "\nsim: ** simulation statistics **\n");
  stat_print_stats(sim_sdb, fd);
  sim_aux_stats(fd);
  fprintf(fd, "\n");
}

/* print stats, uninitialize simulator components, and exit w/ exitcode */
void 
exit_now(int exit_code)
{
  /* print simulation stats */
  sim_print_stats(outfile);

  /* un-initialize the simulator */
  sim_uninit();

  /* all done! */
  fclose(outfile);
  exit(exit_code);
}

void
main(int argc, char **argv, char **envp)
{
  int exit_code;
  char *s;
#ifndef __sparc__
#ifndef __GNUC__
  extern char *strrchr(char *, char);
#endif
#endif
  outfile = stderr;

  /* catch SIGUSR1 and dump intermediate stats */
  signal(SIGUSR1, signal_sim_stats);

  /* catch SIGUSR1 and dump intermediate stats */
  signal(SIGUSR2, signal_exit_now);

  /* register an error handler */
  fatal_hook(sim_print_stats);

  /* set up a non-local exit point */
  if ((exit_code = setjmp(sim_exit_buf)) != 0)
    {
      /* special handling as longjmp cannot pass 0 */
      exit_now(exit_code-1);
    }

  /* register global options */
  sim_odb = opt_new(orphan_fn);
  opt_reg_flag(sim_odb, "-h", "print help message",
	       &help_me, /* default */FALSE, /* !print */FALSE, NULL);
  opt_reg_flag(sim_odb, "-v", "verbose operation",
	       &verbose, /* default */FALSE, /* !print */FALSE, NULL);
#ifdef DEBUG
  opt_reg_flag(sim_odb, "-d", "enable debug message",
	       &debugging, /* default */FALSE, /* !print */FALSE, NULL);
#endif /* DEBUG */
  opt_reg_flag(sim_odb, "-i", "start in Dlite debugger",
	       &dlite_active, /* default */FALSE, /* !print */FALSE, NULL);
  opt_reg_int(sim_odb, "-seed",
	      "random number generator seed (0 for timer seed)",
	      &rand_seed, /* default */1, /* print */TRUE, NULL);
  opt_reg_flag(sim_odb, "-q", "initialize and terminate immediately",
	       &init_quit, /* default */FALSE, /* !print */FALSE, NULL);
  opt_reg_string(sim_odb, "-outfile", "file for simulator output",
		 &outfile_name, "stderr", /* print */TRUE, NULL);

  /* FIXME: add stats intervals and max insts... */

  /* register all module options */
  sim_reg_options(sim_odb);
  mem_reg_options(sim_odb);

  /* need at least two argv values to run */
  if (argc < 2)
    {
      usage(stderr, argc, argv);
      exit(1);
    }

  /* parse simulator options */
  exec_index = -1;
  opt_process_options(sim_odb, argc, argv);

  /* parse 'outfile' */
  if (!strcmp(outfile_name, "stderr"))
    outfile = stderr;
  else if (!strcmp(outfile_name, "stdout"))
    outfile = stdout;
  else if (!strlen(outfile_name))
    outfile = stderr;
  else 
    {
      /* user specified a file */
      outfile = fopen(outfile_name, "w");
      if (outfile == NULL)
	{
	  outfile = stderr;
	  panic("couldn't open %s", outfile_name);
	}
    }

  /* opening banner */
  fprintf(outfile,
	  "%s: Version %d.%d of %s.\n"
	  "Copyright (c) 1998 by Kevin Skadron.  All Rights Reserved.\n"
	  "\n",
	  ((s = strrchr(argv[0], '/')) ? s+1 : argv[0]),
	  VER_MAJOR, VER_MINOR, VER_UPDATE);

  if (help_me)
    {
      /* print help message and exit */
      usage(outfile, argc, argv);
      exit(1);
    }

  /* seed the random number generator */
  if (rand_seed == 0)
    {
      /* seed with the timer value, true random */
#if defined(__CYGWIN32__) || defined(hpux) || defined(__hpux) \
    || defined(__svr4__)
      srand(time((time_t *)NULL));
#else
      srandom(time((time_t *)NULL));
#endif
    }
  else
    {
      /* seed with default or user-specified random number generator seed */
#if defined(__CYGWIN32__) || defined(hpux) || defined(__hpux) \
    || defined(__svr4__)
      srand(rand_seed);	 
#else
      srandom(rand_seed);
#endif
    }

  /* exec_index is set in orphan_fn() */
  if (exec_index == -1)
    {
      /* executable was not found */
      fprintf(outfile, "error: no executable specified\n");
      usage(outfile, argc, argv);
      exit(1);
    }
  /* else, exec_index points to simulated program arguments */

  /* check options */
  sim_check_options(sim_odb, argc, argv);
  mem_check_options(sim_odb, argc, argv);

#ifdef BFD_LOADER
  /* initialize the bfd library */
  bfd_init();
#endif /* BFD_LOADER */

  /* initialize the instruction decoder */
  ss_init_decoder();

  /* initialize address spaces */
  mem_init();

  /* load the program text and data, set up environment, memory, and regs */
  ld_load_prog(mem_access, argc-exec_index, argv+exec_index, envp, TRUE);

  /* initialize address spaces */
  regs_init();

  /* register all simulator stats */
  sim_sdb = stat_new();
  sim_reg_stats(sim_sdb);
  ld_reg_stats(sim_sdb);
  mem_reg_stats(sim_sdb);

  /* more initialization of address spaces (using program specifics) */
  mem_init1();

  /* initialize all simulation modules */
  sim_init();

  /* record start of execution time, used in rate stats */
  sim_start_time = time((time_t *)NULL);

  /* output simulation conditions */
  s = ctime(&sim_start_time);
  if (s[strlen(s)-1] == '\n')
    s[strlen(s)-1] = '\0';
  fprintf(outfile, "\nsim: simulation started @ %s, options follow:\n", s);
  opt_print_options(sim_odb, outfile, /* short */TRUE, /* notes */TRUE);
  sim_aux_config(outfile);
  mem_aux_config(outfile);
  fprintf(outfile, "\n");

  /* omit option dump time from rate stats */
  sim_start_time = time((time_t *)NULL);

  if (init_quit)
    exit_now(0);

  running = TRUE;
  sim_main();

  /* simulation finished early */
  exit_now(0);

  panic("returned to main");
}
