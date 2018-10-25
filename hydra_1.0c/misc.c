/*
 * misc.c - miscellaneous routines
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
 * $Id: misc.c,v 3.4 1998/10/05 00:29:42 skadron Exp $
 *
 * $Log: misc.c,v $
 * Revision 3.4  1998/10/05 00:29:42  skadron
 * Minor tweak on mkfifo return code
 *
 * Revision 3.3  1998/09/03 15:30:38  skadron
 * Made ZFILE stuff available
 *
 * Revision 3.2  1998/08/27 21:06:17  skadron
 * Added my xfopen stuff, until Todd fixes zfopen
 *
 * Revision 3.1  1998/04/17 16:51:35  skadron
 * Versions used thru Mar '98 (retstack sub, ruu_resub, ICS final manuscript)
 *
 * Revision 2.101  1997/12/09 22:20:31  skadron
 * Version Pritpal used for ISCA-25 submission
 *
 * Revision 2.100  1997/12/03 20:05:46  skadron
 * Version Kevin used for ISCA-25 submission
 *
 * Revision 2.5  1997/09/16 15:59:58  skadron
 * Needs to #include sim.h if it's going to use 'outfile'
 *
 * Revision 2.4  1997/09/16 01:57:13  skadron
 * Changed output functions to use outfile
 *
 * Revision 2.3  1997/07/11 21:44:19  skadron
 * Updated to incorporate final changes for public 2.0 release
 *
 * Revision 2.2  1997/07/08 03:45:58  skadron
 * Updated to reflect Todd/Milo's version 2.0e; includes memory-leak fix
 *
 * Revision 1.5  1997/03/11  01:17:36  taustin
 * updated copyright
 * supported added for non-GNU C compilers
 *
 * Revision 1.4  1997/01/06  16:01:45  taustin
 * comments updated
 *
 * Revision 1.1  1996/12/05  18:52:32  taustin
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>
#include <unistd.h>

/* added by KS */
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "sim.h"
#include "misc.h"

/* verbose output flag */
int verbose = FALSE;

#ifdef DEBUG
/* active debug flag */
int debugging = FALSE;
#endif /* DEBUG */

/* fatal function hook, this function is called just before an exit
   caused by a fatal error, used to spew stats, etc. */
static void (*hook_fn)(FILE *stream) = NULL;

/* register a function to be called when an error is detected */
void
fatal_hook(void (*fn)(FILE *stream))	/* fatal hook function */
{
  hook_fn = fn;
}

/* declare a fatal run-time error, calls fatal hook function */
#ifdef __GNUC__
void
_fatal(char *file, char *func, int line, char *fmt, ...)
#else /* !__GNUC__ */
void
fatal(char *fmt, ...)
#endif /* __GNUC__ */
{
  va_list v;
  va_start(v, fmt);

  fprintf(outfile, "fatal: ");
  vfprintf(outfile, fmt, v);
#ifdef __GNUC__
  if (verbose)
    fprintf(outfile, " [%s:%s, line %d]", func, file, line);
#endif /* __GNUC__ */
  fprintf(outfile, "\n");
  if (hook_fn)
    (*hook_fn)(outfile);
  exit(1);
}

/* declare a panic situation, dumps core */
#ifdef __GNUC__
void
_panic(char *file, char *func, int line, char *fmt, ...)
#else /* !__GNUC__ */
void
panic(char *fmt, ...)
#endif /* __GNUC__ */
{
  va_list v;
  va_start(v, fmt);

  fprintf(outfile, "panic: ");
  vfprintf(outfile, fmt, v);
#ifdef __GNUC__
  fprintf(outfile, " [%s:%s, line %d]", func, file, line);
#endif /* __GNUC__ */
  fprintf(outfile, "\n");
  if (hook_fn)
    (*hook_fn)(outfile);
  abort();
}

/* declare a warning */
#ifdef __GNUC__
void
_warn(char *file, char *func, int line, char *fmt, ...)
#else /* !__GNUC__ */
void
warn(char *fmt, ...)
#endif /* __GNUC__ */
{
  va_list v;
  va_start(v, fmt);

  fprintf(outfile, "warning: ");
  vfprintf(outfile, fmt, v);
#ifdef __GNUC__
  if (verbose)
    fprintf(outfile, " [%s:%s, line %d]", func, file, line);
#endif /* __GNUC__ */
  fprintf(outfile, "\n");
}

