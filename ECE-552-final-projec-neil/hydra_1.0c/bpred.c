/*
 * bpred.c - branch predictor routines
 *
 * This file is based on the SimpleScalar (see below) distribution of
 * bpred.c, but has been extensively modified by Kevin Skadron
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
 * $Id: bpred.c,v 3.100 1998/08/06 21:23:13 skadron Exp $
 *
 * $Log: bpred.c,v $
 * Revision 3.100  1998/08/06 21:23:13  skadron
 * NOTE: This rev marks the end of the '98 Micro/HPCA-oriented experiments,
 *    and embarcation on new stuff
 *
 * Revision 3.65  1998/08/05 20:01:05  skadron
 * Fixed another BQ bug: lookup was stopping at first match instead of
 *    searching the whole queue
 *
 * Revision 3.64  1998/08/05 19:15:57  skadron
 * Bug fixes in the BQ implementation:
 * 1.  recover might occur on a misfetch, not just a mispred
 * 2.  bq_add needs to use the most recent history, not the PHT contents
 *
 * Revision 3.63  1998/08/05 16:37:07  skadron
 * Added the 3.55.1 BPredBackwards fix
 *
 * Revision 3.62  1998/08/04 19:56:15  skadron
 * Bug fix in preceding: 'history', near beginning of lookup, was not
 *    properly computed if not using BQ
 *
 * Revision 3.61  1998/07/02 03:20:21  skadron
 * Further bug fixes
 *
 * Revision 3.60  1998/06/27 21:58:11  skadron
 * Removed old local-history spec-update scheme (with spec_regs, etc),
 *    and replaced it with a BQ that acts as a future file
 *
 * Revision 3.59  1998/06/11 04:39:31  skadron
 * Added "ordered" double-buffering, to prevent out-or-order branch
 *    resolution's swapping twice and nuking "good" state
 *
 * Revision 3.58  1998/06/11 03:59:27  skadron
 * Added double-buffering for local-hist spec-update
 *
 * Revision 3.57  1998/06/10 22:21:37  skadron
 * Minor fix to ensure that dont_commit can't accidentally be true
 *
 * Revision 3.56  1998/06/10 02:35:40  skadron
 * Fixed perfect-fixup (both local-hist and retstack) to not do
 *    calloc/memcpy during warmup
 *
 * Revision 3.55  1998/06/02 04:17:00  skadron
 * 1. Bug in hybrid predictors: after moving all history updates into
 *    bpred_history_update, a component had its history updated twice.
 *    Fixed.
 * 2. Warmup wasn't doing spec-update.
 *
 * Revision 3.54  1998/06/02 03:45:00  skadron
 * Reorganized bpred_history_update to cleanly handle the variety of
 *    possible update and repair schemes, as oppposed to the mess of
 *    'doit' conditions
 *
 * Revision 3.53  1998/06/01 02:37:00  skadron
 * Hack to permit global-local bit mixing for hybrid configs: needs
 *    to be improved
 *
 * Revision 3.52  1998/06/01 01:25:08  skadron
 * Bug fix: When adding spec regs, forgot that spec_regs is in a union,
 *    and might be non-zero even if "spec_regs" hasn't been allocated
 *
 * Revision 3.51  1998/06/01 00:47:45  skadron
 * Bug fix: merge_hist, merge_hist_shift, and cat_hist were always doing
 *    an OR of the mixed history with the gselected baddr, which makes no
 *    sense
 *
 * Revision 3.46  1998/05/29 17:20:39  skadron
 * Fixed bug: retstack-patch-whole didn't work properly with
 *    perfect-update due to early freeing of retstack copy
 *
 * Revision 3.45  1998/05/29 04:56:08  skadron
 * Bug fix to previous
 *
 * Revision 3.44  1998/05/29 04:37:06  skadron
 * 1.  Perfect committing of local-history spec-state: mispred
 *     handling scans for branches that will commit and commits their
 *     state before nuking.
 * 2.  Made nuking spec state even on misfetches (and not just "true"
 *     mispredictions) an option
 *
 * Revision 3.43  1998/05/28 02:21:31  skadron
 * Bug fix to previous
 *
 * Revision 3.42  1998/05/28 02:08:41  skadron
 * Added "don't commit" for noclobber: if a branch forces a spec bit over
 *    to the committed local history; it doesn't later commit another bit
 *
 * Revision 3.41  1998/05/27 18:25:15  skadron
 * Fixed 3 bugs:
 * 1.  Committing a bit was in fact appending the whole speculative
 *     history contents to the commited history.
 * 2.  After commit, the speculative register was left shifted: the
 *     committed bit should be zero'd.
 * 3.  Noclobber was committing a bit each and every time, regardless of
 *     whether the spec-reg was in fact full.
 *
 * Revision 3.40  1998/05/27 17:32:32  skadron
 * Added perfect local-history patching; not yet fully debugged
 *
 * Revision 3.39  1998/05/26 19:01:22  skadron
 * Speculative local-history registers only get nuked by "true"
 *    mispredictions
 *
 * Revision 3.38  1998/05/25 20:41:38  skadron
 * Bug fix to previous
 *
 * Revision 3.37  1998/05/25 19:55:30  skadron
 * Bug fix in previous
 *
 * Revision 3.36  1998/05/25 18:35:43  skadron
 * Added noclobber of speculative local-history bits
 *
 * Revision 3.35  1998/05/25 18:02:51  skadron
 * Added spec-update and spec-update-repair for local-history
 *
 * Revision 3.34  1998/05/24 02:31:33  skadron
 * Bug fix to preceding
 *
 * Revision 3.33  1998/05/24 01:49:56  skadron
 * Bug fix in previous
 *
 * Revision 3.32  1998/05/23 23:46:28  skadron
 * Bug fixes in update of local-hist when spec-update is turned on
 *    (aux_global gets updated speculatively; local history doesn't)
 *
 * Revision 3.31  1998/05/23 22:23:27  skadron
 * Reverting to version 3.27 (ie, removing the filtering of well-pred branches)
 *
 * Revision 3.27  1998/05/16 21:54:31  skadron
 * Reorganized bpred to use a common bpred-history; and to allow each
 *    component to separately set the spec-update and spec-update-repair
 *    options.  Also removed premix.
 *
 * Revision 3.26  1998/05/12 19:22:54  skadron
 * Fix to preceding
 *
 * Revision 3.25  1998/05/12 19:18:55  skadron
 * Bug fix in saving aux_global_shift_reg
 *
 * Revision 3.24  1998/05/12 19:15:19  skadron
 * Removed history-update code from bpred_update so all updates now use
 *    bpred_history_update
 *
 * Revision 3.23  1998/05/11 23:12:49  skadron
 * Removed "if-in-btb"
 *
 * Revision 3.21  1998/05/08 21:03:37  skadron
 * Added gshare_shift
 *
 * Revision 3.20  1998/05/07 19:30:31  skadron
 * 1.  Bug fix: aux_global_shift_reg wasn't getting saved in spec-update
 *     mode
 * 2.  Added merge_hist_shift: same as merge_hist, but with global hist
 *     shifted left if possible
 *
 * Revision 3.19  1998/05/07 00:35:04  skadron
 * Removed two debugging asserts that were special-case
 *
 * Revision 3.18  1998/05/06 22:54:15  skadron
 * Added bpred features: premixing, merge-hist, and cat-hist
 *
 * Revision 3.14  1998/05/05 20:45:24  skadron
 * Fixes to history_recover: 1) we were dropping the result of the
 *    mispredicted branch, and 2) we were returning garbage as the
 *    checkpointed history for jumps
 *
 * Revision 3.13  1998/05/05 00:51:25  skadron
 * Implemented "agree" mode
 *
 * Revision 3.12  1998/05/04 21:09:51  skadron
 * Minor fix to preceeding
 *
 * Revision 3.11  1998/05/04 19:37:28  skadron
 * Added option to speculatively update global-branch-history register
 *
 * Revision 3.2  1998/04/21 19:30:37  skadron
 * Added per-thread TOSP's
 *
 * Revision 3.1  1998/04/17 16:51:35  skadron
 * Versions used thru Mar '98 (retstack sub, ruu_resub, ICS final manuscript)
 *
 * Revision 2.106  1998/03/09 17:47:56  skadron
 * Fixed entire-stack-checkpointing memory leak
 *
 * Revision 2.105  1998/03/09 03:41:31  skadron
 * 1. Cleaned up the retstack-patching interface
 * 2. Added another retstack-patching level: checkpointing the entire
 * stack
 *
 * Revision 2.104  1998/03/04 19:38:55  skadron
 * 1.  Cleaned up interface for retstack-patching level
 * 2.  Offered ability to restrict when calls/returns pop/push the
 *     retstack;  this is controlled by the caller via a function
 *     parameter
 * 3.  Offered ability to have returns update the BTB
 *
 * Revision 2.103  1998/02/09  02:25:02  skadron
 * Added choice for retstack patching level
 *
 * Revision 2.100.1.2  1998/02/09 02:23:57  skadron
 * Added choice for retstack patching level
 *
 * Revision 2.100.1.1  1998/01/08 21:36:54  skadron
 * Made retstack size a distinct option; allowed not-taken bpred to have
 *    a retstack and to follow taken branches
 *
 * Revision 2.100  1997/12/03  20:05:46  skadron
 * Version Kevin used for ISCA-25 submission
 *
 * Revision 2.12  1997/11/20 01:12:44  skadron
 * Changed "jr rate" to only report for jr-31's, because "indir rate"
 *    reports for indirect branches as a whole
 *
 * Revision 2.11  1997/11/20 01:04:30  skadron
 * Fixed retstack bugs: (1) JALR/31, and (2) correct JR that pops wrong
 *    value shouldn't restore that value
 * Also implemented patch for pop-push corruptions: save TOS value as
 *    well as TOS idx
 *
 * Revision 2.10  1997/10/27 20:11:39  skadron
 * Bug fix: stack_recover_idx should be updated after pushing onto
 *    ret-addr stack
 *
 * Revision 2.9  1997/10/16 19:29:30  skadron
 * Changes to accommodate bpred interval-miss-rate simulation
 *
 * Revision 2.8  1997/09/17 16:49:21  skadron
 * predPC_if_taken now gets set to 1 for the not-taken branch predictor
 *
 * Revision 2.7  1997/09/12 17:30:33  skadron
 * Lookup now returns history bits to permit Tyson-style conf-pred
 *    -- not fully implemented; only properly handles 8-bit 2lev
 *
 * Revision 2.6  1997/09/09 19:03:53  skadron
 * Returning a BTB result for not-taken has been removed again
 *
 * Revision 2.5  1997/09/05 16:38:31  skadron
 * 1. Changed lookup so that 'taken' and 'nottaken' predictors can still use
 *    a BTB
 * 2. Changed lookup to always return the BTB result, even if PHT says
 *    "not taken"
 *
 * Revision 2.4  1997/08/01 23:23:31  skadron
 * RAS fix from Artur Klaser
 *
 * Revision 2.3  1997/07/11  21:44:19  skadron
 * Updated to incorporate final changes for public 2.0 release
 *
 * Revision 2.2  1997/07/08 03:45:58  skadron
 * Updated to reflect Todd/Milo's version 2.0e; includes memory-leak fix
 *
 * Revision 2.1  1997/07/02 19:15:41  skadron
 * Last version used for Micro-30 submission
 *
 * Revision 1.20  1997/05/22  20:13:01  skadron
 * Some changes to integrate Tyson's predictor better with my simulator
 *
 * Revision 1.19  1997/05/22 19:52:39  skadron
 * From Gary Tyson:
 *    Added branch backwards and perfect predictors
 *    Added two parameters to bpred_lookup (cpred gives correct next
 *    addr and spec_mode says whether it is down the wrong path)
 *    Made shiftregs a structure to keep track of internal confidence
 *    stats
 *
 * Revision 1.18  1997/05/14 02:05:16  skadron
 * Fixed gshare for 2-level prediction
 *
 * Revision 1.17  1997/05/09 15:10:06  skadron
 * Finished adding gshare (note gshare disabled for local prediction);
 *    modified hybrid table lookup to use hashing like 2bit uses;
 *    allowed up-down counters to be more than 2-bit
 *
 * Revision 1.16  1997/05/08 20:24:46  skadron
 * Added some stats for cond-branches only and to see how often hybrid
 *    selects one predictor over the other; also cleaned up the code a
 *    little; also started adding gshare
 *
 * Revision 1.15  1997/05/06 21:52:16  skadron
 * Fixed up BPredTaken and BPredNotTaken to work with hybrid
 *
 * Revision 1.14  1997/05/06 20:00:07  skadron
 * Fixed typo
 *
 * Revision 1.13  1997/05/06 19:56:48  skadron
 * Cleaned up stats -- updates is now a counter; no more retstack counts
 *
 * Revision 1.12  1997/05/06 18:50:01  skadron
 * Added McFarling-style hybrid prediction
 *
 * Revision 1.11  1997/05/01 20:23:00  skadron
 * BTB bug fixes; jumps no longer update direction state; non-taken
 *    branches non longer update BTB
 *
 * Revision 1.10  1997/05/01 00:05:42  skadron
 * Separated BTB from direction-predictor
 *
 * Revision 1.9  1997/04/30  01:42:42  skadron
 * 1. Not aggressively returning the BTB target regardless of hit on jump's,
 *    but instead returning just "taken" when it's a BTB miss yields an
 *    apparent epsilon performance improvement for cc1 and perl.
 * 2. Bug fix: if no retstack, treat return's as any other jump
 *
 * Revision 1.8  1997/04/29  23:50:33  skadron
 * Added r31 info to distinguish between return-JRs and other JRs for bpred
 *
 * Revision 1.7  1997/04/29  22:53:04  skadron
 * Hopefully bpred is now right: bpred now allocates entries only for
 *    branches; on a BTB miss it still returns a direction; and it uses a
 *    return-address stack.  Returns are not yet distinguished among JR's
 *
 * Revision 1.6  1997/04/28  17:37:02  skadron
 * Bpred now allocates entries for any instruction instead of only
 *    branches; also added return-address stack
 *
 * Revision 1.5  1997/04/24  16:57:21  skadron
 * Bpred used to return no prediction if the indexing branch didn't match
 *    in the BTB.  Now it can predict a direction even on a BTB address
 *    conflict
 *
 * Revision 1.4  1997/03/27  16:31:52  skadron
 * Fixed bug: sim-outorder calls bpred_after_priming(), even if no bpred
 *    exists.  Now we check for a null ptr.
 *
 * Revision 1.3  1997/03/25  16:16:33  skadron
 * Statistics now take account of priming: statistics report only
 *    post-prime info.
 *
 * Revision 1.2  1997/02/24  18:02:41  skadron
 * Fixed output format of a formula stat
 *
 * Revision 1.1  1997/02/16  22:23:54  skadron
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <strings.h>
#include <assert.h>

#include "misc.h"
#include "ss.h"
#include "bpred.h"

static unsigned char dummy_taken;
static unsigned char dummy_nottaken;
static unsigned char dummy_backwards;
static unsigned char dummy_perfect;

static SS_COUNTER_TYPE repair_tag = 0;

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
	     unsigned long retstack_size) /* num entries in ret-addr stack */
{
  struct bpred *pred;

  if (!(pred = calloc(1, sizeof(struct bpred))))
    fatal("out of virtual memory");

  pred->class = class;
  pred->gshare = gshare;
  pred->agree = agree;
  pred->spec_update = spec_update ? TRUE : FALSE;
  if (spec_update)
    {
      pred->bq.size = spec_update & 0x7fff;
      pred->spec_update_repair = spec_update_repair & 0xff;
      if (l1size > 1 && pred->spec_update_repair == HISTORY_PATCH_ALL)
	{
	  pred->spec_update_repair_all 
	    = (spec_update_repair & 0x100) ? TRUE : FALSE;
	}
      assert(pred->spec_update_repair != HISTORY_PATCH_PERFECT);
    }
  
  pred->cntr_max = (1 << counter_bits) - 1;
  pred->cntr_threshold = pred->cntr_max / 2;

  switch (class) {
  case BPred2Level:
    {
      if (!l1size || (l1size & (l1size-1)) != 0)
	fatal("level-1 size, `%d', must be non-zero and a power of two", 
	      l1size);
      pred->dirpred.two.l1size = l1size;
      
      if (!l2size || (l2size & (l2size-1)) != 0)
	fatal("level-2 size, `%d', must be non-zero and a power of two", 
	      l2size);
      pred->dirpred.two.l2size = l2size;
      
      if (!shift_width || shift_width > 30)
	fatal("shift register width, `%d', must be non-zero and positive",
	      shift_width);
      pred->dirpred.two.shift_width = shift_width;
      
      pred->dirpred.two.shiftregs = calloc(l1size, 
					   sizeof(struct bpred_tab1_ent));
      if (!pred->dirpred.two.shiftregs)
	fatal("cannot allocate shift register table");

      pred->dirpred.two.l2table = calloc(l2size, sizeof(unsigned char));
      if (!pred->dirpred.two.l2table)
	fatal("cannot allocate second level table");

      break;
    }

  case BPred2bit:
    if (!l1size || (l1size & (l1size-1)) != 0)
      fatal("2bit table size, `%d', must be non-zero and a power of two", 
	    l1size);
    pred->dirpred.bimod.size = l1size;
    if (!(pred->dirpred.bimod.table = calloc(l1size, sizeof(unsigned char))))
      fatal("cannot allocate 2bit storage");
    break;

  case BPredTaken:
    dummy_taken = 0x3;
    break;

  case BPredNotTaken:
    dummy_nottaken = 0;
    break;

  case BPredBackwards:
    dummy_backwards = 0;
    break;

  case BPredPerfect:
    dummy_perfect = 0;
    break;

  default:
    panic("bogus predictor class");
  }

  /* allocate BTB and ret-addr stack */
  switch (class) {
  case BPred2Level:
  case BPred2bit:
  case BPredTaken:
  case BPredNotTaken:
  case BPredBackwards:
    {
      int i;

      /* allocate BTB */
      if ((btb_sets & (btb_sets-1)) != 0)
	fatal("number of BTB sets must be a power of two");
      if ((btb_assoc & (btb_assoc-1)) != 0)
	fatal("BTB associativity must be a power of two");
      if (btb_assoc > 4)
	warn("BTB simulation time may be poor; sets are searched linearly and"
	     "\n   you have declared an associativity of %d", btb_assoc);

      if (btb_sets * btb_assoc > 0)
	if (!(pred->btb.btb_data = calloc(btb_sets * btb_assoc,
					  sizeof(struct bpred_btb_ent))))
	  fatal("cannot allocate BTB");

      pred->btb.sets = btb_sets;
      pred->btb.assoc = btb_assoc;

      if (pred->btb.assoc > 1)
	for (i=0; i < (pred->btb.assoc*pred->btb.sets); i++)
	  {
	    if (i % pred->btb.assoc != pred->btb.assoc - 1)
	      pred->btb.btb_data[i].next = &pred->btb.btb_data[i+1];
	    else
	      pred->btb.btb_data[i].next = NULL;
	    
	    if (i % pred->btb.assoc != pred->btb.assoc - 1)
	      pred->btb.btb_data[i+1].prev = &pred->btb.btb_data[i];
	  }

      /* allocate retstack */
      if ((retstack_size & (retstack_size-1)) != 0)
	fatal("Return-address-stack size must be zero or a power of two");
      
      pred->retstack.size = retstack_size;
      if (retstack_size)
	if (!(pred->retstack.stack = calloc(retstack_size, 
					    sizeof(struct bpred_btb_ent))))
	  fatal("cannot allocate return-address-stack");
      pred->retstack.tos = retstack_size - 1;
      
      pred->retstack.patch_level = RETSTACK_PATCH_PTR_DATA;
      pred->retstack.update_btb = FALSE;
      pred->retstack.caller_supplies_tos = FALSE;

      /* allocate BQ */
      if (pred->bq.size)   /* implies spec_update == TRUE */
	{
	  int i;
	  assert(pred->spec_update);

	  pred->bq.tbl = calloc(pred->bq.size, sizeof(struct bq_ent));
	  if (!pred->bq.tbl)
	    fatal("cannot allocate BQ");

	  for (i = 0; i < pred->bq.size; i++)
	    {
	      pred->bq.tbl[i].addr = BQ_JUNK_VAL;
	      pred->bq.tbl[i].history = BQ_JUNK_VAL;
	    }
	  
	  if (pred->class == BPred2Level && pred->dirpred.two.l1size > 1
	      && spec_update_repair == HISTORY_PATCH_ALL)
	    pred->use_bq = TRUE;
	}
      
      break;
    }

  case BPredPerfect:
    /* no other state */
    break;

  default:
    panic("bogus predictor class");
  }

  return pred;
}

