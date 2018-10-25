/*
 * bpred.h - branch predictor interfaces
 *
 * This file is based on the SimpleScalar (see below) distribution of
 * bpred.h, but has been extensively modified by Kevin Skadron
 * to be part of the HydraScalar simulator.
 * Revisions Copyright (C) 1998, 1999.
 * skadron@cs.princeton.edu
 *
 * This file is a part of the SimpleScalar tool suite written by
 * Todd M. Austin as a part of the Multiscalar Research Project.
 * Extensive additions and revisions have been added by Kevin Skadron.
 * Copyright (C) 1998 by Kevin Skadron
 *  
 * The tool suite is currently maintained by Doug Burger and Todd M. Austin.
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
 * INTERNET: dburger@cs.wisc.edu, skadron@cs.princeton.edu
 * US Mail:  1210 W. Dayton Street, Madison, WI 53706
 *
 * $Id: bpred.h,v 3.100 1998/08/06 21:23:32 skadron Exp $
 *
 * $Log: bpred.h,v $
 * Revision 3.100  1998/08/06 21:23:32  skadron
 * NOTE: This rev marks the end of the '98 Micro/HPCA-oriented experiments,
 *    and embarcation on new stuff
 *
 * Revision 3.64  1998/08/05 19:16:11  skadron
 * Bug fixes in the BQ implementation:
 * 1.  recover might occur on a misfetch, not just a mispred
 * 2.  bq_add needs to use the most recent history, not the PHT contents.
 *
 * Revision 3.60  1998/06/27 21:58:05  skadron
 * Removed old local-history spec-update scheme (with spec_regs, etc),
 *    and replaced it with a BQ that acts as a future file
 *
 * Revision 3.59  1998/06/11 04:39:27  skadron
 * Added "ordered" double-buffering, to prevent out-or-order branch
 *    resolution's swapping twice and nuking "good" state
 *
 * Revision 3.58  1998/06/11 03:59:47  skadron
 * Added double-buffering for local-hist spec-update
 *
 * Revision 3.55  1998/06/02 04:17:29  skadron
 * 1. Bug in hybrid predictors: after moving all history updates into
 *    bpred_history_update, a component had its history updated twice.
 *    Fixed.
 * 2. Warmup wasn't doing spec-update.
 *
 * Revision 3.45  1998/05/29 04:56:14  skadron
 * Bug fix to previous
 *
 * Revision 3.44  1998/05/29 04:37:53  skadron
 * 1.  Perfect committing of local-history spec-state: mispred
 *     handling scans for branches that will commit and commits their
 *     state before nuking.
 * 2.  Made nuking spec state even on misfetches (and not just "true"
 *     mispredictions) an option
 *
 * Revision 3.42  1998/05/28 02:09:02  skadron
 * Added "don't commit" for noclobber: if a branch forces a spec bit over
 *    to the committed local history; it doesn't later commit another bit
 *
 * Revision 3.40  1998/05/27 17:33:02  skadron
 * Added perfect local-history patching; not yet fully debugged
 *
 * Revision 3.39  1998/05/26 19:01:53  skadron
 * Speculative local-history registers only get nuked by "true"
 *    mispredictions
 *
 * Revision 3.36  1998/05/25 18:35:50  skadron
 * Added noclobber of speculative local-history bits
 *
 * Revision 3.35  1998/05/25 18:03:57  skadron
 * Added spec-update and spec-update-repair for local-history
 *
 * Revision 3.32  1998/05/23 23:47:04  skadron
 * Cosmetic changes
 *
 * Revision 3.31  1998/05/23 22:24:44  skadron
 * Reverting to version 3.27 (ie, removing the filtering of well-pred branches)
 *
 * Revision 3.27  1998/05/16 21:55:02  skadron
 * Reorganized bpred to use a common bpred-history; and to allow each
 *    component to separately set the spec-update and spec-update-repair
 *    options.  Also removed premix.
 *
 * Revision 3.22  1998/05/11 23:11:53  skadron
 * Removed "if-in-btb"
 *
 * Revision 3.20  1998/05/08 21:03:30  skadron
 * Added gshare_shift
 *
 * Revision 3.19  1998/05/07 19:31:14  skadron
 * 1.  Bug fix: aux_global_shift_reg wasn't getting saved in spec-update
 *     mode
 * 2.  Added merge_hist_shift: same as merge_hist, but with global hist
 *     shifted left if possible
 *
 * Revision 3.18  1998/05/06 22:54:15  skadron
 * Added bpred features: premixing, merge-hist, and cat-hist
 *
 * Revision 3.14  1998/05/05 20:45:38  skadron
 * Fixes to history_recover: 1) we were dropping the result of the
 *    mispredicted branch, and 2) we were returning garbage as the
 *    checkpointed history for jumps
 *
 * Revision 3.13  1998/05/05 00:51:32  skadron
 * Implemented "agree" mode
 *
 * Revision 3.12  1998/05/04 21:12:39  skadron
 * Minor fix to preceeding
 *
 * Revision 3.11  1998/05/04 19:37:28  skadron
 * Added option to speculatively update global-branch-history register
 *
 * Revision 3.2  1998/04/21 19:30:28  skadron
 * Added per-thread TOSP's
 *
 * Revision 3.1  1998/04/17 16:51:35  skadron
 * Versions used thru Mar '98 (retstack sub, ruu_resub, ICS final manuscript)
 *
 * Revision 2.103  1998/03/09 03:42:24  skadron
 * 1. Cleaned up the retstack-patching interface
 * 2. Added another retstack-patching level: checkpointing the entire
 * stack
 *
 * Revision 2.102  1998/03/04 19:39:11  skadron
 * 1.  Cleaned up interface for retstack-patching level
 * 2.  Offered ability to restrict when calls/returns pop/push the
 *     retstack;  this is controlled by the caller via a function
 *     parameter
 * 3.  Offered ability to have returns update the BTB
 *
 * Revision 2.101  1997/12/09 22:20:31  skadron
 * Version Pritpal used for ISCA-25 submission
 *
 * Revision 2.100  1997/12/03 20:05:46  skadron
 * Version Kevin used for ISCA-25 submission
 *
 * Revision 2.7  1997/11/20 01:04:38  skadron
 * Fixed retstack bugs: (1) JALR/31, and (2) correct JR that pops wrong
 *    value shouldn't restore that value
 * Also implemented patch for pop-push corruptions: save TOS value as
 *    well as TOS idx
 *
 * Revision 2.6  1997/10/16 19:29:25  skadron
 * Changes to accommodate bpred interval-miss-rate simulation
 *
 * Revision 2.5  1997/09/12 17:30:33  skadron
 * Lookup now returns history bits to permit Tyson-style conf-pred
 *    -- not fully implemented; only properly handles 8-bit 2lev
 *
 * Revision 2.4  1997/09/05 16:38:58  skadron
 * Changed b_update_rec structure to include predPC_if_taken -- BTB result
 *    even if PHT says "not taken"
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
 * Revision 1.15  1997/05/22  20:13:08  skadron
 * Some changes to integrate Tyson's predictor better with my simulator
 *
 * Revision 1.14  1997/05/22 19:53:02  skadron
 * From Gary Tyson:
 *    Added branch backwards and perfect predictors
 *    Added two parameters to bpred_lookup (cpred gives correct next
 *    addr and spec_mode says whether it is down the wrong path)
 *    Made shiftregs a structure to keep track of internal confidence
 *    stats
 *
 * Revision 1.13  1997/05/09 15:10:26  skadron
 * Finished adding gshare (note gshare disabled for local prediction);
 *    modified hybrid table lookup to use hashing like 2bit uses;
 *    allowed up-down counters to be more than 2-bit
 *
 * Revision 1.12  1997/05/08 20:25:11  skadron
 * Added some stats for cond-branches only and to see how often hybrid
 *    selects one predictor over the other; also cleaned up the code a
 *    little; also started adding gshare
 *
 * Revision 1.11  1997/05/06 20:00:15  skadron
 * Fixed typo
 *
 * Revision 1.10  1997/05/06 19:56:52  skadron
 * Cleaned up stats -- updates is now a counter; no more retstack counts
 *
 * Revision 1.9  1997/05/06 18:50:09  skadron
 * Added McFarling-style hybrid prediction
 *
 * Revision 1.8  1997/05/01 20:23:06  skadron
 * BTB bug fixes; jumps no longer update direction state; non-taken
 *    branches non longer update BTB
 *
 * Revision 1.7  1997/05/01 00:05:51  skadron
 * Separated BTB from direction-predictor
 *
 * Revision 1.6  1997/04/29  23:50:44  skadron
 * Added r31 info to distinguish between return-JRs and other JRs for bpred
 *
 * Revision 1.5  1997/04/29  22:53:10  skadron
 * Hopefully bpred is now right: bpred now allocates entries only for
 *    branches; on a BTB miss it still returns a direction; and it uses a
 *    return-address stack.  Returns are not yet distinguished among JR's
 *
 * Revision 1.4  1997/04/28  17:37:09  skadron
 * Bpred now allocates entries for any instruction instead of only
 *    branches; also added return-address stack
 *
 * Revision 1.3  1997/04/24  16:57:27  skadron
 * Bpred used to return no prediction if the indexing branch didn't match
 *    in the BTB.  Now it can predict a direction even on a BTB address
 *    conflict
 *
 * Revision 1.2  1997/03/25 16:17:21  skadron
 * Added function called after priming
 *
 * Revision 1.1  1997/02/16  22:23:54  skadron
 * Initial revision
 *
 *
 */