/* print general information */
#ifdef __GNUC__
void
_info(char *file, char *func, int line, char *fmt, ...)
#else /* !__GNUC__ */
void
info(char *fmt, ...)
#endif /* __GNUC__ */
{
  va_list v;
  va_start(v, fmt);

  vfprintf(outfile, fmt, v);
#ifdef __GNUC__
  if (verbose)
    fprintf(outfile, " [%s:%s, line %d]", func, file, line);
#endif /* __GNUC__ */
  fprintf(outfile, "\n");
}

#ifdef DEBUG
/* print a debugging message */
#ifdef __GNUC__
void
_debug(char *file, char *func, int line, char *fmt, ...)
#else /* !__GNUC__ */
void
debug(char *fmt, ...)
#endif /* __GNUC__ */
{
    va_list v;
    va_start(v, fmt);

    if (debugging)
      {
        fprintf(outfile, "debug: ");
        vfprintf(outfile, fmt, v);
#ifdef __GNUC__
        fprintf(outfile, " [%s:%s, line %d]", func, file, line);
#endif
        fprintf(outfile, "\n");
      }
}
#endif /* DEBUG */


/* copy a string to a new storage allocation (NOTE: many machines are missing
   this trivial function, so I funcdup() it here...) */
char *				/* duplicated string */
mystrdup(char *s)		/* string to duplicate to heap storage */
{
  char *buf;

  if (!(buf = (char *)malloc(strlen(s)+1)))
    return NULL;
  strcpy(buf, s);
  return buf;
}

/* find the last occurrence of a character in a string */
char *
mystrrchr(char *s, char c)
{
  char *rtnval = 0;

  do {
    if (*s == c)
      rtnval = s;
  } while (*s++);

  return rtnval;
}

/* case insensitive string compare (NOTE: many machines are missing this
   trivial function, so I funcdup() it here...) */
int				/* compare result, see strcmp() */
mystricmp(char *s1, char *s2)	/* strings to compare, case insensitive */
{
  unsigned char u1, u2;

  for (;;)
    {
      u1 = (unsigned char)*s1++; u1 = tolower(u1);
      u2 = (unsigned char)*s2++; u2 = tolower(u2);

      if (u1 != u2)
	return u1 - u2;
      if (u1 == '\0')
	return 0;
    }
}

/* allocate some core, this memory has overhead no larger than a page
   in size and it cannot be released. the storage is returned cleared */
void *
getcore(int nbytes)
{
#define PURIFY
#ifndef PURIFY
  void *p = (void *)sbrk(nbytes);

  if (p == (void *)-1)
    return NULL;

  /* this may be superfluous */
#if defined(__svr4__)
  memset(p, '\0', nbytes);
#else /* !defined(__svr4__) */
  bzero(p, nbytes);
#endif
  return p;
#else
  return calloc(nbytes, 1);
#endif /* PURIFY */
}

/* return log of a number to the base 2 */
int
log_base2(int n)
{
  int power = 0;

  if (n <= 0 || (n & (n-1)) != 0)
    panic("log2() only works for positive power of two values");

  while (n >>= 1)
    power++;

  return power;
}

/* return string describing elapsed time, passed in SEC in seconds */
char *
elapsed_time(long sec)
{
  static char tstr[256];
  char temp[256];

  if (sec <= 0)
    return "0s";

  tstr[0] = '\0';

  /* days */
  if (sec >= 86400)
    {
      sprintf(temp, "%ldD ", sec/86400);
      strcat(tstr, temp);
      sec = sec % 86400;
    }
  /* hours */
  if (sec >= 3600)
    {
      sprintf(temp, "%ldh ", sec/3600);
      strcat(tstr, temp);
      sec = sec % 3600;
    }
  /* mins */
  if (sec >= 60)
    {
      sprintf(temp, "%ldm ", sec/60);
      strcat(tstr, temp);
      sec = sec % 60;
    }
  /* secs */
  if (sec >= 1)
    {
      sprintf(temp, "%lds ", sec);
      strcat(tstr, temp);
    }
  tstr[strlen(tstr)-1] = '\0';
  return tstr;
}

