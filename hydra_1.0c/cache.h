/*
 * cache.h - cache module interfaces
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
 * $Id: cache.h,v 3.1 1998/04/17 16:51:35 skadron Exp $
 *
 * $Log: cache.h,v $
 * Revision 3.1  1998/04/17 16:51:35  skadron
 * Versions used thru Mar '98 (retstack sub, ruu_resub, ICS final manuscript)
 *
 * Revision 2.101  1997/12/09 22:20:31  skadron
 * Version Pritpal used for ISCA-25 submission
 *
 * Revision 2.100  1997/12/03 20:05:46  skadron
 * Version Kevin used for ISCA-25 submission
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
 * Revision 1.8  1997/04/17  02:26:00  skadron
 * Added some fields to cache struct to allow computing interval miss
 * rates
 *
 * Revision 1.7  1997/04/16  16:39:10  skadron
 * Better bus modeling -- fixed interval between bus transactions
 *
 * Revision 1.6  1997/04/15  19:02:58  skadron
 * Added cache_after_warmup()
 *
 * Revision 1.5  1997/04/11  01:04:17  skadron
 * Added mshr's.  Infinite secondary misses allowed.  Also added ability
 *    to report dist of cache latencies, turned on by compiling w/
 *    -DLAT_INFO.  Removed some bus stuff I'd been planning but will
 * defer.
 *
 * Revision 1.4  1997/04/09  21:38:20  skadron
 * Changed treatment of cache latencies: each cache now has a base access
 *    time (for probing the tags) plus optional extra latency incurred on
 *    hits.  The major change is that previously, misses were not charged
 *    any time!  Now they're charged the base latency.
 *
 * Revision 1.3  1997/03/25  16:17:07  skadron
 * Added function called after priming
 *
 * Revision 1.2  1997/02/24  18:03:41  skadron
 * Added some counters: reads and writes, as well as read_hits.  Added a
 *    'perfect' parameter, too.
 *
 * Revision 1.1  1997/02/16  22:23:54  skadron
 * Initial revision
 *
 * Revision 1.3  1997/01/06  15:57:55  taustin
 * comments updated
 * cache_reg_stats() now works with stats package
 * cp->writebacks stat added to cache
 *
 * Revision 1.1  1996/12/05  18:50:23  taustin
 * Initial revision
 *
 *
 */

#ifndef CACHE_H
#define CACHE_H

#include <stdio.h>
#include "ss.h"
#include "memory.h"
#include "stats.h"

/*
 * This module contains code to implement various cache-like structures.  The
 * user instantiates caches using cache_new().  When instantiated, the user
 * may specify the geometry of the cache (i.e., number of set, line size,
 * associativity), and supply a block access function.  The block access
 * function indicates the latency to access lines when the cache misses,
 * accounting for any component of miss latency, e.g., bus acquire latency,
 * bus transfer latency, memory access latency, etc...  In addition, the user
 * may allocate the cache with or without lines allocated in the cache.
 * Caches without tags are useful when implementing structures that map data
 * other than the address space, e.g., TLBs which map the virtual address
 * space to physical page address, or BTBs which map text addresses to
 * branch prediction state.  Tags are always allocated.  User data may also be
 * optionally attached to cache lines, this space is useful to storing
 * auxilliary or additional cache line information, such as predecode data,
 * physical page address information, etc...
 *
 * The caches implemented by this module provide efficient storage management
 * and fast access for all cache geometries.  When sets become highly
 * associative, a hash table (indexed by address) is allocated for each set
 * in the cache.
 *
 * This module also tracks latency of accessing the data cache, each cache has
 * a hit latency defined when instantiated, miss latency is returned by the
 * cache's block access function, the calling simulator should limit the number
 * of outstanding misses or the number of hits under misses as per the
 * limitations of the particular microarchitecture being simulated.
 *
 * Due to the organization of this cache implementation, the latency of a
 * request cannot be affected by a later request to this module.  As a result,
 * reordering of requests in the memory hierarchy is not possible.
 */

