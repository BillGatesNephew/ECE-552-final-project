/*
 * cache.c - cache module routines
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
 * $Id: cache.c,v 3.1 1998/04/17 16:51:35 skadron Exp $
 *
 * $Log: cache.c,v $
 * Revision 3.1  1998/04/17 16:51:35  skadron
 * Versions used thru Mar '98 (retstack sub, ruu_resub, ICS final manuscript)
 *
 * Revision 2.102  1998/03/24 18:54:52  skadron
 * Fixed latency measurement to not double-count latencies
 *
 * Revision 2.101  1997/12/09 22:20:31  skadron
 * Version Pritpal used for ISCA-25 submission
 *
 * Revision 2.100  1997/12/03 20:05:46  skadron
 * Version Kevin used for ISCA-25 submission
 *
 * Revision 2.6  1997/09/09 19:04:39  skadron
 * Last changes weren't quite right: using an unisgned int screwed up
 *    MAX, so it's an int again
 *
 * Revision 2.5  1997/09/09 17:32:07  skadron
 * Fixed 'lat': needs to be unsigned, because lat's can conceivably be
 *    large.  Also turned off mshr 'when_free' check for perfect caches.
 *
 * Revision 2.4  1997/08/01 22:40:19  skadron
 * Minor bug fix: writebacks counter wasn't updated in cache_flush() and
 *    cache_flush_addr()
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
 * Revision 1.13  1997/04/17  14:55:15  skadron
 * #ifdef DEBUG'd out the 'last_time' debug var in the for loop
 *
 * Revision 1.12  1997/04/17  02:25:46  skadron
 * Added some fields to cache struct to allow computing interval miss
 * rates
 *
 * Revision 1.11  1997/04/16  16:39:02  skadron
 * Better bus modeling -- fixed interval between bus transactions
 *
 * Revision 1.10  1997/04/15  19:02:30  skadron
 * Changes to account for possible use of warmup-cache
 *
 * Revision 1.9  1997/04/13  17:45:24  skadron
 * Bug fix: can hit on mshrs after all
 *
 * Revision 1.8  1997/04/11  01:26:45  skadron
 * Typo fix (misplaced paren) and cosmetic changes
 *
 * Revision 1.7  1997/04/11  01:03:38  skadron
 * Added mshr's.  Infinite secondary misses allowed.  Also added ability
 *    to report dist of cache latencies, turned on by compiling w/
 *    -DLAT_INFO.  Also stopped charging an extra cycle for writebacks.
 *
 * Revision 1.6  1997/04/09  21:38:09  skadron
 * Changed treatment of cache latencies: each cache now has a base access
 *    time (for probing the tags) plus optional extra latency incurred on
 *    hits.  The major change is that previously, misses were not charged
 *    any time!  Now they're charged the base latency.
 *
 * Revision 1.5  1997/03/27  16:30:01  skadron
 * Fixed bug: sim-outorder calls cache_after_priming() on all caches, even
 *    if they don't exist.  Now we check for a null cp.
 *
 * Revision 1.4  1997/03/25  16:16:12  skadron
 * Statistics now take account of priming: most statistics report only
 *    post-prime info; some report both total and post-prime
 *
 * Revision 1.3  1997/03/02  19:31:12  skadron
 * cosmetic changes to output text
 *
 * Revision 1.2  1997/02/24  18:04:30  skadron
 * Added counters for reads, writes, and read-hits.  Added initialization
 *    of perfect-cache param, but it's not currently used for anything.
 *
 * Revision 1.1  1997/02/16  22:23:54  skadron
 * Initial revision
 *
 * Revision 1.3  1997/01/06  15:56:20  taustin
 * comments updated
 * fixed writeback bug when balloc == FALSE
 * strdup() changed to mystrdup()
 * cache_reg_stats() now works with stats package
 * cp->writebacks stat added to cache
 *
 * Revision 1.1  1996/12/05  18:52:32  taustin
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "misc.h"
#include "ss.h"
#include "sim.h"
#include "cache.h"


/* cache access macros */
#define CACHE_TAG(cp, addr)	((addr) >> (cp)->tag_shift)
#define CACHE_SET(cp, addr)	(((addr) >> (cp)->set_shift) & (cp)->set_mask)
#define CACHE_BLK(cp, addr)	((addr) & (cp)->blk_mask)
#define CACHE_TAGSET(cp, addr)	((addr) & (cp)->tagset_mask)
#define CACHE_MSHRTAG(cp, addr) ((addr) >> (cp)->set_shift)

/* extract/reconstruct a block address */
#define CACHE_BADDR(cp, addr)	((addr) & ~(cp)->blk_mask)
#define CACHE_MK_BADDR(cp, tag, set)					\
  (((tag) << (cp)->tag_shift)|((set) << (cp)->set_shift))

/* index an array of cache blocks, non-trivial due to variable length blocks */
#define CACHE_BINDEX(cp, blks, i)					\
  ((struct cache_blk *)(((char *)(blks)) +				\
			(i)*(sizeof(struct cache_blk) +			\
			     ((cp)->balloc ? (cp)->bsize*sizeof(char) : 0))))