/* assume bit positions numbered 31 to 0 (31 high order bit), extract num bits
   from word starting at position pos (with pos as the high order bit of those
   to be extracted), result is right justified and zero filled to high order
   bit, for example, extractl(word, 6, 3) w/ 8 bit word = 01101011 returns
   00000110 */
unsigned int
extractl(int word,		/* the word from which to extract */
         int pos,		/* bit positions 31 to 0 */
         int num)		/* number of bits to extract */
{
    return(((unsigned int) word >> (pos + 1 - num)) & ~(~0 << num));
}

 /* FIXME: not portable... */

static struct {
  char *type;
  char *ext;
  char *cmd;
} zfcmds[] = {
  /* type */	/* extension */		/* command */
  { "r",	".gz",			"gzip -dc < %s" },
  { "r",	".Z",			"uncompress -c < %s" },
  { "rb",	".gz",			"gzip -dc < %s" },
  { "rb",	".Z",			"uncompress -c < %s" },
  { "w",	".gz",			"gzip > %s" },
  { "w",	".Z",			"compress > %s" },
  { "wb",	".gz",			"gzip > %s" },
  { "wb",	".Z",			"compress > %s" }
};

/* same semantics as fopen() except that filenames ending with a ".gz" or ".Z"
   will be automagically get compressed */
ZFILE *
zfopen(char *fname, char *type)
{
  int i;
  char *cmd = NULL, *ext;
  ZFILE *zfd;

  /* get the extension */
  ext = mystrrchr(fname, '.');
  if (ext != NULL)
    {
      if (*ext != '\0')
	{
	  for (i=0; i < N_ELT(zfcmds); i++)
	    if (!strcmp(zfcmds[i].type, type) && !strcmp(zfcmds[i].ext, ext))
	      {
		cmd = zfcmds[i].cmd;
		break;
	      }
	}
    }

  zfd = (ZFILE *)calloc(1, sizeof(ZFILE));
  if (!zfd)
    fatal("out of virtual memory");
  
  if (cmd)
    {
      char str[2048];

      sprintf(str, cmd, fname);

      zfd->ft = ft_prog;
      zfd->fd = popen(str, type);
    }
  else
    {
      zfd->ft = ft_file;
      zfd->fd = fopen(fname, type);
    }

  if (!zfd->fd)
    {
      free(zfd);
      zfd = NULL;
    }
  return zfd;
}

int
zfclose(ZFILE *zfd)
{
  switch (zfd->ft)
    {
    case ft_file:
      fclose(zfd->fd);
      zfd->fd = NULL;
      break;

    case ft_prog:
      pclose(zfd->fd);
      zfd->fd = NULL;
      break;

    default:
      panic("bogus file type");
    }

  free(zfd);

  return TRUE;
}



/****
 * Until Todd's cleaner method is fixed:
 ****/

static int 
is_gz(char *filename)
{
  char *sz, *sz_prev;
  char buf[1024];

  strcpy(buf, filename);
  
  sz = sz_prev = strtok(buf, "/.");
  while ((sz = strtok(NULL, "/.")) != NULL)
    sz_prev = sz;

  if (strcmp(sz_prev, "gz") == 0)
    return TRUE;
  else
    return FALSE;
}