#ifndef BPRED_H
#define BPRED_H

#include <stdio.h>
#include "misc.h"
#include "ss.h"
#include "stats.h"
#include "sim.h"

#define BQ_JUNK_VAL (SS_ADDR_TYPE)3

/* Available types of ret-stack patching (repairing the ret-stack after
 * mis-speculation */
#define RETSTACK_PATCH_WHOLE	3
#define RETSTACK_PATCH_PTR_DATA 2
#define RETSTACK_PATCH_PTR_ONLY 1
#define RETSTACK_PATCH_NONE     0

/* Available types of branch-history patching (repairing branch history
 * after mis-speculation) */
#define HISTORY_PATCH_PERFECT		3	/* OBSOLETE! */
#define HISTORY_PATCH_AUX_GLOBAL_ONLY 	2
#define HISTORY_PATCH_ALL 		1
#define HISTORY_PATCH_NONE 		0

/*
 * This module implements a number of branch predictor mechanisms.  The
 * following predictors are supported:
 *
 *	BPredHybrid: McFarling-style hybrid predictor
 *
 *		It simulates two branch predictors, and uses a 2-bit
 *		table to choose between them.
 *		Simply specify two predictor-types and configurations:
 *		Both can be two-level schemes, or one can be bimodal.
 *		(2 bimodal schemes probably isn't very helpful)
 *
 *	BPred2Level:  two level adaptive branch predictor
 *
 *		It can simulate many prediction mechanisms that have up to
 *		two levels of tables. Parameters are:
 *		     N   # entries in first level (# of shift register(s))
 *		     W   width of shift register(s)
 *		     M   # entries in 2nd level (# of counters, or other FSM)
 *		One BTB entry per level-2 counter.
 *
 *		Configurations:   N, W, M
 *
 *		    counter based: 1, 0, M
 *
 *		    GAg          : 1, W, 2^W
 *		    GAp          : 1, W, M (M > 2^W)
 *		    PAg          : N, W, 2^W
 *		    PAp          : N, W, M (M == 2^(N+W))
 *
 *	BPred2bit:  a simple direct mapped bimodal predictor
 *
 *		This predictor has a table of two bit saturating counters.
 *		Where counter states 0 & 1 are predict not taken and
 *		counter states 2 & 3 are predict taken, the per-branch counters
 *		are incremented on taken branches and decremented on
 *		no taken branches.  One BTB entry per counter.
 *
 *	BPredTaken:  static predict branch taken
 *
 *	BPredNotTaken:  static predict branch not taken
 *
 *	BPredBackwards:  static predict backwards branch taken
 *
 *	BPredPerfect:  perfect branch prediction (simply returns 'cpred')
 *
 */

