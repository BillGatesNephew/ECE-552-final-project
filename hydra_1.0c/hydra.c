/*
 * hydra.c - part of HydraScalar, a multipath-capable, out-of-order 
 * issue, cycle-level simulator 
 *
 * This file is written by Kevin Skadron, Copyright (C) 1998, 1999.
 * skadron@cs.princeton.edu
 *
 * The file is derived from "sim-outorder" in the SimpleScalar tool suite 
 * written by Todd M. Austin as a part of the Multiscalar Research Project.
 * The tool suite is currently maintained by Doug Burger and Todd M. Austin.
 * Copyright (C) 1994, 1995, 1996, 1997 by Todd M. Austin
 * INTERNET: dburger@cs.wisc.edu
 * US Mail:  1210 W. Dayton Street, Madison, WI 53706
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
 * COMPILER-DEFINABLE DIRECTIVES
 *	-DDEBUG			  Simple, fast sanity checks
 *	-DDEBUG_DEF_VALS	  Initialize or reset some variables to
 *	                 	     defined values
 *	-DDEBUG_SPEC_MEM	  Validate allocation of spec memory state
 *	-DDEBUG_PRED_PRI	  Validate correct ident. of predicted paths
 *	-DDEBUG_ALL		  All of the above
 *
 *	-DLIST_FETCH		  Print fetch trace
 *	-DRETSTACK_DEBUG_PRINTOUT Print per-thread retstack behavior
 *
 * $Id: hydra.c,v 3.64 1998/08/05 19:15:48 skadron Exp $
 *
 * $Log: hydra.c,v $
 * Revision 3.64  1998/08/05 19:15:48  skadron
 * Bug fixes in the BQ implementation:
 * 1.  recover might occur on a misfetch, not just a mispred
 * 2.  bq_add needs to use the most recent history, not the PHT contents
 *
 * Revision 3.63  1998/06/27 23:05:24  skadron
 * Another bug fix in counting of in-flight branches
 *
 * Revision 3.62  1998/06/27 21:57:51  skadron
 * 1. Removed old local-history spec-update scheme (with spec_regs, etc),
 *    and replaced it with a BQ that acts as a future file
 * 2. Changed counting of in-fligt-branches to could all non-retired
 *    branches
 *
 * Revision 3.61  1998/06/26 18:53:35  skadron
 * Initialized in_flight_branches (just insurance)
 *
 * Revision 3.60  1998/06/11 05:25:38  skadron
 * Bug fix in previous
 *
 * Revision 3.59  1998/06/11 04:39:20  skadron
 * Added "ordered" double-buffering, to prevent out-or-order branch
 *    resolution's swapping twice and nuking "good" state
 *
 * Revision 3.58  1998/06/11 03:59:54  skadron
 * Added double-buffering for local-hist spec-update
 *
 * Revision 3.55  1998/06/02 04:17:05  skadron
 * 1. Bug in hybrid predictors: after moving all history updates into
 *    bpred_history_update, a component had its history updated twice.
 *    Fixed.
 * 2. Warmup wasn't doing spec-update.
 *
 * Revision 3.53  1998/06/01 02:37:15  skadron
 * Hack to permit global-local bit mixing for hybrid configs: needs
 *    to be improved
 *
 * Revision 3.50  1998/05/30 17:41:09  skadron
 * Minor bug fix: # params wasn't right for 2lev2; didn't account for
 *    spec-update & spec-update-repair
 *
 * Revision 3.49  1998/05/29 18:03:29  skadron
 * Minor fix to preceding
 *
 * Revision 3.48  1998/05/29 17:20:44  skadron
 * Fixed bug: retstack-patch-whole didn't work properly with
 *    perfect-update due to early freeing of retstack copy
 *
 * Revision 3.47  1998/05/29 05:26:03  skadron
 * Bug fix in previous
 *
 * Revision 3.46  1998/05/29 05:25:04  skadron
 * Bug fix to previous
 *
 * Revision 3.45  1998/05/29 04:56:02  skadron
 * Bug fix to previous
 *
 * Revision 3.44  1998/05/29 04:37:43  skadron
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
 * Revision 3.40  1998/05/27 17:32:56  skadron
 * Added perfect local-history patching; not yet fully debugged
 *
 * Revision 3.39  1998/05/26 19:01:53  skadron
 * Speculative local-history registers only get nuked by "true"
 *    mispredictions
 *
 * Revision 3.32  1998/05/23 23:46:56  skadron
 * Cosmetic changes
 *
 * Revision 3.31  1998/05/23 22:24:49  skadron
 * Reverting to version 3.27 (ie, removing the filtering of well-pred branches)
 *
 * Revision 3.27  1998/05/16 21:55:02  skadron
 * Reorganized bpred to use a common bpred-history; and to allow each
 *    component to separately set the spec-update and spec-update-repair
 *    options.  Also removed premix.
 *
 * Revision 3.22  1998/05/11 23:13:24  skadron
 * Added option specifying whether to repair speculatively-updated global
 *    history
 *
 * Revision 3.21  1998/05/10 22:15:00  skadron
 * Added "if-in-btb"
 *
 * Revision 3.20  1998/05/08 21:03:18  skadron
 * Added gshare_shift
 *
 * Revision 3.19  1998/05/07 19:31:20  skadron
 * 1.  Bug fix: aux_global_shift_reg wasn't getting saved in spec-update
 *     mode
 * 2.  Added merge_hist_shift: same as merge_hist, but with global hist
 *     shifted left if possible
 *
 * Revision 3.18  1998/05/06 22:54:02  skadron
 * Added bpred features: premixing, merge-hist, and cat-hist
 *
 * Revision 3.17  1998/05/06 15:25:52  skadron
 * Fix to preceding
 *
 * Revision 3.16  1998/05/06 02:06:12  skadron
 * Found another bug: spec history update was often taken when it
 *    shouldn't be, because fetch_pred_PC was 0 for not-taken
 *
 * Revision 3.15  1998/05/05 20:45:18  skadron
 * Fixes to history_recover: 1) we were dropping the result of the
 *    mispredicted branch, and 2) we were returning garbage as the
 *    checkpointed history for jumps
 *
 * Revision 3.14  1998/05/05 16:03:38  skadron
 * Bug fix in speculative update
 *
 * Revision 3.13  1998/05/05 00:51:45  skadron
 * Implemented "agree" mode
 *
 * Revision 3.12  1998/05/04 21:13:25  skadron
 * Minor fix to preceeding; started adding "agree" mode
 *
 * Revision 3.11  1998/05/04 19:36:50  skadron
 * Added option to speculatively update global-branch-history register in
 *    fetch
 *
 * Revision 3.10  1998/04/27 18:03:03  skadron
 * Changed signal handling to use the more portable psignal()
 *
 * Revision 3.9  1998/04/27 16:59:45  skadron
 * Bug fix: go_backwards was being reset set to 0 every cycle
 *
 * Revision 3.8  1998/04/27 16:40:50  skadron
 * Added thread info to LIST_FETCH output
 *
 * Revision 3.7  1998/04/24 20:36:08  skadron
 * Minor fixes in preceding
 *
 * Revision 3.6  1998/04/24 20:14:03  skadron
 * Cleaned up uses of tracer_recover when IFQ is being cleaned up
 *
 * Revision 3.5  1998/04/24 19:03:26  skadron
 * Cleaned up sim_main(), added better signal handling, fixed names for
 *    -prime_insts and -sim_insts variables, changed name of fetchq to
 *    "ifq", gave better names to DEBUG directives, removed all
 *    "bugcompat" references
 *
 * Revision 3.4  1998/04/22 18:39:45  skadron
 * Added wrapper functions for all the messy per-thread stuff associated
 *    with bpred-lookup and bpred-recovery
 *
 * Revision 3.3  1998/04/21 19:29:19  skadron
 * Added per-thread TOSP's; removed double/halve_priorities()
 *
 * Revision 3.2  1998/04/21 16:42:33  skadron
 * New, cleaner interface for specifying per-thread or pred-only
 *    retstack(s)
 *
 * Revision 3.1  1998/04/17 16:51:35  skadron
 * Versions used thru Mar '98 (retstack sub, ruu_resub, ICS final manuscript)
 */

#ifdef __alpha__
extern int random(void);
#define RAN_MAX INT_MAX
#else
extern long random(void);
#define RAN_MAX LONG_MAX
#endif
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <signal.h>
#ifdef sun
#include <siginfo.h>
#endif
#include <limits.h>
#include <strings.h>

#include "misc.h"
#include "ss.h"
#include "regs.h"
#include "memory.h"
#include "cache.h"
#include "loader.h"
#include "syscall.h"
#include "bpred.h"
#include "bconf.h"
#include "resource.h"
#include "bitmap.h"
#include "options.h"
#include "stats.h"
#include "ptrace.h"
#include "dlite.h"
#include "sim.h"

#ifdef DEBUG_ALL
#define DEBUG
#define DEBUG_DEF_VALS
#define DEBUG_SPEC_MEM
#define DEBUG_PRED_PRI
#endif

#define UNKNOWN 2
#define KILLED 3
#define NOW 4
#define INTERMEDIATE 5

#define KEEP_N_RETIRED_INSTS 1024
#define DEAD_TIME 10000

/*
 * This file implements a very detailed out-of-order issue superscalar
 * processor with a two-level memory system and speculative execution support.
 * Support for multipath execution is included (ie, following both paths
 * of some or all conditional branches, to eliminate misprediction penalites).
 * This simulator is a performance simulator, tracking the latency of all
 * pipeline operations.
 */

/*
 * Notes on perfect-cache and perfect-TLB simulation:
 * 	Perfectness here is simulated by making the access to the perfect
 * level take as long as a hit, regardless of whether it is a hit.  This
 * is done by ignoring the latency of the operation, setting it instead to
 * the hit latency.  The operation is still performed, however, so that
 * the cache/TLB stats still report accurate counts of hits vs. misses
 * and the like.  Note these counts may be slightly perturbed, as changing
 * timing for events can affect cache behavior, especially bus utilization.
 * 	For L1 caches and for the TLBs, perfectness of this type is easily
 * simulated in this file by making the calls to cache_access() but
 * replacing the returned latency with the hit latency.  For L2 caches,
 * a similar technique is followed, but in the L1 miss-handlers.  That
 * way L1->L2 bus contention and L1->L2 writebacks are still correctly 
 * modeled.
 */

/*
 * simulator options
 */

/* can mis-speculate past at most this many branches */
#ifdef CFG64
#define N_SPEC_LEVELS 640 /* related arrays are statically allocated */
#elif CFG40
#define N_SPEC_LEVELS 384
#elif CFG24
#define N_SPEC_LEVELS 256
#elif CFG16
#define N_SPEC_LEVELS 128
#elif CFG8
#define N_SPEC_LEVELS 64
#else
#define N_SPEC_LEVELS 32
#endif
static int max_spec_level;

/* max number of threads */
#ifdef CFG64
#define N_THREAD_RECS 64 /* related arrays are statically allocated */
#elif CFG40
#define N_THREAD_RECS 40
#elif CFG24
#define N_THREAD_RECS 24
#elif CFG16
#define N_THREAD_RECS 16
#elif CFG8
#define N_THREAD_RECS 8
#else
#define N_THREAD_RECS 1
#endif
static int max_threads;

/* fork history depth */
#define THREADS_BMAP_SZ (BITMAP_SIZE(N_SPEC_LEVELS))

/* file to receive simulator output; defaults to stderr */
extern char *outfile_name;

/* pipeline trace range and output filename */
static int ptrace_nelt = 0;
static char *ptrace_opts[3];

/* overflow counts */
static SS_COUNTER_TYPE ruu_overflows;
static SS_COUNTER_TYPE lsq_overflows;
static SS_COUNTER_TYPE func_unit_overflows;
static SS_COUNTER_TYPE ifq_overflows;
static SS_COUNTER_TYPE num_in_flight_branch_overflows;

/* instruction fetch queue size (in insts) */
static int ruu_ifq_size;

/* extra branch mis-prediction latency */
static int ruu_branch_penalty;

/* branch predictor type {nottaken|taken|perfect|bimod|2lev} */
static char *pred_type;

/* perfect prediction enabled */
static int pred_perfect = FALSE;

/* bpred-target patching info */
static int fix_addrs;
static int fix_addrs_indir;
static int perfect_except_retstack;
int retstack_patch_level;

/* experimental: permit global history bits to be used with local history
 * bits */
static int bpred_merge_hist;         /* hash ghist w/ local hist */
static int bpred_merge_hist_shift;   /* hash shifted ghist w/ local hist */
static int bpred_cat_hist;           /* concat ghist w/ local hist */
static int bpred_gshare_shift;       /* hash shifted baddr w/ hist */
static int bpred_gshare_drop_lsbits; /* how many extra (beyond the automatic 3)
				      * low-order bits to drop from the baddr 
				      * when hashing the address with hist */

/* max in-flight branches allowed */
static int max_in_flight_branches;

/* speculative bpred-update enabled */
static int bpred_spec_update; /* all bpred updated, in writeback */

/* perfect-bpred-update enabled */
static int bpred_perf_update;

/* synthetic boosting of branch-prediction accuracy enabled */
static int pred_synth_up = FALSE;
static int pred_synth_down = FALSE;
static double pred_synth_up_threshold;
static double pred_synth_down_threshold;

/* bimodal predictor config (<counter_bits> <table_size> <retstack_size> <agree?>) */
static int bimod_nelt = 4;
static int bimod_config[4] =
    {/* counter bits */ 2, /* bimod tbl size */ 2048, /* retstack size */ 8,
     /* agree? */ 0};

/* 2-level predictor #1 config
 * (<counter_bits> <l1size> <l2size> <hist_size> <retstack_size> <gshare> <agreee>) */
static int twolev_nelt = 9;
static int twolev_config[9] =
    {/* cntr bits*/ 2, /* l1size */ 1, /* l2size */ 1024, /* hist */ 8,
     /* retstack size */ 8, /* gshare? */ 0, /* agree? */ 0,
     /* speculative history update */ 0, /* spec hist fixup type */ 1};

/* 2-level predictor #2 config 
 * (<counter_bits> <l1size> <l2size> <hist_size> <retstack_size> <gshare> <agree>)
 * (for hybrid predictor component #2) */
static int twolev2_nelt = 9;
static int twolev2_config[9] =
    {/* cntr bits */ 2, /* l1size */ 1024, /* l2size */ 1024, /* hist */ 8,
     /* retstack size */ 0, /* gshare? */ 0, /* agree? */ 0,
     /* speculative history update */ 0, /* spec history fixup type */ 1};

/* hybrid predictor config 
 * (<counter_bits>:<pred-pred sz>:<pred type 1>:<pred type 2>:<global shreg sz>:<retstack sz>:<gshare>:<spec history update?>:<spec history fixup type>)
 */
static char *hybrid_config;

/* BTB predictor config (<num_sets> <associativity>) */
static int btb_nelt = 2;
static int btb_config[2] =
    {/* nsets */ 512, /* assoc */ 4};

/* ret-addr stack size 
 * By default, the ret-addr stack size is specified with -bpred:retstack and
 * overrides the size specified in the PHT configuration (eg bimod_cfg).
 * Turning on "old_style_retstack_interface" uses the value from the PHT
 * config, overriding the -bpred:retstack specification */
static int retstack_size;
static int old_style_retstack_interface;

/* does each thread get a private copy of its own ret-addr stack? */
typedef enum
{
  OneStack = 0,     /* one, global, unified stack */
  OneStackPredOnly, /* one global stack, but only pred-path may access */
  PerThreadStacks,  /* per-thread copies of the whole stack */
  PerThreadTOSP,    /* one global stack but with per-thread TOS pointers */
  NUM_PER_THREAD_RETSTACK_OPTIONS
} per_thread_retstack_enum;
static int per_thread_retstack;

/* branch-confidence-predictor */
struct bconf *bconf = NULL;
static int bconf_type;
static int bconf_th_selector;
static int bconf_table_size;
static int bconf_table_width;
static int bconf_gshare;
static int bconf_history_update;
static int output_bconf_dist;
static char *bconf_config_file;
static int output_cbr_hist;
static int output_cbr_acc;
static int output_cbr_dist;
static FILE *cbr_hist_file;
static FILE *cbr_acc_file;

/* how many 1's or 0's needed to trigger Hi or Lo conf prediction */
#define MAX_BCONF_THRESHOLDS 3
static int bconf_num_thresholds = 1;
static int bconf_thresholds[MAX_BCONF_THRESHOLDS] = {7, 6, 5};

/* speculatively update conf predictor? */
static int bconf_spec_update;

/* perfectly update conf predictor? */
static int bconf_perf_update;

/* options for patching bconf/forking to isolate different behaviors */
static int bconf_squash_extra; /* squash forks for correct pred's */
static int bconf_fork_mispred; /* fork if possible for incorrect pred's */

/* forking options */
static int ruu_fork_penalty;
static int fork_in_fetch;
static int approx_fork_in_fetch;
static int penalize_fork_nt;
static int fork_prune;

/* fetch policy for multi-threaded/multi-path */
typedef enum
{
  Simple_RR = 0, /* naive round-robin; random thr gets "left-over" I$ bw*/
  Old_RR,        /* old-naive-rr: more I$ bw/thread; some don't fetch */
  Pred_RR,       /* thread on pred path gets left-over I$ bw */
  Omni_Pri,      /* omniscient priority-based: favors corr. path. NYI */
  Two_Omni_Pri,  /* temporary: simple omni pri scheme f/ 2 threads */
  Pred_Pri,      /* give priority to predicted path */
  Pred_Pri2,     /* new version of predicted-path priority */
  Ruu_Pri,       /* give priority in order of least RUU occupancy */
  FETCH_PRI_NUM
} fetch_pri_type_enum;
int fetch_pri_pol; /* will contain user-specified policy */
int two_pri_lev;   /* will contain pri-level for correct thread under
			 *    Two_Omni_Pri */
int pred_pri_lev;  /* will contain pri-level for pred path */
int ruu_pri_lev;   /* will contain pri-level for ruu path */

/* shortcut variables */
static int fetch_pred_pri;
static int fetch_ruu_pri;

/* number of cache lines to fetch per cycle */
static int fetch_cache_lines;
/* max number of lines to fetch per cycle for a given thread (zero: no limit)*/
static int max_cache_lines;

/* instruction decode B/W (insts/cycle) */
static int ruu_decode_width;

/* instruction issue B/W (insts/cycle) */
static int ruu_issue_width;
static int ruu_int_issue_width;
static int ruu_fp_issue_width;

/* instruction commit B/W (insts/cycle) */
static int ruu_commit_width;

/* extra decode latency -- accounts for decode/queueing that takes
 * more than 1 cycle (for example, some machines devote a full cycle to 
 * register renaming */
static int extra_decode_lat;

/* extra issue latency -- accounts for reading registers */
static int extra_issue_lat;

/* aggressive issue gives priority to mem ops, branches, and long-lat ops */
static int aggressive_issue;

/* run pipeline with in-order issue */
static int ruu_inorder_issue;

/* issue instructions down wrong execution paths */
static int ruu_include_spec = TRUE;

/* register update unit (RUU) size */
static int RUU_size;

/* load/store queue (LSQ) size */
static int LSQ_size;

/* integer issue queue (IIQ) size */
static int IIQ_size;

/* FP issue queue (FIQ) size */
static int FIQ_size;

/* for squashing: remove instruction from RUU, or leave it for commit
 * to discard? */
static int squash_remove;

/* l1 data cache config, i.e., {<config>|none} */
static char *cache_dl1_opt;
static int cache_dl1_perfect;
static int dl1_access_mem = FALSE;

/* l1 data cache hit latency (in cycles) */
static int cache_dl1_lat_nelt = 2;
static int cache_dl1_lat[2] =
    {/* base access lat */ 1, /* extra lat for hits */ 1};

/* l1 data cache ports -- we only have to worry about this for
 * the L1 D$.  The L1 I$ only needs to be single-ported, and if
 * off-chip is presumed to still provide one cache line every cycle. */
static int cache_dl1_ports;
static int cache_dl1_ports_used;
static int cache_dl1_ports_reserved;

/* l2 data cache config, i.e., {<config>|none} */
static char *cache_dl2_opt;
static int cache_dl2_perfect;

/* l2 data cache hit latency (in cycles) */
static int cache_dl2_lat_nelt = 2;
static int cache_dl2_lat[2] =
    {/* base access lat */ 2, /* extra lat for hits */ 1};

/* l1 instruction cache config, i.e., {<config>|dl1|dl2|none} */
static char *cache_il1_opt;
static int cache_il1_perfect;
static int il1_access_mem = FALSE;
static SS_ADDR_TYPE cache_il1_blkshift = 31;
static int user_il1_blksize;
static int MaxFetch = 0;
static int fetch_unit_size; /* max insts from a cache line that can be *
			     * fetched as one unit */

/* l1 instruction cache hit latency (in cycles) */
static int cache_il1_lat_nelt = 2;
static int cache_il1_lat[2] =
    {/* base access lat */ 1, /* extra lat for hits */ 1};

/* l2 instruction cache config, i.e., {<config>|dl1|dl2|none} */
static char *cache_il2_opt;
static int cache_il2_perfect;

/* l2 instruction cache hit latency (in cycles) */
static int cache_il2_lat_nelt = 2;
static int cache_il2_lat[2] =
    {/* base access lat */ 2, /* extra lat for hits */ 1};

/* flush caches on system calls */
int flush_on_syscalls;

/* convert 64-bit inst addresses to 32-bit inst equivalents */
int compress_icache_addrs;

/* memory access latency (<first_chunk> <inter_chunk>) */
static int mem_nelt = 2;
static int mem_lat[2] =
    {/* lat to first chunk */ 18, /* lat between remaining chunks */ 2};

/* memory access bus width (in bytes) */
static int mem_bus_width;

/* instruction TLB config, i.e., {<config>|none} */
static char *itlb_opt;

/* data TLB config, i.e., {<config>|none} */
static char *dtlb_opt;

/* Are tlb's perfect? */
static int tlb_perfect;

/* inst/data TLB miss latency (in cycles) */
static int tlb_miss_lat;

/* total number of integer ALU's available */
static int res_ialu;

/* total number of integer shift / branch units available */
static int res_ibrsh;

/* total number of integer multiplier/dividers available */
static int res_imult;

/* total number of memory system ports available (to CPU) */
static int res_ldport;
static int res_stport;

/* total number of floating point ALU's available */
static int res_fpalu;

/* total number of floating point multipliers available */
static int res_fpmult;

/* total number of floating point divider/square-rooters available */
static int res_fpdiv;

/* number of committed instructions for which to warmup caches */
static unsigned int num_warmup_insn;

/* number of committed instructions for which to prime state */
static unsigned int num_prime_insn;

/* number of committed instructions for which to simulate after priming */
static unsigned int num_fullsim_insn;

/* reporting options */
static int report_fetch = FALSE;
static int report_issue = FALSE;
static int report_commit = FALSE;
static int report_post_issue = FALSE;
static int report_useful_insts = FALSE;
static int report_ready_insts = FALSE;
static int report_miss_indep_insts = FALSE;
static int report_ruu_occ = FALSE;
static int report_imiss_ruu_occ = FALSE;
static int report_branch_info = FALSE;
static int report_issue_loc = FALSE;
static int report_decode_loc = FALSE;
static int report_issue_delay = FALSE;
static int report_miss_clustering = FALSE;
static struct stat_stat_t *fetch_dist = NULL;
static struct stat_stat_t *issue_dist = NULL;
static struct stat_stat_t *useful_issue_dist = NULL;
static struct stat_stat_t *commit_dist = NULL;
static struct stat_stat_t *post_issue_dist = NULL;
static struct stat_stat_t *post_issue_useful_dist = NULL;
static struct stat_stat_t *useful_insts_dist = NULL;
static struct stat_stat_t *ready_insts_dist = NULL;
static struct stat_stat_t *miss_indep_insts_dist = NULL;
static struct stat_stat_t *ruu_occ_dist = NULL;
static struct stat_stat_t *imiss_ruu_occ_dist = NULL;
static struct stat_stat_t *pending_branches_dist = NULL;
static struct stat_stat_t *branch_delay_dist = NULL;
static struct stat_stat_t *issue_loc_dist = NULL;
static struct stat_stat_t *decode_loc_dist = NULL;
static struct stat_stat_t *issue_delay_dist = NULL;
static struct stat_stat_t *operand_delay_dist = NULL;
static struct stat_stat_t *d1miss_cluster_dist = NULL;
static struct stat_stat_t *i1miss_cluster_dist = NULL;
static struct stat_stat_t *cbr_data_dist = NULL;

extern struct stat_stat_t *bconf_correct_dist;
extern struct stat_stat_t *bconf_incorrect_dist;

/* Dummy vars */
static int dummy_flag = FALSE;
static int dummy_int = 0;
static char *dummy_sz = NULL;

/* text-based stat profiles */
#define MAX_PCSTAT_VARS 8
static int pcstat_nelt = 0;
static char *pcstat_vars[MAX_PCSTAT_VARS];

/* operate in backward-compatible bugs mode (for testing only) */
static int bugcompat_mode;

/***
 * SIMULATOR STATE
 ***/

/*
 * functional unit resource configuration
 */

/* resource pool indices, NOTE: update these if you change FU_CONFIG */
/* Also update fu_oplat_arr in init() if you change these */
#define FU_IALU_INDEX 0
#define FU_IBRSH_INDEX 1
#define FU_IMULT_INDEX 2
#define FU_LDPORT_INDEX 3
#define FU_STPORT_INDEX 4
#define FU_FPALU_INDEX 5
#define FU_FPMULT_INDEX 6
#define FU_FPDIV_INDEX 7
#define FU_NUM_ITEMS 8

/* resource pool definition, NOTE: update FU_*_INDEX defs if you change this */
/* Also update fu_oplat_arr in init() if you change these */
struct res_desc fu_config[] = {
    {"integer-ALU",
     4,
     0,
     {{IntALU, 1, 1}}},
    {"integer-shift-branch",
     4,
     0,
     {{Branch, 1, 1},
      {IntSHIFT, 1, 1}}},
    {"integer-MULT/DIV",
     1,
     0,
     {
#ifdef P6
         {IntMULT, 4, 1},
         {IntDIV, 12, 12}
#else
         {IntMULT, 12, 8},
         {IntDIV, 20, 19}
#endif
     }},
    /* FIXME: If mem-port issuelat > 1, dl1-port bookkeeping breaks */
    {
        "memory-load-port",
        2,
        0,
        {{RdPort, 1, 1}}},
    {"memory-store-port",
     1,
     0,
     {{WrPort, 1, 1}}},
    {"FP-adder",
     4,
     0,
     {
#ifdef P6
         {FloatADD, 3, 1},
         {FloatCMP, 3, 1},
         {FloatCVT, 3, 1}
#else
         {FloatADD, 4, 1},
         {FloatCMP, 4, 1},
         {FloatCVT, 4, 1}
#endif
     }},
    {"FP-MULT",
     2,
     0,
     {
#ifdef P6
         {FloatMULT, 5, 2}
#else
         {FloatMULT, 4, 1}
#endif
     }},
    {"FP-DIV/SQRT",
     1,
     0,
     {
#ifdef P6
         {FloatDIV, 18, 18},
         {FloatSQRT, 29, 29}
#else
         {FloatDIV, 16, 16},
         {FloatSQRT, 33, 33}
#endif
     }}};

static int fu_oplat_arr[NUM_FU_CLASSES];
int infinite_fu;

/*
 * simulator stats
 */

/* total number of instructions committed */
SS_COUNTER_TYPE sim_num_insn = 0;
static SS_COUNTER_TYPE primed_insts = 0;

/* total number of instructions executed */
static SS_COUNTER_TYPE sim_total_insn = 0;

/* total number of instructions fetched */
static SS_COUNTER_TYPE sim_total_fetched = 0;

/* total number of memory references committed */
SS_COUNTER_TYPE sim_num_refs = 0;
static SS_COUNTER_TYPE primed_refs = 0;

/* total number of memory references executed */
static SS_COUNTER_TYPE sim_total_refs = 0;

/* total number of loads committed */
SS_COUNTER_TYPE sim_num_loads = 0;
static SS_COUNTER_TYPE primed_loads = 0;

/* total number of loads executed */
static SS_COUNTER_TYPE sim_total_loads = 0;

/* total number of branches committed */
static SS_COUNTER_TYPE sim_num_branches = 0;
static SS_COUNTER_TYPE sim_num_cond_branches = 0;

/* total number of branches executed */
static SS_COUNTER_TYPE sim_total_branches = 0;
static SS_COUNTER_TYPE sim_total_cond_branches = 0;

/* total number of LSQ-fulfilled loads */
static SS_COUNTER_TYPE lsq_hits = 0;

/* cycle counter */
static SS_TIME_TYPE sim_cycle = 0;

/* Priming info */
int done_priming = FALSE;
static SS_TIME_TYPE primed_cycles = 0;

/* forking stats */
static SS_COUNTER_TYPE num_thread_context_overflows;
static SS_COUNTER_TYPE num_forked;
static SS_COUNTER_TYPE total_forked;
static SS_COUNTER_TYPE total_bad_forks_squashed;
static SS_COUNTER_TYPE total_extra_forked;
static SS_COUNTER_TYPE total_forks_pruned;
static SS_COUNTER_TYPE num_unforked_good_lo;
static SS_COUNTER_TYPE num_unforked_bad_hi;
static SS_COUNTER_TYPE total_unforked_good_lo;
static SS_COUNTER_TYPE total_unforked_bad_hi;
static SS_COUNTER_TYPE num_fork_opps;
static SS_COUNTER_TYPE total_fork_opps;
static SS_COUNTER_TYPE num_forked_wasted;
static SS_COUNTER_TYPE num_forked_good;
static SS_COUNTER_TYPE num_unforked_good;
static SS_COUNTER_TYPE num_unforked_bad;
static SS_COUNTER_TYPE total_forked_wasted;
static SS_COUNTER_TYPE total_forked_good;
static SS_COUNTER_TYPE total_unforked_good;
static SS_COUNTER_TYPE total_unforked_bad;
static SS_COUNTER_TYPE num_failed_omni_forks;
static SS_COUNTER_TYPE num_fork_dispatch_resource_inaccuracies;
static SS_COUNTER_TYPE num_fork_dispatch_timing_inaccuracies;
static SS_COUNTER_TYPE total_forks_squashed_pre_decode;
static struct stat_stat_t *forked_dist = NULL;

/* clustering state */
static SS_TIME_TYPE last_imiss_cycle = 0;
static SS_TIME_TYPE last_dmiss_cycle = 0;

/*
 * simulator state variables
 */

/* multi-path/multi-threading state vars */
#define INIT_THREAD 0
static BITMAP_ENT_TYPE fork_hist_bmap_head = 0;
static int num_active_forks = 0;
static int num_zombie_forks = 0;
static int last_thread_fetched = N_THREAD_RECS - 1;

/* current integer-issue-queue (IIQ) occupancy */
static int IIQ_occ = 0;

/* current FP-issue-queue (FIQ) occupancy */
static int FIQ_occ = 0;

/* instruction sequence counter, used to assign unique id's to insts */
static unsigned int inst_seq = 0;

/* pipetrace instruction sequence counter */
static unsigned int ptrace_seq = 0;

/* level 1 instruction cache, entry level instruction cache */
struct cache *cache_il1;

/* level 1 instruction cache */
struct cache *cache_il2;

/* level 1 data cache, entry level data cache */
struct cache *cache_dl1;

/* level 2 data cache */
struct cache *cache_dl2;

/* instruction TLB */
struct cache *itlb;

/* data TLB */
struct cache *dtlb;

/* branch predictor */
struct bpred *pred;

/* functional unit resource pool */
static struct res_pool *fu_pool = NULL;

/* text-based stat profiles */
static struct stat_stat_t *pcstat_stats[MAX_PCSTAT_VARS];
static SS_COUNTER_TYPE pcstat_lastvals[MAX_PCSTAT_VARS];
static struct stat_stat_t *pcstat_sdists[MAX_PCSTAT_VARS];

/* wedge all stat values into a SS_COUNTER_TYPE */
#define STATVAL(STAT)                                              \
  ((STAT)->sc == sc_int                                            \
       ? (SS_COUNTER_TYPE) * ((STAT)->variant.for_int.var)         \
       : ((STAT)->sc == sc_uint                                    \
              ? (SS_COUNTER_TYPE) * ((STAT)->variant.for_uint.var) \
              : ((STAT)->sc == sc_counter                          \
                     ? *((STAT)->variant.for_counter.var)          \
                     : (panic("bad stat class"), 0))))

/* memory access latency, assumed to not cross a page boundary */
static unsigned int            /* total latency of access */
mem_access_latency(int blk_sz) /* block size accessed */
{
  int chunks = (blk_sz + (mem_bus_width - 1)) / mem_bus_width;

  assert(chunks > 0);

  return (/* first chunk latency */ mem_lat[0] +
          (/* remainder chunk latency */ mem_lat[1] * (chunks - 1)));
}

/*
 * cache miss handlers
 */

/* l1 data cache l1 block miss handler function */
static unsigned int                  /* latency of block access */
dl1_access_fn(enum mem_cmd cmd,      /* access cmd, Read or Write */
              SS_ADDR_TYPE baddr,    /* block address to access */
              int bsize,             /* size of block to access */
              struct cache_blk *blk, /* ptr to block in upper level */
              SS_TIME_TYPE now)      /* time of access */
{
  unsigned int lat;

  if (cache_dl2)
  {
    /* access next level of data cache hierarchy */
    if (cache_dl2_perfect)
      lat = cache_dl2_lat[0] + cache_dl2_lat[1];
    else
      lat = cache_access(cache_dl2, cmd, baddr, NULL, bsize,
                         /* now */ now, /* pudata */ NULL, /* repl addr */ NULL);

    if (cmd == Read)
      return lat;
    else
    {
      /* FIXME: unlimited write buffers */
      return 1;
    }
  }
  else
  {
    /* access main memory */
    if (cmd == Read)
      return mem_access_latency(bsize);
    else
    {
      /* FIXME: unlimited write buffers */
      return 1;
    }
  }
}

/* l2 data cache block miss handler function */
static unsigned int                  /* latency of block access */
dl2_access_fn(enum mem_cmd cmd,      /* access cmd, Read or Write */
              SS_ADDR_TYPE baddr,    /* block address to access */
              int bsize,             /* size of block to access */
              struct cache_blk *blk, /* ptr to block in upper level */
              SS_TIME_TYPE now)      /* time of access */
{
  /* this is a miss to the lowest level, so access main memory */
  if (cmd == Read)
    return mem_access_latency(bsize);
  else
  {
    /* FIXME: unlimited write buffers */
    return 1;
  }
}

/* l1 inst cache l1 block miss handler function */
static unsigned int                  /* latency of block access */
il1_access_fn(enum mem_cmd cmd,      /* access cmd, Read or Write */
              SS_ADDR_TYPE baddr,    /* block address to access */
              int bsize,             /* size of block to access */
              struct cache_blk *blk, /* ptr to block in upper level */
              SS_TIME_TYPE now)      /* time of access */
{
  unsigned int lat;

  if (cache_il2)
  {
    /* if L2 I$ is also the L1 D$, check for available port 
       * FIXME: this only works if L1 I$ is single-cycle */
    if (cache_il2 == cache_dl1)
      cache_dl1_ports_reserved++;

    /* access next level of inst cache hierarchy */
    if (cache_il2_perfect)
      lat = cache_il2_lat[0] + cache_il2_lat[1];
    else
      lat = cache_access(cache_il2, cmd, baddr, NULL, bsize,
                         /* now */ now, /* pudata */ NULL, /* repl addr */ NULL);

    if (cmd == Read)
      return lat;
    else
      panic("writes to instruction memory not supported");
  }
  else
  {
    /* access main memory */
    if (cmd == Read)
      return mem_access_latency(bsize);
    else
      panic("writes to instruction memory not supported");
  }
}

/* l2 inst cache block miss handler function */
static unsigned int                  /* latency of block access */
il2_access_fn(enum mem_cmd cmd,      /* access cmd, Read or Write */
              SS_ADDR_TYPE baddr,    /* block address to access */
              int bsize,             /* size of block to access */
              struct cache_blk *blk, /* ptr to block in upper level */
              SS_TIME_TYPE now)      /* time of access */
{
  /* this is a miss to the lowest level, so access main memory */
  if (cmd == Read)
    return mem_access_latency(bsize);
  else
    panic("writes to instruction memory not supported");
}

/*
 * TLB miss handlers
 */

/* inst cache block miss handler function */
static unsigned int                   /* latency of block access */
itlb_access_fn(enum mem_cmd cmd,      /* access cmd, Read or Write */
               SS_ADDR_TYPE baddr,    /* block address to access */
               int bsize,             /* size of block to access */
               struct cache_blk *blk, /* ptr to block in upper level */
               SS_TIME_TYPE now)      /* time of access */
{
  SS_ADDR_TYPE *phy_page_ptr = (SS_ADDR_TYPE *)blk->user_data;

  /* no real memory access, however, should have user data space attached */
  assert(phy_page_ptr);

  /* fake translation, for now... */
  *phy_page_ptr = 0;

  /* return tlb miss latency */
  return tlb_miss_lat;
}

/* data cache block miss handler function */
static unsigned int                   /* latency of block access */
dtlb_access_fn(enum mem_cmd cmd,      /* access cmd, Read or Write */
               SS_ADDR_TYPE baddr,    /* block address to access */
               int bsize,             /* size of block to access */
               struct cache_blk *blk, /* ptr to block in upper level */
               SS_TIME_TYPE now)      /* time of access */
{
  SS_ADDR_TYPE *phy_page_ptr = (SS_ADDR_TYPE *)blk->user_data;

  /* no real memory access, however, should have user data space attached */
  assert(phy_page_ptr);

  /* fake translation, for now... */
  *phy_page_ptr = 0;

  /* return tlb miss latency */
  return tlb_miss_lat;
}

/* OPTIONS
 * register simulator-specific options 
 */
void sim_reg_options(struct opt_odb_t *odb)
{
  static char *bcf_emap[BCF_NUM] = {"none", "naive", "omni", "ones",
                                    "sat", "reset", "pattern"};
  static char *bts_emap[BTS_NUM] = {"none", "profile", "hw"};
  static char *fetch_pri_emap[FETCH_PRI_NUM] = {"simple_rr",
                                                "old_rr", "pred_rr", "omni_pri", "two_omni_pri", "pred_pri", "pred_pri2",
                                                "ruu_pri"};
  static char *per_thread_retstack_emap[NUM_PER_THREAD_RETSTACK_OPTIONS] =
      {"one_xxxx", "one_pred", "per", "tosp_per"};

  opt_reg_header(odb,
                 "sim-outorder: This simulator implements a very detailed out-of-order issue\n"
                 "superscalar processor with a two-level memory system and speculative\n"
                 "execution support.  This simulator is a performance simulator, tracking the\n"
                 "latency of all pipeline operations.\n");

  /* trace options */

  opt_reg_string_list(odb, "-ptrace",
                      "generate pipetrace, i.e., <level> <fname|stdout|stderr> <range>",
                      ptrace_opts, /* arr_sz */ 3, &ptrace_nelt, /* default */ NULL,
                      /* !print */ FALSE, /* format */ NULL, /* !accrue */ FALSE);

  opt_reg_note(odb,
               "  Pipetrace range arguments are formatted as follows:\n"
               "\n"
               "    {{@|#}<start>}:{{@|#|+}<end>}\n"
               "\n"
               "  Both ends of the range are optional, if neither are specified, the entire\n"
               "  execution is traced.  Ranges that start with a `@' designate an address\n"
               "  range to be traced, those that start with an `#' designate a cycle count\n"
               "  range.  All other range values represent an instruction count range.  The\n"
               "  second argument, if specified with a `+', indicates a value relative\n"
               "  to the first argument, e.g., 1000:+100 == 1000:1100.  Program symbols may\n"
               "  be used in all contexts.\n"
               "\n"
               "  The level argument controls how much information the trace includes:\n"
               "  	0: off (default)\n"
               "	1: on, simple mode (just instructions' flow through pipeline)\n"
               "	2: on, simple, plus memory info\n"
               "	3: (not used)\n"
               "	4: on, verbose: simple + mem + the value of each instruction's\n"
               "	   register operands and the result written back\n"
               "	5: on, verbose, but only shows the functional simulation (ie only\n"
               "	   shows correctly-speculated instructions in ruu_dispatch())\n"
               "  Memory info prints each load or store's address, and the value of the\n"
               "  4-byte word at that address after the load/store has executed.\n"
               "\n"
               "    Examples:   -ptrace 1 FOO.trc #0:#1000\n"
               "                -ptrace 1 BAR.trc @2000:\n"
               "                -ptrace 1 BLAH.trc :1500\n"
               "                -ptrace 1 UXXE.trc :\n"
               "                -ptrace 3 FOOBAR.trc @main:+278\n");

  /* multi-path/multi-threading options */
  opt_reg_int(sim_odb, "-threads:max",
              "max number of threads to support",
              &max_threads, /* default */ 1, /* print */ TRUE, NULL);

  opt_reg_flag(odb, "-fork:in_fetch", "do forking in fetch stage",
               &fork_in_fetch, /* deafult */ TRUE, TRUE, NULL);

  opt_reg_int(odb, "-fork:lat", "penalty charged to forked-off path",
              &ruu_fork_penalty, /* default */ 3, TRUE, NULL);

  opt_reg_flag(odb, "-fork:approx_in_fetch",
               "if fork-in-dispatch, try to approx fork-in-fetch",
               &approx_fork_in_fetch, /* default */ FALSE, TRUE, NULL);

  opt_reg_flag(odb, "-fork:penalize_fork_nt",
               "if fork-in-dispatch, charge mplat against fork down not-taken path",
               &penalize_fork_nt, /* default */ TRUE, TRUE, NULL);

  opt_reg_flag(odb, "-fork:prune",
               "only fork on predicted path",
               &fork_prune, /* default */ FALSE, TRUE, NULL);

  /* ifetch options */

  opt_reg_int(odb, "-fetch:num_cache_lines",
              "max no. cache lines to fetch in one cycle",
              &fetch_cache_lines, /* default */ 1, TRUE, NULL);

  opt_reg_int(odb, "-fetch:max_lines_per_thread",
              "max no. cache lines to fetch in one cycle (zero: no limit)",
              &max_cache_lines, /* default */ 0, TRUE, NULL);

  opt_reg_int(odb, "-fetch:max_fetchable_per_line",
              "max no. insts from a cache line that can be fetched as one unit",
              &fetch_unit_size, /* default */ 4, TRUE, NULL);

  opt_reg_int(odb, "-fetch:ifqsize", "instruction fetch queue size (in insts)",
              &ruu_ifq_size, /* default */ 4,
              /* print */ TRUE, /* format */ NULL);

  opt_reg_int(odb, "-fetch:mplat", "extra branch mis-prediction latency",
              &ruu_branch_penalty, /* default */ 3,
              /* print */ TRUE, /* format */ NULL);

  opt_reg_int(odb, "-fetch:max_in_flight_branches",
              "max number of in-flight branches allowed per_thread (0 = no limit)",
              &max_in_flight_branches, /* default */ 4,
              /* print */ TRUE, /* format */ NULL);

  opt_reg_enum(odb, "-fetch:pri_pol",
               "fetch priority policy (simple_rr|old_rr|pred_rr|omni_pri|two_omni_pri|pred_pri|pred_pri2|ruu_pri)",
               &fetch_pri_pol, "simple_rr", fetch_pri_emap, NULL,
               FETCH_PRI_NUM, TRUE, NULL);

  opt_reg_int(odb, "-fetch:two_pri_lev",
              "pri lev for corr. thread under two_omni_pri",
              &two_pri_lev, /* default */ 2,
              /* print */ TRUE, /* format */ NULL);

  opt_reg_int(odb, "-fetch:pred_pri_lev",
              "pri lev for pred path under pred_pri",
              &pred_pri_lev, /* default */ 2,
              /* print */ TRUE, /* format */ NULL);

  opt_reg_int(odb, "-fetch:ruu_pri_lev",
              "pri lev for least-insts-in-ruu under ruu_pri",
              &ruu_pri_lev, /* default */ 2,
              /* print */ TRUE, /* format */ NULL);

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
               "  Predictor `hybrid' combines two non-perfect predictors.\n");

  opt_reg_string(odb, "-bpred",
                 "branch predictor type "
                 "{nottaken|taken|perfect|bimod|2lev|hybrid}",
                 &pred_type, /* default */ "bimod",
                 /* print */ TRUE, /* format */ NULL);

  opt_reg_int_list(odb, "-bpred:bimod",
                   "bimodal pred cfg (<cntr_bits> <tbl_size> <retstack_size>"
                   " <agree?>)",
                   bimod_config, bimod_nelt, &bimod_nelt, bimod_config,
                   /* print */ TRUE, /* format */ NULL, /* !accrue */ FALSE);

  opt_reg_int_list(odb, "-bpred:2lev",
                   "2lev pred cfg (<cntr_bits> <l1_sz> <l2_sz> <hist_sz> "
                   "<retstack_size> <gshare?> <agree?> <spec-update?> "
                   "<spec-update repair?>)",
                   twolev_config, twolev_nelt, &twolev_nelt, twolev_config,
                   /* print */ TRUE, /* format */ NULL, /* !accrue */ FALSE);

  opt_reg_int_list(odb, "-bpred:2lev2",
                   "2lev pred cfg #2 (<cntr_bits> <l1_sz> <l2_sz> <hist_sz> "
                   "<retstack_size> <gshare?> <agree?> <spec-update?> "
                   "<spec-update repair?>)",
                   twolev2_config, twolev2_nelt, &twolev2_nelt, twolev2_config,
                   /* print */ TRUE, /* format */ NULL, /* !accrue */ FALSE);

  opt_reg_string(odb, "-bpred:hybrid",
                 "hybrid predictor config (<cntr_bits>:<tbl-sz>:<pred1>:<pred2>:<sh-reg-sz>:<retstack_sz>:<gshare?>:<spec-update?>:<spec-update repair?>)",
                 &hybrid_config, "2:4096:bimod:2lev2:12:8:0:0:1",
                 /* print */ TRUE, NULL);

  opt_reg_int_list(odb, "-bpred:btb",
                   "BTB config (<num_sets> <associativity>)",
                   btb_config, btb_nelt, &btb_nelt, btb_config,
                   /* print */ TRUE, /* format */ NULL, /* !accrue */ FALSE);

  opt_reg_int(odb, "-bpred:retstack", "retstack size",
              &retstack_size, /* default */ 8, TRUE, NULL);

  opt_reg_flag(odb, "-bpred:use_old_retstack_interface",
               "use retstack size from the PHT config instead of from"
               " '-bpred:retstack'",
               &old_style_retstack_interface, /* default */ FALSE, TRUE, NULL);

  opt_reg_enum(odb, "-bpred:per_thread_retstack",
               "for multipath: what type of retstack?"
               " (one, one_pred, per, tosp_per)",
               &per_thread_retstack, /* default */ "one_xxxx",
               per_thread_retstack_emap, NULL, NUM_PER_THREAD_RETSTACK_OPTIONS,
               TRUE, NULL);

  opt_reg_int(odb, "-bpred:merge_hist",
              "hash global with local history bits; argument specifies"
              " global history size",
              &bpred_merge_hist, /* default */ FALSE, TRUE, NULL);

  opt_reg_int(odb, "-bpred:merge_hist_shift",
              "hash shifted global with local history bits; argument specifies"
              " global history size",
              &bpred_merge_hist_shift, /* default */ FALSE, TRUE, NULL);

  opt_reg_int(odb, "-bpred:cat_hist",
              "concatenate global and local history bits; argument specifies"
              " global history size",
              &bpred_cat_hist, /* default */ FALSE, TRUE, NULL);

  opt_reg_int(odb, "-bpred:gshare_shift",
              "hash shifted baddr with global history bits; argument specifies"
              " address left-shift",
              &bpred_gshare_shift, /* default */ 0, TRUE, NULL);

  opt_reg_int(odb, "-bpred:gshare_drop_lsbits",
              "number of low-order br-address bits (beyond the automatic 3)"
              " to drop for gshare",
              &bpred_gshare_drop_lsbits, /* default */ 0, TRUE, NULL);

  opt_reg_flag(odb, "-bpred:spec_update",
               "update bpred in writeback? (ie speculatively)",
               &bpred_spec_update, /* default */ FALSE,
               /* print */ TRUE, /* format */ NULL);

  opt_reg_flag(odb, "-bpred:perf_update",
               "update bpred perfectly in decode?",
               &bpred_perf_update, /* default */ FALSE,
               /* print */ TRUE, /* format */ NULL);

  opt_reg_double(odb, "-bpred:synthetic_boost",
                 "percentage of mispredictions to boost (as a fraction)",
                 &pred_synth_up_threshold, /* default */ 0.0,
                 TRUE, NULL);

  opt_reg_double(odb, "-bpred:synthetic_decrease",
                 "percentage of good predictions to make incorrect (as a fraction)",
                 &pred_synth_down_threshold, /* default */ 0.0,
                 TRUE, NULL);

  opt_reg_flag(odb, "-bpred:fix_addrs",
               "correct address prediction for br's w/ correct direction?",
               &fix_addrs, /* default */ FALSE, TRUE, NULL);

  opt_reg_flag(odb, "-bpred:fix_addrs_indir",
               "correct address prediction for indir jumps?",
               &fix_addrs_indir, /* default */ FALSE, TRUE, NULL);

  opt_reg_flag(odb, "-bpred:perf_except_retstack",
               "correct address prediction for indir jumps but "
               "not for retstack?",
               &perfect_except_retstack, /* default */ FALSE, TRUE, NULL);

  opt_reg_flag(odb, "-cbr:hist",
               "output cbr history bit-stream to a file?",
               &output_cbr_hist, /* default */ FALSE, TRUE, NULL);

  opt_reg_flag(odb, "-cbr:acc",
               "output cbr accuracy bit-stream to a file?",
               &output_cbr_acc, /* default */ FALSE, TRUE, NULL);

  opt_reg_flag(odb, "-cbr:dist",
               "output cbr distributions?",
               &output_cbr_dist, /* default */ FALSE, TRUE, NULL);

  opt_reg_int(odb, "-bpred:retstack_patch_level",
              "0: no patching, 1: patch TOS pointer only, 2: also patch"
              " TOS contents",
              &retstack_patch_level, /* default */ 2, TRUE, NULL);

  opt_reg_enum(odb, "-bconf",
               "bconf predictor type (naive|omni|ones|sat|reset|pattern)",
               &bconf_type, "naive", bcf_emap, NULL, BCF_NUM, TRUE, NULL);

  opt_reg_enum(odb, "-bconf:selector",
               "bconf predictor threshold selector (none|profile|hw)",
               &bconf_th_selector, "none", bts_emap, NULL, BTS_NUM,
               TRUE, NULL);

  opt_reg_flag(odb, "-bconf:spec_update",
               "speculatively update the branch confidence predictor?",
               &bconf_spec_update, FALSE, TRUE, NULL);

  opt_reg_flag(odb, "-bconf:perf_update",
               "update bconf perfectly in decode?",
               &bconf_perf_update, /* default */ FALSE,
               /* print */ TRUE, /* format */ NULL);

  opt_reg_int_list(odb, "-bconf:thresholds",
                   "thresholds for the bconf predictor"
                   " (<t1> <t2> <t3>)",
                   bconf_thresholds, MAX_BCONF_THRESHOLDS,
                   &bconf_num_thresholds, bconf_thresholds,
                   /* print */ TRUE, /* format */ NULL, /* !accrue */ FALSE);

  opt_reg_int(odb, "-bconf:table_size",
              "number of entries in the branch confidence prediction table",
              &bconf_table_size, /* default */ 1024,
              /* print */ TRUE, /* format */ NULL);

  opt_reg_int(odb, "-bconf:table_width",
              "number of bits per entry in the branch confidence prediction table",
              &bconf_table_width, /* default */ 8,
              /* print */ TRUE, /* format */ NULL);

  opt_reg_int(odb, "-bconf:gshare",
              "xor global history bits with branch-address bits?",
              &bconf_gshare, /* default */ 1, /* print */ TRUE, NULL);

  opt_reg_flag(odb, "-bconf:dist",
               "output bconf correct/incorrect patterns?",
               &output_bconf_dist, /* default */ FALSE, TRUE, NULL);

  opt_reg_flag(odb, "-bconf:squash_extra",
               "squash forks for correctly-pred branches?",
               &bconf_squash_extra, /* default */ FALSE, /* print */ TRUE, NULL);

  opt_reg_flag(odb, "-bconf:fork_mispred",
               "fork (if possible) on any misprediction?",
               &bconf_fork_mispred, /* default */ FALSE, /* print */ TRUE, NULL);

  opt_reg_int(odb, "-bconf:hist_update",
              "use bconf history to update bconf table entries?",
              &bconf_history_update, /* default */ 0, /* print */ TRUE, NULL);

  opt_reg_string(odb, "-bconf:config_file",
                 "bconf configuration file name",
                 &bconf_config_file, /* default */ NULL,
                 /* print */ TRUE, /* format */ NULL);

  /* decode options */

  opt_reg_int(odb, "-decode:width",
              "instruction decode B/W (insts/cycle)",
              &ruu_decode_width, /* default */ 4,
              /* print */ TRUE, /* format */ NULL);

#if 0
  opt_reg_note(odb, 
"  -decode:num_cache_lines, if greater than one, will fetch multiple cache\n"
"  lines in a given cycle even if this means fetching through a predicted-\n"
"  taken branch.  If a predicted-taken branch jumps to the same cache line\n"
"  as the branch, the cache-line is refetched and counted as another line,\n"
"  just as though the branch had jumped to a new cache line.\n"
"  Note that perfect branch prediction works correctly only when\n"
"  -decode:num_cache_lines == 1; perfect prediction always fetches\n"
"  sequentially, so values greater than one just fetch that many sequential\n"
"  cache lines.  This is not a problem with non-perfect prediction.\n"
	       );
#endif
  opt_reg_int(odb, "-decode:extra_lat",
              "extra decode latency",
              &extra_decode_lat, /* default */ 0,
              /* print */ TRUE, /* format */ NULL);

  /* issue options */

  opt_reg_int(odb, "-issue:width",
              "instruction issue B/W (insts/cycle)",
              &ruu_issue_width, /* default */ 4,
              /* print */ TRUE, /* format */ NULL);

  opt_reg_int(odb, "-issue:int_width",
              "int instruction issue B/W (insts/cycle)",
              &ruu_int_issue_width, /* default */ 4,
              /* print */ TRUE, /* format */ NULL);

  opt_reg_int(odb, "-issue:fp_width",
              "fp instruction issue B/W (insts/cycle)",
              &ruu_fp_issue_width, /* default */ 4,
              /* print */ TRUE, /* format */ NULL);

  opt_reg_note(odb,
               "  -issue:int_width and -issue:fp_width restrict operations that use\n"
               "  integer and fp functional units, respectively; total instructions issued\n"
               "  per cycle cannot exceed the general -issue:width parameter\n");

  opt_reg_flag(odb, "-issue:inorder", "run pipeline with in-order issue",
               &ruu_inorder_issue, /* default */ FALSE,
               /* print */ TRUE, /* format */ NULL);

  opt_reg_flag(odb, "-issue:wrongpath",
               "issue instructions down wrong execution paths",
               &ruu_include_spec, /* default */ TRUE,
               /* print */ TRUE, /* format */ NULL);

  opt_reg_int(odb, "-issue:extra_lat",
              "extra issue latency (for extra (eg register-read) stages)",
              &extra_issue_lat, /* default */ 1,
              /* print */ TRUE, /* format */ NULL);

  opt_reg_int(odb, "-issue:intq_size",
              "integer issue queue (IIQ) size",
              &IIQ_size, /* default */ 16,
              /* print */ TRUE, /* format */ NULL);

  opt_reg_int(odb, "-issue:fpq_size",
              "fp issue queue (FIQ) size",
              &FIQ_size, /* default */ 16,
              /* print */ TRUE, /* format */ NULL);

  opt_reg_flag(odb, "-issue:aggressive",
               "gives priority to mem, br, and long-lat ops",
               &aggressive_issue, /* default */ TRUE,
               /* print */ TRUE, /* format */ NULL);

  opt_reg_flag(odb, "-squash:remove",
               "remove RUU/LSQ entries squashed by mispredicts, "
               "or propagate them to commit?",
               &squash_remove, /* default */ TRUE,
               /* print */ TRUE, NULL);

  /* register scheduler options */

  opt_reg_int(odb, "-ruu:size",
              "register update unit (RUU) size",
              &RUU_size, /* default */ 16,
              /* print */ TRUE, /* format */ NULL);

  /* memory scheduler options  */

  opt_reg_int(odb, "-lsq:size",
              "load/store queue (LSQ) size",
              &LSQ_size, /* default */ 8,
              /* print */ TRUE, /* format */ NULL);

  /* commit options  */

  opt_reg_int(odb, "-commit:width",
              "instruction commit B/W (insts/cycle)",
              &ruu_commit_width, /* default */ 4,
              /* print */ TRUE, /* format */ NULL);

  /* cache options */

  opt_reg_string(odb, "-cache:dl1",
                 "l1 data cache config, i.e., {<config>|none}",
                 &cache_dl1_opt, "dl1:128:32:4:l:4:2",
                 /* print */ TRUE, NULL);

  opt_reg_note(odb,
               "  The cache config parameter <config> has the following format:\n"
               "\n"
               "    <name>:<nsets>:<bsize>:<assoc>:<repl>:<mshr's>:<bus interval>\n"
               "\n"
               "    <name>   - name of the cache being defined (can be \"none\" or \"mem\")\n"
               "    <nsets>  - number of sets in the cache\n"
               "    <bsize>  - block size of the cache\n"
               "    <assoc>  - associativity of the cache\n"
               "    <repl>   - block replacement strategy, 'l'-LRU, 'f'-FIFO, 'r'-random\n"
               "    <mshr's> - number of mshr's, ie number of primary misses outstanding\n"
               "               0 means in-cache MSHR structure\n"
               "    <bus interval> - number of cpu cycles per bus transaction\n"
               "  Note we don't specify mshr's or bus interval for TLB's.\n"
               "\n"
               "    Examples:   -cache:dl1 dl1:4096:32:1:l:4:2\n"
               "                -dtlb dtlb:128:4096:32:r\n");

  opt_reg_int_list(odb, "-cache:dl1lat",
                   "l1 data cache access latency (<base lat> <extra hit lat>)",
                   cache_dl1_lat, cache_dl1_lat_nelt, &cache_dl1_lat_nelt,
                   /* default */ cache_dl1_lat, /* print */ TRUE,
                   /* format */ NULL, /* accrue */ FALSE);

  opt_reg_note(odb,
               "  The cache latency parameter has the following format:\n"
               "  <base lat> <extra hit lat>\n"
               "  where 'base lat' is the tag access time for that cache, and\n"
               "  'extra hit lat' is any extra time before data is available.\n"
               "  For example, a small, direct-mapped cache might supply (1, 0), while\n"
               "  a large, 4-way cache might supply (2, 1) as its latency.\n");

  opt_reg_int(odb, "-cache:dl1:ports",
              "Number of dl1 ports",
              &cache_dl1_ports, /* default */ 2, /* print */ TRUE, NULL);

  opt_reg_flag(odb, "-cache:dl1:perfect",
               "Whether L1 D-cache is perfect",
               &cache_dl1_perfect, /* default */ FALSE,
               /* print */ TRUE, /* format */ NULL);

  opt_reg_string(odb, "-cache:dl2",
                 "l2 data cache config, i.e., {<config>|none}",
                 &cache_dl2_opt, "ul2:1024:64:4:l:4:4",
                 /* print */ TRUE, NULL);

  opt_reg_int_list(odb, "-cache:dl2lat",
                   "l2 data cache access latency (<base lat> <extra hit lat>)",
                   cache_dl2_lat, cache_dl2_lat_nelt, &cache_dl2_lat_nelt,
                   /* default */ cache_dl2_lat, /* print */ TRUE,
                   /* format */ NULL, /* accrue */ FALSE);

  opt_reg_flag(odb, "-cache:dl2:perfect",
               "Whether L2 D-cache is perfect",
               &cache_dl2_perfect, /* default */ FALSE,
               /* print */ TRUE, /* format */ NULL);

  opt_reg_string(odb, "-cache:il1",
                 "l1 inst cache config, i.e., {<config>|dl1|dl2|none}",
                 &cache_il1_opt, "il1:512:32:1:l:1:2",
                 /* print */ TRUE, NULL);

  opt_reg_note(odb,
               "  Cache levels can be unified by pointing a level of the instruction cache\n"
               "  hierarchy at the data cache hiearchy using the \"dl1\" and \"dl2\" cache\n"
               "  configuration arguments.  Most sensible combinations are supported, e.g.,\n"
               "\n"
               "    A unified l2 cache (il2 is pointed at dl2):\n"
               "      -cache:il1 il1:128:64:1:l:1:2 -cache:il2 dl2\n"
               "      -cache:dl1 dl1:256:32:1:l:4:2 -cache:dl2 ul2:1024:64:2:l:4:4\n"
               "\n"
               "    Or, a fully unified cache hierarchy (il1 pointed at dl1):\n"
               "      -cache:il1 dl1\n"
               "      -cache:dl1 ul1:256:32:1:l:4:2 -cache:dl2 ul2:1024:64:2:l:4:4\n");

  opt_reg_int_list(odb, "-cache:il1lat",
                   "l1 data cache access latency (<base lat> <extra hit lat>)",
                   cache_il1_lat, cache_il1_lat_nelt, &cache_il1_lat_nelt,
                   /* default */ cache_il1_lat, /* print */ TRUE,
                   /* format */ NULL, /* accrue */ FALSE);

  opt_reg_int(odb, "-cache:il1:blksize",
              "L1 I-cache block size, only needed if L1 I-cache is not"
              " instantiated",
              &user_il1_blksize, /* default */ 32, TRUE, NULL);

  opt_reg_flag(odb, "-cache:il1:perfect",
               "Whether L1 I-cache is perfect",
               &cache_il1_perfect, /* default */ FALSE,
               /* print */ TRUE, /* format */ NULL);

  opt_reg_string(odb, "-cache:il2",
                 "l2 instruction cache config, i.e., {<config>|dl2|none}",
                 &cache_il2_opt, "dl2",
                 /* print */ TRUE, NULL);

  opt_reg_flag(odb, "-cache:il2:perfect",
               "Whether L2 I-cache is perfect",
               &cache_il2_perfect, /* default */ FALSE,
               /* print */ TRUE, /* format */ NULL);

  opt_reg_int_list(odb, "-cache:il2lat",
                   "l1 data cache access latency (<base lat> <extra hit lat>)",
                   cache_il2_lat, cache_il2_lat_nelt, &cache_il2_lat_nelt,
                   /* default */ cache_il2_lat, /* print */ TRUE,
                   /* format */ NULL, /* accrue */ FALSE);

  opt_reg_flag(odb, "-cache:flush", "flush caches on system calls",
               &flush_on_syscalls, /* default */ FALSE, /* print */ TRUE, NULL);

  opt_reg_flag(odb, "-cache:icompress",
               "convert 64-bit inst addresses to 32-bit inst equivalents",
               &compress_icache_addrs, /* default */ FALSE,
               /* print */ TRUE, NULL);

  /* mem options */
  opt_reg_int_list(odb, "-mem:lat",
                   "memory access latency (<first_chunk> <inter_chunk>)",
                   mem_lat, mem_nelt, &mem_nelt, mem_lat,
                   /* print */ TRUE, /* format */ NULL, /* !accrue */ FALSE);

  opt_reg_int(odb, "-mem:width", "memory access bus width (in bytes)",
              &mem_bus_width, /* default */ 8,
              /* print */ TRUE, /* format */ NULL);

  /* TLB options */

  opt_reg_string(odb, "-tlb:itlb",
                 "instruction TLB config, i.e., {<config>|none}",
                 &itlb_opt, "itlb:16:4096:4:l", /* print */ TRUE, NULL);

  opt_reg_string(odb, "-tlb:dtlb",
                 "data TLB config, i.e., {<config>|none}",
                 &dtlb_opt, "dtlb:32:4096:4:l", /* print */ TRUE, NULL);

  opt_reg_int(odb, "-tlb:lat",
              "inst/data TLB miss latency (in cycles)",
              &tlb_miss_lat, /* default */ 30,
              /* print */ TRUE, /* format */ NULL);

  opt_reg_flag(odb, "-tlb:perfect",
               "Whether TLB's are perfect",
               &tlb_perfect, /* default */ FALSE,
               /* print */ TRUE, /* format */ NULL);

  /* resource configuration */

  opt_reg_flag(odb, "-res:infinite",
               "no functional unit limitations",
               &infinite_fu, /* default */ FALSE, TRUE, NULL);

  opt_reg_int(odb, "-res:ialu",
              "total number of integer ALU's available",
              &res_ialu, /* default */ fu_config[FU_IALU_INDEX].quantity,
              /* print */ TRUE, /* format */ NULL);

  opt_reg_int(odb, "-res:ibrsh",
              "total number of branch/I-shift units available",
              &res_ibrsh, /* default */ fu_config[FU_IBRSH_INDEX].quantity,
              /* print */ TRUE, /* format */ NULL);

  opt_reg_int(odb, "-res:imult",
              "total number of integer multiplier/dividers available",
              &res_imult, /* default */ fu_config[FU_IMULT_INDEX].quantity,
              /* print */ TRUE, /* format */ NULL);

  opt_reg_int(odb, "-res:ldport",
              "total number of memory system load ports available (to CPU)",
              &res_ldport, /* default */ fu_config[FU_LDPORT_INDEX].quantity,
              /* print */ TRUE, /* format */ NULL);

  opt_reg_int(odb, "-res:stport",
              "total number of memory system store ports available (to CPU)",
              &res_stport, /* default */ fu_config[FU_STPORT_INDEX].quantity,
              /* print */ TRUE, /* format */ NULL);

  opt_reg_int(odb, "-res:fpalu",
              "total number of floating point ALU's available",
              &res_fpalu, /* default */ fu_config[FU_FPALU_INDEX].quantity,
              /* print */ TRUE, /* format */ NULL);

  opt_reg_int(odb, "-res:fpmult",
              "total number of floating point multipliers available",
              &res_fpmult, /* default */ fu_config[FU_FPMULT_INDEX].quantity,
              /* print */ TRUE, /* format */ NULL);

  opt_reg_int(odb, "-res:fpdiv",
              "total number of floating point divider/sq-root units available",
              &res_fpdiv, /* default */ fu_config[FU_FPDIV_INDEX].quantity,
              /* print */ TRUE, /* format */ NULL);

  opt_reg_string_list(odb, "-pcstat",
                      "profile stat(s) against text addr's (mult uses ok)",
                      pcstat_vars, MAX_PCSTAT_VARS, &pcstat_nelt, NULL,
                      /* !print */ FALSE, /* format */ NULL, /* accrue */ TRUE);

  /* Execution options */

  opt_reg_flag(odb, "-bugcompat",
               "operate in backward-compatible bugs mode (for testing only)",
               &bugcompat_mode, /* default */ FALSE, /* print */ TRUE, NULL);

  opt_reg_uint(odb, "-prime_insts",
               "number of committed instructions for which to prime state",
               &num_prime_insn, /* default */ 0, /* print */ TRUE, NULL);

  opt_reg_uint(odb, "-sim_insts",
               "number of committed instructions for which to simulate state "
               "after priming",
               &num_fullsim_insn, /* default */ 0,
               /* print */ TRUE, /* format */ NULL);

  opt_reg_uint(odb, "-warmup_insts",
               "number of committed instructions for which to warm up caches",
               &num_warmup_insn, /* default */ 0, /* print */ TRUE, NULL);

  /* Reporting options */

  opt_reg_flag(odb, "-report_fetch",
               "report histogram of no. insts fetched/cycle",
               &report_fetch, /* default */ FALSE, /* print */ TRUE, NULL);
  opt_reg_flag(odb, "-report_issue",
               "report histogram of no. insts issued/cycle",
               &report_issue, /* default */ FALSE, /* print */ TRUE, NULL);
  opt_reg_flag(odb, "-report_commit",
               "report histogram of no. insts committed/cycle",
               &report_commit, /* default */ FALSE, /* print */ TRUE, NULL);
  opt_reg_flag(odb, "-report_post_issue",
               "report histogram of no. post-issued insts in RUU/cycle",
               &report_post_issue, /* default */ FALSE, /* print */ TRUE, NULL);
  opt_reg_flag(odb, "-report_useful_insts",
               "report histogram of no. non-mis-speculated pre-issue "
               "insts in RUU/cycle",
               &report_useful_insts,
               /* default */ FALSE, /* print */ TRUE, NULL);
  opt_reg_flag(odb, "-report_ready_insts",
               "report histogram of no. non-mis-speculated ready-to-issue "
               "insts in RUU/cycle",
               &report_ready_insts,
               /* default */ FALSE, /* print */ TRUE, NULL);
  opt_reg_flag(odb, "-report_miss_indep_insts",
               "report histogram of no. insts available to overlap L1 D$ misses",
               &report_miss_indep_insts,
               /* default */ FALSE, /* print */ TRUE, NULL);
  opt_reg_flag(odb, "-report_ruu_occ",
               "report histogram of RUU occupancy",
               &report_ruu_occ,
               /* default */ FALSE, /* print */ TRUE, NULL);
  opt_reg_flag(odb, "-report_imiss_ruu_occ",
               "report histogram of RUU occupancy on I misses",
               &report_imiss_ruu_occ,
               /* default */ FALSE, /* print */ TRUE, NULL);
  opt_reg_flag(odb, "-report_branch_info",
               "report histograms about # pending br's, br-resolution-time",
               &report_branch_info,
               /* default */ FALSE, /* print */ TRUE, NULL);
  opt_reg_flag(odb, "-report_issue_loc",
               "report histograms about from where in the RUU an instruction "
               "gets issued",
               &report_issue_loc,
               /* default */ FALSE, /* print */ TRUE, NULL);
  opt_reg_flag(odb, "-report_decode_loc",
               "report histograms about from where in the RUU an instruction "
               "gets enqueued (decoded)",
               &report_decode_loc,
               /* default */ FALSE, /* print */ TRUE, NULL);
  opt_reg_flag(odb, "-report_issue_delay",
               "report histograms about time spent waiting for operands and "
               "for issue",
               &report_issue_delay,
               /* default */ FALSE, /* print */ TRUE, NULL);
  opt_reg_flag(odb, "-report_miss_clustering",
               "report histograms for D- and I-miss clustering",
               &report_miss_clustering,
               /* default */ FALSE, /* print */ TRUE, NULL);

  /* dummy options */
  opt_reg_flag(odb, "-Z", "dummy", &dummy_flag, FALSE, TRUE, NULL);
  opt_reg_int(odb, "-z", "dummy", &dummy_int, FALSE, TRUE, NULL);
  opt_reg_string(odb, "-dummy", "dummy", &dummy_sz, FALSE, TRUE, NULL);
}

/* check bpred options and create branch predictor(s) */
struct bpred *
sim_check_bpred(struct opt_odb_t *odb,  /* options database */
                char *pred_type,        /* pred type to check/create */
                int hybrid_component_p) /* TRUE = component f/ hybr.,
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
                        /* btb sets */ btb_config[0],
                        /* btb assoc */ btb_config[1],
                        /* ret-addr stack size */
                        (old_style_retstack_interface ? 0 : retstack_size));
  }
  else if (!mystricmp(pred_type, "nottaken"))
  {
    /* static predictor, taken */
    if (btb_nelt != 2 && !hybrid_component_p)
      fatal("bad btb config (<num_sets> <associativity>)");
    pred = bpred_create(BPredNotTaken, 0, 0, 0, 0, 0, 0, 0, 0,
                        /* btb sets */ btb_config[0],
                        /* btb assoc */ btb_config[1],
                        /* ret-addr stack size */
                        (old_style_retstack_interface ? 0 : retstack_size));
  }
  else if (!mystricmp(pred_type, "backwards"))
  {
    /* static predictor, backwards-taken/forwards-not-taken */
    if (btb_nelt != 2 && !hybrid_component_p)
      fatal("bad btb config (<num_sets> <associativity>)");
    pred = bpred_create(BPredBackwards, 0, 0, 0, 0, 0, 0, 0, 0,
                        /* btb sets */ btb_config[0],
                        /* btb assoc */ btb_config[1],
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
                        /* bimod table size */ bimod_config[1],
                        /* l2 size */ 0,
                        /* history reg size */ 0,
                        /* (gshare not applicable) */ 0,
                        /* agree? */ bimod_config[3],
                        /* spec update not applicable */ 0,
                        /* spec update repair not applicable */ 0,
                        /* counter bits */ bimod_config[0],
                        /* btb sets */ btb_config[0],
                        /* btb assoc */ btb_config[1],
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
                        /* l1 size */ twolev_config[1],
                        /* l2 size */ twolev_config[2],
                        /* history reg size */ twolev_config[3],
                        /* gshare? */ twolev_config[5],
                        /* agree? */ twolev_config[6],
                        /* spec update? */ twolev_config[7],
                        /* spec update repair? */ twolev_config[8],
                        /* counter bits */ twolev_config[0],
                        /* btb sets */ btb_config[0],
                        /* btb assoc */ btb_config[1],
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
                        /* l1 size */ twolev2_config[1],
                        /* l2 size */ twolev2_config[2],
                        /* history reg size */ twolev2_config[3],
                        /* gshare? */ twolev2_config[5],
                        /* agree? */ twolev2_config[6],
                        /* spec update? */ twolev2_config[7],
                        /* spec update repair? */ twolev2_config[8],
                        /* counter bits */ twolev2_config[0],
                        /* btb sets */ btb_config[0],
                        /* btb assoc */ btb_config[1],
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
               &h_retstack_sz, &gshare_p, &spec_update_p, &spec_update_repair) != 9)
      fatal("bad hybrid-predictor parms: "
            "(<counter_bits>:<table_size>:<pred1>:<pred2>:<hist_size>:"
            "<retstack_sz>:<gshare?>:<spec-update?>:<spec-update repair?>)");

    pred1 = sim_check_bpred(odb, pred1_nm, TRUE);
    pred2 = sim_check_bpred(odb, pred2_nm, TRUE);

    pred = bpred_hybrid_create(/* predictor-predictor table size */ tbl_sz,
                               /* pred component #1 */ pred1,
                               /* pred component #2 */ pred2,
                               /* global shift-reg size */ shreg_sz,
                               /* gshare? */ gshare_p,
                               /* spec update? */ spec_update_p,
                               /* spec update repair? */ spec_update_repair,
                               /* counter bits */ cntr_bits,
                               /* btb sets */ btb_config[0],
                               /* btb assoc */ btb_config[1],
                               /* ret-addr stack size */
                               (old_style_retstack_interface
                                    ? h_retstack_sz
                                    : retstack_size));
    if (h_retstack_sz && !old_style_retstack_interface)
      warn("ret-addr stack size specified in PHT config, but overridden");
    else if (retstack_size && old_style_retstack_interface)
      warn("ret-addr stack size specified directly with -bpred:retstack,"
           " but overriden by value in PHT config");
  }
  else
    fatal("cannot parse predictor type `%s'", pred_type);

  if (hybrid_component_p)
  {
    btb_config[0] = btb_cfg_save[0];
    btb_config[1] = btb_cfg_save[1];
  }

  if ((bpred_merge_hist && bpred_cat_hist) || (bpred_merge_hist_shift && bpred_cat_hist))
    fatal("merge-hist and cat-hist are mutually exclusive");
  if (bpred_merge_hist && bpred_merge_hist_shift)
    fatal("shifted and unshifted merge-hist are mutually exclusive");

  /* FIXME: allow these to be specified per-bpred-component */
  if (pred && pred->class == BPredHybrid)
    bpred_create_aux(pred,
                     /* type of retstack fixup to use */ retstack_patch_level,
                     /* do rets update BTB? */ per_thread_retstack == OneStackPredOnly,
                     /* must caller supply TOS? */ per_thread_retstack == PerThreadTOSP,
                     /* global hist size to merge w/ local bits */ 0,
                     /* global hist size to merge w/ local bits */ 0,
                     /* global hist size to concat to local bits */ 0,
                     /* bpred left shift for gshare */ 0,
                     /* low-order baddr bits to discard */ 0);
  /* FIXME: This is a temporary HACK: */
  else if (pred && pred->class == BPred2Level && pred->dirpred.two.l1size == 1)
    bpred_create_aux(pred,
                     /* type of retstack fixup to use */ hybrid_component_p ? 0 : retstack_patch_level,
                     /* do rets update BTB? */ per_thread_retstack == OneStackPredOnly,
                     /* must caller supply TOS? */ per_thread_retstack == PerThreadTOSP,
                     /* global hist size to merge w/ local bits */ 0,
                     /* global hist size to merge w/ local bits */ 0,
                     /* global hist size to concat to local bits */ 0,
                     /* bpred left shift for gshare */ bpred_gshare_shift,
                     /* low-order baddr bits to discard */ bpred_gshare_drop_lsbits);
  else if (pred && pred->class == BPred2Level)
    bpred_create_aux(pred,
                     /* type of retstack fixup to use */ hybrid_component_p ? 0 : retstack_patch_level,
                     /* do rets update BTB? */ per_thread_retstack == OneStackPredOnly,
                     /* must caller supply TOS? */ per_thread_retstack == PerThreadTOSP,
                     /* global hist size to merge w/ local bits */ bpred_merge_hist,
                     /* global hist size to merge w/ local bits */ bpred_merge_hist_shift,
                     /* global hist size to concat to local bits */ bpred_cat_hist,
                     /* bpred left shift for gshare */ bpred_gshare_shift,
                     /* low-order baddr bits to discard */ bpred_gshare_drop_lsbits);
  else if (pred)
    bpred_create_aux(pred,
                     /* type of retstack fixup to use */ hybrid_component_p ? 0 : retstack_patch_level,
                     /* do rets update BTB? */ per_thread_retstack == OneStackPredOnly,
                     /* must caller supply TOS? */ per_thread_retstack == PerThreadTOSP,
                     /* global hist size to merge w/ local bits */ 0,
                     /* global hist size to merge w/ local bits */ 0,
                     /* global hist size to concat to local bits */ 0,
                     /* bpred left shift for gshare */ 0,
                     /* low-order baddr bits to discard */ 0);

  return pred;
}

/* check simulator-specific option values */
void sim_check_options(struct opt_odb_t *odb, /* options database */
                       int argc, char **argv) /* command line arguments */
{
  char name[128], c;
  int nsets, bsize, assoc, mshrs, busint;

  if (ptrace_nelt != 3 && ptrace_nelt != 0)
    fatal("ptrace takes 3 arguments: <level> <fname|stdout|stderr> <range>");

  if (num_warmup_insn > 0 && num_prime_insn == 0)
    num_prime_insn = num_warmup_insn + 1000000;
  else if (num_warmup_insn > 0)
    num_prime_insn += num_warmup_insn;

  if (max_threads < 1)
    fatal("max number of threads must be at least 1");
  if (max_threads > N_THREAD_RECS)
    fatal("max number of threads can't currently exceed %d.  \n"
          "   Try increasing N_THREAD_RECS",
          N_THREAD_RECS);

  if (ruu_ifq_size < 1 || (ruu_ifq_size & (ruu_ifq_size - 1)) != 0)
    fatal("inst fetch queue size must be positive > 0 and a power of two");

  if (ruu_branch_penalty < 1)
    warn("mis-prediction penalty has been declared less than 1 cycle");

  if (max_cache_lines < 0)
    fatal("-fetch:max_lines_per_thread must be a positive number");

  if (bconf_type == BCF_None && bconf_th_selector != BTS_Profile)
    fatal("if bconf_type == 'none', then bconf_selector must be 'profile'");

  if (bconf_num_thresholds < 1 || bconf_num_thresholds > MAX_BCONF_THRESHOLDS)
    fatal("number of thresholds must be between 1 and %d",
          MAX_BCONF_THRESHOLDS);

  if (bconf_num_thresholds > 1 && bconf_th_selector == BTS_None)
    fatal("cannot have bconf_selector == 'none' when num thresholds is > 1");

  if (max_threads > 1 && bconf_type != BCF_Naive && bconf_type != BCF_Omni)
  {
    bconf = bconf_create(bconf_type, bconf_th_selector, bconf_table_size,
                         bconf_table_width, bconf_num_thresholds,
                         bconf_thresholds, bconf_gshare,
                         bconf_history_update, output_bconf_dist,
                         bconf_config_file);
  }

  if (max_threads > 1 && squash_remove)
    fatal("can't do squash:remove in multi-path/multi-threaded mode");

  if (fetch_pri_pol == Omni_Pri)
    fatal("Fetch policy 'omni_pri' not yet supported");

  if (max_threads > 1 && bconf_squash_extra && bconf_fork_mispred)
    warn("both -bconf:squash_extra and -bconf:fork_mispred are enabled; statistics may be incorrect");

  if (fork_in_fetch && fork_prune)
    fatal("fork-pruning not yet implemented for fork-in-fetch");

  if (fork_in_fetch && fetch_pri_pol == Pred_Pri)
    fatal("pred-pri fetch policy not yet implemented for fork-in-fetch");
  else if (fetch_pri_pol == Pred_Pri)
    fetch_pred_pri = TRUE;

  if (fetch_pri_pol == Ruu_Pri)
    fetch_ruu_pri = TRUE;

  /* auxiliary function checks bpred opts */
  pred = sim_check_bpred(odb, pred_type, FALSE);

  if (per_thread_retstack == PerThreadStacks && retstack_size == 0)
    fatal("if per-thread retstacks, retstack size must be nonzero");

  if (max_threads > 1 && pred_perfect)
    fatal("can't simulate perfect branch prediction with more than 1 thread");

  if (output_cbr_hist)
  {
    char name[200];

    if (!strlen(outfile_name))
      fatal("please specify an output file name using"
            " the \"-outfile\" option");

    sprintf(name, "%s_cbr_hist", outfile_name);
    cbr_hist_file = fopen(name, "w");
    assert(cbr_hist_file);
  }

  if (output_cbr_acc)
  {
    char name[200];

    if (!strlen(outfile_name))
      fatal("please specify an output file name using"
            " the \"-outfile\" option");

    sprintf(name, "%s_cbr_acc", outfile_name);
    cbr_acc_file = fopen(name, "w");
    assert(cbr_acc_file);
  }

  /* boosting */
  if (pred_synth_up_threshold < 0.0 || pred_synth_up_threshold > 1.0 || pred_synth_down_threshold < 0.0 || pred_synth_down_threshold > 1.0)
    fatal("percentage of mispredictions to boost must be a non-negative "
          "fraction in [0.0, 1.0)");
  else if (pred_synth_up_threshold > 0.0 && pred_synth_down_threshold > 0.0)
    fatal("can't simultaneously synth-boost and synth-decrease prediction rate");
  else if (pred_synth_up_threshold > 0.0)
  {
    if (max_threads > 1)
      fatal("synthetic bpred not supported in conjunction with multi-path/"
            "multi-threaded execution");
    pred_synth_up = TRUE;
  }
  else if (pred_synth_down_threshold > 0.0)
  {
    if (max_threads > 1 && fork_in_fetch)
      fatal("synthetic bpred-decreasing not supported in conjunction with"
            " fork-in-fetch");
    pred_synth_down = TRUE;
  }

  if ((pred_synth_up || pred_synth_down) && pred_perfect)
    fatal("synthetic bpred not supported in conjunction with pred-perfect");

  /* when to update bpred/bconf */
  if (bpred_perf_update && bpred_spec_update)
    fatal("Can only update bpred in one stage; but both -bpred:perf_update\n"
          "   and -bpred:spec_update are set");

  if (bconf_perf_update && bconf_spec_update)
    fatal("Can only update bconf in one stage; but both -bconf:perf_update\n"
          "   and -bconf:spec_update are set");

  if (bconf_squash_extra || bconf_fork_mispred || bconf_type == BCF_Omni)
    if (fork_in_fetch)
      panic("for bconf:squash_extra, bconf:fork_mispred, or omni forking,\n"
            "   fork:in_fetch must be off");

  if ((fix_addrs || fix_addrs_indir) && max_threads > 1 && fork_in_fetch)
    fatal("BTB-address-patching (-fix_addrs/-fix_addrs_indir) not supported in"
          "\n   conjunction with multi-path/multi-threaded execution with"
          "fork-in-fetch");

  if (perfect_except_retstack && (!fix_addrs_indir || !fix_addrs || pred_synth_up_threshold < 1.0))
    fatal("to use -bpred:perf_except_retstack, must turn on "
          "-bpred:fix_addrs, -bpred:fix_addrs_indir, and synth100");

  if (ruu_decode_width < 1)
    fatal("issue width must be a positive, non-zero value");

  if (fetch_cache_lines < 1)
    fatal("number of cache lines to fetch per cycle must be at least 1");

  if (ruu_issue_width < 1 || ruu_int_issue_width < 1 || ruu_fp_issue_width < 1)
    fatal("issue widths must be positive, non-zero values");

  if (RUU_size < 2)
    fatal("RUU size must be a positive number > 1");

  if (IIQ_size == 0)
    fatal("IIQ size must be a positive number >= 1");
  if (IIQ_size > RUU_size)
    warn("IIQ size > RUU size; extra IIQ capacity wasted");

  if (FIQ_size == 0)
    fatal("FIQ size must be a positive number >= 1");
  if (FIQ_size > RUU_size)
    warn("FIQ size > RUU size; extra FIQ capacity wasted");

  if (LSQ_size < 2)
    fatal("LSQ size must be a positive number > 1");
  if (LSQ_size > RUU_size)
    warn("LSQ size > RUU size; extra LSQ capacity wasted");

  if (ruu_commit_width < 1)
    fatal("commit width must be a positive, non-zero value");

  /* use a level 1 D-cache? */
  if (!mystricmp(cache_dl1_opt, "none") || !mystricmp(cache_dl1_opt, "mem"))
  {
    cache_dl1 = NULL;

    /* If we have no D-caches, should we still appropriately model
       * memory access? */
    if (!mystricmp(cache_dl1_opt, "mem"))
      dl1_access_mem = TRUE;

    /* the level 2 D-cache cannot be defined */
    if (strcmp(cache_dl2_opt, "none"))
      fatal("the l1 data cache must defined if the l2 cache is defined");
    cache_dl2 = NULL;
  }
  else /* dl1 is defined */
  {
    if (sscanf(cache_dl1_opt, "%[^:]:%d:%d:%d:%c:%d:%d",
               name, &nsets, &bsize, &assoc, &c, &mshrs, &busint) != 7)
      fatal("bad l1 D-cache parms: "
            "<name>:<nsets>:<bsize>:<assoc>:<repl>:<mshr's>:<bus interval>");
    cache_dl1 = cache_create(name, nsets, bsize, /* balloc */ FALSE,
                             /* usize */ 0, assoc, cache_char2policy(c),
                             dl1_access_fn,
                             /* hit lat */ cache_dl1_lat[0], cache_dl1_lat[1],
                             mshrs, busint);
    if (cache_dl1_ports < 1)
      fatal("dl1 cache must have at least 1 port");

    if (cache_dl1_perfect)
    {
      cache_set_perfect(cache_dl1);
      cache_dl2_perfect = TRUE;
    }

    /* is the level 2 D-cache defined? */
    if (!mystricmp(cache_dl2_opt, "none"))
      cache_dl2 = NULL;
    else
    {
      if (sscanf(cache_dl2_opt, "%[^:]:%d:%d:%d:%c:%d:%d",
                 name, &nsets, &bsize, &assoc, &c, &mshrs, &busint) != 7)
        fatal("bad l2 D-cache parms: "
              "<name>:<nsets>:<bsize>:<assoc>:<repl>:<mshr's>:<bus interval>");
      cache_dl2 = cache_create(name, nsets, bsize, /* balloc */ FALSE,
                               /* usize */ 0, assoc, cache_char2policy(c),
                               dl2_access_fn,
                               /* hit lat */
                               cache_dl2_lat[0], cache_dl2_lat[1],
                               mshrs, busint);
      if (cache_dl2_perfect)
        cache_set_perfect(cache_dl2);
    }
  }

  /* use a level 1 I-cache? */
  if (!mystricmp(cache_il1_opt, "none") || !mystricmp(cache_il1_opt, "mem"))
  {
    cache_il1 = NULL;

    /* If we have no I-caches, should we still appropriately model
       * memory access? */
    if (!mystricmp(cache_il1_opt, "mem"))
      il1_access_mem = TRUE;

    /* the level 2 I-cache cannot be defined */
    if (strcmp(cache_il2_opt, "none"))
      fatal("the l1 inst cache must defined if the l2 cache is defined");
    cache_il2 = NULL;
  }
  else if (!mystricmp(cache_il1_opt, "dl1"))
  {
    if (!cache_dl1)
      fatal("I-cache l1 cannot access D-cache l1 as it's undefined");
    cache_il1 = cache_dl1;

    if (cache_il1_perfect || cache_dl1_perfect)
    {
      if (!cache_dl1_perfect || !cache_il1_perfect)
        fatal("Unified l1 must be perfect for both data and instructions");
    }

    /* the level 2 I-cache cannot be defined */
    if (strcmp(cache_il2_opt, "none"))
      fatal("the l1 inst cache must defined if the l2 cache is defined");
    cache_il2 = NULL;
  }
  else if (!mystricmp(cache_il1_opt, "dl2"))
  {
    if (!cache_dl2)
      fatal("I-cache l1 cannot access D-cache l2 as it's undefined");
    cache_il1 = cache_dl2;

    if (cache_il1_perfect || cache_dl2_perfect)
    {
      if (!cache_dl2_perfect || !cache_il1_perfect)
        fatal("Unified L1-I$ and L2-D$ must be perfect for both cases");
    }

    /* the level 2 I-cache cannot be defined */
    if (strcmp(cache_il2_opt, "none"))
      fatal("the l1 inst cache must defined if the l2 cache is defined");
    cache_il2 = NULL;
  }
  else /* il1 is defined */
  {
    if (sscanf(cache_il1_opt, "%[^:]:%d:%d:%d:%c:%d:%d",
               name, &nsets, &bsize, &assoc, &c, &mshrs, &busint) != 7)
      fatal("bad l1 I-cache parms: "
            "<name>:<nsets>:<bsize>:<assoc>:<repl>:<mshr's>:<bus interval>");
    cache_il1 = cache_create(name, nsets, bsize, /* balloc */ FALSE,
                             /* usize */ 0, assoc, cache_char2policy(c),
                             il1_access_fn,
                             /* hit lat */ cache_il1_lat[0], cache_il1_lat[1],
                             mshrs, busint);
    if (cache_il1_perfect)
    {
      cache_set_perfect(cache_il1);
      cache_il2_perfect = TRUE;
    }

    /* is the level 2 I-cache defined? */
    if (!mystricmp(cache_il2_opt, "none"))
    {
      cache_il2 = NULL;
      if (cache_dl2)
        cache_set_bus(cache_il1, cache_dl2);
      else if (cache_dl1)
        cache_set_bus(cache_il1, cache_dl1);
    }
    else if (!mystricmp(cache_il2_opt, "dl2"))
    {
      if (!cache_dl2)
        fatal("I-cache l2 cannot access D-cache l2 as it's undefined");
      cache_il2 = cache_dl2;

      cache_set_bus(cache_il1, cache_dl1);

      if (cache_il2_perfect || cache_dl2_perfect)
      {
        if (!cache_dl2_perfect || !cache_il2_perfect)
          fatal("Unified l2 must be perfect for both data and "
                "instructions");
      }
    }
    else if (!mystricmp(cache_il2_opt, "dl1"))
    {
      if (!cache_dl1)
        fatal("I-cache l2 cannot access D-cache l1 as it's undefined");
      cache_il2 = cache_dl1;

      if (cache_il2_perfect || cache_dl1_perfect)
      {
        if (!cache_dl1_perfect || !cache_il2_perfect)
          fatal("Unified l2 must be perfect for both data and "
                "instructions");
      }
    }
    else
    {
      if (sscanf(cache_il2_opt, "%[^:]:%d:%d:%d:%c:%d:%d",
                 name, &nsets, &bsize, &assoc, &c, &mshrs, &busint) != 7)
        fatal("bad l2 I-cache parms: "
              "<name>:<nsets>:<bsize>:<assoc>:<repl>:<mshr's>:<bus interval>");
      cache_il2 = cache_create(name, nsets, bsize, /* balloc */ FALSE,
                               /* usize */ 0, assoc, cache_char2policy(c),
                               il2_access_fn,
                               /* hit lat */
                               cache_il2_lat[0], cache_il2_lat[1],
                               mshrs, busint);
      if (cache_dl2)
        cache_set_bus(cache_il2, cache_dl2);
      else if (cache_dl1)
        cache_set_bus(cache_il2, cache_dl1);

      if (cache_il2_perfect)
        cache_set_perfect(cache_il2);
    }
  }

  /* ruu_fetch needs to know the block-mask for the L1 i-cache. */
  if (cache_il1)
    cache_il1_blkshift = cache_il1->set_shift;
  else
  {
    if (user_il1_blksize < SS_INST_SIZE)
      fatal("user_il1_blksize must be greater than %d", SS_INST_SIZE);

    cache_il1_blkshift = log_base2(user_il1_blksize);
  }
  MaxFetch = fetch_cache_lines * (1 << cache_il1_blkshift) / SS_INST_SIZE;
  if (compress_icache_addrs)
    MaxFetch *= 2;

  /* use a D-TLB? */
  if (!mystricmp(dtlb_opt, "none"))
    dtlb = NULL;
  else
  {
    if (sscanf(dtlb_opt, "%[^:]:%d:%d:%d:%c",
               name, &nsets, &bsize, &assoc, &c) != 5)
      fatal("bad TLB parms: "
            "<name>:<nsets>:<page_size>:<assoc>:<repl>");
    dtlb = cache_create(name, nsets, bsize, /* balloc */ FALSE,
                        /* usize */ sizeof(SS_ADDR_TYPE), assoc,
                        cache_char2policy(c), dtlb_access_fn,
                        /* hit latency */ 1, 0, /* mshrs */ 1,
                        /* perfect bus */ 0);
    if (tlb_perfect)
      cache_set_perfect(dtlb);
  }

  /* use an I-TLB? */
  if (!mystricmp(itlb_opt, "none"))
    itlb = NULL;
  else if (!mystricmp(itlb_opt, "dtlb"))
  {
    if (dtlb == NULL)
      fatal("itlb cannot access dtlb as it's undefined");
    itlb = dtlb;
  }
  else
  {
    if (sscanf(itlb_opt, "%[^:]:%d:%d:%d:%c",
               name, &nsets, &bsize, &assoc, &c) != 5)
      fatal("bad TLB parms: "
            "<name>:<nsets>:<page_size>:<assoc>:<repl>");
    itlb = cache_create(name, nsets, bsize, /* balloc */ FALSE,
                        /* usize */ sizeof(SS_ADDR_TYPE), assoc,
                        cache_char2policy(c), itlb_access_fn,
                        /* hit latency */ 1, 0, /* mshrs */ 1,
                        /* perfect bus */ 0);
    if (tlb_perfect)
      cache_set_perfect(itlb);
  }

  if (cache_dl1_lat_nelt != 2)
    fatal("bad l1 data cache latency (<base lat> <extra hit lat>)");

  if (cache_dl2_lat_nelt != 2)
    fatal("bad l2 data cache latency (<base lat> <extra hit lat>)");

  if (cache_il1_lat_nelt != 2)
    fatal("bad l1 instruction cache latency (<base lat> <extra hit lat>)");

  if (cache_il2_lat_nelt != 2)
    fatal("bad l2 instruction cache latency (<base lat> <extra hit lat>)");

  if (cache_dl1_lat[0] < 1)
    fatal("l1 data cache latency must be greater than zero");

  if (cache_dl2_lat[0] < 1)
    fatal("l2 data cache latency must be greater than zero");

  if (cache_il1_lat[0] < 1)
    fatal("l1 instruction cache latency must be greater than zero");

  if (cache_il2_lat[0] < 1)
    fatal("l2 instruction cache latency must be greater than zero");

  if (mem_nelt != 2)
    fatal("bad memory access latency (<first_chunk> <inter_chunk>)");

  if (mem_lat[0] < 1 || mem_lat[1] < 1)
    fatal("all memory access latencies must be greater than zero");

  if (mem_bus_width < 1 || (mem_bus_width & (mem_bus_width - 1)) != 0)
    fatal("memory bus width must be positive non-zero and a power of two");

  if (tlb_miss_lat < 1)
    fatal("TLB miss latency must be greater than zero");

  if (res_ialu < 1)
    fatal("number of integer ALU's must be greater than zero");
  if (res_ibrsh < 1)
    fatal("number of branch/Ishift units must be greater than zero");
  if (res_imult < 1)
    fatal("number of integer multiplier/dividers must be greater than zero");
  if (res_ldport < 1)
    fatal("number of memory system load ports must be greater than zero");
  if (res_stport < 1)
    fatal("number of memory system store ports must be greater than zero");
  if (res_fpalu < 1)
    fatal("number of floating point ALU's must be greater than zero");
  if (res_fpmult < 1)
    fatal("number of floating point multipliers must be > zero");
  if (res_fpdiv < 1)
    fatal("number of floating point divider/sq-root units must be > zero");

  if (!infinite_fu)
  {
    if (res_ialu > MAX_INSTS_PER_CLASS)
      fatal("number of integer ALU's must be <= MAX_INSTS_PER_CLASS");
    fu_config[FU_IALU_INDEX].quantity = res_ialu;
    if (res_ibrsh > MAX_INSTS_PER_CLASS)
      fatal("number of branch/Ishift units must be <= MAX_INSTS_PER_CLASS");
    fu_config[FU_IBRSH_INDEX].quantity = res_ibrsh;
    if (res_imult > MAX_INSTS_PER_CLASS)
      fatal("number of integer mult/div's must be <= MAX_INSTS_PER_CLASS");
    fu_config[FU_IMULT_INDEX].quantity = res_imult;
    if (res_ldport > MAX_INSTS_PER_CLASS)
      fatal("number of load ports must be <= MAX_INSTS_PER_CLASS");
    fu_config[FU_LDPORT_INDEX].quantity = res_ldport;
    if (res_stport > MAX_INSTS_PER_CLASS)
      fatal("number of store ports must be <= MAX_INSTS_PER_CLASS");
    fu_config[FU_STPORT_INDEX].quantity = res_stport;
    if (res_fpalu > MAX_INSTS_PER_CLASS)
      fatal("number of floating point ALU's must be <= MAX_INSTS_PER_CLASS");
    fu_config[FU_FPALU_INDEX].quantity = res_fpalu;
    if (res_fpmult > MAX_INSTS_PER_CLASS)
      fatal("number of FP mult's must be <= MAX_INSTS_PER_CLASS");
    fu_config[FU_FPMULT_INDEX].quantity = res_fpmult;
    if (res_fpdiv > MAX_INSTS_PER_CLASS)
      fatal("number of FP div's must be <= MAX_INSTS_PER_CLASS");
    fu_config[FU_FPDIV_INDEX].quantity = res_fpdiv;
  }

  if ((num_prime_insn + num_fullsim_insn < num_fullsim_insn) ||
      (num_prime_insn + num_fullsim_insn < num_prime_insn))
    fatal("Total of -prime_insts and -sim_insts must be <= MAXUINT");
}

/* print simulator-specific configuration information */
void sim_aux_config(FILE *stream) /* output stream */
{
  /* nada */
}

/* STATS:
 * register simulator-specific statistics 
 */
void sim_reg_stats(struct stat_sdb_t *sdb) /* stats database */
{
  int i;

  /* register baseline stats */
  stat_reg_counter(sdb, "sim_num_insn",
                   "total number of instructions committed",
                   &sim_num_insn, 0, NULL);
  stat_reg_counter(sdb, "sim_num_refs",
                   "total number of loads and stores committed",
                   &sim_num_refs, 0, NULL);
  stat_reg_counter(sdb, "sim_num_loads",
                   "total number of loads committed",
                   &sim_num_loads, 0, NULL);
  stat_reg_formula(sdb, "sim_num_insn.PP",
                   "total number of instructions committed",
                   "sim_num_insn - primed_insts", "%12.0f");
  stat_reg_formula(sdb, "sim_num_refs.PP",
                   "total number of loads and stores committed",
                   "sim_num_refs - primed_refs", "%12.0f");
  stat_reg_formula(sdb, "sim_num_loads.PP",
                   "total number of loads committed",
                   "sim_num_loads - primed_loads", "%12.0f");
  stat_reg_formula(sdb, "sim_num_stores.PP",
                   "total number of stores committed",
                   "sim_num_refs.PP - sim_num_loads.PP", "%12.0f");
  stat_reg_counter(sdb, "sim_num_branches.PP",
                   "total number of branches committed",
                   &sim_num_branches, /* initial value */ 0, /* format */ NULL);
  stat_reg_counter(sdb, "sim_num_cond_branches.PP",
                   "total number of cond'l branches committed",
                   &sim_num_cond_branches, /* initial value */ 0, NULL);
  stat_reg_int(sdb, "sim_elapsed_time",
               "total simulation time in seconds",
               (int *)&sim_elapsed_time, 0, NULL);
  stat_reg_formula(sdb, "sim_inst_rate",
                   "simulation speed (in insts/sec)",
                   "sim_num_insn / sim_elapsed_time", NULL);

  stat_reg_counter(sdb, "sim_total_insn",
                   "total number of instructions executed",
                   &sim_total_insn, 0, NULL);
  stat_reg_counter(sdb, "sim_total_fetched",
                   "total number of instructions fetched",
                   &sim_total_fetched, 0, NULL);
  stat_reg_counter(sdb, "sim_total_refs",
                   "total number of loads and stores executed",
                   &sim_total_refs, 0, NULL);
  stat_reg_counter(sdb, "sim_total_loads",
                   "total number of loads executed",
                   &sim_total_loads, 0, NULL);
  stat_reg_formula(sdb, "sim_total_stores",
                   "total number of stores executed",
                   "sim_total_refs - sim_total_loads", "%12.0f");
  stat_reg_counter(sdb, "sim_total_branches",
                   "total number of branches executed",
                   &sim_total_branches, /* initial value */ 0, /* format */ NULL);
  stat_reg_counter(sdb, "sim_total_cond_branches.PP",
                   "total number of cond'l branches executed",
                   &sim_total_cond_branches, /* initial value */ 0, NULL);

  /* priming info */
  stat_reg_counter(sdb, "primed_insts",
                   "number of insts for which state was primed",
                   &primed_insts, 0, NULL);
  stat_reg_counter(sdb, "primed_refs",
                   "number of refs for which state was primed",
                   &primed_refs, 0, NULL);
  stat_reg_counter(sdb, "primed_loads",
                   "number of loads for which state was primed",
                   &primed_loads, 0, NULL);
  stat_reg_counter(sdb, "primed_cycles",
                   "cycles for which state was primed",
                   &primed_cycles, /* init val */ 0, /* format */ NULL);

  /* register performance stats */
  stat_reg_counter(sdb, "sim_cycle",
                   "total simulation time in cycles",
                   &sim_cycle, /* initial value */ 0, /* format */ NULL);
  stat_reg_formula(sdb, "sim_cycle.PP",
                   "POST_PRIME_CYCLES",
                   "sim_cycle - primed_cycles", /* format */ "%12.0f");
  stat_reg_formula(sdb, "sim_IPC",
                   "instructions per cycle",
                   "sim_num_insn / sim_cycle", /* format */ NULL);
  stat_reg_formula(sdb, "sim_CPI",
                   "cycles per instruction",
                   "sim_cycle / sim_num_insn", /* format */ NULL);
  stat_reg_formula(sdb, "sim_IPC.PP",
                   "instructions per cycle",
                   "sim_num_insn.PP / (sim_cycle - primed_cycles)",
                   /* format */ NULL);
  stat_reg_formula(sdb, "sim_CPI.PP",
                   "cycles per instruction",
                   "(sim_cycle - primed_cycles) / sim_num_insn.PP",
                   /* format */ NULL);
  stat_reg_formula(sdb, "sim_exec_BW",
                   "total instructions (mis-spec + committed) per cycle",
                   "sim_total_insn / sim_cycle", /* format */ NULL);
  stat_reg_formula(sdb, "sim_IPB.PP",
                   "instruction per branch",
                   "sim_num_insn.PP / sim_num_branches.PP", /* format */ NULL);

  /* register predictor stats */
  if (pred)
    bpred_reg_stats(pred, sdb);

  /* forking stats */
  stat_reg_counter(sdb, "multi-path: num_thread_context_overflows",
                   "number of forks that failed from lack of contexts",
                   &num_thread_context_overflows, /* init val */ 0, NULL);
  stat_reg_counter(sdb, "multi-path: num forked overall.PP",
                   "no. of committed branches that forked",
                   &num_forked, /* init val */ 0, NULL);
  stat_reg_counter(sdb, "multi-path: total forked overall.PP",
                   "total no. of branches that forked",
                   &total_forked, /* init val */ 0, NULL);
  stat_reg_counter(sdb, "multi-path: total bad forks squashed.PP",
                   "semi-omni: total no. of forks squashed by bconf_squash_extra",
                   &total_bad_forks_squashed, /* init val */ 0, NULL);
  stat_reg_counter(sdb, "multi-path: total extra forked overall.PP",
                   "semi-omni: total no. of mispreds forked by bconf_fork_mispred",
                   &total_extra_forked, /* init val */ 0, NULL);
  stat_reg_counter(sdb, "multi-path: total forks pruned.PP",
                   "pred-path-pruning: total no. of forks pruned",
                   &total_forks_pruned, /* init val */ 0, NULL);
  if (bconf)
    bconf_reg_stats(bconf, sdb);
  stat_reg_counter(sdb, "bconf:num_unforked_right_lo.PP",
                   "no. of committed, low-conf non-forks done for correct "
                   "predictions",
                   &num_unforked_good_lo, /* init val */ 0, NULL);
  stat_reg_counter(sdb, "bconf:num_unforked_wrong_hi.PP",
                   "no. of committed, hi-conf non-forks done for incorrect "
                   "predictions",
                   &num_unforked_bad_hi, /* init val */ 0, NULL);
  stat_reg_counter(sdb, "bconf:total_unforked_right_lo.PP",
                   "total no. of low-conf non-forks done for correct "
                   "predictions",
                   &total_unforked_good_lo, /* init val */ 0, NULL);
  stat_reg_counter(sdb, "bconf:total_unforked_wrong_hi.PP",
                   "total no. of hi-conf non-forks done for incorrect "
                   "predictions",
                   &total_unforked_bad_hi, /* init val */ 0, NULL);
  stat_reg_counter(sdb, "num_forked_right.PP",
                   "no. of committed forks done for correct predictions",
                   &num_forked_wasted, /* init val */ 0, NULL);
  stat_reg_counter(sdb, "num_forked_wrong.PP",
                   "no. of committed forks done for incorrect predictions",
                   &num_forked_good, /* init val */ 0, NULL);
  stat_reg_counter(sdb, "num_unforked_right.PP",
                   "no. of committed non-forks for correct predictions",
                   &num_unforked_good, /* init val */ 0, NULL);
  stat_reg_counter(sdb, "num_unforked_wrong.PP",
                   "no. of committed non-forks for incorrect predictions",
                   &num_unforked_bad, /* init val */ 0, NULL);
  stat_reg_counter(sdb, "total_forked_right.PP",
                   "total no. of forks done for correct predictions",
                   &total_forked_wasted, /* init val */ 0, NULL);
  stat_reg_counter(sdb, "total_forked_wrong.PP",
                   "total no. of forks done for incorrect predictions",
                   &total_forked_good, /* init val */ 0, NULL);
  stat_reg_counter(sdb, "total_unforked_right.PP",
                   "total no. of non-forks for correct predictions",
                   &total_unforked_good, /* init val */ 0, NULL);
  stat_reg_counter(sdb, "total_unforked_wrong.PP",
                   "total no. of non-forks for incorrect predictions",
                   &total_unforked_bad, /* init val */ 0, NULL);
  stat_reg_counter(sdb, "multi-path: num_fork_opps.PP",
                   "no. of committed cond br's that could have forked",
                   &num_fork_opps, /* init val */ 0, NULL);
  stat_reg_counter(sdb, "multi-path: total_fork_opps.PP",
                   "total no. of cond br's that could have forked",
                   &total_fork_opps, /* init val */ 0, NULL);
  stat_reg_counter(sdb, "multi-path: num_fork_dispatch_timing_inaccuracies.PP",
                   "number of committed, mispred branches that "
                   "forked down the not-taken path",
                   &num_fork_dispatch_timing_inaccuracies,
                   /* init val */ 0, NULL);
  stat_reg_counter(sdb, "multi-path: num_failed_omni_forks.PP",
                   "bconf-omni: number of committed, mispred branches that "
                   "could not fork",
                   &num_failed_omni_forks, /* init val */ 0, NULL);
  stat_reg_counter(sdb,
                   "multi-path: num_fork_dispatch_resource_inaccuracies.PP",
                   "bconf-omni: number of committed, mispred branches that"
                   " could not fork due to simulation inaccuracies",
                   &num_fork_dispatch_resource_inaccuracies,
                   /* init val */ 0, NULL);
  stat_reg_counter(sdb, "multi-path: total pre-decode-squashed.PP",
                   "total no. of paths squashed before reaching decode",
                   &total_forks_squashed_pre_decode, /* init val */ 0, NULL);

  /* number of loads fulfilled by forwards from the LSQ */
  stat_reg_counter(sdb, "lsq_hits.PP",
                   "no. of loads to valid addrs met by LSQ",
                   &lsq_hits, /* initial value */ 0, /* format */ NULL);

  /* register cache stats */
  if (cache_il1 && (cache_il1 != cache_dl1 && cache_il1 != cache_dl2))
    cache_reg_stats(cache_il1, sdb);
  if (cache_il2 && (cache_il2 != cache_dl1 && cache_il2 != cache_dl2))
    cache_reg_stats(cache_il2, sdb);
  if (cache_dl1)
    cache_reg_stats(cache_dl1, sdb);
  if (cache_dl2)
    cache_reg_stats(cache_dl2, sdb);
  if (itlb)
    cache_reg_stats(itlb, sdb);
  if (dtlb)
    cache_reg_stats(dtlb, sdb);

  stat_reg_int(sdb, "max_spec_level",
               "max number of pending misspeculated branches",
               &max_spec_level, /* init val */ 1, NULL);
  stat_reg_counter(sdb, "ruu_overflows",
                   "number of times decoding was halted by a full RUU",
                   &ruu_overflows, /* init val */ 0, NULL);
  stat_reg_counter(sdb, "lsq_overflows",
                   "number of times decoding was halted by a full LSQ",
                   &lsq_overflows, /* init val */ 0, NULL);
  stat_reg_counter(sdb, "func_unit_overflows",
                   "number of times issue failed due to lack of func units",
                   &func_unit_overflows, /* init val */ 0, NULL);
  stat_reg_counter(sdb, "ifq_overflows",
                   "number of times fetching was halted by a full IFQ",
                   &ifq_overflows, /* init val */ 0, NULL);
  stat_reg_counter(sdb, "num_in_flight_branch_overflows",
                   "number of times fetching was halted by full shadow state",
                   &num_in_flight_branch_overflows, /* init val */ 0, NULL);

  for (i = 0; i < pcstat_nelt; i++)
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
                                      /* initial value */ 0,
                                      /* print format */ (PF_COUNT | PF_PDF),
                                      /* format */ "0x%lx %lu %.2f",
                                      /* print fn */ NULL);
  }

  if (report_fetch)
    fetch_dist = stat_reg_dist(sdb, "fetch:dist.PP",
                               "Number insts fetched per cycle",
                               /* init */ 0, /* arr sz */ ruu_ifq_size + 1,
                               /* bucket sz */ 1, (PF_COUNT | PF_PDF | PF_CDF),
                               NULL, NULL, NULL);
  if (report_issue)
  {
    issue_dist = stat_reg_dist(sdb, "issue:dist.PP",
                               "Number insts issued per cycle",
                               /* init */ 0, /* arr sz */ ruu_issue_width + 1,
                               /* bucket sz */ 1, (PF_COUNT | PF_PDF | PF_CDF),
                               NULL, NULL, NULL);
    useful_issue_dist = stat_reg_dist(sdb, "useful_issue:dist.PP",
                                      "Number committed insts issued per cycle",
                                      /* init */ 0, /* arr sz */ ruu_issue_width + 1,
                                      /* bucket sz */ 1, (PF_COUNT | PF_PDF | PF_CDF),
                                      NULL, NULL, NULL);
  }
  if (report_commit)
    commit_dist = stat_reg_dist(sdb, "commit:dist.PP",
                                "Number insts committed per cycle",
                                /* init */ 0, /* arr sz */ ruu_commit_width + 1,
                                /* bucket sz */ 1, (PF_COUNT | PF_PDF | PF_CDF),
                                NULL, NULL, NULL);
  if (report_post_issue)
    post_issue_dist = stat_reg_dist(sdb, "post_issue:dist.PP",
                                    "Number post_issued insts in RUU per cycle",
                                    /* init */ 0, /* arr sz */ RUU_size + 1,
                                    /* bucket sz */ 1, (PF_COUNT | PF_PDF | PF_CDF),
                                    NULL, NULL, NULL);
  if (report_post_issue)
    post_issue_useful_dist = stat_reg_dist(sdb, "post_issue_useful:dist.PP",
                                           "Number post_issued insts in RUU per cycle "
                                           "that will be committed",
                                           /* init */ 0, /* arr sz */ RUU_size + 1,
                                           /* bucket sz */ 1, (PF_COUNT | PF_PDF | PF_CDF),
                                           NULL, NULL, NULL);
  if (report_useful_insts)
    useful_insts_dist = stat_reg_dist(sdb, "useful_insts:dist.PP",
                                      "Number useful insts in RUU per cycle",
                                      /* init */ 0, /* arr sz */ RUU_size + 1,
                                      /* bucket sz */ 1, (PF_COUNT | PF_PDF | PF_CDF),
                                      NULL, NULL, NULL);
  if (report_ready_insts)
    ready_insts_dist = stat_reg_dist(sdb, "ready_insts:dist.PP",
                                     "Number insts in RUU waiting to issue per cycle",
                                     /* init */ 0, /* arr sz */ RUU_size + 1,
                                     /* bucket sz */ 1, (PF_COUNT | PF_PDF | PF_CDF),
                                     NULL, NULL, NULL);
  if (report_ready_insts)
    miss_indep_insts_dist = stat_reg_dist(sdb, "miss_indep_insts:dist.PP",
                                          "Number insts available to overlap L1 D$ misses",
                                          /* init */ 0, /* arr sz */ RUU_size + 1,
                                          /* bucket sz */ 1, (PF_COUNT | PF_PDF | PF_CDF),
                                          NULL, NULL, NULL);
  if (report_ruu_occ)
    ruu_occ_dist = stat_reg_dist(sdb, "ruu_occ:dist.PP",
                                 "RUU occupancy (all insts) per cycle",
                                 /* init */ 0, /* arr sz */ RUU_size + 1,
                                 /* bucket sz */ 1, (PF_COUNT | PF_PDF | PF_CDF),
                                 NULL, NULL, NULL);
  if (report_imiss_ruu_occ)
    imiss_ruu_occ_dist = stat_reg_dist(sdb, "imiss_ruu_occ:dist.PP",
                                       "RUU occupancy (all insts) per I-miss",
                                       /* init */ 0, /* arr sz */ RUU_size + 1,
                                       /* bucket sz */ 1, (PF_COUNT | PF_PDF | PF_CDF),
                                       NULL, NULL, NULL);
  if (report_branch_info)
    pending_branches_dist = stat_reg_dist(sdb, "pending_branches:dist.PP",
                                          "Number simultaneous unresolved branches",
                                          /* init */ 0, /* arr sz */ RUU_size / 2,
                                          /* bucket sz */ 1, (PF_COUNT | PF_PDF | PF_CDF),
                                          NULL, NULL, NULL);
  if (report_branch_info)
    branch_delay_dist = stat_reg_dist(sdb, "branch_delay:dist.PP",
                                      "Number cycles after fetch til branch resolution",
                                      /* init */ 0, /* arr sz */ 256,
                                      /* bucket sz */ 1, (PF_COUNT | PF_PDF | PF_CDF),
                                      NULL, NULL, NULL);
  if (report_issue_loc)
    issue_loc_dist = stat_reg_dist(sdb, "ruu_issue_loc:dist.PP",
                                   "RUU depth at which (useful) instruction was issued",
                                   /* init */ 0, /* arr sz */ RUU_size,
                                   /* bucket sz */ 1, (PF_COUNT | PF_PDF | PF_CDF),
                                   NULL, NULL, NULL);
  if (report_decode_loc)
    decode_loc_dist = stat_reg_dist(sdb, "ruu_decode_loc:dist.PP",
                                    "RUU depth into which (useful) instruction was enqueued",
                                    /* init */ 0, /* arr sz */ RUU_size,
                                    /* bucket sz */ 1, (PF_COUNT | PF_PDF | PF_CDF),
                                    NULL, NULL, NULL);
  if (report_issue_delay)
    issue_delay_dist = stat_reg_dist(sdb, "issue_delay:dist.PP",
                                     "Time spent waiting for issue once ready",
                                     /* init */ 0, /* arr sz */ 256,
                                     /* bucket sz */ 1, (PF_COUNT | PF_PDF | PF_CDF),
                                     NULL, NULL, NULL);
  if (report_issue_delay)
    operand_delay_dist = stat_reg_dist(sdb, "operand_delay:dist.PP",
                                       "Time spent waiting for operands once eligible for issue",
                                       /* init */ 0, /* arr sz */ 256,
                                       /* bucket sz */ 1, (PF_COUNT | PF_PDF | PF_CDF),
                                       NULL, NULL, NULL);
  if (report_miss_clustering)
  {
    d1miss_cluster_dist = stat_reg_dist(sdb, "d1miss_cluster:dist.PP",
                                        "cycles between D1-misses",
                                        /* init */ 0, /* arr sz */ 256,
                                        /* bucket sz */ 1, (PF_COUNT | PF_PDF | PF_CDF),
                                        NULL, NULL, NULL);
    i1miss_cluster_dist = stat_reg_dist(sdb, "i1miss_cluster:dist.PP",
                                        "cycles between I1-misses",
                                        /* init */ 0, /* arr sz */ 256,
                                        /* bucket sz */ 1, (PF_COUNT | PF_PDF | PF_CDF),
                                        NULL, NULL, NULL);
  }
  if (max_threads > 1)
    forked_dist = stat_reg_dist(sdb, "forked:dist.PP",
                                "Number of forked paths active each cycle",
                                /* init */ 0, /* arr sz */ max_threads,
                                /* bucket sz */ 1, (PF_COUNT | PF_PDF | PF_CDF),
                                NULL, NULL, NULL);

  if (output_cbr_dist)
    cbr_data_dist = stat_reg_sdist(sdb,          /* stat database */
                                   "cbr data",   /* stat variable name */
                                   "0:dyn cnt, " /* stat variable descrip. */
                                   "1:mispr, "
                                   "2:conf mispr",
                                   0, /* dist initial value */
                                   (PF_COUNT | PF_PDF | PF_CDF),
                                   NULL,  /* opt output format */
                                   NULL); /* optional user print func*/
}

/* forward declarations */
static void ruu_init(void);
static void lsq_init(void);
static void rslink_init(int nlinks);
static void eventq_init(void);
static void readyq_init(void);
static void cv_init(void);
static void tracer_init(void);
static void fetch_init(void);

#ifdef USE_DLITE
/* default register state accessor, used by DLite */
static char *                              /* err str, NULL for no err */
simoo_reg_obj(enum dlite_access_t at,      /* access type */
              enum dlite_reg_t rt,         /* reg bank to probe */
              int reg,                     /* register number */
              union dlite_reg_val_t *val); /* input, output */

/* default memory state accessor, used by DLite */
static char *                         /* err str, NULL for no err */
simoo_mem_obj(enum dlite_access_t at, /* access type */
              SS_ADDR_TYPE addr,      /* address to access */
              char *p,                /* input/output buffer */
              int nbytes);            /* size of access */
#endif

/* default machine state accessor, used by DLite and possibly others */
static char *                  /* err str, NULL for no err */
simoo_mstate_obj(FILE *stream, /* output stream */
                 char *cmd,    /* optional command string */
                 int thread);  /* optional thread id */

/* total RS links allocated at program start */
#ifdef HUGE_CFG
#define MAX_RS_LINKS 65536
#else
#define MAX_RS_LINKS 32768
#endif

/*
 * processor core definitions and declarations
 */

/* inst tag type, used to tag an operation instance in the RUU */
typedef unsigned int INST_TAG_TYPE;

/* inst sequence type, used to order instructions in the ready list, if
   this rolls over the ready list order temporarily will get messed up,
   but execution will continue and complete correctly */
typedef unsigned int INST_SEQ_TYPE;

/* total input dependencies possible */
#define MAX_IDEPS 3

/* total output dependencies possible */
#define MAX_ODEPS 2

/* a register update unit (RUU) station, this record is contained in the
   processors RUU, which serves as a collection of ordered reservations
   stations.  The reservation stations capture register results and await
   the time when all operands are ready, at which time the instruction is
   issued to the functional units; the RUU is an order circular queue, in which
   instructions are inserted in fetch (program) order, results are stored in
   the RUU buffers, and later when an RUU entry is the oldest entry in the
   machines, it and its instruction's value is retired to the architectural
   register file in program order, NOTE: the RUU and LSQ share the same
   structure, this is useful because loads and stores are split into two
   operations: an effective address add and a load/store, the add is inserted
   into the RUU and the load/store inserted into the LSQ, allowing the add
   to wake up the load/store when effective address computation has finished */
struct RUU_station
{
  /* inst info */
  SS_INST_TYPE IR;                             /* instruction bits */
  enum ss_opcode op;                           /* decoded instruction opcode */
  SS_ADDR_TYPE PC, next_PC, pred_PC;           /* inst PC, next PC, predicted PC */
  SS_ADDR_TYPE base_pred_PC;                   /* pred PC had branch not forked */
  SS_ADDR_TYPE used_PC;                        /* next fetch PC ultimately used */
  int in_LSQ;                                  /* non-zero if op is in LSQ */
  int ea_comp;                                 /* non-zero if op is an addr comp */
  int recover_inst;                            /* start of mis-speculation? */
  struct bpred_recover_info bpred_recover_rec; /* retstack fixup info */
  struct bpred_update_info b_update_rec;       /* bpred info needed for update */
  int spec_mode;                               /* non-zero if issued in spec_mode */
  int spec_level;                              /* which spec state to use */
  SS_ADDR_TYPE addr;                           /* effective address for ld/st's */
  INST_TAG_TYPE tag;                           /* RUU slot tag, increment to
					   squash operation */
  INST_SEQ_TYPE seq;                           /* instruction sequence, used to
					   sort the ready list and tag inst */
  unsigned int ptrace_seq;                     /* pipetrace sequence number */

  /* instruction status */
  SS_TIME_TYPE ready_to_iss; /* when inst may issue */
  SS_TIME_TYPE issued_at;    /* when inst issued */
  SS_TIME_TYPE ready_time;   /* when operands became available */
  int decoded;               /* decoded by ruu_dispatch and in IQ */
  int queued;                /* operands ready and queued */
  int issued;                /* operation is/was executing */
  int completed;             /* operation has completed execution */

  /* output operand dependency list, these lists are used to
     limit the number of associative searches into the RUU when
     instructions complete and need to wake up dependent insts */
  int onames[MAX_ODEPS];                /* output logical names (NA=unused) */
  struct RS_link *odep_list[MAX_ODEPS]; /* chains to consuming operations */
  struct res_template *fu;              /* a ptr to the func. unit used by
					 * this instr. */

  /* input dependent links, the output chains rooted above use these
     fields to mark input operands as ready, when all these fields have
     been set non-zero, the RUU operation has all of its register
     operands, it may commence execution as soon as all of its memory
     operands are known to be read (see lsq_refresh() for details on
     enforcing memory dependencies) */
  int idep_ready[MAX_IDEPS]; /* input operand ready? */

  /* misc info */
  int l1_miss; /* indicates inst had a D-miss */
  int flag;    /* flag used in tracing dep chains */

  /* multi-path/multi-threaded execution */
  int br_taken;
  int br_pred_taken;
  int conf;          /* conf pred. on this branch */
  int forked;        /* this branch forked */
  int forked_thread; /* thread_id of forked-off thread */
  int taken_thread;
  int nottaken_thread;
  int pred_thread;
  int correct_thread;
  int token2forked;                           /* gave token to forked-off thread? */
  int squashable;                             /* short-term flag: to be squashed? */
  int squashed;                               /* squashed inst; treat as no-op */
  int thread_id;                              /* which thread fetched this inst */
  int pred_path_token;                        /* branches:is this on the pred-path?*/
  int new_pred_path_token;                    /* same as above, for a diff. scheme */
  BITMAP_TYPE(N_SPEC_LEVELS, fork_hist_bmap); /* fork history bitmap */
  BITMAP_ENT_TYPE fork_hist_bmap_ptr;         /* ptr to this inst's pos'n in bmap */
};

/* non-zero if all register operands are ready, update with MAX_IDEPS */
#define OPERANDS_READY(RS) \
  ((RS)->idep_ready[0] && (RS)->idep_ready[1] && (RS)->idep_ready[2])

/* register update unit, combination of reservation stations and reorder
   buffer device, organized as a circular queue */
static struct RUU_station *RUU; /* register update unit */
static int RUU_head, RUU_tail;  /* RUU head and tail pointers */
static int RUU_num;             /* num entries currently in RUU */

/* allocate and initialize register update unit (RUU) */
static void
ruu_init(void)
{
  RUU = calloc(RUU_size, sizeof(struct RUU_station));
  if (!RUU)
    fatal("out of virtual memory");

  RUU_num = 0;
  RUU_head = RUU_tail = 0;
}

/* dump the contents of the RUU */
static void
ruu_dumpent(struct RUU_station *rs, /* ptr to RUU station */
            int index,              /* entry index */
            FILE *stream,           /* output stream */
            int header)             /* print header? */
{
  if (header)
    fprintf(stream, "idx: %2d: opcode: %s, inst: `",
            index, SS_OP_NAME(rs->op));
  else
    fprintf(stream, "       opcode: %s, inst: `",
            SS_OP_NAME(rs->op));
  ss_print_insn(rs->IR, rs->PC, stream);
  fprintf(stream, "'\n");
  fprintf(stream, "         PC: 0x%08x, NPC: 0x%08x\n"
                  "            (pred_PC: 0x%08x, base_pred_PC: 0x%08x)\n",
          rs->PC, rs->next_PC, rs->pred_PC, rs->base_pred_PC);
  fprintf(stream, "         in_LSQ: %s, ea_comp: %s, recover_inst: %s\n",
          rs->in_LSQ ? "t" : "f",
          rs->ea_comp ? "t" : "f",
          rs->recover_inst ? "t" : "f");
  fprintf(stream,
          "         spec_mode: %s, spec_lev: %d, addr: 0x%08x, tag: 0x%08x\n",
          rs->spec_mode ? "t" : "f", rs->spec_level, rs->addr, rs->tag);
  fprintf(stream, "         seq: 0x%08x, ptrace_seq: 0x%08x\n",
          rs->seq, rs->ptrace_seq);
  fprintf(stream,
          "         decoded: %s, queued: %s, issued: %s, completed: %s\n",
          rs->decoded ? "t" : "f",
          rs->queued ? "t" : "f",
          rs->issued ? "t" : "f",
          rs->completed ? "t" : "f");
  fprintf(stream, "         operands ready: %s\n",
          OPERANDS_READY(rs) ? "t" : "f");
  fprintf(stream, "         operands available: %.0f, ready_to_iss: %.0f\n",
          (double)rs->ready_time, (double)rs->ready_to_iss);
  fprintf(stream, "         issued_at: %.0f\n", (double)rs->issued_at);
  fprintf(stream, "         conf: %s, forked: %s, squashed: %s\n",
          (rs->conf == HighConf) ? "hi" : "lo",
          rs->forked ? "t" : "f", rs->squashed ? "t" : "f");
  fprintf(stream, "         fork_hist_bmap: 0x");
  BITMAP_PRINT_BITSTR(rs->fork_hist_bmap, THREADS_BMAP_SZ, stream);
  fprintf(stream, "\n         fork_hist_bmap_ptr: %d\n",
          rs->fork_hist_bmap_ptr);
  fprintf(stream, "         thread_id: %d\n", rs->thread_id);
}

/* dump the contents of the RUU */
static void
ruu_dump(FILE *stream) /* output stream */
{
  int num, head;
  struct RUU_station *rs;

  fprintf(stream, "** RUU state **\n");
  fprintf(stream, "RUU_head: %d, RUU_tail: %d\n", RUU_head, RUU_tail);
  fprintf(stream, "RUU_num: %d\n", RUU_num);

  num = RUU_num;
  head = RUU_head;
  while (num)
  {
    rs = &RUU[head];
    ruu_dumpent(rs, rs - RUU, stream, /* header */ TRUE);
    head = (head + 1) % RUU_size;
    num--;
  }
}

/*
 * load/store queue (LSQ): holds loads and stores in program order, indicating
 * status of load/store access:
 *
 *   - issued: address computation complete, memory access in progress
 *   - completed: memory access has completed, stored value available
 *   - squashed: memory access was squashed, ignore this entry
 *
 * loads may execute when:
 *   1) register operands are ready, and
 *   2) memory operands are ready (no earlier unresolved store)
 *
 * loads are serviced by:
 *   1) previous store at same address in LSQ (hit latency), or
 *   2) data cache (hit latency + miss latency)
 *
 * stores may execute when:
 *   1) register operands are ready
 *
 * stores are serviced by:
 *   1) depositing store value into the load/store queue
 *   2) writing store value to the store buffer (plus tag check) at commit
 *   3) writing store buffer entry to data cache when cache is free
 *
 * NOTE: the load/store queue can bypass a store value to a load in the same
 *   cycle the store executes (using a bypass network), thus stores complete
 *   in effective zero time after their effective address is known
 */
static struct RUU_station *LSQ; /* load/store queue */
static int LSQ_head, LSQ_tail;  /* LSQ head and tail pointers */
static int LSQ_num;             /* num entries currently in LSQ */

/*
 * input dependencies for stores in the LSQ:
 *   idep #0 - operand input (value that is store'd)
 *   idep #1 - effective address input (address of store operation)
 */
#define STORE_OP_INDEX 0
#define STORE_ADDR_INDEX 1

#define STORE_OP_READY(RS) ((RS)->idep_ready[STORE_OP_INDEX])
#define STORE_ADDR_READY(RS) ((RS)->idep_ready[STORE_ADDR_INDEX])

/* allocate and initialize the load/store queue (LSQ) */
static void
lsq_init(void)
{
  LSQ = calloc(LSQ_size, sizeof(struct RUU_station));
  if (!LSQ)
    fatal("out of virtual memory");

  LSQ_num = 0;
  LSQ_head = LSQ_tail = 0;
}

/* dump the contents of the LSQ */
static void
lsq_dump(FILE *stream) /* output stream */
{
  int num, head;
  struct RUU_station *rs;

  fprintf(stream, "** LSQ state **\n");
  fprintf(stream, "LSQ_head: %d, LSQ_tail: %d\n", LSQ_head, LSQ_tail);
  fprintf(stream, "LSQ_num: %d\n", LSQ_num);

  num = LSQ_num;
  head = LSQ_head;
  while (num)
  {
    rs = &LSQ[head];
    ruu_dumpent(rs, rs - LSQ, stream, /* header */ TRUE);
    head = (head + 1) % LSQ_size;
    num--;
  }
}

/* 
 * thread_info structure; and functions for maintaining thread state
 */
struct thread_state
{
  SS_ADDR_TYPE fetch_regs_PC; /* current fetch PC */
  SS_ADDR_TYPE fetch_pred_PC; /* predicted next fetch PC */
  SS_TIME_TYPE fetchable;     /* fetch stalled til when? */
  int base_priority;          /* intrinisic fetch priority */
  int priority;               /* curr fetch priority */
  int last_inst_missed;       /* did last fetch I$-miss? */
  int last_inst_tmissed;      /* did last fetch ITLB-miss? */
  int pred_path_token;        /* is this thr on pred path? */
#ifdef DEBUG_PRED_PRI
  int new_pred_path_token; /* same as above, for a 
						 * different scheme */
#endif
  BITMAP_ENT_TYPE fork_hist_bmap_ptr;         /* this thread's bmap slot */
  BITMAP_TYPE(N_SPEC_LEVELS, fork_hist_bmap); /* this thread's br-hist bmap*/

  struct bpred_btb_ent *retstack; /* return-address stack */
  int retstack_tos;               /* ret-stack top-of-stack */

  int fetched_this_cycle;       /* controls multi-path fetch */
  int lines_fetched_this_cycle; /* "" */
  int did_decode;               /* did any insts from this
						 * thread make it to decode? */
  int spec_mode;                /* on a mis-speculated path? */
  int spec_level;               /* speculated past this many
						 * branches (count from 1) */
  int squashed;                 /* flag for cleanup */
  int valid;                    /* is this record valid? */
};

/* The central structure to keep track of per-thread state */
static struct thread_state thread_info[N_THREAD_RECS];

/* which thread is currently at leaf of predicted path? */
int pred_thread = INIT_THREAD;

#ifdef DEBUG_PRED_PRI
/* current value of a valid new_pred_path_token; any other value is
 * invalid */
static int new_pred_path_token_val = 1;
#endif

/* keeps data about how many insts each thread has in the ruu.  designed
 * to be qsorted */
struct ruu_occ_by_thread_t
{
  int count; /* # of insts in RUU */
  int thread;
};

static struct ruu_occ_by_thread_t ruu_occ_by_thread[N_THREAD_RECS];

/* sum CURRENT priority values */
static int
sum_priorities(void)
{
  int t, sum = 0;
  for (t = 0; t < N_THREAD_RECS; t++)
    if (thread_info[t].valid == TRUE && thread_info[t].fetchable <= sim_cycle)
      sum += thread_info[t].priority;

  if (sum < 0)
    panic("thread-priority sum overflowed or got wedged");

  return sum;
}

/* reset current-priority values to the base values */
static void
reset_priorities(void)
{
  int t;

  if (!fetch_pred_pri)
    for (t = 0; t < N_THREAD_RECS; t++)
      thread_info[t].priority = thread_info[t].base_priority;
  else
  {
    for (t = 0; t < N_THREAD_RECS; t++)
      if (thread_info[t].pred_path_token && thread_info[t].valid == TRUE)
        thread_info[t].priority = pred_pri_lev;
      else if (thread_info[t].valid == TRUE)
        thread_info[t].priority = 1;
#ifdef DEBUG_DEF_VALS
      else
        thread_info[t].priority = 0;
#endif
  }
}

int ruu_occ_cmp(const void *el1, const void *el2)
{
  int count1 = ((struct ruu_occ_by_thread_t *)el1)->count;
  int count2 = ((struct ruu_occ_by_thread_t *)el2)->count;

  if (count1 > count2)
    return 1;
  else if (count2 > count1)
    return -1;

  return 0;
}

static int next_by_ruu_thread = 0;
static int saw_first_ruu_thread = 0;

/* reset current-priority values based on RUU occupancy.  Meant to be
 * called every cycle.  Starvation shouldn't occur; some thread will
 * eventually have so few active insts that it gets high-priority */
static void
reset_priorities_by_ruu()
{
  int i, n, t;

  dassert(fetch_ruu_pri);

  /* initialize ruu_occ_by_thread[] */
  for (t = 0; t < N_THREAD_RECS; t++)
  {
    ruu_occ_by_thread[t].thread = t;
    ruu_occ_by_thread[t].count = 0;
  }

  /* count per-thread RUU occs */
  for (i = RUU_head, n = 0; n < RUU_num; i = (i + 1) % RUU_size, n++)
  {
    t = RUU[i].thread_id;
    if (thread_info[t].valid == TRUE && thread_info[t].fetchable <= sim_cycle && RUU[i].squashed != TRUE && !RUU[i].issued)
    {
      dassert(!RUU[i].completed);
      dassert(ruu_occ_by_thread[t].thread == t);
      ruu_occ_by_thread[t].count++;
    }
  }

  /* sort in ascending order */
  qsort(ruu_occ_by_thread, N_THREAD_RECS, sizeof(struct ruu_occ_by_thread_t),
        ruu_occ_cmp);
  next_by_ruu_thread = 0;
  saw_first_ruu_thread = 0;
}

/* change a thread/path's next fetch address, and update it's
 * next-time-fetchable to reflect a penalty of 'penalty; cycles */
static void
thread_fetch_redirect(int thread, SS_ADDR_TYPE next_PC, int penalty)
{
  dassert(thread_info[thread].valid);
  thread_info[thread].fetch_pred_PC = next_PC;
  thread_info[thread].fetchable = sim_cycle + penalty;
}

/* change a thread/path's next-time-fetchable, charging it 'penalty' cycles */
static void
thread_set_fetch_penalty(int thread, int penalty)
{
  dassert(thread_info[thread].valid == TRUE);
  thread_info[thread].fetchable = sim_cycle + penalty;
}

static int
reset_fetch(int go_backwards)
{
  int t;
  for (t = 0; t < N_THREAD_RECS; t++)
  {
    if (thread_info[t].lines_fetched_this_cycle > max_cache_lines && max_cache_lines != 0)
      fatal("error: thread_info[t].lines_fetched_this_cycle > max_cache_lines"
            "\n   Check whether max_cache_lines has been set correctly");
    thread_info[t].fetched_this_cycle = FALSE;
    thread_info[t].lines_fetched_this_cycle = 0;
  }

  if (fetch_ruu_pri)
    reset_priorities_by_ruu();

  return !go_backwards;
}

static int
num_fetchable_threads()
{
  int t, num = 0;
  for (t = 0; t < N_THREAD_RECS; t++)
    if (thread_info[t].fetchable <= sim_cycle && thread_info[t].valid == TRUE && thread_info[t].squashed != TRUE && (ruu_include_spec || thread_info[t].spec_mode != TRUE))
      num++;

  return num;
}

/* pick the next thread/path from which to fetch, based on 
 * 'last_thread_fetched'.  Current policy is priority-based, which
 * can allow various duty-cycles among threads (or other policies). 
 * 'fetched_this_cycle' is used to allow multiple threads to be fetched
 * from in a cycle, while ensuring we fetch only once per cycle from
 * any given thread */
/* FIXME: maybe instead this should be round-robin among threads
 * with non-zero priorities?  Lower-priority threads would go to
 * zero faster... */
static int
get_next_fetch_thread(int *pri, int go_backwards)
{
  int i, saw_ready = FALSE;
  /* next candidate for fetching is the one with the next thread id */
  int t;
  /* fetch from highest-priority thread */
  int largest_pri = 0;
  int best_candidate = -1;

  /* if we've gone through a "round" of fetching, start a new round */
  if (fetch_pri_pol == Omni_Pri || fetch_pri_pol == Two_Omni_Pri || fetch_pri_pol == Pred_Pri)
  {
    if (sum_priorities() == 0)
      reset_priorities();
  }

  /* 
   * we handle ruu-pri completely differently 
   */
  if (fetch_pri_pol == Ruu_Pri)
  {
    while (next_by_ruu_thread < N_THREAD_RECS)
    {
      best_candidate = ruu_occ_by_thread[next_by_ruu_thread].thread;
      if (thread_info[best_candidate].valid == TRUE && thread_info[best_candidate].squashed != TRUE && !thread_info[best_candidate].fetched_this_cycle && thread_info[best_candidate].fetchable <= sim_cycle && (ruu_include_spec || thread_info[best_candidate].spec_mode != TRUE))
      {
        thread_info[best_candidate].fetched_this_cycle = TRUE;

        if (!saw_first_ruu_thread)
        {
          saw_first_ruu_thread = 1;
          *pri = ruu_pri_lev;
        }
        else
          *pri = 1;

        next_by_ruu_thread++;
        return best_candidate;
      }
      else
        next_by_ruu_thread++;
    }
    return -1;
  }

  /*
   * if we got here, we're not doing ruu-pri
   */

  switch (fetch_pri_pol)
  {
  case Simple_RR:
  case Old_RR:
  case Omni_Pri:
  case Two_Omni_Pri:
  case Pred_Pri:
    t = (last_thread_fetched + 1) % N_THREAD_RECS;
    break;

  case Pred_RR:
  case Pred_Pri2:
    dassert(pred_thread >= 0 && pred_thread < N_THREAD_RECS);
    t = (last_thread_fetched + (go_backwards ? N_THREAD_RECS - 1 : 1)) % N_THREAD_RECS;
    break;

  default:
    panic("unrecognized fetch-pri-pol");
  }

  /* other pri policies */
  for (i = 0;
       i < N_THREAD_RECS;
       t = (t +
            (((fetch_pri_pol == Pred_Pri2 || fetch_pri_pol == Pred_RR) && go_backwards)
                 ? N_THREAD_RECS - 1
                 : 1)) %
           N_THREAD_RECS,
      i++)
    if (thread_info[t].fetchable <= sim_cycle /* STILL NEED TO FIX THIS */
        && thread_info[t].valid == TRUE && thread_info[t].squashed != TRUE && (!thread_info[t].fetched_this_cycle || fetch_pri_pol == Simple_RR || fetch_pri_pol == Pred_RR) && (ruu_include_spec || thread_info[t].spec_mode != TRUE))
    {
      switch (fetch_pri_pol)
      {
      case Simple_RR:
      case Old_RR:
      case Pred_RR:
      case Pred_Pri2:
        /* found a fetchable thread */
        dassert(thread_info[t].base_priority == -1 && thread_info[t].priority == -1);
        last_thread_fetched = t;
        if (fetch_pri_pol != Simple_RR && fetch_pri_pol != Pred_RR && fetch_pri_pol != Pred_Pri2)
          thread_info[t].fetched_this_cycle = TRUE;
        *pri = 1;
        return t;

      case Omni_Pri:
      case Two_Omni_Pri:
      case Pred_Pri:
        saw_ready = TRUE;
        if (thread_info[t].priority > largest_pri)
        {
          assert(thread_info[t].priority >= 0 && thread_info[t].base_priority > 0);
          largest_pri = thread_info[t].priority;
          best_candidate = t;
        }
        break;

      default:
        panic("unknown fetch priority policy");
      }
    }

  switch (fetch_pri_pol)
  {
  case Simple_RR:
  case Old_RR:
  case Pred_RR:
  case Pred_Pri2:
    /* if we got here, no threads fetchable this cycle */
    return -1;

  case Omni_Pri:
  case Two_Omni_Pri:
  case Pred_Pri:
    if (best_candidate >= 0)
    {
      /* found a fetchable thread */
      last_thread_fetched = best_candidate;
      /* pass this thread's current fetch priority to caller, then
	   * adjust it according to the policy */
      *pri = thread_info[best_candidate].priority;
      if (fetch_pred_pri)
        thread_info[best_candidate].priority = 0;
      else
        thread_info[best_candidate].priority--;
      /* indicate we fetched from this thread this cycle */
      thread_info[best_candidate].fetched_this_cycle = TRUE;
    }
    else
      assert(!saw_ready);

    return best_candidate; /* will be -1 if no threads are fetchable */

  default:
    assert(FALSE);
  }
}

/* Note: to dump the contents of the thread_info structure, use fetch_dump() */

/* 
 * retired-inst list
 *
 * We maintain a list of already-retired insts so that we can look back
 * in time and gather some statistics.  The list is kept sorted from most 
 * recent to oldest.
 */
struct retired_inst
{
  struct RUU_station ruu_entry;
  struct retired_inst *next, *prev;
};

static struct retired_inst *retired_insts_head, *retired_insts_tail;
static struct retired_inst retired_inst_list[KEEP_N_RETIRED_INSTS];

static void
retired_inst_list_init(void)
{
  int i;

  retired_inst_list[0].ruu_entry.issued_at = 0;
  retired_inst_list[0].prev = NULL;
  retired_inst_list[0].next = &retired_inst_list[1];

  for (i = 1; i < KEEP_N_RETIRED_INSTS - 1; i++)
  {
    retired_inst_list[i].ruu_entry.issued_at = 0;
    retired_inst_list[i].prev = &retired_inst_list[i - 1];
    retired_inst_list[i].next = &retired_inst_list[i + 1];
  }

  retired_inst_list[KEEP_N_RETIRED_INSTS - 1].ruu_entry.issued_at = 0;
  retired_inst_list[KEEP_N_RETIRED_INSTS - 1].prev =
      &retired_inst_list[KEEP_N_RETIRED_INSTS - 2];

  retired_inst_list[KEEP_N_RETIRED_INSTS - 1].next = NULL;

  retired_insts_head = &retired_inst_list[0];
  retired_insts_tail = &retired_inst_list[KEEP_N_RETIRED_INSTS - 1];
}

static void
add_retired_insts(struct RUU_station *rs)
{
  struct retired_inst *item = retired_insts_tail, *ptr;
  item->ruu_entry = *rs;

  /* insert at tail? */
  if (rs->issued_at <= retired_insts_tail->prev->ruu_entry.issued_at)
    /* nothing to do */
    return;

  /* unlink tail */
  retired_insts_tail->prev->next = NULL;
  retired_insts_tail = retired_insts_tail->prev;

  /* insert at head? */
  if (rs->issued_at >= retired_insts_head->ruu_entry.issued_at)
  {
    item->next = retired_insts_head;
    item->prev = NULL;
    retired_insts_head->prev = item;
    retired_insts_head = item;
    return;
  }

  /* insert in middle of list */
  for (ptr = retired_insts_head->next; ptr; ptr = ptr->next)
    if (rs->issued_at >= ptr->ruu_entry.issued_at)
    {
      item->next = ptr;
      item->prev = ptr->prev;
      ptr->prev->next = item;
      ptr->prev = item;
      return;
    }

  panic("add_retired_insts(): should never get here!");
}

static int
num_retired_insts_issued_after(SS_TIME_TYPE time)
{
  struct retired_inst *ptr;
  int count = 0;

  for (ptr = retired_insts_head; ptr; ptr = ptr->next)
  {
    if (ptr->ruu_entry.issued_at > time)
      count++;
    else
      /* rest of list is too old */
      break;
  }

  return count;
}

/*
 * RS_LINK defs and decls
 */

/* a reservation station link: this structure links elements of a RUU
   reservation station list; used for ready instruction queue, event queue, and
   output dependency lists; each RS_LINK node contains a pointer to the RUU
   entry it references along with an instance tag, the RS_LINK is only valid if
   the instruction instance tag matches the instruction RUU entry instance tag;
   this strategy allows entries in the RUU can be squashed and reused without
   updating the lists that point to it, which significantly improves the
   performance of (all to frequent) squash events */
struct RS_link
{
  struct RS_link *next;   /* next entry in list */
  struct RUU_station *rs; /* referenced RUU resv station */
  INST_TAG_TYPE tag;      /* inst instance sequence number */
  union {
    SS_TIME_TYPE when; /* time stamp of entry (for eventq) */
    INST_SEQ_TYPE seq; /* inst sequence */
    int opnum;         /* input/output operand number */
  } x;
};

/* RS link free list, grab RS_LINKs from here, when needed */
static struct RS_link *rslink_free_list;

/* NULL value for an RS link */
#define RSLINK_NULL_DATA \
  {                      \
    NULL, NULL, 0        \
  }
static struct RS_link RSLINK_NULL = RSLINK_NULL_DATA;

/* Create and initialize an RS link */
#define RSLINK_INIT(RSL, RS) \
  ((RSL).next = NULL, (RSL).rs = (RS), (RSL).tag = (RS)->tag)

/* non-zero if RS link is NULL */
#define RSLINK_IS_NULL(LINK) ((LINK)->rs == NULL)

/* non-zero if RS link is to a valid (non-squashed) entry */
#define RSLINK_VALID(LINK) ((LINK)->tag == (LINK)->rs->tag)

/* extra RUU reservation station pointer */
#define RSLINK_RS(LINK) ((LINK)->rs)

/* get a new RS link record */
#define RSLINK_NEW(DST, RS)                    \
  {                                            \
    struct RS_link *n_link;                    \
    if (!rslink_free_list)                     \
      panic("out of rs links");                \
    n_link = rslink_free_list;                 \
    rslink_free_list = rslink_free_list->next; \
    n_link->next = NULL;                       \
    n_link->rs = (RS);                         \
    n_link->tag = n_link->rs->tag;             \
    (DST) = n_link;                            \
  }

/* free an RS link record */
#define RSLINK_FREE(LINK)            \
  {                                  \
    struct RS_link *f_link = (LINK); \
    f_link->rs = NULL;               \
    f_link->tag = 0;                 \
    f_link->next = rslink_free_list; \
    rslink_free_list = f_link;       \
  }

/* FIXME: could this be faster!!! */
/* free an RS link list */
#define RSLINK_FREE_LIST(LINK)                              \
  {                                                         \
    struct RS_link *fl_link, *fl_link_next;                 \
    for (fl_link = (LINK); fl_link; fl_link = fl_link_next) \
    {                                                       \
      fl_link_next = fl_link->next;                         \
      RSLINK_FREE(fl_link);                                 \
    }                                                       \
  }

/* initialize the free RS_LINK pool */
static void
rslink_init(int nlinks) /* total number of RS_LINK available */
{
  int i;
  struct RS_link *link;

  rslink_free_list = NULL;
  for (i = 0; i < nlinks; i++)
  {
    link = calloc(1, sizeof(struct RS_link));
    if (!link)
      fatal("out of virtual memory");
    link->next = rslink_free_list;
    rslink_free_list = link;
  }
}

/* service all functional unit release events, this function is called
   once per cycle, and it used to step the BUSY timers attached to each
   functional unit in the function unit resource pool, as long as a functional
   unit's BUSY count is > 0, it cannot be issued an operation */
static void
ruu_release_fu(void)
{
  int i;

  /* walk all resource units, decrement busy counts by one */
  for (i = 0; i < fu_pool->num_resources; i++)
  {
    /* resource is released when BUSY hits zero */
    if (fu_pool->resources[i].busy > 0)
      fu_pool->resources[i].busy--;
  }
}

/*
 * the execution unit event queue implementation follows, the event queue
 * indicates which instruction will complete next, the writeback handler
 * drains this queue
 */

/* pending event queue, sorted from soonest to latest event (in time), NOTE:
   RS_LINK nodes are used for the event queue list so that it need not be
   updated during squash events */
static struct RS_link *event_queue;

/* initialize the event queue structures */
static void
eventq_init(void)
{
  event_queue = NULL;
}

/* dump the contents of the event queue */
static void
eventq_dump(FILE *stream) /* output stream */
{
  struct RS_link *ev;

  fprintf(stream, "** event queue state **\n");

  for (ev = event_queue; ev != NULL; ev = ev->next)
  {
    /* is event still valid? */
    if (RSLINK_VALID(ev))
    {
      struct RUU_station *rs = RSLINK_RS(ev);

      fprintf(stream, "idx: %2d: @ %.0f\n",
              rs - (rs->in_LSQ ? LSQ : RUU), (double)ev->x.when);
      ruu_dumpent(rs, rs - (rs->in_LSQ ? LSQ : RUU),
                  stream, /* !header */ FALSE);
    }
  }
}

/* insert an event for RS into the event queue, event queue is sorted from
   earliest to latest event, event and associated side-effects will be
   apparent at the start of cycle WHEN */
static void
eventq_queue_event(struct RUU_station *rs, SS_TIME_TYPE when)
{
  struct RS_link *prev, *ev, *new_ev;

  if (rs->completed)
    panic("event completed");

  if (when <= sim_cycle)
    panic("event occurred in the past");

  /* get a free event record */
  RSLINK_NEW(new_ev, rs);
  new_ev->x.when = when;

  /* locate insertion point */
  for (prev = NULL, ev = event_queue;
       ev && ev->x.when < when;
       prev = ev, ev = ev->next)
    ;

  if (prev)
  {
    /* insert middle or end */
    new_ev->next = prev->next;
    prev->next = new_ev;
  }
  else
  {
    /* insert at beginning */
    new_ev->next = event_queue;
    event_queue = new_ev;
  }
}

/* return the next event that has already occurred, returns NULL when no
   remaining events or all remaining events are in the future */
static struct RUU_station *
eventq_next_event(void)
{
  struct RS_link *ev;

  if (event_queue && event_queue->x.when <= sim_cycle)
  {
    /* unlink and return first event on priority list */
    ev = event_queue;
    event_queue = event_queue->next;

    /* event still valid? */
    if (RSLINK_VALID(ev))
    {
      struct RUU_station *rs = RSLINK_RS(ev);

      /* reclaim event record */
      RSLINK_FREE(ev);

      /* event is valid, return resv station */
      return rs;
    }
    else
    {
      /* reclaim event record */
      RSLINK_FREE(ev);

      /* receiving inst was squashed, return next event */
      return eventq_next_event();
    }
  }
  else
  {
    /* no event or no event is ready */
    return NULL;
  }
}

/*
 * the ready instruction queue implementation follows, the ready instruction
 * queue indicates which instruction have all of there *register* dependencies
 * satisfied, instruction will issue when 1) all memory dependencies for
 * the instruction have been satisfied (see lsq_refresh() for details on how
 * this is accomplished) and 2) resources are available; ready queue is fully
 * constructed each cycle before any operation is issued from it -- this
 * ensures that instruction issue priorities are properly observed; NOTE:
 * RS_LINK nodes are used for the event queue list so that it need not be
 * updated during squash events
 */

/* the ready instruction queue */
static struct RS_link *ready_queue;

/* initialize the event queue structures */
static void
readyq_init(void)
{
  ready_queue = NULL;
}

/* dump the contents of the ready queue */
static void
readyq_dump(FILE *stream) /* output stream */
{
  struct RS_link *link;

  fprintf(stream, "** ready queue state **\n");

  for (link = ready_queue; link != NULL; link = link->next)
  {
    /* is entry still valid? */
    if (RSLINK_VALID(link))
    {
      struct RUU_station *rs = RSLINK_RS(link);

      ruu_dumpent(rs, rs - (rs->in_LSQ ? LSQ : RUU),
                  stream, /* header */ TRUE);
    }
  }
}

/* insert ready node into the ready list using ready instruction scheduling
   policy; currently two policies are available:
   Aggressive: 
   	memory and long latency operands, and branch instructions first
        then
   	all other instructions, oldest instructions first
   Non-aggressive:
   	sorted strictly by program order

  Aggressive works well because branches pass through the machine quicker
  which works to reduce branch misprediction latencies, and very long latency
  instructions (such loads and multiplies) get priority since they are very
  likely on the program's critical path */
static void
readyq_enqueue(struct RUU_station *rs) /* RS to enqueue */
{
  struct RS_link *prev, *node, *new_node;

  /* node is now queued */
  if (rs->queued)
    panic("node is already queued");
  assert(rs->ready_time);
  rs->queued = TRUE;

  /* get a free ready list node */
  RSLINK_NEW(new_node, rs);
  new_node->x.seq = rs->seq;

  /* 
   * locate insertion point 
   */

  if (aggressive_issue && (rs->in_LSQ || (SS_OP_FLAGS(rs->op) & (F_LONGLAT | F_CTRL))))
  {
    /* insert mem ops, br's, and long latency ops at the head of the queue 
       * (but in program order among themselves) */
    for (prev = NULL, node = ready_queue;
         node && node->x.seq < rs->seq && (node->rs->in_LSQ || (SS_OP_FLAGS(node->rs->op) & (F_LONGLAT | F_CTRL)));
         prev = node, node = node->next)
      ;
  }
  else if (aggressive_issue)
  {
    /* find the end of the high-priority insts */
    for (prev = NULL, node = ready_queue;
         node && (node->rs->in_LSQ || (SS_OP_FLAGS(node->rs->op) & (F_LONGLAT | F_CTRL)));
         prev = node, node = node->next)
      ;
  }
  else
  {
    prev = NULL;
    node = ready_queue;
  }

  /* now either (1) we've found the insertion point if this is a high-
   * priority inst, or (2) we've found the end of the high-priority insts,
   * or (3) we're doing straight in-order enqueuing.
   *
   * If (1), insert the inst.  If (2) or (3), look at seq numbers to
   * find the in-order insertion point. */

  if (!aggressive_issue || (aggressive_issue && !(rs->in_LSQ || (SS_OP_FLAGS(rs->op) & (F_LONGLAT | F_CTRL)))))
  {
    /* FIXME: for multi-path; this means priority is by fetch order */
    for (;
         node && node->x.seq < rs->seq;
         prev = node, node = node->next)
      ;
  }

  if (prev)
  {
    /* insert middle or end */
    new_node->next = prev->next;
    prev->next = new_node;
  }
  else
  {
    /* insert at beginning */
    new_node->next = ready_queue;
    ready_queue = new_node;
  }
}

/*
 * the create vector maps a logical register to a creator in the RUU (and
 * specific output operand) or the architected register file (if RS_link
 * is NULL)
 */

/* an entry in the create vector */
struct CV_link
{
  struct RUU_station *rs; /* creator's reservation station */
  int odep_num;           /* specific output operand */
};

/* a NULL create vector entry */
static struct CV_link CVLINK_NULL = {NULL, 0};

/* get a new create vector link */
#define CVLINK_INIT(CV, RS, ONUM) ((CV).rs = (RS), (CV).odep_num = (ONUM))

/* size of the create vector (one entry per architected register) */
#define CV_BMAP_SZ (BITMAP_SIZE(SS_TOTAL_REGS))

/* the create vector, NOTE: speculative copy on write storage provided
   for fast recovery during wrong path execute (see spec_mode_recover() for
   details on this process */
static struct CV_link create_vector[SS_TOTAL_REGS];
static struct CV_link spec_create_vector[N_THREAD_RECS][N_SPEC_LEVELS][SS_TOTAL_REGS];

/* these arrays shadow the create vector and indicate when a register was
   last created */
static SS_TIME_TYPE create_vector_rt[SS_TOTAL_REGS];
static SS_TIME_TYPE spec_create_vector_rt[N_THREAD_RECS][N_SPEC_LEVELS][SS_TOTAL_REGS];

/* read a create vector entry */
#define CREATE_VECTOR(N, THREAD)                   \
  (spec_level                                      \
       ? spec_create_vector[THREAD][spec_level][N] \
       : create_vector[N])

/* read a create vector timestamp entry */
#define CREATE_VECTOR_RT(N, THREAD)                   \
  (spec_level                                         \
       ? spec_create_vector_rt[THREAD][spec_level][N] \
       : create_vector_rt[N])

/* set a create vector entry */
#define SET_CREATE_VECTOR(N, L, THREAD)                  \
  ((thread_info[THREAD].spec_mode == TRUE)               \
       ? spec_create_vector[THREAD][spec_level][N] = (L) \
       : (create_vector[N] = (L)))

/* initialize the create vector */
static void
cv_init(void)
{
  int i, t, s;

  /* Note: static decl's should already initialize everything to 0... */

  /* initially all registers are valid in the architected register file,
     i.e., the create vector entry is CVLINK_NULL */
  for (i = 0; i < SS_TOTAL_REGS; i++)
  {
    create_vector[i] = CVLINK_NULL;
    dEval(create_vector_rt[i] = 0);
  }

  /* initially all spec_create info is empty */
  for (t = 0; t < N_THREAD_RECS; t++)
    for (s = 0; s < N_SPEC_LEVELS; s++)
      for (i = 0; i < SS_TOTAL_REGS; i++)
      {
        spec_create_vector[t][s][i] = CVLINK_NULL;
        dEval(spec_create_vector_rt[t][s][i] = 0);
      }
}

/* dependency index names */
static char *dep_names[SS_TOTAL_REGS] = {
    "n/a", "$r1", "$r2", "$r3", "$r4", "$r5", "$r6", "$r7", "$r8", "$r9",
    "$r10", "$r11", "$r12", "$r13", "$r14", "$r15", "$r16", "$r17", "$r18",
    "$r19", "$r20", "$r21", "$r22", "$r23", "$r24", "$r25", "$r26", "$r27",
    "$r28", "$r29", "$r30", "$r31",
    "$f0", "$f1", "$f2", "$f3", "$f4", "$f5", "$f6", "$f7", "$f8", "$f9",
    "$f10", "$f11", "$f12", "$f13", "$f14", "$f15", "$f16", "$f17", "$f18",
    "$f19", "$f20", "$f21", "$f22", "$f23", "$f24", "$f25", "$f26", "$f27",
    "$f28", "$f29", "$f30", "$f31",
    "$hi", "$lo", "$fcc", "$tmp",
    "n/a", "n/a"};

/* dump the contents of the create vector */
static void
cv_dump(FILE *stream) /* output stream; thread no. */
{
  int i, t, spec_level;
  struct CV_link ent;

  fprintf(stream, "** create vector state **\n");

  for (t = 0; t < N_THREAD_RECS; t++)
  {
    if (!thread_info[t].valid)
      continue;

    fprintf(stream, "thread_id: %d\n", t);

    for (i = 0; i < SS_TOTAL_REGS; i++)
    {
      spec_level = thread_info[t].spec_level;
      ent = CREATE_VECTOR(i, t);
      if (!ent.rs)
        fprintf(stream, "   [%4s]: from architected reg file\n",
                dep_names[i]);
      else
        fprintf(stream, "   [%4s]: from %s, idx: %d\n",
                dep_names[i], (ent.rs->in_LSQ ? "LSQ" : "RUU"),
                ent.rs - (ent.rs->in_LSQ ? LSQ : RUU));
    }
  }
}

/*
 *  RUU_COMMIT() - instruction retirement pipeline stage
 */

/* this function commits the results of the oldest completed entries from the
   RUU and LSQ to the architected reg file, stores in the LSQ will commit
   their store data to the data cache at this point as well */
static void
ruu_commit(void)
{
  int lat, events, i;
  int committed = 0, discarded = 0;
  struct RUU_station *rs;

  /* all values must be retired to the architected reg file in program order */
  /* for multi-path/multi-threaded execution, squashed RUU/LSQ entries are
   * flagged during mispredict recovery, but the entries are not deallocated;
   * instead they propagate to commit and are discarded at that point.
   * Entries discarded in this fashion do consume commit b/w. */
  while (RUU_num > 0 && committed < ruu_commit_width)
  {
    rs = &(RUU[RUU_head]);

    /* sanity checks */
    if (!rs->completed)
    {
      /* at least RUU entry must be complete */
      assert(!rs->squashed);
      break;
    }
    /* one or the other is true */
    assert(!!(rs->spec_mode == FALSE) ^ !!rs->squashed);

    /* default commit events */
    events = 0;

    /* load/stores must retire load/store queue entry as well */
    if (rs->ea_comp)
    {
      /* load/store, retire head of LSQ as well */
      if (LSQ_num <= 0 || !LSQ[LSQ_head].in_LSQ)
        panic("RUU out of sync with LSQ");

      /* load/store operation must be complete */
      if (!LSQ[LSQ_head].completed)
      {
        /* load/store operation is not yet complete */
        assert(!rs->squashed);
        break;
      }

      /* unsquashed stores must retire their store value to the cache 
	   * at commit; try to get a store port (func, unit allocation) */
      if ((SS_OP_FLAGS(LSQ[LSQ_head].op) & (F_MEM | F_STORE)) == (F_MEM | F_STORE) && !LSQ[LSQ_head].squashed)
      {
        struct res_template *fu;

        if (!infinite_fu)
          fu = res_get(fu_pool, SS_OP_FUCLASS(LSQ[LSQ_head].op));
        if (infinite_fu || fu)
        {
          /* reserve the functional unit */
          if (!infinite_fu && fu->master->busy)
            panic("functional unit already in use");

          /* schedule functional unit release event */
          if (!infinite_fu)
            fu->master->busy = fu->issuelat;

          /* go to the data cache */
          if (cache_dl1)
          {
            /* commit store value to D-cache */
            /* NOTE: Stores are treated as almost instantaneous,
		       *   (infinite L0->L1 write buffering), so we
		       *   don't have to do anything here to simulate
		       *   perfect caches.  */
            if (cache_dl1_perfect)
              lat = 1;
            else
              lat =
                  cache_access(cache_dl1, Write,
                               (LSQ[LSQ_head].addr & ~3),
                               NULL, 4, sim_cycle, NULL, NULL);
            if (lat > cache_dl1_lat[0] + cache_dl1_lat[1])
              events |= PEV_CACHEMISS;
          }

          /* all loads and stores must access D-TLB.  */
          /* NOTE: We don't need to do any perfect-TLB fakery here 
		   * either, since TLB latency for stores is also ignored 
		   * for purposes of counting cycles. */
          /* FIXME (KS): On a TLB miss, commit shouldn't be allowed
		   * to continue. Actually, shouldn't TLB access occur at
		   * issue? */
          if (dtlb)
          {
            /* access the D-TLB */
            if (tlb_perfect)
              lat = 1;
            else
              lat =
                  cache_access(dtlb, Read, (LSQ[LSQ_head].addr & ~3),
                               NULL, 4, sim_cycle, NULL, NULL);
            if (lat > 1)
              events |= PEV_TLBMISS;
          }

          /* Commit the memory access, locking the allocated memory 
		   * in place */
          MEM_ACCESS_COMMIT(LSQ[LSQ_head].addr);
        }
        else
        {
          /* no store ports left, cannot continue to commit insts */
          break;
        }
      }

      /* invalidate load/store operation instance */
      LSQ[LSQ_head].tag++;

      /* indicate to pipeline trace that this instruction retired */
      dassert(LSQ[LSQ_head].spec_mode != UNKNOWN);
      if (!rs->squashed && ptrace_level != PTRACE_FUNSIM)
      {
        ptrace_newstage(LSQ[LSQ_head].ptrace_seq,
                        LSQ[LSQ_head].thread_id,
                        PST_COMMIT,
                        events | (LSQ[LSQ_head].spec_mode ? PEV_SPEC_MODE : 0));
        ptrace_endinst(LSQ[LSQ_head].ptrace_seq,
                       LSQ[LSQ_head].thread_id);
      }

      /* commit head of LSQ as well */
      LSQ_head = (LSQ_head + 1) % LSQ_size;
      LSQ_num--;
    }

    /* if we non-speculatively update branch-predictor, do it here */
    if (pred && !bpred_spec_update && !bpred_perf_update &&
        (SS_OP_FLAGS(rs->op) & F_CTRL) && !rs->squashed)
    {
      SS_INST_TYPE inst = rs->IR;
#ifdef RETSTACK_DEBUG_PRINTOUT
      if (rs->op == JR && ((RS) == 31) && rs->pred_PC != rs->next_PC)
        fprintf(stderr, "BAD: 0x%x (t%d, %d) predicted 0x%x, got 0x%x\n",
                rs->PC, rs->thread_id, rs->ptrace_seq,
                rs->pred_PC, rs->next_PC);
#endif
      bpred_update(pred, rs->PC, rs->next_PC, OFS,
                   /* taken? */ rs->next_PC != (rs->PC +
                                                sizeof(SS_INST_TYPE)),
                   /* pred taken? */ rs->base_pred_PC != (rs->PC +
                                                          sizeof(SS_INST_TYPE)),
                   /* correct pred? */ rs->base_pred_PC == rs->next_PC,
                   /* opcode */ rs->op, (RS) == 31,
                   /* gate */ TRUE,
                   /* hybrid component */ FALSE,
                   /* dir predictor update pointer */ rs->b_update_rec,
                   /* retstack fixup rec */ &rs->bpred_recover_rec);
    }
    if (bconf && !bconf_spec_update && !bconf_perf_update &&
        (SS_OP_FLAGS(rs->op) & F_COND) && !rs->squashed)
    {
      int br_taken = (rs->next_PC != (rs->PC + sizeof(SS_INST_TYPE)));
      int br_pred_taken = (rs->base_pred_PC !=
                           (rs->PC + sizeof(SS_INST_TYPE)));

      bconf_update(bconf, rs->PC,
                   /* branch taken? */ br_taken,
                   /* correct pred? */ br_pred_taken == br_taken,
                   /* conf pred?    */ rs->conf);
    }

    /* don't need stack copy or local-history copy */
    if (pred && (SS_OP_FLAGS(rs->op) & F_CTRL))
    {
      if (pred->retstack.patch_level == RETSTACK_PATCH_WHOLE && rs->bpred_recover_rec.contents.stack_copy != NULL)
      {
        free(rs->bpred_recover_rec.contents.stack_copy);
        rs->bpred_recover_rec.contents.stack_copy = NULL;
      }
    }

    /* put this instruction on a list of insts that have committed but
       * might be needed for statistics-gathering */
    if (report_miss_indep_insts && !rs->squashed)
      add_retired_insts(rs);

    /* invalidate RUU operation instance */
    rs->tag++;

    /* indicate to pipeline trace that this instruction retired */
    if (!rs->squashed && ptrace_level != PTRACE_FUNSIM)
    {
      ptrace_newstage(rs->ptrace_seq, rs->thread_id, PST_COMMIT,
                      events | (rs->spec_mode ? PEV_SPEC_MODE : 0));
      ptrace_endinst(rs->ptrace_seq, rs->thread_id);
    }

    /* commit head entry of RUU */
    RUU_head = (RUU_head + 1) % RUU_size;
    RUU_num--;

    if (rs->squashed)
      discarded++;

    committed++;

    for (i = 0; i < MAX_ODEPS; i++)
    {
      if (rs->odep_list[i])
        panic("retired instruction has odeps\n");
    }
  }

  if (report_commit && done_priming)
    stat_add_sample(commit_dist, committed - discarded);
}

/*
 *  RUU_RECOVER() - squash mispredicted microarchitecture state
 */

/* recover per thread retstack state and call bpred_retstack_recover();
 * also call bpred_history_recover if appropriate */
static void
hydra_bpred_recover(int curr_thread,                        /* thread-id of recovering branch inst */
                    SS_ADDR_TYPE currPC,                    /* PC    of    "         "         */
                    SS_ADDR_TYPE nextPC,                    /* known or predicted next PC      */
                    enum ss_opcode op,                      /* instruction type */
                    struct bpred_recover_info *recover_rec, /* recovery info */
                    int *dont_commit,
                    enum pipestage stage, /* caller's pipestage */
                    char *caller_string)  /* caller of this func, f/ printouts*/
{
  int tosp = -1;
  int gate = TRUE;

#ifdef LIST_FETCH
  fprintf(outfile, "recover, PC 0x%08x, t%d, in %s, cycle %.0f\n",
          currPC, curr_thread, caller_string, (double)sim_cycle);
#endif

  if (per_thread_retstack == OneStackPredOnly && (fetch_pri_pol == Pred_RR || fetch_pri_pol == Pred_Pri2) && curr_thread != pred_thread)
    gate = FALSE;

  bpred_retstack_recover(pred, currPC, &tosp, gate, recover_rec);
  if (per_thread_retstack == PerThreadTOSP)
  {
    assert(tosp >= 0);
    thread_info[curr_thread].retstack_tos = tosp;
  }
  else if (per_thread_retstack == PerThreadStacks)
  {
    if (retstack_patch_level != RETSTACK_PATCH_NONE)
    {
      thread_info[curr_thread].retstack_tos = recover_rec->tos;

      if (retstack_patch_level == RETSTACK_PATCH_PTR_DATA)
        thread_info[curr_thread].retstack[recover_rec->tos].target = recover_rec->contents.tos_value;
      else if (retstack_patch_level == RETSTACK_PATCH_WHOLE)
      {
        assert(recover_rec->contents.stack_copy);
        memcpy(thread_info[curr_thread].retstack,
               recover_rec->contents.stack_copy,
               retstack_size * sizeof(struct bpred_btb_ent));
      }
#ifdef RETSTACK_DEBUG_PRINTOUT
      {
        int i, n;
        fprintf(stderr, "Restoring in %s (t%d):\n",
                caller_string, curr_thread);
        for (i = thread_info[curr_thread].retstack_tos, n = 0;
             n < retstack_size;
             n++, i = (i + retstack_size - 1) % retstack_size)
          fprintf(stderr, "\t\t\t%d: 0x%x\n",
                  i, thread_info[curr_thread].retstack[i].target);
      }
#endif
    }
  }

  /* if doing speculative history updates, repair that bpred state, too */
  bpred_history_recover(pred, currPC,
                        /* is this branch taken? */ nextPC != (currPC + SS_INST_SIZE),
                        /* branch type */ op,
                        /* caller's pipestage */ stage,
                        /* checkpointed history  */ recover_rec);
}

/* recover processor microarchitecture state back to point of the
   mis-predicted branch at RUU[BRANCH_INDEX] */
static void
ruu_recover(int branch_index) /* index of mis-pred branch */
{
  int i, RUU_index, LSQ_index;
  int RUU_prev_tail = RUU_tail, LSQ_prev_tail = LSQ_tail;

  /* recover from the tail of the RUU towards the head until the branch index
     is reached, this direction ensures that the LSQ can be synchronized with
     the RUU */

  /* go to first element to squash */
  RUU_index = (RUU_tail + (RUU_size - 1)) % RUU_size;
  LSQ_index = (LSQ_tail + (LSQ_size - 1)) % LSQ_size;

  /* traverse to older insts until the mispredicted branch is encountered */
  while (RUU_index != branch_index)
  {
    /* the RUU should not drain since the mispredicted branch will remain */
    if (!RUU_num)
      panic("empty RUU");

    /* should meet up with the tail first */
    if (RUU_index == RUU_head)
      panic("RUU head and tail broken");

    /* is this operation an effective addr calc for a load or store? */
    if (RUU[RUU_index].ea_comp)
    {
      /* should be at least one load or store in the LSQ */
      if (!LSQ_num)
        panic("RUU and LSQ out of sync");

      if (LSQ[LSQ_index].squashable)
      {
        /* Deallocate the space the might have been allocated for
		 misspeculated memory accesses */
        MEM_ACCESS_SQUASH(LSQ[LSQ_index].addr);

        /* recover any resources consumed by the memory operation */
        for (i = 0; i < MAX_ODEPS; i++)
        {
          RSLINK_FREE_LIST(LSQ[LSQ_index].odep_list[i]);
          /* blow away the consuming op list */
          LSQ[LSQ_index].odep_list[i] = NULL;
        }

        /* remove from IQ if appropriate */
        if (LSQ[LSQ_index].decoded)
          IIQ_occ--;

        /* squash this LSQ entry */
        LSQ[LSQ_index].tag++;
        LSQ[LSQ_index].squashed = TRUE;
        LSQ[LSQ_index].completed = TRUE;

        /* indicate in pipetrace that this instruction was squashed */
        if (ptrace_level != PTRACE_FUNSIM)
          ptrace_endinst(LSQ[LSQ_index].ptrace_seq,
                         LSQ[LSQ_index].thread_id);

        if (squash_remove)
          LSQ_num--;
      }

      /* go to next earlier LSQ slot */
      LSQ_prev_tail = LSQ_index;
      LSQ_index = (LSQ_index + (LSQ_size - 1)) % LSQ_size;
    }

    if (RUU[RUU_index].squashable)
    {
      /* recover any resources used by this RUU operation */
      if (pred->retstack.patch_level == RETSTACK_PATCH_WHOLE && RUU[RUU_index].bpred_recover_rec.contents.stack_copy)
      {
        free(RUU[RUU_index].bpred_recover_rec.contents.stack_copy);
        RUU[RUU_index].bpred_recover_rec.contents.stack_copy = NULL;
      }
      for (i = 0; i < MAX_ODEPS; i++)
      {
        RSLINK_FREE_LIST(RUU[RUU_index].odep_list[i]);
        /* blow away the consuming op list */
        RUU[RUU_index].odep_list[i] = NULL;
      }

      /* remove from IQ if appropriate */
      if (RUU[RUU_index].decoded)
      {
        if (SS_OP_FLAGS(RUU[RUU_index].op) & F_FCOMP)
          FIQ_occ--;
        else
          IIQ_occ--;
      }

      /* reset the func. unit used by this instr -- if fu is occupied,
	   * it will be free for the NEXT cycle.  This simple mechanism
	   * works because we know all later insts that might use this
	   * fu get squashed, too. */
      if (RUU[RUU_index].fu && (RUU[RUU_index].fu->master->busy != 0))
        RUU[RUU_index].fu->master->busy = 1;

      /* squash this RUU entry */
      RUU[RUU_index].tag++;
      RUU[RUU_index].squashed = TRUE;
      RUU[RUU_index].completed = TRUE;

      /* indicate in pipetrace that this instruction was squashed */
      if (ptrace_level != PTRACE_FUNSIM)
        ptrace_endinst(RUU[RUU_index].ptrace_seq,
                       RUU[RUU_index].thread_id);

      if (squash_remove)
        RUU_num--;
    }

    /* go to next earlier slot in the RUU */
    RUU_prev_tail = RUU_index;
    RUU_index = (RUU_index + (RUU_size - 1)) % RUU_size;
  }

  /* if we removed RUU/LSQ entries, reset head/tail pointers to point to 
   * the mis-predicted branch */
  if (squash_remove)
  {
    RUU_tail = RUU_prev_tail;
    LSQ_tail = LSQ_prev_tail;
  }
}

/*
 *  RUU_WRITEBACK() - instruction result writeback pipeline stage
 */

/* 
 * Walk dep chain, flagging 
 */
static void
flag_odep_chain(struct RUU_station *rs)
{
  int i;
  struct RS_link *olink, *olink_next;

  if (rs->flag)
    return;
  else
    rs->flag = TRUE;

  for (i = 0; i < MAX_ODEPS; i++)
    if (rs->onames[i] != NA)
      for (olink = rs->odep_list[i]; olink; olink = olink_next)
      {
        if (RSLINK_VALID(olink))
          flag_odep_chain(olink->rs);
        olink_next = olink->next;
      }
}

/* forward declarations */
static void tracer_recover(struct RUU_station *rs_branch, int penalty,
                           int ifq_patch_only);
static void tracer_recover_wrapper(int curr_thread, SS_ADDR_TYPE regs_PC,
                                   SS_ADDR_TYPE next_PC, int recover_inst,
                                   int penalty);
static void thread_cleanup(struct RUU_station *rs_branch);
static void post_thread_cleanup(struct RUU_station *rs_branch);

/* writeback completed operation results from the functional units to RUU,
   at this point, the output dependency chains of completing instructions
   are also walked to determine if any dependent instruction now has all
   of its register operands, if so the (nearly) ready instruction is inserted
   into the ready instruction queue */
static void
ruu_writeback(void)
{
  int i, s, t, n, curr, num_pending_branches = 0;
  struct RUU_station *rs;
  int new_token_holder = -1;
#ifdef DEBUG_PRED_PRI
  struct RUU_station *last_branch_seen;
#endif

  /* FIXME: this doesn't count uncond. branches and doesn't count branches
   * in the IFQ */
  for (i = 0, curr = RUU_head; i < RUU_num; i++, curr = (curr + 1) % RUU_size)
  {
    if (!report_branch_info)
      break;

    if ((SS_OP_FLAGS(RUU[curr].op) & (F_CTRL | F_COND)) == (F_CTRL | F_COND) && !RUU[curr].completed)
      num_pending_branches++;
  }

  dassert(num_pending_branches <= RUU_num);
  if (report_branch_info && done_priming)
    stat_add_sample(pending_branches_dist, num_pending_branches);

  /* service all completed events */
  while ((rs = eventq_next_event()))
  {
    /* RS has completed execution and (possibly) produced a result */
    if (!OPERANDS_READY(rs) || rs->queued || !rs->issued || rs->completed)
      panic("inst completed and !ready, !issued, or completed");

    /* operation has completed */
    rs->completed = TRUE;

    if (rs->squashed)
    {
      dassert(rs->spec_mode);
      continue;
    }

    if ((SS_OP_FLAGS(rs->op) & (F_CTRL | F_COND)) == (F_CTRL | F_COND))
    {
      if (report_branch_info && done_priming)
      {
        int delay = sim_cycle - rs->ready_to_iss + extra_decode_lat + 1;
        assert(delay > 0);
        stat_add_sample(branch_delay_dist, delay);
      }
    }

    /* 
       * Branches require special treatment
       */

    if (SS_OP_FLAGS(rs->op) & F_CTRL)
    {
      /* sanity checks */
      if (rs->in_LSQ)
        panic("branch uses LSQ???");

      /* if this branch forked (regardless of whether it was mispredicted),
	   * or it is any mispredicted branch, squash the incorrect paths,
	   * recover processor state, and re-init fetch to correct path */
      if (rs->forked || rs->recover_inst)
      {
        thread_cleanup(rs);
        ruu_recover(rs - RUU);
        tracer_recover(rs, ruu_branch_penalty, FALSE);
        post_thread_cleanup(rs);
      }

      /* if fork-pruning or doing fetch_pred_pri, give token to 
	   * correct thread */
      /* FIXME: replace this fork-pruning and fetch-pred-pri
	   * with scheme below */
      if (rs->forked && (fork_prune || fetch_pred_pri))
      {
        assert(rs->pred_path_token || !fork_prune);
        if (rs->token2forked && thread_info[rs->forked_thread].valid == KILLED && thread_info[rs->thread_id].valid == TRUE)
          thread_info[rs->thread_id].pred_path_token = TRUE;
        else if (!rs->token2forked && thread_info[rs->thread_id].valid == KILLED && thread_info[rs->forked_thread].valid == TRUE)
          thread_info[rs->forked_thread].pred_path_token = TRUE;
      }
      if ((fork_prune || fetch_pred_pri) && rs->pred_path_token)
        thread_info[rs->thread_id].pred_path_token = TRUE;

      /* if doing pred_rr or pred_pri2, give token to correct, 
	     predicted, path */
      if (rs->forked && (fetch_pri_pol == Pred_RR || fetch_pri_pol == Pred_Pri2) && rs->new_pred_path_token && rs->squashed != TRUE && rs->br_taken != rs->br_pred_taken)
      {
        /* if forked branch was mispredicted, token shifts to this
	       * branch's not-predicted path, and belongs with the
	       * thread that's at the new pred-path's leaf */
#ifdef DEBUG_PRED_PRI
        last_branch_seen = rs;
        pred_thread = -1;
#endif
        if (rs->br_taken)
          new_token_holder = rs->taken_thread;
        else
          new_token_holder = rs->nottaken_thread;

        for (i = (rs - RUU + 1) % RUU_size, n = 0;
             i != RUU_tail;
             i = (i + 1) % RUU_size, n++)
          if ((SS_OP_FLAGS(RUU[i].op) & F_CTRL) && RUU[i].thread_id == new_token_holder && RUU[i].squashed != TRUE)
          {
            RUU[i].new_pred_path_token = TRUE;
#ifdef DEBUG_PRED_PRI
            last_branch_seen = &RUU[i];
#endif
            if (RUU[i].completed && RUU[i].forked)
              new_token_holder = RUU[i].correct_thread;
            else if (RUU[i].forked)
              new_token_holder = RUU[i].pred_thread;
            else
              dassert(new_token_holder == RUU[i].thread_id);

            dassert(new_token_holder >= 0 && new_token_holder < N_THREAD_RECS);
          }
          else
            continue;

        pred_thread = new_token_holder;
        dassert(pred_thread >= 0 && pred_thread < N_THREAD_RECS);
#ifdef DEBUG_PRED_PRI
        thread_info[new_token_holder].new_pred_path_token = ++new_pred_path_token_val;
        assert(last_branch_seen->new_pred_path_token == TRUE);
#endif
      }
      else if ((rs->br_taken != rs->br_pred_taken || ((SS_OP_FLAGS(rs->op) & (F_UNCOND)) && rs->used_PC != rs->next_PC)) && rs->squashed != TRUE && rs->new_pred_path_token && (fetch_pri_pol == Pred_RR || fetch_pri_pol == Pred_Pri2))
      {
        dassert(!rs->forked);

        /* if unforked branch was mispredicted, reclaim the
	       * token from any children */
        pred_thread = rs->thread_id;
#ifdef DEBUG_PRED_PRI
        thread_info[rs->thread_id].new_pred_path_token = ++new_pred_path_token_val;
#endif
      }
      /* else correctly predicted branch or branch not on 
	   * currently known pred path */

      /* bpred module might want to know about the misprediction,
	   * eg to fix ret-addr stack */
      if (rs->recover_inst)
      {
        hydra_bpred_recover(rs->thread_id, rs->PC, rs->next_PC, rs->op,
                            &rs->bpred_recover_rec,
                            &rs->b_update_rec.dont_commit,
                            Writeback, "writeback");
        assert(thread_info[rs->thread_id].spec_mode == rs->spec_mode &&
               thread_info[rs->thread_id].spec_level == rs->spec_level);
      }
    }
    else
    {
      dassert(!rs->forked);
      dassert(!rs->recover_inst);
      dassert(rs->pred_PC == rs->next_PC);
    }

    /* 
       * Continue writeback of the branch/control instruction 
       */

    /* if we speculatively update branch-predictor, do it here */
    if (pred && bpred_spec_update && !bpred_perf_update &&
        !rs->in_LSQ && (SS_OP_FLAGS(rs->op) & F_CTRL) && !rs->squashed)
    {
      SS_INST_TYPE inst = rs->IR;
      bpred_update(pred, rs->PC, rs->next_PC, OFS,
                   /* taken? */ rs->next_PC != (rs->PC +
                                                sizeof(SS_INST_TYPE)),
                   /* pred taken? */ rs->base_pred_PC != (rs->PC +
                                                          sizeof(SS_INST_TYPE)),
                   /* correct pred? */ rs->base_pred_PC == rs->next_PC,
                   /* opcode */ rs->op, (RS) == 31,
                   /* gate */ TRUE,
                   /* hybrid component */ FALSE,
                   /* dir predictor update pointer */ rs->b_update_rec,
                   /* retstack fixup rec */ &rs->bpred_recover_rec);
    }
    if (bconf && bconf_spec_update && !bconf_perf_update &&
        !rs->in_LSQ && (SS_OP_FLAGS(rs->op) & F_COND) && !rs->squashed)
    {
      int br_taken = (rs->next_PC != (rs->PC + sizeof(SS_INST_TYPE)));
      int br_pred_taken = (rs->base_pred_PC !=
                           (rs->PC + sizeof(SS_INST_TYPE)));

      bconf_update(bconf, rs->PC,
                   /* branch taken? */ br_taken,
                   /* correct pred? */ br_pred_taken == br_taken,
                   /* conf pred?    */ rs->conf);
    }

    /* entered writeback stage, indicate in pipe trace */
    dassert(rs->spec_mode != UNKNOWN);
    if (ptrace_level != PTRACE_FUNSIM)
      ptrace_newstage(rs->ptrace_seq, rs->thread_id, PST_WRITEBACK,
                      /* events */
                      ((rs->recover_inst || rs->forked) ? PEV_MPDETECT : 0) | (rs->spec_mode ? PEV_SPEC_MODE : 0));

    /* Count number of insts in RUU that could have been used
       * to hide the miss latency */
    if (rs->l1_miss && !rs->spec_mode && report_miss_indep_insts)
    {
      int j, curr;
      int indep_insts = 0;

      flag_odep_chain(rs);

      for (j = 0, curr = RUU_head;
           j < RUU_num;
           j++, curr = (curr + 1) % RUU_size)
      {
        if (!RUU[curr].flag && !RUU[curr].spec_mode && (RUU[curr].issued_at > rs->issued_at || RUU[curr].queued))
          indep_insts++;
        RUU[curr].flag = FALSE;
      }
      indep_insts += num_retired_insts_issued_after(rs->issued_at);

      if (report_miss_indep_insts && done_priming)
        stat_add_sample(miss_indep_insts_dist, indep_insts);
    }

    /* broadcast results to consuming operations; this is more efficiently
         accomplished by walking the output dependency chains of the
	 completed instruction */
    for (i = 0; i < MAX_ODEPS; i++)
    {
      if (rs->onames[i] != NA && !rs->squashed)
      {
        struct CV_link link;
        struct RS_link *olink, *olink_next;

        /* update all threads' spec-create-vectors if they still see 
	       * this inst as last writer; future operations get value from 
	       * architected reg file or later creator */
        for (t = 0; t < N_THREAD_RECS; t++)
        {
          /* FIXME: I think this loop is a big efficiency problem */

          int thread_max_spec_level;

          if (thread_info[t].valid == FALSE)
            continue;

          thread_max_spec_level = thread_info[t].spec_level;
          for (s = 0; s <= thread_max_spec_level; s++)
          {
            link = spec_create_vector[t][s][rs->onames[i]];

            if (/* !NULL */ link.rs && /* refs RS */ (link.rs == rs && link.odep_num == i))
            {
              assert(s >= rs->spec_level);
              /* the result can now be read from a physical 
			 * register; indicate this as so */
              spec_create_vector[t][s][rs->onames[i]] = CVLINK_NULL;
              spec_create_vector_rt[t][s][rs->onames[i]] = sim_cycle;
            }
            /* else, creator invalidated or there's another creator*/
          }
        }
        if (rs->spec_mode == FALSE)
        {
          /* also update the non-speculative create vector; future
		     operations get value from later creator or architected
		     reg file */
          dassert(rs->spec_level == 0);
          link = create_vector[rs->onames[i]];
          if (/* !NULL */ link.rs && /* refs RS */ (link.rs == rs && link.odep_num == i))
          {
            /* the result can now be read from a physical register,
			 indicate this as so */
            create_vector[rs->onames[i]] = CVLINK_NULL;
            create_vector_rt[rs->onames[i]] = sim_cycle;
          }
          /* else, creator invalidated or there is another creator */
        }

        /* walk output list, queue up ready operations */
        for (olink = rs->odep_list[i]; olink; olink = olink_next)
        {
          if (RSLINK_VALID(olink))
          {
            if (olink->rs->idep_ready[olink->x.opnum])
              panic("output dependence already satisfied");

            /* input is now ready */
            olink->rs->idep_ready[olink->x.opnum] = TRUE;

            /* are all the register operands of target ready? */
            if (OPERANDS_READY(olink->rs))
            {
              /* yes! enqueue instruction as ready, */
              if (!olink->rs->in_LSQ || ((SS_OP_FLAGS(olink->rs->op) & (F_MEM | F_STORE)) == (F_MEM | F_STORE)))
              {
                olink->rs->ready_time = sim_cycle;
                readyq_enqueue(olink->rs);
              }
              /* else, ld op, issued when no mem conflict */
            }
          }

          /* grab link to next element prior to free */
          olink_next = olink->next;

          /* free dependence link element */
          RSLINK_FREE(olink);
        }
        /* blow away the consuming op list */
        rs->odep_list[i] = NULL;

      } /* if not NA output */
    }   /* for all outputs */
  }     /* for all writeback events */
}

/*
 *  LSQ_REFRESH() - memory access dependence checker/scheduler
 */

/* this function locates ready instructions whose memory dependencies have
   been satisfied, this is accomplished by walking the LSQ for loads, looking
   for blocking memory dependency condition (e.g., earlier store with an
   unknown address) */
#ifdef HUGE_CFG
#define MAX_STD_UNKNOWNS 16384
#elif PLUS_CFG
#define MAX_STD_UNKNOWNS 8192
#elif INF_RUU
#define MAX_STD_UNKNOWNS 32768
#else
#define MAX_STD_UNKNOWNS 8192
#endif
static void
lsq_refresh(void)
{
  int i, j, index, n_std_unknowns;
  SS_ADDR_TYPE std_unknowns[MAX_STD_UNKNOWNS];

  /* scan entire queue for ready loads: scan from oldest instruction
     (head) until we reach the tail or an unresolved store, after which no
     other instruction will become ready */
  for (i = 0, index = LSQ_head, n_std_unknowns = 0;
       i < LSQ_num;
       i++, index = (index + 1) % LSQ_size)
  {
    /* ignore squashed entries */
    if (LSQ[index].squashed)
      continue;

    /* terminate search for ready loads after first unresolved store,
	 as no later load could be resolved in its presence */
    if (/* store? */
        (SS_OP_FLAGS(LSQ[index].op) & (F_MEM | F_STORE)) == (F_MEM | F_STORE))
    {
      if (!STORE_ADDR_READY(&LSQ[index]))
      {
        /* FIXME: a later STD + STD known could hide the STA unknown */
        /* sta unknown, blocks all later loads, stop search */
        break;
      }
      else if (!OPERANDS_READY(&LSQ[index]))
      {
        /* sta known, but std unknown, may block a later store, record
		 this address for later referral, we use an array here because
		 for most simulations the number of entries to search will be
		 very small */
        if (n_std_unknowns == MAX_STD_UNKNOWNS)
          fatal("STD unknown array overflow, increase MAX_STD_UNKNOWNS");
        std_unknowns[n_std_unknowns++] = LSQ[index].addr;
      }
      else /* STORE_ADDR_READY() && OPERANDS_READY() */
      {
        /* a later STD known hides an earlier STD unknown */
        for (j = 0; j < n_std_unknowns; j++)
        {
          if (std_unknowns[j] == /* STA/STD known */ LSQ[index].addr)
            std_unknowns[j] = /* bogus addr */ 0;
        }
      }
    }

    if (/* load? */ (SS_OP_FLAGS(LSQ[index].op) & F_LOAD) && /* !queued? */ !LSQ[index].queued && /* !waiting? */ !LSQ[index].issued && /* !completed? */ !LSQ[index].completed && /* regs ready? */ OPERANDS_READY(&LSQ[index]))
    {
      /* no STA unknown conflict (because we got to this check), check for
	     a STD unknown conflict */
      for (j = 0; j < n_std_unknowns; j++)
      {
        /* found a relevant STD unknown? */
        if (std_unknowns[j] == LSQ[index].addr)
          break;
      }
      if (j == n_std_unknowns)
      {
        /* no STA or STD unknown conflicts, put load on ready queue */
        LSQ[index].ready_time = sim_cycle;
        readyq_enqueue(&LSQ[index]);
      }
    }
  }
}

/*
 *  RUU_ISSUE() - issue instructions to functional units
 */

/* attempt to issue all operations in the ready queue; insts in the ready
   instruction queue have all register dependencies satisfied, this function
   must then 1) ensure the instructions memory dependencies have been satisfied
   (see lsq_refresh() for details on this process) and 2) a function unit
   is available in this cycle to commence execution of the operation; if all
   goes well, the function unit is allocated, a writeback event is scheduled,
   and the instruction begins execution.  Note the ready queue establishes
   issue priority.
*/
static void
ruu_issue(void)
{
  int i, curr, load_lat, tlb_lat, oplat = -1;
  int n_issued, n_issued_useful, n_int_issued, n_fp_issued;
  struct RS_link *node, *next_node;
  struct RUU_station *rs;
  struct res_template *fu;
  int num_post_issue = 0, num_post_issue_useful = 0,
      num_useful_insts = 0, num_ready_insts = 0, issue_depth;

  /* 
   * Collect some stats about what kind of insts are in RUU 
   */
  for (i = 0, curr = RUU_head; i < RUU_num; i++, curr = (curr + 1) % RUU_size)
  {
    if (!report_post_issue && !report_useful_insts && !report_ready_insts)
      break;

    if (RUU[curr].issued)
    {
      num_post_issue++;

      if (!RUU[curr].spec_mode)
        num_post_issue_useful++;
    }
    else if (!RUU[curr].spec_mode) /* note spec_mode supersets squashed */
    {
      num_useful_insts++;

      if (RUU[curr].queued)
        num_ready_insts++;
    }
  }
  if (report_post_issue && done_priming)
  {
    stat_add_sample(post_issue_dist, num_post_issue);
    stat_add_sample(post_issue_useful_dist, num_post_issue_useful);
  }
  if (report_useful_insts && done_priming)
    stat_add_sample(useful_insts_dist, num_useful_insts);
  if (report_ready_insts && done_priming)
    stat_add_sample(ready_insts_dist, num_ready_insts);
  if (report_ruu_occ && done_priming)
    stat_add_sample(ruu_occ_dist, RUU_num);

  /* FIXME: could be a little more efficient when scanning the ready queue */

  /* copy and then blow away the ready list, NOTE: the ready list is
     always totally reclaimed each cycle, and instructions that are not
     issue are explicitly reinserted into the ready instruction queue,
     this management strategy ensures that the ready instruction queue
     is always properly sorted */
  node = ready_queue;
  ready_queue = NULL;

  /* visit all ready instructions (i.e., insts whose register input
     dependencies have been satisfied, stop issue when no more instructions
     are available or issue bandwidth is exhausted */
  for (n_issued = 0, n_issued_useful = 0, n_int_issued = 0, n_fp_issued = 0;
       node && n_issued < ruu_issue_width && n_int_issued < ruu_int_issue_width && n_fp_issued < ruu_fp_issue_width;
       node = next_node)
  {
    next_node = node->next;
    rs = RSLINK_RS(node);

    /* still valid? */
    if (RSLINK_VALID(node) && !rs->squashed)
    {
      /* sanity check */
      if (!OPERANDS_READY(rs) || !rs->queued || rs->issued || rs->completed)
        panic("issued inst !ready, issued, or completed");
      dassert(rs->spec_mode != UNKNOWN);

      /* node is now un-queued */
      rs->queued = FALSE;

      /* is this inst in fact allowed to issue yet?  This is where
	   * we account for extra decode cycles */
      /* FIXME: there's surely a more efficient way to handle these
	   * extra cycles...probably sorting the readyq by time as well
	   * as program order */
      if (rs->ready_to_iss > sim_cycle)
      {
        /* not ready to issue; requeue */
        readyq_enqueue(rs);
        goto inst_requeued;
      }

      /* 
	   * issue operation, both reg and mem deps have been satisfied 
	   */
      if (rs->in_LSQ && ((SS_OP_FLAGS(rs->op) & (F_MEM | F_STORE)) == (F_MEM | F_STORE)))
      {
        /* FIXME: shouldn't D_TLB access occur here? */

        /* stores complete in effectively zero time, result is
		 written into the load/store queue, the actual store into
		 the memory system occurs when the instruction is retired
		 (see ruu_commit()) */
        rs->decoded = FALSE;
        rs->issued = TRUE;
        rs->issued_at = sim_cycle;
        rs->completed = TRUE;

        if (rs->onames[0] || rs->onames[1])
          panic("store creates result");

        if (rs->recover_inst)
          panic("mis-predicted store");

        /* entered execute stage, indicate in pipe trace */
        if (ptrace_level != PTRACE_FUNSIM)
          ptrace_newstage(rs->ptrace_seq, rs->thread_id, PST_WRITEBACK,
                          (rs->spec_mode ? PEV_SPEC_MODE : 0));

        /* We issued an inst */
        n_issued++;
        n_int_issued++;
        IIQ_occ--;
        if (!rs->spec_mode)
          n_issued_useful++;
      }
      else
      {
        /* issue the instruction to a functional unit */
        if (SS_OP_FUCLASS(rs->op) != NA)
        {
          oplat = fu_oplat_arr[SS_OP_FUCLASS(rs->op)];
          if (!infinite_fu)
            fu = res_get(fu_pool, SS_OP_FUCLASS(rs->op));

          if (fu || infinite_fu)
          {
            /* got one! issue inst to functional unit */
            rs->decoded = FALSE;
            rs->issued = TRUE;
            rs->issued_at = sim_cycle;

            /* reserve the functional unit */
            if (!infinite_fu && fu->master->busy)
              panic("functional unit already in use");

            /* schedule functional unit release event */
            if (!infinite_fu)
              fu->master->busy = fu->issuelat;

            /* We issued an inst */
            n_issued++;
            if (!rs->spec_mode)
              n_issued_useful++;
            if (SS_OP_FLAGS(rs->op) & F_FCOMP)
            {
              n_fp_issued++;
              FIQ_occ--;
            }
            else
            {
              n_int_issued++;
              IIQ_occ--;
            }

            /* schedule a result writeback event */
            if (rs->in_LSQ && ((SS_OP_FLAGS(rs->op) & (F_MEM | F_LOAD)) == (F_MEM | F_LOAD)))
            {
              int events = 0;

              /* for loads, determine cache access latency:
			     first scan LSQ to see if a store forward is
			     possible, if not, access the data cache */
              load_lat = 0;
              i = (rs - LSQ);
              if (i != LSQ_head)
              {
                for (;;)
                {
                  /* go to next earlier LSQ entry */
                  i = (i + (LSQ_size - 1)) % LSQ_size;

                  /* FIXME: not dealing with partials! */
                  if ((SS_OP_FLAGS(LSQ[i].op) & F_STORE) && (LSQ[i].addr == rs->addr) && !LSQ[i].squashed)
                  {
                    /* hit in the LSQ */
                    load_lat = 1;
                    break;
                  }

                  /* scan finished? */
                  if (i == LSQ_head)
                    break;
                }
              }

              /* was the value store forwared from the LSQ? */
              if (!load_lat)
              {
                /* no! go to the data cache */
                if (cache_dl1
                    /* ports available? */
                    && cache_dl1_ports_used < cache_dl1_ports
                    /* valid address? */
                    && (rs->addr >= ld_data_base && rs->addr < ld_stack_base))
                {
                  /* access the cache if non-faulting */
                  cache_dl1_ports_used++;
                  if (cache_dl1_perfect)
                    load_lat = cache_dl1_lat[0] + cache_dl1_lat[1];
                  else
                    load_lat =
                        cache_access(cache_dl1, Read,
                                     (rs->addr & ~3), NULL, 4,
                                     sim_cycle + extra_issue_lat,
                                     NULL, NULL);

                  if (load_lat >
                      cache_dl1_lat[0] + cache_dl1_lat[1])
                  {
                    if (report_miss_clustering)
                    {
                      int miss_dist = sim_cycle - last_dmiss_cycle;
                      assert(miss_dist >= 0);
                      last_dmiss_cycle = sim_cycle;
                      stat_add_sample(d1miss_cluster_dist,
                                      miss_dist);
                    }

                    events |= PEV_CACHEMISS;
                    rs->l1_miss = TRUE;
                  }
                }
                else if (dl1_access_mem && rs->addr >= ld_data_base && rs->addr < ld_stack_base)
                {
                  /* No D-caches, but get accurate mem
				   * access behavior */
                  load_lat = dl2_access_fn(Read,
                                           (rs->addr * ~3),
                                           4, NULL,
                                           sim_cycle + extra_issue_lat);
                }
                else if (cache_dl1 &&
                         cache_dl1_ports_used >= cache_dl1_ports)
                {
                  /* yuck -- can't do the load this cycle.
				   * This load will have to wait */
                  rs->issued = FALSE;
                  rs->decoded = TRUE;
                  if (!infinite_fu)
                    fu->master->busy = 0;
                  n_issued--;
                  n_int_issued--;
                  IIQ_occ++;
                  if (!rs->spec_mode)
                    n_issued_useful--;
                  readyq_enqueue(rs);
                  goto inst_requeued;
                }
                else
                {
                  /* no caches defined, or ref to an invalid
				   * address -- just use op latency */
                  load_lat = oplat;
                }
              }
              else if (rs->addr >= ld_data_base && rs->addr < ld_stack_base)
              {
                /* value was forwarded from LSQ -- count it,
			       * so long as it's a valid ref */
                lsq_hits++;
              }
              /* else ref to an invalid address that got
			   * forwarded in LSQ */

              /* all loads and stores must access D-TLB */
              if (dtlb
                  /* valid address? */
                  && (rs->addr >= ld_data_base && rs->addr < ld_stack_base))
              {
                /* access the D-TLB, NOTE: this code will
				 initiate speculative TLB misses */
                if (tlb_perfect)
                  tlb_lat = 1;
                else
                  tlb_lat =
                      cache_access(dtlb, Read, (rs->addr & ~3),
                                   NULL, 4,
                                   sim_cycle + extra_issue_lat,
                                   NULL, NULL);
                if (tlb_perfect)
                  tlb_lat = 1;

                if (tlb_lat > 1)
                  events |= PEV_TLBMISS;

                /* D-cache/D-TLB accesses occur in parallel */
                load_lat = MAX(tlb_lat, load_lat);
              }
              /* use computed cache access latency */
              eventq_queue_event(rs, sim_cycle + load_lat + extra_issue_lat);

              /* entered execute stage, indicate in pipe trace */
              if (ptrace_level != PTRACE_FUNSIM)
                ptrace_newstage(rs->ptrace_seq, rs->thread_id,
                                PST_EXECUTE,
                                (rs->ea_comp ? PEV_AGEN : 0) | events | (rs->spec_mode ? PEV_SPEC_MODE : 0));
            }
            else /* !load && !store */
            {
              /* use deterministic functional unit latency */
              eventq_queue_event(rs, sim_cycle + oplat + extra_issue_lat);

              /* entered execute stage, indicate in pipe trace */
              if (ptrace_level != PTRACE_FUNSIM)
                ptrace_newstage(rs->ptrace_seq, rs->thread_id,
                                PST_EXECUTE,
                                (rs->ea_comp ? PEV_AGEN : 0) |
                                    (rs->spec_mode ? PEV_SPEC_MODE : 0));
            }
          }
          else /* no functional unit */
          {
            /* insufficient functional unit resources, put operation
			 back onto the ready list, we'll try to issue it
			 again next cycle */
            func_unit_overflows++;
            readyq_enqueue(rs);
          }
        }
        else /* does not require a functional unit! */
        {
          /* FIXME: need better solution for these */
          /* the instruction does not need a functional unit */
          rs->decoded = FALSE;
          rs->issued = TRUE;
          rs->issued_at = sim_cycle;

          /* schedule a result event */
          eventq_queue_event(rs, sim_cycle + 1 + extra_issue_lat);

          /* entered execute stage, indicate in pipe trace */
          if (ptrace_level != PTRACE_FUNSIM)
            ptrace_newstage(rs->ptrace_seq, rs->thread_id,
                            PST_EXECUTE,
                            (rs->ea_comp ? PEV_AGEN : 0) | (rs->spec_mode ? PEV_SPEC_MODE : 0));

          /* We issued one */
          n_issued++;
          if (!rs->spec_mode)
            n_issued_useful++;
          if (SS_OP_FLAGS(rs->op) & F_FCOMP)
          {
            n_fp_issued++;
            FIQ_occ--;
          }
          else
          {
            n_int_issued++;
            IIQ_occ--;
          }
        }

        if (report_issue_loc && done_priming && rs->issued &&
            !rs->spec_mode && !rs->in_LSQ)
        {
          issue_depth = ((long)rs - (long)&RUU[0]) / sizeof(struct RUU_station) - RUU_head;
          if (issue_depth < 0)
            issue_depth = RUU_size + issue_depth;
          stat_add_sample(issue_loc_dist, issue_depth);
        }

        if (report_issue_delay && done_priming && rs->issued &&
            !rs->spec_mode && !rs->in_LSQ)
        {
          stat_add_sample(issue_delay_dist, sim_cycle - rs->ready_time);
          assert(rs->ready_time != 0);
          stat_add_sample(operand_delay_dist,
                          MAX(0, rs->ready_time - rs->ready_to_iss));
        }
      } /* !store */
    }
    /* else, RUU entry was squashed */

  inst_requeued:
    /* reclaim ready list entry, NOTE: this is done whether or not the
         instruction issued, since the instruction was once again reinserted
         into the ready queue if it did not issue, this ensures that the ready
         queue is always properly sorted */
    RSLINK_FREE(node);
  }

  /* Clean up insts we didn't get to process because we fulfilled the
   * issue width. */
  for (; node; node = next_node)
  {
    struct RUU_station *rs = RSLINK_RS(node);
    next_node = node->next;

    if (RSLINK_VALID(node) && !rs->squashed)
    {
      /* issue operation, both reg and mem deps have been satisfied */
      if (!OPERANDS_READY(rs) || !rs->queued || rs->issued || rs->completed)
        panic("issued inst !ready, issued, or completed");

      /* node is now un-queued */
      rs->queued = FALSE;

      /* Just requeue it into the ready list, for another try next cycle */
      readyq_enqueue(rs);
    }
    /* else RUU entry was squashed */

    /* reclaim ready list entry, NOTE: this is done whether or not the
         instruction issued, since the instruction was once again reinserted
         into the ready queue if it did not issue, this ensures that the ready
         queue is always properly sorted */
    RSLINK_FREE(node);
  }

  if (report_issue && done_priming)
  {
    stat_add_sample(issue_dist, n_issued);
    stat_add_sample(useful_issue_dist, n_issued_useful);
  }
}

/*
 * routines for generating on-the-fly instruction traces with support
 * for control and data misspeculation modeling
 */

/* integer register file */
#define R_BMAP_SZ (BITMAP_SIZE(SS_NUM_REGS))
static SS_WORD_TYPE spec_regs_R[N_THREAD_RECS][N_SPEC_LEVELS][SS_NUM_REGS];

/* floating point register file */
#define F_BMAP_SZ (BITMAP_SIZE(SS_NUM_REGS))
static union regs_FP spec_regs_F[N_THREAD_RECS][N_SPEC_LEVELS];

/* miscellaneous registers */
static SS_WORD_TYPE spec_regs_HI[N_THREAD_RECS][N_SPEC_LEVELS];
static SS_WORD_TYPE spec_regs_LO[N_THREAD_RECS][N_SPEC_LEVELS];
static int spec_regs_FCC[N_THREAD_RECS][N_SPEC_LEVELS];

/* dump speculative register state */
static void
rspec_dump(FILE *stream) /* output stream, thread no. */
{
  int i, s, t;

  fprintf(stream, "** speculative register contents **\n");

  for (t = 0; t < N_THREAD_RECS; t++)
  {
    if (!thread_info[t].valid)
      continue;

    fprintf(stream, "thread_id: %d\n", t);

    /* is this thread in spec_mode? */
    fprintf(stream, "   spec_mode: %s, spec_lev: %d\n",
            thread_info[t].spec_mode ? "t" : "f", thread_info[t].spec_level);

    /* dump speculative integer regs */
    for (s = 0; s < thread_info[t].spec_level; s++)
      for (i = 0; i < SS_NUM_REGS; i++)
      {
        /* speculative state */
        fprintf(stream, "   [%4s]: %12d/0x%08x\n", dep_names[i],
                spec_regs_R[t][s][i], spec_regs_R[t][s][i]);
      }

    /* dump speculative FP regs */
    for (s = 0; s < thread_info[t].spec_level; s++)
      for (i = 0; i < SS_NUM_REGS; i++)
      {
        /* speculative state */
        fprintf(stream,
                "   [%4s]: %12d/0x%08x/%f ([%4s] as double: %f)\n\n",
                dep_names[i + 32],
                spec_regs_F[t][s].l[i], spec_regs_F[t][s].l[i],
                spec_regs_F[t][s].f[i],
                dep_names[i + 32],
                spec_regs_F[t][s].d[i >> 1]);
      }

    /* dump speculative miscellaneous regs */
    for (s = 0; s < thread_info[t].spec_level; s++)
    {
      fprintf(stream, "   [ $hi]: %12d/0x%08x\n",
              spec_regs_HI[t][s], spec_regs_HI[t][s]);
      fprintf(stream, "   [ $lo]: %12d/0x%08x\n",
              spec_regs_LO[t][s], spec_regs_LO[t][s]);
      fprintf(stream, "   [$fcc]: 0x%08x\n", spec_regs_FCC[t][s]);
    }
  }
}

/* speculative memory hash table size, NOTE: this must be a power-of-two */
#define STORE_HASH_SIZE 32

/* speculative memory hash table definition, accesses go through this hash
   table when accessing memory in speculative mode, the hash table flush the
   table when recovering from mispredicted branches */
struct spec_mem_ent
{
  struct spec_mem_ent *next; /* ptr to next hash table bucket */
  SS_ADDR_TYPE addr;         /* virtual address of spec state */
  int spec_level;            /* passed this many br mispreds */
  int thread;                /* thread that created this addr */
  unsigned int data[2];      /* spec buffer, up to 8 bytes */
};

/* speculative memory hash table */
static struct spec_mem_ent *store_htable[STORE_HASH_SIZE];

/* speculative memory hash table bucket free list */
static struct spec_mem_ent *bucket_free_list = NULL;

/* 
 * THREAD_CLEANUP() - on a branch resolution, kill threads that have been 
 * identified misspeculated as a result, mark entries in the RUU and LSQ 
 * that should be squashed as a result (squashing done by ruu_recover()),
 */

static void
spec_mode_recover(int spec_level, int t);

static SS_TIME_TYPE last_true_token_seen = 0;

/* kill threads for which all instructions from a squashed thread have 
 * completed */
static void
kill_threads(void)
{
  int t, n, n2;
  int saw_token = FALSE, saw_token_overall = FALSE;
  BITMAP_TYPE(N_SPEC_LEVELS, active_bmap);
  BITMAP_CLEAR_MAP(active_bmap, THREADS_BMAP_SZ);

  assert(max_threads > 1);

  /* record threads still live */
  for (t = RUU_head, n = 0; n < RUU_num; t = (t + 1) % RUU_size, n++)
    if ((SS_OP_FLAGS(RUU[t].op) & F_COND) && !RUU[t].squashed)
      (void)BITMAP_SET(active_bmap, THREADS_BMAP_SZ, RUU[t].thread_id);

  /* kill the rest */
  for (t = 0, n = 0, n2 = 0; t < N_THREAD_RECS; t++)
  {
    if (thread_info[t].valid == FALSE)
      continue;

    if (!BITMAP_SET_P(active_bmap, THREADS_BMAP_SZ, t))
    {
      /* doesn't appear in active_bmap, so it's dead*/
      if (thread_info[t].valid == KILLED && thread_info[t].squashed == TRUE)
      {
        /* it's a thread that needs to be deallocated -- do it */
        spec_mode_recover(0, t); /* sometimes but not always redundant */
        thread_info[t].valid = FALSE;
        thread_info[t].squashed = UNKNOWN;
        num_zombie_forks--;
        if (ptrace_level != PTRACE_FUNSIM)
          ptrace_killthread(t);
      }
      else if (thread_info[t].squashed == TRUE && thread_info[t].spec_mode)
        panic("thread kill state wedged");
    }
    else
      assert(thread_info[t].valid);

#ifndef NDEBUG
    /* count non-zeroed-out threads, record whether prune token was seen */
    if (thread_info[t].valid)
      n++;
    if (thread_info[t].valid == TRUE)
    {
      n2++;

      if (fork_prune || fetch_pred_pri)
      {
        if (thread_info[t].pred_path_token && thread_info[t].spec_mode == FALSE && saw_token)
          panic("prune-token wedged");
        else if (thread_info[t].pred_path_token)
        {
          saw_token_overall = TRUE;
          if (thread_info[t].spec_mode == FALSE)
          {
            saw_token = TRUE;
            last_true_token_seen = sim_cycle;
          }
        }
      }
    }

    /* any thread that hasn't been fetched from for a long time is
       * probably orphaned */
    if (thread_info[t].valid && thread_info[t].fetchable + DEAD_TIME < sim_cycle)
      panic("orphaned thread");
#endif
  }

#ifndef NDEBUG
  /* check token */
  if ((fork_prune || fetch_pred_pri) && !saw_token_overall)
    panic("lost prune-token!");
  if ((fork_prune || fetch_pred_pri) && sim_cycle - last_true_token_seen > DEAD_TIME)
    panic("lost prune-token!");

  /* check new token */
  if (fetch_pri_pol == Pred_RR || fetch_pri_pol == Pred_Pri2)
  {
    dassert(pred_thread >= 0 && pred_thread < N_THREAD_RECS);
    assert(thread_info[pred_thread].valid == TRUE);
  }

  /* check for a "thread leak" */
  assert(num_active_forks >= 0);
  assert(num_active_forks + num_zombie_forks <= N_THREAD_RECS);
  if (n != num_active_forks + 1 + num_zombie_forks)
    panic("number of occupied thread contexts doesn't match count");
  if (n2 != num_active_forks + 1)
    panic("number of threads active doesn't match fork count");
#endif
#ifdef DEBUG
  n = BITMAP_COUNT_ONES(active_bmap, THREADS_BMAP_SZ);
  if (n > num_active_forks + 1 + num_zombie_forks)
    panic("number of threads active exceeds fork count");
#endif

  stat_add_sample(forked_dist, num_active_forks);
}

/* we're in the process of recovering from a mispredicted branch: revert all
 * speculative state, including speculative memory state */
static void
spec_mode_recover(int spec_level, /* kill all successive levels */
                  int t)          /* thread to kill */
{
  struct spec_mem_ent *ent, *ent_next, *ent_prev;
  int i, s, max_thread_spec_level = thread_info[t].spec_level;

  /* this thread has been squashed, and all pending instructions
   * have been squashed or have completed: revert create vector, and spec-mem
   * state for the thread we just squashed back to last precise state. */
  for (s = spec_level + 1; s <= max_thread_spec_level; s++)
  {
    for (i = 0; i < SS_TOTAL_REGS; i++)
      spec_create_vector[t][s][i] = CVLINK_NULL;
  }

  /* FIXME: could version stamps be used here?!?!? */
  for (i = 0; i < STORE_HASH_SIZE; i++)
  {
    ent_prev = NULL;

    /* release all hash table buckets for squashed levels of this thread */
    for (ent = store_htable[i]; ent; ent = ent_next)
    {
      ent_next = ent->next;

      if (ent->thread != t || ent->spec_level <= spec_level)
      {
        /* this is state that's not getting squashed */
        ent_prev = ent;
        continue;
      }

      ent->next = bucket_free_list;
      bucket_free_list = ent;

      if (ent_prev)
        ent_prev->next = ent_next;
      else
        store_htable[i] = ent_next;
    }
  }
}

/* recover instruction trace generator state to precise state state immediately
   before the first mis-predicted branch; this is accomplished by resetting
   all register value copied-on-write bitmasks are reset, and the speculative
   memory hash table is cleared.  Misspeculated threads are deallocated. */
static void
thread_cleanup(struct RUU_station *rs_branch)
{
  int i, t, n, did_squash, taken;
  int kill_bmap_ptr = rs_branch->fork_hist_bmap_ptr;
  BITMAP_TYPE(N_SPEC_LEVELS, kill_bmap);
  BITMAP_TYPE(N_SPEC_LEVELS, tmp_bmap);

  dassert(thread_info[rs_branch->thread_id].valid);
  assert(!pred_perfect);

  /* working copy of the fork-hist bmap corresponding to the killed path */
  BITMAP_COPY(kill_bmap, rs_branch->fork_hist_bmap, THREADS_BMAP_SZ);

  /* update the working copy with the result of this branch -- not applicable
   * for non-forked branches */
  if (rs_branch->forked)
  {
    /* branch taken? */
    taken = (rs_branch->next_PC != rs_branch->PC + SS_INST_SIZE);

    kill_bmap_ptr = (kill_bmap_ptr + 1) % N_SPEC_LEVELS;

#if 0
      if (bconf_type != Omni)
	{
	  /* forked path always goes down taken path; parent follows not-taken
	   * path */
#endif
    if (!taken)
      (void)BITMAP_SET(kill_bmap, THREADS_BMAP_SZ, kill_bmap_ptr);
    else
      (void)BITMAP_CLEAR(kill_bmap, THREADS_BMAP_SZ, kill_bmap_ptr);
#if 0
	}
      else
	{
	  /* forked path might be the taken or the not-taken path */
	  int br_followed = (pred_PC != regs_PC + SS_INST_SIZE); 
	}
#endif
  }

  /* walk the thread_info structure, identifying threads that are on
   * misspeculated paths */
  for (t = 0; t < N_THREAD_RECS; t++)
  {
    did_squash = FALSE;

    if (!thread_info[t].valid)
      continue;

    /* find bits where the candidate and squashed-thread bitmaps match */
    BITMAP_XOR(tmp_bmap,                      /* answer stored here */
               kill_bmap,                     /* input bitmap */
               thread_info[t].fork_hist_bmap, /* "    "    */
               THREADS_BMAP_SZ);              /* bitmap size */
    BITMAP_NOT(tmp_bmap, tmp_bmap, THREADS_BMAP_SZ);

    /* if candidate thread's bmap doesn't match for all bits from the
       * fork_hist_bmap_head through the bit corresponding to the branch 
       * just resolved, then the candidate isn't a child and hasn't
       * just been identified as misspeculated.  */
    for (i = fork_hist_bmap_head; !did_squash; i = (i + 1) % N_SPEC_LEVELS)
    {
      /* if bit 'i' doesn't match, 't' isn't a child of the
	   * thread being squashed */
      if (!BITMAP_SET_P(tmp_bmap, THREADS_BMAP_SZ, i))
      {
        if (thread_info[t].squashed != TRUE)
          thread_info[t].squashed = FALSE;

        break;
      }

      /* if we just examined the squashed branch's "own" bit without 
	   * a failed match, all relevant bits match, we should squash 
	   * thread number 't', and we're done */
      if (i == kill_bmap_ptr)
        did_squash = TRUE;
    }

    /* if candidate matches all bits above, then squash it */
    if (did_squash)
    {
      thread_info[t].squashed = NOW;

#ifndef NDEBUG
      if (thread_info[t].spec_mode == FALSE && rs_branch->thread_id != t)
        panic("tried to squash the correctly-speculated path");
#endif
    }
  }

  for (t = RUU_head, n = 0; n < RUU_num; t = (t + 1) % RUU_size, n++)
    if (thread_info[RUU[t].thread_id].squashed == NOW)
      RUU[t].squashable = TRUE;

  for (t = LSQ_head, n = 0; n < LSQ_num; t = (t + 1) % LSQ_size, n++)
    if (thread_info[LSQ[t].thread_id].squashed == NOW)
      LSQ[t].squashable = TRUE;

  for (t = 0; t < N_THREAD_RECS; t++)
    if (thread_info[t].squashed == NOW)
    {
      thread_info[t].squashed = TRUE;

      /* if this thread never had any instructions make it to decode,
	 * we really wasted some effort */
      if (!thread_info[t].did_decode)
        total_forks_squashed_pre_decode++;
    }
}

/* after branch-misprediction recovery, reset squashable flags, finish
 * killing threads that have been canceled */
static void
post_thread_cleanup(struct RUU_station *rs_branch)
{
  int t, n;
  int branch_thread = rs_branch->thread_id;
  int saw_valid = FALSE;

  /* reset squashable flags in RUU and LSQ.  Also advance fork_hist_bmap_head
   * as far as possible.  */
  for (t = LSQ_head, n = 0; n < LSQ_num; t = (t + 1) % LSQ_size, n++)
    LSQ[t].squashable = FALSE;
  for (t = RUU_head, n = 0; n < RUU_num; t = (t + 1) % RUU_size, n++)
  {
    RUU[t].squashable = FALSE;

    if ((SS_OP_FLAGS(RUU[t].op) & F_CTRL) && !RUU[t].squashed && !saw_valid)
    {
      /* found a live branch */
      fork_hist_bmap_head = RUU[t].fork_hist_bmap_ptr;
      saw_valid = TRUE;
    }
  }

  /* revert spec resources for each squashed thread.  Note killing the
   * thread must occur later */
  for (t = 0; t < N_THREAD_RECS; t++)
  {
#ifdef DEBUG
    /* sanity checks */
    if (thread_info[t].squashed == TRUE && rs_branch->forked)
      assert(thread_info[t].valid);
    if (thread_info[t].valid)
      assert(thread_info[t].squashed != UNKNOWN);
    else
      assert(thread_info[t].squashed == UNKNOWN);
#endif

    if (thread_info[t].squashed == TRUE)
    {
      /* this thread has been squashed -- revert its speculative state 
	   * if not already done */
      if (thread_info[t].valid == TRUE)
      {
        spec_mode_recover(rs_branch->spec_level, t);
        thread_info[t].valid = KILLED;

        num_active_forks--;
        num_zombie_forks++;

        if (rs_branch->forked || t != branch_thread)
          if (ptrace_level != PTRACE_FUNSIM)
            ptrace_squashthread(t);
      }
      /* else already squashed */
    }
    else
      thread_info[t].squashed = UNKNOWN;
  }

  /* if this branch didn't fork, we shouldn't kill its thread -- just all
   * its children threads. */
  if (!rs_branch->forked)
  {
    dassert(rs_branch->pred_PC != rs_branch->next_PC);
    dassert(rs_branch->squashed == FALSE && rs_branch->recover_inst);

    thread_info[branch_thread].valid = TRUE;
    thread_info[branch_thread].squashed = UNKNOWN;
    num_zombie_forks--;
    num_active_forks++;

    /* a later forked branch from this thread, if it squashes this thread,
       * squashes the whole thread but only reverts the spec level to that
       * branch's spec level.  So an earlier branch may need to further
       * revert the spec level. (A later branch should trip the above assert */
    spec_mode_recover(rs_branch->spec_level, branch_thread);
    thread_info[branch_thread].spec_mode = rs_branch->spec_mode;
    thread_info[branch_thread].spec_level = rs_branch->spec_level;

    assert(thread_info[branch_thread].spec_level >= 0);
  }
#ifdef DEBUG
  else
    assert(thread_info[branch_thread].squashed != TRUE || thread_info[branch_thread].valid != TRUE);
#endif
}

/* 
 * IFETCH -> DISPATCH instruction queue definition 
 */
struct fetch_rec
{
  SS_INST_TYPE IR;                             /* inst register */
  SS_ADDR_TYPE regs_PC, pred_PC;               /* current PC, predicted next PC */
  SS_ADDR_TYPE base_pred_PC;                   /* pred PC if no forking */
  SS_ADDR_TYPE predPC_if_taken;                /* BTB's predPC if br-taken */
  struct bpred_update_info b_update_rec;       /* bpred info needed for update */
  struct bpred_recover_info bpred_recover_rec; /* retstack fixup info */
  struct bpred_recover_info retstack_copy_rec; /* for fork-in-decode */
  unsigned int ptrace_seq;                     /* print trace sequence id */
  int conf;                                    /* conf pred on this branch */
  int conf_data;                               /* data returned by conf pred. */
  int could_have_forked;                       /* forking resources available */
  int forked;                                  /* did we fork on this branch? */
  int forked_thread;                           /* if this thread forked, the
					 * id of the forked child */
  int thread;                                  /* this inst's thread id */
  BITMAP_TYPE(N_SPEC_LEVELS, fork_hist_bmap);  /* fork history bitmap */
  BITMAP_ENT_TYPE fork_hist_bmap_ptr;          /* ptr to this inst's pos'n in bmap */
  SS_TIME_TYPE fetched_at;                     /* when this inst was fetched */
  int valid;
};
static struct fetch_rec *ifq;  /* IFETCH -> DISPATCH inst queue */
static int ifq_num;            /* num entries in IF -> DIS queue */
static int ifq_tail, ifq_head; /* head and tail pointers of queue */

/* this only cleans up the IF-DA fetch queue
 * note: might be called from misfetch-patch-up in ruu_dispatch(), in
 * which case thread_info[thread].squashed might be UNKNOWN */
static void
tracer_recover(struct RUU_station *rs_branch, /* branch triggering recovery */
               int penalty,                   /* fetch-redirection penalty */
               int ifq_patch_only)            /* called by wrapper -- only
					       * cleaning up IFQ */
{
  int i, n;
  int thread = rs_branch->thread_id;
  int num_in_q = ifq_num;
#ifdef BUG_COMPAT_FETCHQ
  int saw_valid = FALSE;
#endif

  /* squash appropriate instructions in the inst fetch queue. */
  for (i = (ifq_tail + (ruu_ifq_size - 1)) % ruu_ifq_size, n = 0;
       n < num_in_q;
       i = (i + (ruu_ifq_size - 1)) % ruu_ifq_size, n++)
  {
    /* squash the next instruction from the IFETCH -> DISPATCH queue? */
    if (thread_info[ifq[i].thread].squashed == TRUE)
    {
      ifq[i].valid = FALSE;
      if (pred->retstack.patch_level == RETSTACK_PATCH_WHOLE && ifq[i].bpred_recover_rec.contents.stack_copy)
      {
        free(ifq[i].bpred_recover_rec.contents.stack_copy);
        ifq[i].bpred_recover_rec.contents.stack_copy = NULL;
      }
      if (ifq[i].retstack_copy_rec.contents.stack_copy)
      {
        free(ifq[i].retstack_copy_rec.contents.stack_copy);
        ifq[i].retstack_copy_rec.contents.stack_copy = NULL;
      }

      /* if pipetracing, indicate we squashed a fetched instruction */
      if (ptrace_level != PTRACE_FUNSIM)
        ptrace_endinst(ifq[i].ptrace_seq, ifq[i].thread);

#ifdef BUG_COMPAT_FETCHQ
      if (!saw_valid)
      {
        ifq_tail = i;
        ifq_num--;
      }
#endif
    }
#ifdef BUG_COMPAT_FETCHQ
    else
      saw_valid = TRUE;
#endif
  }

  if (rs_branch->recover_inst)
  {
    void fetch_dump(FILE * stream);
    assert(ifq_patch_only || rs_branch->next_PC != rs_branch->pred_PC);
    thread_fetch_redirect(thread, rs_branch->next_PC, penalty);
  }
}

static void
tracer_recover_wrapper(int curr_thread, SS_ADDR_TYPE regs_PC,
                       SS_ADDR_TYPE next_PC, int recover_inst, int penalty)
{
  struct RUU_station dummy_rs;

  /* Create a dummy rs that provides the necessary info to tracer_recover() */
#ifdef DEBUG_DEF_VALS
  bzero(&dummy_rs, sizeof(struct RUU_station));
  dummy_rs.PC = regs_PC;
#endif
  dummy_rs.recover_inst = recover_inst;
  dummy_rs.next_PC = next_PC;
  dummy_rs.thread_id = curr_thread;

  tracer_recover(&dummy_rs, penalty, TRUE);
}

/* initialize the speculative instruction state generator state */
static void
tracer_init(void)
{
  ; /* store_htable should get initialized via static decls */
}

/* 
 * FORK_THREAD: fork a new thread, or patch a thread that was forked
 * to the wrong PC 
 */

/* fork a new path, to start fetching at 'newPC' and whose parent is
 * 'forking_thread' */
static int
fork_thread(SS_ADDR_TYPE newPC, int forking_thread)
{
  int t, n;
  int new_thread = -1;
#ifdef DEBUG_SPEC_MEM
  int i;
  struct spec_mem_ent *ent;
#endif
  BITMAP_ENT_TYPE old_bmap_ptr = thread_info[forking_thread].fork_hist_bmap_ptr;
  dassert(thread_info[forking_thread].valid == TRUE);

  /* find a vacant thread record */
  if (num_active_forks == max_threads - 1)
    return new_thread;

  for (t = forking_thread, n = 0; n < N_THREAD_RECS;
       t = (t + 1) % N_THREAD_RECS, n++)
  {
    if (!thread_info[t].valid)
    {
      new_thread = t;
      break;
    }
  }

  if (new_thread < 0)
  {
    num_thread_context_overflows++;
    return new_thread;
  }

  /* set up new thread record */
  thread_info[new_thread].valid = TRUE;
  thread_info[new_thread].fetch_regs_PC = newPC - SS_INST_SIZE;
  thread_info[new_thread].fetch_pred_PC = newPC;
  thread_info[new_thread].fetchable = sim_cycle + ruu_fork_penalty;
  if (fetch_pri_pol == Omni_Pri || fetch_pri_pol == Two_Omni_Pri || fetch_pri_pol == Pred_Pri || fetch_pri_pol == Ruu_Pri)
  {
    thread_info[new_thread].priority = 1;
    thread_info[new_thread].base_priority = 1;
  }
  thread_info[new_thread].did_decode = FALSE;
  thread_info[new_thread].last_inst_missed = FALSE;
  thread_info[new_thread].last_inst_tmissed = FALSE;
  thread_info[new_thread].pred_path_token = FALSE;
  thread_info[new_thread].fork_hist_bmap_ptr = (old_bmap_ptr + 1) % N_SPEC_LEVELS;
  BITMAP_COPY(thread_info[new_thread].fork_hist_bmap,
              thread_info[forking_thread].fork_hist_bmap,
              THREADS_BMAP_SZ);
  (void)BITMAP_SET(thread_info[new_thread].fork_hist_bmap, THREADS_BMAP_SZ,
                   thread_info[new_thread].fork_hist_bmap_ptr);
  thread_info[new_thread].retstack_tos = thread_info[forking_thread].retstack_tos;
  if (per_thread_retstack == PerThreadStacks)
  {
    dassert(retstack_size);
    memcpy(thread_info[new_thread].retstack,
           thread_info[forking_thread].retstack,
           retstack_size * sizeof(struct bpred_btb_ent));
  }
  thread_info[new_thread].spec_mode = UNKNOWN;
  thread_info[new_thread].spec_level = -1;
  thread_info[new_thread].squashed = UNKNOWN;
  assert(thread_info[new_thread].fork_hist_bmap_ptr != fork_hist_bmap_head);

#ifdef DEBUG_SPEC_MEM
  /* check spec-mem-state */
  for (i = 0; i < STORE_HASH_SIZE; i++)
  {
    /* should be no entries for this thread */
    for (ent = store_htable[i]; ent; ent = ent->next)
    {
      if (ent->thread != t)
        continue;
      panic("spec-mem-state leak, thread %t", new_thread);
    }
  }
#endif

  /*
  double_priorities();
  */

  /* WARNING: caller is responsible for updating parent path to reflect this
   * fork! */

  num_active_forks++;
  return new_thread;
}

/* if predPC from BTB was incorrect or nonexistent, and this thread 
 * therefore needs to be redirected, do so.  */
void thread_refork(int forked_thread,
                   int forking_thread,
                   SS_ADDR_TYPE forking_branch_PC,
                   SS_ADDR_TYPE forking_branch_base_pred_PC,
                   SS_ADDR_TYPE PC_if_taken,
                   enum ss_opcode op,
                   BITMAP_PTR_TYPE forking_branch_bmap,
                   BITMAP_ENT_TYPE forking_branch_bmap_ptr,
                   struct bpred_recover_info *forking_bpred_recover_rec_p,
                   int ruu_refork_penalty)
{
  struct RUU_station dummy_rs;
  int junk;

  /* only proceed if this thread hasn't already been killed */
  if (thread_info[forked_thread].valid == KILLED)
    return;
  dassert(thread_info[forked_thread].squashed != TRUE && thread_info[forked_thread].valid == TRUE);

  /* squash any instructions fetched by this thread -- they must be bogus --
   * and any child threads.  Note no instructions from this mis-directed
   * thread or its children can have made it into the RUU */
  bzero(&dummy_rs, sizeof(struct RUU_station));
  dummy_rs.PC = forking_branch_PC;
  dummy_rs.pred_PC = dummy_rs.next_PC = forking_branch_PC + SS_INST_SIZE;
  dummy_rs.recover_inst = FALSE;
  dummy_rs.thread_id = forking_thread;
  dummy_rs.forked = TRUE;
  dummy_rs.fork_hist_bmap_ptr = forking_branch_bmap_ptr;
  BITMAP_COPY(dummy_rs.fork_hist_bmap, forking_branch_bmap, THREADS_BMAP_SZ);
  ifq_num--; /* don't nuke forking branch from IFQ*/

  thread_cleanup(&dummy_rs);
  tracer_recover(&dummy_rs, ruu_refork_penalty, FALSE);
  hydra_bpred_recover(forked_thread, forking_branch_PC,
                      forking_branch_base_pred_PC, op,
                      forking_bpred_recover_rec_p, &junk,
                      Decode, "refork");
  post_thread_cleanup(&dummy_rs);

  ifq_num++;
  num_active_forks++;
  num_zombie_forks--;

  /* if ptracing, post_thread_cleanup() shows this thread as squashed;
   * show that it was just reforked */
  if (ptrace_level != PTRACE_FUNSIM)
    ptrace_newthread(forked_thread, forking_branch_PC, PC_if_taken);

  /* reset thread state to correct values; charge fetch-redirection penalty */
  thread_info[forked_thread].squashed = UNKNOWN;
  thread_info[forked_thread].valid = TRUE;
  if (fetch_pri_pol == Omni_Pri || fetch_pri_pol == Two_Omni_Pri || fetch_pri_pol == Pred_Pri || fetch_pri_pol == Ruu_Pri)
  {
    thread_info[forked_thread].priority = 1;
    thread_info[forked_thread].base_priority = 1;
  }
  thread_info[forked_thread].spec_mode = UNKNOWN;
  thread_info[forked_thread].spec_level = -1;
  thread_info[forked_thread].fetch_regs_PC = PC_if_taken - SS_INST_SIZE;
  thread_info[forked_thread].fetch_pred_PC = PC_if_taken;
  thread_info[forked_thread].fetchable = sim_cycle + ruu_refork_penalty;
  dassert(!thread_info[forked_thread].did_decode);
  thread_info[forked_thread].last_inst_missed = FALSE;
  thread_info[forked_thread].last_inst_tmissed = FALSE;
  thread_info[forked_thread].pred_path_token = FALSE;

  thread_info[forked_thread].retstack_tos = thread_info[forking_thread].retstack_tos;
  if (per_thread_retstack == PerThreadStacks)
  {
    dassert(retstack_size);
    memcpy(thread_info[forked_thread].retstack,
           thread_info[forking_thread].retstack,
           retstack_size * sizeof(struct bpred_btb_ent));
  }

  /* since this thread might have fetched and forked already, reset its
   * fork history based on the forking branch's history, just like when
   * this thread was originally forked */
  thread_info[forked_thread].fork_hist_bmap_ptr = (forking_branch_bmap_ptr + 1) % N_SPEC_LEVELS;
  BITMAP_COPY(thread_info[forked_thread].fork_hist_bmap,
              forking_branch_bmap,
              THREADS_BMAP_SZ);
  (void)BITMAP_SET(thread_info[forked_thread].fork_hist_bmap, THREADS_BMAP_SZ,
                   thread_info[forked_thread].fork_hist_bmap_ptr);
}

/* speculative memory hash table address hash function */
#define HASH_ADDR(ADDR) \
  ((((ADDR) >> 24) ^ ((ADDR) >> 16) ^ ((ADDR) >> 8) ^ (ADDR)) & (STORE_HASH_SIZE - 1))

/* this functional provides a layer of mis-speculated state over the
   non-speculative memory state, when in mis-speculation trace generation mode,
   the simulator will call this function to access memory, instead of the
   non-speculative memory access interfaces defined in memory.h; when storage
   is written, an entry is allocated in the speculative memory hash table,
   future reads and writes while in mis-speculative trace generation mode will
   access this buffer instead of non-speculative memory state; when the trace
   generator transitions back to non-speculative trace generation mode,
   spec_mode_recover() clears this table */
static void
spec_mem_access(enum mem_cmd cmd,  /* Read or Write access cmd */
                SS_ADDR_TYPE addr, /* virtual address of access */
                int spec_level,    /* passed this many mispreds */
                int thread,        /* thread of accessing inst */
                void *p,           /* input/output buffer */
                int nbytes)        /* number of bytes to access */
{
  int index;
  struct spec_mem_ent *ent, *prev;
#ifdef DEBUG_SPEC_MEM
  int better_find_it = FALSE;
#endif

  /* FIXME: partially overlapping writes are not combined... */
  /* FIXME: partially overlapping reads are not handled correctly... */

  /* check alignments, even speculative this test should always pass */
  if ((nbytes & (nbytes - 1)) != 0 || (addr & (nbytes - 1)) != 0)
  {
    /* no can do */
    return;
  }

  /* check permissions */
  if (!((addr >= ld_text_base && addr < (ld_text_base + ld_text_size) && cmd == Read) || (addr >= ld_data_base && addr < ld_stack_base)))
  {
    /* no can do */
    return;
  }

  /* Make sure any pages we allocate are marked as speculative and marked with
   * the writing thread, so we can recover the memory.  mem_access_mode_spec 
   * is a global variable checked in memory.h */
  mem_access_mode_spec = TRUE;

  /* has this memory state been copied by this thread on mis-spec write? */
  index = HASH_ADDR(addr);
  for (prev = NULL, ent = store_htable[index]; ent; prev = ent, ent = ent->next)
  {
    if (ent->addr == addr && ent->thread == thread && ent->spec_level == spec_level)
    {
      /* reorder chains to speed access into hash table */
      if (prev != NULL)
      {
        /* not at head of list, relink the hash table entry at front */
        prev->next = ent->next;
        ent->next = store_htable[index];
        store_htable[index] = ent;
      }
      break;
    }
#ifdef DEBUG_SPEC_MEM
    else if (ent->addr == addr && ent->thread == thread && ent->spec_level <= spec_level)
      better_find_it = TRUE;
#endif
  }

  dassert(!better_find_it);

  /* no, if it is a write, allocate a hash table entry to hold the data */
  if (!ent && cmd == Write)
  {
    /* try to get an entry from the free list, if available */
    if (!bucket_free_list)
    {
      /* otherwise, call calloc() to get the needed storage */
      bucket_free_list = calloc(1, sizeof(struct spec_mem_ent));
      if (!bucket_free_list)
        fatal("out of virtual memory");
    }
    ent = bucket_free_list;
    bucket_free_list = bucket_free_list->next;

    /* insert into hash table */
    ent->next = store_htable[index];
    store_htable[index] = ent;
    ent->addr = addr;
    ent->spec_level = spec_level;
    ent->thread = thread;
    ent->data[0] = 0;
    ent->data[1] = 0;
  }

  /* handle the read or write to speculative or non-speculative storage */
  switch (nbytes)
  {
  case 1:
    if (cmd == Read)
    {
      if (ent)
      {
        /* read from mis-speculated state buffer */
        *((unsigned char *)p) = *((unsigned char *)(&ent->data[0]));
      }
      else
      {
        /* read from non-speculative memory state */
        *((unsigned char *)p) = MEM_READ_BYTE(addr);
      }
    }
    else
    {
      /* always write into mis-speculated state buffer */
      *((unsigned char *)(&ent->data[0])) = *((unsigned char *)p);
    }
    break;
  case 2:
    if (cmd == Read)
    {
      if (ent)
      {
        /* read from mis-speculated state buffer */
        *((unsigned short *)p) = *((unsigned short *)(&ent->data[0]));
      }
      else
      {
        /* read from non-speculative memory state */
        *((unsigned short *)p) = MEM_READ_HALF(addr);
      }
    }
    else
    {
      /* always write into mis-speculated state buffer */
      *((unsigned short *)&ent->data[0]) = *((unsigned short *)p);
    }
    break;
  case 4:
    if (cmd == Read)
    {
      if (ent)
      {
        /* read from mis-speculated state buffer */
        *((unsigned int *)p) = *((unsigned int *)&ent->data[0]);
      }
      else
      {
        /* read from non-speculative memory state */
        *((unsigned int *)p) = MEM_READ_WORD(addr);
      }
    }
    else
    {
      /* always write into mis-speculated state buffer */
      *((unsigned int *)&ent->data[0]) = *((unsigned int *)p);
    }
    break;
  case 8:
    if (cmd == Read)
    {
      if (ent)
      {
        /* read from mis-speculated state buffer */
        *((unsigned int *)p) = *((unsigned int *)&ent->data[0]);
        *(((unsigned int *)p) + 1) = *((unsigned int *)&ent->data[1]);
      }
      else
      {
        /* read from non-speculative memory state */
        *((unsigned int *)p) = MEM_READ_WORD(addr);
        *(((unsigned int *)p) + 1) =
            MEM_READ_WORD(addr + sizeof(SS_WORD_TYPE));
      }
    }
    else
    {
      /* always write into mis-speculated state buffer */
      *((unsigned int *)&ent->data[0]) = *((unsigned int *)p);
#ifndef BUG_COMPAT_MEM
      *((unsigned int *)&ent->data[1]) = *(((unsigned int *)p) + 1);
#endif
    }
    break;
  default:
    panic("access size not supported in mis-speculative mode");
  }

  /* We are now returning to non-speculative memory accesses */
  mem_access_mode_spec = FALSE;
}

/* dump speculative memory state */
static void
mspec_dump(FILE *stream) /* output stream */
{
  int i;
  struct spec_mem_ent *ent;

  fprintf(stream, "** speculative memory contents **\n");

  for (i = 0; i < STORE_HASH_SIZE; i++)
  {
    /* dump contents of all hash table buckets */
    for (ent = store_htable[i]; ent; ent = ent->next)
    {
      fprintf(stream,
              "[0x%08x, thread %2d]: %12.0f/0x%08x:%08x, "
              "spec mode: %s, spec lev: %d\n",
              ent->addr, ent->thread,
              (double)(*((double *)ent->data)),
              *((unsigned int *)&ent->data[0]),
              *(((unsigned int *)&ent->data[0]) + 1),
              (thread_info[ent->thread].spec_mode ? "t" : "f"),
              thread_info[ent->thread].spec_level);
    }
  }
}

#ifdef USE_DLITE /* Not updated to accommodate multi-path/multi-threading */
/* default memory state accessor, used by DLite */
static char *                         /* err str, NULL for no err */
simoo_mem_obj(enum dlite_access_t at, /* access type */
              SS_ADDR_TYPE addr,      /* address to access */
              char *p,                /* input/output buffer */
              int nbytes)             /* size of access */
{
  char *errstr;
  enum mem_cmd cmd;

  if (at == at_read)
    cmd = Read;
  else if (at == at_write)
    cmd = Write;
  else
    panic("bogus access type");

  errstr = mem_valid(cmd, addr, nbytes, /* declare */ FALSE);
  if (errstr)
    return errstr;

  /* else, no error, access memory */
  if (spec_mode)
    spec_mem_access(cmd, addr, p, nbytes);
  else
    mem_access(cmd, addr, p, nbytes);

  /* no error */
  return NULL;
}
#endif

/*
 *  RUU_DISPATCH() - decode instructions and allocate RUU and LSQ resources
 */

/* link RS onto the output chain number of whichever operation will next
   create the architected register value IDEP_NAME */
static INLINE void
ruu_link_idep(struct RUU_station *rs, /* rs station to link */
              int idep_num,           /* input dependence number */
              int idep_name,          /* input register name */
              int thread)             /* current thread no. */
{
  struct CV_link head;
  struct RS_link *link;
  int spec_level;

  /* any dependence? */
  if (idep_name == NA)
  {
    /* no input dependence for this input slot, mark operand as ready */
    rs->idep_ready[idep_num] = TRUE;
    return;
  }

  /* locate creator of operand */
  spec_level = thread_info[thread].spec_level;
  head = CREATE_VECTOR(idep_name, thread);

  /* any creator? */
  if (!head.rs)
  {
    /* no active creator, use value available in architected reg file,
         indicate the operand is ready for use */
    rs->idep_ready[idep_num] = TRUE;
    return;
  }
  /* else, creator operation will make this value sometime in the future */

  /* indicate value will be created sometime in the future, i.e., operand
     is not yet ready for use */
  rs->idep_ready[idep_num] = FALSE;

  /* link onto creator's output list of dependant operand */
  RSLINK_NEW(link, rs);
  link->x.opnum = idep_num;
  link->next = head.rs->odep_list[head.odep_num];
  head.rs->odep_list[head.odep_num] = link;
}

/* make RS the creator of architected register ODEP_NAME */
static INLINE void
ruu_install_odep(struct RUU_station *rs, /* creating RUU station */
                 int odep_num,           /* output operand number */
                 int odep_name,          /* output register name */
                 int thread)             /* current thread no. */
{
  struct CV_link cv;
  int spec_level;

  /* any dependence? */
  if (odep_name == NA)
  {
    /* no value created */
    rs->onames[odep_num] = NA;
    return;
  }
  /* else, create a RS_NULL terminated output chain in create vector */

  /* record output name, used to update create vector at completion */
  rs->onames[odep_num] = odep_name;

  /* initialize output chain to empty list */
  rs->odep_list[odep_num] = NULL;

  /* indicate this operation is latest creator of ODEP_NAME */
  spec_level = thread_info[thread].spec_level;
  CVLINK_INIT(cv, rs, odep_num);
  SET_CREATE_VECTOR(odep_name, cv, thread);
}

/*
 * configure the instruction decode engine
 */

/* general register dependence decoders */
#define DGPR(N) (N)
#define DCGPR(N) (SS_COMP_OP != SS_COMP_NOP ? (N) : 0)
#define DGPR_D(N) ((N) & ~1)

/* floating point register dependence decoders */
#define DFPR_L(N) (((N) + 32) & ~1)
#define DFPR_F(N) (((N) + 32) & ~1)
#define DFPR_D(N) (((N) + 32) & ~1)

/* miscellaneous register dependence decoders */
#define DHI (0 + 32 + 32)
#define DLO (1 + 32 + 32)
#define DFCC (2 + 32 + 32)
#define DTMP (3 + 32 + 32)
#define DNA (0)

/*
 * configure the execution engine
 */

/* Warning: these require regs_PC, next_PC, PC_if_taken, spec_mode,
 * and spec_level to be locally defined */

/* next program counter */
#define SET_NPC(EXPR) (next_PC = (EXPR))

/* target program counter */
#undef SET_TPC
#define SET_TPC(EXPR) (PC_if_taken = (EXPR))

/* current program counter */
#define CPC (regs_PC)
#define SET_CPC(EXPR) (regs_PC = (EXPR))

/* general purpose register accessors, NOTE: speculative copy on write storage
   provided for fast recovery during wrong path execute (see 
   spec_mode_recover() for details on this process */
#define GPR(N) (spec_level                                    \
                    ? spec_regs_R[curr_thread][spec_level][N] \
                    : regs_R[N])
#define SET_GPR(N, EXPR) ((spec_mode == TRUE)                                            \
                              ? (void)(spec_regs_R[curr_thread][spec_level][N] = (EXPR)) \
                              : (regs_R[N] = (EXPR)))

/* floating point register accessors, NOTE: speculative copy on write storage
   provided for fast recovery during wrong path execute (see 
   spec_mode_recover() for details on this process */
#define FPR_L(N) (spec_level                                        \
                      ? spec_regs_F[curr_thread][spec_level].l[(N)] \
                      : regs_F.l[(N)])
#define SET_FPR_L(N, EXPR) ((spec_mode == TRUE)                                              \
                                ? (void)(spec_regs_F[curr_thread][spec_level].l[N] = (EXPR)) \
                                : (regs_F.l[(N)] = (EXPR)))
#define FPR_F(N) (spec_level                                        \
                      ? spec_regs_F[curr_thread][spec_level].f[(N)] \
                      : regs_F.f[(N)])
#define SET_FPR_F(N, EXPR) ((spec_mode == TRUE)                                              \
                                ? (void)(spec_regs_F[curr_thread][spec_level].f[N] = (EXPR)) \
                                : (regs_F.f[(N)] = (EXPR)))
#define FPR_D(N) (spec_level                                             \
                      ? spec_regs_F[curr_thread][spec_level].d[(N) >> 1] \
                      : regs_F.d[(N) >> 1])
#define SET_FPR_D(N, EXPR) ((spec_mode == TRUE)                                                     \
                                ? (void)(spec_regs_F[curr_thread][spec_level].d[(N) >> 1] = (EXPR)) \
                                : (regs_F.d[(N) >> 1] = (EXPR)))

/* miscellanous register accessors, NOTE: speculative copy on write storage
   provided for fast recovery during wrong path execute (see 
   spec_mode_recover() for details on this process */
#define HI (spec_level                                  \
                ? spec_regs_HI[curr_thread][spec_level] \
                : regs_HI)
#define SET_HI(EXPR) ((spec_mode == TRUE)                                          \
                          ? (void)(spec_regs_HI[curr_thread][spec_level] = (EXPR)) \
                          : (regs_HI = (EXPR)))
#define LO (spec_level                                  \
                ? spec_regs_LO[curr_thread][spec_level] \
                : regs_LO)
#define SET_LO(EXPR) ((spec_mode == TRUE)                                          \
                          ? (void)(spec_regs_LO[curr_thread][spec_level] = (EXPR)) \
                          : (regs_LO = (EXPR)))
#define FCC (spec_level                                   \
                 ? spec_regs_FCC[curr_thread][spec_level] \
                 : regs_FCC)
#define SET_FCC(EXPR) ((spec_mode == TRUE)                                           \
                           ? (void)(spec_regs_FCC[curr_thread][spec_level] = (EXPR)) \
                           : (regs_FCC = (EXPR)))

/* precise architected memory state accessor macros, NOTE: speculative copy on
   write storage provided for fast recovery during wrong path execute (see
   spec_mode_recover() for details on this process. */
#define __READ_SPECMEM(DST_T, SRC_N, SRC, THREAD, SPEC_LEVEL)                  \
  (addr = (SRC),                                                               \
   ((spec_mode == TRUE)                                                        \
        ? spec_mem_access(Read, addr, (SPEC_LEVEL), (THREAD),                  \
                          SYMCAT(&temp_, SRC_N), sizeof(SYMCAT(temp_, SRC_N))) \
        : mem_access(Read, addr, SYMCAT(&temp_, SRC_N),                        \
                     sizeof(SYMCAT(temp_, SRC_N)))),                           \
   ((unsigned int)((DST_T)SYMCAT(temp_, SRC_N))))

/* Currently these can only be called from ruu_dispatch() */
#define READ_WORD(SRC) \
  __READ_SPECMEM(unsigned int, uint, (SRC), curr_thread, spec_level)
#define READ_UNSIGNED_HALF(SRC) \
  __READ_SPECMEM(unsigned int, ushort, (SRC), curr_thread, spec_level)
#define READ_SIGNED_HALF(SRC) \
  __READ_SPECMEM(signed int, short, (SRC), curr_thread, spec_level)
#define READ_UNSIGNED_BYTE(SRC) \
  __READ_SPECMEM(unsigned int, uchar, (SRC), curr_thread, spec_level)
#define READ_SIGNED_BYTE(SRC) \
  __READ_SPECMEM(signed int, char, (SRC), curr_thread, spec_level)

#define __WRITE_SPECMEM(SRC, DST_T, DST_N, DST, THREAD, SPEC_LEVEL)            \
  (addr = (DST),                                                               \
   SYMCAT(temp_, DST_N) = (DST_T)((unsigned int)(SRC)),                        \
   ((spec_mode == TRUE)                                                        \
        ? spec_mem_access(Write, addr, (SPEC_LEVEL), (THREAD),                 \
                          SYMCAT(&temp_, DST_N), sizeof(SYMCAT(temp_, DST_N))) \
        : mem_access(Write, addr, SYMCAT(&temp_, DST_N),                       \
                     sizeof(SYMCAT(temp_, DST_N)))))

#define WRITE_WORD(SRC, DST) \
  __WRITE_SPECMEM((SRC), unsigned int, uint, (DST), curr_thread, spec_level)
#define WRITE_HALF(SRC, DST) \
  __WRITE_SPECMEM((SRC), unsigned short, ushort, (DST), curr_thread, spec_level)
#define WRITE_BYTE(SRC, DST) \
  __WRITE_SPECMEM((SRC), unsigned char, uchar, (DST), curr_thread, spec_level)

/* system call handler macro */
#ifdef NO_SYSCALL_STALL
#define SYSCALL(INST) \
  ((spec_mode == TRUE) ? (void)0 : ss_syscall(mem_access, INST))
#else
#define SYSCALL(INST)                                              \
  (/* only execute system calls in non-speculative mode */         \
   ((spec_mode == TRUE) ? panic("speculative syscall") : (void)0), \
   ss_syscall(mem_access, INST))
#endif

/* default register state accessor, used by DLite and ptrace*/
static char *                             /* err str, NULL for no err */
simoo_reg_obj(enum dlite_access_t at,     /* access type */
              enum dlite_reg_t rt,        /* reg bank to probe */
              int reg,                    /* register number */
              int thread,                 /* thread number */
              union dlite_reg_val_t *val) /* input, output */
{
  int curr_thread = thread;
  int spec_mode = thread_info[curr_thread].spec_mode;
  int spec_level = thread_info[curr_thread].spec_level;

  if (reg < 0 || reg >= SS_NUM_REGS)
    return "register number out of range";

  if (at == at_read || at == at_write)
  {
    switch (rt)
    {
    case rt_gpr:
      if (at == at_read)
        val->as_word = GPR(reg);
      else
        SET_GPR(reg, val->as_word);
      break;
    case rt_lpr:
      if (at == at_read)
        val->as_word = FPR_L(reg);
      else
        SET_FPR_L(reg, val->as_word);
      break;
    case rt_fpr:
      if (at == at_read)
        val->as_float = FPR_F(reg);
      else
        SET_FPR_F(reg, val->as_float);
      break;
    case rt_dpr:
      /* 1/2 as many regs in this mode */
      if (reg < 0 || reg >= SS_NUM_REGS / 2)
        return "register number out of range";

      if (at == at_read)
        val->as_double = FPR_D(reg * 2);
      else
        SET_FPR_D(reg * 2, val->as_double);
      break;
    case rt_hi:
      if (at == at_read)
        val->as_word = HI;
      else
        SET_HI(val->as_word);
      break;
    case rt_lo:
      if (at == at_read)
        val->as_word = LO;
      else
        SET_LO(val->as_word);
      break;
    case rt_FCC:
      if (at == at_read)
        val->as_condition = FCC;
      else
        SET_FCC(val->as_condition);
      break;
    case rt_PC:
      if (at == at_read)
        val->as_address = regs_PC;
      else
        regs_PC = val->as_address;
      break;
    default:
      panic("bogus register bank");
    }
  }
  else
    panic("bogus access type");

  /* no error */
  return NULL;
}

/* wrapper function: given a register "dependency number", call 
 * simoo_reg_obj() with the correct arguments to get the register's contents */
void get_reg_contents(int regnum,                 /* result reg's "dep number" */
                      int curr_thread,            /* current thread */
                      enum ss_opcode op,          /* this inst's opcode */
                      union dlite_reg_val_t *val) /* store result here */
{
  char *errstr = NULL;

  if (regnum < 0)
    panic("Invalid reg num in get_reg_contents(): %d", regnum);
  else if (regnum < SS_NUM_REGS)
    errstr = simoo_reg_obj(at_read, rt_gpr, regnum, curr_thread, val);
  else if (regnum < 2 * SS_NUM_REGS)
  {
    if (SS_OP_FLAGS(op) & F_DDEP)
      /* this inst uses (mostly) double registers; get the register
	 * contents as a double */
      errstr = simoo_reg_obj(at_read, rt_dpr, (regnum - SS_NUM_REGS) >> 1,
                             curr_thread, val);
    else
      /* this inst uses single-prec. registers; get the register 
	 * contents as single */
      errstr = simoo_reg_obj(at_read, rt_fpr, regnum - SS_NUM_REGS,
                             curr_thread, val);
  }
  else if (regnum == DHI)
    errstr = simoo_reg_obj(at_read, rt_hi, 0, curr_thread, val);
  else if (regnum == DLO)
    errstr = simoo_reg_obj(at_read, rt_lo, 0, curr_thread, val);
  else if (regnum == DFCC)
    errstr = simoo_reg_obj(at_read, rt_FCC, 0, curr_thread, val);
  else
    panic("Invalid reg num in get_reg_contents(): %d", regnum);

  if (errstr)
    panic(errstr);
}

/* wrapper function: get this instruction's operand values and store them 
 * in in_vals */
void get_operands(int in1, int in2, int in3,      /* operand regs' "dep numbers" */
                  int curr_thread,                /* current thread */
                  enum ss_opcode op,              /* this inst's opcode */
                  union dlite_reg_val_t *in_vals) /* store operand values here */
{
  if (in1)
    get_reg_contents(in1, curr_thread, op, &in_vals[0]);
  if (in2)
    get_reg_contents(in2, curr_thread, op, &in_vals[1]);
  if (in3)
    get_reg_contents(in3, curr_thread, op, &in_vals[2]);
}

/* wrapper function: get this instruction's result values and store them 
 * in out_vals */
void get_results(int out1, int out2,              /* result regs' "dep numbers" */
                 int curr_thread,                 /* current thread */
                 enum ss_opcode op,               /* this inst's opcode */
                 union dlite_reg_val_t *out_vals) /* store result values here */
{
  if (out1)
    get_reg_contents(out1, curr_thread, op, &out_vals[0]);
  if (out2)
    get_reg_contents(out2, curr_thread, op, &out_vals[1]);
}

/* non-zero for a valid address */
#define VALID_ADDR(ADDR) (SS_DATA_BASE <= (ADDR) && (ADDR) <= SS_STACK_BASE)

/* the last operation that ruu_dispatch() attempted to dispatch, for
   implementing in-order issue */
static struct RS_link last_op = RSLINK_NULL_DATA;

/* dispatch instructions from the IFETCH -> DISPATCH queue: instructions are
   first decoded, then they allocated RUU (and LSQ for load/stores) resources
   and input and output dependence chains are updated accordingly */
static void
ruu_dispatch(void)
{
  /* Note: we define a local regs_PC here, making the top-level regs_PC
   * inaccessible */
  int i, n;
  int n_dispatched = 0;                        /* total insts dispatched */
  SS_INST_TYPE inst;                           /* actual instruction bits */
  enum ss_opcode op;                           /* decoded opcode enum */
  int out1, out2, in1, in2, in3;               /* output/input register names */
  SS_ADDR_TYPE regs_PC;                        /* current PC address */
  SS_ADDR_TYPE next_PC;                        /* true next PC address */
  SS_ADDR_TYPE pred_PC;                        /* predicted next PC address */
  SS_ADDR_TYPE PC_if_taken;                    /* next PC if branch is taken */
  SS_ADDR_TYPE base_pred_PC;                   /* pred PC if branch weren't forked */
  SS_ADDR_TYPE predPC_if_taken;                /* BTB's pred PC if branch is taken */
  SS_ADDR_TYPE patch_PC;                       /* on misfetch, redirected address */
  SS_ADDR_TYPE addr;                           /* effective address, if load/store */
  struct RUU_station *rs;                      /* RUU station being allocated */
  struct RUU_station *lsq;                     /* LSQ station for ld/st's */
  int RUU_old_tail;                            /* stores old tail; needed for NOPs */
  struct bpred_update_info b_update_rec;       /* bpred info needed for update */
  struct bpred_recover_info bpred_recover_rec; /* retstack fixup info */
  struct bpred_recover_info retstack_copy_rec; /* for fork-in-decode */
  unsigned int pseq;                           /* pipetrace sequence number */
  SS_TIME_TYPE fetched_at;                     /* when current inst was fetched */
  int could_have_forked;                       /* forking resources available */
  int conf;                                    /* for branches, conf prediction */
  int conf_data;                               /* data returned by conf pred. */
  int forked;                                  /* for branches, did this one fork? */
  int forked_thread = -1;                      /* if this branch forked, id of child*/
  int curr_thread;                             /* which thread's inst? */
  BITMAP_TYPE(N_SPEC_LEVELS, fork_hist_bmap);  /* fork history bitmap */
  BITMAP_ENT_TYPE fork_hist_bmap_ptr;          /* ptr to this inst's pos'n in bmap */
  int is_write;                                /* store? */
  int spec_mode;                               /* is trace-gen in mis-spec mode? */
  int spec_level;                              /* passed this many mispred branches */
  int br_taken, br_pred_taken,                 /* if this was a branch, was it      */
      would_be_taken;                          /* taken?  predicted taken?  would it*
					 * be pred-taken if non-forking?     */
  int patch_type;                              /* patching misfetches? */
  int fetch_redirected;                        /* true if ruu_dispatch redirects */
  int decode_depth;                            /* records RUU depth where inst is
					 * enqueued */
  unsigned int temp_uint = 0;                  /* temp variable for spec mem access */
  signed short temp_short = 0;                 /* " ditto " */
  unsigned short temp_ushort = 0;              /* " ditto " */
  signed char temp_char = 0;                   /* " ditto " */
  unsigned char temp_uchar = 0;                /* " ditto " */
  union dlite_reg_val_t in_vals[3];            /* instruction operands, for ptrace */
  union dlite_reg_val_t out_vals[2];           /* instruction results,   "     " */
#ifdef USE_DLITE
  int made_check = FALSE; /* used to ensure DLite entry */
#endif
#ifdef DEBUG
  int n_non_spec = 0, j;
#endif

  if (RUU_num == RUU_size)
    ruu_overflows++;
  else if (LSQ_num == LSQ_size)
    lsq_overflows++;

  while (/* instruction decode B/W left? */
         n_dispatched < ruu_decode_width
         /* RUU and LSQ not full? */
         && RUU_num < RUU_size && LSQ_num < LSQ_size
         /* insts still available from fetch unit? */
         && ifq_num != 0)
  {
    fetch_redirected = FALSE;
    patch_type = 0;

    if (!ifq[ifq_head].valid)
      goto empty_fetch_entry;

    /* if not issuing mis-speculated instructions, 
       * 1. if not doing multi-path, stall until branch is resolved 
       * 2. otherwise, purge this misspeculated path from the IFQ so
       *    correctly-speculated paths can continue */
    if (!ruu_include_spec && (thread_info[ifq[ifq_head].thread].spec_mode == TRUE))
    {
      if (num_active_forks >= 1)
      {
        /* Warning: hasn't been properly tested */
        int old_squashed = thread_info[curr_thread].squashed;
        thread_info[curr_thread].squashed = TRUE;
        tracer_recover_wrapper(ifq[ifq_head].thread,
                               ifq[ifq_head].regs_PC,
                               ifq[ifq_head].regs_PC,
                               FALSE, 0);
        /* FIXME: need to repair ret-addr stack; use stack-copy */
        thread_info[curr_thread].squashed = old_squashed;
        continue;
      }

      break;
    }

    /* if issuing in-order, block until last op issues */
    if (ruu_inorder_issue && (last_op.rs && RSLINK_VALID(&last_op) && !OPERANDS_READY(last_op.rs)))
    {
      break;
    }

    /* get the next instruction from the IFETCH -> DISPATCH queue */
    inst = ifq[ifq_head].IR;
    regs_PC = ifq[ifq_head].regs_PC;
    pred_PC = patch_PC = ifq[ifq_head].pred_PC;
    base_pred_PC = ifq[ifq_head].base_pred_PC;
    predPC_if_taken = ifq[ifq_head].predPC_if_taken;
    b_update_rec = ifq[ifq_head].b_update_rec;
    bpred_recover_rec = ifq[ifq_head].bpred_recover_rec;
    retstack_copy_rec = ifq[ifq_head].retstack_copy_rec;
    pseq = ifq[ifq_head].ptrace_seq;
    fetched_at = ifq[ifq_head].fetched_at;
    could_have_forked = ifq[ifq_head].could_have_forked;
    dassert(could_have_forked >= 0);
    conf = ifq[ifq_head].conf;
    conf_data = ifq[ifq_head].conf_data;
    forked = ifq[ifq_head].forked;
    assert(fork_in_fetch || !forked);
    forked_thread = ifq[ifq_head].forked_thread;
    curr_thread = ifq[ifq_head].thread;
    if (bconf_type != BCF_Omni && fork_in_fetch)
    {
      fork_hist_bmap_ptr = ifq[ifq_head].fork_hist_bmap_ptr;
      BITMAP_COPY(fork_hist_bmap, ifq[ifq_head].fork_hist_bmap,
                  THREADS_BMAP_SZ);
    }
    else /* Omni or fork-in-dispatch */
    {
      fork_hist_bmap_ptr = thread_info[curr_thread].fork_hist_bmap_ptr;
      BITMAP_COPY(fork_hist_bmap, thread_info[curr_thread].fork_hist_bmap,
                  THREADS_BMAP_SZ);
    }
    spec_mode = thread_info[curr_thread].spec_mode;
    spec_level = thread_info[curr_thread].spec_level;
    dassert(spec_mode != UNKNOWN);
    dassert(spec_level >= 0);

    /* decode the inst */
    op = SS_OPCODE(inst);

    /* sanity checks */
    assert(thread_info[curr_thread].valid == TRUE && thread_info[curr_thread].squashed == UNKNOWN);
#ifdef DEBUG
    if (forked)
      assert(SS_OP_FLAGS(op) & F_COND);
#endif

    /* compute default next_PC */
    next_PC = regs_PC + sizeof(SS_INST_TYPE);
    if (!(SS_OP_FLAGS(op) & F_CTRL))
      assert(pred_PC == next_PC);

    if ((SS_OP_FLAGS(op) & F_FCOMP) && FIQ_occ == FIQ_size)
      break;
    else if ((SS_OP_FLAGS(op) & F_MEM) && IIQ_occ >= IIQ_size - 1)
      break;
    else if (!(SS_OP_FLAGS(op) & F_FCOMP) && IIQ_occ == IIQ_size)
      break;

      /* drain RUU for TRAPs and system calls */
#ifndef NO_SYSCALL_STALL
    if (SS_OP_FLAGS(op) & F_TRAP)
    {
      if (RUU_num != 0)
        break;

      /* else, syscall is only instruction in the machine, at this
	     point we should not be in (mis-)speculative mode */
      if (spec_mode == TRUE)
        panic("drained and speculative");
    }
#endif

    /* this thread did something vaguely useful */
    thread_info[curr_thread].did_decode = TRUE;

    /* maintain $r0 semantics (in spec and non-spec space) */
    regs_R[0] = 0;
    spec_regs_R[curr_thread][spec_level][0] = 0;

    /* default effective address (none) and access */
    addr = 0;
    is_write = FALSE;

    /* more decoding and execution */
    switch (op)
    {

/* the divide-by-zero instruction check macro is redefined because there
   is no portable and reliable way to hide integer divide-by-zero faults */
#undef DIV0
#define DIV0(N) ((spec_mode == TRUE)            \
                     ? /* do nothing */ (void)0 \
                     : (((N) == 0) ? IFAIL("divide by 0") : (void)0))

/* the divide operator semantics are also redefined because there
   are no portable and reliable way to hide integer divide-by-zero faults */
#undef IDIV
#define IDIV(A, B) (((B) == 0) ? 0 : ((A) / (B)))

#undef IMOD
#define IMOD(A, B) (((B) == 0) ? 0 : ((A) % (B)))

#undef FDIV
#define FDIV(A, B) (((B) == 0) ? 0 : ((A) / (B)))

#undef FINT
#define FINT(A) (isnan(A) ? 0 : ((int)(A)))

/* decode and execute the instruction */
/* the following macro wraps the instruction check failure declaration
   with a test to see if the trace generator is in non-speculative
   mode, if so the instruction fault is declared, otherwise, the error
   is shunted because instruction faults need to be masked on the
   mis-speculated instruction paths */
#undef IFAIL
#define IFAIL(S) \
  ((spec_mode == TRUE) ? /* ignore */ (void)0 : /* declare */ (void)fatal(S))

#define DEFINST(OP, MSK, NAME, OPFORM, RES, CLASS, O1, O2, I1, I2, I3, EXPR) \
  case OP:                                                                   \
    out1 = O1;                                                               \
    out2 = O2;                                                               \
    in1 = I1;                                                                \
    in2 = I2;                                                                \
    in3 = I3;                                                                \
    if (ptrace_level & PTRACE_VERBOSE_MASK)                                  \
      get_operands(in1, in2, in3, curr_thread, op, in_vals);                 \
    EXPR;                                                                    \
    if (ptrace_level & PTRACE_VERBOSE_MASK)                                  \
      get_results(out1, out2, curr_thread, op, out_vals);                    \
    break;
#define DEFLINK(OP, MSK, NAME, MASK, SHIFT)       \
  case OP:                                        \
    /* could speculatively decode a bogus inst */ \
    op = NOP;                                     \
    out1 = NA;                                    \
    out2 = NA;                                    \
    in1 = NA;                                     \
    in2 = NA;                                     \
    in3 = NA;                                     \
    /* no EXPR */                                 \
    break;
#define CONNECT(OP)
#include "ss.def"
#undef DEFINST
#undef DEFLINK
#undef CONNECT
    default:
      /* can speculatively decode a bogus inst */
      op = NOP;
      out1 = NA;
      out2 = NA;
      in1 = NA;
      in2 = NA;
      in3 = NA;
      /* no EXPR */
    }
    /* operation sets regs_PC, next_PC, and PC_if_taken */

    br_taken = (next_PC != (regs_PC + sizeof(SS_INST_TYPE)));
    br_pred_taken = (pred_PC != (regs_PC + sizeof(SS_INST_TYPE)));
    would_be_taken = (base_pred_PC != (regs_PC + sizeof(SS_INST_TYPE)));

#define PATCH_PERFECT 1
#define PATCH_MISFETCH 2
#define PATCH_BTB_IJMP 3
#define PATCH_SYNTH_UP 4
#define PATCH_SYNTH_DOWN 5
#define PATCH_NOT_TAKEN 6

    if (/* any kind of misfetch under perfect-bpred */
        (pred_PC != next_PC && pred_perfect && (patch_type = PATCH_PERFECT))
        /* or, misfetch (ie right dir, wrong target) on a direct branch */
        || ((SS_OP_FLAGS(op) & (F_CTRL | F_DIRJMP)) == (F_CTRL | F_DIRJMP) && PC_if_taken != pred_PC && br_pred_taken && (patch_type = PATCH_MISFETCH))
        /* or, misfetch on an indirect branch (used only to make
	   * indir branches always hit the BTB to measure the impact
	   * of their BTB misses) */
        || ((SS_OP_FLAGS(op) & (F_CTRL | F_INDIRJMP)) == (F_CTRL | F_INDIRJMP) && pred_PC != next_PC && fix_addrs_indir && (patch_type = PATCH_BTB_IJMP))
        /* or, direction-mispredict under synthetic-boosted bpred */
        || ((SS_OP_FLAGS(op) & (F_CTRL | F_COND)) == (F_CTRL | F_COND) && br_taken != br_pred_taken && pred_synth_up && (patch_type = PATCH_SYNTH_UP)) || ((SS_OP_FLAGS(op) & (F_CTRL | F_COND)) == (F_CTRL | F_COND) && br_taken == br_pred_taken && pred_synth_down && (patch_type = PATCH_SYNTH_DOWN)) || (pred_PC != next_PC && (pred->class == BPredNotTaken || !pred) && (SS_OP_FLAGS(op) & (F_DIRJMP | F_INDIRJMP)) && !(SS_OP_FLAGS(op) & F_COND) && br_taken && (patch_type = PATCH_NOT_TAKEN)))
    {
      /* 1. Either we're simulating perfect prediction and
	   * are in a mis-predict state and need to patch up (PERFECT), or 
	   * 2. We're not simulating perfect prediction, we've
	   * predicted the branch taken and our predicted target
	   * doesn't match the computed target (MISFETCH), or
	   * 3. We're simulating a perfect BTB in some way, the predicted
	   * target doesn't match the computed target, and we need to patch 
	   * up, or (option 'fix_addrs' is true or, for indirect jumps,
	   * option 'fix_addrs_indir' is true and BTB_IJMP)
	   * 4,5. We're synthetically boosting branch-prediction accuracy
	   * by randomly making mispredictions look like correct predictions,
	   * or synthetically breaking correct predictions.
	   * (SYNTH)
	   * 6. We're using a not-taken predictor but want to still
	   * treat jumps as pred-taken
	   *
	   * If case #2, possibly charge a mispredict penatly for redirecting
	   * fetch 
	   * In case 3, we patch the BTB miss with no penalty
	   * (fix_addrs or fix_indir_addrs)
	   *
	   * What we do:
	   * just update the PC values and call tracer_recover() -- it
	   * does a fetch squash and redirects this thread to the appropriate
	   * PC.  We protect this inst's ifq entry, anticipating the
	   * update at the end of ruu_dispatch()
	   */
      int old_squashed = thread_info[curr_thread].squashed;
      int perfect = pred_perfect;
      int penalty;
      int junk;
      double ranval;

      assert(!forked);

      /* handle synthetic branch-accuracy decreasing.  Have to have
	   * part of the work here; a correctly-pred but misfetched
	   * branch that enters this mongo patch clause must be considered
	   * for synthetic misprediction */
      if (pred_synth_down && br_taken == br_pred_taken && (SS_OP_FLAGS(op) & F_COND))
      {
        dassert(!pred_perfect);
        ranval = (double)random();
        ranval /= (double)RAN_MAX;
        if (ranval < pred_synth_down_threshold)
        {
          if (br_pred_taken)
            base_pred_PC = pred_PC = patch_PC = regs_PC + SS_INST_SIZE;
          else
            base_pred_PC = pred_PC = patch_PC = PC_if_taken;

          dassert(patch_PC != next_PC || PC_if_taken == regs_PC + SS_INST_SIZE);
          penalty = 0;
          goto dump_ifq;
        }
        else if (patch_type == PATCH_SYNTH_DOWN)
          goto normal_branches;
      }

      /* handle synthetic branch boosting */
      if (pred_synth_up && br_taken != br_pred_taken && (SS_OP_FLAGS(op) & F_COND))
      {
        dassert(!pred_perfect && (SS_OP_FLAGS(op) & F_COND));
        ranval = (double)random();
        ranval /= (double)RAN_MAX;
        if (ranval > pred_synth_up_threshold)
          goto normal_branches;
        else
        {
          /* charge for fixing a BTB miss if appropriate -- this
		   * separates PHT and BTB effects */
          if (br_taken && predPC_if_taken != PC_if_taken && !fix_addrs)
            penalty = ruu_branch_penalty;
          else
            penalty = 0;

          base_pred_PC = pred_PC = patch_PC = next_PC;

          goto dump_ifq;
        }
      }

      /* else we're doing pred_perfect or patching a BTB miss */
      dassert(pred_perfect || br_pred_taken || patch_type == PATCH_BTB_IJMP || patch_type == PATCH_NOT_TAKEN);

      /* charge a penalty for redirecting fetch?  Not if pred_perfect, or 
	   * if we're "perfecting" BTB misses (ie pretending they were hits) */
      if (perfect || fix_addrs || patch_type == PATCH_BTB_IJMP)
        penalty = 0;
      else
        penalty = ruu_branch_penalty;

      /* choose the correct PC to use for patching -- PC_if_taken
	   * for imperfect pred, otherwise next_PC.  Also, if perfect bpred, 
	   * fix pred_PC so anything that uses it will believe the prediction 
	   * was correct */
      if (perfect || (fix_addrs && br_taken) || patch_type == PATCH_BTB_IJMP)
      {
        if (!(op == JR && RS == 31) || !perfect_except_retstack)
          base_pred_PC = pred_PC = patch_PC = next_PC;
      }
      else if (fix_addrs || patch_type == PATCH_NOT_TAKEN)
        base_pred_PC = pred_PC = patch_PC = PC_if_taken;
      else
        patch_PC = PC_if_taken;

      dassert(br_taken == br_pred_taken || patch_PC != next_PC || pred_perfect || patch_type == PATCH_BTB_IJMP || patch_type == PATCH_NOT_TAKEN);

    dump_ifq:
      /* use tracer_recover to patch IF->DA queue, since it knows
	   * how to squash only instructions associated with this misfetch 
	   * while leaving those from other threads alone.  We do a little
	   * hackery first so that this branch isn't touched */
      ifq_num--; /* tracer_recover goes tail-backward, so this
				 * is sufficient to protect this branch */
      thread_info[curr_thread].squashed = TRUE;
      tracer_recover_wrapper(curr_thread, regs_PC, patch_PC,
                             TRUE, penalty);
      thread_info[curr_thread].squashed = old_squashed;
      ifq_num++;

      /* recover the retstack(s) */
      hydra_bpred_recover(curr_thread, regs_PC, base_pred_PC, op,
                          &bpred_recover_rec, &junk,
                          Decode, "decode");

      fetch_redirected = TRUE;
    }

  normal_branches:

    /* if this branch forked a new path and that path is following the
       * wrong PC, fix up. Note this only happens with conditional branches */
    if (forked && predPC_if_taken != PC_if_taken)
    {
      assert(fork_in_fetch);

      thread_refork(forked_thread, curr_thread,
                    regs_PC, base_pred_PC, PC_if_taken, op,
                    fork_hist_bmap, fork_hist_bmap_ptr,
                    &bpred_recover_rec, ruu_branch_penalty);
      fetch_redirected = TRUE;
    }
    /* 
       * omniscient forking: fork iff the branch mispredicted (and there
       * are thread contexts available 
       */
    else if (bconf_type == BCF_Omni && /* if approx'ing fork-in-fetch, resources avail at fetch time */
             (could_have_forked > 0 || !approx_fork_in_fetch) && /* resources still avail */ num_active_forks < max_threads - 1 && /* this branch mispredicted */ patch_PC != next_PC && /* only fork for cond br's */ (SS_OP_FLAGS(op) & F_COND) && /* only fork from the correct path */ spec_mode != TRUE)
    {
      /* this branch mispredicted (not just misfetched), we're
	   * doing omniscient forking, and there are forking resources avail.
	   * Normally the forked path is always the taken path, and the
	   * parent path follows the non-taken path.  Here, we may already
	   * have fetched beyond the branch, so the forked-off path needs
	   * to follow which ever path this branch did not. */
      int penalty;
      int br_followed = (patch_PC != regs_PC + SS_INST_SIZE);
      int fork_PC = (br_followed ? (regs_PC + SS_INST_SIZE) : PC_if_taken);
      dassert(PC_if_taken && PC_if_taken != regs_PC);

      if (thread_info[curr_thread].pred_path_token || !fork_prune)
      {
        forked_thread = fork_thread(fork_PC, curr_thread);
        assert(retstack_copy_rec.contents.stack_copy);
        if (per_thread_retstack == PerThreadStacks && forked_thread >= 0)
        {
          thread_info[forked_thread].retstack_tos = retstack_copy_rec.tos;
          memcpy(thread_info[forked_thread].retstack,
                 retstack_copy_rec.contents.stack_copy,
                 retstack_size * sizeof(struct bpred_btb_ent));
        }
        else
        {
          pred->retstack.tos = retstack_copy_rec.tos;
          memcpy(pred->retstack.stack,
                 retstack_copy_rec.contents.stack_copy,
                 pred->retstack.size * sizeof(struct bpred_btb_ent));
        }
      }
      else
      {
        forked_thread = -2;
        if (fork_prune && num_active_forks < max_threads - 1)
          total_forks_pruned++;
      }
      assert(num_active_forks >= 0 && num_active_forks < max_threads);

      if (forked_thread >= 0)
      {
        forked = TRUE;

        /* this thread needs its fork_hist info updated */
        thread_info[curr_thread].fork_hist_bmap_ptr = (thread_info[curr_thread].fork_hist_bmap_ptr + 1) % N_SPEC_LEVELS;

        if (br_followed)
        /* this thread is now on the taken path */
        {
          (void)BITMAP_SET(thread_info[curr_thread].fork_hist_bmap,
                           THREADS_BMAP_SZ,
                           thread_info[curr_thread].fork_hist_bmap_ptr);
          (void)BITMAP_CLEAR(thread_info[forked_thread].fork_hist_bmap,
                             THREADS_BMAP_SZ,
                             thread_info[forked_thread].fork_hist_bmap_ptr);
          if (done_priming)
            num_fork_dispatch_timing_inaccuracies++;
          penalty = (penalize_fork_nt ? ruu_fork_penalty : 0);
        }
        else
        {
          /* this thread is now on the not-taken path */
          (void)BITMAP_CLEAR(thread_info[curr_thread].fork_hist_bmap,
                             THREADS_BMAP_SZ,
                             thread_info[curr_thread].fork_hist_bmap_ptr);
          penalty = ruu_fork_penalty + (fetch_redirected ? ruu_branch_penalty : 0);
          /* FIXME: I think 'fetch_redirected' is always 0 */
        }

        /* correct the forked threads fetchable time to reflect
	       * time spent going from IF->DA; this accounts for additional
	       * misfetch penalties */
        thread_info[forked_thread].fetchable = fetched_at + penalty;

        /* if ptracing, record the fork */
        if (ptrace_level != PTRACE_FUNSIM)
          ptrace_newthread(forked_thread, regs_PC, fork_PC);

        /* adjust forking resources: if we fork here, we
	       * consume forking resources that subsequent insts'
	       * 'could_have_forked' values don't reflect */
        if (approx_fork_in_fetch)
          for (i = 0, n = 0; n < ifq_num; i = (i + 1) % ruu_ifq_size, n++)
            if (ifq[i].could_have_forked > 0)
              ifq[i].could_have_forked--;
      }
      dassert(base_pred_PC == pred_PC);

      conf = LowConf;
    }
    else if (bconf_type == BCF_Omni && /* this branch mispredicted */ patch_PC != next_PC && /* only fork for cond br's */ (SS_OP_FLAGS(op) & F_COND) && /* only fork from the correct path */ spec_mode != TRUE)
    {
      assert(!could_have_forked || num_active_forks >= max_threads - 1);

      if (done_priming)
      {
        num_failed_omni_forks++;
        if (could_have_forked && approx_fork_in_fetch)
          /* resources avail at fetch (if approx'ing fork-in-fetch),
		 * but can't fork now */
          num_fork_dispatch_resource_inaccuracies++;
      }
      conf = LowConf;
    }
    else if (bconf_type == BCF_Omni && /* only consider cond br's */ (SS_OP_FLAGS(op) & F_COND))
    {
      assert(patch_PC == next_PC || spec_mode == TRUE);
      conf = HighConf;
    }
    /* 
       * in-dispatch forking
       * note conf-prediction comes from lookup done in ruu_fetch
       */
    else if (!fork_in_fetch && bconf_type != BCF_Omni)
    {
      if (                                                                                                                      /* if approx'ing fork-in-fetch, resources avail at fetch time */
          (could_have_forked > 0 || !approx_fork_in_fetch) && /* resources still avail */ num_active_forks < max_threads - 1 && /* low-confidence branch or semi-omni fork-mispred */
          (conf == LowConf || (patch_PC != next_PC && !spec_mode && bconf_fork_mispred)) && /* only fork for cond br's */ (SS_OP_FLAGS(op) & F_COND))
      {
        /* this branch is low-conf and there are forking resources avail.
	       * Normally the forked path is always the taken path, and the
	       * parent path follows the non-taken path.  Here, we may already
	       * have fetched beyond the branch, so the forked-off path needs
	       * to follow which ever path this branch did not. */
        int penalty;
        int br_followed = (patch_PC != regs_PC + SS_INST_SIZE);
        int fork_PC = (br_followed
                           ? (regs_PC + SS_INST_SIZE)
                           : PC_if_taken);
        dassert(PC_if_taken && PC_if_taken != regs_PC);

        /* handle semi-omni-squash-extra */
        if (!bconf_squash_extra || (patch_PC != next_PC && !spec_mode))
        {
          if (thread_info[curr_thread].pred_path_token || !fork_prune)
          {
            forked_thread = fork_thread(fork_PC, curr_thread);
            assert(retstack_copy_rec.contents.stack_copy);
            if (per_thread_retstack == PerThreadStacks && forked_thread >= 0)
            {
              thread_info[forked_thread].retstack_tos = retstack_copy_rec.tos;
              memcpy(thread_info[forked_thread].retstack,
                     retstack_copy_rec.contents.stack_copy,
                     retstack_size * sizeof(struct bpred_btb_ent));
            }
            else
            {
              pred->retstack.tos = retstack_copy_rec.tos;
              memcpy(pred->retstack.stack,
                     retstack_copy_rec.contents.stack_copy,
                     pred->retstack.size * sizeof(struct bpred_btb_ent));
            }
          }
          else
          {
            forked_thread = -2;
            if (fork_prune && num_active_forks < max_threads - 1)
              total_forks_pruned++;
          }
        }
        else
        {
          assert(bconf_squash_extra);
          total_bad_forks_squashed++;
          forked_thread = -2;
        }
        assert(num_active_forks >= 0 && num_active_forks < max_threads);

        if (forked_thread >= 0)
        {
          forked = TRUE;
          if (conf == HighConf)
          {
            assert(patch_PC != next_PC && !spec_mode && bconf_fork_mispred);
            total_extra_forked++;
          }

          /* this thread needs its fork_hist info updated */
          thread_info[curr_thread].fork_hist_bmap_ptr = (thread_info[curr_thread].fork_hist_bmap_ptr + 1) % N_SPEC_LEVELS;

          if (br_followed)
          /* this thread is now on the taken path */
          {
            (void)BITMAP_SET(thread_info[curr_thread].fork_hist_bmap,
                             THREADS_BMAP_SZ,
                             thread_info[curr_thread].fork_hist_bmap_ptr);
            (void)BITMAP_CLEAR(
                thread_info[forked_thread].fork_hist_bmap,
                THREADS_BMAP_SZ,
                thread_info[forked_thread].fork_hist_bmap_ptr);
            if (done_priming)
              num_fork_dispatch_timing_inaccuracies++;
            penalty = (penalize_fork_nt ? ruu_fork_penalty : 0);
          }
          else
          {
            /* this thread is now on the not-taken path */
            (void)BITMAP_CLEAR(
                thread_info[curr_thread].fork_hist_bmap,
                THREADS_BMAP_SZ,
                thread_info[curr_thread].fork_hist_bmap_ptr);
            penalty = ruu_fork_penalty + (fetch_redirected ? ruu_branch_penalty : 0);
            /* FIXME: I think 'fetch_redirected' is always 0 */
          }

          /* correct the forked threads fetchable time to reflect
		   * time spent going from IF->DA; this accounts for additional
		   * misfetch penalties */
          thread_info[forked_thread].fetchable = fetched_at + penalty;

          /* if ptracing, record the fork */
          if (ptrace_level != PTRACE_FUNSIM)
            ptrace_newthread(forked_thread, regs_PC, fork_PC);

          /* adjust forking resources: if we fork here, we
		   * consume forking resources that subsequent insts'
		   * 'could_have_forked' values don't reflect */
          if (approx_fork_in_fetch)
            for (i = 0, n = 0; n < ifq_num; i = (i + 1) % ruu_ifq_size, n++)
              if (ifq[i].could_have_forked > 0)
                ifq[i].could_have_forked--;
        }
        dassert(base_pred_PC == pred_PC);
      }
      else if (                                                                                                                  /* resources avail at fetch (if approx'ing fork-in-fetch) */
               (could_have_forked && approx_fork_in_fetch) && /*resources not now avail*/ num_active_forks >= max_threads - 1 && /* low-confidence branch or semi-omni fork-mispred */
               (conf == LowConf || (patch_PC != next_PC && !spec_mode && bconf_fork_mispred)) && /* only fork for cond br's */ (SS_OP_FLAGS(op) & F_COND))
      {
        /* would have forked under fork-in-fetch but can't now */
        if (done_priming)
          num_fork_dispatch_resource_inaccuracies++;
      }
    }

    if (done_priming)
    {
      if (!(SS_OP_FLAGS(op) & F_COND))
        dassert(!forked);

      else
      {
        SS_ADDR_TYPE cbr_idx;

        /* cbr stats */
        if (output_bconf_dist)
        {
          if (bconf_type == BCF_Ones || bconf_type == BCF_Sat ||
              bconf_type == BCF_Reset)
          {
            /* Gather some conf stats */
            if (br_taken != br_pred_taken)
              stat_add_sample(bconf_incorrect_dist, conf_data);
            else
              stat_add_sample(bconf_correct_dist, conf_data);
          }
        }

        if (output_cbr_dist)
        {
          cbr_idx = regs_PC & (~0x7);

          /* times executed is at index 'cbr_idx' */
          stat_add_sample(cbr_data_dist, cbr_idx);

          /* times mispredicted is at index 'cbr_idx + 1' */
          if (br_taken != br_pred_taken)
            stat_add_sample(cbr_data_dist, cbr_idx + 1);

          /* times conf pred is wrong is at index 'cbr_idx + 2' */
          if ((br_taken != br_pred_taken && conf == HighConf) ||
              (br_taken == br_pred_taken && conf == LowConf))
            stat_add_sample(cbr_data_dist, cbr_idx + 2);
        }

        if (output_cbr_hist)
          fprintf(cbr_hist_file, "%u %d\n", regs_PC, br_taken);

        if (output_cbr_acc)
          fprintf(cbr_acc_file, "%u %d\n", regs_PC,
                  br_taken == br_pred_taken);

        /* forking stats */
        if (forked)
        {
          total_forked++;

          dassert(SS_OP_FLAGS(op) & F_COND);
          if (would_be_taken != br_taken)
            total_forked_good++;
          else
            total_forked_wasted++;
        }
        else
        {
          assert(would_be_taken == br_pred_taken);
          if (!pred_perfect)
            dassert(base_pred_PC == pred_PC);

          if (br_taken != br_pred_taken && !pred_perfect)
          {
            total_unforked_bad++;
            if (conf == HighConf)
              total_unforked_bad_hi++;
            dassert(total_unforked_bad_hi <= total_unforked_bad);
          }
          else
          {
            total_unforked_good++;
            if (conf == LowConf)
              total_unforked_good_lo++;
            dassert(total_unforked_good_lo <= total_unforked_good);
          }
        }
      }
    }

    /* free stack_copy */
    if (retstack_copy_rec.contents.stack_copy)
    {
      free(retstack_copy_rec.contents.stack_copy);
      retstack_copy_rec.contents.stack_copy = NULL;
    }

    /* allocate RUU entry */
    rs = &RUU[RUU_tail];
    RUU_old_tail = RUU_tail;
    RUU_tail = (RUU_tail + 1) % RUU_size;
    RUU_num++;

    /* record some forking info */
    rs->used_PC = patch_PC;
    rs->forked = forked;
    rs->forked_thread = forked_thread;
    rs->br_taken = br_taken;
    rs->br_pred_taken = would_be_taken;
    rs->fork_hist_bmap_ptr = fork_hist_bmap_ptr;
    BITMAP_COPY(rs->fork_hist_bmap, fork_hist_bmap, THREADS_BMAP_SZ);

    if (br_pred_taken)
    {
      rs->taken_thread = curr_thread;
      rs->nottaken_thread = forked_thread;
      dassert(!fork_in_fetch);
    }
    else
    {
      rs->taken_thread = forked_thread;
      rs->nottaken_thread = curr_thread;
    }

    if (patch_PC == next_PC)
      rs->correct_thread = curr_thread;
    else if (forked)
      rs->correct_thread = forked_thread;
    else
      rs->correct_thread = curr_thread;

    /* if fork-pruning, pass token */
    if ((fork_prune || fetch_pred_pri) && forked)
    {
      if (thread_info[curr_thread].pred_path_token)
        rs->pred_path_token = TRUE;

      if (!fork_in_fetch)
      {
        assert(thread_info[curr_thread].pred_path_token || !fork_prune);
        thread_info[curr_thread].pred_path_token = TRUE;
        rs->token2forked = FALSE;
      }
#if 0
/* fork-in-fetch and pruning not supported */
	  else
	    {
	      if (would_be_taken && thread_info[curr_thread].pred_path_token)
		{
		  thread_info[forked_thread].pred_path_token = TRUE;
		  thread_info[curr_thread].pred_path_token = FALSE;
		  rs->token2forked = TRUE;
		}
	      else if (!would_be_taken)
		{
		  thread_info[curr_thread].pred_path_token = TRUE;
		  rs->token2forked = FALSE;
		}
	    }
#endif
    }
    else if ((fetch_pri_pol == Pred_RR || fetch_pri_pol == Pred_Pri2) && forked && pred_thread == curr_thread)
    {
#ifdef DEBUG_PRED_PRI
      assert(thread_info[curr_thread].new_pred_path_token == new_pred_path_token_val);
#endif
      rs->new_pred_path_token = TRUE;

      if (!fork_in_fetch)
      {
        /* then this thread is by definition on the predicted path */
#ifdef DEBUG_PRED_PRI
        thread_info[curr_thread].new_pred_path_token = ++new_pred_path_token_val;
#endif
        pred_thread = curr_thread;
        rs->pred_thread = curr_thread;
      }
      else
      {
        if (would_be_taken)
        {
#ifdef DEBUG_PRED_PRI
          thread_info[forked_thread].new_pred_path_token = ++new_pred_path_token_val;
#endif
          pred_thread = forked_thread;
          rs->pred_thread = forked_thread;
        }
        else
        {
#ifdef DEBUG_PRED_PRI
          thread_info[curr_thread].new_pred_path_token = ++new_pred_path_token_val;
#endif
          pred_thread = curr_thread;
          rs->pred_thread = curr_thread;
        }
      }
    }
    else if (forked)
    {
      if (!fork_in_fetch)
        /* then this thread is by definition on the predicted path */
        rs->pred_thread = curr_thread;
      else if (would_be_taken)
        rs->pred_thread = forked_thread;
      else
        rs->pred_thread = curr_thread;

      rs->new_pred_path_token = FALSE;
    }
    else /* else not on pred path or not tracking it */
    {
      if ((SS_OP_FLAGS(op) & F_CTRL) && pred_thread == curr_thread)
        rs->new_pred_path_token = TRUE;
      else
        rs->new_pred_path_token = FALSE;

      rs->pred_path_token = FALSE;
      rs->token2forked = -1;
      rs->pred_thread = -1;
    }

    /* one more instruction executed, speculative or otherwise */
    sim_total_insn++;
    if (SS_OP_FLAGS(op) & F_CTRL)
    {
      sim_total_branches++;

      if (done_priming && (SS_OP_FLAGS(op) & F_COND))
      {
        sim_total_cond_branches++;
        if (could_have_forked && done_priming)
          total_fork_opps++;
        dassert(total_fork_opps <= sim_total_cond_branches);
      }
    }
    else
      assert(!forked);

    /* entered decode/allocate stage, indicate in pipe trace, and 
       * record operand and result info.  Only do this here if
       * PTRACE_FUNSIM, else do it at the bottom of this function */
    if (ptrace_level == PTRACE_FUNSIM)
      if (!spec_mode)
      {
        ptrace_newinst(pseq, inst, regs_PC, curr_thread, addr);
        ptrace_newstage_verbose(pseq, curr_thread, PST_DISPATCH,
                                ((pred_PC != next_PC) ? PEV_MPOCCURED : 0) | (spec_mode ? PEV_SPEC_MODE : 0),
                                op, in1, in2, in3, in_vals,
                                out1, out2, out_vals);
      }

    /* must do this early */
    rs->spec_mode = spec_mode;
    rs->spec_level = spec_level;

    /* set misspeculation mode as appropriate */
    if (spec_mode != TRUE)
    {
      /* one more non-speculative instruction executed */
      sim_num_insn++;

      /* record RUU depth at which inst was inserted */
      if (report_decode_loc && done_priming)
      {
        decode_depth = RUU_tail - RUU_head;
        if (decode_depth < 0)
          decode_depth = RUU_size + decode_depth;
        stat_add_sample(decode_loc_dist, decode_depth);
      }

      if (SS_OP_FLAGS(op) & F_CTRL)
      {
        sim_num_branches++;
        if (done_priming && (SS_OP_FLAGS(op) & F_COND))
        {
          sim_num_cond_branches++;
          if (could_have_forked && done_priming)
            num_fork_opps++;
          dassert(num_fork_opps <= sim_num_cond_branches);
        }

        if (done_priming)
        {
          if (forked)
          {
            num_forked++;

            if (would_be_taken != br_taken)
              num_forked_good++;
            else
              num_forked_wasted++;
          }
          else if (SS_OP_FLAGS(op) & F_COND)
          {
            if (br_taken != br_pred_taken && !pred_perfect)
            {
              num_unforked_bad++;
              if (conf == HighConf)
                num_unforked_bad_hi++;
              dassert(num_unforked_bad_hi <= num_unforked_bad);
            }
            else
            {
              num_unforked_good++;
              if (conf == LowConf)
                num_unforked_good_lo++;
              dassert(num_unforked_bad_hi <= num_unforked_bad);
            }
          }
        }

        if (pred && !bpred_spec_update && bpred_perf_update)
          /* To simulate even more accuracte (but not boosted) bpred,
		 * do "perfect" update: update immediately and only with
		 * correct, non-speculative info */
          bpred_update(pred, regs_PC, next_PC, OFS,
                       /* taken? */ next_PC != (regs_PC +
                                                sizeof(SS_INST_TYPE)),
                       /* pred taken? */ base_pred_PC != (regs_PC +
                                                          sizeof(SS_INST_TYPE)),
                       /* correct pred? */ base_pred_PC == next_PC,
                       /* opcode */ op, (RS) == 31,
                       /* gate */ TRUE,
                       /* hybrid component */ FALSE,
                       /* dir predictor update pointer */ b_update_rec,
                       /* retstack fixup rec */ &bpred_recover_rec);

        if (bconf && !bconf_spec_update && bconf_perf_update &&
            (SS_OP_FLAGS(rs->op) & F_COND))
        {
          int br_taken = (next_PC != (regs_PC + sizeof(SS_INST_TYPE)));
          int br_pred_taken = (base_pred_PC != (regs_PC + sizeof(SS_INST_TYPE)));

          bconf_update(bconf, regs_PC,
                       /* branch taken? */ br_taken,
                       /* correct pred? */ br_pred_taken == br_taken,
                       /* conf pred?    */ conf);
        }

#ifdef RETSTACK_DEBUG_PRINTOUT
        if (op == JAL || (op == JALR && ((RS) == 31)))
          fprintf(stderr, "CALL: 0x%x (t%d, %d), return address is 0x%x\n",
                  regs_PC, curr_thread, pseq, regs_PC + SS_INST_SIZE);
        else if (op == JR && ((RS) == 31))
          fprintf(stderr, "RETURN: 0x%x (t%d, %d), return address is 0x%x\n",
                  regs_PC, curr_thread, pseq, next_PC);
#endif

        /* is the trace generator trasitioning into mis-speculation mode?
	       * But it's only misspeculation if we haven't already caught
	       * the mistake, cleaned up, and redirected fetch */
        if (patch_PC != next_PC)
        {
          /* entering mis-speculation mode; forked path is correct */
          spec_mode = TRUE;
          dassert(spec_level == 0);
          spec_level = 1;
          thread_info[curr_thread].spec_mode = TRUE;
          thread_info[curr_thread].spec_level = 1;

          /* this thread needs its own copy of the regs */
          memcpy(spec_regs_R[curr_thread][1], regs_R,
                 SS_NUM_REGS * sizeof(SS_WORD_TYPE));
          memcpy(&spec_regs_F[curr_thread][1], &regs_F,
                 sizeof(union regs_FP));
          spec_regs_HI[curr_thread][1] = regs_HI;
          spec_regs_LO[curr_thread][1] = regs_LO;
          spec_regs_FCC[curr_thread][1] = regs_FCC;

          /* ...and of the create vectors */
          memcpy(spec_create_vector[curr_thread][1], create_vector,
                 SS_TOTAL_REGS * sizeof(struct CV_link));
          memcpy(spec_create_vector_rt[curr_thread][1],
                 create_vector_rt, SS_TOTAL_REGS * sizeof(SS_TIME_TYPE));

          /* adjust priorities */
          if ((fetch_pri_pol == Omni_Pri || fetch_pri_pol == Two_Omni_Pri) && forked)
          {
            thread_info[curr_thread].base_priority = 1;
            thread_info[curr_thread].priority = 1;
          }

          if (forked)
          {
            thread_info[forked_thread].spec_mode = FALSE;
            thread_info[forked_thread].spec_level = 0;
            rs->recover_inst = FALSE;
            assert(thread_info[forked_thread].valid == TRUE);

            /* adjust priorities */
            if (fetch_pri_pol == Omni_Pri)
              thread_info[forked_thread].priority = thread_info[forked_thread].base_priority = sum_priorities();
            else if (fetch_pri_pol == Two_Omni_Pri)
              thread_info[forked_thread].priority = thread_info[forked_thread].base_priority = two_pri_lev;
          }
          else
            rs->recover_inst = TRUE;
        }
        else
        {
          /* this path is correct; forked path is mis-speculated */
          rs->recover_inst = FALSE;
          assert(thread_info[curr_thread].spec_mode == FALSE);

          /* adjust priorities */
          if (fetch_pri_pol == Omni_Pri)
            thread_info[curr_thread].priority = thread_info[curr_thread].base_priority = sum_priorities();
          else if (fetch_pri_pol == Two_Omni_Pri)
            thread_info[curr_thread].priority = thread_info[curr_thread].base_priority = two_pri_lev;

          if (forked)
          {
            thread_info[forked_thread].spec_mode = TRUE;
            thread_info[forked_thread].spec_level = 1;
            assert(thread_info[forked_thread].valid == TRUE);

            /* adjust priorities */
            if (fetch_pri_pol == Omni_Pri || fetch_pri_pol == Two_Omni_Pri)
            {
              thread_info[forked_thread].base_priority = 1;
              thread_info[forked_thread].priority = 1;
            }

            /* forked thread needs its own copy of the regs */
            memcpy(spec_regs_R[forked_thread][1], regs_R,
                   SS_NUM_REGS * sizeof(SS_WORD_TYPE));
            memcpy(&spec_regs_F[forked_thread][1], &regs_F,
                   sizeof(union regs_FP));
            spec_regs_HI[forked_thread][1] = regs_HI;
            spec_regs_LO[forked_thread][1] = regs_LO;
            spec_regs_FCC[forked_thread][1] = regs_FCC;

            /* ...and of the create vectors */
            memcpy(spec_create_vector[forked_thread][1],
                   create_vector,
                   SS_TOTAL_REGS * sizeof(struct CV_link));
            memcpy(spec_create_vector_rt[forked_thread][1],
                   create_vector_rt,
                   SS_TOTAL_REGS * sizeof(SS_TIME_TYPE));
          }
        }
      }
      else
        rs->recover_inst = FALSE;
    }
    else if (patch_PC != next_PC)
    {
      int old_spec_level;

      /* already in misspeculation mode, but we've mispredicted again; 
	   * forked path stays at "old" level of speculation */
      dassert(SS_OP_FLAGS(op) & F_CTRL);
#if 0
	  dassert(fetch_pri_pol != Omni_Pri 
		  || thread_info[curr_thread].base_priority == 1 );
#endif

      if (forked)
      {
        thread_info[forked_thread].spec_mode = TRUE;
        thread_info[forked_thread].spec_level = spec_level;
        rs->recover_inst = FALSE;
        assert(thread_info[forked_thread].valid == TRUE);
#if 0
	      dassert(fetch_pri_pol != Omni_Pri 
		      || thread_info[forked_thread].base_priority == 1);
#endif

        /* this thread needs its own copy of the regs */
        memcpy(spec_regs_R[forked_thread][spec_level],
               spec_regs_R[curr_thread][spec_level],
               SS_NUM_REGS * sizeof(SS_WORD_TYPE));
        memcpy(&spec_regs_F[forked_thread][spec_level],
               &spec_regs_F[curr_thread][spec_level],
               sizeof(union regs_FP));
        spec_regs_HI[forked_thread][spec_level] = spec_regs_HI[curr_thread][spec_level];
        spec_regs_LO[forked_thread][spec_level] = spec_regs_LO[curr_thread][spec_level];
        spec_regs_FCC[forked_thread][spec_level] = spec_regs_FCC[curr_thread][spec_level];

        /* ... and of the create vectors */
        memcpy(spec_create_vector[forked_thread][spec_level],
               spec_create_vector[curr_thread][spec_level],
               SS_TOTAL_REGS * sizeof(struct CV_link));
        memcpy(spec_create_vector_rt[forked_thread][spec_level],
               spec_create_vector_rt[curr_thread][spec_level],
               SS_TOTAL_REGS * sizeof(SS_TIME_TYPE));
      }
#ifndef BUG_COMPAT_RECOVER_INST
      else
        rs->recover_inst = TRUE;
#endif

      old_spec_level = spec_level;
      spec_level++;
      thread_info[curr_thread].spec_level++;
      if (spec_level > N_SPEC_LEVELS || spec_level < 0)
        panic("speculated past too many branches; change N_SPEC_LEVELS");
      if (spec_level > max_spec_level)
        max_spec_level = spec_level;

      /* and the current thread needs a new copy of the registers
	   * corresponding to the new spec level */
      memcpy(spec_regs_R[curr_thread][spec_level],
             spec_regs_R[curr_thread][old_spec_level],
             SS_NUM_REGS * sizeof(SS_WORD_TYPE));
      memcpy(&spec_regs_F[curr_thread][spec_level],
             &spec_regs_F[curr_thread][old_spec_level],
             sizeof(union regs_FP));
      spec_regs_HI[curr_thread][spec_level] = spec_regs_HI[curr_thread][old_spec_level];
      spec_regs_LO[curr_thread][spec_level] = spec_regs_LO[curr_thread][old_spec_level];
      spec_regs_FCC[curr_thread][spec_level] = spec_regs_FCC[curr_thread][old_spec_level];

      /* ... and of the create vectors */
      memcpy(spec_create_vector[curr_thread][spec_level],
             spec_create_vector[curr_thread][old_spec_level],
             SS_TOTAL_REGS * sizeof(struct CV_link));
      memcpy(spec_create_vector_rt[curr_thread][spec_level],
             spec_create_vector_rt[curr_thread][old_spec_level],
             SS_TOTAL_REGS * sizeof(SS_TIME_TYPE));

      /* ... and both threads need new spec-mem-state entries */
      for (i = 0; i < STORE_HASH_SIZE; i++)
      {
        struct spec_mem_ent *ent, *newent;

        for (ent = store_htable[i]; ent; ent = ent->next)
        {
          if (ent->thread != curr_thread || ent->spec_level != old_spec_level)
            /* this entry belongs to some other thread/epoch */
            continue;

          /* 
		   * the new spec level needs a copy of this entry,
		   * and if forked, the forked thread also needs a copy 
		   */

          /* try to get an entry from the free list, if avail */
          if (!bucket_free_list)
          {
            /* otherwise, call calloc() to get more storage */
            bucket_free_list = calloc(1,
                                      sizeof(struct spec_mem_ent));
            if (!bucket_free_list)
              fatal("out of virtual memory");
          }
          newent = bucket_free_list;
          bucket_free_list = bucket_free_list->next;

          newent->next = store_htable[i];
          store_htable[i] = newent;
          newent->addr = ent->addr;
          newent->thread = curr_thread;
          newent->spec_level = spec_level;
          newent->data[0] = ent->data[0];
          newent->data[1] = ent->data[1];

          if (forked)
          {
            /* try to get an entry from the free list, if avail */
            if (!bucket_free_list)
            {
              /* otherwise, call calloc() to get more storage */
              bucket_free_list = calloc(1,
                                        sizeof(struct spec_mem_ent));
              if (!bucket_free_list)
                fatal("out of virtual memory");
            }
            newent = bucket_free_list;
            bucket_free_list = bucket_free_list->next;

            newent->next = store_htable[i];
            store_htable[i] = newent;
            newent->addr = ent->addr;
            newent->thread = forked_thread;
            newent->spec_level = old_spec_level;
            newent->data[0] = ent->data[0];
            newent->data[1] = ent->data[1];
          }
        }
      }
    }
    else
    {
      int new_spec_level = spec_level + 1;
      if (new_spec_level > N_SPEC_LEVELS || new_spec_level < 0)
        panic("speculated past too many branches; change N_SPEC_LEVELS");
      if (spec_level > max_spec_level)
        max_spec_level = spec_level;

      /* already in misspeculation mode, but this branch was predicted
	   * correctly; forked path adds another level of speculation.
	   * Note we don't have to do any copying for the current thread */
      rs->recover_inst = FALSE;

      if (forked)
      {
        dassert(SS_OP_FLAGS(op) & F_CTRL);

        thread_info[forked_thread].spec_mode = TRUE;
        thread_info[forked_thread].spec_level = new_spec_level;
        assert(thread_info[forked_thread].valid == TRUE);
#if 0
	      dassert(fetch_pri_pol != Omni_Pri 
		      || thread_info[forked_thread].base_priority == 1);
	      dassert(fetch_pri_pol != Omni_Pri 
		      || thread_info[curr_thread].base_priority == 1);
#endif

        /* this thread needs its own copy of the regs */
        memcpy(spec_regs_R[forked_thread][new_spec_level],
               spec_regs_R[curr_thread][spec_level],
               SS_NUM_REGS * sizeof(SS_WORD_TYPE));
        memcpy(&spec_regs_F[forked_thread][new_spec_level],
               &spec_regs_F[curr_thread][spec_level],
               sizeof(union regs_FP));
        spec_regs_HI[forked_thread][new_spec_level] = spec_regs_HI[curr_thread][spec_level];
        spec_regs_LO[forked_thread][new_spec_level] = spec_regs_LO[curr_thread][spec_level];
        spec_regs_FCC[forked_thread][new_spec_level] = spec_regs_FCC[curr_thread][spec_level];

        /* ... and of the create vectors */
        memcpy(spec_create_vector[forked_thread][new_spec_level],
               spec_create_vector[curr_thread][spec_level],
               SS_TOTAL_REGS * sizeof(struct CV_link));
        memcpy(spec_create_vector_rt[forked_thread][new_spec_level],
               spec_create_vector_rt[curr_thread][spec_level],
               SS_TOTAL_REGS * sizeof(SS_TIME_TYPE));

        /* ... and of spec mem state */
        for (i = 0; i < STORE_HASH_SIZE; i++)
        {
          struct spec_mem_ent *ent, *newent;

          for (ent = store_htable[i]; ent; ent = ent->next)
          {
            if (ent->thread != curr_thread || ent->spec_level != spec_level)
              /* this entry belongs to some other thread/epoch */
              continue;

            /* 
		       * the forked thread needs a copy of this entry 
		       */

            /* try to get an entry from the free list, if avail */
            if (!bucket_free_list)
            {
              /* otherwise, call calloc() to get more storage */
              bucket_free_list = calloc(1,
                                        sizeof(struct spec_mem_ent));
              if (!bucket_free_list)
                fatal("out of virtual memory");
            }
            newent = bucket_free_list;
            bucket_free_list = bucket_free_list->next;

            newent->next = store_htable[i];
            store_htable[i] = newent;
            newent->addr = ent->addr;
            newent->thread = forked_thread;
            newent->spec_level = new_spec_level;
            newent->data[0] = ent->data[0];
            newent->data[1] = ent->data[1];
          }
        }
      }
    }

#ifdef DEBUG
    assert(spec_mode != UNKNOWN);
    assert(spec_mode == thread_info[curr_thread].spec_mode);
    assert(spec_level == thread_info[curr_thread].spec_level);
    if (forked)
      assert(thread_info[forked_thread].spec_mode != UNKNOWN);
    if (!(SS_OP_FLAGS(op) & F_CTRL))
      assert(rs->recover_inst == FALSE);
    assert(sim_num_cond_branches == num_forked_good + num_forked_wasted + num_unforked_bad + num_unforked_good);
    assert(sim_total_cond_branches == total_forked_good + total_forked_wasted + total_unforked_bad + total_unforked_good);
#endif

    /* update memory access stats */
    if (SS_OP_FLAGS(op) & F_MEM)
    {
      sim_total_refs++;
      if (!spec_mode)
        sim_num_refs++;

      if (SS_OP_FLAGS(op) & F_STORE)
        is_write = TRUE;
      else
      {
        sim_total_loads++;
        if (!spec_mode)
          sim_num_loads++;
      }
    }

    /* NOPS can be discarded in the decode stage */
    if (op != NOP)
    {
      /* for load/stores:
	     idep #0     - store operand (value that is store'ed)
	     idep #1, #2 - eff addr computation inputs (addr of access)
	     
	     resulting RUU/LSQ operation pair:
	     RUU (effective address computation operation):
	     idep #0, #1 - eff addr computation inputs (addr of access)
	     LSQ (memory access operation):
	     idep #0     - operand input (value that is store'd)
	     idep #1     - eff addr computation result (from RUU op)
	     
	     effective address computation is transfered via the reserved
	     name DTMP

	     note load/stores occupy 2 IIQ slots -- one for the addr gen,
	     one for the mem op itself
	     */

      /* insert in IIQ */
      if (SS_OP_FLAGS(op) & F_FCOMP)
        FIQ_occ++;
      else
        IIQ_occ++;

      rs->IR = inst;
      rs->op = op;
      rs->PC = regs_PC;
      rs->next_PC = next_PC;
      rs->pred_PC = pred_PC;
      rs->base_pred_PC = base_pred_PC;
      rs->in_LSQ = FALSE;
      rs->ea_comp = FALSE;
      rs->bpred_recover_rec = bpred_recover_rec;
      rs->b_update_rec = b_update_rec;
      rs->addr = 0;
      /* rs->tag is already set */
      rs->seq = ++inst_seq;
      rs->queued = rs->issued = rs->completed = FALSE;
      rs->decoded = TRUE;
      rs->ready_to_iss = sim_cycle + extra_decode_lat + 1;
      rs->issued_at = 0;
      rs->ready_time = 0;
      rs->ptrace_seq = pseq;
      rs->l1_miss = rs->flag = FALSE;
      rs->conf = conf;
      rs->squashed = FALSE;
      rs->thread_id = curr_thread;

      /* split ld/st's into two operations: eff addr comp + mem access */
      if (SS_OP_FLAGS(op) & F_MEM)
      {
        /* convert RUU operation from ld/st to an add (eff addr comp) */
        rs->op = ADD;
        rs->ea_comp = TRUE;

        /* insert in IIQ */
        IIQ_occ++;

        /* fill in LSQ reservation station */
        lsq = &LSQ[LSQ_tail];

        lsq->IR = inst;
        lsq->op = op;
        lsq->PC = regs_PC;
        lsq->next_PC = next_PC;
        lsq->pred_PC = pred_PC;
        lsq->base_pred_PC = base_pred_PC;
        lsq->in_LSQ = TRUE;
        lsq->ea_comp = FALSE;
        lsq->spec_mode = rs->spec_mode;
        lsq->spec_level = rs->spec_level;
        lsq->recover_inst = FALSE;
        lsq->addr = addr;
        /* lsq->tag is already set */
        lsq->seq = ++inst_seq;
        lsq->queued = lsq->issued = lsq->completed = FALSE;
        lsq->decoded = TRUE;
        lsq->ready_to_iss = sim_cycle + extra_decode_lat + 1;
        lsq->issued_at = 0;
        lsq->ready_time = 0;
        lsq->ptrace_seq = ptrace_seq++;
        lsq->l1_miss = lsq->flag = FALSE;
        lsq->conf = conf;
        dassert(lsq->conf == HighConf);
        lsq->squashed = FALSE;
        lsq->thread_id = curr_thread;
        dassert(!forked);
        lsq->forked = FALSE;
        lsq->fork_hist_bmap_ptr = fork_hist_bmap_ptr;
        BITMAP_COPY(lsq->fork_hist_bmap, fork_hist_bmap, THREADS_BMAP_SZ);

        /* pipetrace this uop; record mem addr and its contents (note
	       * ptracing level determines whether mem info gets printed */
        if (ptrace_level != PTRACE_FUNSIM)
        {
          ptrace_newuop(lsq->ptrace_seq, "internal ld/st", lsq->PC,
                        curr_thread, addr);
          ptrace_newstage_mem(lsq->ptrace_seq, curr_thread,
                              PST_DISPATCH,
                              (spec_mode ? PEV_SPEC_MODE : 0),
                              addr, READ_WORD((addr >> 2) << 2));
        }

        /* link eff addr computation onto operand's output chains */
        ruu_link_idep(rs, /* idep_ready[] index */ 0, NA, curr_thread);
        ruu_link_idep(rs, /* idep_ready[] index */ 1, in2, curr_thread);
        ruu_link_idep(rs, /* idep_ready[] index */ 2, in3, curr_thread);

        /* install output after inputs to prevent self reference */
        ruu_install_odep(rs, /* odep_list[] index */ 0, DTMP, curr_thread);
        ruu_install_odep(rs, /* odep_list[] index */ 1, NA, curr_thread);

        /* link memory access onto output chain of eff addr operation */
        ruu_link_idep(lsq,
                      /* idep_ready[] index */ STORE_OP_INDEX /* 0 */,
                      in1, curr_thread);
        ruu_link_idep(lsq,
                      /* idep_ready[] index */ STORE_ADDR_INDEX /* 1 */,
                      DTMP, curr_thread);
        ruu_link_idep(lsq, /* idep_ready[] index */ 2, NA, curr_thread);

        /* install output after inputs to prevent self reference */
        ruu_install_odep(lsq, /*odep_list[] index*/ 0, out1, curr_thread);
        ruu_install_odep(lsq, /*odep_list[] index*/ 1, out2, curr_thread);

        /* install operation in the LSQ */
        n_dispatched++;
        LSQ_tail = (LSQ_tail + 1) % LSQ_size;
        LSQ_num++;

        if (OPERANDS_READY(rs))
        {
          /* eff addr computation ready, queue it on ready list */
          rs->ready_time = rs->ready_to_iss;
          readyq_enqueue(rs);
        }
        /* issue may continue when the load/store is issued */
        RSLINK_INIT(last_op, lsq);

        /* issue stores only, loads are issued by lsq_refresh() */
        if (((SS_OP_FLAGS(op) & (F_MEM | F_STORE)) == (F_MEM | F_STORE)) && OPERANDS_READY(lsq))
        {
          /* panic("store immediately ready"); */
          /* put operation on ready list, ruu_issue() issue it later */
          lsq->ready_time = lsq->ready_to_iss;
          readyq_enqueue(lsq);
        }
      }
      else /* !(SS_OP_FLAGS(op) & F_MEM) */
      {
        /* link onto producing operation */
        ruu_link_idep(rs, /* idep_ready[] index */ 0, in1, curr_thread);
        ruu_link_idep(rs, /* idep_ready[] index */ 1, in2, curr_thread);
        ruu_link_idep(rs, /* idep_ready[] index */ 2, in3, curr_thread);

        /* install output after inputs to prevent self reference */
        ruu_install_odep(rs, /* odep_list[] index */ 0, out1, curr_thread);
        ruu_install_odep(rs, /* odep_list[] index */ 1, out2, curr_thread);

        /* we've finished installing this inst in the RUU */
        n_dispatched++;

        /* issue op if all its reg operands are ready (no mem input) */
        if (OPERANDS_READY(rs))
        {
          /* put operation on ready list, ruu_issue() issue it later */
          rs->ready_time = rs->ready_to_iss;
          readyq_enqueue(rs);
          /* issue may continue */
          last_op = RSLINK_NULL;
        }
        else
        {
          /* could not issue this inst, stall issue until we can */
          RSLINK_INIT(last_op, rs);
        }
      }
    }
    else
    {
      /* this is a NOP, no need to update RUU/LSQ state; deallocate 
	   * the RUU entry we allocated above */
      rs = NULL;
      RUU_tail = RUU_old_tail;
      RUU_num--;
    }

    /* entered decode/allocate stage, indicate in pipe trace, and 
       * record operand and result info.  Ptracing level determines whether 
       * verbose information actually gets printed */
    if (ptrace_level != PTRACE_FUNSIM)
      ptrace_newstage_verbose(pseq, curr_thread, PST_DISPATCH,
                              /* events */
                              (fetch_redirected ? PEV_MFOCCURED : 0) | ((!forked && (patch_PC != next_PC)) ? PEV_MPOCCURED : 0) | (spec_mode ? PEV_SPEC_MODE : 0),
                              /* operand info for verbose mode */
                              op, in1, in2, in3, in_vals,
                              out1, out2, out_vals);

    if (op == NOP)
    {
      /* end of the line */
      if (ptrace_level != PTRACE_FUNSIM)
        ptrace_endinst(pseq, curr_thread);
    }

    /* update any stats tracked by PC */
    for (i = 0; i < pcstat_nelt; i++)
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

  empty_fetch_entry:
    /* consume (possibly empty) instruction from IFETCH -> DISPATCH queue */
    ifq[ifq_head].valid = FALSE;
    ifq_head = (ifq_head + 1) & (ruu_ifq_size - 1);
    ifq_num--;

#ifdef USE_DLITE
    /* check for DLite debugger entry condition */
    made_check = TRUE;
    if (dlite_check_break(pred_PC,
                          is_write ? ACCESS_WRITE : ACCESS_READ,
                          addr, sim_num_insn, sim_cycle))
      dlite_main(regs_PC, pred_PC, sim_cycle);
#endif

    set_regs_PC(regs_PC); /* set top-level regs_PC */
#ifdef DEBUG
    /* at most one thread at a time should be in non-spec mode
       * (can be zero, if we don't fork on a branch and end up mispredicting,
       * or if all threads in SMT are in mis-spec mode */
    for (j = 0, n_non_spec = 0; j < N_THREAD_RECS; j++)
    {
      if (thread_info[j].valid && thread_info[j].spec_mode == FALSE)
        n_non_spec++;
      if (n_non_spec > 1)
        panic("More than one non-speculative path!");
    }
#endif
  }

#ifdef USE_DLITE
  /* need to enter DLite at least once per cycle */
  if (!made_check)
  {
    if (dlite_check_break(/* no next PC */ 0,
                          is_write ? ACCESS_WRITE : ACCESS_READ,
                          addr, sim_num_insn, sim_cycle))
      dlite_main(regs_PC, /* no next PC */ 0, sim_cycle);
  }
#endif
}

/*
 *  RUU_FETCH() - instruction fetch pipeline stage(s)
 */

/* initialize the instruction fetch pipeline stage */
static void
fetch_init(void)
{
  /* allocate the IFETCH -> DISPATCH instruction queue */
  ifq =
      (struct fetch_rec *)calloc(ruu_ifq_size, sizeof(struct fetch_rec));
  if (!ifq)
    fatal("out of virtual memory");

  ifq_num = 0;
  ifq_tail = ifq_head = 0;
}

/* dump contents of fetch stage registers and fetch queue */
void fetch_dump(FILE *stream) /* output stream */
{
  int num, head, i;

  fprintf(stream, "** fetch stage state ** current cycle: %.0f **\n",
          (double)sim_cycle);
  if (fetch_pri_pol != Simple_RR && fetch_pri_pol != Old_RR && fetch_pri_pol != Pred_RR && fetch_pri_pol != Pred_Pri2)
    fprintf(stream, "       priority sum = %d\n", sum_priorities());
  else
    fprintf(stream, "       priority sum = %s\n", "na");
  fprintf(stream, "\n");

  for (i = 0; i < N_THREAD_RECS; i++)
  {
    if (thread_info[i].valid)
    {
      fprintf(stream, "thread id: %d\n", i);
      fprintf(stream, "        fetch_regs_PC: 0x%08x, "
                      "fetch_pred_PC: 0x%08x, recover_PC: 0x%08x\n",
              thread_info[i].fetch_regs_PC, thread_info[i].fetch_pred_PC,
              0);
      fprintf(stream, "        spec_mode: %s, spec_level: %d\n",
              thread_info[i].spec_mode ? "t" : "f",
              thread_info[i].spec_level);
      fprintf(stream, "        when fetchable: %.0f\n",
              (double)thread_info[i].fetchable);
      fprintf(stream, "        priority: %d, base_priority: %d\n",
              thread_info[i].priority, thread_info[i].base_priority);
      fprintf(stream,
              "        last_inst_missed: %s, last_inst_tmissed: %s\n",
              thread_info[i].last_inst_missed ? "t" : "f",
              thread_info[i].last_inst_tmissed ? "t" : "f");
      fprintf(stream, "        fork_hist_bmap: 0x");
      BITMAP_PRINT_BITSTR(thread_info[i].fork_hist_bmap, THREADS_BMAP_SZ, stream);
      fprintf(stream, "\n        fork_hist_bmap_ptr: %d\n",
              thread_info[i].fork_hist_bmap_ptr);
    }
  }
  fprintf(stream, "\n");

  fprintf(stream, "** fetch queue contents **\n");
  fprintf(stream, "ifq_num: %d\n", ifq_num);
  fprintf(stream, "ifq_head: %d, ifq_tail: %d\n",
          ifq_head, ifq_tail);

  num = ifq_num;
  head = ifq_head;
  while (num)
  {
    fprintf(stream, "idx: %2d: inst: `", head);
    fprintf(stream, "valid: %d\n", ifq[head].valid);
    ss_print_insn(ifq[head].IR, ifq[head].regs_PC, stream);
    fprintf(stream, "'\n");
    fprintf(stream, "         thread_id: 0x%04x\n", ifq[head].thread);
    fprintf(stream, "         fork_hist_bmap: 0x");
    BITMAP_PRINT_BITSTR(ifq[head].fork_hist_bmap, THREADS_BMAP_SZ, stream);
    fprintf(stream, "\n         fork_hist_bmap_ptr: %d\n",
            ifq[head].fork_hist_bmap_ptr);
    fprintf(stream, "         conf: %s, forked: %s\n",
            ((ifq[head].conf == HighConf) ? "hi" : "lo"),
            (ifq[head].forked ? "t" : "f"));
    fprintf(stream,
            "         regs_PC: 0x%08x, pred_PC: 0x%08x, base_pred_PC: 0x%08x\n",
            ifq[head].regs_PC, ifq[head].pred_PC,
            ifq[head].base_pred_PC);
    head = (head + 1) & (ruu_ifq_size - 1);
    num--;
  }
}

/* convert 64-bit inst text addresses to 32-bit inst equivalents */
#define IACOMPRESS(A) \
  (compress_icache_addrs ? ((((A)-SS_TEXT_BASE) >> 1) + SS_TEXT_BASE) : (A))
#define ISCOMPRESS(SZ) \
  (compress_icache_addrs ? ((SZ) >> 1) : (SZ))

int mylog2(int x)
{
  int n = 0;

  while (x / 2 > 0)
  {
    x /= 2;
    n++;
  }

  return n;
}

static SS_ADDR_TYPE
hydra_bpred_lookup(int thread,
                   SS_ADDR_TYPE fetch_regs_PC,                    /* PC of inst being predicted */
                   SS_INST_TYPE inst,                             /* raw bits for inst being predicted */
                   struct bpred_update_info *b_update_recp,       /* ptr to info f/ update*/
                   struct bpred_recover_info *bpred_recover_recp) /* retstack 
							     * recovery info */
{
  SS_ADDR_TYPE fetch_pred_PC;
  int gate = TRUE;
  int tosp = -1;
  enum ss_opcode op = SS_OPCODE(inst);
  int jr_r31p = ((RS) == 31);
  int jalr_r31p = ((RD) == 31);
  int pred_taken = TRUE;
  SS_ADDR_TYPE btarget;

  if (SS_OP_FLAGS(op) & F_COND)
    btarget = fetch_regs_PC + 8 + (OFS << 2);

  /* if keeping per-thread retstacks, push ret-addr if this
   * is a function call */
  if (per_thread_retstack == PerThreadStacks && /* call? */ (op == JAL || (op == JALR && jalr_r31p)))
  {
    dassert(retstack_size && thread_info[thread].retstack && thread_info[thread].retstack_tos >= 0 && thread_info[thread].retstack_tos < retstack_size);

    thread_info[thread].retstack_tos = (thread_info[thread].retstack_tos + 1) % retstack_size;
    thread_info[thread].retstack[thread_info[thread].retstack_tos].target = fetch_regs_PC + sizeof(SS_INST_TYPE);
  }

  /* if this is a return (JR through r31), pop the 
   * per-thread ret-addr stack if appropriate; else call
   * bpred_lookup() */

  if (per_thread_retstack == OneStackPredOnly && (fetch_pri_pol == Pred_RR || fetch_pri_pol == Pred_Pri2) && thread != pred_thread)
    gate = FALSE;

  if (per_thread_retstack == PerThreadTOSP)
    tosp = thread_info[thread].retstack_tos;

  fetch_pred_PC = bpred_lookup(pred, fetch_regs_PC, btarget, 0, 0,
                               op, jr_r31p, jalr_r31p, &tosp, gate,
                               b_update_recp,
                               bpred_recover_recp);
  if (pred->retstack.patch_level == RETSTACK_PATCH_WHOLE)
    assert(bpred_recover_recp->contents.stack_copy);

  if (per_thread_retstack == PerThreadTOSP)
  {
    assert(tosp >= 0);
    thread_info[thread].retstack_tos = tosp;
  }

  if (per_thread_retstack == PerThreadStacks && op == JR && jr_r31p)
  {
#ifdef RETSTACK_DEBUG_PRINTOUT
    int i, n;
    fprintf(stderr, "Predicting jr31 0x%x, (t%d, %d)\n",
            fetch_regs_PC, thread, ptrace_seq);
    for (i = thread_info[thread].retstack_tos, n = 0;
         n < retstack_size;
         n++, i = (i + retstack_size - 1) % retstack_size)
      fprintf(stderr, "\t%d: 0x%x\n",
              i, thread_info[thread].retstack[i].target);
#endif
    dassert(retstack_size && thread_info[thread].retstack && thread_info[thread].retstack_tos >= 0 && thread_info[thread].retstack_tos < retstack_size);

    fetch_pred_PC = thread_info[thread].retstack[thread_info[thread].retstack_tos].target;
    thread_info[thread].retstack_tos = (thread_info[thread].retstack_tos + retstack_size - 1) % retstack_size;
  }

  /* if per-thread retstacks, record per-thread stack_recover_idx
   * instead of what bpred_lookup() gave us */
  if (per_thread_retstack == PerThreadStacks)
  {
    bpred_recover_recp->tos = thread_info[thread].retstack_tos;
    if (pred->retstack.patch_level == RETSTACK_PATCH_PTR_DATA)
      bpred_recover_recp->contents.tos_value = thread_info[thread].retstack[bpred_recover_recp->tos].target;
    else if (pred->retstack.patch_level == RETSTACK_PATCH_WHOLE)
    {
      assert(bpred_recover_recp->contents.stack_copy);
      memcpy(bpred_recover_recp->contents.stack_copy,
             thread_info[thread].retstack,
             retstack_size * sizeof(struct bpred_btb_ent));
    }
  }

  if (fetch_pred_PC == 0)
    pred_taken = FALSE;
  if (fetch_pred_PC == fetch_regs_PC + SS_INST_SIZE)
    pred_taken = FALSE;

  bpred_history_update(pred, fetch_regs_PC, pred_taken, op, TRUE,
                       b_update_recp, bpred_recover_recp);
  return fetch_pred_PC;
}

/* fetch as many instruction as one branch prediction and one cache line
   access will support without overflowing the IFQ.  Return TRUE if a
   blocking condition (either a pred-taken branch or a miss) was encountered */
static int
ruu_fetch(int thread,         /* thread from which to fetch a line */
          int *p_num_fetched) /* # insts fetched */
{
  int i, curr, lat, tlb_lat, num_fetched = 0, done = FALSE;
  SS_INST_TYPE inst;
  SS_ADDR_TYPE cache_line, base_pred_PC;
  struct bpred_update_info b_update_rec;
  struct bpred_recover_info bpred_recover_rec;
  struct bpred_recover_info retstack_copy_rec;
  int in_flight_branches = 0;
  int could_have_forked = FALSE;
  int conf = HighConf;
  int conf_data;
  int forked = FALSE;
  enum ss_opcode op;
  int new_thread = -2;

  /* get per-thread fetch state */
  SS_ADDR_TYPE fetch_regs_PC = thread_info[thread].fetch_regs_PC;
  SS_ADDR_TYPE fetch_pred_PC = thread_info[thread].fetch_pred_PC;
  int last_inst_missed = thread_info[thread].last_inst_missed;
  int last_inst_tmissed = thread_info[thread].last_inst_tmissed;

  /* Sanity checks */
  assert(thread_info[thread].valid == TRUE && thread_info[thread].squashed == UNKNOWN);
  assert(thread_info[thread].fetchable <= sim_cycle);
  assert(ifq_num < ruu_ifq_size); /* can't fetch if ifq is full */

  /* If il1 and dl1 are unified, we can only fetch if ports are available */
  /* FIXME: kind of obsolete with the addition of multi-path */
  if (cache_il1 == cache_dl1 && cache_dl1_ports_used >= cache_dl1_ports)
  {
    /* No port available; wait a cycle; fetch gets priority next cycle */
    cache_dl1_ports_reserved++;
    panic("failed fetch");
    goto failed_fetch;
  }
  else if (cache_il1 == cache_dl1)
    cache_dl1_ports_used++;

  /* If the last access was only a cache miss, the data should be
   * passed directly to the ifetch buffer, so we don't do another
   * access.  If a TLB miss happened last time, then we have to
   * start the access over. */
  if (!done && (!last_inst_missed || last_inst_tmissed))
  {
    /* We only fetch one cache line at a time.  Determine if it's a
       * cache/tlb hit/miss. If it's a miss, stall fetch for a while. 
       * Note that when we start fetching, fetch_regs_PC <= fetch_pred_PC,
       * so here we use fetch_pred_PC. */
    if (ld_text_base <= fetch_pred_PC && fetch_pred_PC < (ld_text_base + ld_text_size) && !(fetch_pred_PC & (sizeof(SS_INST_TYPE) - 1)))
    {
      /* address is within program text, read instruction from memory */
      lat = cache_il1_lat[0] + cache_il1_lat[1];
      if (cache_il1)
      {
        /* access the I-cache */
        if (cache_il1_perfect)
          lat = 1;
        else
          lat =
              cache_access(cache_il1, Read, IACOMPRESS(fetch_pred_PC),
                           NULL, ISCOMPRESS(sizeof(SS_INST_TYPE)),
                           sim_cycle, NULL, NULL);

        if (lat > cache_il1_lat[0] + cache_il1_lat[1])
        {
          if (report_miss_clustering)
          {
            int miss_dist = sim_cycle - last_imiss_cycle;
            assert(miss_dist >= 0);
            last_imiss_cycle = sim_cycle;
            stat_add_sample(i1miss_cluster_dist, miss_dist);
          }
          if (report_imiss_ruu_occ)
            stat_add_sample(imiss_ruu_occ_dist, RUU_num);

          last_inst_missed = TRUE;
        }
      }
      else if (il1_access_mem)
      {
        /* no I-caches, but get accurate mem access behavior */
        lat = il2_access_fn(Read, IACOMPRESS(fetch_pred_PC),
                            ISCOMPRESS(sizeof(SS_INST_TYPE)),
                            NULL, sim_cycle);
      }
      if (itlb)
      {
        /* access the I-TLB, NOTE: this code will initiate
		 speculative TLB misses */
        if (tlb_perfect)
          tlb_lat = 1;
        else
          tlb_lat =
              cache_access(itlb, Read, IACOMPRESS(fetch_pred_PC),
                           NULL, ISCOMPRESS(sizeof(SS_INST_TYPE)),
                           sim_cycle, NULL, NULL);
        if (tlb_perfect)
          tlb_lat = 1;

        if (tlb_lat > 1)
          last_inst_tmissed = TRUE;

        /* I-cache/I-TLB accesses occur in parallel */
        lat = MAX(tlb_lat, lat);
      }
      /* I-cache/I-TLB miss? assumes I-cache hit >= I-TLB hit */
      if (lat > cache_il1_lat[0] + cache_il1_lat[1])
      {
        /* I-cache miss, block fetch until it is resolved */
        thread_set_fetch_penalty(thread, lat);
        done = TRUE;
        goto failed_fetch;

        /* When we return to ruu_fetch, last_inst_{,t}missed, set above,
	       * will indicate what kind of miss we encountered so that
	       * ptrace, below, shows the miss. */
      }
      /* else, I-cache/I-TLB hit or "fake hit" due to perfectness */
    }
  }
  else if (done)
    assert(FALSE);
  /* FIXME: some kind of assert on thread id in the 'else' would be good... */

  /* Once we reach here, we've successfully fetched an I-cache line
   * and processed any associated misses.  Each instruction gets
   * fetched from fetch_regs_PC. 
   */

  /* Note which cache_line the first inst-address belongs to.  Stop
   * fetching when we move to a new cache line or we take a branch. */
  cache_line = IACOMPRESS(fetch_pred_PC) >> cache_il1_blkshift;

  if (ifq_num >= ruu_ifq_size - 1)
    ifq_overflows++;

  /* count how many branches are in-flight (don't bother if we're not
   * imposing a limit) */
  if (max_in_flight_branches)
  {
    for (in_flight_branches = 0, i = 0, curr = RUU_head;
         i < RUU_num;
         i++, curr = (curr + 1) % RUU_size)
    {
      if ((SS_OP_FLAGS(RUU[curr].op) & F_CTRL) && RUU[curr].thread_id == thread) /*  && !RUU[curr].completed) */
        in_flight_branches++;
    }
    for (i = 0, curr = ifq_head;
         i < ifq_num;
         i++, curr = (curr + 1) % ruu_ifq_size)
    {
      if ((SS_OP_FLAGS(SS_OPCODE(ifq[curr].IR)) & F_CTRL) && ifq[curr].thread == thread && ifq[curr].valid)
        in_flight_branches++;
    }
    assert(in_flight_branches <= max_in_flight_branches);
    assert(!pred->use_bq || in_flight_branches == pred->bq.num);
  }

  for (i = 0;
       /* fetch until IFETCH -> DISPATCH queue fills */
       ifq_num < ruu_ifq_size - 1 /* FIXME: why -1? */
       /* and no branch taken */
       && !done
       /* and still in the same fetch unit (max insts from a cache line that
	* can be fetched in one cycle; <= cache line size) */
       && i < fetch_unit_size
       /* and still in the same cache line */
       && (IACOMPRESS(fetch_pred_PC) >> cache_il1_blkshift) == cache_line;
       i++, conf = HighConf, forked = FALSE, could_have_forked = FALSE)
  {
    /* fetch an instruction at the next predicted fetch address */
    fetch_regs_PC = fetch_pred_PC;

    /* is this a bogus text address? (can happen on mis-spec path) */
    if (ld_text_base <= fetch_regs_PC && fetch_regs_PC < (ld_text_base + ld_text_size) && !(fetch_regs_PC & (sizeof(SS_INST_TYPE) - 1)))
    {
      /* read instruction from memory */
      mem_access(Read, fetch_regs_PC, &inst, sizeof(SS_INST_TYPE));
    }
    else
    {
      /* fetch PC is bogus, send a NOP down the pipeline */
      inst = SS_NOP_INST;
    }

    /* have a valid inst, here */
    op = SS_OPCODE(inst);

#ifdef LIST_FETCH
    if (!(SS_OP_FLAGS(op) & F_CTRL))
      fprintf(outfile, "0x%08x %s\t%.0f, t%d\n",
              fetch_regs_PC, SS_OP_NAME(op), (double)sim_cycle, thread);
#endif
    /* if too many branches are in-flight, stall fetch for this thread
       * (max_in_flight_branches of 0 means no limit) */
    if (max_in_flight_branches)
    {
      if ((SS_OP_FLAGS(op) & F_CTRL) && in_flight_branches >= max_in_flight_branches)
      {
        done = TRUE;
        num_in_flight_branch_overflows++;
        fetch_regs_PC = fetch_pred_PC - SS_INST_SIZE;
        continue;
      }
      else if (SS_OP_FLAGS(op) & F_CTRL)
        in_flight_branches++;
    }

    /* get the next predicted fetch address; only use branch
       * predictor result for branches (assumes pre-decode bits).
       * NOTE: returned value may be 1 if bpred can only predict a 
       * direction */
    if (pred)
    {
      bpred_recover_rec.contents.stack_copy = NULL;
      retstack_copy_rec.contents.stack_copy = NULL;

      if (SS_OP_FLAGS(op) & F_CTRL)
      {
        int hw_threshold;

        fetch_pred_PC = hydra_bpred_lookup(thread, fetch_regs_PC, inst,
                                           &b_update_rec,
                                           &bpred_recover_rec);

#ifdef LIST_FETCH
        fprintf(outfile, "0x%08x %s\t%.0f, t%d, 0x%08x\n",
                fetch_regs_PC, SS_OP_NAME(op), (double)sim_cycle,
                thread, 0 /*bpred_recover_rec.global_history*/);
#endif
        /* if we're not doing fork-in-fetch, we need to have a copy
	       * of the retstack available */
        if (!fork_in_fetch)
        {
          retstack_copy_rec.contents.stack_copy = calloc(retstack_size, sizeof(struct bpred_btb_ent));
          if (!retstack_copy_rec.contents.stack_copy)
            fatal("out of memory:couldn't allocate retstack copy");
          if (per_thread_retstack == PerThreadStacks)
          {
            memcpy(retstack_copy_rec.contents.stack_copy,
                   thread_info[thread].retstack,
                   retstack_size * sizeof(struct bpred_btb_ent));
            retstack_copy_rec.tos = thread_info[thread].retstack_tos;
          }
          else
          {
            memcpy(retstack_copy_rec.contents.stack_copy,
                   pred->retstack.stack,
                   pred->retstack.size * sizeof(struct bpred_btb_ent));
            retstack_copy_rec.tos = pred->retstack.tos;
          }
        }

        /* pass predPC_if_taken to next stage */
        ifq[ifq_tail].predPC_if_taken = b_update_rec.predPC_if_taken;

        /* get a confidence prediction for this branch, regardless
	       * of where we do forking */
        hw_threshold = max_threads - num_active_forks;

        if (bconf && (SS_OP_FLAGS(op) & F_COND))
          conf = bconf_lookup(bconf, fetch_regs_PC,
                              hw_threshold, &conf_data);
        /*	      
	      if (bconf && (SS_OP_FLAGS(op) & F_COND))
		conf = bconf_lookup(bconf, fetch_regs_PC,
				    (b_update_rec.num_bits 
				     ? b_update_rec.bits
				     : 0xffffffff)); */
        /* (might want to pass in num_bits, too */
        else if (bconf_type == BCF_Naive && (SS_OP_FLAGS(op) & F_COND))
          conf = LowConf;
      }
      else
        fetch_pred_PC = 0;

      /* were we able to fork (ie thread contexts available) this cycle? */
      could_have_forked = max_threads - 1 - num_active_forks;

      /* record what the pred PC would be if we hadn't forked */
      if (!fetch_pred_PC)
        base_pred_PC = fetch_regs_PC + SS_INST_SIZE;
      else
        base_pred_PC = fetch_pred_PC;

      /* if we're doing forking in the fetch stage, try to fork for
	   * low-confidence branches */
      if (fork_in_fetch)
      {
        if (conf == LowConf)
        {
          dassert(b_update_rec.predPC_if_taken);

          /* FIXME: maybe we shouldn't fork in the (rare) cases where 
		   * pred-taken but pred-PC is just the incremented PC */
          if (thread_info[thread].pred_path_token || !fork_prune)
            new_thread = fork_thread(b_update_rec.predPC_if_taken,
                                     thread);
          else if (fork_prune && num_active_forks < max_threads - 1)
            total_forks_pruned++;

          assert(num_active_forks >= 0 && num_active_forks < max_threads);

          /* if fork was successful, this path continues down not-taken
		   * path */
          if (new_thread >= 0)
          {
            ifq[ifq_tail].forked_thread = new_thread;
            fetch_pred_PC = fetch_regs_PC + sizeof(SS_INST_TYPE);
            forked = TRUE;
          }
        }
      }

      if (conf == HighConf || new_thread < 0)
      {
        /* either we have a high-conf prediction, couldn't fork
	       * a thread, or an unconditional branch -- treat this
	       * branch "the old fashioned way" */

        if (!fetch_pred_PC)
        {
          /* no predicted taken target, attempt not taken target */
          fetch_pred_PC = fetch_regs_PC + sizeof(SS_INST_TYPE);
        }
        else
        {
          /* go with target, NOTE: discontinuous fetch, so terminate 
		   * NOTE: fetch_pred_PC might now contain a 1 if predicted-
		   * taken but miss in BTB */
          done = TRUE;
        }
      }
    }
    else
    {
      /* no predictor, just default to predict not taken, and
	     continue fetching instructions linearly */
      fetch_pred_PC = base_pred_PC = fetch_regs_PC + sizeof(SS_INST_TYPE);
    }

    /* commit this instruction to the IFETCH -> DISPATCH queue */
    ifq[ifq_tail].IR = inst;
    ifq[ifq_tail].regs_PC = fetch_regs_PC;
    ifq[ifq_tail].pred_PC = fetch_pred_PC;
    ifq[ifq_tail].base_pred_PC = base_pred_PC;
    ifq[ifq_tail].b_update_rec = b_update_rec;
    ifq[ifq_tail].bpred_recover_rec = bpred_recover_rec;
    ifq[ifq_tail].retstack_copy_rec = retstack_copy_rec;
    ifq[ifq_tail].ptrace_seq = ptrace_seq++;
    ifq[ifq_tail].could_have_forked = could_have_forked;
    ifq[ifq_tail].conf = conf;
    ifq[ifq_tail].conf_data = conf_data;
    ifq[ifq_tail].forked = forked;
    ifq[ifq_tail].thread = thread;
    BITMAP_COPY(ifq[ifq_tail].fork_hist_bmap,
                thread_info[thread].fork_hist_bmap,
                THREADS_BMAP_SZ);
    ifq[ifq_tail].fork_hist_bmap_ptr = thread_info[thread].fork_hist_bmap_ptr;
    dassert(!ifq[ifq_tail].valid);
    ifq[ifq_tail].fetched_at = sim_cycle;
    ifq[ifq_tail].valid = TRUE;

    /* for pipe trace */
    if (ptrace_level != PTRACE_FUNSIM)
    {
      ptrace_newinst(ifq[ifq_tail].ptrace_seq,
                     inst, ifq[ifq_tail].regs_PC, thread, 0);
      ptrace_newstage(ifq[ifq_tail].ptrace_seq, thread,
                      PST_IFETCH,
                      ((forked ? PEV_FORKED : 0) | (last_inst_missed ? PEV_CACHEMISS : 0) | (last_inst_tmissed ? PEV_TLBMISS : 0) | (thread_info[thread].spec_mode ? PEV_SPEC_MODE : 0)));
      if (forked)
        ptrace_newthread(new_thread,
                         fetch_regs_PC, b_update_rec.predPC_if_taken);
    }

    /* if we just forked, modify not-taken path's thread record accordingly
       * (only taken path's thread got changed by fork_thread()). */
    if (forked)
    {
      assert(fork_in_fetch);
      thread_info[thread].fork_hist_bmap_ptr = (thread_info[thread].fork_hist_bmap_ptr + 1) % N_SPEC_LEVELS;
      (void)BITMAP_CLEAR(thread_info[thread].fork_hist_bmap,
                         THREADS_BMAP_SZ,
                         thread_info[thread].fork_hist_bmap_ptr);
    }

    /* adjust instruction fetch queue */
    ifq_tail = (ifq_tail + 1) & (ruu_ifq_size - 1);
    ifq_num++;
    num_fetched++;
  }

  /* We're done with this cache line.  Clear the miss flags. */
  last_inst_missed = FALSE;
  last_inst_tmissed = FALSE;

  thread_info[thread].fetchable = sim_cycle;

failed_fetch:
  /* update per-thread fetch state */
  thread_info[thread].fetch_regs_PC = fetch_regs_PC;
  thread_info[thread].fetch_pred_PC = fetch_pred_PC;
  thread_info[thread].last_inst_missed = last_inst_missed;
  thread_info[thread].last_inst_tmissed = last_inst_tmissed;

  *p_num_fetched = num_fetched;
  return done;
}

/* ruu_fetch_wrapper:
 * controls how many times each thread may call ruu_fetch(), which in
 * turn does the actual fetching and enqueueing in the IFQ
 */
static void
ruu_fetch_wrapper(void)
{
  int num_fetched = 0;
  int done, i;
  int cache_lines_left, lines_per_thread, pred_thread_fetchable, pri, thread;
  static int go_backwards;

  if (fetch_pri_pol == Pred_RR || fetch_pri_pol == Pred_Pri2)
  {
    if (go_backwards)
      last_thread_fetched = pred_thread + 1;
    else
      last_thread_fetched = pred_thread - 1;
    pred_thread_fetchable = (thread_info[pred_thread].fetchable <= sim_cycle);
  }

  /* call instruction fetch unit -- for time being, can only
   * fetch once per thread per cycle */
  for (cache_lines_left = fetch_cache_lines;
       cache_lines_left > 0 && ifq_num < ruu_ifq_size;)
  {
    /* choose a thread to fetch from; if get_next returns -1,
       * no threads are ready and we end fetching for this cycle */
    if ((thread = get_next_fetch_thread(&pri, go_backwards)) < 0)
      break;
    done = FALSE;

    /* how many lines may this thread fetch? 
       * Divide the I$ bandwidth among the active threads.  Round
       * UP to favor a particular thread's getting more instructions
       * each cycle, even if this means some thread can't fetch */
    if (fetch_pri_pol != Simple_RR && fetch_pri_pol != Pred_RR && fetch_pri_pol != Pred_Pri2)
      lines_per_thread = MAX(1, fetch_cache_lines >> mylog2(num_active_forks + 1));
    /* but if policy is the "new" simple_rr, fetch one line
       * from each thread til we run out of bandwidth.  Stop
       * fetching from a thread if it hits a taken branch, but
       * using 'fetched_this_cycle', waste bw this thread would
       * realistically receive */
    else
      lines_per_thread = 1;

    if (max_cache_lines)
    {
      /* FIXME: max_cache_lines NYI for these policies... */
      dassert(fetch_pri_pol != Simple_RR && fetch_pri_pol != Pred_RR && fetch_pri_pol != Pred_Pri2);
      lines_per_thread = MIN(max_cache_lines, lines_per_thread);
    }

    /* accommodate fetch-priority information; if a higher-pri
       * thread doesn't get extra B/W this cycle because a lot's 
       * already been consumed, give it priority next cycle */
    if (fetch_pred_pri || fetch_ruu_pri)
    {
      lines_per_thread *= pri;
      if (max_cache_lines)
        lines_per_thread = MIN(max_cache_lines, lines_per_thread);

      if (lines_per_thread > cache_lines_left || lines_per_thread > max_cache_lines)
        thread_info[thread].priority = 1;
    }

    /* don't fetch more lines than there is B/W remaining */
    lines_per_thread = MIN(cache_lines_left, lines_per_thread);

    for (i = 0; i < lines_per_thread; i++)
    {
      int fetched_this_line = 0;
      /* each call to ruu_fetch() fetches one cache line and
	   * returns TRUE if a blocking condition (pred-taken or
	   * a miss) was encountered.
	   * Each thread's allocation is fixed once per cycle,
	   * so if we encounter a blocking condition, remaining
	   * cache lines for this thread are wasted */
      if (!done && !thread_info[thread].fetched_this_cycle)
      {
        done = ruu_fetch(thread, &fetched_this_line);

        if (!done && (fetch_pri_pol == Simple_RR || fetch_pri_pol == Pred_RR))
          thread_info[thread].fetched_this_cycle = FALSE;
        else if (done)
          thread_info[thread].fetched_this_cycle = INTERMEDIATE;
        else if (fetch_pri_pol == Pred_Pri2 && thread != pred_thread && pred_thread_fetchable)
          thread_info[thread].fetched_this_cycle = TRUE;

        num_fetched += fetched_this_line;
        thread_info[thread].lines_fetched_this_cycle++;
        if (thread_info[thread].lines_fetched_this_cycle >= max_cache_lines)
          thread_info[thread].fetched_this_cycle = TRUE;
      }
      cache_lines_left--;
      assert(cache_lines_left >= 0);
      /* ruu_fetch() updates thread_info[] so that the next call to
	   * get_next_fetch_thread() makes an accurate choice */
    }
  }

  /* reset how-many-times-fetched-per-cache-line state */
  go_backwards = reset_fetch(go_backwards);

  /* record fetch statistics if appropriate */
  assert(num_fetched <= MaxFetch);
  if (report_fetch && done_priming)
    stat_add_sample(fetch_dist, num_fetched);
  sim_total_fetched += num_fetched;
}

/*
 * OTHER AUXILLIARY FUNCTIONS
 */

/* default machine state accessor, used by DLite and possibly others */
static char *                  /* err str, NULL for no err */
simoo_mstate_obj(FILE *stream, /* output stream */
                 char *cmd,    /* optional command string */
                 int thread)   /* optional thread id */
{
  if (!cmd || !strcmp(cmd, "help"))
    fprintf(stream,
            "mstate commands:\n"
            "\n"
            "    mstate help   - show all machine-specific commands (this list)\n"
            "    mstate stats  - dump all statistical variables\n"
            "    mstate res    - dump current functional unit resource states\n"
            "    mstate ruu    - dump contents of the register update unit\n"
            "    mstate lsq    - dump contents of the load/store queue\n"
            "    mstate eventq - dump contents of event queue\n"
            "    mstate readyq - dump contents of ready instruction queue\n"
            "    mstate cv     - dump contents of the register create vector\n"
            "    mstate rspec  - dump contents of speculative regs\n"
            "    mstate mspec  - dump contents of speculative memory\n"
            "    mstate fetch  - dump contents of fetch stage registers and fetch queue\n"
            "\n");
  else if (!strcmp(cmd, "stats"))
  {
    /* just dump intermediate stats */
    sim_print_stats(stream);
  }
  else if (!strcmp(cmd, "res"))
  {
    /* dump resource state */
    res_dump(fu_pool, stream);
  }
  else if (!strcmp(cmd, "ruu"))
  {
    /* dump RUU contents */
    ruu_dump(stream);
  }
  else if (!strcmp(cmd, "lsq"))
  {
    /* dump LSQ contents */
    lsq_dump(stream);
  }
  else if (!strcmp(cmd, "eventq"))
  {
    /* dump event queue contents */
    eventq_dump(stream);
  }
  else if (!strcmp(cmd, "readyq"))
  {
    /* dump ready queue contents */
    readyq_dump(stream);
  }
  else if (!strcmp(cmd, "cv"))
  {
    /* dump create-vector contents */
    cv_dump(stream);
  }
  else if (!strcmp(cmd, "rspec"))
  {
    /* dump speculative register contents */
    rspec_dump(stream);
  }
  else if (!strcmp(cmd, "mspec"))
  {
    /* dump speculative memory state */
    mspec_dump(stream);
  }
  else if (!strcmp(cmd, "fetch"))
  {
    /* dump fetch->dispatch queue contents */
    fetch_dump(stream);
  }
  else
    return "unknown mstate command";

  /* no error */
  return NULL;
}

/* initialize the simulator */
void sim_init(void)
{
  int t;

  cache_dl1_ports_used = 0;
  cache_dl1_ports_reserved = 0;

  sim_num_insn = 0;
  sim_num_refs = 0;

  /* initialize here, so symbols can be loaded */
  if (ptrace_nelt == 3)
  {
    /* generate a pipeline trace */
    ptrace_open(/* level */ ptrace_opts[0],
                /* fname */ ptrace_opts[1], /* range */ ptrace_opts[2]);
  }
  else if (ptrace_nelt == 0)
  {
    /* no pipetracing */;
  }
  else
    fatal("bad pipetrace args, use: <level> <fname|stdout|stderr> <range>");

  /* decode all instructions */
  {
    SS_ADDR_TYPE addr;
    SS_INST_TYPE inst;

    if (OP_MAX > 255)
      fatal("cannot do fast decoding, too many opcodes");

    debug("sim: decoding text segment...");
    for (addr = ld_text_base;
         addr < (ld_text_base + ld_text_size);
         addr += SS_INST_SIZE)
    {
      inst = __UNCHK_MEM_ACCESS(SS_INST_TYPE, addr);
      inst.a = (inst.a & ~0xff) | (unsigned int)SS_OP_ENUM(SS_OPCODE(inst));
      __UNCHK_MEM_ACCESS(SS_INST_TYPE, addr) = inst;
    }
  }

  /* initialize the simulation engine */
#if 0
  fprintf(stderr, "running with N_SPEC_LEVELS at %d\n", N_SPEC_LEVELS);
#endif
  if (!infinite_fu)
    fu_pool = res_create_pool("fu-pool", fu_config, N_ELT(fu_config));
  fu_oplat_arr[NA] = 0;
  fu_oplat_arr[IntALU] = fu_config[FU_IALU_INDEX].x[0].oplat;
  fu_oplat_arr[IntSHIFT] = fu_config[FU_IBRSH_INDEX].x[1].oplat;
  fu_oplat_arr[IntMULT] = fu_config[FU_IMULT_INDEX].x[0].oplat;
  fu_oplat_arr[IntDIV] = fu_config[FU_IMULT_INDEX].x[1].oplat;
  fu_oplat_arr[FloatADD] = fu_config[FU_FPALU_INDEX].x[0].oplat;
  fu_oplat_arr[FloatCMP] = fu_config[FU_FPALU_INDEX].x[1].oplat;
  fu_oplat_arr[FloatCVT] = fu_config[FU_FPALU_INDEX].x[2].oplat;
  fu_oplat_arr[FloatMULT] = fu_config[FU_FPMULT_INDEX].x[0].oplat;
  fu_oplat_arr[FloatDIV] = fu_config[FU_FPDIV_INDEX].x[0].oplat;
  fu_oplat_arr[FloatSQRT] = fu_config[FU_FPDIV_INDEX].x[1].oplat;
  fu_oplat_arr[RdPort] = fu_config[FU_LDPORT_INDEX].x[0].oplat;
  fu_oplat_arr[WrPort] = fu_config[FU_STPORT_INDEX].x[0].oplat;
  fu_oplat_arr[Branch] = fu_config[FU_IBRSH_INDEX].x[0].oplat;

  rslink_init(MAX_RS_LINKS);
  tracer_init();
  fetch_init();
  cv_init();
  eventq_init();
  readyq_init();
  ruu_init();
  lsq_init();
  retired_inst_list_init();

  thread_info[INIT_THREAD].valid = TRUE;
  thread_info[INIT_THREAD].fetchable = 0;
  thread_info[INIT_THREAD].pred_path_token = TRUE;
#ifdef DEBUG_PRED_PRI
  thread_info[INIT_THREAD].new_pred_path_token = 1;
#endif

  for (t = 0; t < N_THREAD_RECS; t++)
  {
    thread_info[t].squashed = UNKNOWN;
    thread_info[t].base_priority = -1;
    thread_info[t].priority = -1;
    thread_info[t].fetched_this_cycle = FALSE;

    if (per_thread_retstack == PerThreadStacks)
    {
      if (!(thread_info[t].retstack = calloc(retstack_size,
                                             sizeof(struct bpred_btb_ent))))
        fatal("cannot allocate ret-addr-stack for thread %d", t);
      thread_info[t].retstack_tos = retstack_size - 1;
    }
    if (per_thread_retstack == PerThreadTOSP)
      thread_info[t].retstack_tos = retstack_size - 1;
  }
  if (fetch_pri_pol == Omni_Pri || fetch_pri_pol == Two_Omni_Pri || fetch_pri_pol == Pred_Pri || fetch_pri_pol == Ruu_Pri)
  {
    thread_info[INIT_THREAD].base_priority = 1;
    thread_info[INIT_THREAD].priority = 1;
  }
  /* thread PCs get set up in sim_main after warmup */

#ifdef USE_DLITE
  /* initialize the DLite debugger */
  dlite_init(simoo_reg_obj, simoo_mem_obj, simoo_mstate_obj);
#endif
}

/* dump simulator-specific auxiliary simulator statistics */
void sim_aux_stats(FILE *stream) /* output stream */
{
  /* nada */
}

/* un-initialize the simulator */
void sim_uninit(void)
{
  if (ptrace_nelt > 0)
    ptrace_close();
}

/* exit signal handler */
static void
signal_fatal(int sigtype)
{
  psignal(sigtype, "FATAL");
  exit_now(sigtype);
}

void after_priming(void)
{
  primed_cycles = sim_cycle;
  primed_insts = sim_num_insn;
  primed_refs = sim_num_refs;
  primed_loads = sim_num_loads;

  sim_num_branches = 0;
  lsq_hits = 0;

  cache_after_priming(cache_dl1);
  cache_after_priming(cache_dl2);
  cache_after_priming(cache_il1);
  cache_after_priming(cache_il2);
  cache_after_priming(itlb);
  cache_after_priming(dtlb);
  bpred_after_priming(pred);
  bconf_after_priming(bconf);
}

void after_warmup(void)
{
  cache_after_warmup(cache_dl1);
  cache_after_warmup(cache_dl2);
  cache_after_warmup(cache_il1);
  cache_after_warmup(cache_il2);
  cache_after_warmup(itlb);
  cache_after_warmup(dtlb);
}

int sim_warmup = FALSE;
extern void warmup_main(SS_COUNTER_TYPE num_warmup_insn);

/* start simulation, program loaded, processor precise state initialized */
void sim_main(void)
{
  fprintf(outfile, "N_SPEC_LEVELS = %d, N_THREAD_RECS = %d\n\n",
          N_SPEC_LEVELS, N_THREAD_RECS);
  fprintf(outfile, "sim: ** starting performance simulation **\n");

  /* ignore any floating point exceptions, which may occur on mis-speculated
   * execution paths */
  signal(SIGFPE, SIG_IGN);
  /* dump stats after seg-fault, etc, to make debugging easier */
  signal(SIGSEGV, signal_fatal);
  signal(SIGBUS, signal_fatal);
  signal(SIGILL, signal_fatal);

#ifdef USE_DLITE
  /* check for DLite debugger entry condition */
  if (dlite_check_break(regs_PC, /* no access */ 0, /* addr */ 0, 0, 0))
    dlite_main(regs_PC, regs_PC + SS_INST_SIZE, sim_cycle);
#endif

  /*
   * WARMUP CACHES
   *
   * sim-cache derivative will run for num_warmup_insn.  State will
   * then be primed for POST_WARMUP_PRIME insts, and then "real"
   * simulation will proceed for num_fullsim_insn.
   */
  if (num_warmup_insn > 0)
  {
    /* regs_PC should have been initialized in regs_init() */

    fprintf(outfile, "sim: ** warming up caches for %d instructions **\n",
            num_warmup_insn);
    sim_warmup = TRUE;
    warmup_main((SS_COUNTER_TYPE)num_warmup_insn);
    sim_warmup = FALSE;
    fprintf(outfile,
            "sim: ***** finished warming up caches for %.0f cycles *****\n",
            (double)num_warmup_insn);
    after_warmup();
    assert(pred->retstack.caller_supplies_tos ==
           !!(per_thread_retstack == PerThreadTOSP));
    fprintf(outfile,
            "sim: will prime microarchitectural state for further %d cycles\n",
            num_prime_insn - num_warmup_insn);
  }

  if (num_fullsim_insn)
    fprintf(outfile, "sim: will simulate with full detail for %.0f cycles\n",
            (double)num_fullsim_insn);
  else
    fprintf(outfile,
            "sim: will simulate with full detail until program exits\n");

  /* set up program entry state; other variables are set to 0 because
   * thread_info[] is static.  regs_PC was initialized in regs_init(),
   * or contains the last instruction executed in warmup. */
  thread_info[INIT_THREAD].fetch_regs_PC = regs_PC - sizeof(SS_INST_TYPE);
  thread_info[INIT_THREAD].fetch_pred_PC = regs_PC;

  /* we no longer use regs_PC except for pipetracing, instead using the 
   * thread_info records. Instructions will still set regs_PC, since 
   * instructions are defined in ss.def, common to all the simulators.
   * Note we have no concept of "current thread": get_next_fetch_thread()
   * chooses which thread(s) to fetch from; after that all insts are tagged
   * with thread id as they flow through the machine. */

  /*
   * main simulator loop, NOTE: the pipe stages are traversed in reverse order
   * to eliminate this/next state synchronization and relaxation problems 
   */
  for (;;)
  {
    cache_dl1_ports_reserved = 0;

    /* 
       * Check simulation's progress
       */

    /* Decide whether we've finished priming */
    if (!done_priming && sim_num_insn >= num_prime_insn)
    {
      /* We've moved into the post-prime state */
      done_priming = TRUE;
      after_priming();
    }

    /* Decide whether we've finished executing */
    if (num_fullsim_insn != 0)
      if ((sim_num_insn >= (num_prime_insn + num_fullsim_insn)) || sim_exit_now)
        exit_now(0);

    /* Decide whether to dump intermediate stats */
    if (sim_dump_stats)
    {
      simoo_mstate_obj(outfile, "stats", 0);
      sim_dump_stats = FALSE;
    }

    /* RUU/LSQ sanity checks */
#ifdef DEBUG
    if (RUU_num < LSQ_num)
      panic("RUU_num < LSQ_num");
    if (((RUU_head + RUU_num) % RUU_size) != RUU_tail)
      panic("RUU_head/RUU_tail wedged");
    if (((LSQ_head + LSQ_num) % LSQ_size) != LSQ_tail)
      panic("LSQ_head/LSQ_tail wedged");
    if ((IIQ_occ > IIQ_size) || (FIQ_occ > FIQ_size))
      panic("IIQ or FIQ overflow");
    if (IIQ_occ + FIQ_occ > RUU_num + LSQ_num)
      panic("IIQ or FIQ wedged");
#endif

    /* check if pipetracing is still active */
    ptrace_check_active(regs_PC, sim_num_insn, sim_cycle);

    /* indicate new cycle in pipetrace */
#ifndef FUNSIM_PRINT_CYCLES
    if (ptrace_level != PTRACE_FUNSIM)
#endif
      ptrace_newcycle(sim_cycle, RUU_num, LSQ_num, ifq_num);

    /*
       * TRAVERSE PIPE STAGES
       */

    /* commit entries from RUU/LSQ to architected register file */
    ruu_commit();

    /* service function unit release events */
    if (!infinite_fu)
      ruu_release_fu();

    /* ==> may have ready queue entries carried over from previous cycles */

    /* service result completions, also readies dependent operations */
    /* ==> inserts operations into ready queue --> register deps resolved */
    ruu_writeback();

    /* try to locate memory operations that are ready to execute */
    /* ==> inserts operations into ready queue --> mem deps resolved */
    lsq_refresh();

    /* issue operations ready to execute from a previous cycle */
    /* <== drains ready queue <-- ready operations commence execution */
    ruu_issue();

    /* decode and dispatch new operations */
    /* ==> insert ops w/ no deps or all regs ready --> reg deps resolved */
    ruu_dispatch();

    /* fetch new instructions
       * ==> insert ops into IFQ */
    ruu_fetch_wrapper();

    /* kill squashed threads that no longer have any active instructions */
    if (max_threads > 1)
      kill_threads();
    else
      assert(thread_info[INIT_THREAD].valid == TRUE);

    /* go to next cycle */
    sim_cycle++;

    /* cache_dl1 port bookkeeping */
    assert(cache_dl1_ports_reserved <= cache_dl1_ports && cache_dl1_ports_used <= cache_dl1_ports);
    cache_dl1_ports_used = cache_dl1_ports_reserved;
  }
}