/* cache data block accessor, type parameterized */
#define __CACHE_ACCESS(type, data, bofs)				\
  (*((type *)(((char *)data) + (bofs))))

/* cache data block accessors, by type */
#define CACHE_DOUBLE(data, bofs)  __CACHE_ACCESS(double, data, bofs)
#define CACHE_FLOAT(data, bofs)	  __CACHE_ACCESS(float, data, bofs)
#define CACHE_DWORD(data, bofs)	  __CACHE_ACCESS(long long, data, bofs)
#define CACHE_WORD(data, bofs)	  __CACHE_ACCESS(unsigned int, data, bofs)
#define CACHE_HALF(data, bofs)	  __CACHE_ACCESS(unsigned short, data, bofs)
#define CACHE_BYTE(data, bofs)	  __CACHE_ACCESS(unsigned char, data, bofs)

/* cache block hashing macros, this macro is used to index into a cache
   set hash table (to find the correct block on N in an N-way cache), the
   cache set index function is CACHE_SET, defined above */
#define CACHE_HASH(cp, key)						\
  (((key >> 24) ^ (key >> 16) ^ (key >> 8) ^ key) & ((cp)->hsize-1))

/* copy data out of a cache block to buffer indicated by argument pointer p */
#define CACHE_BCOPY(cmd, blk, bofs, p, nbytes)	\
  if (cmd == Read)							\
    {									\
      switch (nbytes) {							\
      case 1:								\
	*((unsigned char *)p) = CACHE_BYTE(&blk->data[0], bofs); break;	\
      case 2:								\
	*((unsigned short *)p) = CACHE_HALF(&blk->data[0], bofs); break;\
      case 4:								\
	*((unsigned int *)p) = CACHE_WORD(&blk->data[0], bofs); break;	\
      default:								\
	{ /* >= 8, power of two, fits in block */			\
	  int words = nbytes >> 2;					\
	  while (words-- > 0)						\
	    {								\
	      *((unsigned int *)p) = CACHE_WORD(&blk->data[0], bofs);	\
	      p += 4; bofs += 4;					\
	    }\
	}\
      }\
    }\
  else /* cmd == Write */						\
    {									\
      switch (nbytes) {							\
      case 1:								\
	CACHE_BYTE(&blk->data[0], bofs) = *((unsigned char *)p); break;	\
      case 2:								\
        CACHE_HALF(&blk->data[0], bofs) = *((unsigned short *)p); break;\
      case 4:								\
	CACHE_WORD(&blk->data[0], bofs) = *((unsigned int *)p); break;	\
      default:								\
	{ /* >= 8, power of two, fits in block */			\
	  int words = nbytes >> 2;					\
	  while (words-- > 0)						\
	    {								\
	      CACHE_WORD(&blk->data[0], bofs) = *((unsigned int *)p);	\
	      p += 4; bofs += 4;					\
	    }\
	}\
    }\
  }

/* unlink BLK from the hash table bucket chain in SET */
static void
unlink_htab_ent(struct cache *cp,		/* cache to update */
		struct cache_set *set,		/* set containing bkt chain */
		struct cache_blk *blk)		/* block to unlink */
{
  struct cache_blk *prev, *ent;
  int index = CACHE_HASH(cp, blk->tag);

  /* locate the block in the hash table bucket chain */
  for (prev=NULL,ent=set->hash[index];
       ent;
       prev=ent,ent=ent->hash_next)
    {
      if (ent == blk)
	break;
    }
  assert(ent);

  /* unlink the block from the hash table bucket chain */
  if (!prev)
    {
      /* head of hash bucket list */
      set->hash[index] = ent->hash_next;
    }
  else
    {
      /* middle or end of hash bucket list */
      prev->hash_next = ent->hash_next;
    }
  ent->hash_next = NULL;
}

/* insert BLK onto the head of the hash table bucket chain in SET */
static void
link_htab_ent(struct cache *cp,		/* cache to update */
	      struct cache_set *set,	/* set containing bkt chain */
	      struct cache_blk *blk)	/* block to insert */
{
  int index = CACHE_HASH(cp, blk->tag);

  /* insert block onto the head of the bucket chain */
  blk->hash_next = set->hash[index];
  set->hash[index] = blk;
}

/* where to insert a block onto the ordered way chain */
enum list_loc_t { Head, Tail };

/* insert BLK into the order way chain in SET at location WHERE */
static void
update_way_list(struct cache_set *set,	/* set contained way chain */
		struct cache_blk *blk,	/* block to insert */
		enum list_loc_t where)	/* insert location */
{
  /* unlink entry from the way list */
  if (!blk->way_prev && !blk->way_next)
    {
      /* only one entry in list (direct-mapped), no action */
      assert(set->way_head == blk && set->way_tail == blk);
      /* Head/Tail order already */
      return;
    }
  /* else, more than one element in the list */
  else if (!blk->way_prev)
    {
      assert(set->way_head == blk && set->way_tail != blk);
      if (where == Head)
	{
	  /* already there */
	  return;
	}
      /* else, move to tail */
      set->way_head = blk->way_next;
      blk->way_next->way_prev = NULL;
    }
  else if (!blk->way_next)
    {
      /* end of list (and not front of list) */
      assert(set->way_head != blk && set->way_tail == blk);
      if (where == Tail)
	{
	  /* already there */
	  return;
	}
      set->way_tail = blk->way_prev;
      blk->way_prev->way_next = NULL;
    }
  else
    {
      /* middle of list (and not front or end of list) */
      assert(set->way_head != blk && set->way_tail != blk);
      blk->way_prev->way_next = blk->way_next;
      blk->way_next->way_prev = blk->way_prev;
    }

