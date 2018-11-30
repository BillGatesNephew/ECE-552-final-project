/*
 * bconf.h - branch-confidence predictor interfaces 
 *
 * This file is written by Pritpal S. Ahuja, Copyright (C) 1998, 1999.
 * For more information, contact skadron@cs.princeton.edu
 *
 * $Id: bconf.h,v 3.1 1998/04/17 16:51:35 skadron Exp $
 *
 * $Log: bconf.h,v $
 * Revision 3.1  1998/04/17 16:51:35  skadron
 * Versions used thru Mar '98 (retstack sub, ruu_resub, ICS final manuscript)
 *
 * Revision 2.102  1998/03/24 18:54:18  skadron
 * Added "output_dist" option
 *
 * Revision 2.101  1997/12/09 22:20:31  skadron
 * Version Pritpal used for ISCA-25 submission
 *
 * Revision 2.9  1997/11/22 18:21:40  psa
 * More bug fixes.
 *
 * Revision 2.8  1997/11/21 04:16:12  psa
 * Added profile-based and adaptive conf predictors.
 * Bug fixes.
 *
 * Revision 2.7  1997/09/21 19:35:47  skadron
 * Added -bconf:gshare
 *
 * Revision 2.6  1997/09/21 15:23:46  skadron
 * Added counter for number of "very low confidence" predictions
 *
 * Revision 2.5  1997/09/15 21:24:00  skadron
 * Added RCS headers
 *
 */
#ifndef BCONF_H
#define BCONF_H

#include "ss.h"
#include "stats.h"

#define HighConf 10
#define LowConf   0

typedef enum
{
  BCF_None=0,
  BCF_Naive,		/* naive forking */
  BCF_Omni,		/* omniscient */
  BCF_Ones,		/* ones counter */
  BCF_Sat,		/* saturating counter */
  BCF_Reset,		/* resetting counter */
  BCF_Pattern,		/* use PHT history bits, as proposed by Tyson */
  BCF_NUM
} bconf_type_enum;

typedef enum
{
  BTS_None=0,		/* for when there is only one threshold */
  BTS_Profile,		/* use a cbr's class to select a threshold */
  BTS_HW,		/* use the # of free contexts to select a t/h */
  BTS_NUM
} bconf_selector_enum;

struct profile_entry		/* For profile-based conf, each branch */
{				/*  has an address and a class value    */
  SS_ADDR_TYPE addr;		
  int class;
};

struct bconf
{
  bconf_type_enum type;		/* type of confidence predictor */
  bconf_selector_enum selector;	/* how to choose amongst thresholds */
  int history_update;		/* use conf history to update conf entry? */
  int table_size;		/* number of entries in the table */
  int entry_size;		/* number of bits per entry */
  int num_thresholds;		/* num of thresholds (>1 for adaptive only) */
  int *thresholds;		/* used to determine hi/lo confidence */
  int min, max;			/* min and max bit values for each entry */
  int gshare;			/* xor global-hist with br-addr to get idx? */
  int output_dist;		/* output correct/incorrect pattern dist's? */
  
  unsigned int gbhr;		/* global branch history register */

  SS_COUNTER_TYPE very_low;	/* how many times conf is so low we might
				 * want to flip prediction */
  
  int *table;			/* confidence history table */
  struct profile_entry *forks;	/* array of cbr entries for Profile-type bcf */
  int num_forks;		/* number of cbr entries */
  
  struct stat_stat_t *correct_dist;
  struct stat_stat_t *incorrect_dist;
};

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
	     char *config_file_name);	/* optional config file */

int
bconf_lookup(struct bconf *bc, 
	     SS_ADDR_TYPE addr,		/* branch address to look up */
	     int free_contexts,		/* num of free forking contexts */
	     int *conf_data);		/* data returned by the conf 
					   predictor for statistics  */

unsigned int
bconf_update(struct bconf *bc, 
	     SS_ADDR_TYPE addr,		/* branch address */
	     char taken,		/* was branch predicted taken? */
	     char bpr_correct,		/* was branch predictor correct? */
	     char bcf_pred);		/* did the bcf say high or low conf? */

void
bconf_reg_stats(struct bconf *bc,
		struct stat_sdb_t *sdb);/* stats database */

void
bconf_after_priming(struct bconf *bc);

#endif
