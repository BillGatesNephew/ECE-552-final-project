/*
 * memory.c - flat memory space routines
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
 * $Id: memory.c,v 3.1 1998/04/17 16:51:35 skadron Exp $
 *
 * $Log: memory.c,v $
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
 * Revision 1.5  1997/03/11  01:15:25  taustin
 * updated copyright
 * mem_valid() added, indicates if an address is bogus, used by DLite!
 * long/int tweaks made for ALPHA target support
 *
 * Revision 1.4  1997/01/06  16:00:51  taustin
 * stat_reg calls now do not initialize stat variable values
 *
 * Revision 1.3  1996/12/27  15:52:46  taustin
 * updated comments
 * integrated support for options and stats packages
 *
 * Revision 1.1  1996/12/05  18:52:32  taustin
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "misc.h"
#include "ss.h"
#include "loader.h"
#include "regs.h"
#include "memory.h"

/* top of the data segment */
SS_ADDR_TYPE mem_brk_point = 0;

/* lowest address accessed on the stack */
SS_ADDR_TYPE mem_stack_min = 0x7fffffff;

/* first level memory block table */
char *mem_table[MEM_TABLE_SIZE];

/* memory block tables for recovering memory allocated speculatively */
int mem_table_arch[MEM_TABLE_SIZE];
int mem_access_mode_spec = FALSE;

/* determines if the memory access is valid, returns error str or NULL */
char *					/* error string, or NULL */
mem_valid(enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	  SS_ADDR_TYPE addr,		/* target address to access */
	  int nbytes,			/* number of bytes to access */
	  int declare)			/* declare the error if detected? */
{
  char *err_str = NULL;

  /* check alignments */
  if ((nbytes & (nbytes-1)) != 0 || (addr & (nbytes-1)) != 0)
    {
      err_str = "bad size or alignment";
    }
  /* check permissions, no probes allowed into undefined segment regions */
  else if (!(/* text access and a read */
	   (addr >= ld_text_base && addr < (ld_text_base+ld_text_size)
	    && cmd == Read)
	   /* data access within bounds */
	   || (addr >= ld_data_base && addr < ld_stack_base)))
    {
      err_str = "segmentation violation";
    }

  /* track the minimum SP for memory access stats */
  if (addr > mem_brk_point && addr < mem_stack_min)
    mem_stack_min = addr;

  if (!declare)
    return err_str;
  else if (err_str != NULL)
    fatal(err_str);
  else /* no error */
    return NULL;
}

/* generic memory access function, its safe because alignments and permissions
   are checks, handles any resonable transfer size; note, bombs if nbytes
   is larger then MEM_BLOCK_SIZE */
void
mem_access(enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	   SS_ADDR_TYPE addr,		/* target address to access */
	   void *vp,			/* host memory address to access */
	   int nbytes)			/* number of bytes to access */
{
  char *p = vp;

  /* check alignments */
  if ((nbytes & (nbytes-1)) != 0 || (addr & (nbytes-1)) != 0)
    fatal("access error: bad size or alignment, addr 0x%08x", addr);

  /* check permissions, no probes allowed into undefined segment regions */
  if (!(/* text access and a read */
	(addr >= ld_text_base && addr < (ld_text_base+ld_text_size)
	 && cmd == Read)
	/* data access within bounds */
	|| (addr >= ld_data_base && addr < ld_stack_base)))
    fatal("access error: segmentation violation, addr 0x%08x", addr);

  /* track the minimum SP for memory access stats */
  if (addr > mem_brk_point && addr < mem_stack_min)
    mem_stack_min = addr;

  /* perform the copy */
  switch (nbytes) {
  case 1:
    if (cmd == Read)
      *((unsigned char *)p) = __MEM_READ_BYTE(addr);
    else
      __MEM_WRITE_BYTE(addr, *((unsigned char *)p));
    break;
  case 2:
    if (cmd == Read)
      *((unsigned short *)p) = __MEM_READ_HALF(addr);
    else
      __MEM_WRITE_HALF(addr, *((unsigned short *)p));
    break;
  case 4:
    if (cmd == Read)
      *((unsigned int *)p) = __MEM_READ_WORD(addr);
    else
      __MEM_WRITE_WORD(addr, *((unsigned int *)p));
    break;
  default:
    {
      /* nbytes >= 8 and power of two */
      int words = nbytes >> 2;
      if (cmd == Read)
	{
	  while (words-- > 0)
	    {
	      *((unsigned int *)p) = __MEM_READ_WORD(addr);
	      p += 4;
	      addr += 4;
	    }
	}
      else
	{
	  while (words-- > 0)
	    {
	      __MEM_WRITE_WORD(addr, *((unsigned int *)p));
	      p += 4;
	      addr += 4;
	    }
	}
    }
    break;
  }
}

/* allocate a memory block */
char *
mem_newblock(void)
{
  /* see misc.c for details on the getcore() function */
  return getcore(MEM_BLOCK_SIZE);
}

/* copy a '\0' terminated string through a memory access function, returns
   the number of bytes copied, returns the number of bytes copied */