static FILE *
setup_gzip(char *filename, 	/* file that will contain gzip'd output */
	   char *szMode,   	/* 'r' or 'w' */
	   pid_t *pid, 		/* OUTPUT: pid of child */
	   char *fifo_name)	/* OUTPUT: name of fifo file */
{
  FILE *fp;
  int ok;
  char *sz, *sz_prev;
  char buf[16];

  strcpy(fifo_name, filename);
  sz = sz_prev = strstr(fifo_name, "gz");
  while ((sz = strstr(sz, "gz")) != sz_prev)
    sz_prev = sz;

  assert(sz_prev != NULL);
  sprintf(buf, "%d%s", getpid(), szMode);
  strcpy(sz, buf);
  strcat(sz, ".fifo");

  ok = mkfifo(fifo_name, S_IFIFO | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (ok)
    {
      char buf[1024];
      sprintf(buf, "Couldn't open FIFO %s", fifo_name);
      perror(buf);
      exit(1);
    }

  if ((*pid = fork()) != 0)
    /* Parent, ie generating the data */
    {
      fp = fopen(fifo_name, szMode);
      if (fp == NULL)
        {
          char buf[1024];
          sprintf(buf, "Couldn't open FIFO %s", fifo_name);
          perror(buf);
          exit(1);
        }
    }
  else
    /* Child, id gzip */
    {
      FILE *fp1, *fp2;
      char *zipapp;
      char buf[1024];

      if (szMode[0] == 'w')
        {
          fp1 = freopen(fifo_name, "r", stdin);
          fp2 = freopen(filename, "w", stdout);
          zipapp = "gzip";
        }
      else if (szMode[0] == 'r')
        {
          fp1 = freopen(fifo_name, "w", stdout);
          fp2 = freopen(filename, "r", stdin);
          zipapp = "gunzip";
        }
      else
	{
	  fprintf(stderr,
		  "setup_gzip: can't handle mode '%s', only 'r' or 'w'\n", 
		  szMode);
	  exit(1);
	}

      if (fp1 == NULL || fp2 == NULL)
        {
          sprintf(buf, "Couldn't open input/output files for %s'ing %s", 
                  zipapp, filename);
          perror(buf);
          exit(1);
        }

      (void)execlp(zipapp, zipapp, "-c", 0);
      sprintf(buf, "Couldn't execlp %s", zipapp);
      perror(buf);
      exit(1);
    }

  return fp;
}

static FILE *
setup_normal(char *filename, char *szMode)
{
  FILE *fp = fopen(filename, szMode);
  if (fp == NULL)
    {
      char buf[512];
      sprintf(buf, "Error -- couldn't open %s for action \"%s\"", 
              filename, szMode);
      perror(buf);
      exit(1);
    }

  return fp;
}



/***
 * tracking structure for fp-opens *
 ***/
#define MAX_XFOPENS 16
static nXfopen = 0;

struct xfopen_rec
{
  int fileno;
  int isZip;
  char *name;
  pid_t pid;
};
static struct xfopen_rec xfopen_db[MAX_XFOPENS];


FILE *
xfopen(char *filename, char *szMode)
{
  char fifoname[1024];
  pid_t pid;
  FILE *fp;
  int isZip = is_gz(filename);

  if (nXfopen >= MAX_XFOPENS)
    {
      fprintf(stderr, "xfopen: too many xfopen's\n");
      exit(1);
    }

  if (isZip)
    fp = setup_gzip(filename, szMode, &pid, fifoname);
  else
    fp = setup_normal(filename, szMode);
  assert(fp);

  xfopen_db[nXfopen].fileno = fileno(fp);
  xfopen_db[nXfopen].isZip = isZip;
  if (isZip)
    {
      xfopen_db[nXfopen].name = strdup(fifoname);
      if (!xfopen_db[nXfopen].name)
	{
	  fprintf(stderr, "xfopen: insufficient memory\n");
	  exit(1);
	}
      xfopen_db[nXfopen].pid = pid;
    }
  nXfopen++;

  return fp;
}

int
xfclose(FILE *stream)
{
  int i, fno, retval;

  if (stream == NULL)
    return ENXIO;

  fno = fileno(stream);
  if (fno == -1)
    return ENXIO;

  for (i = 0; i < nXfopen; i++)
    {
      if (xfopen_db[i].fileno == fno)
	break;
    }
  if (i >= MAX_XFOPENS)
    {
      fprintf(stderr, 
	      "Warning, xfopen: couldn't find stream-to-close in xfopen_db;"
	      " using fclose\n");
      return fclose(stream);
    }

  retval = fclose(stream);

  if (xfopen_db[i].isZip)
    {
      waitpid(xfopen_db[i].pid, NULL, 0);
      if (unlink(xfopen_db[i].name) == -1)
	{
          char buf[1024];
          sprintf(buf, "Warning: couldn't unlink fifo %s", xfopen_db[i].name);
          perror(buf);
	}
    }

  return i;
}
