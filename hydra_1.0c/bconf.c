/*
 * bconf.c - branch-confidence predictor routines
 *
 * This file is written by Pritpal S. Ahuja, Copyright (C) 1998, 1999.
 * For more information, contact skadron@cs.princeton.edu
 *
 * $Id: bconf.c,v 3.1 1998/04/17 16:51:35 skadron Exp $
 *
 * $Log: bconf.c,v $
 * Revision 3.1  1998/04/17 16:51:35  skadron
 * Versions used thru Mar '98 (retstack sub, ruu_resub, ICS final manuscript)
 *
 * Revision 2.104  1998/04/17 16:31:36  skadron
 * minor fix: malloc -> calloc to get more correct memory behavior
 *
 * Revision 2.103  1998/03/24 18:54:30  skadron
 * Added "output_dist" option
 *
 * Revision 2.102  1997/12/10 20:15:35  skadron
 * Cosmetic changes
 *
 * Revision 2.101  1997/12/09 22:20:31  skadron
 * Version Pritpal used for ISCA-25 submission
 *
 * Revision 2.10  1997/11/22 18:21:23  psa
 * More bug fixes.
 *
 * Revision 2.9  1997/11/21 04:16:51  psa
 * Added profile-based and adpative conf predictors.
 * Many bug fixes.
 *
 * Revision 2.8  1997/09/21 19:35:31  skadron
 * Added -bconf:gshare
 *
 * Revision 2.7  1997/09/21 15:23:09  skadron
 * Even further fixed masking out bits in lookup; added counter for
 *    number of "very low confidence" predictions
 *
 * Revision 2.6  1997/09/18 21:13:58  skadron
 * Further fixed masking out bits in lookup
 *
 * Revision 2.5  1997/09/15 21:25:41  skadron
 * 1. Bug fix: using mod on the table value for Ones was giving too few bits
 * 2. Cleaned up the code a little bit
 * 3. Added RCS headers
 *
 */

#include <assert.h>
#include <malloc.h>
#include <stdio.h>

#include "misc.h"
#include "bconf.h" 

struct stat_stat_t *bconf_correct_dist = NULL;
struct stat_stat_t *bconf_incorrect_dist = NULL;

static FILE *config_file = NULL;

#ifdef BCONF_DUMP
extern char *outfile_name;
static FILE *dump_file = NULL;


FILE *
bconf_open_dump_file(char *fname)
{
  char str[20];
  FILE *file;
  
  sprintf(str, "%s_%s", fname, outfile_name);
  file = fopen(str, "w");
  assert(file);
  return file;
}
#endif

struct bconf *
bconf_create(bconf_type_enum type,	/* desired bconf type */
	     bconf_selector_enum selector, /* type of threshold selector? */
	     int table_size,		/* how many entries in the table */
	     int entry_size,		/* how many bits per entry */
	     int num_thresholds,
	     int *thresholds,		/* how to interpret entries */
	     int bconf_gshare,		/* combine history and address bits? */
	     int history_update,	/* use bcf hist to update bcf entry? */
	     int output_dist,		/* print correct/incorrect dist's? */
	     char *config_file_name)	/* optional config file */
{
  int i;
  struct bconf *bc;
  int free_entries=0; 

#ifdef BCONF_DUMP
  if (dump_file == NULL)
    dump_file = bconf_open_dump_file("conf");
#endif

  bc = calloc(1, sizeof(struct bconf));
  if (!bc)
    fatal("couldn't allocate bconf structure");
  
  bc->type = type;
  bc->selector = selector;

  if (selector == BTS_Profile)
    {
      config_file = fopen(config_file_name, "r");
      if (config_file == NULL)
	fatal("cannot open file '%s'", config_file_name);
      
      i=0;
      while (!feof(config_file))
	{
	  if (!free_entries)
	    {
	      bc->forks = realloc(bc->forks, 
				  (i+20) * sizeof(struct profile_entry));
	      assert(bc->forks);
	      free_entries = 20;
	    }
      
	  fscanf(config_file, "%u %d", 
		 &(bc->forks[i].addr), &(bc->forks[i].class));

	  if (feof(config_file))
	    break;
	  
	  free_entries--;
	  i++;
	}

      bc->num_forks = i; 

#ifdef BCONF_DUMP
      for (--i; i>=0; i--)
	fprintf(dump_file, "fork addr = 0x%x, class = %d\n", 
		bc->forks[i].addr, bc->forks[i].class);

      fprintf(dump_file, "-------\n\n");
      
#endif

      if (type == BCF_None)
	return bc;
    }
  
  bc->table_size = table_size;
  if (history_update)
   panic("We no longer allow bconf history to be updated by bconf prediction");
  bc->history_update = history_update;
  bc->entry_size = entry_size;
  bc->num_thresholds = num_thresholds;

  if (num_thresholds > 1 && selector == BTS_None)
    fatal("There must be a threshold selector if there is more"
	  "than one threshold");

  bc->thresholds = calloc(num_thresholds, sizeof(int));
  assert(bc->thresholds);
  
  for (i=0; i<num_thresholds; i++)
    bc->thresholds[i] = thresholds[i];
  
  bc->gbhr = 0;
  bc->gshare = bconf_gshare;
  bc->table = calloc(table_size, sizeof(unsigned int));
  if (!bc->table)
    fatal("couldn't allocate bconf table");
  
  switch (type)
    {
    case BCF_Ones:
      bc->min = 0;
      bc->max = (1<<entry_size) - 1;
      
      for (i=0; i<table_size; i++)
	bc->table[i] = 0;
      break;
      
    case BCF_Sat:
      bc->min = -(1 << (entry_size-1));
      bc->max = -(1 + bc->min);
      
      for (i=0; i<table_size; i++)
	bc->table[i] = 0;
      break;
      
    case BCF_Reset:
      bc->min = 0;
      bc->max = (1<<entry_size) - 1;
      break;
      
    default:
      panic("illegal bconf type");
    }
  
  bc->output_dist = output_dist;
  return bc;
}

