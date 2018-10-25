/*
 * sim-cheetah.c - single-pass multiple-configuration cache simulator
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
 * $Id: sim-cheetah.c,v 3.1 1998/04/17 16:51:35 skadron Exp $
 *
 * $Log: sim-cheetah.c,v $
 * Revision 3.1  1998/04/17 16:51:35  skadron
 * Versions used thru Mar '98 (retstack sub, ruu_resub, ICS final manuscript)
 *
 * Revision 2.101  1997/12/09 22:20:31  skadron
 * Version Pritpal used for ISCA-25 submission
 *
 * Revision 2.100  1997/12/03 20:05:46  skadron
 * Version Kevin used for ISCA-25 submission
 *
 * Revision 2.4  1997/09/16 01:56:51  skadron
 * Added outfile
 *
 * Revision 2.3  1997/07/11 21:44:19  skadron
 * Updated to incorporate final changes for public 2.0 release
 *
 * Revision 2.2  1997/07/08 03:45:58  skadron
 * Updated to reflect Todd/Milo's version 2.0e; includes memory-leak fix
 *
 * Revision 1.1  1997/03/11  01:33:09  taustin
 * Initial revision
 *
 * Revision 1.3  1996/12/27  15:54:04  taustin
 * updated comments
 * integrated support for options and stats packages
 * added sim_init() code
 *
 * Revision 1.1  1996/12/05  18:52:32  taustin
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "misc.h"
#include "ss.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "syscall.h"
#include "dlite.h"
#include "options.h"
#include "stats.h"
#include "libcheetah/libcheetah.h"
#include "sim.h"

/*
 * This file implements a functional simulator driver for Cheetah.  Cheetah
 * is a cache simulation package written by Rabin Sugumar and Santosh Abraham
 * which can efficiently simulate multiple cache configurations in a single
 * run of a program.  Specifically, Cheetah can simulate ranges of single
 * level set-associative and fully-associative caches.  See the directory
 * libcheetah/ for more details on Cheetah.
 */

/* track number of insn and refs */
static SS_COUNTER_TYPE sim_num_insn = 0;
static SS_COUNTER_TYPE sim_num_refs = 0;

/* replacement policy, i.e., lru or opt */
static char *repl_str;

/* cache configuration, i.e., fa, sa, or dm */
static char *conf_str;

/* minimum number of sets to analyze (log base 2) */
static int min_sets;

/* minimum number of sets to analyze (log base 2) */
static int max_sets;

/* line size of the caches (log base 2) */
static int line_size;

/* max degree of associativity to analyze (log base 2) */
static int max_assoc;

/* cache size intervals at which miss ratio is shown */
static int cache_interval;

/* maximum cache size of interest */
static int max_cache;

/* size of cache (log base 2) for DM analysis */
static int cache_size;

/* reference stream to analyze, i.e., {inst|data|unified} */
static char *ref_stream;

/* reference stream to analyze */
#define REFS_INST		0x01
#define REFS_DATA		0x02
static int refs;

/* register simulator-specific options */
void
sim_reg_options(struct opt_odb_t *odb)
{
  fprintf(outfile,
	  "Portions Copyright (C) 1989-1993 by "
	  "Rabin A. Sugumar and Santosh G. Abraham.\n");

  opt_reg_header(odb, 
"sim-cheetah: This program implements a functional simulator driver for\n"
"Cheetah.  Cheetah is a cache simulation package written by Rabin Sugumar\n"
"and Santosh Abraham which can efficiently simulate multiple cache\n"
"configurations in a single run of a program.  Specifically, Cheetah can\n"
"simulate ranges of single level set-associative and fully-associative\n"
"caches.  See the directory libcheetah/ for more details on Cheetah.\n"
		 );

  opt_reg_string(odb, "-refs",
		 "reference stream to analyze, i.e., {inst|data|unified}",
		 &ref_stream, "data", /* print */TRUE, /* format */NULL);

  opt_reg_string(odb, "-R", "replacement policy, i.e., lru or opt",
		 &repl_str, "lru", /* print */TRUE, /* format */NULL);

  opt_reg_string(odb, "-C", "cache configuration, i.e., fa, sa, or dm",
		 &conf_str, "sa", /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-a", "min number of sets (log base 2, line size for DM)",
	      &min_sets, 7, /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-b", "max number of sets (log base 2, line size for DM)",
	      &max_sets, 14, /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-l", "line size of the caches (log base 2)",
	      &line_size, 4, /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-n", "max degree of associativity to analyze (log base 2)",
	      &max_assoc, 1, /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-in", "cache size intervals at which miss ratio is shown",
	      &cache_interval, 512, /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-M", "maximum cache size of interest",
	      &max_cache, 524288, /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-c", "size of cache (log base 2) for DM analysis",
	      &cache_size, 16, /* print */TRUE, /* format */NULL);
}