  /* link BLK back into the list */
  if (where == Head)
    {
      /* link to the head of the way list */
      blk->way_next = set->way_head;
      blk->way_prev = NULL;
      set->way_head->way_prev = blk;
      set->way_head = blk;
    }
  else if (where == Tail)
    {
      /* link to the tail of the way list */
      blk->way_prev = set->way_tail;
      blk->way_next = NULL;
      set->way_tail->way_next = blk;
      set->way_tail = blk;
    }
  else
    panic("bogus WHERE designator");
}

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
	     int bus_interval)		/* number of cycles per bus xctn */
{
  struct cache *cp;
  struct cache_blk *blk;
  int i, j, bindex;

  /* check all cache parameters */
  if (nsets <= 0)
    fatal("cache size (in sets) `%d' must be non-zero", nsets);
  if ((nsets & (nsets-1)) != 0)
    fatal("cache size (in sets) `%d' is not a power of two", nsets);
  /* blocks must be at least one datum large, i.e., 8 bytes for SS */
  if (bsize < 8)
    fatal("cache block size (in bytes) `%d' must be 8 or greater", bsize);
  if ((bsize & (bsize-1)) != 0)
    fatal("cache block size (in bytes) `%d' must be a power of two", bsize);
  if (usize < 0)
    fatal("user data size (in bytes) `%d' must be a positive value", usize);
  if (assoc <= 0)
    fatal("cache associativity `%d' must be non-zero and positive", assoc);
  if ((assoc & (assoc-1)) != 0)
    fatal("cache associativity `%d' must be a power of two", assoc);
  if (!blk_access_fn)
    fatal("must specify miss/replacement functions");

  /* Current mshr implementation doesn't work when user data is maintained
   * by cache module.  Num_mshrs must therefore be 0 (which really means
   * infinite mshrs */
  if (balloc && num_mshrs)
    fatal("can't specify 'balloc' and finite number of mshrs together");

  /* allocate the cache structure */
  cp = (struct cache *)
    calloc(1, sizeof(struct cache) + (nsets-1)*sizeof(struct cache_set));
  if (!cp)
    fatal("out of virtual memory");

  /* initialize user parameters */
  cp->name = mystrdup(name);
  cp->nsets = nsets;
  cp->bsize = bsize;
  cp->balloc = balloc;
  cp->usize = usize;
  cp->assoc = assoc;
  cp->policy = policy;
  cp->base_lat = base_lat;
  cp->extra_hit_lat = extra_hit_lat;
  cp->num_mshrs = num_mshrs;
  cp->bus_free = calloc(1, sizeof(SS_TIME_TYPE));
  cp->bus_interval = bus_interval;
  cp->perfect = FALSE;

  /* miss/replacement functions */
  cp->blk_access_fn = blk_access_fn;

  /* compute derived parameters */
  cp->hsize = CACHE_HIGHLY_ASSOC(cp) ? (assoc >> 2) : 0;
  cp->blk_mask = bsize-1;
  cp->set_shift = log_base2(bsize);
  cp->set_mask = nsets-1;
  cp->tag_shift = cp->set_shift + log_base2(nsets);
  cp->tag_mask = (1 << (32 - cp->tag_shift))-1;
  cp->tagset_mask = ~cp->blk_mask;

  /* print derived parameters during debug */
  debug("%s: cp->hsize     = %d", cp->hsize);
  debug("%s: cp->blk_mask  = 0x%08x", cp->blk_mask);
  debug("%s: cp->set_shift = %d", cp->set_shift);
  debug("%s: cp->set_mask  = 0x%08x", cp->set_mask);
  debug("%s: cp->tag_shift = %d", cp->tag_shift);
  debug("%s: cp->tag_mask  = 0x%08x", cp->tag_mask);

  /* initialize cache stats */
  cp->hits = 0;
  cp->misses = 0;
  cp->reads = 0;
  cp->writes = 0;
  cp->read_hits = 0;
  cp->replacements = 0;
  cp->writebacks = 0;
  cp->invalidations = 0;

  /* initialize interval stats */
  cp->int_hits = 0;
  cp->int_misses = 0;

  /* blow away the last block accessed */
  cp->last_tagset = 0;
  cp->last_blk = NULL;

  /* allocate data blocks */
  cp->data = (char *)calloc(nsets * assoc,
			    sizeof(struct cache_blk) +
			    (cp->balloc ? (bsize*sizeof(char)) : 0));
  if (!cp->data)
    fatal("out of virtual memory");

  /* slice up the data blocks */
  for (bindex=0,i=0; i<nsets; i++)
    {
      cp->sets[i].way_head = NULL;
      cp->sets[i].way_tail = NULL;
      /* get a hash table, if needed */
      if (cp->hsize)
	{
	  cp->sets[i].hash =
	    (struct cache_blk **)calloc(cp->hsize, sizeof(struct cache_blk *));
	  if (!cp->sets[i].hash)
	    fatal("out of virtual memory");
	}
      /* NOTE: all the blocks in a set *must* be allocated contiguously,
	 otherwise, block accesses through SET->BLKS will fail (used
	 during random replacement selection) */
      cp->sets[i].blks = CACHE_BINDEX(cp, cp->data, bindex);
      
      /* link the data blocks into ordered way chain and hash table bucket
         chains, if hash table exists */
      for (j=0; j<assoc; j++)
	{
	  /* locate next cache block */
	  blk = CACHE_BINDEX(cp, cp->data, bindex);
	  bindex++;

	  /* invalidate new cache block */
	  blk->status = 0;
	  blk->tag = 0;
	  blk->ready = 0;
	  blk->user_data = usize ? calloc(usize, sizeof(char)) : NULL;

	  /* insert cache block into set hash table */
	  if (cp->hsize)
	    link_htab_ent(cp, &cp->sets[i], blk);

	  /* insert into head of way list, order is arbitrary at this point */
	  blk->way_next = cp->sets[i].way_head;
	  blk->way_prev = NULL;
	  if (cp->sets[i].way_head)
	    cp->sets[i].way_head->way_prev = blk;
	  cp->sets[i].way_head = blk;
	  if (!cp->sets[i].way_tail)
	    cp->sets[i].way_tail = blk;
	}
    }

  /* allocate the mshr's */
  if (cp->num_mshrs != 0)
    {
      cp->mshrs = (struct mshr *)calloc(cp->num_mshrs, sizeof(struct mshr));
      if (!cp->mshrs)
	fatal("out of virtual memory");

      /* Chain the mshr's together */
      for (j = 0; j < cp->num_mshrs; j++)
	{
	  if (j == cp->num_mshrs - 1)
	    cp->mshrs[j].next = NULL;
	  else
	    cp->mshrs[j].next = &(cp->mshrs[j+1]);
	}
    }
  else
    cp->mshrs = NULL;
  
  return cp;
}

