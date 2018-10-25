/*
 * loader.c - program loader routines
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
 * $Id: loader.c,v 3.1 1998/04/17 16:51:35 skadron Exp $
 *
 * $Log: loader.c,v $
 * Revision 3.1  1998/04/17 16:51:35  skadron
 * Versions used thru Mar '98 (retstack sub, ruu_resub, ICS final manuscript)
 *
 * Revision 2.101  1997/12/09 22:20:31  skadron
 * Version Pritpal used for ISCA-25 submission
 *
 * Revision 2.100  1997/12/03 20:05:46  skadron
 * Version Kevin used for ISCA-25 submission
 *
 * Revision 2.4  1997/08/05 01:14:42  skadron
 * Don't need to include regs.h
 *
 * Revision 2.3  1997/07/11 21:44:19  skadron
 * Updated to incorporate final changes for public 2.0 release
 *
 * Revision 2.2  1997/07/08 03:45:58  skadron
 * Updated to reflect Todd/Milo's version 2.0e; includes memory-leak fix
 *
 * Revision 1.6  1997/04/16  22:09:05  taustin
 * added standalone loader support
 *
 * Revision 1.5  1997/03/11  01:12:39  taustin
 * updated copyright
 * swapping supported disabled until it can be tested further
 *
 * Revision 1.4  1997/01/06  15:59:22  taustin
 * stat_reg calls now do not initialize stat variable values
 * ld_prog_fname variable exported
 *
 * Revision 1.3  1996/12/27  15:51:28  taustin
 * updated comments
 *
 * Revision 1.1  1996/12/05  18:52:32  taustin
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#ifdef BFD_LOADER
#include <bfd.h>
#else /* !BFD_LOADER */
#include "ecoff.h"
#endif /* BFD_LOADER */
#include "misc.h"
#include "ss.h"
#include "memory.h"
#include "sim.h"
#include "loader.h"

/* program text (code) segment base */
SS_ADDR_TYPE ld_text_base = 0;

/* program text (code) size in bytes */
unsigned int ld_text_size = 0;

/* program initialized data segment base */
SS_ADDR_TYPE ld_data_base = 0;

/* program initialized ".data" and uninitialized ".bss" size in bytes */
unsigned int ld_data_size = 0;

/* program stack segment base (highest address in stack) */
SS_ADDR_TYPE ld_stack_base = SS_STACK_BASE;

/* program initial stack size */
unsigned int ld_stack_size = 0;

/* program file name */
char *ld_prog_fname = NULL;

/* program entry point (initial PC) */
SS_ADDR_TYPE ld_prog_entry = 0;

/* program environment base address address */
SS_ADDR_TYPE ld_environ_base = 0;

/* target executable endian-ness, non-zero if big endian */
int ld_target_big_endian;

/* register simulator-specific statistics */
void
ld_reg_stats(struct stat_sdb_t *sdb)	/* stats data base */
{
  stat_reg_uint(sdb, "ld_text_base",
		"program text (code) segment base",
		(unsigned int *)&ld_text_base, ld_text_base, "  0x%08x");
  stat_reg_uint(sdb, "ld_text_size",
		"program text (code) size in bytes",
		&ld_text_size, ld_text_size, NULL);
  stat_reg_uint(sdb, "ld_data_base",
		"program initialized data segment base",
		(unsigned int *)&ld_data_base, ld_data_base, "  0x%08x");
  stat_reg_uint(sdb, "ld_data_size",
		"program init'ed `.data' and uninit'ed `.bss' size in bytes",
		&ld_data_size, ld_data_size, NULL);
  stat_reg_uint(sdb, "ld_stack_base",
		"program stack segment base (highest address in stack)",
		(unsigned int *)&ld_stack_base, ld_stack_base, "  0x%08x");
  stat_reg_uint(sdb, "ld_stack_size",
		"program initial stack size",
		&ld_stack_size, ld_stack_size, NULL);
  stat_reg_uint(sdb, "ld_prog_entry",
		"program entry point (initial PC)",
		(unsigned int *)&ld_prog_entry, ld_prog_entry, "  0x%08x");
  stat_reg_uint(sdb, "ld_environ_base",
		"program environment base address address",
		(unsigned int *)&ld_environ_base, ld_environ_base, "  0x%08x");
  stat_reg_int(sdb, "ld_target_big_endian",
	       "target executable endian-ness, non-zero if big endian",
	       &ld_target_big_endian, ld_target_big_endian, NULL);
}


