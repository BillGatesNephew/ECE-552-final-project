/*
 * sim-cache.c - sample cache simulator implementation
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
 * $Id: sim-cache.c,v 3.1 1998/04/17 16:51:35 skadron Exp $
 *
 * $Log: sim-cache.c,v $
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
 * Revision 1.6  1997/04/16  22:09:45  taustin
 * fixed "bad l2 D-cache parms" fatal string
 *
 * Revision 1.5  1997/03/11  01:27:08  taustin
 * updated copyright
 * `-pcstat' option support added
 * long/int tweaks made for ALPHA target support
 * better defaults defined for caches/TLBs
 * "mstate" command supported added for DLite!
 * supported added for non-GNU C compilers
 *
 * Revision 1.4  1997/01/06  16:03:07  taustin
 * comments updated
 * supported added for 2-level cache memory system
 * instruction TLB supported added
 * -icompress now compresses 64-bit instruction addresses to 32-bit equiv
 * main loop simplified
 *
 * Revision 1.3  1996/12/30  17:14:11  taustin
 * updated to support options and stats packages
 *
 * Revision 1.1  1996/12/05  18:52:32  taustin
 * Initial revision
 *
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
#include "loader.h"
#include "syscall.h"
#include "dlite.h"
#include "sim.h"

/*
 * This file implements a functional cache simulator.  Cache statistics are
 * generated for a user-selected cache and TLB configuration, which may include
 * up to two levels of instruction and data cache (with any levels unified),
 * and one level of instruction and data TLBs.  No timing information is
 * generated (hence the distinction, "functional" simulator).
 */

/* track number of insn and refs */
static SS_COUNTER_TYPE sim_num_insn = 0;
static SS_COUNTER_TYPE sim_num_refs = 0;

/* level 1 instruction cache, entry level instruction cache */
static struct cache *cache_il1 = NULL;

/* level 1 instruction cache */
static struct cache *cache_il2 = NULL;

/* level 1 data cache, entry level data cache */
static struct cache *cache_dl1 = NULL;

/* level 2 data cache */
static struct cache *cache_dl2 = NULL;

/* instruction TLB */
static struct cache *itlb = NULL;

/* data TLB */
static struct cache *dtlb = NULL;

/* simulation length, in instructions */
static unsigned int sim_length;

/* text-based stat profiles */
#define MAX_PCSTAT_VARS 8
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

/* l1 data cache l1 block miss handler function */
static unsigned int			/* latency of block access */
dl1_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
	      SS_ADDR_TYPE baddr,	/* block address to access */
	      int bsize,		/* size of block to access */
	      struct cache_blk *blk,	/* ptr to block in upper level */
	      SS_TIME_TYPE now)		/* time of access */
{
  if (cache_dl2)
    {
      /* access next level of data cache hierarchy */
      return cache_access(cache_dl2, cmd, baddr, NULL, bsize,
			  /* now */now, /* pudata */NULL, /* repl addr */NULL);
    }
  else
    {
      /* access main memory, which is always done in the main simulator loop */
      return /* access latency, ignored */1;
    }
}

/* l2 data cache block miss handler function */
static unsigned int			/* latency of block access */
dl2_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
	      SS_ADDR_TYPE baddr,	/* block address to access */
	      int bsize,		/* size of block to access */
	      struct cache_blk *blk,	/* ptr to block in upper level */
	      SS_TIME_TYPE now)		/* time of access */
{
  /* this is a miss to the lowest level, so access main memory, which is
     always done in the main simulator loop */
  return /* access latency, ignored */1;
}

/* l1 inst cache l1 block miss handler function */
static unsigned int			/* latency of block access */
il1_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
	      SS_ADDR_TYPE baddr,	/* block address to access */
	      int bsize,		/* size of block to access */
	      struct cache_blk *blk,	/* ptr to block in upper level */
	      SS_TIME_TYPE now)		/* time of access */
{
  if (cache_il2)
    {
      /* access next level of inst cache hierarchy */
      return cache_access(cache_il2, cmd, baddr, NULL, bsize,
			  /* now */now, /* pudata */NULL, /* repl addr */NULL);
    }
  else
    {
      /* access main memory, which is always done in the main simulator loop */
      return /* access latency, ignored */1;
    }
}