/* create a hybird predictor */
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
		    unsigned long retstack_size)/* num entries in retstack */
{
  struct bpred *pred;
  int i, global_history_size = 0;

  if (!(pred = calloc(1, sizeof(struct bpred))))
    fatal("out of virtual memory");

  pred->class = BPredHybrid;
  pred->gshare = gshare;
  pred->agree = FALSE;		/* not meaningful for selector's 2-bit cntrs */
  pred->spec_update = spec_update;
  if (spec_update)
    pred->spec_update_repair = spec_update_repair;
  else
    pred->spec_update_repair = FALSE;

  pred->cntr_max = (1 << counter_bits) - 1;
  pred->cntr_threshold = pred->cntr_max / 2;

  /* allocate hybrid-predictor info */
  if (pred1 == NULL || pred2 == NULL)
    fatal("for BPredHybrid, predictor components 1 & 2 must already be "
	  "declared");
#ifndef DEBUG2
  if (pred1 == pred2)
    fatal("for BPredHybrid, predictor components 1 & 2 must be different");
#endif
  if (pred1->class == BPredHybrid || pred2->class == BPredHybrid)
    fatal("hybrid predictor components cannot also be hybrid");

  pred->dirpred.hybrid.pred1 = pred1;
  pred->dirpred.hybrid.pred2 = pred2;

  if (!size || (size & (size-1)) != 0)
    fatal("hybrid-bpred table size, `%d', must be non-zero and a power of two",
	  size);
  if ((1 << shift_width) > size)
    fatal("hybrid-pred shift-register width `%d' is too wide; tbl sz is only "
	  "%d", shift_width, size);

  pred->dirpred.hybrid.shift_width = shift_width;
  pred->dirpred.hybrid.size = size;
  if (!(pred->dirpred.hybrid.table = calloc(size, sizeof(unsigned char))))
    fatal("cannot allocate hybrid table storage");
  
  /* allocate BTB */
  if ((btb_sets & (btb_sets-1)) != 0)
    fatal("number of BTB sets must be a power of two");
  if ((btb_assoc & (btb_assoc-1)) != 0)
    fatal("BTB associativity must be a power of two");
  if (btb_assoc > 4)
    warn("BTB simulation time may be poor; sets are searched linearly and"
	 "\n   you have declared an associativity of %d", btb_assoc);
  
  if (btb_sets * btb_assoc > 0)
    if (!(pred->btb.btb_data = calloc(btb_sets * btb_assoc,
				      sizeof(struct bpred_btb_ent))))
      fatal("cannot allocate BTB");
  
  pred->btb.sets = btb_sets;
  pred->btb.assoc = btb_assoc;
  
  if (pred->btb.assoc > 1)
    for (i=0; i < (pred->btb.assoc*pred->btb.sets); i++)
      {
	if (i % pred->btb.assoc != pred->btb.assoc - 1)
	  pred->btb.btb_data[i].next = &pred->btb.btb_data[i+1];
	else
	  pred->btb.btb_data[i].next = NULL;
	
	if (i % pred->btb.assoc != pred->btb.assoc - 1)
	  pred->btb.btb_data[i+1].prev = &pred->btb.btb_data[i];
      }
  
  /* allocate retstack */
  if ((retstack_size & (retstack_size-1)) != 0)
    fatal("Return-address-stack size must be zero or a power of two");
  
  pred->retstack.size = retstack_size;
  if (retstack_size)
    if (!(pred->retstack.stack = calloc(retstack_size, 
					sizeof(struct bpred_btb_ent))))
      fatal("cannot allocate return-address-stack");
  pred->retstack.tos = retstack_size - 1;

  pred->retstack.patch_level = RETSTACK_PATCH_PTR_DATA;
  pred->retstack.update_btb = FALSE;
  pred->retstack.caller_supplies_tos = FALSE;

  /* if doing spec-update, a hybrid predictor has many potential sources 
   * of global history.  Determine here which one to be saving in the
   * recovery record */
  if (pred->dirpred.hybrid.shift_width && pred->spec_update_repair)
    {
      pred->dirpred.hybrid.global_save_p = &pred->dirpred.hybrid.shift_reg;
      global_history_size = pred->dirpred.hybrid.shift_width;
    }
  if (pred->dirpred.hybrid.pred1->class == BPred2Level
      && pred->dirpred.hybrid.pred1->dirpred.two.l1size == 1
      && pred->dirpred.hybrid.pred1->spec_update_repair
      && pred->dirpred.hybrid.pred1->dirpred.two.shift_width 
         > global_history_size)
    {
      pred->dirpred.hybrid.global_save_p
	= &pred->dirpred.hybrid.pred1->dirpred.two.shiftregs[0].history;
      global_history_size 
	= pred->dirpred.hybrid.pred1->dirpred.two.shift_width;
    }
  if (pred->dirpred.hybrid.pred2->class == BPred2Level
      && pred->dirpred.hybrid.pred2->dirpred.two.l1size == 1
      && pred->dirpred.hybrid.pred2->spec_update_repair
      && pred->dirpred.hybrid.pred2->dirpred.two.shift_width 
         > global_history_size)
    {
      pred->dirpred.hybrid.global_save_p
	= &pred->dirpred.hybrid.pred2->dirpred.two.shiftregs[0].history;
      global_history_size 
	= pred->dirpred.hybrid.pred2->dirpred.two.shift_width;
    }
  if (pred->dirpred.hybrid.pred1->aux_global_size > global_history_size
      && pred->dirpred.hybrid.pred1->spec_update_repair)
    {
      pred->dirpred.hybrid.global_save_p
	= &pred->dirpred.hybrid.pred1->aux_global_shift_reg;
      global_history_size = pred->dirpred.hybrid.pred1->aux_global_size;
    }
  if (pred->dirpred.hybrid.pred2->aux_global_size > global_history_size
      && pred->dirpred.hybrid.pred2->spec_update_repair)
    {
      pred->dirpred.hybrid.global_save_p
	= &pred->dirpred.hybrid.pred1->aux_global_shift_reg;
      global_history_size = pred->dirpred.hybrid.pred2->aux_global_size;
    }
  if (global_history_size == 0)
    pred->dirpred.hybrid.global_save_p = NULL;

  return pred;
}