/* libcheetah argument count and string vector */
static int lib_argc = 0;
static char *lib_argv[16];

/* check simulator-specific option values */
void
sim_check_options(struct opt_odb_t *odb, int argc, char **argv)
{
  char buf[512];

  if (!strcmp(ref_stream, "inst"))
    refs = REFS_INST;
  else if (!strcmp(ref_stream, "data"))
    refs = REFS_DATA;
  else if (!strcmp(ref_stream, "unified"))
    refs = (REFS_INST|REFS_DATA);
  else
    fatal("bad reference stream specifier, use {inst|data|unified}");

  /* marshall up the libcheetah arguments */
  lib_argc = 0;

  sprintf(buf, "-R%s", repl_str);
  lib_argv[lib_argc++] = mystrdup(buf);

  sprintf(buf, "-C%s", conf_str);
  lib_argv[lib_argc++] = mystrdup(buf);

  sprintf(buf, "-a%d", min_sets);
  lib_argv[lib_argc++] = mystrdup(buf);

  sprintf(buf, "-b%d", max_sets);
  lib_argv[lib_argc++] = mystrdup(buf);

  sprintf(buf, "-l%d", line_size);
  lib_argv[lib_argc++] = mystrdup(buf);

  sprintf(buf, "-n%d", max_assoc);
  lib_argv[lib_argc++] = mystrdup(buf);

  sprintf(buf, "-i%d", cache_interval);
  lib_argv[lib_argc++] = mystrdup(buf);

  sprintf(buf, "-M%d", max_cache);
  lib_argv[lib_argc++] = mystrdup(buf);

  sprintf(buf, "-c%d", cache_size);
  lib_argv[lib_argc++] = mystrdup(buf);
}

/* register simulator-specific statistics */
void
sim_reg_stats(struct stat_sdb_t *sdb)
{
  stat_reg_counter(sdb, "sim_num_insn",
		   "total number of instructions executed",
		   &sim_num_insn, 0, NULL);
  stat_reg_counter(sdb, "sim_num_refs",
		   "total number of loads and stores executed",
		   &sim_num_refs, 0, NULL);
  stat_reg_int(sdb, "sim_elapsed_time",
	       "total simulation time in seconds",
	       (int *)&sim_elapsed_time, 0, NULL);
  stat_reg_formula(sdb, "sim_inst_rate",
		   "simulation speed (in insts/sec)",
		   "sim_num_insn / sim_elapsed_time", NULL);
}

/* local machine state accessor */
static char *					/* err str, NULL for no err */
cheetah_mstate_obj(FILE *stream,		/* output stream */
		   char *cmd)			/* optional command string */
{
  sim_end_time = time((time_t *)NULL);
  sim_elapsed_time = MAX(sim_end_time - sim_start_time, 1);

  /* print simulation stats */
  fprintf(stream, "\nsim: ** simulation statistics **\n");
  stat_print_stats(sim_sdb, stream);

  /* print libcheetah stats */
  cheetah_stats(stream, /* mid */TRUE);

  /* no error */
  return NULL;
}