/* l2 inst cache block miss handler function */
static unsigned int			/* latency of block access */
il2_access_fn(enum mem_cmd cmd,		/* access cmd, Read or Write */
	      SS_ADDR_TYPE baddr,	/* block address to access */
	      int bsize,		/* size of block to access */
	      struct cache_blk *blk,	/* ptr to block in upper level */
	      SS_TIME_TYPE now)		/* time of access */
{
  /* this is a miss to the lowest level, so access main memory, which is
     always done in the main simulator loop */
  return /* access latency, ignored */1;
}

/* inst cache block miss handler function */
static unsigned int			/* latency of block access */
itlb_access_fn(enum mem_cmd cmd,	/* access cmd, Read or Write */
	       SS_ADDR_TYPE baddr,	/* block address to access */
	       int bsize,		/* size of block to access */
	       struct cache_blk *blk,	/* ptr to block in upper level */
	       SS_TIME_TYPE now)	/* time of access */
{
  SS_ADDR_TYPE *phy_page_ptr = (SS_ADDR_TYPE *)blk->user_data;

  /* no real memory access, however, should have user data space attached */
  assert(phy_page_ptr);

  /* fake translation, for now... */
  *phy_page_ptr = 0;

  return /* access latency, ignored */1;
}

/* data cache block miss handler function */
static unsigned int			/* latency of block access */
dtlb_access_fn(enum mem_cmd cmd,	/* access cmd, Read or Write */
	       SS_ADDR_TYPE baddr,	/* block address to access */
	       int bsize,		/* size of block to access */
	       struct cache_blk *blk,	/* ptr to block in upper level */
	       SS_TIME_TYPE now)	/* time of access */
{
  SS_ADDR_TYPE *phy_page_ptr = (SS_ADDR_TYPE *)blk->user_data;

  /* no real memory access, however, should have user data space attached */
  assert(phy_page_ptr);

  /* fake translation, for now... */
  *phy_page_ptr = 0;

  return /* access latency, ignored */1;
}

/* cache/TLB options */
static char *cache_dl1_opt /* = "none" */;
static char *cache_dl2_opt /* = "none" */;
static char *cache_il1_opt /* = "none" */;
static char *cache_il2_opt /* = "none" */;
static char *itlb_opt /* = "none" */;
static char *dtlb_opt /* = "none" */;
static int flush_on_syscalls /* = FALSE */;
static int compress_icache_addrs /* = FALSE */;

/* text-based stat profiles */
static int pcstat_nelt = 0;
static char *pcstat_vars[MAX_PCSTAT_VARS];

/* convert 64-bit inst text addresses to 32-bit inst equivalents */
#define IACOMPRESS(A)							\
  (compress_icache_addrs ? ((((A) - SS_TEXT_BASE) >> 1) + SS_TEXT_BASE) : (A))
#define ISCOMPRESS(SZ)							\
  (compress_icache_addrs ? ((SZ) >> 1) : (SZ))