void cache_set_perfect(struct cache *cp)
{
  cp->perfect = TRUE;
}

void cache_set_bus(struct cache *cp_set, struct cache* cp_target)
{
  cp_set->bus_free = cp_target->bus_free;
}

/* Update interval stats */
void cache_new_interval(struct cache *cp)
{
  cp->int_hits = cp->hits;
  cp->int_misses = cp->misses;
}

/* parse policy */
enum cache_policy			/* replacement policy enum */
cache_char2policy(char c)		/* replacement policy as a char */
{
  switch (c) {
  case 'l': return LRU;
  case 'r': return Random;
  case 'f': return FIFO;
  default: fatal("bogus replacement policy, `%c'", c);
  }
}

/* print cache configuration */
void
cache_config(struct cache *cp,		/* cache instance */
	     FILE *stream)		/* output stream */
{
  fprintf(stream,
	  "cache: %s: %d sets, %d byte blocks, %d bytes user data/block\n",
	  cp->name, cp->nsets, cp->bsize, cp->usize);
  fprintf(stream,
	  "cache: %s: %d-way, `%s' replacement policy, write-back\n",
	  cp->name, cp->assoc,
	  cp->policy == LRU ? "LRU"
	  : cp->policy == Random ? "Random"
	  : cp->policy == FIFO ? "FIFO"
	  : (abort(), ""));
}

/* register cache stats */
void
cache_reg_stats(struct cache *cp,	/* cache instance */
		struct stat_sdb_t *sdb)	/* stats database */
{
  char buf[512], buf1[512], *name;

  /* get a name for this cache */
  if (!cp->name || !cp->name[0])
    name = "<unknown>";
  else
    name = cp->name;

  sprintf(buf, "%s.accesses", name);
  sprintf(buf1, "%s.hits + %s.misses", name, name);
  stat_reg_formula(sdb, buf, "total number of accesses", buf1, "%12.0f");
  sprintf(buf, "%s.reads", name);
  stat_reg_llong(sdb, buf, "number of read accesses", &cp->reads, 0, NULL);
  sprintf(buf, "%s.writes", name);
  stat_reg_llong(sdb, buf, "number of write accesses", &cp->writes, 0, NULL);
  sprintf(buf, "%s.hits", name);
  stat_reg_llong(sdb, buf, "total number of hits", &cp->hits, 0, NULL);
  sprintf(buf, "%s.misses", name);
  stat_reg_llong(sdb, buf, "total number of misses", &cp->misses, 0, NULL);
  sprintf(buf, "%s.read_hits", name);
  stat_reg_llong(sdb, buf, "total number of read hits", 
		 &cp->read_hits, 0, NULL);