/* initialize the simulator */
void
sim_init(void)
{
  SS_INST_TYPE inst;

  sim_num_insn = 0;
  sim_num_refs = 0;

  regs_PC = ld_prog_entry;

  /* decode all instructions */
  {
    SS_ADDR_TYPE addr;

    if (OP_MAX > 255)
      fatal("cannot perform fast decoding, too many opcodes");

    debug("sim: decoding text segment...");
    for (addr=ld_text_base;
	 addr < (ld_text_base+ld_text_size);
	 addr += SS_INST_SIZE)
      {
	inst = __UNCHK_MEM_ACCESS(SS_INST_TYPE, addr);
	inst.a = SWAP_WORD(inst.a);
	inst.b = SWAP_WORD(inst.b);
	inst.a = (inst.a & ~0xff) | (unsigned int)SS_OP_ENUM(SS_OPCODE(inst));
	__UNCHK_MEM_ACCESS(SS_INST_TYPE, addr) = inst;
      }
  }

  /* initialize the DLite debugger (NOTE: mem is always precise) */
  dlite_init(dlite_reg_obj, dlite_mem_obj, cheetah_mstate_obj);

  /* initialize libcheetah */
  cheetah_init(lib_argc, lib_argv);
}


/* print simulator-specific configuration information */
void
sim_aux_config(FILE *stream)		/* output stream */
{
  /* print libcheetah configuration */
  cheetah_config(stream);
}

/* dump simulator-specific auxiliary simulator statistics */
void
sim_aux_stats(FILE *stream)		/* output stream */
{
  /* print libcheetah stats */
  cheetah_stats(stream, /* final */FALSE);
}

/* un-initialize simulator-specific state */
void
sim_uninit(void)
{
  /* nada */
}


/*
 * configure the execution engine
 */

/*
 * precise architected register accessors
 */

/* next program counter */
#define SET_NPC(EXPR)		(next_PC = (EXPR))

/* current program counter */
#define CPC			(regs_PC)

/* general purpose registers */
#define GPR(N)			(regs_R[N])
#define SET_GPR(N,EXPR)		(regs_R[N] = (EXPR))

/* floating point registers, L->word, F->single-prec, D->double-prec */
#define FPR_L(N)		(regs_F.l[(N)])
#define SET_FPR_L(N,EXPR)	(regs_F.l[(N)] = (EXPR))
#define FPR_F(N)		(regs_F.f[(N)])
#define SET_FPR_F(N,EXPR)	(regs_F.f[(N)] = (EXPR))
#define FPR_D(N)		(regs_F.d[(N) >> 1])
#define SET_FPR_D(N,EXPR)	(regs_F.d[(N) >> 1] = (EXPR))

/* miscellaneous register accessors */
#define SET_HI(EXPR)		(regs_HI = (EXPR))
#define HI			(regs_HI)
#define SET_LO(EXPR)		(regs_LO = (EXPR))
#define LO			(regs_LO)
#define FCC			(regs_FCC)
#define SET_FCC(EXPR)		(regs_FCC = (EXPR))

/* precise architected memory state help functions */
#define __READ_WORD(DST_T, SRC_T, SRC)					\
  (addr = (SRC),							\
   ((refs & REFS_DATA) ? cheetah_access(addr) : (void)0),		\
   ((unsigned int)((DST_T)(SRC_T)MEM_READ_WORD(addr))))

#define __READ_HALF(DST_T, SRC_T, SRC)					\
  (addr = (SRC),							\
   ((refs & REFS_DATA) ? cheetah_access(addr) : (void)0),		\
   (unsigned int)((DST_T)(SRC_T)MEM_READ_HALF(addr)))

#define __READ_BYTE(DST_T, SRC_T, SRC)					\
  (addr = (SRC),							\
   ((refs & REFS_DATA) ? cheetah_access(addr) : (void)0),		\
   (unsigned int)((DST_T)(SRC_T)MEM_READ_BYTE(addr)))

/* precise architected memory state accessor macros */
#define READ_WORD(SRC)							\
  __READ_WORD(unsigned int, unsigned int, (SRC))

#define READ_UNSIGNED_HALF(SRC)						\
  __READ_HALF(unsigned int, unsigned short, (SRC))

#define READ_SIGNED_HALF(SRC)						\
  __READ_HALF(signed int, signed short, (SRC))

