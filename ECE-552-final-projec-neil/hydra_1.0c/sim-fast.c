/*
 * sim-fast.c - sample fast functional simulator implementation
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
 * $Id: sim-fast.c,v 3.1 1998/04/17 16:51:35 skadron Exp $
 *
 * $Log: sim-fast.c,v $
 * Revision 3.1  1998/04/17 16:51:35  skadron
 * Versions used thru Mar '98 (retstack sub, ruu_resub, ICS final manuscript)
 *
 * Revision 2.103  1998/01/26 15:41:54  skadron
 * Cosmetic change
 *
 * Revision 2.102  1998/01/25 21:22:39  skadron
 * Added count of FP ops
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
 * Revision 2.1  1997/07/02 19:15:41  skadron
 * Last version used for Micro-30 submission
 *
 * Revision 1.3  1997/05/07  00:16:18  skadron
 * Updated to refect new macro in ss.def: SET_TPC
 *
 * Revision 1.2  1997/03/25 16:17:53  skadron
 * Enabled inst-counting
 *
 * Revision 1.1  1997/02/16  22:23:54  skadron
 * Initial revision
 *
 * Revision 1.3  1997/01/06  16:04:21  taustin
 * comments updated
 * SPARC-specific compilation supported deleted (most opts now generalized)
 * new stats and options package support added
 * NO_INSN_COUNT added for conditional instruction count support
 * USE_JUMP_TABLE added to support simple and complex main loops
 *
 * Revision 1.1  1996/12/05  18:52:32  taustin
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/*
 * This file implements a very fast functional simulator.  This functional
 * simulator implementation is much more difficult to digest than the simpler,
 * cleaner sim-safe functional simulator.  By default, this simulator performs
 * no instruction error checking, as a result, any instruction errors will
 * manifest as simulator execution errors, possibly causing sim-fast to
 * execute incorrectly or dump core.  Such is the price we pay for speed!!!!
 *
 * The following options configure the bag of tricks used to make sim-fast
 * live up to its name.  For most machines, defining all the options results
 * in the fastest functional simulator.
 */

/* no instruction checks, for speed */
#define NO_ICHECKS

/* don't count instructions flag, enabled by default, disable for inst count */
/* #define NO_INSN_COUNT */

/* use a jump table for speed, this is a GNU GCC specific optimization,
   CAVEAT: some versions of GNU GCC core dump when optimizing the jump table
   code with optimization levels higher than -O1 */
/* #define USE_JUMP_TABLE */

#include "misc.h"
#include "ss.h"
#include "regs.h"

/* the following is some macro magic needed to register allocate the
   pointer to the level one memory page map table */

/* the level 1 page table map */
#define mem_table local_mem_table
#define HIDE_MEM_TABLE_DEF
#include "memory.h"
#undef HIDE_MEM_TABLE_DEF
#undef mem_table
extern char *mem_table[MEM_TABLE_SIZE];
#define mem_table local_mem_table

#include "loader.h"
#include "syscall.h"
#include "dlite.h"
#include "sim.h"

#ifndef NO_INSN_COUNT
/* register allocate instruction counter */
unsigned int sim_num_insn = 0;
#else /* NO_INSN_COUNT */
/* no insn or ref count in sim-fast, use sim-safe for instruction counts */
#endif /* !NO_INSN_COUNT */

#ifdef TYPE_COUNTS
unsigned int sim_num_flops = 0;
#endif

/* register simulator-specific options */
void
sim_reg_options(struct opt_odb_t *odb)
{
  opt_reg_header(odb, 
"sim-fast: This simulator implements a very fast functional simulator.  This\n"
"functional simulator implementation is much more difficult to digest than\n"
"the simpler, cleaner sim-safe functional simulator.  By default, this\n"
"simulator performs no instruction error checking, as a result, any\n"
"instruction errors will manifest as simulator execution errors, possibly\n"
"causing sim-fast to execute incorrectly or dump core.  Also note that the\n"
"event counters, if used, are only ints and thus might overflow. Such is the\n"
"price we pay for speed!!!!\n"
		 );
}

/* check simulator-specific option values */
void
sim_check_options(struct opt_odb_t *odb, int argc, char **argv)
{
  if (dlite_active)
    fatal("sim-fast does not support DLite debugging");
}