  sprintf(buf, "%s.accesses.PP", name);
  sprintf(buf1, "%s.hits.PP + %s.misses.PP", name, name);
  stat_reg_formula(sdb, buf, "total number of accesses", buf1, "%12.0f");
  sprintf(buf, "%s.reads.PP", name);
  sprintf(buf1, "%s.reads - %s.prime_reads", name, name);
  stat_reg_formula(sdb, buf, "number of read accesses", buf1, "%12.0f");
  sprintf(buf, "%s.writes.PP", name);
  sprintf(buf1, "%s.writes - %s.prime_writes", name, name);
  stat_reg_formula(sdb, buf, "number of write accesses", buf1, "%12.0f");
  sprintf(buf, "%s.hits.PP", name);
  sprintf(buf1, "%s.hits - %s.prime_hits", name, name);
  stat_reg_formula(sdb, buf, "total number of hits", buf1, "%12.0f");
  sprintf(buf, "%s.misses.PP", name);
  sprintf(buf1, "%s.misses - %s.prime_misses", name, name);
  stat_reg_formula(sdb, buf, "total number of misses", buf1, "%12.0f");
  sprintf(buf, "%s.read_hits.PP", name);
  sprintf(buf1, "%s.read_hits - %s.prime_read_hits", name, name);
  stat_reg_formula(sdb, buf, "total number of read hits", buf1, "%12.0f");

  sprintf(buf, "%s.prime_reads", name);
  stat_reg_llong(sdb, buf, "number of read accesses during priming", 
		 &cp->prime_reads, 0, NULL);
  sprintf(buf, "%s.prime_writes", name);
  stat_reg_llong(sdb, buf, "number of write accesses during priming", 
		 &cp->prime_writes, 0, NULL);
  sprintf(buf, "%s.prime_hits", name);
  stat_reg_llong(sdb, buf, "number of hits during priming", 
		 &cp->prime_hits, 0, NULL);
  sprintf(buf, "%s.prime_misses", name);
  stat_reg_llong(sdb, buf, "number of misses during priming", 
		 &cp->prime_misses, 0, NULL);
  sprintf(buf, "%s.prime_read_hits", name);
  stat_reg_llong(sdb, buf, "number of read hits during priming", 
		 &cp->prime_read_hits, 0, NULL);

  sprintf(buf, "%s.replacements.PP", name);
  stat_reg_llong(sdb, buf, "total number of replacements",
		 &cp->replacements, 0, NULL);
  sprintf(buf, "%s.writebacks.PP", name);
  stat_reg_llong(sdb, buf, "total number of writebacks",
		 &cp->writebacks, 0, NULL);
  sprintf(buf, "%s.invalidations.PP", name);
  stat_reg_llong(sdb, buf, "total number of invalidations",
		 &cp->invalidations, 0, NULL);
  sprintf(buf, "%s.miss_rate", name);
  sprintf(buf1, "%s.misses / %s.accesses", name, name);
  stat_reg_formula(sdb, buf, "miss rate (i.e., misses/ref)", buf1, NULL);
  sprintf(buf, "%s.miss_rate.PP", name);
  sprintf(buf1, "%s.misses.PP / %s.accesses.PP", name, name);
  stat_reg_formula(sdb, buf, "miss rate (i.e., misses/ref)", buf1, NULL);
  sprintf(buf, "%s.repl_rate.PP", name);
  sprintf(buf1, "%s.replacements.PP / %s.accesses.PP", name, name);
  stat_reg_formula(sdb, buf, "replacement rate (i.e., repls/ref)", buf1, NULL);
  sprintf(buf, "%s.wb_rate.PP", name);
  sprintf(buf1, "%s.writebacks.PP / %s.accesses.PP", name, name);
  stat_reg_formula(sdb, buf, "writeback rate (i.e., wrbks/ref)", buf1, NULL);
  sprintf(buf, "%s.inv_rate.PP", name);
  sprintf(buf1, "%s.invalidations.PP / %s.accesses.PP", name, name);
  stat_reg_formula(sdb, buf, "invalidation rate (i.e., invs/ref)", buf1, NULL);
#ifdef LAT_INFO
  /* Cache access latency info */
  sprintf(buf, "%s.lat_dist.PP", name);
  cp->lat_dist = stat_reg_dist(sdb, buf, "Cache access latencies",
			       /* init */0, /* arr sz */1024, /* bucket sz */1,
			       (PF_COUNT | PF_PDF | PF_CDF), NULL, NULL, NULL);
#endif
}

/* print cache stats */
void
cache_stats(struct cache *cp,		/* cache instance */
	    FILE *stream)		/* output stream */
{
  double sum = (double)(cp->hits + cp->misses);

  fprintf(stream,
	  "cache: %s: %.0f hits %.0f misses %.0f repls %.0f invalidations\n",
	  cp->name, (double)cp->hits, (double)cp->misses,
	  (double)cp->replacements, (double)cp->invalidations);
  fprintf(stream,
	  "cache: %s: miss rate=%f  repl rate=%f  invalidation rate=%f\n",
	  cp->name,
	  (double)cp->misses/sum, (double)(double)cp->replacements/sum,
	  (double)cp->invalidations/sum);
}

void cache_after_priming(struct cache *cp)
{
  if (cp == NULL)
    return;

  cp->prime_reads = cp->reads;
  cp->prime_writes = cp->writes;
  cp->prime_hits = cp->hits;
  cp->prime_misses = cp->misses;
  cp->prime_read_hits = cp->read_hits;
  cp->replacements = 0;
  cp->writebacks = 0;
  cp->invalidations = 0;
}

