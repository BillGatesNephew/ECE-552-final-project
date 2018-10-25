/*
 * sim-profile.c - sample functional simulator implementation w/ profiling
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
 * $Id: sim-profile.c,v 3.1 1998/04/17 16:51:35 skadron Exp $
 *
 * $Log: sim-profile.c,v $
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
 * Revision 1.1  1997/03/11  01:33:36  taustin
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
#include "symbol.h"
#include "options.h"
#include "stats.h"
#include "sim.h"

/*
 * This file implements a functional simulator with profiling support.  Run
 * with the `-h' flag to see profiling options available.
 */

/* track number of insn and refs */
static SS_COUNTER_TYPE sim_num_insn = 0;
static SS_COUNTER_TYPE sim_num_refs = 0;

/* profiling options */
static int prof_all /* = FALSE */;
static int prof_ic /* = FALSE */;
static int prof_inst /* = FALSE */;
static int prof_bc /* = FALSE */;
static int prof_am /* = FALSE */;
static int prof_seg /* = FALSE */;
static int prof_tsyms /* = FALSE */;
static int prof_dsyms /* = FALSE */;
static int load_locals /* = FALSE */;
static int prof_taddr /* = FALSE */;

/* text-based stat profiles */
#define MAX_PCSTAT_VARS 8
static int pcstat_nelt = 0;
static char *pcstat_vars[MAX_PCSTAT_VARS];

/* register simulator-specific options */
void
sim_reg_options(struct opt_odb_t *odb)
{
  opt_reg_header(odb, 
"sim-profile: This simulator implements a functional simulator with\n"
"profiling support.  Run with the `-h' flag to see profiling options\n"
"available.\n"
		 );

  opt_reg_flag(odb, "-all", "enable all profile options",
	       &prof_all, /* default */FALSE, /* print */TRUE, NULL);

  opt_reg_flag(odb, "-iclass", "enable instruction class profiling",
	       &prof_ic, /* default */FALSE, /* print */TRUE, NULL);

  opt_reg_flag(odb, "-iprof", "enable instruction profiling",
	       &prof_inst, /* default */FALSE, /* print */TRUE, NULL);

  opt_reg_flag(odb, "-brprof", "enable branch instruction profiling",
	       &prof_bc, /* default */FALSE, /* print */TRUE, NULL);

  opt_reg_flag(odb, "-amprof", "enable address mode profiling",
	       &prof_am, /* default */FALSE, /* print */TRUE, NULL);

  opt_reg_flag(odb, "-segprof", "enable load/store address segment profiling",
	       &prof_seg, /* default */FALSE, /* print */TRUE, NULL);

  opt_reg_flag(odb, "-tsymprof", "enable text symbol profiling",
	       &prof_tsyms, /* default */FALSE, /* print */TRUE, NULL);

  opt_reg_flag(odb, "-taddrprof", "enable text address profiling",
	       &prof_taddr, /* default */FALSE, /* print */TRUE, NULL);

  opt_reg_flag(odb, "-dsymprof", "enable data symbol profiling",
	       &prof_dsyms, /* default */FALSE, /* print */TRUE, NULL);

  opt_reg_flag(odb, "-internal",
	       "include compiler-internal symbols during symbol profiling",
	       &load_locals, /* default */FALSE, /* print */TRUE, NULL);

  opt_reg_string_list(odb, "-pcstat",
		      "profile stat(s) against text addr's (mult uses ok)",
		      pcstat_vars, MAX_PCSTAT_VARS, &pcstat_nelt, NULL,
		      /* !print */FALSE, /* format */NULL, /* accrue */TRUE);
}

/* check simulator-specific option values */
void
sim_check_options(struct opt_odb_t *odb, int argc, char **argv)
{
  if (prof_all)
    {
      /* enable all options */
      prof_ic = TRUE;
      prof_inst = TRUE;
      prof_bc = TRUE;
      prof_am = TRUE;
      prof_seg = TRUE;
      prof_tsyms = TRUE;
      prof_dsyms = TRUE;
      prof_taddr = TRUE;
    }
}

/* instruction classes */
enum inst_class_t {
  ic_load,		/* load inst */
  ic_store,		/* store inst */
  ic_uncond,		/* uncond branch */
  ic_cond,		/* cond branch */
  ic_icomp,		/* all other integer computation */
  ic_fcomp,		/* all floating point computation */
  ic_trap,		/* system call */
  ic_NUM
};