#define READ_UNSIGNED_BYTE(SRC)						\
  __READ_BYTE(unsigned int, unsigned char, (SRC))

#define READ_SIGNED_BYTE(SRC)						\
  __READ_BYTE(signed int, signed char, (SRC))

#define WRITE_WORD(SRC, DST)						\
  (addr = (DST),							\
   ((refs & REFS_DATA) ? cheetah_access(addr) : (void)0),		\
   MEM_WRITE_WORD(addr, (unsigned int)(SRC)))

#define WRITE_HALF(SRC, DST)						\
  (addr = (DST),							\
   ((refs & REFS_DATA) ? cheetah_access(addr) : (void)0),		\
   MEM_WRITE_HALF(addr, (unsigned short)(unsigned int)(SRC)))

#define WRITE_BYTE(SRC, DST)						\
  (addr = (DST),							\
   ((refs & REFS_DATA) ? cheetah_access(addr) : (void)0),		\
   MEM_WRITE_BYTE(addr, (unsigned char)(unsigned int)(SRC)))

/* system call memory access function */
void
cheetah_access_fn(enum mem_cmd cmd,	/* memory access cmd, Read or Write */
		  SS_ADDR_TYPE addr,	/* data address to access */
		  void *p,		/* data input/output buffer */
		  int nbytes)		/* number of bytes to access */
{
  if (refs & REFS_DATA) cheetah_access(addr);
  mem_access(cmd, addr, p, nbytes);
}

/* system call handler macro */
#define SYSCALL(INST)		(ss_syscall(cheetah_access_fn, INST))

/* instantiate the helper functions in the '.def' file */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,CLASS,O1,O2,I1,I2,I3,EXPR)
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT)
#define CONNECT(OP)
#define IMPL
#include "ss.def"
#undef DEFINST
#undef DEFLINK
#undef CONNECT
#undef IMPL

/* start simulation, program loaded, processor precise state initialized */
void
sim_main(void)
{
  SS_INST_TYPE inst;
  enum ss_opcode op;
  register SS_ADDR_TYPE next_PC;
  register SS_ADDR_TYPE addr;
  register int is_write;

  fprintf(outfile, "sim: ** starting functional simulation **\n");

  /* set up initial default next PC */
  next_PC = regs_PC + SS_INST_SIZE;

  /* check for DLite debugger entry condition */
  if (dlite_check_break(regs_PC, /* no access */0, /* addr */0, 0, 0))
    dlite_main(regs_PC - SS_INST_SIZE, regs_PC, sim_num_insn);

  while (TRUE)
    {
      /* maintain $r0 semantics */
      regs_R[0] = 0;

      /* keep an instruction count */
      sim_num_insn++;

      /* get the next instruction to execute */
      if (refs & REFS_INST)
	cheetah_access(regs_PC);
      mem_access(Read, regs_PC, &inst, SS_INST_SIZE);

      /* set default reference address and access mode */
      addr = 0; is_write = FALSE;

      /* decode the instruction */
      op = SS_OPCODE(inst);
      switch (op)
	{
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,EXPR)	\
	case OP:                                                        \
          EXPR;                                                         \
          break;
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT)                                 \
        case OP:                                                        \
          panic("attempted to execute a linking opcode");
#define CONNECT(OP)
#include "ss.def"
#undef DEFINST
#undef DEFLINK
#undef CONNECT
	default:
	  panic("bogus opcode");
      }

      if (SS_OP_FLAGS(op) & F_MEM)
	{
	  sim_num_refs++;
	  if (SS_OP_FLAGS(op) & F_STORE)
	    is_write = TRUE;
	}

      /* check for DLite debugger entry condition */
      if (dlite_check_break(next_PC,
			    is_write ? ACCESS_WRITE : ACCESS_READ,
			    addr, sim_num_insn, sim_num_insn))
	dlite_main(regs_PC, next_PC, sim_num_insn);

      /* go to the next instruction */
      regs_PC = next_PC;
      next_PC += SS_INST_SIZE;
    }
}
