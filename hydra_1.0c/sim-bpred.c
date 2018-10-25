/*
 * sim-bpred.c - sample branch predictor simulator implementation
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
 * $Id: sim-bpred.c,v 3.65 1998/08/06 17:22:58 skadron Exp $
 *
 * $Log: sim-bpred.c,v $
 * Revision 3.65  1998/08/06 17:22:58  skadron
 * Bug fix in fix-addr handling, and made compatible with bpred.c v3.65
 *
 * Revision 3.55  1998/08/04 19:13:43  skadron
 * 1. Modified to use bpred.c v3.55
 * 2. Added warming-up; modified stats accordingly (PP)
 * 3. Added support for BPredBackwards
 * 4. Turned off mem-ref counting and DLite (with ifdefs)
 * 5. Added fix-addrs options
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
 * Revision 2.6  1997/11/20 01:05:31  skadron
 * Changes as a result of retstack bug fixes
 *
 * Revision 2.5  1997/09/16 01:56:51  skadron
 * Added outfile
 *
 * Revision 2.4  1997/07/15 21:25:05  skadron
 * Added some stats-keeping about types of branches
 *
 * Revision 2.3  1997/07/11 21:44:19  skadron
 * Updated to incorporate final changes for public 2.0 release
 *
 * Revision 2.2  1997/07/08 03:45:58  skadron
 * Updated to reflect Todd/Milo's version 2.0e; includes memory-leak fix
 *
 * Revision 2.1  1997/07/06 03:25:06  skadron
 * Initial rev
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "misc.h"
#include "ss.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "syscall.h"
#include "options.h"
#include "stats.h"
#include "bpred.h"
#include "sim.h"
#ifdef USE_DLITE
#include "dlite.h"
#endif

/* Temp compatibility hack */
int sim_warmup = FALSE;

/*
 * This file implements a branch predictor analyzer.
 */

/* branch predictor type {nottaken|taken|perfect|bimod|2lev} */
static char *pred_type;

/* perfect prediction enabled (currently just prevents a bpred module
 * from being instantiated) */
static int pred_perfect = FALSE;

/* optional BTB post-patching */
static int fix_addrs;
static int fix_addrs_indir;
static int fix_addrs_except_retstack;

/* experimental: permit global history bits to be used with local history
 * bits */
static int bpred_merge_hist;	/* hash ghist w/ local hist */
static int bpred_merge_hist_shift;/* hash shifted ghist w/ local hist */
static int bpred_cat_hist;	/* concat ghist w/ local hist */
static int bpred_gshare_shift;  /* hash shifted baddr w/ hist */
static int bpred_gshare_drop_lsbits; /* how many extra (beyond the automatic 3)
				      * low-order bits to drop from the baddr 
				      * when hashing the address with hist */

/* bimodal predictor config (<counter_bits> <table_size> <retstack_size> <agree?>) */
static int bimod_nelt = 4;
static int bimod_config[4] = 
  { /* counter bits */2, /* bimod tbl size */2048, /* retstack size */8,
    /* agree? */0 };

/* 2-level predictor #1 config
 * (<counter_bits> <l1size> <l2size> <hist_size> <retstack_size> <gshare> <agreee> <spec history update?> <spec history fixup type>) */
static int twolev_nelt = 9;
static int twolev_config[9] = 
  { /* cntr bits*/2, /* l1size */1, /* l2size */1024, /* hist */8, 
    /* retstack size */8, /* gshare? */0, /* agree? */0,
    /* speculative history update */0, /* spec hist fixup type */1 };

/* 2-level predictor #2 config 
 * (<counter_bits> <l1size> <l2size> <hist_size> <retstack_size> <gshare> <agree> <spec history update?> <spec history fixup type>))
 * (for hybrid predictor component #2) */
static int twolev2_nelt = 9;
static int twolev2_config[9] = 
  { /* cntr bits */2, /* l1size */1024, /* l2size */1024, /* hist */8, 
    /* retstack size */0, /* gshare? */0, /* agree? */0,
    /* speculative history update */0, /* spec history fixup type */1 };