/* instruction class strings */
static char *inst_class_str[ic_NUM] = {
  "load",		/* load inst */
  "store",		/* store inst */
  "uncond branch",	/* uncond branch */
  "cond branch",	/* cond branch */
  "int computation",	/* all other integer computation */
  "fp computation",	/* all floating point computation */
  "trap"		/* system call */
};

/* instruction class profile */
static struct stat_stat_t *ic_prof = NULL;

/* instruction description strings */
static char *inst_str[OP_MAX];

/* instruction profile */
static struct stat_stat_t *inst_prof = NULL;

/* branch class profile */
enum branch_class_t {
  bc_uncond_dir,	/* direct unconditional branch */
  bc_cond_dir,		/* direct conditional branch */
  bc_call_dir,		/* direct functional call */
  bc_uncond_indir,	/* indirect unconditional branch */
  bc_cond_indir,	/* indirect conditional branch */
  bc_call_indir,	/* indirect function call */
  bc_NUM
};

/* branch class description strings */
static char *branch_class_str[bc_NUM] = {
  "uncond direct",	/* direct unconditional branch */
  "cond direct",	/* direct conditional branch */
  "call direct",	/* direct functional call */
  "uncond indirect",	/* indirect unconditional branch */
  "cond indirect",	/* indirect conditional branch */
  "call indirect"	/* indirect function call */
};

/* branch profile */
static struct stat_stat_t *bc_prof = NULL;

/* address modes */
enum addr_mode_t {
  am_imm,		/* immediate addressing mode */
  am_gp,		/* global data access through global pointer */
  am_sp,		/* stack access through stack pointer */
  am_fp,		/* stack access through frame pointer */
  am_disp,		/* (reg + const) addressing */
  am_rr,		/* (reg + reg) addressing */
  am_NUM
};

/* address mode description strings */
static char *addr_mode_str[am_NUM] = {
  "(const)",		/* immediate addressing mode */
  "(gp + const)",	/* global data access through global pointer */
  "(sp + const)",	/* stack access through stack pointer */
  "(fp + const)",	/* stack access through frame pointer */
  "(reg + const)",	/* (reg + const) addressing */
  "(reg + reg)"		/* (reg + reg) addressing */
};

/* addressing mode profile */
static struct stat_stat_t *am_prof = NULL;

/* address segments */
enum addr_seg_t {
  seg_data,		/* data segment */
  seg_heap,		/* heap segment */
  seg_stack,		/* stack segment */
  seg_NUM
};

/* address segment strings */
static char *addr_seg_str[seg_NUM] = {
  "data segment",	/* data segment */
  "heap segment",	/* heap segment */
  "stack segment",	/* stack segment */
};

/* address segment profile */
static struct stat_stat_t *seg_prof = NULL;

/* bind ADDR to the segment it references */
static enum addr_seg_t			/* segment referenced by ADDR */
bind_to_seg(SS_ADDR_TYPE addr)		/* address to bind to a segment */
{
  if (ld_data_base <= addr && addr < (ld_data_base + ld_data_size))
    return seg_data;
  else if ((ld_data_base + ld_data_size) <= addr && addr < mem_brk_point)
    return seg_heap;
  else if (mem_stack_min <= addr && addr < SS_STACK_BASE)
    return seg_stack;
  else
    panic("cannot bind address to segment");
}

/* text symbol profile */
static struct stat_stat_t *tsym_prof = NULL;
static char **tsym_names = NULL;

/* data symbol profile */
static struct stat_stat_t *dsym_prof = NULL;
static char **dsym_names = NULL;

/* text address profile */
static struct stat_stat_t *taddr_prof = NULL;

/* text-based stat profiles */
static struct stat_stat_t *pcstat_stats[MAX_PCSTAT_VARS];
static SS_COUNTER_TYPE pcstat_lastvals[MAX_PCSTAT_VARS];
static struct stat_stat_t *pcstat_sdists[MAX_PCSTAT_VARS];

/* wedge all stat values into a SS_COUNTER_TYPE */
#define STATVAL(STAT)							\
  ((STAT)->sc == sc_int							\
   ? (SS_COUNTER_TYPE)*((STAT)->variant.for_int.var)			\
   : ((STAT)->sc == sc_uint						\
      ? (SS_COUNTER_TYPE)*((STAT)->variant.for_uint.var)		\
      : ((STAT)->sc == sc_counter					\
	 ? *((STAT)->variant.for_counter.var)				\
	 : (panic("bad stat class"), 0))))