/* auxilliary configuration options; "normal" runs won't use these at all */
/* FIXME: allow these to be specified per-bpred-component */
void 
bpred_create_aux(struct bpred *pred,            /* branch predictor instance */
		 int retstack_patch_level,      /* 0:none, 1:ptr, 2:ptr-data */
		 int retstack_update_btb,       /* do returns update BTB? */
		 int retstack_caller_supplies_tos,/* must caller supply TOS? */
		 int merge_hist,		/* merge global & local hist */
		 int merge_hist_shift,		/* same, but offset */
		 int cat_hist,			/* concat global/local hist */
		 int gshare_shift,		/* gshare, but shifted addr */
		 int gshare_drop_lsbits)	/* drop low-order baddr bits */
{
  pred->retstack.patch_level = retstack_patch_level;
  pred->retstack.update_btb = retstack_update_btb;
  pred->retstack.caller_supplies_tos = retstack_caller_supplies_tos;

  if ((merge_hist || merge_hist_shift || cat_hist)
      && (pred->class == BPredHybrid
	  || pred->class == BPred2bit
	  || pred->class == BPredTaken
	  || pred->class == BPredNotTaken
	  || pred->class == BPredBackwards
	  || pred->class == BPredPerfect
	  || (pred->class == BPred2Level && pred->dirpred.two.l1size == 1)))
    warn("merge/merge_shift/cat-hist option invalid for this type of bpred,"
	 " ignored");

  if ((gshare_shift || gshare_drop_lsbits)
      && (pred->class == BPredHybrid
	  || pred->class == BPred2bit
	  || pred->class == BPredTaken
	  || pred->class == BPredNotTaken
	  || pred->class == BPredBackwards
	  || pred->class == BPredPerfect))
    fatal("gshare_shift/gshare_drop_lsbits options invalid for this type of"
	  " bpred");

  pred->merge_hist = merge_hist;
  pred->merge_hist_shift = merge_hist_shift;
  pred->cat_hist = cat_hist;

  if (merge_hist)
    pred->aux_global_size = merge_hist;
  if (merge_hist_shift)
    pred->aux_global_size = merge_hist_shift;
  if (cat_hist)
    pred->aux_global_size = cat_hist;
  
  if (gshare_shift)
    pred->gshare_shift = gshare_shift;

  pred->gshare_drop_lsbits = gshare_drop_lsbits;
}