/* branch predictor types */
enum bpred_class {
  BPredHybrid,			/* McFarling-style hybrid-predictor */
  BPred2Level,			/* 2-level correlating pred w/2-bit counters */
  BPred2bit,			/* 2-bit saturating cntr pred (dir mapped) */
  BPredTaken,			/* static predict taken */
  BPredNotTaken,		/* static predict not taken */
  BPredBackwards,		/* static predict backwards taken */
  BPredPerfect,			/* perfect prediction */
  BPred_NUM
};

/* an entry in a BTB */
struct bpred_btb_ent {
  SS_ADDR_TYPE addr;		/* address of branch being tracked */
  enum ss_opcode op;		/* opcode of branch corresp. to addr */
  SS_ADDR_TYPE target;		/* last destination of branch when taken */
  struct bpred_btb_ent *prev, *next; /* lru chaining pointers */
};

struct bpred_tab1_ent {
  SS_ADDR_TYPE addr;		/* address of branch being tracked */
  int refs;			/* number of refs for this history reg */
  int hits;			/* how often the ref gave a correct pred */
  unsigned int history;		/* history bits */
};

struct bq_ent {
  SS_ADDR_TYPE addr;		/* address of branch being tracked */
  unsigned int history;		/* history being saved */
  SS_COUNTER_TYPE repair_id;	/* tag for debugging purposes */
  /* other stuff will be added later */
};