/* hybrid predictor config 
 * (<counter_bits>:<pred-pred sz>:<pred type 1>:<pred type 2>:<global shreg sz>:<retstack sz>:<gshare>:<spec history update?>:<spec history fixup type>)
 */
static char *hybrid_config;

/* BTB predictor config (<num_sets> <associativity>) */
static int btb_nelt = 2;
static int btb_config[2] =
  { /* nsets */512, /* assoc */4 };

/* ret-addr stack size 
 * By default, the ret-addr stack size is specified with -bpred:retstack and
 * overrides the size specified in the PHT configuration (eg bimod_cfg).
 * Turning on "old_style_retstack_interface" uses the value from the PHT
 * config, overriding the -bpred:retstack specification */
static int retstack_size;
static int old_style_retstack_interface;

/* simulation length, in instructions */
static unsigned int sim_length, warmup_length;

/* warmup stats */
static SS_COUNTER_TYPE primed_insts = 0, primed_branches = 0;
#ifdef COUNT_MEM
static int primed_refs = 0;
#endif

/* branch predictor */
static struct bpred *pred;

/* track number of insn and refs */
static SS_COUNTER_TYPE sim_num_insn;
#ifdef COUNT_MEM
static SS_COUNTER_TYPE sim_num_refs;
#endif

/* total number of branches executed */
static SS_COUNTER_TYPE sim_num_branches;
static SS_COUNTER_TYPE num_cond_branches;
static SS_COUNTER_TYPE num_uncond_branches;
static SS_COUNTER_TYPE num_indir_branches;
static SS_COUNTER_TYPE num_fpcond_branches;


