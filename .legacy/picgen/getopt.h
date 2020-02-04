#pragma once

#ifndef _GETOPT_H
#define _GETOPT_H

extern int  opterr,        /* if error message should be printed */
       optind,        /* index into parent argv vector */
       optopt,        /* character checked for validity */
       optreset;      /* reset getopt */
extern char const * optarg;    /* argument associated with option */

int getopt(int nargc, char * const * nargv, const char * ostr);

#endif