/* print branch predictor configuration */
void
bpred_config(struct bpred *pred,	/* branch predictor instance */
	     FILE *stream)		/* output stream */
{
  switch (pred->class) {
  case BPredHybrid:
    fprintf(stream,
	    "pred: hybrid: %ld tbl-sz, direct-mapped; %d-bit hist-register\n",
	    pred->dirpred.hybrid.size, pred->dirpred.hybrid.shift_width);
    bpred_config(pred->dirpred.hybrid.pred1, stream);
    bpred_config(pred->dirpred.hybrid.pred2, stream);
    fprintf(stream, "btb: %d sets x %d associativity", 
	    pred->btb.sets, pred->btb.assoc);
    fprintf(stream, "ret_stack: %d entries", pred->retstack.size);
    break;

  case BPred2Level:
    fprintf(stream,
	    "pred: 2-lvl: %d l1-sz, %d bits/ent, %d l2-sz, direct-mapped\n",
	    pred->dirpred.two.l1size, pred->dirpred.two.shift_width,
	    pred->dirpred.two.l2size);
    fprintf(stream, "btb: %d sets x %d associativity", 
	    pred->btb.sets, pred->btb.assoc);
    fprintf(stream, "ret_stack: %d entries", pred->retstack.size);
    break;

  case BPred2bit:
    fprintf(stream, "pred: 2-bit: %d entries, direct-mapped\n",
	    pred->dirpred.bimod.size);
    fprintf(stream, "btb: %d sets x %d associativity", 
	    pred->btb.sets, pred->btb.assoc);
    fprintf(stream, "ret_stack: %d entries", pred->retstack.size);
    break;

  case BPredTaken:
    fprintf(stream, "pred: predict taken\n");
    break;

  case BPredNotTaken:
    fprintf(stream, "pred: predict not taken\n");
    break;

  case BPredBackwards:
    fprintf(stream, "pred: predict backwards taken\n");
    break;

  case BPredPerfect:
    fprintf(stream, "pred: perfect prediction\n");
    break;

  default:
    panic("bogus branch predictor class");
  }
}

/* print predictor stats */
void
bpred_stats(struct bpred *pred,		/* branch predictor instance */
	    FILE *stream)		/* output stream */
{
  fprintf(stream, "pred: addr-prediction rate = %f\n",
	  (double)pred->addr_hits/(double)(pred->addr_hits+pred->misses));
  fprintf(stream, "pred: dir-prediction rate = %f\n",
	  (double)pred->dir_hits/(double)(pred->dir_hits+pred->misses));
}

/* register branch predictor stats */
void
bpred_reg_stats(struct bpred *pred,	/* branch predictor instance */
		struct stat_sdb_t *sdb)	/* stats database */
{
  char buf[512], buf1[512], *name;

  /* get a name for this predictor */
  switch (pred->class)
    {
    case BPredHybrid:
      name = "bpred_hybrid";
      break;
    case BPred2Level:
      name = "bpred_2lev";
      break;
    case BPred2bit:
      name = "bpred_bimod";
      break;
    case BPredTaken:
      name = "bpred_taken";
      break;
    case BPredNotTaken:
      name = "bpred_nottaken";
      break;
    case BPredBackwards:
      name = "bpred_backwards";
      break;
    case BPredPerfect:
      name = "bpred_perfect";
      break;
    default:
      panic("bogus branch predictor class");
    }

  sprintf(buf, "%s.lookups.PP", name);
  stat_reg_counter(sdb, buf, "number of bpred lookups",
		 &pred->lookups, 0, NULL);
#ifdef FSIM_IN_FETCH
  sprintf(buf, "%s.spec_lookups.PP", name);
  stat_reg_counter(sdb, buf, "number of bpred speculate (wrong path) lookups",
		 &pred->spec_lookups, 0, NULL);
#endif
  sprintf(buf, "%s.updates.PP", name);
  stat_reg_counter(sdb, buf, "number of bpred updates", 
		 &pred->updates, 0, NULL);
  sprintf(buf, "%s.addr_hits.PP", name);
  stat_reg_counter(sdb, buf, "number of address-predicted hits", 
		 &pred->addr_hits, 0, NULL);
  sprintf(buf, "%s.dir_hits.PP", name);
  stat_reg_counter(sdb, buf, 
		 "number of direction-predicted hits (incl. addr-hits)", 
		 &pred->dir_hits, 0, NULL);
  sprintf(buf, "%s.misses.PP", name);
  stat_reg_counter(sdb, buf, "number of misses", &pred->misses, 0, NULL);
  sprintf(buf, "%s.cond_hits.PP", name);
  stat_reg_counter(sdb, buf,
		 "number of direction-predicted hits for cond branches",
		 &pred->cond_hits, 0, NULL);
  sprintf(buf, "%s.cond_seen.PP", name);
  stat_reg_counter(sdb, buf,
		 "number of cond branches seen",
		 &pred->cond_seen, 0, NULL);
  sprintf(buf, "%s.jr_hits.PP", name);
  stat_reg_counter(sdb, buf,
		 "number of address-predicted hits for JR-31's",
		 &pred->jr_hits, 0, NULL);
  sprintf(buf, "%s.jr_seen.PP", name);
  stat_reg_counter(sdb, buf,
		 "number of JR-31's seen",
		 &pred->jr_seen, 0, NULL);
  sprintf(buf, "%s.indir_hits.PP", name);
  stat_reg_counter(sdb, buf,
		 "number of address-predicted hits for indir jumps",
		 &pred->indir_hits, 0, NULL);
  sprintf(buf, "%s.indir_seen.PP", name);
  stat_reg_counter(sdb, buf,
		 "number of indir jumps seen",
		 &pred->indir_seen, 0, NULL);
  sprintf(buf, "%s.bpred_addr_rate.PP", name);
  sprintf(buf1, "%s.addr_hits.PP / %s.updates.PP", name, name);
  stat_reg_formula(sdb, buf,
		   "branch address-prediction rate (i.e., addr-hits/updates)",
		   buf1, "%9.4f");
  sprintf(buf, "%s.bpred_dir_rate.PP", name);
  sprintf(buf1, "%s.dir_hits.PP / %s.updates.PP", name, name);
  stat_reg_formula(sdb, buf,
		  "branch direction-prediction rate (i.e., all-hits/updates)",
		  buf1, "%9.4f");
  sprintf(buf, "%s.bpred_cond_rate.PP", name);
  sprintf(buf1, "%s.cond_hits.PP / %s.cond_seen.PP", name, name);
  stat_reg_formula(sdb, buf,
		  "cond branch direction-prediction rate",
		  buf1, "%9.4f");
  sprintf(buf, "%s.bpred_jr_rate.PP", name);
  sprintf(buf1, "%s.jr_hits.PP / %s.jr_seen.PP", name, name);
  stat_reg_formula(sdb, buf,
		 "JR-31 address-prediction rate (i.e., JR addr-hits/JRs seen)",
		 buf1, "%9.4f");
  sprintf(buf, "%s.bpred_indir_rate.PP", name);
  sprintf(buf1, "%s.indir_hits.PP / %s.indir_seen.PP", name, name);
  stat_reg_formula(sdb, buf,
		  "indir-jump address-prediction rate (i.e., indir addr-hits/indir's seen)",
		  buf1, "%9.4f");
  sprintf(buf, "%s.hybrid_used_pred1.PP", name);
  stat_reg_counter(sdb, buf,
		 "number of times hybrid pred. used pred #1",
		 &pred->used_pred1, 0, NULL);
  sprintf(buf, "%s.hybrid_used_pred1_rate.PP", name);
  sprintf(buf1, "%s.hybrid_used_pred1.PP / %s.cond_seen.PP", name, name);
  stat_reg_formula(sdb, buf,
		   "percentage of cond branches that used pred #1",
		   buf1, "%9.4f");
  sprintf(buf, "%s.bq_overflows.PP", name);
  stat_reg_counter(sdb, buf,
		 "number of times BQ overwrote an entry because it was full",
		 &pred->bq_overflows, 0, NULL);
  sprintf(buf, "%s.bq_overflow_rate.PP", name);
  sprintf(buf1, "%s.bq_overflows.PP / %s.cond_seen.PP", name, name);
  stat_reg_formula(sdb, buf,
		   "percentage of cond branches that overflowed a spec reg",
		   buf1, "%9.4f");
#ifdef RETSTACK_COUNTS
  sprintf(buf, "%s.retstack_pushes.PP", name);
  stat_reg_counter(sdb, buf,
		 "total number of address pushed onto ret-addr stack",
		 &pred->retstack_pushes, 0, NULL);
  sprintf(buf, "%s.retstack_pops.PP", name);
  stat_reg_counter(sdb, buf,
		 "total number of address popped off of ret-addr stack",
		 &pred->retstack_pops, 0, NULL);
#endif
}

void 
bpred_after_priming(struct bpred *bpred)
{
  if (bpred == NULL)
    return;

  bpred->lookups = 0;
#ifdef FSIM_IN_FETCH
  bpred->spec_lookups = 0;
#endif
  bpred->updates = 0;
  bpred->addr_hits = 0;
  bpred->dir_hits = 0;
  bpred->cond_hits = 0;
  bpred->cond_seen = 0;
  bpred->jr_hits = 0;
  bpred->jr_seen = 0;
  bpred->misses = 0;
  bpred->used_pred1 = 0;
  bpred->bq_overflows = 0;
#ifdef RETSTACK_COUNTS
  bpred->retstack_pops = 0;
  bpred->retstack_pushes = 0;
#endif
}

/* Update interval stats */
void 
bpred_new_interval(struct bpred *bpred)
{
  bpred->int_cond_hits = bpred->cond_hits;
  bpred->int_cond_seen = bpred->cond_seen;
  bpred->int_addr_hits = bpred->addr_hits;
  bpred->int_lookups = bpred->lookups;
  bpred->int_indir_hits = bpred->indir_hits;
  bpred->int_indir_seen = bpred->indir_seen;
}

#define TWOBIT_HASH(PRED, ADDR)						\
  ((((ADDR) >> 19) ^ ((ADDR) >> 3)) & ((PRED)->dirpred.bimod.size-1))
    /* was: ((baddr >> 16) ^ baddr) & (pred->dirpred.bimod.size-1) */
#define HYBRID_HASH(PRED, ADDR)						\
  ((((ADDR) >> 19) ^ ((ADDR) >> 3)) & ((PRED)->dirpred.hybrid.size-1))

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
	     int spec_mode,		/* is this mis-speculated? */
	     enum ss_opcode op,		/* opcode of instruction */
	     int jr_r31p,		/* is this a JR using r31? */
	     int jalr_r31p,		/* is this a JALR using r31? */
	     int *p_caller_tos,		/* retstack TOS if supplied by caller*/
	     int retstack_gate,		/* True = ok to use retstack */
	     struct bpred_update_info *b_update_rec, /* ptr to info f/ update*/
	     struct bpred_recover_info *recover_rec) /* retstack/hist recovery
						      * info */
