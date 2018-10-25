/*
 * memory.h - flat memory space interfaces
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
 * $Id: memory.h,v 3.1 1998/04/17 16:51:35 skadron Exp $
 *
 * $Log: memory.h,v $
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
 * Revision 1.5  1997/03/11  01:16:23  taustin
 * updated copyright
 * long/int tweaks made for ALPHA target support
 * major macro reorganization to support CC portability
 * mem_valid() added, indicates if an address is bogus, used by DLite!
 *
 * Revision 1.4  1997/01/06  16:01:24  taustin
 * HIDE_MEM_TABLE_DEF added to help with sim-fast.c compilation
 *
 * Revision 1.3  1996/12/27  15:53:15  taustin
 * updated comments
 *
 * Revision 1.1  1996/12/05  18:50:23  taustin
 * Initial revision
 *
 *
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <stdio.h>
#include "endian.h"
#include "options.h"
#include "stats.h"
#include "ss.h"

/* memory command */
enum mem_cmd {
  Read,			/* read memory from target (simulated prog) to host */
  Write			/* write memory from host (simulator) to target */
};

/* memory access function type, this is a generic function exported for the
   purpose of access the simulated vitual memory space */
typedef void
(*mem_access_fn)(enum mem_cmd cmd,	/* Read or Write */
		 unsigned int addr,	/* target memory address to access */
		 void *p,		/* where to copy to/from */
		 int nbytes);		/* transfer length in bytes */


/*
 * The SimpleScalar virtual memory address space is 2^31 bytes mapped from
 * 0x00000000 to 0x7fffffff.  The upper 2^31 bytes are currently reserved for
 * future developments.  The address space from 0x00000000 to 0x00400000 is
 * currently unused.  The address space from 0x00400000 to 0x10000000 is used
 * to map the program text (code), although accessing any memory outside of
 * the defined program space causes an error to be declared.  The address
 * space from 0x10000000 to "mem_brk_point" is used for the program data
 * segment.  This section of the address space is initially set to contain the
 * initialized data segment and then the uninitialized data segment.
 * "mem_brk_point" then grows to higher memory when sbrk() is called to
 * service heap growth.  The data segment can continue to expand until it
 * collides with the stack segment.  The stack segment starts at 0x7fffc000
 * and grows to lower memory as more stack space is allocated.  Initially,
 * the stack contains program arguments and environment variables (see
 * loader.c for details on initial stack layout).  The stack may continue to
 * expand to lower memory until it collides with the data segment.
 *
 * The SimpleScalar virtual memory address space is implemented with a
 * one level page table, where the first level table contains MEM_TABLE_SIZE
 * pointers to MEM_BLOCK_SIZE byte pages in the second level table.  Pages
 * are allocated in MEM_BLOCK_SIZE size chunks when first accessed, the initial
 * value of page memory is all zero.
 *
 * Graphically, it all looks like this:
 *
 *                 Virtual        Level 1    Host Memory Pages
 *                 Address        Page       (allocated as needed)
 *                 Space          Table
 * 0x00000000    +----------+      +-+      +-------------------+
 *               | unused   |      | |----->| memory page (64k) |
 * 0x00400000    +----------+      +-+      +-------------------+
 *               |          |      | |
 *               | text     |      +-+
 *               |          |      | |
 * 0x10000000    +----------+      +-+
 *               |          |      | |
 *               | data seg |      +-+      +-------------------+
 *               |          |      | |----->| memory page (64k) |
 * mem_brk_point +----------+      +-+      +-------------------+
 *               |          |      | |
 *               |          |      +-+
 *               |          |      | |
 * regs_R[29]    +----------+      +-+
 * (stack ptr)   |          |      | |
 *               | stack    |      +-+
 *               |          |      | |
 * 0x7fffc000    +----------+      +-+      +-------------------+
 *               | unsed    |      | |----->| memory page (64k) |
 * 0x7fffffff    +----------+      +-+      +-------------------+

 */

/* top of the data segment, sbrk() moves this to higher memory */
extern SS_ADDR_TYPE mem_brk_point;

/* lowest address accessed on the stack */
extern SS_ADDR_TYPE mem_stack_min;

/*
 * memory page table defs
 */

/* memory indirect table size (upper mem is not used) */
#define MEM_TABLE_SIZE		0x8000 /* was: 0x7fff */

/* memory block tables for recovering memory allocated speculatively */
extern int mem_table_arch[MEM_TABLE_SIZE];
extern int mem_access_mode_spec;

