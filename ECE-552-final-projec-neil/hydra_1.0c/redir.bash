#!/usr/local/bin/bash -norc

#
# redir - redirect stdout and stderr to files
#
# This file is a part of the SimpleScalar tool suite written by
# Todd M. Austin as a part of the Multiscalar Research Project.
#  
# The tool suite is currently maintained by Doug Burger and Todd M. Austin.
# 
# Copyright (C) 1994, 1995, 1996, 1997 by Todd M. Austin
#
# This source file is distributed "as is" in the hope that it will be
# useful.  It is distributed with no warranty, and no author or
# distributor accepts any responsibility for the consequences of its
# use. 
#
# Everyone is granted permission to copy, modify and redistribute
# this source file under the following conditions:
#
#    This tool set is distributed for non-commercial use only. 
#    Please contact the maintainer for restrictions applying to 
#    commercial use of these tools.
#
#    Permission is granted to anyone to make or distribute copies
#    of this source code, either as received or modified, in any
#    medium, provided that all copyright notices, permission and
#    nonwarranty notices are preserved, and that the distributor
#    grants the recipient permission for further redistribution as
#    permitted by this document.
#
#    Permission is granted to distribute this file in compiled
#    or executable form under the same conditions that apply for
#    source code, provided that either:
#
#    A. it is accompanied by the corresponding machine-readable
#       source code,
#    B. it is accompanied by a written offer, with no time limit,
#       to give anyone a machine-readable copy of the corresponding
#       source code in return for reimbursement of the cost of
#       distribution.  This written offer must permit verbatim
#       duplication by anyone, or
#    C. it is distributed by someone who received only the
#       executable form, and is accompanied by a copy of the
#       written offer of source code that they received concurrently.
#
# In other words, you are welcome to use, share and improve this
# source file.  You are forbidden to forbid anyone else to use, share
# and improve what you give them.
#
# INTERNET: dburger@cs.wisc.edu
# US Mail:  1210 W. Dayton Street, Madison, WI 53706
#
# $Id: redir.bash,v 3.1 1998/04/17 16:51:35 skadron Exp $
#
# $Log: redir.bash,v $
# Revision 3.1  1998/04/17 16:51:35  skadron
# Versions used thru Mar '98 (retstack sub, ruu_resub, ICS final manuscript)
#
# Revision 2.101  1997/12/09 22:20:31  skadron
# Version Pritpal used for ISCA-25 submission
#
# Revision 2.100  1997/12/03 20:05:46  skadron
# Version Kevin used for ISCA-25 submission
#
# Revision 2.2  1997/07/11 21:44:19  skadron
# Updated to incorporate final changes for public 2.0 release
#
# Revision 2.1  1997/07/06 03:27:02  skadron
# Initial rev
#
# 

if [ $# -lt 3 ]
then
    echo Usage: redir \<stdout_file\> \<stderr_file\> \<command and args...\>;
    exit 1;
fi

out=$1
shift
err=$1
shift

$* >$out 2>$err
#($* >! $out) >&! $err
#exit $?