/* retstack gating is determined by caller */
{
  unsigned char *p = NULL;
  struct bpred_btb_ent *pbtb = NULL;
  int index, i;
  int taken = -1;
  assert(b_update_rec && recover_rec);

  /* if this is not a branch, or if pred is null, return not-taken */
  if (!(SS_OP_FLAGS(op) & F_CTRL) || pred == NULL)
    return 0;

  pred->lookups++;
#ifdef FSIM_IN_FETCH
  if (spec_mode) pred->spec_lookups++;
#endif

  /* a tag for debugging purposes */
  recover_rec->repair_id = ++repair_tag;
  recover_rec->bq_idx = -1;

  /* Except for jumps, get a pointer to direction-prediction bits */
  switch (pred->class)
    {
    case BPredHybrid:
      {
	unsigned char *dir_update_ptr1, *dir_update_ptr2;
	unsigned char bits1, bits2, num_bits1, num_bits2;
	unsigned char which = 0;
	SS_ADDR_TYPE component_pred1, component_pred2;
	struct bpred_recover_info ignore1;
	int ignore0 = -1;

	if ((SS_OP_FLAGS(op) & (F_CTRL|F_UNCOND)) != (F_CTRL|F_UNCOND))
	  {
	    /* get component predictions.  We can use 'b_update_rec'
	     * that was passed in, since it doesn't yet contain useful info */
	    component_pred1 = bpred_lookup(pred->dirpred.hybrid.pred1,
					  baddr,
					  btarget,
					  cpred,
					  spec_mode,
					  op,
					  jr_r31p, jalr_r31p,
					  &ignore0,
					  TRUE,
					  b_update_rec,
					  &ignore1);
	    dir_update_ptr1 = b_update_rec->dir_update_ptr1;
	    bits1 = b_update_rec->bits;
	    num_bits1 = b_update_rec->num_bits;
	    component_pred2 = bpred_lookup(pred->dirpred.hybrid.pred2,
					  baddr,
					  btarget,
					  cpred,
					  spec_mode,
					  op,
					  jr_r31p, jalr_r31p,
					  &ignore0,
					  TRUE,
					  b_update_rec,
					  &ignore1);
	    dir_update_ptr2 = b_update_rec->dir_update_ptr1; /* sic */
	    bits2 = b_update_rec->bits;
	    num_bits2 = b_update_rec->num_bits;
	    
	    /* fill out update-info record with components' predictions */
	    b_update_rec->dir_update_ptr1 = dir_update_ptr1;
	    b_update_rec->dir_update_ptr2 = dir_update_ptr2;
	    b_update_rec->dir1 = !!component_pred1;
	    b_update_rec->dir2 = !!component_pred2;

	    /* choose which predictor to believe */
	    if (pred->dirpred.hybrid.shift_width)
	      {
		index = pred->dirpred.hybrid.shift_reg;
		if (pred->gshare)
		  index ^= (baddr >> 3);
		else /* gselect */
		  index |= (baddr >> 3) << pred->dirpred.hybrid.shift_width;
		index &= pred->dirpred.hybrid.size - 1;
	      }
	    else
	      index = HYBRID_HASH(pred, baddr);
	      /* was '((baddr >> 3) & (pred->dirpred.hybrid.size - 1)))' */

	    p = &pred->dirpred.hybrid.table[index];

	    if (*p <= pred->cntr_threshold)
	      {
		/* use first predictor */ 
		taken = b_update_rec->dir1;
		b_update_rec->bits = bits1;
		b_update_rec->num_bits = num_bits1;
		which = 1;
	      }
	    else
	      {
		/* use second predictor */ 
		taken = b_update_rec->dir2;
		b_update_rec->bits = bits2;
		b_update_rec->num_bits = num_bits2;
		which = 2;
	      }
	  }	      
	else 
	  taken = 1;

	/* checkpoint the appropriate global-history info */
	if (pred->dirpred.hybrid.global_save_p)
	  recover_rec->global_history = *pred->dirpred.hybrid.global_save_p;
#ifdef DEBUG_HYB
	recover_rec->global_history = pred->dirpred.hybrid.pred1->dirpred.two.shiftregs[0].history;
	assert(pred->dirpred.hybrid.pred1->dirpred.two.shiftregs[0].history
	      == pred->dirpred.hybrid.pred2->dirpred.two.shiftregs[0].history);
	assert(pred->dirpred.hybrid.shift_reg 
	       == (pred->dirpred.hybrid.pred1->dirpred.two.shiftregs[0].history
		   & ((1 << pred->dirpred.hybrid.shift_width) - 1)));
#endif
	    
	b_update_rec->pred_pred_ptr = p;
	b_update_rec->which = which;
	break;
      }
    case BPred2Level:
      {
	int l1index, l2index;
	unsigned int history;

	if ((SS_OP_FLAGS(op) & (F_CTRL|F_UNCOND)) != (F_CTRL|F_UNCOND))
	  {
	    /* 
	     * traverse 2-level tables 
	     */

	    l1index = (baddr >> 3) & (pred->dirpred.two.l1size - 1);
	    history = pred->dirpred.two.shiftregs[l1index].history;

	    if (pred->use_bq)
	      {
		int i, n, found = FALSE;

		/* search the BQ for an entry matching this baddr;
		 * must search the whole queue because a baddr might have
		 * multiple entries */
		for (i = pred->bq.head, n = 0 ; 
		     n < pred->bq.num ;
		     n++, i = (i + 1) % pred->bq.size)
		  {
		    assert(pred->bq.tbl[i].addr != BQ_JUNK_VAL);
		    if (pred->bq.tbl[i].addr == baddr)
		      history = pred->bq.tbl[i].history;
		  }
	      }

	    l2index = history & ((1 << pred->dirpred.two.shift_width) - 1);
	    assert(l2index == history);
	    b_update_rec->history = history;
	    b_update_rec->bits = l2index;	/* pass hist bits for conf */
	    
	    /* possibly combine some g-hist and l-hist bits */
	    if (pred->merge_hist)
	      l2index ^= pred->aux_global_shift_reg;
	    else if (pred->merge_hist_shift)
	      l2index ^= pred->aux_global_shift_reg 
		<< (log_base2(pred->dirpred.two.l2size)-pred->aux_global_size);
	    else if (pred->cat_hist)
	      l2index |= pred->aux_global_shift_reg 
		<< pred->dirpred.two.shift_width;

	    /* possibly combine the above hist bits with the baddr */
	    if (pred->gshare) /* && pred->dirpred.two.l1size == 1) */
	      /* allow address-hashing for global or local schemes */
	      l2index ^= (baddr >> (3 + pred->gshare_drop_lsbits));
	    else if (pred->gshare_shift)
	      /* address hashing, but with baddr shifted left to preserve
	       * some low-order (ie recent) local-history bits unmolested */
	      l2index ^= (baddr >> (3 + pred->gshare_drop_lsbits))
				<< pred->gshare_shift;
#ifdef BUG_COMPAT_MIX
	    else
	      /* gselect */
	      l2index |= (baddr >> (3 + pred->gshare_drop_lsbits)) 
		<< pred->dirpred.two.shift_width;
#else
	    else if (!pred->merge_hist && !pred->merge_hist_shift 
		     && !pred->cat_hist)
	      /* gselect */
	      l2index |= (baddr >> (3 + pred->gshare_drop_lsbits)) 
		<< pred->dirpred.two.shift_width;
	    else if (pred->merge_hist || pred->cat_hist)
	      /* if mixing global with local history and there are unused
	       * index bits, fill them in with baddr bits in a gselect style */
	      {
		int left_shift;

		if (pred->cat_hist)
		  left_shift = pred->dirpred.two.shift_width 
		    + pred->aux_global_size;
		else if (pred->merge_hist)
		  left_shift = MAX(pred->dirpred.two.shift_width,
				   pred->aux_global_size);

		l2index |= (baddr >> (3 + pred->gshare_drop_lsbits)) 
		  << left_shift;
	      }
#endif

	    l2index &= pred->dirpred.two.l2size - 1;

	    pred->dirpred.two.shiftregs[l1index].refs++;

	    /* get a pointer to prediction state information */
	    p = &pred->dirpred.two.l2table[l2index];

	    if (pred->agree)
	      taken = ((*p <= pred->cntr_threshold)
		       ? /* agree with static */!!(baddr > btarget)
		       : /* disagree w static */!(baddr > btarget));
	    else
	      taken = ((*p > pred->cntr_threshold)
		       ? /* taken */ 1
		       : /* not taken */ 0);
	  }
	else
	  {
	    taken = 1;
	    b_update_rec->bits = 0xffffffff;
	  }

	/* provide current global and/or local-history-register contents in 
	 * case we're doing speculative history updates */
#ifndef LIST_FETCH
	if (pred->spec_update_repair)
#endif
	  {
	    if (pred->dirpred.two.l1size == 1)
	      recover_rec->global_history 
		= pred->dirpred.two.shiftregs[0].history;
	    /* (note; used to have local stuff here) */
	    if (pred->aux_global_size > 0)
	      {
		assert(pred->dirpred.two.l1size > 1);
		recover_rec->global_history = pred->aux_global_shift_reg;
	      }
	  }
	
	b_update_rec->num_bits = pred->dirpred.two.shift_width;
	b_update_rec->dir_update_ptr1 = p;
	break;
      }

    case BPred2bit:
      {
	if ((SS_OP_FLAGS(op) & (F_CTRL|F_UNCOND)) != (F_CTRL|F_UNCOND))
	  {
	    p = &pred->dirpred.bimod.table[TWOBIT_HASH(pred, baddr)];

	    if (pred->agree)
	      taken = ((*p <= pred->cntr_threshold)
		       ? /* agree with static */!!(baddr > btarget)
		       : /* disagree w static */!(baddr > btarget));
	    else
	      taken = ((*p > pred->cntr_threshold)
		       ? /* taken */ 1
		       : /* not taken */ 0);

	    b_update_rec->bits = *p;	/* these are the only history bits */
	  }                             /*    available */
	else
	  {
	    taken = 1;
	    b_update_rec->bits = 0xffffffff;
	  }

	b_update_rec->num_bits = pred->dirpred.two.shift_width;
	b_update_rec->dir_update_ptr1 = p;
	break;
      }

    case BPredTaken:
      b_update_rec->dir_update_ptr1 = &dummy_taken;
      b_update_rec->num_bits = 0;
      taken = 1;
      break;

    case BPredNotTaken:
      b_update_rec->dir_update_ptr1 = &dummy_nottaken;
      b_update_rec->num_bits = 0;
      b_update_rec->predPC_if_taken = 1; /* was baddr + sizeof(SS_INST_TYPE) */
      taken = 0;
      break;

    case BPredBackwards:
      b_update_rec->dir_update_ptr1 = &dummy_backwards;
      b_update_rec->num_bits = 0;
      if (SS_OP_FLAGS(op) & F_UNCOND)
	taken = 1;
      else
	taken = (baddr > btarget) ? 1 : 0;
      break;

    case BPredPerfect:
      b_update_rec->dir_update_ptr1 = &dummy_perfect;
      b_update_rec->num_bits = 0;
      return cpred;

    default:
      panic("bogus predictor class");
    }

  /*
   * We have gotten a direction prediction, in 'taken'
   */

  /* if this is a return, pop return-address stack.  Use caller-supplied
   * TOS if appropriate. */
  if (op == JR && jr_r31p && pred->retstack.size && retstack_gate)
    {
      SS_ADDR_TYPE target;

      if (pred->retstack.caller_supplies_tos)
	pred->retstack.tos = *p_caller_tos;
      assert(pred->retstack.tos >= 0);

      target = pred->retstack.stack[pred->retstack.tos].target;
      pred->retstack.tos = (pred->retstack.tos + pred->retstack.size - 1)
	                   % pred->retstack.size;

      if (pred->retstack.caller_supplies_tos)
	*p_caller_tos = pred->retstack.tos;

#ifdef RETSTACK_COUNTS
      pred->retstack_pops++;
#endif
      /* record pre-pop TOS info, since we return right now.  Note this
       * will use the caller-supplied TOS if appropriate. */
      recover_rec->tos = pred->retstack.tos;
      if (pred->retstack.patch_level == RETSTACK_PATCH_PTR_DATA)
	recover_rec->contents.tos_value 
	  = pred->retstack.stack[pred->retstack.tos].target;
      else if (pred->retstack.patch_level == RETSTACK_PATCH_WHOLE 
	       && !sim_warmup)
	{
	  recover_rec->contents.stack_copy = calloc(pred->retstack.size,
						 sizeof(struct bpred_btb_ent));
	  if (!recover_rec->contents.stack_copy)
	    fatal("out of memory: couldn't allocate retstack copy");
	  memcpy(recover_rec->contents.stack_copy,
		 pred->retstack.stack,
		 pred->retstack.size * sizeof(struct bpred_btb_ent));
	}
      else
	recover_rec->contents.stack_copy = NULL;

#ifdef SINGLE_RETSTACK_LOG
      fprintf(stderr, "0x%08x popped 0x%08x from slot %d\n", 
	      baddr, target, (pred->retstack.tos + 1) % pred->retstack.size);
#endif

      return target;
    }

  /* if this is a function call, push return-address onto return-address 
   * stack. Use caller-supplied TOS if appropriate. */
  if ((op == JAL || (op == JALR && jalr_r31p)) && pred->retstack.size 
      && retstack_gate)
    {
      if (pred->retstack.caller_supplies_tos)
	pred->retstack.tos = *p_caller_tos;
      assert(pred->retstack.tos >= 0);

      pred->retstack.tos = (pred->retstack.tos + 1) % pred->retstack.size;
      pred->retstack.stack[pred->retstack.tos].target = 
	baddr + sizeof(SS_INST_TYPE);

      if (pred->retstack.caller_supplies_tos)
	*p_caller_tos = pred->retstack.tos;

#ifdef RETSTACK_COUNTS
      pred->retstack_pushes++;
#endif

#ifdef SINGLE_RETSTACK_LOG
      fprintf(stderr, "0x%08x pushed 0x%08x into slot %d\n", 
	      baddr, baddr+SS_INST_SIZE, pred->retstack.tos);
#endif      
    }
  
  /* record pre-pop TOS info; if this branch is executed speculatively
   * and is squashed, we'll restore the TOS and its value and hope the data
   * wasn't corrupted in the meantime.  Note this will use the caller-supplied
   * TOS if appropriate. */
  if (pred->retstack.size)
    {
      recover_rec->tos = pred->retstack.tos;
      if (pred->retstack.patch_level == RETSTACK_PATCH_PTR_DATA)
	recover_rec->contents.tos_value 
	  = pred->retstack.stack[pred->retstack.tos].target;
      else if (pred->retstack.patch_level == RETSTACK_PATCH_WHOLE
	       && !sim_warmup)
	{
	  recover_rec->contents.stack_copy = calloc(pred->retstack.size,
						 sizeof(struct bpred_btb_ent));
	  if (!recover_rec->contents.stack_copy)
	    fatal("out of memory: couldn't allocate retstack copy");
	  memcpy(recover_rec->contents.stack_copy,
		 pred->retstack.stack,
		 pred->retstack.size * sizeof(struct bpred_btb_ent));
	}
      else
	recover_rec->contents.stack_copy = NULL;
    }
  else
    {
      recover_rec->tos = 0;
      recover_rec->contents.tos_value = 0;
    }

  /* not a return, or bypassed the retstack. Get a pointer into the BTB */
  if (pred->btb.btb_data)
    {
      index = (baddr >> 3) & (pred->btb.sets - 1);
      
      if (pred->btb.assoc > 1)
	{
	  index *= pred->btb.assoc;
	  
	  /* Now we know the set; look for a PC match */
	  for (i = index; i < (index+pred->btb.assoc) ; i++)
	    if (pred->btb.btb_data[i].addr == baddr)
	      {
		/* match */
		pbtb = &pred->btb.btb_data[i];
		break;
	      }
	}	
      else
	{
	  pbtb = &pred->btb.btb_data[index];
	  if (pbtb->addr != baddr)
	    pbtb = NULL;
	}
    }

  /*
   * We now also have a pointer into the BTB for a hit, or NULL otherwise
   */
  b_update_rec->predPC_if_taken = (pbtb ? pbtb->target : 1);

  /* if this is a jump, ignore predicted direction; we know it's taken. */
  if ((SS_OP_FLAGS(op) & (F_CTRL|F_UNCOND)) == (F_CTRL|F_UNCOND))
    return (pbtb ? pbtb->target : 1);

  /* otherwise we have a conditional branch */
  if (pbtb == NULL)
    {
      /* BTB miss -- just return a predicted direction */
      return taken;
    }
  else
    {
      /* BTB hit, so return target if it's a predicted-taken branch */
      return (taken
	      ? /* taken */ pbtb->target
	      : /* not taken */ 0);
    }
}