/* highly associative caches are implemented using a hash table lookup to
   speed block access, this macro decides if a cache is "highly associative" */
#define CACHE_HIGHLY_ASSOC(cp)	((cp)->assoc > 4)

/* cache replacement policy */
enum cache_policy {
  LRU,		/* replace least recently used block (perfect LRU) */
  Random,	/* replace a random block */
  FIFO		/* replace the oldest block in the set */
};

/* block status values */
#define CACHE_BLK_VALID		0x00000001	/* block in valid, in use */
#define CACHE_BLK_DIRTY		0x00000002	/* dirty block */

/* cache block (or line) definition */
struct cache_blk
{
  struct cache_blk *way_next;	/* next block in the ordered way chain, used
				   to order blocks for replacement */
  struct cache_blk *way_prev;	/* previous block in the order way chain */
  struct cache_blk *hash_next;	/* next block in the hash bucket chain, only
				   used in highly-associative caches */
  /* since hash table lists are typically small, there is no previous
     pointer, deletion requires a trip through the hash table bucket list */
  SS_ADDR_TYPE tag;		/* data block tag value */
  unsigned int status;		/* block status, see CACHE_BLK_* defs above */
  SS_TIME_TYPE ready;		/* time when block will be accessible, field
				   is set when a miss fetch is initiated */
  char *user_data;		/* pointer to user defined data, e.g.,
				   pre-decode data or physical page address */
  /* DATA should be pointer-aligned due to preceeding field */
  char data[1];			/* actual data block starts here, block size
				   should probably be a multiple of 8 */
};

/* cache set definition (one or more blocks sharing the same set index) */
struct cache_set
{
  struct cache_blk **hash;	/* hash table: for fast access w/assoc, NULL
				   for low-assoc caches */
  struct cache_blk *way_head;	/* head of way list */
  struct cache_blk *way_tail;	/* tail pf way list */
  struct cache_blk *blks;	/* cache blocks, allocated sequentially, so
				   this pointer can also be used for random
				   access to cache blocks */
};

/* mshr definition 
 * An mshr is allocated for each primary miss (a miss is primary when there
 * are no currently outstanding misses to that block; secondary otherwise)
 * and freed once current time reaches 'when_free' */
struct mshr
{
  int valid;			/* does this contain a pending miss? */
  SS_ADDR_TYPE tag;		/* which cache block does this mshr describe */
  SS_TIME_TYPE when_free;	/* when will this mshr next be available */
  struct mshr *next;		/* list is kept sorted by ascending time */
};

/* cache definition */
struct cache
{
  /* parameters */
  char *name;			/* cache name */
  int nsets;			/* number of sets */
  int bsize;			/* block size in bytes */
  int balloc;			/* maintain cache contents? */
  int usize;			/* user allocated data size */
  int assoc;			/* cache associativity */
  enum cache_policy policy;	/* cache replacement policy */
  unsigned int base_lat;	/* cache access latency -- incurred on every
				 * access, including misses */
  unsigned int extra_hit_lat; 	/* extra latency incurred on a hit */
  int num_mshrs;		/* number of simult. primary misses allowed */
  int perfect;			/* is cache perfect (missless) or not 
				 * NOTE: not currently used 		*/

  /* miss/replacement handler, read/write BSIZE bytes starting at BADDR
     from/into cache block BLK, returns the latency of the operation
     if initiated at NOW, returned latencies indicate how long it takes
     for the cache access to continue (e.g., fill a write buffer), the
     miss/repl functions are required to track how this operation will
     effect the latency of later operations (e.g., write buffer fills),
     if !BALLOC, then just return the latency; BLK_ACCESS_FN is also
     responsible for generating any user data and incorporating the latency
     of that operation */
  unsigned int					/* latency of block access */
    (*blk_access_fn)(enum mem_cmd cmd,		/* block access command */
		     SS_ADDR_TYPE baddr,	/* program address to access */
		     int bsize,			/* size of the cache block */
		     struct cache_blk *blk,	/* ptr to cache block struct */
		     SS_TIME_TYPE now);		/* when fetch was initiated */