/* register simulator-specific statistics */
void
sim_reg_stats(struct stat_sdb_t *sdb)
{
  int i;

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

  if (prof_ic)
    {
      /* instruction class profile */
      ic_prof = stat_reg_dist(sdb, "sim_inst_class_prof",
			      "instruction class profile",
			      /* initial value */0,
			      /* array size */ic_NUM,
			      /* bucket size */1,
			      /* print format */(PF_COUNT|PF_PDF),
			      /* format */NULL,
			      /* index map */inst_class_str,
			      /* print fn */NULL);
    }

  if (prof_inst)
    {
      int i;
      char buf[512];

      /* conjure up appropriate instruction description strings */
      for (i=0; i < /* skip NA */OP_MAX-1; i++)
	{
	  sprintf(buf, "%-8s %-6s", ss_op2name[i+1], ss_op2format[i+1]);
	  inst_str[i] = mystrdup(buf);
	}
      
      /* instruction profile */
      inst_prof = stat_reg_dist(sdb, "sim_inst_prof",
				"instruction profile",
				/* initial value */0,
				/* array size */ /* skip NA */OP_MAX-1,
				/* bucket size */1,
				/* print format */(PF_COUNT|PF_PDF),
				/* format */NULL,
				/* index map */inst_str,
				/* print fn */NULL);
    }

  if (prof_bc)
    {
      /* instruction branch profile */
      bc_prof = stat_reg_dist(sdb, "sim_branch_prof",
			      "branch instruction profile",
			      /* initial value */0,
			      /* array size */bc_NUM,
			      /* bucket size */1,
			      /* print format */(PF_COUNT|PF_PDF),
			      /* format */NULL,
			      /* index map */branch_class_str,
			      /* print fn */NULL);
    }

  if (prof_am)
    {
      /* instruction branch profile */
      am_prof = stat_reg_dist(sdb, "sim_addr_mode_prof",
			      "addressing mode profile",
			      /* initial value */0,
			      /* array size */am_NUM,
			      /* bucket size */1,
			      /* print format */(PF_COUNT|PF_PDF),
			      /* format */NULL,
			      /* index map */addr_mode_str,
			      /* print fn */NULL);
    }

  if (prof_seg)
    {
      /* instruction branch profile */
      seg_prof = stat_reg_dist(sdb, "sim_addr_seg_prof",
			       "load/store address segment profile",
			       /* initial value */0,
			       /* array size */seg_NUM,
			       /* bucket size */1,
			       /* print format */(PF_COUNT|PF_PDF),
			       /* format */NULL,
			       /* index map */addr_seg_str,
			       /* print fn */NULL);
    }

  if (prof_tsyms)
    {
      int i;

      /* load program symbols */
      sym_loadsyms(ld_prog_fname, load_locals);

      /* conjure up appropriate instruction description strings */
      tsym_names = (char **)calloc(sym_ntextsyms, sizeof(char *));

      for (i=0; i < sym_ntextsyms; i++)
	tsym_names[i] = sym_textsyms[i]->name;
      
      /* text symbol profile */
      tsym_prof = stat_reg_dist(sdb, "sim_text_sym_prof",
				"text symbol profile",
				/* initial value */0,
				/* array size */sym_ntextsyms,
				/* bucket size */1,
				/* print format */(PF_COUNT|PF_PDF),
				/* format */NULL,
				/* index map */tsym_names,
				/* print fn */NULL);
    }

  if (prof_dsyms)
    {
      int i;

      /* load program symbols */
      sym_loadsyms(ld_prog_fname, load_locals);

      /* conjure up appropriate instruction description strings */
      dsym_names = (char **)calloc(sym_ndatasyms, sizeof(char *));

      for (i=0; i < sym_ndatasyms; i++)
	dsym_names[i] = sym_datasyms[i]->name;
      
      /* data symbol profile */
      dsym_prof = stat_reg_dist(sdb, "sim_data_sym_prof",
				"data symbol profile",
				/* initial value */0,
				/* array size */sym_ndatasyms,
				/* bucket size */1,
				/* print format */(PF_COUNT|PF_PDF),
				/* format */NULL,
				/* index map */dsym_names,
				/* print fn */NULL);
    }

  if (prof_taddr)
    {
      /* text address profile (sparse profile), NOTE: a dense print format
         is used, its more difficult to read, but the profiles are *much*
	 smaller, I've assumed that the profiles are read by programs, at
	 least for your sake I hope this is the case!! */
      taddr_prof = stat_reg_sdist(sdb, "sim_text_addr_prof",
				  "text address profile",
				  /* initial value */0,
				  /* print format */(PF_COUNT|PF_PDF),
				  /* format */"0x%lx %lu %.2f",
				  /* print fn */NULL);
    }

  for (i=0; i<pcstat_nelt; i++)
    {
      char buf[512], buf1[512];
      struct stat_stat_t *stat;

      /* track the named statistical variable by text address */

      /* find it... */
      stat = stat_find_stat(sdb, pcstat_vars[i]);
      if (!stat)
	fatal("cannot locate any statistic named `%s'", pcstat_vars[i]);

      /* stat must be an integral type */
      if (stat->sc != sc_int && stat->sc != sc_uint && stat->sc != sc_counter)
	fatal("`-pcstat' statistical variable `%s' is not an integral type",
	      stat->name);

      /* register this stat */
      pcstat_stats[i] = stat;
      pcstat_lastvals[i] = STATVAL(stat);

      /* declare the sparce text distribution */
      sprintf(buf, "%s_by_pc", stat->name);
      sprintf(buf1, "%s (by text address)", stat->desc);
      pcstat_sdists[i] = stat_reg_sdist(sdb, buf, buf1,
					/* initial value */0,
					/* print format */(PF_COUNT|PF_PDF),
					/* format */"0x%lx %lu %.2f",
					/* print fn */NULL);
    }
}