/* register simulator-specific options */
void
sim_reg_options(struct opt_odb_t *odb)
{
  opt_reg_header(odb, 
"sim-bpred: This simulator implements a branch predictor analyzer.\n"
		 );

  /* branch predictor options */
  opt_reg_note(odb,
"  Branch predictor configuration examples for 2-level predictor:\n"
"    Configurations:   N, M, W, X\n"
"      N   # entries in first level (# of shift register(s))\n"
"      W   width of shift register(s)\n"
"      M   # entries in 2nd level (# of counters, or other FSM)\n"
"      X   (yes-1/no-0) xor history and address for 2nd level index\n"
"    Sample predictors:\n"
"      GAg     : 1, W, 2^W, 0\n"
"      GAp     : 1, W, M (M > 2^W), 0\n"
"      PAg     : N, W, 2^W, 0\n"
"      PAp     : N, W, M (M == 2^(N+W)), 0\n"
"      gshare  : 1, W, 2^W, 1\n"
"  Predictor `hybrid' combines 2 of the above predictors.\n"
               );

  opt_reg_string(odb, "-bpred",
		 "branch predictor type "
		 "{nottaken|taken|perfect|bimod|2lev|hybrid}",
		 &pred_type, /* default */"bimod",
		 /* print */TRUE, /* format */NULL);

  opt_reg_int_list(odb, "-bpred:bimod", 
		   "bimodal pred cfg (<cntr_bits> <tbl_size> <retstack_size>"
		   " <agree?>)",
		   bimod_config, bimod_nelt, &bimod_nelt, bimod_config,
		   /* print */TRUE, /* format */NULL, /* !accrue */FALSE);

  opt_reg_int_list(odb, "-bpred:2lev",
		   "2lev pred cfg (<cntr_bits> <l1_sz> <l2_sz> <hist_sz> "
		   "<retstack_size> <gshare?> <agree?> <spec-update?> "
		   "<spec-update repair?>)",
		   twolev_config, twolev_nelt, &twolev_nelt, twolev_config,
		   /* print */TRUE, /* format */NULL, /* !accrue */FALSE);

  opt_reg_int_list(odb, "-bpred:2lev2",
		   "2lev pred cfg #2 (<cntr_bits> <l1_sz> <l2_sz> <hist_sz> "
		   "<retstack_size> <gshare?> <agree?> <spec-update?> "
		   "<spec-update repair?>)",
		   twolev2_config, twolev2_nelt, &twolev2_nelt, twolev2_config,
		   /* print */TRUE, /* format */NULL, /* !accrue */FALSE);

  opt_reg_string(odb, "-bpred:hybrid",
		 "hybrid predictor config (<cntr_bits>:<tbl-sz>:<pred1>:<pred2>:<sh-reg-sz>:<retstack_sz>:<gshare?>:<spec-update?>:<spec-update repair?>)",
		 &hybrid_config, "2:4096:bimod:2lev2:12:8:0:0:1",
		 /* print */TRUE, NULL);

  opt_reg_int_list(odb, "-bpred:btb", 
		   "BTB config (<num_sets> <associativity>)",
		   btb_config, btb_nelt, &btb_nelt, btb_config,
		   /* print */TRUE, /* format */NULL, /* !accrue */FALSE);

  opt_reg_int(odb, "-bpred:retstack", "retstack size",
	      &retstack_size, /* default */8, /* print */TRUE, NULL);

  opt_reg_flag(odb, "-bpred:use_old_retstack_interface",
	       "use retstack size from the PHT config instead of from"
	       " '-bpred:retstack'", 
	       &old_style_retstack_interface, /* default */FALSE, TRUE, NULL);
  
  opt_reg_int(odb, "-bpred:merge_hist",
	      "hash global with local history bits; argument specifies"
	      " global history size",
	      &bpred_merge_hist, /* default */FALSE, TRUE, NULL);
  
  opt_reg_int(odb, "-bpred:merge_hist_shift",
	      "hash shifted global with local history bits; argument specifies"
	      " global history size",
	      &bpred_merge_hist_shift, /* default */FALSE, TRUE, NULL);
  
  opt_reg_int(odb, "-bpred:cat_hist",
	      "concatenate global and local history bits; argument specifies"
	      " global history size",
	      &bpred_cat_hist, /* default */FALSE, TRUE, NULL);
  
  opt_reg_int(odb, "-bpred:gshare_shift",
	      "hash shifted baddr with global history bits; argument specifies"
	      " address left-shift",
	      &bpred_gshare_shift, /* default */0, TRUE, NULL);
  
  opt_reg_int(odb, "-bpred:gshare_drop_lsbits",
	      "number of low-order br-address bits (beyond the automatic 3)"
	      " to drop for gshare",
	      &bpred_gshare_drop_lsbits, /* default */0, TRUE, NULL);
  
  opt_reg_flag(odb, "-bpred:fix_addrs",
	       "correct address prediction for br's w/ correct direction?",
	       &fix_addrs, /* default */FALSE, TRUE, NULL);
  
  opt_reg_flag(odb, "-bpred:fix_addrs_indir",
	       "correct address prediction for indir jumps?",
	       &fix_addrs_indir, /* default */FALSE, TRUE, NULL);

  opt_reg_flag(odb, "-bpred:perf_except_retstack",
	       "correct address prediction for indir jumps but "
	       "not for retstack?",
	       &fix_addrs_except_retstack, /* default */FALSE, TRUE, NULL);

  opt_reg_uint(odb, "-warmup_insts",
	       "number of instructions for which stats are discarded",
	       &warmup_length, /* default */0, /* print */TRUE, NULL);

  opt_reg_uint(odb, "-sim_insts",
	   "total number of instructions for which to simulate before exiting",
	   &sim_length, /* default */0, /* print */TRUE, NULL);
}