/* register simulator-specific options */
void
sim_reg_options(struct opt_odb_t *odb)	/* options database */
{
  opt_reg_header(odb, 
"sim-cache: This simulator implements a functional cache simulator.  Cache\n"
"statistics are generated for a user-selected cache and TLB configuration,\n"
"which may include up to two levels of instruction and data cache (with any\n"
"levels unified), and one level of instruction and data TLBs.  No timing\n"
"information is generated.\n"
		 );
  opt_reg_string(odb, "-cache:dl1",
		 "l1 data cache config, i.e., {<config>|none}",
		 &cache_dl1_opt, "dl1:256:32:1:l:0:2", /* print */TRUE, NULL);
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
"    Examples:   -cache:dl1 dl1:4096:32:1:l:0:2\n"
"                -dtlb dtlb:128:4096:32:r\n"
	       );
  opt_reg_string(odb, "-cache:dl2",
		 "l2 data cache config, i.e., {<config>|none}",
		 &cache_dl2_opt, "ul2:1024:64:4:l:0:4", /* print */TRUE, NULL);
  opt_reg_string(odb, "-cache:il1",
		 "l1 inst cache config, i.e., {<config>|dl1|dl2|none}",
		 &cache_il1_opt, "il1:256:32:1:l:1:2", /* print */TRUE, NULL);
  opt_reg_note(odb,
"  Cache levels can be unified by pointing a level of the instruction cache\n"
"  hierarchy at the data cache hiearchy using the \"dl1\" and \"dl2\" cache\n"
"  configuration arguments.  Most sensible combinations are supported, e.g.,\n"
"\n"
"    A unified l2 cache (il2 is pointed at dl2):\n"
"      -cache:il1 il1:128:64:1:l:1:2 -cache:il2 dl2\n"
"      -cache:dl1 dl1:256:32:1:l:0:2 -cache:dl2 ul2:1024:64:2:l:0:4\n"
"\n"
"    Or, a fully unified cache hierarchy (il1 pointed at dl1):\n"
"      -cache:il1 dl1\n"
"      -cache:dl1 ul1:256:32:1:l:0:2 -cache:dl2 ul2:1024:64:2:l:0:4\n"
	       );
  opt_reg_string(odb, "-cache:il2",
		 "l2 instruction cache config, i.e., {<config>|dl2|none}",
		 &cache_il2_opt, "dl2", /* print */TRUE, NULL);
  opt_reg_string(odb, "-tlb:itlb",
		 "instruction TLB config, i.e., {<config>|none}",
		 &itlb_opt, "itlb:16:4096:4:l", /* print */TRUE, NULL);
  opt_reg_string(odb, "-tlb:dtlb",
		 "data TLB config, i.e., {<config>|none}",
		 &dtlb_opt, "dtlb:32:4096:4:l", /* print */TRUE, NULL);
  opt_reg_flag(odb, "-flush", "flush caches on system calls",
	       &flush_on_syscalls, /* default */FALSE, /* print */TRUE, NULL);
  opt_reg_flag(odb, "-icompress",
	       "convert 64-bit inst addresses to 32-bit inst equivalents",
	       &compress_icache_addrs, /* default */FALSE,
	       /* print */TRUE, NULL);
  opt_reg_uint(odb, "-sim_insts",
	       "number of instructions for which to simulate before exiting",
	       &sim_length, /* default */1000000000U, /* print */TRUE, NULL);
  opt_reg_string_list(odb, "-pcstat",
		      "profile stat(s) against text addr's (mult uses ok)",
		      pcstat_vars, MAX_PCSTAT_VARS, &pcstat_nelt, NULL,
		      /* !print */FALSE, /* format */NULL, /* accrue */TRUE);
}

/* check simulator-specific option values */
void
sim_check_options(struct opt_odb_t *odb,	/* options database */
		  int argc, char **argv)	/* command line arguments */
{
  char name[128], c;
  int nsets, bsize, assoc, mshrs, busint;

  /* use a level 1 D-cache? */
  if (!mystricmp(cache_dl1_opt, "none") || !mystricmp(cache_dl1_opt, "mem"))
    {
      cache_dl1 = NULL;

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
      cache_dl1 = cache_create(name, nsets, bsize, /* balloc */FALSE,
			      /* usize */0, assoc, cache_char2policy(c),
			      dl1_access_fn, 
			      /* hit lat */1, 1,
			      mshrs, busint);

      /* is the level 2 D-cache defined? */
      if (!mystricmp(cache_dl2_opt, "none"))
	cache_dl2 = NULL;
      else
	{
	  if (sscanf(cache_dl2_opt, "%[^:]:%d:%d:%d:%c:%d:%d",
		     name, &nsets, &bsize, &assoc, &c, &mshrs, &busint) != 7)
	    fatal("bad l2 D-cache parms: "
	      "<name>:<nsets>:<bsize>:<assoc>:<repl>:<mshr's>:<bus interval>");
	  cache_dl2 = cache_create(name, nsets, bsize, /* balloc */FALSE,
				   /* usize */0, assoc, cache_char2policy(c),
				   dl2_access_fn, 
				   /* hit lat */1, 1,
				   mshrs, busint);
	}
    }

  /* use a level 1 I-cache? */
  if (!mystricmp(cache_il1_opt, "none") || !mystricmp(cache_il1_opt, "mem"))
    {
      cache_il1 = NULL;

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
      cache_il1 = cache_create(name, nsets, bsize, /* balloc */FALSE,
			      /* usize */0, assoc, cache_char2policy(c),
			      il1_access_fn, 
			      /* hit lat */1, 1,
			      mshrs, busint);

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
	}
      else if (!mystricmp(cache_il2_opt, "dl1"))
	{
	  if (!cache_dl1)
	    fatal("I-cache l2 cannot access D-cache l1 as it's undefined");
	  cache_il2 = cache_dl1;
	}
      else
	{
	  if (sscanf(cache_il2_opt, "%[^:]:%d:%d:%d:%c:%d:%d",
		     name, &nsets, &bsize, &assoc, &c, &mshrs, &busint) != 7)
	    fatal("bad l2 I-cache parms: "
	      "<name>:<nsets>:<bsize>:<assoc>:<repl>:<mshr's>:<bus interval>");
	  cache_il2 = cache_create(name, nsets, bsize, /* balloc */FALSE,
				   /* usize */0, assoc, cache_char2policy(c),
				   il2_access_fn, 
				   /* hit lat */1, 1,
				   mshrs, busint);
	  if (cache_dl2)
	    cache_set_bus(cache_il2, cache_dl2);
	  else if (cache_dl1)
	    cache_set_bus(cache_il2, cache_dl1);
	}
    }

  /* use a D-TLB? */
  if (!mystricmp(dtlb_opt, "none"))
    dtlb = NULL;
  else
    {
      if (sscanf(dtlb_opt, "%[^:]:%d:%d:%d:%c",
		 name, &nsets, &bsize, &assoc, &c) != 5)
	fatal("bad TLB parms: "
	      "<name>:<nsets>:<page_size>:<assoc>:<repl>");
      dtlb = cache_create(name, nsets, bsize, /* balloc */FALSE,
			  /* usize */sizeof(SS_ADDR_TYPE), assoc,
			  cache_char2policy(c), dtlb_access_fn,
			  /* hit latency */1, 0, /* mshrs */1,
			  /* perfect bus */0);
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
      itlb = cache_create(name, nsets, bsize, /* balloc */FALSE,
			  /* usize */sizeof(SS_ADDR_TYPE), assoc,
			  cache_char2policy(c), itlb_access_fn,
			  /* hit latency */1, 0, /* mshrs */1,
			  /* perfect bus */0);
    }
}

