/*
 * expr.h - expression evaluator interfaces
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
 * $Id: eval.h,v 3.1 1998/04/17 16:51:35 skadron Exp $
 *
 * $Log: eval.h,v $
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
 *
 */

#ifndef EVAL_H
#define EVAL_H

#include <stdio.h>
#include <errno.h>
/* forward declarations */
struct eval_state_t;
struct eval_value_t;

/* an identifier evaluator, when an evaluator is instantiated, the user
   must supply a function of this type that returns the value of identifiers
   encountered in expressions */
typedef struct eval_value_t		  /* value of the identifier */
(*eval_ident_t)(struct eval_state_t *es); /* ident string in es->tok_buf */

/* expression tokens */
enum eval_token_t {
  tok_ident,		/* user-valued identifiers */
  tok_const,		/* numeric literals */
  tok_plus,		/* `+' */
  tok_minus,		/* `-' */
  tok_mult,		/* `*' */
  tok_div,		/* `/' */
  tok_oparen,		/* `(' */
  tok_cparen,		/* `)' */
  tok_eof,		/* end of file */
  tok_whitespace,	/* ` ', `\t', `\n' */
  tok_invalid		/* unrecognized token */
};

/* an evaluator state record */
struct eval_state_t {
  char *p;			/* ptr to next char to consume from expr */
  char *lastp;			/* save space for token peeks */
  eval_ident_t f_eval_ident;	/* identifier evaluator */
  void *user_ptr;		/* user-supplied argument pointer */
  char tok_buf[512];		/* text of last token returned */
  enum eval_token_t peek_tok;	/* peek buffer, for one token look-ahead */
};

/* evaluation errors */
enum eval_err_t {
  ERR_NOERR,			/* no error */
  ERR_UPAREN,			/* unmatched parenthesis */
  ERR_NOTERM,			/* expression term is missing */
  ERR_DIV0,			/* divide by zero */
  ERR_BADCONST,			/* badly formed constant */
  ERR_BADEXPR,			/* badly formed constant */
  ERR_UNDEFVAR,			/* variable is undefined */
  ERR_EXTRA,			/* extra characters at end of expression */
  ERR_NUM
};

/* expression evaluation error, this must be a global */
extern enum eval_err_t eval_error /* = ERR_NOERR */;

/* enum eval_err_t -> error description string map */
extern char *eval_err_str[ERR_NUM];

/* expression value types */
enum eval_type_t {
  et_int,			/* signed integer result */
  et_uint,			/* unsigned integer result */
  et_float,			/* single-precision floating point value */
  et_double,			/* double-precision floating point value */
  et_symbol,			/* non-numeric result (!allowed in exprs)*/
  et_NUM
};

/* enum eval_type_t -> expression type description string map */
extern char *eval_type_str[et_NUM];

/* an expression value */
struct eval_value_t {
  enum eval_type_t type;		/* type of expression value */
  union {
    int as_int;				/* value for type == et_int */
    unsigned int as_uint;		/* value for type == et_uint */
    float as_float;			/* value for type == et_float */
    double as_double;			/* value for type == et_double */
    char *as_symbol;			/* value for type == et_symbol */
  } value;
};

/*
 * expression value arithmetic conversions
 */

/* eval_value_t (any numeric type) -> double */
double eval_as_double(struct eval_value_t val);

/* eval_value_t (any numeric type) -> float */
float eval_as_float(struct eval_value_t val);

/* eval_value_t (any numeric type) -> unsigned int */
unsigned int eval_as_uint(struct eval_value_t val);

/* eval_value_t (any numeric type) -> int */
int eval_as_int(struct eval_value_t val);

/* create an evaluator */
struct eval_state_t *			/* expression evaluator */
eval_new(eval_ident_t f_eval_ident,	/* user ident evaluator */
	 void *user_ptr);		/* user ptr passed to ident fn */

/* delete an evaluator */
void
eval_delete(struct eval_state_t *es);	/* evaluator to delete */

/* evaluate an expression, if an error occurs during evaluation, the
   global variable eval_error will be set to a value other than ERR_NOERR */
struct eval_value_t			/* value of the expression */
eval_expr(struct eval_state_t *es,	/* expression evaluator */
	  char *p,			/* ptr to expression string */
	  char **endp);			/* returns ptr to 1st unused char */

/* print an expression value */
void
eval_print(FILE *stream,		/* output stream */
	   struct eval_value_t val);	/* expression value to print */

#endif /* EVAL_H */