#ifndef HIDE_MEM_TABLE_DEF	/* used by sim-fast.c */
/* the level 1 page table map */
extern char *mem_table[MEM_TABLE_SIZE];
#endif /* HIDE_MEM_TABLE_DEF */

/* memory block size, in bytes */
#define MEM_BLOCK_SIZE		0x10000

/* memory access macros, most significant bit is ignored */
#define MEM_BLOCK(addr) 	((((SS_ADDR_TYPE)(addr)) >> 16) & 0x7fff)
#define MEM_OFFSET(addr)	((addr) & 0xffff)

/*
 * memory page table accessors
 */

/* memory tickle function, this version allocates pages when they are touched
   for the first time */

/*
#define __MEM_TICKLE(addr)                                              \
  (!mem_table[MEM_BLOCK(addr)]                                          \
   ? ((mem_access_mode_spec                                             \
       ? (0)                     \
       : (mem_table_arch[MEM_BLOCK(addr)] = TRUE)),                                                             \
      mem_table[MEM_BLOCK(addr)] = mem_newblock())                      \
   : 0)
*/

#define __MEM_TICKLE(addr)						\
  ((!mem_table[MEM_BLOCK(addr)]						\
    ? ((mem_access_mode_spec                                            \
	? (mem_table_arch[MEM_BLOCK(addr)] = FALSE)                     \
	: (0)),                                                         \
       mem_table[MEM_BLOCK(addr)] = mem_newblock())                     \
    : 0))
   
#define MEM_ACCESS_COMMIT(addr)                                         \
  (assert(mem_table[MEM_BLOCK(addr)]),                                  \
  mem_table_arch[MEM_BLOCK(addr)] = TRUE)

#define MEM_ACCESS_SQUASH(addr)                                         \
  ((mem_table[MEM_BLOCK(addr)] &&                                       \
    !mem_table_arch[MEM_BLOCK(addr)])                                   \
    ? (free(mem_table[MEM_BLOCK(addr)]),                                \
       (mem_table_arch[MEM_BLOCK(addr)] = TRUE),                        \
       mem_table[MEM_BLOCK(addr)] = 0)                                  \
    : 0)

/* fast memory access function, this is not checked so only use this function
   if you are sure that it cannot fault, e.g., instruction fetches, this
   function returns NULL is the memory page is not allocated */
#define __UNCHK_MEM_ACCESS(type, addr)					\
  (*((type *)(mem_table[MEM_BLOCK(addr)] + MEM_OFFSET(addr))))

/* fast memory access macros, these are unsafe, use lower case versions
   to enable alignment and permission checks; note, all macros return
   unsigned integer values, cast as needed */
#define __MEM_READ_WORD(addr)						\
  (__MEM_TICKLE(addr), __UNCHK_MEM_ACCESS(unsigned int, (addr)))
#define __MEM_WRITE_WORD(addr, word)					\
  (__MEM_TICKLE(addr), __UNCHK_MEM_ACCESS(unsigned int, (addr)) = (word))

#define __MEM_READ_HALF(addr)						\
  (__MEM_TICKLE(addr), __UNCHK_MEM_ACCESS(unsigned short, (addr)))
#define __MEM_WRITE_HALF(addr, half)					\
  (__MEM_TICKLE(addr), __UNCHK_MEM_ACCESS(unsigned short, (addr)) = (half))

#define __MEM_READ_BYTE(addr)						\
  (__MEM_TICKLE(addr), __UNCHK_MEM_ACCESS(unsigned char, (addr)))
#define __MEM_WRITE_BYTE(addr, byte)					\
  (__MEM_TICKLE(addr), __UNCHK_MEM_ACCESS(unsigned char, (addr)) = (byte))

/* memory access macros, these are safe */
#define MEM_READ_WORD(addr)						\
  (mem_valid(Read, (addr), sizeof(unsigned int), /* declare */TRUE),	\
   __MEM_READ_WORD(addr))
#define MEM_WRITE_WORD(addr, word)					\
  (mem_valid(Write, (addr), sizeof(unsigned int), /* declare */TRUE),	\
   __MEM_WRITE_WORD((addr), (word)))

#define MEM_READ_HALF(addr)						\
  (mem_valid(Read, (addr), sizeof(unsigned short), /* declare */TRUE),	\
   __MEM_READ_HALF(addr))
#define MEM_WRITE_HALF(addr, half)					\
  (mem_valid(Write, (addr), sizeof(unsigned short), /* declare */TRUE),	\
   __MEM_WRITE_HALF((addr), (half)))

#define MEM_READ_BYTE(addr)						\
  (mem_valid(Read, (addr), sizeof(unsigned char), /* declare */TRUE),	\
   __MEM_READ_BYTE(addr))