/* branch predictor def */
struct bpred {
  enum bpred_class class;	/* type of predictor */
  /* Basic flags controlling the manipulation of the PHT */
  int gshare;			/* when combining address bits with history
				 * bits, use xor instead of concatenation? */
  int agree;			/* when reading 2-bit counters, TRUE means
				 * read 00 or 01 as agreeing with "backwards"
				 * static predictor; FALSE means traditional
				 * not-taken or taken meaning */
  int spec_update;		/* update history speculatively */
  int spec_update_repair;	/* what type of repair to perform on the
				 * history after detecting a misprediction */
  int spec_update_repair_all;	/* do repair for non-writeback mispreds? */
  /* Specialized bit-mixing techniques */
  int merge_hist;		/* hash local and global history together;
				 * also hash baddress if gshare is set */
  int merge_hist_shift;		/* hash local and global history together,
				 * but first shift global history as far left
				 * as possible without losing bits;
				 * also hash baddress if gshare is set */
  int cat_hist;			/* concatenate global and local hist;
				 * also hash baddress if gshare is set */
  int gshare_shift;		/* like merge_hist_shift, but shifting
				 * the baddr left and partially overlapping it
				 * with the history bits */
  int gshare_drop_lsbits; 	/* how many low-order bits to drop from the
				 * baddr; note that the 3 lowest-order bits are
				 * meaningless and always dropped */
  /* General state and operation info */
  unsigned char cntr_max;	/* max value for an n-bit up-down counter */
  unsigned char cntr_threshold; /* threshold value for an n-bit up-down
				 * counter, after which it's meaning changes */
  int aux_global_size;		/* size of aux global history register */
  int aux_global_shift_reg;     /* actual global shift reg */

  union {
    struct {
      unsigned int size;	/* number of entries in direct-mapped table */
      unsigned char *table;	/* prediction state table */
    } bimod;
    struct {
      int l1size;		/* level-1 size, number of history regs */
      int l2size;		/* level-2 size, number of pred states */
      int shift_width;		/* amount of history in level-1 shift regs */
      struct bpred_tab1_ent *shiftregs;	/* level-1 history table */
      unsigned char *l2table;	/* level-2 prediction state table */
    } two;
    struct {
      struct bpred *pred1;	/* pointer to predictor #1 */
      struct bpred *pred2;	/* pointer to predictor #2 */
      int shift_width;          /* amount of history in level-1 shift regs */
      int shift_reg;		/* global history register */
      unsigned long size;	/* size of predictor-predictor table */
      int *global_save_p;	/* pointer to ghist to save (spec_update) */
      unsigned char *table;	/* predictor-predictor state table */
    } hybrid;
  } dirpred;