/* print simulator-specific configuration information */
void
sim_aux_config(FILE *stream)		/* output stream */
{
  /* nada */
}

/* register simulator-specific statistics */
void
sim_reg_stats(struct stat_sdb_t *sdb)	/* stats database */
{
  int i;

  /* register baseline stats */
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

  /* register cache stats */
  if (cache_il1
      && (cache_il1 != cache_dl1 && cache_il1 != cache_dl2))
    cache_reg_stats(cache_il1, sdb);
  if (cache_il2
      && (cache_il2 != cache_dl1 && cache_il2 != cache_dl2))
    cache_reg_stats(cache_il2, sdb);
  if (cache_dl1)
    cache_reg_stats(cache_dl1, sdb);
  if (cache_dl2)
    cache_reg_stats(cache_dl2, sdb);
  if (itlb)
    cache_reg_stats(itlb, sdb);
  if (dtlb)
    cache_reg_stats(dtlb, sdb);

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
					/* print fmt */(PF_COUNT|PF_PDF),
					/* format */"0x%lx %lu %.2f",
					/* print fn */NULL);
    }
}

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

  /* initialize the DLite debugger (NOTE: mem is always precise) */
  dlite_init(dlite_reg_obj, dlite_mem_obj, cache_mstate_obj);
}

/* dump simulator-specific auxiliary simulator statistics */
void
sim_aux_stats(FILE *stream)		/* output stream */
{
  /* nada */
}

/* un-initialize the simulator */
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

/* start simulation, program loaded, processor precise state initialized */
void
sim_main(void)
{
  int i;
  SS_INST_TYPE inst;
  register SS_ADDR_TYPE next_PC;
  register SS_ADDR_TYPE addr;
  enum ss_opcode op;
  register int is_write;

  fprintf(outfile, "sim: ** starting functional simulation w/ caches **\n");

  /* set up initial default next PC */
  next_PC = regs_PC + SS_INST_SIZE;

  /* check for DLite debugger entry condition */
  if (dlite_check_break(regs_PC, /* no access */0, /* addr */0, 0, 0))
    dlite_main(regs_PC - SS_INST_SIZE, regs_PC, sim_num_insn);

  while (TRUE)
    {
      if (sim_dump_stats)
	{
	  cache_mstate_obj(outfile, NULL);
	  sim_dump_stats = FALSE;
	}

      if ((sim_length && sim_num_insn >= sim_length) 
	  || sim_exit_now)
	exit_now(0);

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
          panic("attempted to execute a bogus opcode");
	}

      if (SS_OP_FLAGS(op) & F_MEM)
	{
	  sim_num_refs++;
	  if (SS_OP_FLAGS(op) & F_STORE)
	    is_write = TRUE;
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