/* warmup doesn't use time, so the cache times get trashed.  reset. */
void cache_after_warmup(struct cache *cp)
{
  int i, j, bindex;
  struct cache_blk *blk;

  struct mshr *mshr_ptr;

  if (cp == NULL)
    return;

  *cp->bus_free = 0;

  /* mshr's */
  for (mshr_ptr = cp->mshrs, i = 0; 
       i < cp->num_mshrs; 
       mshr_ptr = mshr_ptr->next, i++)
    {
      mshr_ptr->when_free = 0;
      mshr_ptr->valid = 0;
    }

  /* cache blocks */
  for (bindex=0, i=0; i < cp->nsets; i++)
    {
      for (j=0; j < cp->assoc; j++)
	{
	  /* locate next cache block */
	  blk = CACHE_BINDEX(cp, cp->data, bindex);
	  bindex++;

	  /* invalidate new cache block */
	  blk->ready = 0;
	}
    }
}


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
	     SS_ADDR_TYPE *repl_addr)	/* for address of replaced block */
{
  SS_ADDR_TYPE tag = CACHE_TAG(cp, addr);
  SS_ADDR_TYPE set = CACHE_SET(cp, addr);
  SS_ADDR_TYPE bofs = CACHE_BLK(cp, addr);
  SS_ADDR_TYPE mshrtag = CACHE_MSHRTAG(cp, addr);
  struct cache_blk *blk = NULL, *repl = NULL;
  unsigned int lat = cp->base_lat;	/* Every access incurs the base lat */
  SS_TIME_TYPE curr_time = now + cp->base_lat;
  struct mshr *mshr_ptr = NULL;
  int i;
#ifndef __alpha__
  extern long random(void);
#endif

  /* default replacement address */
  if (repl_addr)
    *repl_addr = 0;

  /* check alignments */
  if ((nbytes & (nbytes-1)) != 0 || (addr & (nbytes-1)) != 0)
    fatal("cache: access error: bad size or alignment, addr 0x%08x", addr);

  /* access must fit in cache block */
  if ((addr + nbytes) > ((addr & ~cp->blk_mask) + cp->bsize))
    fatal("cache: access error: access spans block, addr 0x%08x", addr);

  /* Update read/write counts */
  if (cmd == Read)
    cp->reads++;
  else
    cp->writes++;

  /* permissions are checked on cache misses */

  /* check for a fast hit: access to same block */
  if (CACHE_TAGSET(cp, addr) == cp->last_tagset)
    {
      /* hit in the same block */
      blk = cp->last_blk;
      goto cache_all_true_hits;
    }
    
  if (cp->hsize)
    {
      /* high-associativity cache, access through the per-set hash tables */
      int hindex = CACHE_HASH(cp, tag);

      for (blk=cp->sets[set].hash[hindex];
	   blk;
	   blk=blk->hash_next)
	{
	  if (blk->tag == tag && (blk->status & CACHE_BLK_VALID))
	    goto cache_hit;
	}
    }
  else
    {
      /* low-associativity cache, linear search the way list */
      for (blk=cp->sets[set].way_head;
	   blk;
	   blk=blk->way_next)
	{
	  if (blk->tag == tag && (blk->status & CACHE_BLK_VALID))
	    goto cache_hit;
	}
    }

  /* cache block not found */

  /* **MISS** */
  cp->misses++;

  /* Check mshrs for reads */
  if (cp->num_mshrs != 0)
    {
#ifndef NDEBUG
      SS_TIME_TYPE last_time = 0;
#endif
      /* See if an mshr has been allocated for this block.  If not, 
       * wait for the head of the mshr list to get free (list is sorted). */
      for (mshr_ptr = cp->mshrs, i = 0; 
	   i < cp->num_mshrs; 
#ifndef NDEBUG
	   last_time = mshr_ptr->when_free, 
#endif
	     mshr_ptr = mshr_ptr->next, i++)
	{
	  if (mshr_ptr->tag == mshrtag && 
	      mshr_ptr->when_free > now && mshr_ptr->valid)
	    {
	      /* An mshr-hit!  Normal mshr-hits are hits as above.
	       * We got here because the hit-upon block has already
	       * been replaced.  But we should treat this like a hit. */
	      lat = MAX(0, mshr_ptr->when_free - curr_time);
	      curr_time += lat;
	      goto mshr_hit;
	    }
#ifndef NDEBUG
	  if (mshr_ptr->when_free < last_time && !sim_warmup && !cp->perfect)
	    panic("mshr ordering broken");
#endif
	}

      lat = MAX(0, (int)(cp->mshrs->when_free - curr_time));
      curr_time += lat;
      mshr_ptr = cp->mshrs;
    }
  else 
    mshr_ptr = NULL;

  /* select the appropriate block to replace, and re-link this entry to
     the appropriate place in the way list */
  switch (cp->policy) {
  case LRU:
  case FIFO:
    repl = cp->sets[set].way_tail;
    update_way_list(&cp->sets[set], repl, Head);
    break;
  case Random:
    {
#if defined(__CYGWIN32__) || defined(hpux) || defined(__hpux) \
    || defined(__svr4__)
      int bindex = rand() & (cp->assoc - 1);
#else
      int bindex = random() & (cp->assoc - 1);
#endif
      repl = CACHE_BINDEX(cp, cp->sets[set].blks, bindex);
    }
    break;
  default:
    panic("bogus replacement policy");
  }

  /* remove this block from the hash bucket chain, if hash exists */
  if (cp->hsize)
    unlink_htab_ent(cp, &cp->sets[set], repl);

  /* blow away the last block to hit */
  cp->last_tagset = 0;
  cp->last_blk = NULL;

  /* write back replaced block data -- FIXME: note infinite write buffering */
  if (repl->status & CACHE_BLK_VALID)
    {
      cp->replacements++;

      if (repl_addr)
	*repl_addr = CACHE_MK_BADDR(cp, repl->tag, set);

      /* don't replace the block until outstanding misses are satisfied */
      lat = MAX(0, (int)(repl->ready - curr_time));
      curr_time += lat;

      if (repl->status & CACHE_BLK_DIRTY)
	{
	  /* write back the cache block -- note infinite write buffering */
	  cp->writebacks++;
	  (void)cp->blk_access_fn(Write,
				  CACHE_MK_BADDR(cp, repl->tag, set),
				  cp->bsize, repl, curr_time);
	}
    }
  
  /* stall until the bus to next level of memory is available */
  assert(cp->bus_interval > 0 || *cp->bus_free == 0);
  lat = MAX(0, (int)(*cp->bus_free - curr_time));
  curr_time += lat;
 
  /* track bus resource usage */
  if (cp->bus_interval > 0)
    *cp->bus_free = MAX(*cp->bus_free, curr_time) + cp->bus_interval;

  /* update block tags */
  repl->tag = tag;
  repl->status = CACHE_BLK_VALID;	/* dirty bit set on update */

  /* read data block */
  lat = cp->blk_access_fn(Read, CACHE_BADDR(cp, addr), cp->bsize,
			   repl, curr_time);
  curr_time += lat;

  /* copy data out of cache block */
  if (cp->balloc)
    {
      CACHE_BCOPY(cmd, repl, bofs, p, nbytes);
    }

  /* update dirty status */
  if (cmd == Write)
    repl->status |= CACHE_BLK_DIRTY;

  /* get user block data, if requested and it exists */
  if (udata)
    *udata = repl->user_data;

  /* update block status */
  repl->ready = curr_time;

  /* link this entry back into the hash table */
  if (cp->hsize)
    link_htab_ent(cp, &cp->sets[set], repl);

#ifdef LAT_INFO
  if (done_priming)
    stat_add_sample(cp->lat_dist, (curr_time - now));
#endif

  /* allocate the mshr to this miss */
  if (mshr_ptr && (mshr_ptr->tag != mshrtag || mshr_ptr->when_free <= now))
    {
      /* Have a primary miss, here, and so mshr_ptr == cp->mshrs */
      struct mshr *prev, *curr, *new_head;
      new_head = cp->mshrs->next;

      mshr_ptr->tag = mshrtag;
      mshr_ptr->valid = TRUE;
      mshr_ptr->when_free = curr_time;
      /* Note mshr is available at time 'when_free', but isn't actually
       * freed til needed */

      /* Insert new mshr entry into sorted mshr-list */
      for (prev = NULL, curr = cp->mshrs->next, i = 0; 
	   curr != NULL; 
	   i++, prev = curr, curr = curr->next)
	{
	  if (mshr_ptr->when_free < curr->when_free)
	    break;
	}
      
      if (curr == NULL && prev == NULL)
	;/* Only one mshr; leave item at head */
      else if (curr == NULL)
	{
	  /* Insert at tail */
	  cp->mshrs = new_head;
	  prev->next = mshr_ptr;
	  mshr_ptr->next = NULL;
	}
      else if (prev == NULL)
	;/* Multiple mshrs; Leave item at head */
      else /* (i > 0 && i < cp->num_mshrs-1) */
	{
	  /* Insert in middle */
	  cp->mshrs = new_head;
	  mshr_ptr->next = curr;
	  prev->next = mshr_ptr;
	}
    }

  return (curr_time - now);

 mshr_hit: /* mshr-hit handler */

 /* **MSHR HIT** */
  /* blow away the last block to hit */
  cp->last_tagset = 0;
  cp->last_blk = NULL;

  goto cache_all_hits;

 cache_hit: /* slow hit handler */
  
 /* **HIT** */
  /* if LRU replacement and this is not the first element of list, reorder */
  if (blk->way_prev && cp->policy == LRU)
    {
      /* move this block to head of the way (MRU) list */
      update_way_list(&cp->sets[set], blk, Head);
    }

 cache_all_true_hits: /* work done for all "true hits" (ie not mshr hits) */

 /* **TRUE HITS** */

  /* tag is unchanged, so hash links (if they exist) are still valid */
  /* for fast hits, we don't need to change the way list */

  /* copy data out of cache block, if block exists */
  if (cp->balloc)
    {
      CACHE_BCOPY(cmd, blk, bofs, p, nbytes);
    }

  /* get user block data, if requested and it exists */
  if (udata)
    *udata = blk->user_data;

  /* update dirty status */
  if (cmd == Write)
    blk->status |= CACHE_BLK_DIRTY;

  /* record the last block to hit */
  cp->last_tagset = CACHE_TAGSET(cp, addr);
  cp->last_blk = blk;

 cache_all_hits:  /* common portion of hit handler, incl. mshr hits */

 /* **ALL HITS** */
  cp->hits++;
  if (cmd == Read)
    cp->read_hits++;

  /* for mshr hits, the block's already been replaced -- we're sort
   * of looking back in time -- so don't change hash links, way list, 
   * or dirty status */

  /* return first cycle data is available to access */
  lat = cp->extra_hit_lat;
  curr_time += lat;
  lat = MAX((int)(curr_time - now), (blk ? (int)(blk->ready - now) : 0));

#ifdef LAT_INFO
  if (done_priming)
    stat_add_sample(cp->lat_dist, lat);
#endif

  return lat;
}

