/*
 * warmup-cache.c - cache and branch-predictor instruction-level simulator
 * implementation; for warming up these structures for later simulation 
 * by a more detailed simulator.
 *	-- created by Kevin Skadron, based on sim-cache.c
 *         Copyright (C) 1998, 1999.
 *         skadron@cs.princeton.edu
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "misc.h"
#include "ss.h"
#include "regs.h"
#include "memory.h"
#include "cache.h"
#include "bpred.h"
#include "bconf.h"
#include "loader.h"
#include "syscall.h"
#if 0
#include "dlite.h"
#endif
#include "sim.h"

/*
 * This file implements a functional cache simulator.  Cache statistics are
 * generated for a user-selected cache and TLB configuration, which may include
 * up to two levels of instruction and data cache (with any levels unified),
 * and one level of instruction and data TLBs.  No timing information is
 * generated (hence the distinction, "functional" simulator).
 */

/* track number of insn and refs */
extern SS_COUNTER_TYPE sim_num_insn;
extern SS_COUNTER_TYPE sim_num_refs;
extern SS_COUNTER_TYPE sim_num_loads;

/* level 1 instruction cache, entry level instruction cache */
extern struct cache *cache_il1;

/* level 1 instruction cache */
extern struct cache *cache_il2;

/* level 1 data cache, entry level data cache */
extern struct cache *cache_dl1;

/* level 2 data cache */
extern struct cache *cache_dl2;

/* instruction TLB */
extern struct cache *itlb;

/* data TLB */
extern struct cache *dtlb;

/* branch predictor */
extern struct bpred *pred;
extern struct bconf *bconf;

extern int flush_on_syscalls;
extern int compress_icache_addrs;

/* convert 64-bit inst text addresses to 32-bit inst equivalents */
#define IACOMPRESS(A)							\
  (compress_icache_addrs ? ((((A) - SS_TEXT_BASE) >> 1) + SS_TEXT_BASE) : (A))
#define ISCOMPRESS(SZ)							\
  (compress_icache_addrs ? ((SZ) >> 1) : (SZ))

