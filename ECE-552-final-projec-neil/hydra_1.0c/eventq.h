/*
 * eventq.h - event queue manager interfaces
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
 * $Id: eventq.h,v 3.1 1998/04/17 16:51:35 skadron Exp $
 *
 * $Log: eventq.h,v $
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
 * Revision 1.1  1997/02/16  22:23:54  skadron
 * Initial revision
 *
 * Revision 1.1  1996/12/05  18:50:23  taustin
 * Initial revision
 *
 *
 */

#ifndef EVENTQ_H
#define EVENTQ_H

#include <stdio.h>
#include "bitmap.h"

/* This module implements a time ordered event queue.  Users insert
 *
 */

/* event actions */
enum eventq_action {
  EventSetBit,			/* set a bit: long *, bit # */
  EventClearBit,		/* clear a bit: long *, bit # */
  EventSetFlag,			/* set a flag: int *, value */
  EventAddOp,			/* add a value to a summand */
  EventCallback,		/* call event handler: fn * */
};

/* ID zero (0) is unused */
typedef unsigned long EVENTQ_ID_TYPE;

/* event descriptor */
struct eventq_desc {
  struct eventq_desc *next;	/* next event in the sorted list */
  SS_TIME_TYPE when;		/* time to schedule write back event */
  EVENTQ_ID_TYPE id;		/* unique event ID */
  enum eventq_action action;	/* action on event occurrance */
  union eventq_data {
    struct {
      BITMAP_ENT_TYPE *bmap;	/* bitmap to access */
      int sz;			/* bitmap size */
      int bitnum;		/* bit to set */
    } bit;
    struct {
      int *pflag;		/* flag to set */
      int value;
    } flag;
    struct {
      int *summand;		/* value to add to */
      int addend;		/* amount to add */
    } addop;
    struct {
      void (*fn)(SS_TIME_TYPE time, int arg);	/* function to call */
      int arg;			/* argument to pass */
    } callback;
  } data;
};

/* initialize the event queue module, MAX_EVENT is the most events allowed
   pending, pass a zero if there is no limit */
void eventq_init(int max_events);

/* schedule an action that occurs at WHEN, action is visible at WHEN,
   and invisible before WHEN */
EVENTQ_ID_TYPE eventq_queue_setbit(SS_TIME_TYPE when,
				   BITMAP_ENT_TYPE *bmap, int sz, int bitnum);
EVENTQ_ID_TYPE eventq_queue_clearbit(SS_TIME_TYPE when, BITMAP_ENT_TYPE *bmap,
				     int sz, int bitnum);
EVENTQ_ID_TYPE eventq_queue_setflag(SS_TIME_TYPE when,
				    int *pflag, int value);
EVENTQ_ID_TYPE eventq_queue_addop(SS_TIME_TYPE when,
				  int *summand, int addend);
EVENTQ_ID_TYPE eventq_queue_callback(SS_TIME_TYPE when,
				     void (*fn)(SS_TIME_TYPE time, int arg),
				     int arg);

/* execute an event immediately, returns non-zero if the event was
   located an deleted */
int eventq_execute(EVENTQ_ID_TYPE id);

/* remove an event from the eventq, action is never performed, returns
   non-zero if the event was located an deleted */
int eventq_remove(EVENTQ_ID_TYPE id);

/* service all events in order of occurrance until and at NOW */
void eventq_service_events(SS_TIME_TYPE now);

void eventq_dump(FILE *stream);

#endif /* EVENT_H */