/* return non-zero if block containing address ADDR is contained in cache
   CP, this interface is used primarily for debugging and asserting cache
   invariants */
int					/* non-zero if access would hit */
cache_probe(struct cache *cp,		/* cache instance to probe */
	    SS_ADDR_TYPE addr)		/* address of block to probe */
{
  SS_ADDR_TYPE tag = CACHE_TAG(cp, addr);
  SS_ADDR_TYPE set = CACHE_SET(cp, addr);
  struct cache_blk *blk;

  /* permissions are checked on cache misses */

  if (cp->hsize)
  {
    /* higly-associativity cache, access through the per-set hash tables */
    int hindex = CACHE_HASH(cp, tag);
    
    for (blk=cp->sets[set].hash[hindex];
	 blk;
	 blk=blk->hash_next)
    {	
      if (blk->tag == tag && (blk->status & CACHE_BLK_VALID))
	  return TRUE;
    }
  }
  else
  {
    /* low-associativity cache, linear search the way list */
    for (blk=cp->sets[set].way_head;
	 blk;
	 blk=blk->way_next)
    {
      if (blk->tag == tag && (blk->status & CACHE_BLK_VALID))
	  return TRUE;
    }
  }
  
  /* cache block not found */
  return FALSE;
}

/* flush the entire cache, returns latency of the operation */
unsigned int				/* latency of the flush operation */
cache_flush(struct cache *cp,		/* cache instance to flush */
	    SS_TIME_TYPE now)		/* time of cache flush */
{
  int i, lat = cp->base_lat; /* min latency to probe cache */
  struct cache_blk *blk;

  /* blow away the last block to hit */
  cp->last_tagset = 0;
  cp->last_blk = NULL;

  /* no way list updates required because all blocks are being invalidated */
  for (i=0; i<cp->nsets; i++)
    {
      for (blk=cp->sets[i].way_head; blk; blk=blk->way_next)
	{
	  if (blk->status & CACHE_BLK_VALID)
	    {
	      cp->invalidations++;
	      blk->status &= ~CACHE_BLK_VALID;

	      if (blk->status & CACHE_BLK_DIRTY)
		{
		  /* write back the invalidated block */
		  cp->writebacks++;
		  lat += cp->blk_access_fn(Write,
					   CACHE_MK_BADDR(cp, blk->tag, i),
					   cp->bsize, blk, now+lat);
		}
	    }
	}
    }

  /* return latency of the flush operation */
  return lat;
}