/* register simulator-specific statistics */
void
sim_reg_stats(struct stat_sdb_t *sdb)
{
#ifndef NO_INSN_COUNT
  stat_reg_uint(sdb, "sim_num_insn",
		"total number of instructions executed",
		&sim_num_insn, 0, NULL);
#endif /* !NO_INSN_COUNT */
  stat_reg_int(sdb, "sim_elapsed_time",
	       "total simulation time in seconds",
	       (int *)&sim_elapsed_time, 0, NULL);
#ifndef NO_INSN_COUNT
  stat_reg_formula(sdb, "sim_inst_rate",
		   "simulation speed (in insts/sec)",
		   "sim_num_insn / sim_elapsed_time", NULL);
#endif /* !NO_INSN_COUNT */
#ifdef TYPE_COUNTS
  stat_reg_uint(sdb, "sim_num_flops",
		"total number of FP instructions executed",
		&sim_num_flops, 0, NULL);
#endif
}

/* these are global equivalent defs to the register defs in sim_main(), and
   they are used by the helper functions defined in the `.def' file and
   sim_init() memory accessors because they cannot access the registerized
   version of these vars in sim_main() */
SS_WORD_TYPE *local_regs_R = regs_R;
#undef mem_table
char **local_mem_table = mem_table;
#define mem_table local_mem_table

/* initialize the simulator */
void
sim_init(void)
{
  SS_INST_TYPE inst;

  /* decode all instructions */
  {
    SS_ADDR_TYPE addr;

    if (OP_MAX > 255)
      fatal("cannot do fast decoding, too many opcodes");

    debug("sim: decoding text segment...");
    for (addr=ld_text_base;
	 addr < (ld_text_base+ld_text_size);
	 addr += SS_INST_SIZE)
      {
	inst = __UNCHK_MEM_ACCESS(SS_INST_TYPE, addr);
	inst.a = (inst.a & ~0xff) | (unsigned int)SS_OP_ENUM(SS_OPCODE(inst));
	__UNCHK_MEM_ACCESS(SS_INST_TYPE, addr) = inst;
      }
  }
}

/* print simulator-specific configuration information */
void
sim_aux_config(FILE *stream)
{
  /* nothing currently */
}

/* dump simulator-specific auxiliary simulator statistics */
void
sim_aux_stats(FILE *stream)
{
  /* nada */
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

/* next program counter */
#define SET_NPC(EXPR)		(local_next_PC = (EXPR))

/* current program counter */
#define CPC			(local_regs_PC)

/* general purpose registers */
#define GPR(N)			(local_regs_R[N])
#define SET_GPR(N,EXPR)		(local_regs_R[N] = (EXPR))

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

/* precise architected memory state accessor macros, all unsafe for speed  */
#define READ_WORD(SRC)							\
  (__MEM_READ_WORD(SRC))
#define READ_UNSIGNED_HALF(SRC)						\
  ((unsigned int)((unsigned short)__MEM_READ_HALF(SRC)))
#define READ_SIGNED_HALF(SRC)						\
  ((signed int)((signed short)__MEM_READ_HALF(SRC)))
#define READ_UNSIGNED_BYTE(SRC)						\
  ((unsigned int)((unsigned char)__MEM_READ_BYTE(SRC)))
#define READ_SIGNED_BYTE(SRC)						\
  ((unsigned int)((signed int)((signed char)__MEM_READ_BYTE(SRC))))

#define WRITE_WORD(SRC, DST)                                            \
  (__MEM_WRITE_WORD((DST), (unsigned int)(SRC)))
#define WRITE_HALF(SRC, DST)                                            \
  (__MEM_WRITE_HALF((DST), (unsigned short)((unsigned int)(SRC))))
#define WRITE_BYTE(SRC, DST)                                            \
  (__MEM_WRITE_BYTE((DST), (unsigned char)((unsigned int)(SRC))))

/* system call handler macro */
#define SYSCALL(INST)		(ss_syscall(mem_access, INST))

/* instantiate the helper functions in the '.def' file */
/* access global register */
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
#ifdef USE_JUMP_TABLE
  /* the jump table employs GNU GCC label extensions to construct an array
     of pointers to instruction implementation code, the simulator then uses
     the table to lookup the location of instruction's implementing code, a
     GNU GCC `goto' extension is then used to jump to the inst's implementing
     code through the op_jump table; as a result, there is no need for
     a main simulator loop, which eliminates one branch from the simulator
     interpreter - crazy, no!?!? */

  /* instruction jump table, this code is GNU GCC specific */
  static void *op_jump[/* max opcodes */256] = {
    &&opcode_NA, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,EXPR)	\
    &&opcode_##OP,
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT)					\
    &&opcode_##OP,
