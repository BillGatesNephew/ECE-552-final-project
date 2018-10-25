/*
 * sysprobe.c - host endian probe implementation
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
 * $Id: sysprobe.c,v 3.1 1998/04/17 16:51:35 skadron Exp $
 *
 * $Log: sysprobe.c,v $
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
 * Revision 1.4  1997/04/16  22:12:36  taustin
 * added standalone loader support
 *
 * Revision 1.3  1997/03/11  01:35:38  taustin
 * updated copyright
 * support added for portable SYMCAT()
 * -libs support added for portability
 * -flags support added for portability
 * various target supports added
 *
 * Revision 1.1  1996/12/05  18:52:32  taustin
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "misc.h"

#define HOST_ONLY
#include "endian.c"

#define CAT(a,b)	a/**/b

void
main(int argc, char **argv)
{
  int little_bytes = 0, little_words = 0;

  if (argc == 2 && !strcmp(argv[1], "-s"))
    {
      switch (endian_host_byte_order())
	{
	case endian_big:
	  fprintf(stdout, "big\n");
	  break;
	case endian_little:
	  fprintf(stdout, "little\n");
	  break;
	case endian_unknown:
	  fprintf(stderr, "\nerror: cannot determine byte order!\n");
	  exit(1);
	detault:
	  abort();
	}
    }
  else if (argc == 2 && !strcmp(argv[1], "-libs"))
    {
#ifdef BFD_LOADER
      fprintf(stdout, "-lbfd -liberty ");
#endif /* BFD_LOADER */

#ifdef linux
      fprintf(stdout, "-lbsd ");
#else
      /* nada */
#endif
      fprintf(stdout, " \n");
    }
  else if (argc == 1 || (argc == 2 && !strcmp(argv[1], "-flags")))
    {
      switch (endian_host_byte_order())
	{
	case endian_big:
	  fprintf(stdout, "-DBYTES_BIG_ENDIAN ");
	  break;
	case endian_little:
	  fprintf(stdout, "-DBYTES_LITTLE_ENDIAN ");
	  little_bytes = 1;
	  break;
	case endian_unknown:
	  fprintf(stderr, "\nerror: cannot determine byte order!\n");
	  exit(1);
	default:
	  abort();
	}

      switch (endian_host_word_order())
	{
	case endian_big:
	  fprintf(stdout, "-DWORDS_BIG_ENDIAN ");
	  break;
	case endian_little:
	  fprintf(stdout, "-DWORDS_LITTLE_ENDIAN ");
	  little_words = 1;
	  break;
	case endian_unknown:
	  fprintf(stderr, "\nerror: cannot determine word order!\n");
	  exit(1);
	default:
	  abort();
	}

#ifdef _AIX
	fprintf(stdout, "-D_ALL_SOURCE ");
#endif /* _AIX */

#ifndef __GNUC__
      /* probe compiler approach needed to concatenate symbols in CPP,
	 new style concatenation is always used with GNU GCC */
      {
	int i = 5, j;

	j = CAT(-,-i);

	if (j == 4)
	  {
	    /* old style symbol concatenation worked */
	    fprintf(stdout, "-DOLD_SYMCAT ");
	  }
	else if (j == 5)
	  {
	    /* old style symbol concatenation does not work, assume that
	       new style symbol concatenation works */
	    ;
	  }
	else
	  {
	    /* huh!?!?! */
	    fprintf(stderr, "\nerror: cannot grok symbol concat method!\n");
	    exit(1);
	  }
      }
#endif /* __GNUC__ */

    }

  /* check for different byte/word endian-ness */
  if (little_bytes != little_words)
    {
      fprintf(stderr,
	      "\nerror: opposite byte/word endian currently not supported!\n");
      exit(1);
    }
  exit(0);
}