/* check simulator-specific option values */
struct bpred *
sim_check_bpred(struct opt_odb_t *odb,		/* options database */
		char *pred_type,		/* pred type to check/create */
		int hybrid_component_p)		/* TRUE = component f/ hybr.,
						 * FALSE = top-level bpred */
{
  struct bpred *pred = NULL;
  int btb_cfg_save[2];

  if (hybrid_component_p)
    {
      btb_cfg_save[0] = btb_config[0];
      btb_cfg_save[1] = btb_config[1];
      btb_config[0] = 0;
      btb_config[1] = 0;
    }

  if (!mystricmp(pred_type, "perfect"))
    {
      /* perfect predictor */
      pred = NULL;
      pred_perfect = TRUE;
    }
  else if (!mystricmp(pred_type, "taken"))
    {
      /* static predictor, not taken */
      if (btb_nelt != 2 && !hybrid_component_p)
	fatal("bad btb config (<num_sets> <associativity>)");
      pred = bpred_create(BPredTaken, 0, 0, 0, 0, 0, 0, 0, 0,
			  /* btb sets */btb_config[0],
			  /* btb assoc */btb_config[1],
			  /* ret-addr stack size */
			   (old_style_retstack_interface ? 0 : retstack_size));
    }
  else if (!mystricmp(pred_type, "nottaken"))
    {
      /* static predictor, taken */
      if (btb_nelt != 2 && !hybrid_component_p)
	fatal("bad btb config (<num_sets> <associativity>)");
      pred = bpred_create(BPredNotTaken, 0, 0, 0, 0, 0, 0, 0, 0,
			  /* btb sets */btb_config[0],
			  /* btb assoc */btb_config[1],
			  /* ret-addr stack size */
			   (old_style_retstack_interface ? 0 : retstack_size));
    }
  else if (!mystricmp(pred_type, "backwards"))
    {
      /* static predictor, backwards-taken/forwards-not-taken */
      if (btb_nelt != 2 && !hybrid_component_p)
	fatal("bad btb config (<num_sets> <associativity>)");
      pred = bpred_create(BPredBackwards, 0, 0, 0, 0, 0, 0, 0, 0,
			  /* btb sets */btb_config[0],
			  /* btb assoc */btb_config[1],
			  /* ret-addr stack size */
			   (old_style_retstack_interface ? 0 : retstack_size));
    }
  else if (!mystricmp(pred_type, "bimod"))
    {
      /* bimodal predictor, bpred_create() checks args */
      if (bimod_nelt != 4)
	fatal("bad bimod predictor config "
	      "(<counter_bits> <table_size> <retstack_size> <agree?>)");
      if (btb_nelt != 2 && !hybrid_component_p)
	fatal("bad btb config (<num_sets> <associativity>)");
      pred = bpred_create(BPred2bit, 
			  /* bimod table size */bimod_config[1],
			  /* l2 size */0,
			  /* history reg size */0,
			  /* (gshare not applicable) */0,
			  /* agree? */bimod_config[3],
			  /* spec update not applicable */0,
			  /* spec update repair not applicable */0,
			  /* counter bits */bimod_config[0],
			  /* btb sets */btb_config[0],
			  /* btb assoc */btb_config[1],
			  /* ret-addr stack size */
			      hybrid_component_p
			      ? 0
			      : (old_style_retstack_interface 
				 ? bimod_config[2]
				 : retstack_size));
      if (bimod_config[2] && !old_style_retstack_interface)
	warn("ret-addr stack size specified in PHT config, but overridden");
      else if (retstack_size && old_style_retstack_interface)
	warn("ret-addr stack size specified directly with -bpred:retstack,"
	     " but overriden by value in PHT config");
    }
  else if (!mystricmp(pred_type, "2lev"))
    {
      /* 2-level adaptive predictor, bpred_create() checks args */
      if (twolev_nelt != 9)
	fatal("bad 2-level predictor config "
	      "(<counter_bits> <l1size> <l2size> <hist_size> <retstack_size> "
	      "<gshare?> <agree?> <spec_update?> <spec_update_repair?>)");
      if (btb_nelt != 2 && !hybrid_component_p)
	fatal("bad btb config (<num_sets> <associativity>)");
      pred = bpred_create(BPred2Level,
			  /* l1 size */twolev_config[1],
			  /* l2 size */twolev_config[2],
			  /* history reg size */twolev_config[3],
			  /* gshare? */twolev_config[5],
			  /* agree? */twolev_config[6],
			  /* spec update? */twolev_config[7],
			  /* spec update repair? */twolev_config[8],
			  /* counter bits */twolev_config[0],
			  /* btb sets */btb_config[0],
			  /* btb assoc */btb_config[1],
			  /* ret-addr stack size */
			      hybrid_component_p
			      ? 0
			      : (old_style_retstack_interface 
				 ? twolev_config[4]
				 : retstack_size));
      if (twolev_config[4] && !old_style_retstack_interface)
	warn("ret-addr stack size specified in PHT config, but overridden");
      else if (retstack_size && old_style_retstack_interface)
	warn("ret-addr stack size specified directly with -bpred:retstack,"
	     " but overriden by value in PHT config");
    }
  else if (!mystricmp(pred_type, "2lev2"))
    {
      /* 2-level adaptive predictor, bpred_create() checks args */
      if (twolev2_nelt != 9)
	fatal("bad 2-level predictor #2 config "
	      "(<counter_bits> <l1size> <l2size> <hist_size> <retstack_size> "
	      "<gshare?> <agree?> <spec_update?> <spec_update_repair?>)");
      if (btb_nelt != 2 && !hybrid_component_p)
	fatal("bad btb config (<num_sets> <associativity>)");
      pred = bpred_create(BPred2Level,
			  /* l1 size */twolev2_config[1],
			  /* l2 size */twolev2_config[2],
			  /* history reg size */twolev2_config[3],
			  /* gshare? */twolev2_config[5],
			  /* agree? */twolev2_config[6],
			  /* spec update? */twolev2_config[7],
			  /* spec update repair? */twolev2_config[8],
			  /* counter bits */twolev2_config[0],
			  /* btb sets */btb_config[0],
			  /* btb assoc */btb_config[1],
			  /* ret-addr stack size */
			      hybrid_component_p
			      ? 0
			      : (old_style_retstack_interface 
				 ? twolev2_config[4]
				 : retstack_size));
      if (twolev2_config[4] && !old_style_retstack_interface)
	warn("ret-addr stack size specified in PHT config, but overridden");
      else if (retstack_size && old_style_retstack_interface)
	warn("ret-addr stack size specified directly with -bpred:retstack,"
	     " but overriden by value in PHT config");
    }
  else if (!mystricmp(pred_type, "hybrid"))
    {
      /* McFarling-style hybrid predictor, bpred_hybrid_create() checks args */
      int cntr_bits, tbl_sz, shreg_sz, h_retstack_sz, gshare_p,
	  spec_update_p, spec_update_repair;
      char pred1_nm[32], pred2_nm[32];
      struct bpred *pred1, *pred2;

      if (hybrid_component_p)
	fatal("hybrid predictor can't have another hybrid as a component");

      if (sscanf(hybrid_config, "%d:%d:%[^:]:%[^:]:%d:%d:%d:%d:%d",
		 &cntr_bits, &tbl_sz, pred1_nm, pred2_nm, &shreg_sz, 
		 &h_retstack_sz, &gshare_p, &spec_update_p, &spec_update_repair
		 ) != 9)
	fatal("bad hybrid-predictor parms: "
	      "(<counter_bits>:<table_size>:<pred1>:<pred2>:<hist_size>:"
	      "<retstack_sz>:<gshare?>:<spec-update?>:<spec-update repair?>)");

      pred1 = sim_check_bpred(odb, pred1_nm, TRUE);
      pred2 = sim_check_bpred(odb, pred2_nm, TRUE);

      pred = bpred_hybrid_create(/* predictor-predictor table size */tbl_sz,
				 /* pred component #1 */pred1,
				 /* pred component #2 */pred2,
				 /* global shift-reg size */shreg_sz,
				 /* gshare? */gshare_p,
				 /* spec update? */spec_update_p,
				 /* spec update repair? */spec_update_repair,
				 /* counter bits */cntr_bits,
				 /* btb sets */btb_config[0],
				 /* btb assoc */btb_config[1],
				 /* ret-addr stack size */
				     (old_style_retstack_interface 
				      ? h_retstack_sz
				      : retstack_size));
      if (h_retstack_sz && !old_style_retstack_interface)
	warn("ret-addr stack size specified in PHT config, but overridden");
      else if (retstack_size && old_style_retstack_interface)
	warn("ret-addr stack size specified directly with -bpred:retstack,"
	     " but overriden by value in PHT config");    }
  else
    fatal("cannot parse predictor type `%s'", pred_type);

  if (hybrid_component_p)
    {
      btb_config[0] = btb_cfg_save[0];
      btb_config[1] = btb_cfg_save[1];
    }

  if ((bpred_merge_hist && bpred_cat_hist) 
      || (bpred_merge_hist_shift && bpred_cat_hist))
    fatal("merge-hist and cat-hist are mutually exclusive");
  if (bpred_merge_hist && bpred_merge_hist_shift)
    fatal("shifted and unshifted merge-hist are mutually exclusive");

  /* FIXME: allow these to be specified per-bpred-component */
  if (pred && pred->class == BPredHybrid)
    bpred_create_aux(pred, 
	   /* type of retstack fixup to use */RETSTACK_PATCH_NONE,
	   /* do rets update BTB? */FALSE,
	   /* must caller supply TOS? */FALSE,
	   /* global hist size to merge w/ local bits */0,
	   /* global hist size to merge w/ local bits */0,
	   /* global hist size to concat to local bits */0,
	   /* bpred left shift for gshare */0,
	   /* low-order baddr bits to discard */0);
  /* FIXME: This is a temporary HACK: */
  else if (pred && pred->class == BPred2Level && pred->dirpred.two.l1size == 1)
    bpred_create_aux(pred, 
	   /* type of retstack fixup to use */hybrid_component_p 
		                              ? 0
		                              : RETSTACK_PATCH_NONE,
	   /* do rets update BTB? */FALSE,
	   /* must caller supply TOS? */FALSE,
	   /* global hist size to merge w/ local bits */0,
	   /* global hist size to merge w/ local bits */0,
	   /* global hist size to concat to local bits */0,
	   /* bpred left shift for gshare */bpred_gshare_shift,
	   /* low-order baddr bits to discard */bpred_gshare_drop_lsbits);
  else if (pred && pred->class == BPred2Level)
    bpred_create_aux(pred, 
	   /* type of retstack fixup to use */hybrid_component_p 
		                              ? 0
		                              : RETSTACK_PATCH_NONE,
	   /* do rets update BTB? */FALSE,
	   /* must caller supply TOS? */FALSE,
	   /* global hist size to merge w/ local bits */bpred_merge_hist,
	   /* global hist size to merge w/ local bits */bpred_merge_hist_shift,
	   /* global hist size to concat to local bits */bpred_cat_hist,
	   /* bpred left shift for gshare */bpred_gshare_shift,
	   /* low-order baddr bits to discard */bpred_gshare_drop_lsbits);
  else if (pred)
    bpred_create_aux(pred, 
	   /* type of retstack fixup to use */hybrid_component_p 
		                              ? 0
		                              : RETSTACK_PATCH_NONE,
	   /* do rets update BTB? */FALSE,
	   /* must caller supply TOS? */FALSE,
	   /* global hist size to merge w/ local bits */0,
	   /* global hist size to merge w/ local bits */0,
	   /* global hist size to concat to local bits */0,
	   /* bpred left shift for gshare */0,
	   /* low-order baddr bits to discard */0);

  return pred;
}

