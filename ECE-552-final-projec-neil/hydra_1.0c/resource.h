/*
 * resource.h - resource manager interfaces
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
 * $Id: resource.h,v 3.1 1998/04/17 16:51:35 skadron Exp $
 *
 * $Log: resource.h,v $
 * Revision 3.1  1998/04/17 16:51:35  skadron
 * Versions used thru Mar '98 (retstack sub, ruu_resub, ICS final manuscript)
 *
 * Revision 2.102  1998/03/20 17:42:03  skadron
 * MAX_INSTS_PER_CLASS of 16 should be enough for WIDE_CFG, I think...
 *
 * Revision 2.101  1997/12/09 22:20:31  skadron
 * Version Pritpal used for ISCA-25 submission
 *
 * Revision 2.100  1997/12/03 20:05:46  skadron
 * Version Kevin used for ISCA-25 submission
 *
 * Revision 2.7  1997/11/16 06:53:36  skadron
 * PLUS_CFG and HUGE_CFG now assume -res:infinite
 *
 * Revision 2.6  1997/11/11 20:28:21  skadron
 * Added PLUS_CFG; changed MAX_CFG to WIDE_CFG
 *
 * Revision 2.5  1997/11/08 02:00:29  skadron
 * Added a HUGE_CONFIG (which is bigger than MAX_CFG)
 *
 * Revision 2.4  1997/10/01 22:11:54  skadron
 * Changes to facilitate "max-cfg" -- wide open except realistic fetch
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

#ifndef RESOURCE_H
#define RESOURCE_H

#include <stdio.h>

/* maximum number of resource classes supported */
#define MAX_RES_CLASSES		16

/* maximum number of resource instances for a class supported */
#ifdef HUGE_CFG
#define MAX_INSTS_PER_CLASS	1
#elif PLUS_CFG
#define MAX_INSTS_PER_CLASS	1
#elif WIDE_CFG
#define MAX_INSTS_PER_CLASS	16
#else
#define MAX_INSTS_PER_CLASS	8
#endif

/* resource descriptor */
struct res_desc {
  char *name;				/* name of functional unit */
  int quantity;				/* total instances of this unit */
  int busy;				/* non-zero if this unit is busy */
  struct res_template {
    int class;				/* matching resource class: insts
					   with this resource class will be
					   able to execute on this unit */
    int oplat;				/* operation latency: cycles until
					   result is ready for use */
    int issuelat;			/* issue latency: number of cycles
					   before another operation can be
					   issued on this resource */
    struct res_desc *master;		/* master resource record */
  } x[MAX_RES_CLASSES];
};

/* resource pool: one entry per resource instance */
struct res_pool {
  char *name;				/* pool name */
  int num_resources;			/* total number of res instances */
  struct res_desc *resources;		/* resource instances */
  /* res class -> res template mapping table, lists are NULL terminated */
  int nents[MAX_RES_CLASSES];
  struct res_template *table[MAX_RES_CLASSES][MAX_INSTS_PER_CLASS];
};

/* create a resource pool */
struct res_pool *res_create_pool(char *name, struct res_desc *pool, int ndesc);

/* get a free resource from resource pool POOL that can execute a
   operation of class CLASS, returns a pointer to the resource template,
   returns NULL, if there are currently no free resources available,
   follow the MASTER link to the master resource descriptor;
   NOTE: caller is responsible for reseting the busy flag in the beginning
   of the cycle when the resource can once again accept a new operation */
struct res_template *res_get(struct res_pool *pool, int class);

/* dump the resource pool POOL to stream STREAM */
void res_dump(struct res_pool *pool, FILE *stream);

#endif /* RESOURCE_H */