/* flush the block containing ADDR from the cache CP, returns the latency of
   the block flush operation */
unsigned int				/* latency of flush operation */
cache_flush_addr(struct cache *cp,	/* cache instance to flush */
		 SS_ADDR_TYPE addr,	/* address of block to flush */
		 SS_TIME_TYPE now)	/* time of cache flush */
{
  SS_ADDR_TYPE tag = CACHE_TAG(cp, addr);
  SS_ADDR_TYPE set = CACHE_SET(cp, addr);
  struct cache_blk *blk;
  int lat = cp->base_lat; /* min latency to probe cache */

  if (cp->hsize)
    {
      /* higly-associativity cache, access through the per-set hash tables */
      int hindex = CACHE_HASH(cp, tag);

      for (blk=cp->sets[set].hash[hindex];
	   blk;
	   blk=blk->hash_next)
	{
	  if (blk->tag == tag && (blk->status & CACHE_BLK_VALID))
	    break;
	}
    }
  else
    {
      /* low-associativity cache, linear search the way list */
      for (blk=cp->sets[set].way_head;
	   blk;
	   blk=blk->way_next)
	{
	  if (blk->tag == tag && (blk->status & CACHE_BLK_VALID))
	    break;
	}
    }

  if (blk)
    {
      cp->invalidations++;
      blk->status &= ~CACHE_BLK_VALID;

      /* blow away the last block to hit */
      cp->last_tagset = 0;
      cp->last_blk = NULL;

      if (blk->status & CACHE_BLK_DIRTY)
	{
	  /* write back the invalidated block */
	  cp->writebacks++;
	  lat += cp->blk_access_fn(Write,
				   CACHE_MK_BADDR(cp, blk->tag, set),
				   cp->bsize, blk, now+lat);
	}
      /* move this block to tail of the way (LRU) list */
      update_way_list(&cp->sets[set], blk, Tail);
    }

  /* return latency of the operation */
  return lat;
}