int
bconf_lookup(struct bconf *bc, 
	     SS_ADDR_TYPE addr,		/* branch address to look up */
	     int free_contexts,		/* num of free forking contexts */
	     int *conf_data)		/* data returned by the conf 
					   predictor for statistics  */
{
  int i, idx, bits, val;
  int retval;
  int class = -1;
  int thresh;
  
  assert(bc);

  if (bc->selector == BTS_Profile)
    {
      /* Scan through list of branches, searching for a match */
      for (i=0; i<bc->num_forks; i++)
	if (addr == bc->forks[i].addr)
	  {
	    /* Branch found -- obtain its class */
	    class = bc->forks[i].class;
	    break;
	  }

      /* If conf predictor is profile only, then make forking decision
       based on # free contexts */
      if (bc->type == BCF_None)
	{
	  /* We didn't find the branch in the loop above, so
	   it must be a HighConf branch */
	  if (class < 0)
	    retval = HighConf;

	  else
	    retval = (class < free_contexts) ? LowConf : HighConf;
	      
	  *conf_data = (retval == HighConf) ? 1 : 0;
      
	  return retval;
	}
      
    }
  
  /* If there is any form of hardware conf prediction, we need to
     index the predictor table */
  if (bc->gshare)
    idx = ((addr>>3) ^ bc->gbhr) % bc->table_size;
  else
    idx = (addr>>3) % bc->table_size;

  /* only examine 'entry_size' bits */
  /*  bits = bc->table[idx] & ((1<<bc->entry_size) - 1);*/
  bits = bc->table[idx];

  /* Interpret the bits from the conf table according to the type of
     predictor */
  switch (bc->type)
    {
    case BCF_Ones:
      /* Count the number of 1's (i.e. correct predictions) */
      for (i=0, val=0; i<bc->entry_size; i++)
	{
	  val += bits & 0x1;
	  bits = bits >> 1;
	}

      *conf_data = bc->table[idx] & ((1<<bc->entry_size) - 1);
      /* *conf_data = val;*/
      break;
      
    case BCF_Sat:
    case BCF_Reset:
      val = bits;
      assert(val >= bc->min);
      assert(val <= bc->max);
      
      *conf_data = val - bc->min;
      break;

    default:
      fatal("Invalid bc->type value '%d'\n", bc->type);
    }

  /* Choose the appropriate threshold */
  switch (bc->selector)
    {
    case BTS_None:
      thresh = 0;
      break;
	  
    case BTS_Profile:
      thresh = class;
      break;

    case BTS_HW:
      thresh = MIN(free_contexts-1, bc->num_thresholds-1);
      break;
	  
    default:
      fprintf(stderr,"HEY! sel=%d, none=%d, profile=%d, hw=%d\n",
	      bc->selector, BTS_None, BTS_Profile, BTS_HW);
      
      fatal("Invalid bc->selector value '%d'\n", bc->selector);
    }

  /* Determine High or Low confidence */
  retval = (thresh < 0 || val >= bc->thresholds[thresh]) ? HighConf : LowConf;


#ifdef BCONF_DUMP
  fprintf(dump_file, "addr=0x%lx, class=%d, threshold=%d, idx=%u, "
	  "conf=%d, bits=%d, val=%d, *conf_data=%d\n",
	  addr, class, class<0 ? -1 : bc->thresholds[class], idx,
	  retval, bc->table[idx], val, *conf_data);
#endif

  return retval;
}