  struct {
    int sets;			/* num BTB sets */
    int assoc;			/* BTB associativity */
    struct bpred_btb_ent *btb_data; /* BTB addr-prediction table */
  } btb;
  
  struct {
    int size;			/* return-address stack size */
    int patch_level;		/* type of retstack patching to use */
    int update_btb;		/* if non-zero stack, do returns update BTB? */
    int caller_supplies_tos;    /* must caller supply the TOS? */
    int tos;			/* top-of-stack */
    struct bpred_btb_ent *stack; /* return-address stack */
  } retstack;

  int use_bq;			/* whether to use the BQ */
  struct {			/* queue of outstanding branches */
    int head;
    int tail;
    int num;
    int size;
    struct bq_ent *tbl;
  } bq;

  /* stats; jr and indir counts are mut. exclusive */
  SS_COUNTER_TYPE addr_hits;	/* num correct addr-predictions */
  SS_COUNTER_TYPE dir_hits;	/* num correct dir-predictions (incl addr) */
  SS_COUNTER_TYPE cond_hits;	/* cond branches correctly dir-predicted */
  SS_COUNTER_TYPE cond_seen;	/* cond branches seen */
  SS_COUNTER_TYPE jr_hits;	/* num correct addr-predictions for JR's */
  SS_COUNTER_TYPE jr_seen;	/* num JR's seen */
  SS_COUNTER_TYPE indir_hits;	/* num correct addr-predictions for INDIR's */
  SS_COUNTER_TYPE indir_seen;	/* num INDIR's seen */
  SS_COUNTER_TYPE misses;	/* num incorrect predictions */

  SS_COUNTER_TYPE lookups;	/* num lookups */
#ifdef FSIM_IN_FETCH
  SS_COUNTER_TYPE spec_lookups;	/* num wrong path lookups */
#endif
  SS_COUNTER_TYPE updates;	/* num updates */
  SS_COUNTER_TYPE used_pred1;	/* number of times hybrid pred used pred #1 
				 * for a cond branch */
  SS_COUNTER_TYPE bq_overflows; /* num times bq overwrote an entry because it
				 * was full */
#ifdef RETSTACK_COUNTS
  SS_COUNTER_TYPE retstack_pops;   /* number of times a value was popped */
  SS_COUNTER_TYPE retstack_pushes; /* number of times a value was pushed */
#endif
  SS_COUNTER_TYPE int_addr_hits; /* a-hits seen through end of last interval */
  SS_COUNTER_TYPE int_cond_hits; /* hits seen through end of last interval */
  SS_COUNTER_TYPE int_cond_seen; /* cond's seen through end of last interval */
  SS_COUNTER_TYPE int_indir_hits; /* hits seen through end of last interval */
  SS_COUNTER_TYPE int_indir_seen; /* indir's through end of last interval */
  SS_COUNTER_TYPE int_lookups;   /* lookups through end of last interval */
};

struct bpred_update_info {
  unsigned char *dir_update_ptr1;
  unsigned char *dir_update_ptr2;
  unsigned char dir1;
  unsigned char dir2;
  unsigned char *pred_pred_ptr;
  unsigned char which;
  SS_ADDR_TYPE predPC_if_taken;
  unsigned int history;
  unsigned int bits;
  int num_bits;
  int protect_table;
  int dont_commit;
};

struct bpred_recover_info {
  /* speculative history-update: non-speculative history bits */
  int global_history;
  
  /* BQ index; -1 if not used */
  int bq_idx;

  /* repair tag, for debugging purposes */
  SS_COUNTER_TYPE repair_id;