/* Speculative execution can corrupt the ret-addr stack.  So for each
 * lookup we return the top-of-stack (TOS) at that point; a mispredicted
 * branch, as part of its recovery, restores the TOS using this value --
 * hopefully this uncorrupts the stack. */
void
bpred_retstack_recover(struct bpred *pred,	/* branch predictor instance */
	      SS_ADDR_TYPE baddr,	/* branch address */
	      int *p_caller_tos,	/* retstack TOS supplied to caller */
	      int retstack_gate,	/* True = ok to use retstack */
	      struct bpred_recover_info *recover_rec) /* retstack/hist
						       * recovery info */
/* retstack gating is determined by caller */
{
  if (pred == NULL || pred->retstack.size == 0 || !retstack_gate)
    return;

  if (pred->retstack.patch_level == RETSTACK_PATCH_NONE)
    return;

  /* We don't need to use a caller-supplied TOS here, because we'd just
   * overwrite it with the recovered TOS */
  pred->retstack.tos = recover_rec->tos;
  
  if (pred->retstack.caller_supplies_tos)
    *p_caller_tos = pred->retstack.tos;

  if (pred->retstack.patch_level == RETSTACK_PATCH_PTR_DATA)
    pred->retstack.stack[pred->retstack.tos].target 
      = recover_rec->contents.tos_value;
  else if (pred->retstack.patch_level == RETSTACK_PATCH_WHOLE 
	   && !sim_warmup && recover_rec->contents.stack_copy)
    {
      memcpy(pred->retstack.stack,
	     recover_rec->contents.stack_copy,
	     pred->retstack.size * sizeof(struct bpred_btb_ent));
    }

#ifdef SINGLE_RETSTACK_LOG
  fprintf(stderr, "\t0x%08x repaired stack; new tos = %d\n",
	  baddr, recover_rec->tos);
#endif
}

/* add a branch to the BQ.  Return value is the assigned BQ index */
int
bpred_bq_add(struct bpred *pred,
	     SS_ADDR_TYPE baddr,	/* branch address */
	     unsigned int history,	/* most recent history for this br */
	     int taken,			/* non-zero if branch was pred taken */
	     enum ss_opcode op, 	/* opcode of instruction */
	     SS_COUNTER_TYPE repair_id) /* tag for debugging */
{
  int curr = pred->bq.tail;
  unsigned int shift_reg;

  /* note that when we eventually allow overflow, we have to
   * match address on commit (ie bq_remove) */
  assert(pred->bq.num <= pred->bq.size);

  /* allocate BQ entry */
  pred->bq.tbl[curr].addr = baddr;
  pred->bq.tbl[curr].repair_id = repair_id;

  /* uncond branches don't need to store history; cond branches save
   * the specualtive history value that's being generated (leaving the
   * committed but stale history in the BHT) */
  if (SS_OP_FLAGS(op) & F_UNCOND)
    pred->bq.tbl[curr].history = 0;
  else
    {
      shift_reg = (history << 1) | (!!taken);
      pred->bq.tbl[curr].history = shift_reg 
	& ((1 << pred->dirpred.two.shift_width) - 1);
    }
  
  pred->bq.num++;
  pred->bq.tail = (pred->bq.tail + 1) % pred->bq.size;

  return curr;
}