void
sim_check_options(struct opt_odb_t *odb, int argc, char **argv)
{
  /* auxiliary function checks bpred opts */
  pred = sim_check_bpred(odb, pred_type, FALSE);
}

/* register simulator-specific statistics */
void
sim_reg_stats(struct stat_sdb_t *sdb)
{
  stat_reg_counter(sdb, "sim_num_insn",
		   "total number of instructions executed",
		   &sim_num_insn, 0, NULL);
#ifdef COUNT_MEM
  stat_reg_counter(sdb, "sim_num_refs",
		   "total number of loads and stores executed",
		   &sim_num_refs, 0, NULL);
#endif
  stat_reg_formula(sdb, "sim_num_insn.PP",
		   "post-warmup number of instructions committed",
		   "sim_num_insn - primed_insts", "%12.0f");
#ifdef COUNT_MEM
  stat_reg_formula(sdb, "sim_num_refs.PP",
		   "post-warmup number of loads and stores committed",
		   "sim_num_refs - primed_refs", "%12.0f");
#endif
  stat_reg_int(sdb, "sim_elapsed_time",
	       "total simulation time in seconds",
	       (int *)&sim_elapsed_time, 0, NULL);
  stat_reg_formula(sdb, "sim_inst_rate",
		   "simulation speed (in insts/sec)",
		   "sim_num_insn / sim_elapsed_time", NULL);

  stat_reg_counter(sdb, "sim_num_branches",
                   "total number of branches executed",
                   &sim_num_branches, /* initial value */0, /* format */NULL);
  stat_reg_formula(sdb, "sim_num_branches.PP",
                   "post-prime number of branches executed",
		   "sim_num_branches - primed_branches", "%12.0f");
  stat_reg_formula(sdb, "sim_IPB.PP",
                   "post-prime instruction per branch",
                   "sim_num_insn.PP / sim_num_branches.PP", /* format */NULL);

  stat_reg_counter(sdb, "num_cond_branches.PP",
		   "post-prime number of conditional branches executed",
		   &num_cond_branches, /* init */0, NULL);
  stat_reg_counter(sdb, "num_uncond_branches.PP",
		   "post-prime number of unconditional branches executed",
		   &num_uncond_branches, /* init */0, NULL);
  stat_reg_counter(sdb, "num_indir_branches.PP",
		   "post-prime number of indirect branches executed",
		   &num_indir_branches, /* init */0, NULL);
  stat_reg_counter(sdb, "num_fpcond_branches.PP",
		   "post-prime number of FP-conditional branches executed",
		   &num_fpcond_branches, /* init */0, NULL);

  /* priming info */
  stat_reg_counter(sdb, "primed_insts",
		   "number of insts for which state was primed",
		   &primed_insts, 0, NULL);
#ifdef COUNT_MEM
  stat_reg_counter(sdb, "primed_refs",
		   "number of refs for which state was primed",
		   &primed_refs, 0, NULL);
#endif
  stat_reg_counter(sdb, "primed_branches",
		   "number of branches for which state was primed",
		   &primed_branches, 0, NULL);

  /* register predictor stats */
  if (pred)
    bpred_reg_stats(pred, sdb);
}