/* load program text and initialized data into simulated virtual memory
   space and initialize program segment range variables */
void
ld_load_prog(mem_access_fn mem_fn,	/* user-specified memory accessor */
	     int argc, char **argv,	/* simulated program cmd line args */
	     char **envp,		/* simulated program environment */
	     int zero_bss_segs)		/* zero uninit data segment? */
{
  int i;
  SS_WORD_TYPE temp;
  SS_ADDR_TYPE sp, data_break = 0, null_ptr = 0, argv_addr, envp_addr;

#ifdef BFD_LOADER

  bfd *abfd;
  asection *sect;

  /* set up a local stack pointer, this is where the argv and envp
     data is written into program memory */
  ld_stack_base = SS_STACK_BASE;
  sp = ROUND_DOWN(SS_STACK_BASE - SS_MAX_ENVIRON, sizeof(SS_DOUBLE_TYPE));
  ld_stack_size = ld_stack_base - sp;

  /* initial stack pointer value */
  ld_environ_base = sp;

  /* load the program into memory, try both endians */
  if (!(abfd = bfd_openr(argv[0], "ss-coff-big")))
    if (!(abfd = bfd_openr(argv[0], "ss-coff-little")))
      fatal("cannot open executable `%s'", argv[0]);

  /* this call is mainly for its side effect of reading in the sections.
     we follow the traditional behavior of `strings' in that we don't
     complain if we don't recognize a file to be an object file.  */
  if (!bfd_check_format(abfd, bfd_object))
    {
      bfd_close(abfd);
      fatal("cannot open executable `%s'", argv[0]);
    }

  /* record profile file name */
  ld_prog_fname = argv[0];

  /* record endian of target */
  ld_target_big_endian = abfd->xvec->byteorder_big_p;

  debug("processing %d sections in `%s'...",
	bfd_count_sections(abfd), argv[0]);

  /* read all sections in file */
  for (sect=abfd->sections; sect; sect=sect->next)
    {
      char *p;

      debug("processing section `%s', %d bytes @ 0x%08x...",
	    bfd_section_name(abfd, sect), bfd_section_size(abfd, sect),
	    bfd_section_vma(abfd, sect));

      /* read the section data, if allocated and loadable and non-NULL */
      if ((bfd_get_section_flags(abfd, sect) & SEC_ALLOC)
	  && (bfd_get_section_flags(abfd, sect) & SEC_LOAD)
	  && bfd_section_vma(abfd, sect)
	  && bfd_section_size(abfd, sect))
	{
	  /* allocate a section buffer */
	  p = calloc(bfd_section_size(abfd, sect), sizeof(char));
	  if (!p)
	    fatal("cannot allocate %d bytes for section `%s'",
		  bfd_section_size(abfd, sect), bfd_section_name(abfd, sect));

	  if (!bfd_get_section_contents(abfd, sect, p, (file_ptr)0,
					bfd_section_size(abfd, sect)))
	    fatal("could not read entire `%s' section from executable",
		  bfd_section_name(abfd, sect));

	  /* copy program section it into simulator target memory */
	  mem_bcopy(mem_fn, Write, bfd_section_vma(abfd, sect),
		    p, bfd_section_size(abfd, sect));

	  /* release the section buffer */
	  free(p);
	}
      /* zero out of the section if it is loadable but not allocated in exec */
      else if (zero_bss_segs
	       && (bfd_get_section_flags(abfd, sect) & SEC_LOAD)
	       && bfd_section_vma(abfd, sect)
	       && bfd_section_size(abfd, sect))
	{
	  /* zero out the section region */
	  mem_bzero(mem_fn,
		    bfd_section_vma(abfd, sect), bfd_section_size(abfd, sect));
	}
      else
	{
	  /* else do nothing with this section, it's probably debug data */
	  debug("ignoring section `%s' during load...",
		bfd_section_name(abfd, sect));
	}

      /* expected text section */
      if (!strcmp(bfd_section_name(abfd, sect), ".text"))
	{
	  /* .text section processing */
	  ld_text_size =
	    ((bfd_section_vma(abfd, sect) + bfd_section_size(abfd, sect))
	     - SS_TEXT_BASE) + /* for speculative fetches/decodes */128;
	}
      /* expected data sections */
      else if (!strcmp(bfd_section_name(abfd, sect), ".rdata")
	       || !strcmp(bfd_section_name(abfd, sect), ".data")
	       || !strcmp(bfd_section_name(abfd, sect), ".sdata")
	       || !strcmp(bfd_section_name(abfd, sect), ".bss")
	       || !strcmp(bfd_section_name(abfd, sect), ".sbss"))
	{
	  /* data section processing */
	  if (bfd_section_vma(abfd, sect) + bfd_section_size(abfd, sect) >
	      data_break)
	    data_break = (bfd_section_vma(abfd, sect) +
			  bfd_section_size(abfd, sect));
	}
      else
	{
	  /* what is this section??? */
	  fatal("encountered unknown section `%s', %d bytes @ 0x%08x",
		bfd_section_name(abfd, sect), bfd_section_size(abfd, sect),
		bfd_section_vma(abfd, sect));
	}
    }

  /* compute data segment size from data break point */
  ld_text_base = SS_TEXT_BASE;
  ld_data_base = SS_DATA_BASE;
  ld_prog_entry = bfd_get_start_address(abfd);
  ld_data_size = data_break - ld_data_base;

  /* done with the executable, close it */
  if (!bfd_close(abfd))
    fatal("could not close executable `%s'", argv[0]);

#else /* !BFD_LOADER, i.e., standalone loader */

  FILE *fobj;
  long floc;
  struct ecoff_filehdr fhdr;
  struct ecoff_aouthdr ahdr;
  struct ecoff_scnhdr shdr;

  /* set up a local stack pointer, this is where the argv and envp
     data is written into program memory */
  ld_stack_base = SS_STACK_BASE;
  sp = ROUND_DOWN(SS_STACK_BASE - SS_MAX_ENVIRON, sizeof(SS_DOUBLE_TYPE));
  ld_stack_size = ld_stack_base - sp;

  /* initial stack pointer value */
  ld_environ_base = sp;

  /* record profile file name */
  ld_prog_fname = argv[0];

  /* load the program into memory, try both endians */
#ifdef __CYGWIN32__
  fobj = fopen(argv[0], "rb");
#else
  fobj = fopen(argv[0], "r");
#endif
  if (!fobj)
    fatal("cannot open executable `%s'", argv[0]);

  if (fread(&fhdr, sizeof(struct ecoff_filehdr), 1, fobj) < 1)
    fatal("cannot read header from executable `%s'", argv[0]);

  /* record endian of target */
  if (fhdr.f_magic == ECOFF_EB_MAGIC)
    ld_target_big_endian = TRUE;
  else if (fhdr.f_magic == ECOFF_EL_MAGIC)
    ld_target_big_endian = FALSE;
  else
    fatal("bad magic number in executable `%s'", argv[0]);

  if (fread(&ahdr, sizeof(struct ecoff_aouthdr), 1, fobj) < 1)
    fatal("cannot read AOUT header from executable `%s'", argv[0]);

  data_break = SS_DATA_BASE + ahdr.dsize + ahdr.bsize;

#if 0
  Data_start = ahdr.data_start;
  Data_size = ahdr.dsize;
  Bss_size = ahdr.bsize;
  Bss_start = ahdr.bss_start;
  Gp_value = ahdr.gp_value;
  Text_entry = ahdr.entry;
#endif

  /* seek to the beginning of the first section header, the file header comes
     first, followed by the optional header (this is the aouthdr), the size
     of the aouthdr is given in Fdhr.f_opthdr */
  fseek(fobj, sizeof(struct ecoff_filehdr) + fhdr.f_opthdr, 0);

  debug("processing %d sections in `%s'...", fhdr.f_nscns, argv[0]);

  /* loop through the section headers */
  floc = ftell(fobj);
  for (i = 0; i < fhdr.f_nscns; i++)
    {
      char *p;

      if (fseek(fobj, floc, 0) == -1)
	fatal("could not reset location in executable");
      if (fread(&shdr, sizeof(struct ecoff_scnhdr), 1, fobj) < 1)
	fatal("could not read section %d from executable", i);
      floc = ftell(fobj);

      switch (shdr.s_flags)
	{
	case ECOFF_STYP_TEXT:
	  ld_text_size = ((shdr.s_vaddr + shdr.s_size) - SS_TEXT_BASE) + 128;

	  p = calloc(shdr.s_size, sizeof(char));
	  if (!p)
	    fatal("out of virtual memory");

	  if (fseek(fobj, shdr.s_scnptr, 0) == -1)
	    fatal("could not read `.text' from executable", i);
	  if (fread(p, shdr.s_size, 1, fobj) < 1)
	    fatal("could not read text section from executable");

	  /* copy program section it into simulator target memory */
	  mem_bcopy(mem_fn, Write, shdr.s_vaddr, p, shdr.s_size);

	  /* release the section buffer */
	  free(p);

#if 0
	  Text_seek = shdr.s_scnptr;
	  Text_start = shdr.s_vaddr;
	  Text_size = shdr.s_size / 4;
	  /* there is a null routine after the supposed end of text */
	  Text_size += 10;
	  Text_end = Text_start + Text_size * 4;
	  /* create_text_reloc(shdr.s_relptr, shdr.s_nreloc); */
#endif
	  break;

	case ECOFF_STYP_RDATA:
	  /* The .rdata section is sometimes placed before the text
	   * section instead of being contiguous with the .data section.
	   */
#if 0
	  Rdata_start = shdr.s_vaddr;
	  Rdata_size = shdr.s_size;
	  Rdata_seek = shdr.s_scnptr;
#endif
	  /* fall through */
	case ECOFF_STYP_DATA:
#if 0
	  Data_seek = shdr.s_scnptr;
#endif
	  /* fall through */
	case ECOFF_STYP_SDATA:
#if 0
	  Sdata_seek = shdr.s_scnptr;
#endif

	  p = calloc(shdr.s_size, sizeof(char));
	  if (!p)
	    fatal("out of virtual memory");

	  if (fseek(fobj, shdr.s_scnptr, 0) == -1)
	    fatal("could not read `.text' from executable", i);
	  if (fread(p, shdr.s_size, 1, fobj) < 1)
	    fatal("could not read text section from executable");

	  /* copy program section it into simulator target memory */
	  mem_bcopy(mem_fn, Write, shdr.s_vaddr, p, shdr.s_size);

	  /* release the section buffer */
	  free(p);

	  break;

	case ECOFF_STYP_BSS:
	  break;

	case ECOFF_STYP_SBSS:
	  break;
        }
    }

  /* compute data segment size from data break point */
  ld_text_base = SS_TEXT_BASE;
  ld_data_base = SS_DATA_BASE;
  ld_prog_entry = ahdr.entry;
  ld_data_size = data_break - ld_data_base;

  /* done with the executable, close it */
  if (fclose(fobj))
    fatal("could not close executable `%s'", argv[0]);

#endif /* BFD_LOADER */

  /* perform sanity checks on segment ranges */
  if (!ld_text_base || !ld_text_size)
    fatal("executable is missing a `.text' section");
  if (!ld_data_base || !ld_data_size)
    fatal("executable is missing a `.data' section");
  if (!ld_prog_entry)
    fatal("program entry point not specified");

  /* determine byte/words swapping required to execute on this host */
  sim_swap_bytes = (endian_host_byte_order() != endian_target_byte_order());
  if (sim_swap_bytes)
    {
#if 0 /* FIXME: disabled until further notice... */
      /* cross-endian is never reliable, why this is so is beyond the scope
	 of this comment, e-mail me for details... */
      fprintf(stderr, "sim: *WARNING*: swapping bytes to match host...\n");
      fprintf(stderr, "sim: *WARNING*: swapping may break your program!\n");
#else
      fatal("binary endian does not match host endian");
#endif
    }
  sim_swap_words = (endian_host_word_order() != endian_target_word_order());
  if (sim_swap_words)
    {
#if 0 /* FIXME: disabled until further notice... */
      /* cross-endian is never reliable, why this is so is beyond the scope
	 of this comment, e-mail me for details... */
      fprintf(stderr, "sim: *WARNING*: swapping words to match host...\n");
      fprintf(stderr, "sim: *WARNING*: swapping may break your program!\n");
#else
      fatal("binary endian does not match host endian");
#endif
    }

  /* write [argc] to stack */
  temp = SWAP_WORD(argc);
  mem_fn(Write, sp, &temp, sizeof(SS_WORD_TYPE));
  sp += sizeof(SS_WORD_TYPE);

  /* skip past argv array and NULL */
  argv_addr = sp;
  sp = sp + (argc + 1) * sizeof(SS_PTR_TYPE);

  /* save space for envp array and NULL */
  envp_addr = sp;
  for (i=0; envp[i]; i++)
    sp += sizeof(SS_PTR_TYPE);
  sp += sizeof(SS_PTR_TYPE);

  /* fill in the argv pointer array and data */
  for (i=0; i<argc; i++)
    {
      /* write the argv pointer array entry */
      temp = SWAP_WORD(sp);
      mem_fn(Write, argv_addr + i*sizeof(SS_PTR_TYPE),
	     &temp, sizeof(SS_PTR_TYPE));
      /* and the data */
      sp += mem_strcpy(mem_fn, Write, sp, argv[i]);
    }
  /* terminate argv array with a NULL */
  mem_fn(Write, argv_addr + i*sizeof(SS_PTR_TYPE),
	 &null_ptr, sizeof(SS_PTR_TYPE));

  /* write envp pointer array and data to stack */
  for (i = 0; envp[i]; i++)
    {
      /* write the envp pointer array entry */
      temp = SWAP_WORD(sp);
      mem_fn(Write, envp_addr + i*sizeof(SS_PTR_TYPE),
	     &temp, sizeof(SS_PTR_TYPE));
      /* and the data */
      sp += mem_strcpy(mem_fn, Write, sp, envp[i]);
    }
  /* terminate the envp array with a NULL */
  mem_fn(Write, envp_addr + i*sizeof(SS_PTR_TYPE),
	       &null_ptr, sizeof(SS_PTR_TYPE));

  /* did we tromp off the stop of the stack? */
  if (sp > ld_stack_base)
    {
      /* we did, indicate to the user that SS_MAX_ENVIRON must be increased,
	 alternatively, you can use a smaller environment, or fewer
	 command line arguments */
      fatal("environment overflow, increase SS_MAX_ENVIRON in ss.h");
    }

  debug("ld_text_base: 0x%08x  ld_text_size: 0x%08x",
	ld_text_base, ld_text_size);
  debug("ld_data_base: 0x%08x  ld_data_size: 0x%08x",
	ld_data_base, ld_data_size);
  debug("ld_stack_base: 0x%08x  ld_stack_size: 0x%08x",
	ld_stack_base, ld_stack_size);
  debug("ld_prog_entry: 0x%08x", ld_prog_entry);

}