/* remove a branch from the BQ */
void
bpred_bq_remove(struct bpred *pred,
		SS_ADDR_TYPE baddr,	/* branch address */
		int taken,		/* non-zero if branch was pred taken */
		enum ss_opcode op, 	/* opcode of instruction */
		SS_COUNTER_TYPE repair_id) /* tag for debugging */
{
  int curr = pred->bq.head;
  int l1index = (baddr >> 3) & (pred->dirpred.two.l1size - 1);

  if (pred->bq.tbl[curr].addr != baddr 
      || pred->bq.tbl[curr].repair_id != repair_id)
    /* note this will no longer be true when we allow overflow */
    panic("BQ's head does not match the branch being committed");

  if (SS_OP_FLAGS(op) & F_COND)
    pred->dirpred.two.shiftregs[l1index].history 
      = pred->bq.tbl[curr].history;

  pred->bq.tbl[curr].addr = BQ_JUNK_VAL;
  pred->bq.tbl[curr].history = BQ_JUNK_VAL;

  pred->bq.num--;
  pred->bq.head = (pred->bq.head + 1) % pred->bq.size;
}

/* If doing speculative history updates, history info can get corrupted.
 * So each branch saves some history info, and a mispredicted branch,
 * as part of its recovery, restores the history state and shifts in the
 * correct branch result. */
void
bpred_history_recover(struct bpred *pred,	/* branch predictor instance */
		      SS_ADDR_TYPE baddr,	/* branch address */
		      int taken,		/* non-zero if branch taken */
		      enum ss_opcode op,	/* opcode of instruction */
		      enum pipestage stage,	/* caller's pipestage */
		      struct bpred_recover_info *recover_rec) /* recov. info */
{
  int global_history_recover = recover_rec->global_history;
  int doit = pred->spec_update && pred->spec_update_repair;
  int bq_idx = recover_rec->bq_idx;

  if (pred == NULL)
    return;

  /* take global history register contents from before misprediction
   * and, if this is a conditional branch, shift in its result */
  if (doit && (SS_OP_FLAGS(op) & F_COND))
    {
      global_history_recover <<= 1;
      global_history_recover |= !!taken;
    }

  switch (pred->class)
    {
    case BPredHybrid:
      {
	assert(!pred->aux_global_size);

	bpred_history_recover(pred->dirpred.hybrid.pred1, baddr, 
			      taken, op, stage, recover_rec);
	bpred_history_recover(pred->dirpred.hybrid.pred2, baddr, 
			      taken, op, stage, recover_rec);
	
	if (doit && pred->dirpred.hybrid.shift_width)
	  pred->dirpred.hybrid.shift_reg = global_history_recover 
	    & ((1 << pred->dirpred.hybrid.shift_width) - 1);

	break;
      }

    case BPred2Level:
      {
	if (doit && pred->aux_global_size)
	  /* for aux-global history, just restore the correct aux-global
	   * history-register contents */
	  pred->aux_global_shift_reg = global_history_recover
	    & ((1 << pred->aux_global_size) - 1);

	if (doit && pred->dirpred.two.l1size == 1)
	  {
	    /* for global history, just restore the correct global history-
	     * register contents */
	    assert(!pred->aux_global_size);
	    pred->dirpred.two.shiftregs[0].history = global_history_recover 
	      & ((1 << pred->dirpred.two.shift_width) - 1);
	  }
	else if (doit && pred->use_bq)
	  {
	    int curr, new_tail;
	    assert(bq_idx >= 0 && bq_idx < pred->bq.size);

	    /* will have to be fixed once we allow overflow */
	    assert(pred->bq.tbl[bq_idx].addr == baddr &&
		   pred->bq.tbl[bq_idx].repair_id == recover_rec->repair_id);

	    /* nuke BQ contents after this mispredicted branch */
	    curr = (bq_idx + 1) % pred->bq.size;
	    new_tail = curr;
	    while (curr != pred->bq.tail) 
	      {
		pred->bq.tbl[curr].addr = BQ_JUNK_VAL;
		pred->bq.tbl[curr].history = BQ_JUNK_VAL;
		
		pred->bq.num--;
		curr = (curr + 1) % pred->bq.size;
	      } 
	    pred->bq.tail = new_tail;

	    /* now, for conditional branches replace the wrong speculative
	     * history with the correct one; it'll get committed on update() */
	    if (SS_OP_FLAGS(op) & F_COND)
	      {
		pred->bq.tbl[bq_idx].history &= ~1;
		pred->bq.tbl[bq_idx].history |= !!taken;
	      }
	  }
	
	break;
      }
    
    default:
      break;
    }
}

/* Update the branch predictor's history bits, but not the 
 * direction-prediction bits.  This can be done at branch-commit time in
 * conjunction with updating the direction-prediction bits, or can be
 * done separately (eg speculatively).  It updates the entry for instruction
 * type OP at address BADDR, with taken/not-taken info in PRED_TAKEN.
 * Return value is a BQ index, if assigned, else -1 */
void
bpred_history_update(struct bpred *pred,/* branch predictor instance */
	             SS_ADDR_TYPE baddr,/* branch address */
		     int taken,		/* non-zero if branch was pred taken */
		     enum ss_opcode op, /* opcode of instruction */
		     int speculative,	/* is this a speculative update? */
		     struct bpred_update_info *b_update_rec,/* misc br state */
		     struct bpred_recover_info *recover_rec) /* recov. info */
{
  int bq_idx = -1;
  SS_COUNTER_TYPE repair_id = recover_rec->repair_id;
  unsigned int prior_hist = b_update_rec->history;

  /* update this predictor's history bits only once per branch -- but
   * for some update schemes, we update some state speculatively, some
   * state non-speculatively */
  int do_update = !!(pred->spec_update == speculative);

  /* don't change bpred state for non-branch instructions or (obviously) 
   * if pred is NULL */
  if (!(SS_OP_FLAGS(op) & F_CTRL) || pred == NULL)
    return;

  /* can exit now if this is an uncond. branch */
  if ((SS_OP_FLAGS(op) & F_UNCOND) && pred->class != BPredHybrid)
    {
      if (pred->use_bq && speculative)
	{
	  /* local spec-update: don't need to save a speculative history, 
	   * but do need a placeholder to make squashing easy */
	  bq_idx = bpred_bq_add(pred, baddr, prior_hist, taken, op, repair_id);
	}
      else if (pred->use_bq && !speculative)
	{
	  /* local spec-update: don't need to commit this branch's 
	   * speculative history, but do remove it from the BQ */
	  bpred_bq_remove(pred, baddr, taken, op, repair_id);
	  bq_idx = -1;
	}

      recover_rec->bq_idx = bq_idx;
      return;
    }

  /* Have a conditional branch here */

  switch (pred->class)
    {
    case BPredHybrid:
      {
	assert(!pred->aux_global_size);

	/* update component predictors */
	bpred_history_update(pred->dirpred.hybrid.pred1,
			     baddr, taken, op, speculative, 
			     b_update_rec, recover_rec);
	
	bpred_history_update(pred->dirpred.hybrid.pred2,
			     baddr, taken, op, speculative, 
			     b_update_rec, recover_rec);

	/* update global shift-reg if appropriate */
	if (do_update && pred->dirpred.hybrid.shift_width)
	  {
	    pred->dirpred.hybrid.shift_reg <<= 1; 
	    pred->dirpred.hybrid.shift_reg |= !!taken;
	    pred->dirpred.hybrid.shift_reg &= 
	      (1 << pred->dirpred.hybrid.shift_width) - 1;
	  }
	break;
      }

    case BPred2Level:
      {
	int l1index;
	unsigned int shift_reg;

	/* global-history */
	if (do_update && pred->dirpred.two.l1size == 1)
	  {
	    shift_reg = (pred->dirpred.two.shiftregs[0].history << 1)
	      | (!!taken);
	    pred->dirpred.two.shiftregs[0].history 
	      = shift_reg & ((1 << pred->dirpred.two.shift_width) - 1);
	  }
	/* LOCAL HISTORY */
	else if (pred->dirpred.two.l1size > 1) 
	  {
	    /* update aux_global register if it exists */
	    if (do_update && pred->aux_global_size)
	      {
		assert(pred->dirpred.two.l1size > 1);
		pred->aux_global_shift_reg <<= 1;
		pred->aux_global_shift_reg |= !!taken;
		pred->aux_global_shift_reg &= (1 << pred->aux_global_size) - 1;
	      }

	    /* 
	     * update appropriate L1 history register or BQ
	     */
	    if (pred->use_bq && speculative)
	      {
		/* local spec-update: put speculative history in the BQ */
		bq_idx = bpred_bq_add(pred, baddr, prior_hist, taken, 
				      op, repair_id);
	      }
	    else if (pred->use_bq && !speculative)
	      {
		/* local spec-update: we can now commit this branch's 
		 * speculative history from the BQ to the BHT */
		bpred_bq_remove(pred, baddr, taken, op, repair_id);
		bq_idx = -1;
	      }
	    else if ((do_update && 
		      pred->spec_update_repair!=HISTORY_PATCH_AUX_GLOBAL_ONLY)
		    || (pred->spec_update_repair==HISTORY_PATCH_AUX_GLOBAL_ONLY
			&& pred->spec_update && !speculative))
	      {
		l1index = (baddr >> 3) & (pred->dirpred.two.l1size - 1);
		
		shift_reg = (pred->dirpred.two.shiftregs[l1index].history << 1)
		  | (!!taken);
		pred->dirpred.two.shiftregs[l1index].history 
		  = shift_reg & ((1 << pred->dirpred.two.shift_width) - 1);
	      }
	  }
	    
	recover_rec->bq_idx = bq_idx;
	break;
      }

    default:
      break;
    }

  /* Sanity checks */
  if (pred->use_bq)
    {
      assert(pred->bq.num >= 0 && pred->bq.num <= pred->bq.size);
      assert(pred->bq.head >= 0 && pred->bq.head < pred->bq.size);
      assert(pred->bq.tail >= 0 && pred->bq.tail < pred->bq.size);
      assert((pred->bq.head + pred->bq.num) % pred->bq.size == pred->bq.tail);
    }

}

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
	     int correct,		/* was earlier addr prediction ok? */
	     enum ss_opcode op,		/* opcode of instruction */
	     int jr_r31p,		/* is this a JR using r31? */
	     int retstack_gate,		/* True = ok to use retstack */
	     int hybrid_component,	/* Is this a hybrid component? */
	     struct bpred_update_info b_update_rec, /* pred info f/ update */
	     struct bpred_recover_info *recover_rec) /* retstack/hist recovery
						      * info */