#define MEM_WRITE_BYTE(addr, byte)					\
  (mem_valid(Write, (addr), sizeof(unsigned char), /* declare */TRUE),	\
   __MEM_WRITE_BYTE((addr), (byte)))

#if 0 /* these are now obsolete, use the interfaces defined above */
/* memory access functions, these are safe, swapped, and check alignment
   and permissions, all these macros return unsigned integer values, cast
   as needed */
#define mem_read_word(cmd, addr, p)					\
  (mem_access(cmd, (addr), p, sizeof(unsigned int)),			\
   *((unsigned int *)p) = SWAP_WORD(*((unsigned int *)p)))
#define mem_write_word(cmd, addr, p)					\
  ({ unsigned int _temp = SWAP_WORD(*((unsigned int *)p));		\
     mem_access(cmd, (addr), &_temp, sizeof(unsigned int)); })

#define mem_read_half(cmd, addr, p)					\
  (mem_access(cmd, (addr), p, sizeof(unsigned short)),			\
   *((unsigned short *)p) = SWAP_HALF(*((unsigned short *)p)))
#define mem_write_half(cmd, addr, p)					\
  ({ unsigned short _temp = SWAP_HALF(*((unsigned short *)p));		\
     mem_access(cmd, (addr), &_temp, sizeof(unsigned short)); })

#define mem_read_byte(cmd, addr, p)					\
  mem_access(cmd, (addr), p, sizeof(char))
#define mem_write_byte(cmd, addr, p)					\
  mem_access(cmd, (addr), p, sizeof(char))
#endif


/* determines if the memory access is valid, returns error str or NULL */
char *					/* error string, or NULL */
mem_valid(enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	  SS_ADDR_TYPE addr,		/* target address to access */
	  int nbytes,			/* number of bytes to access */
	  int declare);			/* declare any detected error? */

/* generic memory access function, its safe because alignments and permissions
   are checks, handles any resonable transfer size; note, bombs if nbytes
   is larger then MEM_BLOCK_SIZE */
void
mem_access(enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	   SS_ADDR_TYPE addr,		/* target address to access */
	   void *vp,			/* host memory address to access */
	   int nbytes);			/* number of bytes to access */

/* allocate a memory block */
char *mem_newblock(void);

/* copy a '\0' terminated string through a memory access function, returns
   the number of bytes copied, returns the number of bytes copied */
int
mem_strcpy(mem_access_fn mem_fn,	/* user-specified memory accessor */
	   enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	   SS_ADDR_TYPE addr,		/* target address to access */
	   char *s);			/* host memory string buffer */

/* copy NBYTES through a memory access function */
void
mem_bcopy(mem_access_fn mem_fn,		/* user-specified memory accessor */
	  enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	  SS_ADDR_TYPE addr,		/* target address to access */
	  void *vp,			/* host memory address to access */
	  int nbytes);			/* number of bytes to access */

/* copy NBYTES through a memory access function, NBYTES must be a multiple
   of 4 bytes, this function is faster than mem_bcopy() */
void
mem_bcopy4(mem_access_fn mem_fn,	/* user-specified memory accessor */
	   enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	   SS_ADDR_TYPE addr,		/* target address to access */
	   void *vp,			/* host memory address to access */
	   int nbytes);			/* number of bytes to access */

/* zero out NBYTES through a memory access function */
void
mem_bzero(mem_access_fn mem_fn,		/* user-specified memory accessor */
	  SS_ADDR_TYPE addr,		/* target address to access */
	  int nbytes);			/* number of bytes to clear */

/* register memory system-specific options */
void
mem_reg_options(struct opt_odb_t *odb);	/* options data base */

/* check memory system-specific option values */
void
mem_check_options(struct opt_odb_t *odb,/* options data base */
		  int argc, char **argv);/* simulator arguments */

/* register memory system-specific statistics */
void
mem_reg_stats(struct stat_sdb_t *sdb);	/* stats data base */

/* initialize memory system */
void mem_init(void);			/* call before loader.c */
void mem_init1(void);			/* call after loader.c */

/* print out memory system configuration */
void mem_aux_config(FILE *stream);	/* output stream */

/* dump memory system stats */
void mem_aux_stats(FILE *stream);	/* output stream */

/* dump a block of memory */
void
mem_dump(mem_access_fn mem_fn,		/* user-specified memory access */
	 SS_ADDR_TYPE addr,		/* target address to dump */
	 int len,			/* number bytes to dump */
	 FILE *stream);			/* output stream */

#endif /* MEMORY_H */