/* local machine state accessor */
static char *					/* err str, NULL for no err */
profile_mstate_obj(FILE *stream,		/* output stream */
		   char *cmd)			/* optional command string */
{
  /* just dump intermediate stats */
  sim_print_stats(stream);

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

    fprintf(outfile, "sim: decoding text segment...");
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
    fprintf(outfile, "done.\n");
  }

  /* initialize the DLite debugger */
  dlite_init(dlite_reg_obj, dlite_mem_obj, profile_mstate_obj);
}


/* print simulator-specific configuration information */
void
sim_aux_config(FILE *stream)		/* output stream */
{
  /* nothing currently */
}

/* dump simulator-specific auxiliary simulator statistics */
void
sim_aux_stats(FILE *stream)		/* output stream */
{
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
  ((unsigned int)((DST_T)(SRC_T)MEM_READ_WORD(addr = (SRC))))

#define __READ_HALF(DST_T, SRC_T, SRC)					\
  ((unsigned int)((DST_T)(SRC_T)MEM_READ_HALF(addr = (SRC))))

#define __READ_BYTE(DST_T, SRC_T, SRC)					\
  ((unsigned int)((DST_T)(SRC_T)MEM_READ_BYTE(addr = (SRC))))

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
  (MEM_WRITE_WORD(addr = (DST), (unsigned int)(SRC)))

#define WRITE_HALF(SRC, DST)						\
  (MEM_WRITE_HALF(addr = (DST), (unsigned short)(unsigned int)(SRC)))

#define WRITE_BYTE(SRC, DST)						\
  (MEM_WRITE_BYTE(addr = (DST), (unsigned char)(unsigned int)(SRC)))

/* system call handler macro */
#define SYSCALL(INST)		(ss_syscall(mem_access, INST))

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

/* destination of last LUI, used for decoding addressing modes */
static unsigned int imm_reg = 0;

/* start simulation, program loaded, processor precise state initialized */
void
sim_main(void)
{
  int i;
  SS_INST_TYPE inst;
  register SS_ADDR_TYPE next_PC;
  register SS_ADDR_TYPE addr;
  register int is_write;
  enum ss_opcode op;
  unsigned int flags;

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
      inst = __UNCHK_MEM_ACCESS(SS_INST_TYPE, regs_PC);

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

      /*
       * profile this instruction
       */
      flags = SS_OP_FLAGS(op);

      if (prof_ic)
	{
	  enum inst_class_t ic;

	  /* compute instruction class */
	  if (flags & F_LOAD)
	    ic = ic_load;
	  else if (flags & F_STORE)
	    ic = ic_store;
	  else if (flags & F_UNCOND)
	    ic = ic_uncond;
	  else if (flags & F_COND)
	    ic = ic_cond;      
	  else if (flags & F_ICOMP)
	    ic = ic_icomp;
	  else if (flags & F_FCOMP)
	    ic = ic_fcomp;
	  else if (flags & F_TRAP)
	    ic = ic_trap;
	  else
	    panic("instruction has no class");

	  /* update instruction class profile */
	  stat_add_sample(ic_prof, (int)ic);
	}

      if (prof_inst)
	{
	  /* update instruction profile */
	  stat_add_sample(inst_prof, (int)op - /* skip NA */1);
	}

      if (prof_bc)
	{
	  enum branch_class_t bc;

	  /* compute instruction class */
	  if (flags & F_CTRL)
	    {
	      if ((flags & (F_CALL|F_DIRJMP)) == (F_CALL|F_DIRJMP))
		bc = bc_call_dir;
	      else if ((flags & (F_CALL|F_INDIRJMP)) == (F_CALL|F_INDIRJMP))
		bc = bc_call_indir;
	      else if ((flags & (F_UNCOND|F_DIRJMP)) == (F_UNCOND|F_DIRJMP))
		bc = bc_uncond_dir;
	      else if ((flags & (F_UNCOND|F_INDIRJMP))== (F_UNCOND|F_INDIRJMP))
		bc = bc_uncond_indir;
	      else if ((flags & (F_COND|F_DIRJMP)) == (F_COND|F_DIRJMP))
		bc = bc_cond_dir;
	      else if ((flags & (F_COND|F_INDIRJMP)) == (F_COND|F_INDIRJMP))
		bc = bc_cond_indir;
	      else
		panic("branch has no class");

	      /* update instruction class profile */
	      stat_add_sample(bc_prof, (int)bc);
	    }
	}

      if (prof_am)
	{
	  enum addr_mode_t am;

	  /* track LUI's for decoding immediate addressing modes */
	  if (op == LUI)
	    {
	      /* remember the immediate destination */
	      imm_reg = (RT);
	    }

	  if (flags & F_MEM)
	    {
	      /* decode the addressing mode */
	      if (flags & F_DISP)
		{
		  if ((BS) == imm_reg)
		    am = am_imm;
		  else if ((BS) == Rgp)
		    am = am_gp;
		  else if ((BS) == Rsp)
		    am = am_sp;
		  else if ((BS) == Rfp && bind_to_seg(addr) == seg_stack)
		    am = am_fp;
		  else
		    am = am_disp;
		}
	      else if (flags & F_RR)
		am = am_rr;
	      else
		panic("cannot decode addressing mode");

	      /* update the addressing mode profile */
	      stat_add_sample(am_prof, (int)am);

	      /* blow away IMM_REG after next load/store inst is probed */
	      imm_reg = 0;
	    }
	}

      if (prof_seg)
	{
	  if (flags & F_MEM)
	    {
	      /* update instruction profile */
	      stat_add_sample(seg_prof, (int)bind_to_seg(addr));
	    }
	}

      if (prof_tsyms)
	{
	  int tindex;

	  /* attempt to bind inst address to a text segment symbol */
	  sym_bind_addr(regs_PC, &tindex, /* !exact */FALSE, sdb_text);

	  if (tindex >= 0)
	    {
	      if (tindex > sym_ntextsyms)
		panic("bogus text symbol index");

	      stat_add_sample(tsym_prof, tindex);
	    }
	  /* else, could not bind to a symbol */
	}

      if (prof_dsyms)
	{
	  int dindex;

	  if (flags & F_MEM)
	    {
	      /* attempt to bind inst address to a text segment symbol */
	      sym_bind_addr(addr, &dindex, /* !exact */FALSE, sdb_data);

	      if (dindex >= 0)
		{
		  if (dindex > sym_ndatasyms)
		    panic("bogus data symbol index");

		  stat_add_sample(dsym_prof, dindex);
		}
	      /* else, could not bind to a symbol */
	    }
	}

      if (prof_taddr)
	{
	  /* add regs_PC exec event to text address profile */
	  stat_add_sample(taddr_prof, regs_PC);
	}

      /* update any stats tracked by PC */
      for (i=0; i<pcstat_nelt; i++)
	{
	  SS_COUNTER_TYPE newval;
	  int delta;

	  /* check if any tracked stats changed */
	  newval = STATVAL(pcstat_stats[i]);
	  delta = newval - pcstat_lastvals[i];
	  if (delta != 0)
	    {
	      stat_add_samples(pcstat_sdists[i], regs_PC, delta);
	      pcstat_lastvals[i] = newval;
	    }

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