  /* retstack stuff */
  int tos;				/* Non-speculative top-of-stack info */
  union {
    SS_ADDR_TYPE tos_value;		/* TOS contents */
    struct bpred_btb_ent *stack_copy;	/* pointer to structure containing 
					 * entire stack contents */
  } contents;
};


/* create a branch predictor */
struct bpred *				/* branch predictory instance */
bpred_create(enum bpred_class class,	/* type of predictor to create */
	     unsigned long l1size,	/* level-1 table size */
	     unsigned long l2size,	/* level-2 table size--if relevant */
	     unsigned long shift_width,	/* history register width */
	     int gshare,		/* combine hist/addr w/ xor? */
	     int agree,			/* use agree mode with 2-bit cntrs? */
	     int spec_update,		/* do speculative history updates? */
	     int spec_update_repair,	/* type of repair for spec-updates */
	     unsigned int counter_bits, /* number of bits in up-down counters*/
	     unsigned long btb_sets,	/* number of sets in BTB */ 
	     unsigned long btb_assoc,	/* BTB associativity */
	     unsigned long retstack_size);/* num entries in ret-addr stack */

struct bpred *
bpred_hybrid_create(unsigned long size, 	/* predictor-predictor tbl sz*/
		    struct bpred *pred1,	/* pointer to predictor 1 */
		    struct bpred *pred2,	/* pointer to predictor 2 */
		    unsigned long shift_width,	/* history register width */
		    int gshare,			/* combine hist/addr w/ xor? */
		    int spec_update,		/* do spec history updates? */
		    int spec_update_repair,	/* repair for spec-updates */
		    unsigned int counter_bits,	/* # of bits in up-dn cntrs */
		    unsigned long btb_sets,	/* number of sets in BTB */ 
		    unsigned long btb_assoc,	/* BTB associativity */
		    unsigned long retstack_size);/* num entries in retstack */

/* auxilliary configuration options; "normal" runs won't use these at all */
void 
bpred_create_aux(struct bpred *pred,           /* branch predictor instance */
		 int retstack_patch_level,     /* 0:none, 1:ptr, 2:ptr-data */
		 int retstack_update_btb,      /* do returns update BTB? */
		 int retstack_caller_supplies_tos, /*must caller supply TOS?*/
		 int merge_hist,		/* merge global & local hist */
		 int merge_hist_shift,		/* same, but offset */
		 int cat_hist,			/* concat global/local hist */
		 int gshare_shift,		/* gshare, but shifted addr */
		 int gshare_drop_lsbits);	/* drop low-order baddr bits */

/* print branch predictor configuration */
void
bpred_config(struct bpred *pred,	/* branch predictor instance */
	     FILE *stream);		/* output stream */

/* print predictor stats */
void
bpred_stats(struct bpred *pred,		/* branch predictor instance */
	    FILE *stream);		/* output stream */

/* register branch predictor stats */
void
bpred_reg_stats(struct bpred *pred,	/* branch predictor instance */
		struct stat_sdb_t *sdb);/* stats database */

/* reset stats after priming, if appropriate */
void 
bpred_after_priming(struct bpred *bpred);

/* reflect a new interval for interval-miss-rate observations */
void 
bpred_new_interval(struct bpred *bpred);

/* 
 * Retstack gating: whether retstack may be touched on this call.
 * Determined by caller
 */

/* probe a predictor for a next fetch address, the predictor is probed
   with branch address BADDR, the branch target is BTARGET (used for
   static predictors), and OP is the instruction opcode (used to simulate
   predecode bits; a pointer to the predictor state entry (or null for jumps)
   is returned in *DIR_UPDATE_PTR (used for updating predictor state),
   and the non-speculative top-of-stack is returned in stack_recover_idx 
   (used for recovering ret-addr stack after mis-predict).  */