int
mem_strcpy(mem_access_fn mem_fn,	/* user-specified memory accessor */
	   enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	   SS_ADDR_TYPE addr,		/* target address to access */
	   char *s)			/* host memory string buffer */
{
  int n = 0;
  char c;

  switch (cmd) {
  case Read:
    /* copy until string terminator ('\0') is encountered */
    do {
      mem_fn(Read, addr++, &c, 1);
      *s++ = c;
      n++;
    } while (c);
    break;
  case Write:
    /* copy until string terminator ('\0') is encountered */
    do {
      c = *s++;
      mem_fn(Write, addr++, &c, 1);
      n++;
    } while (c);
    break;
  default:
    panic("bogus memory command");
  }
  return n;
}

/* copy NBYTES through a memory access function */
void
mem_bcopy(mem_access_fn mem_fn,		/* user-specified memory accessor */
	  enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	  SS_ADDR_TYPE addr,		/* target address to access */
	  void *vp,			/* host memory address to access */
	  int nbytes)			/* number of bytes to access */
{
  char *p = vp;

  /* copy NBYTES bytes of simulator memory */
  while (nbytes-- > 0)
    mem_fn(cmd, addr++, p++, 1);
}

/* copy NBYTES through a memory access function, NBYTES must be a multiple
   of 4 bytes, this function is faster than mem_bcopy() */
void
mem_bcopy4(mem_access_fn mem_fn,	/* user-specified memory accessor */
	   enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	   SS_ADDR_TYPE addr,		/* target address to access */
	   void *vp,			/* host memory address to access */
	   int nbytes)			/* number of bytes to access */
{
  char *p = vp;
  int words = nbytes >> 2;		/* note: nbytes % 2 == 0 is assumed */

  while (words-- > 0)
    mem_fn(cmd, addr += 4, p += 4, 4);
}

/* zero out NBYTES through a memory access function */
void
mem_bzero(mem_access_fn mem_fn,		/* user-specified memory accessor */
	  SS_ADDR_TYPE addr,		/* target address to access */
	  int nbytes)			/* number of bytes to clear */
{
  char c = 0;

  /* zero out NBYTES of simulator memory */
  while (nbytes-- > 0)
    mem_fn(Write, addr++, &c, 1);
}

/* register memory system-specific options */
void
mem_reg_options(struct opt_odb_t *odb)	/* option data base */
{
  /* none currently */
}

/* check memory system-specific option values */
void
mem_check_options(struct opt_odb_t *odb, int argc, char **argv)
{
  /* nada */
}

/* register memory system-specific statistics */
void
mem_reg_stats(struct stat_sdb_t *sdb)
{
  stat_reg_uint(sdb, "mem_brk_point",
		"data segment break point",
		(unsigned int *)&mem_brk_point, mem_brk_point, "  0x%08x");
  stat_reg_uint(sdb, "mem_stack_min",
		"lowest address accessed in stack segment",
		(unsigned int *)&mem_stack_min, mem_stack_min, "  0x%08x");

  stat_reg_formula(sdb, "mem_total_data",
		   "total bytes used in init/uninit data segment",
		   "(ld_data_size + 1023) / 1024",
		   "%11.0fk");
  stat_reg_formula(sdb, "mem_total_heap",
		   "total bytes used in program heap segment",
		   "(((mem_brk_point - (ld_data_base + ld_data_size)))+1023)"
		   " / 1024", "%11.0fk");
  stat_reg_formula(sdb, "mem_total_stack",
		   "total bytes used in stack segment",
		   "((ld_stack_base - mem_stack_min) + 1024) / 1024",
		   "%11.0fk");
  stat_reg_formula(sdb, "mem_total_mem",
		   "total bytes used in data, heap, and stack segments",
		   "mem_total_data + mem_total_heap + mem_total_stack",
		   "%11.0fk");
}

/* initialize memory system, call before loader.c */
void
mem_init(void)
{
  int i;

  /* initialize the first level page table to all empty */
  for (i=0; i<MEM_TABLE_SIZE; i++)
    mem_table[i] = NULL;

  /* initialize the first level page table arch state to TRUE */
  for (i=0; i<MEM_TABLE_SIZE; i++)
    mem_table_arch[i] = TRUE;
}

/* initialize memory system, call after loader.c */
void
mem_init1(void)
{

  /* initialize the bottom of heap to top of data segment */
  mem_brk_point = ROUND_UP(ld_data_base + ld_data_size, SS_PAGE_SIZE);

  /* set initial minimum stack pointer value to initial stack value */
  mem_stack_min = regs_R[SS_STACK_REGNO];
}

/* print out memory system configuration */
void
mem_aux_config(FILE *stream)	/* output stream */
{
  /* none currently */
}

/* dump memory system stats */
void
mem_aux_stats(FILE *stream)	/* output stream */
{
  /* zippo */
}

/* dump a block of memory */
void
mem_dump(mem_access_fn mem_fn,		/* user-specified memory access */
	 SS_ADDR_TYPE addr,		/* target address to dump */
	 int len,			/* number bytes to dump */
	 FILE *stream)			/* output stream */
{
  int data;

  if (!stream)
    stream = stderr;

  addr &= sizeof(int);
  len = (len+(sizeof(int)-1)) & sizeof(int);
  while (len-- > 0)
    {
      mem_fn(Read, addr, &data, sizeof(int));
      fprintf(stream, "0x%08x: %08x\n", addr, data);
      addr += sizeof(int);
    }
}