/* retstack gating is determined by caller */
{
  struct bpred_btb_ent *pbtb = NULL;
  struct bpred_btb_ent *lruhead = NULL, *lruitem = NULL;
  int index, i;

  /* don't change bpred state for non-branch instructions or (obviously) 
   * if pred is NULL */
  if (!(SS_OP_FLAGS(op) & F_CTRL) || pred == NULL)
    return;

  /* Have a branch here */

  pred->updates++;

  /* keep general stats */
  if (correct)
    pred->addr_hits++;

  if (!!pred_taken == !!taken)
    {
      pred->dir_hits++;
      if ((SS_OP_FLAGS(op) & (F_CTRL|F_UNCOND)) != (F_CTRL|F_UNCOND))
	pred->cond_hits++;
    }
  else
    pred->misses++;

  if ((SS_OP_FLAGS(op) & (F_CTRL|F_UNCOND)) != (F_CTRL|F_UNCOND))
    pred->cond_seen++;

  /* Do history update:
   * history update is done by bpred_history_update(), which determines
   * whether the update has already been done (spec-hist-updates enabled). 
   * But don't call update again for hybrid components, because it's
   * not idempotent */
  if (!hybrid_component)
    {
      bpred_history_update(pred, baddr, taken, op, FALSE, 
			   &b_update_rec, recover_rec);
      assert(recover_rec->bq_idx == -1);
    }
    
  /* keep stats about JR's; also, but don't change any bpred state for JR's
   * which are returns unless there's no retstack */
  if (op == JR && jr_r31p)
    {
      pred->jr_seen++;
      if (correct)
	pred->jr_hits++;
#ifdef SINGLE_RETSTACK_LOG
      else
	fprintf(stderr, "JR-31 missed.  PC = 0x%08x, correct = 0x%08x\n",
		baddr, btarget);
#endif
      
      if (pred->retstack.size && !pred->retstack.update_btb)
	/* if returns aren't supposed to update BTB, we can return now */
	return;
    }
  /* keep stats about indir jumps overall */
  if (((SS_OP_FLAGS(op) & (F_CTRL|F_INDIRJMP)) == (F_CTRL|F_INDIRJMP))
      && !(op == JR && jr_r31p))
    {
      pred->indir_seen++;
      if (correct)
	pred->indir_hits++;
    }

  /* can exit now if this is the perfect predictor */
  if (pred->class == BPredPerfect)
    return;

  /* 
   * Now we know the branch didn't use the ret-addr stack
   */

  /* update direction-predictors or predictor-predictors (but not for jumps) */
  if ((SS_OP_FLAGS(op) & (F_CTRL|F_UNCOND)) != (F_CTRL|F_UNCOND))
  {
    switch (pred->class)
      {
      case BPredHybrid:
	{
	  unsigned char *p;
	  struct bpred_update_info component_rec = {NULL, NULL, 5, 5, NULL};
	  int p1c = (b_update_rec.dir1 == !!taken);
	  int p2c = (b_update_rec.dir2 == !!taken);

	  /* update component predictors */
	  component_rec.dir_update_ptr1 = b_update_rec.dir_update_ptr1;
	  bpred_update(pred->dirpred.hybrid.pred1,
		       baddr,
		       btarget,
		       offset,
		       taken,
		       b_update_rec.dir1,
		       FALSE,
		       op,
		       jr_r31p,
		       TRUE,
		       TRUE,
		       component_rec,
		       NULL);

	  component_rec.dir_update_ptr1 = b_update_rec.dir_update_ptr2;
	  bpred_update(pred->dirpred.hybrid.pred2,
		       baddr,
		       btarget,
		       offset,
		       taken,
		       b_update_rec.dir2,
		       FALSE,
		       op,
		       jr_r31p,
		       TRUE,
		       TRUE,
		       component_rec,
		       NULL);

	  /* update predictor-predictor table */
	  p = b_update_rec.pred_pred_ptr;
	  switch (p1c - p2c)
	    {
	    case 0:
	      /* both predictors gave the same answer -- no change */
	      break;

	    case 1:
	      /* pred1 was correct, pred2 incorrect */
	      if (*p > 0)
		--*p;
	      break;

	    case -1:
	      /* pred2 was correct, pred1 incorrect */
	      if (*p < pred->cntr_max)
		++*p;
	      break;

	    default:
	      fatal("eek!");
	    }

	  /* keep track of usage pattern */
	  if (b_update_rec.which == 1)
	    pred->used_pred1++;

#ifdef DEBUG_HYB
	recover_rec->global_history = pred->dirpred.hybrid.pred1->dirpred.two.shiftregs[0].history;
	assert(pred->dirpred.hybrid.pred1->dirpred.two.shiftregs[0].history
	      == pred->dirpred.hybrid.pred2->dirpred.two.shiftregs[0].history);
	assert(pred->dirpred.hybrid.shift_reg 
	       == (pred->dirpred.hybrid.pred1->dirpred.two.shiftregs[0].history
		   & ((1 << pred->dirpred.hybrid.shift_width) - 1)));
#endif
	  break;
	}

      case BPred2Level:
	{
	  int l1index = (baddr >> 3) & (pred->dirpred.two.l1size - 1);
	  unsigned char *p = b_update_rec.dir_update_ptr1;
	  
	  if (correct) 
	    pred->dirpred.two.shiftregs[l1index].hits++;

	  /* update L2 direction-predictor */
	  if (pred->agree)
	    {
	      SS_ADDR_TYPE btarget = baddr + 8 + (offset << 2);   /* local! */
	      if (p)
		{
		  if ((taken && (baddr > btarget))
		      || (!taken && (baddr < btarget)))
		    {
		      /* agrees */
		      if (*p > 0)
			--*p;
		    }
		  else
		    {
		      /* disagrees */
		      if (*p < pred->cntr_max)
			++*p;
		    }
		}
	    }
	  else
	    {
	      if (p)
		{
		  if (taken)
		    {
		      if (*p < pred->cntr_max)
			++*p;
		    }
		  else
		    {
		      /* not taken */
		      if (*p > 0)
			--*p;
		    }
		}
	    }

	  break;
	}

      case BPred2bit:
	{
	  unsigned char *p = b_update_rec.dir_update_ptr1;

	  if (pred->agree)
	    {
	      SS_ADDR_TYPE btarget = baddr + 8 + (offset << 2);   /* local! */
	      if (p)
		{
		  if ((taken && (baddr > btarget))
		      || (!taken && (baddr < btarget)))
		    {
		      /* agrees */
		      if (*p > 0)
			--*p;
		    }
		  else
		    {
		      /* disagrees */
		      if (*p < pred->cntr_max)
			++*p;
		    }
		}
	    }
	  else
	    {
	      if (p)
		{
		  if (taken)
		    {
		      if (*p < pred->cntr_max)
			++*p;
		    }
		  else
		    {
		      /* not taken */
		      if (*p > 0)
			--*p;
		    }
		}
	    }

	  break;
	}

      default:
	break;
      }
  }
  /* else it's a jump */

  /*
   * Direction-predictor and hybrid predictor-predictor have been updated.
   * Now update BTB
   */

  /* find BTB entry if it's a taken branch (don't allocate for non-taken) */
  if (taken && pred->btb.btb_data)
    {
      index = (baddr >> 3) & (pred->btb.sets - 1);
      
      if (pred->btb.assoc > 1)
	{
	  index *= pred->btb.assoc;
	  
	  /* Now we know the set; look for a PC match; also identify
	   * MRU and LRU items */
	  for (i = index; i < (index+pred->btb.assoc) ; i++)
	    {
	      if (pred->btb.btb_data[i].addr == baddr)
		/* match */
		pbtb = &pred->btb.btb_data[i];
	      
	      if (pred->btb.btb_data[i].prev == NULL)
		/* this is the head of the lru list, ie current MRU item */
		lruhead = &pred->btb.btb_data[i];
	      if (pred->btb.btb_data[i].next == NULL)
		/* this is the tail of the lru list, ie the LRU item */
		lruitem = &pred->btb.btb_data[i];
	    }
	  
	  if (!pbtb)
	    /* missed in BTB; choose the LRU item in this set as the victim */
	    pbtb = lruitem;	
	  /* else hit, and pbtb points to matching BTB entry */
	  
	  /* Update LRU state: selected item, whether selected because it
	   * matched or because it was LRU and selected as a victim, becomes 
	   * MRU */
	  if (pbtb != lruhead)
	    {
	      /* this splices out the matched entry... */
	      if (pbtb->prev)
		pbtb->prev->next = pbtb->next;
	      if (pbtb->next)
		pbtb->next->prev = pbtb->prev;
	      /* ...and this puts the matched entry at the head of the list */
	      pbtb->next = lruhead;
	      pbtb->prev = NULL;
	      lruhead->prev = pbtb;
	    }
	  /* else pbtb is already MRU item; do nothing */
	}
      else
	pbtb = &pred->btb.btb_data[index];
    }
      
  /* now 'pbtb' is a possibly null pointer into the BTB (either to a 
   * matched-on entry or a victim which was LRU in its set); update the BTB 
   * (but only for taken branches) */
  if (pbtb)
    {
      if (pbtb->addr == baddr)
	{
	  if (!correct)
	    pbtb->target = btarget;
	}
      else
	{
	  /* enter a new branch in the table */
	  pbtb->addr = baddr;
	  pbtb->op = op;
	  pbtb->target = btarget;
	}
    }
}

enum bpred_class
bpred_str2class(char *str)
{
  if (!mystricmp(str, "hybrid"))
    return BPredHybrid;
  else if (!mystricmp(str, "2lev"))
    return BPred2Level;
  else if (!mystricmp(str, "bimod"))
    return BPred2bit;
  else if (!mystricmp(str, "nottaken"))
    return BPredNotTaken;
  else if (!mystricmp(str, "taken"))
    return BPredTaken;
  else if (!mystricmp(str, "backwards"))
    return BPredBackwards;
  else if (!mystricmp(str, "perfect"))
    return BPredPerfect;
  else
    fatal("cannot parse bpred type `%s'", str);
}