  /* derived data, for fast decoding */
  int hsize;			/* cache set hash table size */
  SS_ADDR_TYPE blk_mask;
  int set_shift;
  SS_ADDR_TYPE set_mask;	/* use *after* shift */
  int tag_shift;
  SS_ADDR_TYPE tag_mask;	/* use *after* shift */
  SS_ADDR_TYPE tagset_mask;	/* used for fast hit detection */

  /* bus resource 
   * NOTE: the bus model assumes only a single, pipelined port to the next
   * level of memory that requires 'bus_interval' cycles per bus
   * transaction.  (0 means perfect bus.)  A cache line is received
   * in a single cycle. */
  SS_TIME_TYPE *bus_free;       /* time when bus to next level of cache is
                                   free.  This is a pointer so several
				   caches may share a bus. */
  int bus_interval;		/* How many cycles per request --  1
				   means one new request every cycle, 2
				   one new request every other cycle, etc. */

  /* mshrs */
  struct mshr *mshrs;		/* soonest-to-retire mshr */

  /* per-cache stats */
  SS_COUNTER_TYPE hits;		/* total number of hits */
  SS_COUNTER_TYPE misses;	/* total number of misses */
  SS_COUNTER_TYPE reads;	/* total number of read accesses */
  SS_COUNTER_TYPE read_hits;	/* total number of reads that are hits */
  SS_COUNTER_TYPE writes;	/* total number of write accesses */
  SS_COUNTER_TYPE replacements;	/* total number of replacements at misses */
  SS_COUNTER_TYPE writebacks;	/* total number of writebacks at misses */
  SS_COUNTER_TYPE invalidations; /* total number of external invalidations */

  SS_COUNTER_TYPE prime_reads;  /* reads during priming */
  SS_COUNTER_TYPE prime_writes; /* reads during priming */
  SS_COUNTER_TYPE prime_hits;   /* reads during priming */
  SS_COUNTER_TYPE prime_misses; /* reads during priming */
  SS_COUNTER_TYPE prime_read_hits; /* reads during priming */
  struct stat_stat_t *lat_dist; /* dist of cache access lat's */

  /* for interval miss rates; compute stat in current interval by taking
   * stat - int_stat, where 'stat' is statistic of interest */
  SS_COUNTER_TYPE int_hits;	/* total number of hits thru last interval */
  SS_COUNTER_TYPE int_misses;	/* total number of misses thru last interval */

  /* last block to hit, used to optimize cache hit processing */
  SS_ADDR_TYPE last_tagset;	/* tag of last line accessed */
  struct cache_blk *last_blk;	/* cache block last accessed */

  /* data blocks */
  char *data;			/* pointer to data blocks allocation */
  struct cache_set sets[1];	/* each entry is a set */
};

/* create and initialize a general cache structure */
struct cache *				/* pointer to cache created */
cache_create(char *name,		/* name of the cache */
	     int nsets,			/* total number of sets in cache */
	     int bsize,			/* block (line) size of cache */
	     int balloc,		/* allocate data space for blocks? */
	     int usize,			/* size of user data to alloc w/blks */
	     int assoc,			/* associativity of cache */
	     enum cache_policy policy,	/* replacement policy w/in sets */
	     /* block access function, see description w/in struct cache def */
	     unsigned int (*blk_access_fn)(enum mem_cmd cmd,
					    SS_ADDR_TYPE baddr, int bsize,
					    struct cache_blk *blk,
					    SS_TIME_TYPE now),
	     unsigned int base_lat,	/* base access latency */
	     unsigned int extra_hit_lat, /* extra latency on a hit */
	     int num_mshrs,		/* number of primary-miss mshr's */
	     int bus_interval);		/* number of cycles per bus xctn */