#define CONNECT(OP)
#include "ss.def"
#undef DEFINST
#undef DEFLINK
#undef CONNECT
  };
#endif /* USE_JUMP_TABLE */

  /* register allocate PC and next PC */
  register SS_ADDR_TYPE local_regs_PC = regs_PC, local_next_PC;
  /* register allocate integer register file base pointer address */
  register SS_WORD_TYPE *local_regs_R = regs_R;
  /* register allocate level one memory page map */
#undef mem_table
  register char **local_mem_table = mem_table;
#define mem_table local_mem_table
#ifdef USE_JUMP_TABLE
  /* register allocate local jump table base pointer address */
  register void **local_op_jump = op_jump;
#endif /* USE_JUMP_TABLE */
  /* register allocate instruction buffer */
  register SS_INST_TYPE inst;

  fprintf(outfile, "sim: ** starting *fast* functional simulation **\n");

  /* must have natural byte/word ordering */
  if (sim_swap_bytes || sim_swap_words)
    fatal("sim: *fast* functional simulation cannot swap bytes or words");

#ifdef USE_JUMP_TABLE

  /* set up initial default next PC */
  local_next_PC = regs_PC;

  /* load instruction */
  inst = __UNCHK_MEM_ACCESS(SS_INST_TYPE, local_next_PC);

  /* jump to instruction implementation */
  goto *local_op_jump[SS_OPCODE(inst)];

#ifndef NO_INSN_COUNT
#define INC_INSN_CTR()	sim_num_insn++
#else /* !NO_INSN_COUNT */
#define INC_INSN_CTR()	/* nada */
#endif /* NO_INSN_COUNT */

#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,EXPR)	\
  opcode_##OP:								\
    /* maintain $r0 semantics */					\
    local_regs_R[0] = 0;						\
    /* keep an instruction count */					\
    INC_INSN_CTR();							\
    /* execute next instruction */					\
    local_regs_PC = local_next_PC;					\
    /* set up default next PC */					\
    local_next_PC += 8;							\
    /* execute the instruction */					\
    EXPR;								\
    /* get the next instruction */					\
    inst = __UNCHK_MEM_ACCESS(SS_INST_TYPE, local_next_PC);		\
    /* jump to instruction implementation */				\
    goto *local_op_jump[SS_OPCODE(inst)];
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT)					\
  opcode_##OP:								\
    panic("attempted to execute a linking opcode");
#define CONNECT(OP)
#include "ss.def"
#undef DEFINST
#undef DEFLINK
#undef CONNECT
 opcode_NA:
    panic("attempted to execute a bogus opcode");

  /* should not get here... */
  panic("exited sim-fast main loop");

#else /* !USE_JUMP_TABLE */

  /* set up initial default next PC */
  local_next_PC = regs_PC;

  while (TRUE)
    {
      /* maintain $r0 semantics */
      local_regs_R[0] = 0;

      /* keep an instruction count */
#ifndef NO_INSN_COUNT
      sim_num_insn++;
#endif /* !NO_INSN_COUNT */

      inst = __UNCHK_MEM_ACCESS(SS_INST_TYPE, local_regs_PC);

      switch (SS_OPCODE(inst)) {
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,EXPR)	\
      case OP:								\
        EXPR;								\
        break;
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT)					\
      case OP:								\
        panic("attempted to execute a linking opcode");
#define CONNECT(OP)
#include "ss.def"
#undef DEFINST
#undef DEFLINK
#undef CONNECT
      }

#ifdef TYPE_COUNTS
      if (SS_OP_FLAGS(SS_OPCODE(inst)) & F_FCOMP)
	sim_num_flops++;
#endif

      /* execute next instruction */
      local_regs_PC = local_next_PC;
      local_next_PC += SS_INST_SIZE;
    }

#endif /* USE_JUMP_TABLE */

}