unsigned int
bconf_update(struct bconf *bc, 
	     SS_ADDR_TYPE addr,		/* branch address */
	     char taken,		/* was branch predicted taken? */
	     char bpr_correct,		/* was branch predictor correct? */
	     char bcf_pred)		/* bcf's pred.  Cruft, no longer used*/
{
  int i, idx, old, correct;
  
  assert(bc);

  /* If there's no hardware component of the conf predictor, there's
   no work to be done here */
  if (bc->type == BCF_None)
    return 0;
  
  /* PONDER: Should high confidence branches update the conf predictor
   when using a profile-based scheme? The following "if" removes them */
  if (bc->selector == BTS_Profile)
    {
      int found=0;
      
      for (i=0; i<bc->num_forks; i++)
	if (addr == bc->forks[i].addr)
	  {
	    /* Branch found */
	    found = 1;
	    break;
	  }
      
      if (!found)
	{

#ifdef BCONF_DUMP
	  fprintf(dump_file, "Branch not in profile -- not updating predictor"
		  "addr=0x%lx, taken=%d, correct=%d\n",
		  addr, taken, correct, old);
#endif

	  return 0;	/* PONDER: what's the correct value to return? */
	}
    }
  
  if (bc->gshare)
    idx = ((addr>>3) ^ bc->gbhr) % bc->table_size;
  else
    idx = (addr>>3) % bc->table_size;

#ifdef BCONF_DUMP
  old = bc->table[idx];
#endif  

  bpr_correct = bpr_correct ? 1 : 0;
  bcf_pred = (bcf_pred == HighConf) ? 1 : 0;
  
  /* 
   * If we're using the bconf history to update the bconf table, then
   * 'correct' is 1 if:
   *  	branch was correctly predicted and confidence was high, OR
   *    branch was mispredicted and confidence was low.
   *
   * Else, 'correct' is 1 if the branch was correctly predicted.
   *
   * This is CRUFT; we no longer use bconf's prediction to update bconf.
   */
  assert(!bc->history_update);
  correct = bc->history_update ? (bpr_correct ^ bcf_pred) : bpr_correct;

  switch (bc->type)
    {
    case BCF_Ones:
      /* shift in a 1 for correct pred, 0 for incorrect pred */
      bc->table[idx] = (bc->table[idx] << 1) | correct;
      break;
      
    case BCF_Sat:
      /* increment/decrement counter within min/max values */
      if (!correct)
	bc->table[idx] = MAX(bc->table[idx] - 1, bc->min);
      else
	bc->table[idx] = MIN(bc->table[idx] + 1, bc->max);
      break;
      
    case BCF_Reset:
      /* reset ctr on a misprediction */
      if (!correct)
	bc->table[idx] = 0;

      /* increment ctr on correct prediction, but only up to the max value */
      else
	bc->table[idx] = MIN(bc->table[idx] + 1, bc->max);
      break;

    default:
      fatal("Invalid bc->type value '%d'\n", bc->type);
    }

  if (bc->gshare)
    bc->gbhr = (bc->gbhr << 1) | (taken ? 1 : 0);
  
#ifdef BCONF_DUMP
  fprintf(dump_file, 
	  "addr=0x%lx, idx=%u, taken=%d, correct=%d, old=0x%x, new=0x%x\n",
	  addr, idx, taken, correct, old, bc->table[idx]);
#endif

  return bc->table[idx];
}

/* register confidence predictor stats */
void
bconf_reg_stats(struct bconf *bc,
		struct stat_sdb_t *sdb)	/* stats database */
{
  int arr_size = 2;
  
  /*
  if (bc->type == Ones)
    arr_size = bc->entry_size+1;
    
  else*/ 
  if (bc->type == BCF_Ones || bc->type == BCF_Sat || bc->type == BCF_Reset)
    arr_size = 1 << bc->entry_size;
  
  if (bc->output_dist && 
      (bc->type == BCF_Ones || bc->type == BCF_Sat || bc->type == BCF_Reset))
    {
      bconf_correct_dist = stat_reg_dist(sdb, "bconf: correct patterns.PP", 
					 "Bit patterns from the conf "
					 "predictor, correctly predicted",
					 /* init */0, 
					 /* arr sz */arr_size,
					 /* bucket sz */1, 
					 (PF_COUNT | PF_PDF | PF_CDF),
					 NULL, NULL, NULL);

      bconf_incorrect_dist = stat_reg_dist(sdb, "bconf: incorrect "
					   "patterns.PP", 
					   "Bit patterns from the conf "
					   "predictor, incorrectly predicted",
					   /* init */0, 
					   /* arr sz */arr_size,
					   /* bucket sz */1, 
					   (PF_COUNT | PF_PDF | PF_CDF),
					   NULL, NULL, NULL);
    }
}

void
bconf_after_priming(struct bconf *bc)
{
  if (bc == NULL)
    return;

  bc->very_low = 0;
}