/* Make a cache behave perfectly; should be called in conjunction with
 * cache_create(); */
void cache_set_perfect(struct cache *cp);

/* Allow caches to share a bus */
void cache_set_bus(struct cache *cp_set, struct cache* cp_target);

/* Update interval stats */
void cache_new_interval(struct cache *cp);

/* parse policy */
enum cache_policy			/* replacement policy enum */
cache_char2policy(char c);		/* replacement policy as a char */

/* print cache configuration */
void
cache_config(struct cache *cp,		/* cache instance */
	     FILE *stream);		/* output stream */

/* register cache stats */
void
cache_reg_stats(struct cache *cp,	/* cache instance */
		struct stat_sdb_t *sdb);/* stats database */

/* print cache stats */
void
cache_stats(struct cache *cp,		/* cache instance */
	    FILE *stream);		/* output stream */

/* print cache stats */
void cache_stats(struct cache *cp, FILE *stream);

/* reset stats after priming, if appropriate */
void cache_after_priming(struct cache *cp);

/* reset state after warmup, if appropriate */
void cache_after_warmup(struct cache *cp);

/* access a cache, perform a CMD operation on cache CP at address ADDR,
   places NBYTES of data at *P, returns latency of operation if initiated
   at NOW, places pointer to block user data in *UDATA, *P is untouched if
   cache blocks are not allocated (!CP->BALLOC), UDATA should be NULL if no
   user data is attached to blocks */
unsigned int				/* latency of access in cycles */
cache_access(struct cache *cp,		/* cache to access */
	     enum mem_cmd cmd,		/* access type, Read or Write */
	     SS_ADDR_TYPE addr,		/* address of access */
	     void *p,			/* ptr to buffer for input/output */
	     int nbytes,		/* number of bytes to access */
	     SS_TIME_TYPE now,		/* time of access */
	     char **udata,		/* for return of user data ptr */
	     SS_ADDR_TYPE *repl_addr);	/* for address of replaced block */

/* cache access functions, these are safe, they check alignment and
   permissions */
#define cache_double(cp, cmd, addr, p, now, udata)	\
  cache_access(cp, cmd, addr, p, sizeof(double), now, udata)
#define cache_float(cp, cmd, addr, p, now, udata)	\
  cache_access(cp, cmd, addr, p, sizeof(float), now, udata)
#define cache_dword(cp, cmd, addr, p, now, udata)	\
  cache_access(cp, cmd, addr, p, sizeof(long long), now, udata)
#define cache_word(cp, cmd, addr, p, now, udata)	\
  cache_access(cp, cmd, addr, p, sizeof(int), now, udata)
#define cache_half(cp, cmd, addr, p, now, udata)	\
  cache_access(cp, cmd, addr, p, sizeof(short), now, udata)
#define cache_byte(cp, cmd, addr, p, now, udata)	\
  cache_access(cp, cmd, addr, p, sizeof(char), now, udata)

/* return non-zero if block containing address ADDR is contained in cache
   CP, this interface is used primarily for debugging and asserting cache
   invariants */
int					/* non-zero if access would hit */
cache_probe(struct cache *cp,		/* cache instance to probe */
	    SS_ADDR_TYPE addr);		/* address of block to probe */

/* flush the entire cache, returns latency of the operation */
unsigned int				/* latency of the flush operation */
cache_flush(struct cache *cp,		/* cache instance to flush */
	    SS_TIME_TYPE now);		/* time of cache flush */

/* flush the block containing ADDR from the cache CP, returns the latency of
   the block flush operation */
unsigned int				/* latency of flush operation */
cache_flush_addr(struct cache *cp,	/* cache instance to flush */
		 SS_ADDR_TYPE addr,	/* address of block to flush */
		 SS_TIME_TYPE now);	/* time of cache flush */

#endif /* CACHE_H */