SS_ADDR_TYPE				/* predicted branch target addr */
bpred_lookup(struct bpred *pred,	/* branch predictor instance */
	    SS_ADDR_TYPE baddr,	/* branch address */
	    SS_ADDR_TYPE btarget,	/* branch target if taken */
	    SS_ADDR_TYPE cpred,	/* correct br targ-addr f/ perf pred */
	    int spec_mode,		/* is this a wrong path lookup? */
	    enum ss_opcode op,		/* opcode of instruction */
	    int jr_r31p,		/* is this a JR using r31? */
	    int jalr_r31p,		/* is this a JALR using r31? */
	    int *p_caller_tos,		/* retstack TOS supplied by caller */
	    int retstack_gate,		/* True = ok to use retstack */
	    struct bpred_update_info *b_update_rec, /* ptr to info f/ update*/
	    struct bpred_recover_info *recover_rec); /* retstack/hist recovery
						      * info */

/* Speculative execution can corrupt the ret-addr stack.  So for each
 * lookup we return the top-of-stack (TOS) at that point; a mispredicted
 * branch, as part of its recovery, restores the TOS using this value --
 * hopefully this uncorrupts the stack. */
void
bpred_retstack_recover(struct bpred *pred,	/* branch predictor instance */
	      SS_ADDR_TYPE baddr,	/* branch address */
	      int *p_caller_tos,	/* retstack TOS supplied to caller */
	      int retstack_gate,	/* True = ok to use retstack */
	      struct bpred_recover_info *recover_rec); /* retstack recovery 
							* info */

/* If doing speculative history updates, history info can get corrupted.
 * So each branch saves some history info, and a mispredicted branch,
 * as part of its recovery, restores the history state 
 * FIXME: Currently only works with global history */
void
bpred_history_recover(struct bpred *pred,	/* branch predictor instance */
		      SS_ADDR_TYPE baddr,	/* branch address */
		      int taken,		/* non-zero if branch taken */
		      enum ss_opcode op,	/* opcode of instruction */
		      enum pipestage stage,	/* caller's pipestage */
		      struct bpred_recover_info *recover_rec); /* recov info */

/* Speculatively update the branch predictor's history bits, but not
 * the direction-prediction bits.  This updates the entry for instruction
 * type OP at address BADDR, with taken/not-taken info in PRED_TAKEN */
void
bpred_history_update(struct bpred *pred,/* branch predictor instance */
		     SS_ADDR_TYPE baddr,/* branch address */
		     int taken,		/* non-zero if branch was pred taken */
		     enum ss_opcode op, /* opcode of instruction */
		     int speculative,	/* is this a speculative update? */
		     struct bpred_update_info *b_update_rec,/* misc br state */
		     struct bpred_recover_info *recover_rec); /* recov. info */

/* update the branch predictor, only useful for stateful predictors; updates
   entry for instruction type OP at address BADDR.  BTB only gets updated
   for branches which are taken.  Inst was determined to jump to
   address BTARGET and was taken if TAKEN is non-zero.  Predictor 
   statistics are updated with result of prediction, indicated by CORRECT and 
   PRED_TAKEN, predictor state for update is contained in B_UPDATE_REC. Note
   if bpred_update is done speculatively, branch-prediction may get polluted.*/
void
bpred_update(struct bpred *pred,	/* branch predictor instance */
	     SS_ADDR_TYPE baddr,	/* branch address */
	     SS_ADDR_TYPE btarget,	/* resolved branch target */
	     int offset,		/* branch offset */
	     int taken,			/* non-zero if branch was taken */
	     int pred_taken,		/* non-zero if branch was pred taken */
	     int correct,		/* was earlier prediction correct? */
	     enum ss_opcode op,		/* opcode of instruction */
	     int jr_r31p,		/* is this using r31? */
	     int retstack_gate,		/* True = ok to use retstack */
	     int hybrid_component,	/* Is this a hybrid component? */
	     struct bpred_update_info b_update_rec, /* pred info f/ update */
	     struct bpred_recover_info *recover_rec);/* retstack/hist recovery
						      * info */

/* convert a string supplied by user on command line into an enum bpred_class 
*/
enum bpred_class
bpred_str2class(char *str);

#endif /* BPRED_H */