/* local machine state accessor */
static char *					/* err str, NULL for no err */
cache_mstate_obj(FILE *stream,			/* output stream */
		 char *cmd)			/* optional command string */
{
  /* just dump intermediate stats */
  sim_print_stats(stream);

  /* no error */
  return NULL;
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
#define __READ_CACHE(addr, SRC_T)					\
  ((dtlb								\
    ? cache_access(dtlb, Read, (addr), NULL, sizeof(SRC_T), 0, NULL, NULL)\
    : 0),								\
   (cache_dl1								\
    ? cache_access(cache_dl1, Read, (addr), NULL, sizeof(SRC_T), 0, NULL, NULL)\
    : 0))

#define __READ_WORD(DST_T, SRC_T, SRC)					\
  (addr = (SRC),							\
   __READ_CACHE(addr, SRC_T),						\
   ((unsigned int)((DST_T)(SRC_T)MEM_READ_WORD(addr))))

#define __READ_HALF(DST_T, SRC_T, SRC)					\
  (addr = (SRC),							\
   __READ_CACHE(addr, SRC_T),						\
   (unsigned int)((DST_T)(SRC_T)MEM_READ_HALF(addr)))

#define __READ_BYTE(DST_T, SRC_T, SRC)					\
  (addr = (SRC),							\
   __READ_CACHE(addr, SRC_T),						\
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

/* precise architected memory state help functions */

#define __WRITE_CACHE(addr, DST_T)					\
  ((dtlb								\
    ? cache_access(dtlb, Write, (addr), NULL, sizeof(DST_T), 0, NULL, NULL)\
    : 0),								\
   (cache_dl1								\
    ? cache_access(cache_dl1, Write, (addr), NULL, sizeof(DST_T),	\
		   0, NULL, NULL)					\
    : 0))

#define WRITE_WORD(SRC, DST)						\
  (addr = (DST),							\
   __WRITE_CACHE(addr, unsigned int),					\
   MEM_WRITE_WORD(addr, (unsigned int)(SRC)))

#define WRITE_HALF(SRC, DST)						\
  (addr = (DST),							\
   __WRITE_CACHE(addr, unsigned short),					\
   MEM_WRITE_HALF(addr, (unsigned short)(unsigned int)(SRC)))

#define WRITE_BYTE(SRC, DST)						\
  (addr = (DST),							\
   __WRITE_CACHE(addr, unsigned char),					\
   MEM_WRITE_BYTE(addr, (unsigned char)(unsigned int)(SRC)))

/* system call memory access function */
void
dcache_access_fn(enum mem_cmd cmd,	/* memory access cmd, Read or Write */
		 SS_ADDR_TYPE addr,	/* data address to access */
		 void *p,		/* data input/output buffer */
		 int nbytes)		/* number of bytes to access */
{
  if (dtlb)
    cache_access(dtlb, cmd, addr, NULL, nbytes, 0, NULL, NULL);
  if (cache_dl1)
    cache_access(cache_dl1, cmd, addr, NULL, nbytes, 0, NULL, NULL);
  mem_access(cmd, addr, p, nbytes);
}

/* system call handler macro */
#define SYSCALL(INST)							\
  (flush_on_syscalls							\
   ? ((dtlb ? cache_flush(dtlb, 0) : 0),				\
      (cache_dl1 ? cache_flush(cache_dl1, 0) : 0),			\
      (cache_dl2 ? cache_flush(cache_dl2, 0) : 0),			\
      ss_syscall(mem_access, INST))					\
   : (ss_syscall(dcache_access_fn, INST)))

/* instantiate the helper functions in the '.def' file */
#define DEFICLASS(ICLASS,DESC)
#define DEFINST(OP,MSK,NAME,OPFORM,RES,CLASS,O1,O2,I1,I2,I3,EXPR)
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT)
#define CONNECT(OP)
#define IMPL
#include "ss.def"
#undef DEFICLASS
#undef DEFINST
#undef DEFLINK
#undef CONNECT
#undef IMPL

/* Stuff to do after warmup, but before returning */
static void 
warmup_done(void)
{
  cache_mstate_obj(outfile, NULL);
}

/* start simulation, program loaded, processor precise state initialized */
void
warmup_main(SS_COUNTER_TYPE num_warmup_insn)
{
  SS_INST_TYPE inst;
  register SS_ADDR_TYPE next_PC, pred_PC;
  register SS_ADDR_TYPE addr;
  enum ss_opcode op;
  register int is_write;
  struct bpred_update_info b_update_rec;
  struct bpred_recover_info bpred_recover_rec;
  int junk = -1;
  SS_COUNTER_TYPE start_insn = sim_num_insn;
  int base_caller_supplies_tos = pred->retstack.caller_supplies_tos;

  pred->retstack.caller_supplies_tos = FALSE;
  fprintf(outfile, "sim: ** starting functional simulation w/ caches **\n");

  /* set up initial PC, default next PC */
  next_PC = regs_PC + SS_INST_SIZE;

#if 0
  /* check for DLite debugger entry condition */
  if (dlite_check_break(regs_PC, /* no access */0, /* addr */0, 0, 0))
    dlite_main(regs_PC - SS_INST_SIZE, regs_PC, sim_num_insn);
#endif

  while (TRUE)
    {
      if (sim_num_insn - start_insn > num_warmup_insn)
	{
	  pred->retstack.caller_supplies_tos = base_caller_supplies_tos;
	  warmup_done();
	  return;
	}

      /* Check for exit condition */
      if (sim_exit_now)
	exit_now(0);

      /* Decide whether to dump intermediate stats */
      if (sim_dump_stats)
	{
	  cache_mstate_obj(outfile, NULL);
	  sim_dump_stats = FALSE;
	}

      /* maintain $r0 semantics */
      regs_R[0] = 0;

      /* keep an instruction count */
      sim_num_insn++;

      /* get the next instruction to execute */
      if (itlb)
	cache_access(itlb, Read, IACOMPRESS(regs_PC),
		     NULL, ISCOMPRESS(SS_INST_SIZE), 0, NULL, NULL);
      if (cache_il1)
	cache_access(cache_il1, Read, IACOMPRESS(regs_PC),
		     NULL, ISCOMPRESS(SS_INST_SIZE), 0, NULL, NULL);
      mem_access(Read, regs_PC, &inst, SS_INST_SIZE);

      op = SS_OPCODE(inst);
 
      /* prime the branch predictor: lookup */
      if (pred && (SS_OP_FLAGS(op) & F_CTRL))
	{
	  pred_PC = bpred_lookup(pred, regs_PC, 0, 0, 0, 
				 op, (RS) == 31, (RD) == 31, &junk, TRUE,
				 &b_update_rec, &bpred_recover_rec);
	  if (pred_PC == 0)
	    /* predicted not taken */
	    pred_PC = regs_PC + sizeof(SS_INST_TYPE);

	  bpred_history_update(pred, regs_PC, 
			       pred_PC != (regs_PC + SS_INST_SIZE),
			       op, TRUE, &b_update_rec, &bpred_recover_rec);
	}

      /* set default reference address and access mode */
      addr = 0; is_write = FALSE;

      /* decode the instruction */
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
          panic("attempted to execute a bogus opcode");
	}

      if (SS_OP_FLAGS(op) & F_MEM)
	{
	  sim_num_refs++;
	  if (SS_OP_FLAGS(op) & F_STORE)
	    is_write = TRUE;
	  else
	    sim_num_loads++;
	}

      /* prime the branch predictor: update on mis-predict */
      if (pred && (SS_OP_FLAGS(op) & F_CTRL))
	{
	  if (pred_PC != next_PC)
	    bpred_history_recover(pred, regs_PC,
			       /* taken? */next_PC != (regs_PC + SS_INST_SIZE),
			       /* opcode */op,
			       /* stage */Writeback,
		               /* bpred fixup rec */&bpred_recover_rec);

	  bpred_update(pred, regs_PC, next_PC, OFS,
		       /* taken? */next_PC != (regs_PC + SS_INST_SIZE),
		       /* pred taken? */pred_PC != (regs_PC + SS_INST_SIZE),
		       /* correct pred? */pred_PC == next_PC,
		       /* opcode */op, (RS) == 31,
		       /* retstack gate */TRUE,
		       /* hybrid component */FALSE,
		       /* dir predictor update pointer */b_update_rec,
		       /* bpred fixup rec */&bpred_recover_rec);

	  /* don't need stack copy or local-history copy */
	  if (pred->retstack.patch_level == RETSTACK_PATCH_WHOLE
	      && bpred_recover_rec.contents.stack_copy != NULL)
	    {
	      free(bpred_recover_rec.contents.stack_copy);
	      bpred_recover_rec.contents.stack_copy = NULL;
	    }
	}
      if (bconf && (SS_OP_FLAGS(op) & F_COND))
	{
	  int br_taken = (next_PC != (regs_PC + SS_INST_SIZE));
	  int br_pred_taken = (pred_PC != (regs_PC + SS_INST_SIZE));
	  
	  bconf_update(bconf, regs_PC, 
		       /* branch taken? */br_taken,
		       /* correct pred? */br_pred_taken == br_taken,
		       /* conf pred value */HighConf);
	}

      /* check for DLite debugger entry condition */
#if 0
      if (dlite_check_break(next_PC,
			    is_write ? ACCESS_WRITE : ACCESS_READ,
			    addr, sim_num_insn, sim_num_insn))
	dlite_main(regs_PC, next_PC, sim_num_insn);
#endif

      /* go to the next instruction */
      regs_PC = next_PC;
      next_PC += SS_INST_SIZE;
    }
}