/* initialize the simulator */
void
sim_init(void)
{
  SS_INST_TYPE inst;

  sim_num_insn = 0;
#ifdef COUNT_MEM
  sim_num_refs = 0;
#endif

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

#ifdef USE_DLITE
  /* initialize the DLite debugger */
  dlite_init(dlite_reg_obj, dlite_mem_obj, dlite_mstate_obj);
#endif
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
  /* nada */
}

/* un-initialize simulator-specific state */
void
sim_uninit(void)
{
  /* nada */
}

void 
after_priming(void)
{
  primed_insts = sim_num_insn;
  primed_branches = sim_num_branches;
#ifdef COUNT_MEM
  primed_refs = sim_num_refs;
#endif

  num_cond_branches = 0;
  num_uncond_branches = 0;
  num_indir_branches = 0;
  num_fpcond_branches = 0;

  bpred_after_priming(pred);
}

/*
 * configure the execution engine
 */

/*
 * precise architected register accessors
 */

/* next program counter */
#define SET_NPC(EXPR)		(next_PC = (EXPR))

/* target program counter */
#undef  SET_TPC
#define SET_TPC(EXPR)		(target_PC = (EXPR))

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

/* start simulation, program loaded, processor precise state initialized */
void
sim_main(void)
{
  SS_INST_TYPE inst;
  register SS_ADDR_TYPE pred_PC, next_PC, target_PC;
  register SS_ADDR_TYPE addr;
  enum ss_opcode op;
  register int is_write;
  struct bpred_update_info b_update_rec;
  struct bpred_recover_info ignored1;
  int ignored0 = -1;

  fprintf(outfile, "sim: ** starting functional simulation **\n");

  /* set up initial default next PC */
  next_PC = regs_PC + SS_INST_SIZE;

#ifdef USE_DLITE
  /* check for DLite debugger entry condition */
  if (dlite_check_break(regs_PC, /* no access */0, /* addr */0, 0, 0))
    dlite_main(regs_PC - SS_INST_SIZE, regs_PC, sim_num_insn);
#endif

  while (TRUE)
    {
      if ((sim_length && sim_num_insn >= sim_length) 
	  || sim_exit_now)
	exit_now(0);

      if (warmup_length && sim_num_insn >= warmup_length)
	{
	  warmup_length = 0;
	  after_priming();
	}

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

#ifdef COUNT_MEM
      if (SS_OP_FLAGS(op) & F_MEM)
	{
	  sim_num_refs++;
	  if (SS_OP_FLAGS(op) & F_STORE)
	    is_write = TRUE;
	}
#endif

      if (SS_OP_FLAGS(op) & F_CTRL)
	{
	  sim_num_branches++;

	  if (SS_OP_FLAGS(op) & F_COND)
	    num_cond_branches++;
	  else
	    {
	      assert(SS_OP_FLAGS(op) & F_UNCOND);
	      num_uncond_branches++;
	    }

	  if (SS_OP_FLAGS(op) & F_INDIRJMP)
	    num_indir_branches++;
	  if (SS_OP_FLAGS(op) & F_FPCOND)
	    num_fpcond_branches++;

	  if (pred)
	    {
	      SS_ADDR_TYPE btarget;
	      if (SS_OP_FLAGS(op) & F_COND)
		btarget = regs_PC + 8 + (OFS << 2);
	      if ((SS_OP_FLAGS(op)&(F_UNCOND|F_DIRJMP)) == (F_UNCOND|F_DIRJMP))
		btarget = (regs_PC & 036000000000) | (TARG << 2);

	      /* get the next predicted fetch address */
	      pred_PC = bpred_lookup(pred, regs_PC, btarget, next_PC, 0, op, 
				     (RS) == 31, (RD) == 31, &ignored0, TRUE,
				     &b_update_rec, &ignored1);
	      if (pred_PC == 0)
		/* predicted not taken */
		pred_PC = regs_PC + sizeof(SS_INST_TYPE);

#ifdef TRACE_BPRED
	      fprintf(outfile, "0x%08x %s\t%s\n",
		      regs_PC, SS_OP_NAME(op), 
		      (pred_PC == next_PC) ? "" : "x");
#endif

	      /* if simulating some kind of perfect BTB, modify pred_PC
	       * accordingly */
	      if (fix_addrs_indir && (SS_OP_FLAGS(op) & F_INDIRJMP))
		{
		  /* we know indir jumps are taken, so we can use next_PC */
		  if (!fix_addrs_except_retstack && op == JR && (RS) == 31)
		    pred_PC = next_PC;
		  else if (op != JR || (RS) != 31)
		    pred_PC = next_PC;
		}
	      else if (fix_addrs 
		       && /* direct */(!(SS_OP_FLAGS(op) & F_INDIRJMP))
		       && /* pred taken */(pred_PC != regs_PC + SS_INST_SIZE))
		{
		  /* must use computed btarget; next_PC could correspond to
		   * not-taken */
		  pred_PC = btarget;
		}

	      bpred_update(pred, regs_PC, next_PC, OFS, 
			   /* taken? */next_PC != regs_PC + SS_INST_SIZE,
			   /* pred taken? */pred_PC != regs_PC + SS_INST_SIZE,
			   /* correct pred? */pred_PC == next_PC,
			   /* opcode */op, (RS) == 31,
			   /* retstack gate */TRUE,
			   /* hybrid_component*/FALSE,
			   /* dir predictor update pointer */b_update_rec,
			   &ignored1);
	    }
	}

#ifdef USE_DLITE
      /* check for DLite debugger entry condition */
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
