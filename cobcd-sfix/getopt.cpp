/*    I found this on the internet, and it seems to be a perfectly good
    implementation of getopt(), and so I modified it slightly and, well,
    here it is.

    Bob Dubner, 2017-09-13

    */

/*-
* See the file LICENSE for redistribution information.
*
* Copyright (c) 1996,2007 Oracle.  All rights reserved.
*/
/*
* Copyright (c) 1987, 1993, 1994
*    The Regents of the University of California.  All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. Neither the name of the University nor the names of its contributors
*    may be used to endorse or promote products derived from this software
*    without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*
* $Id: getopt.c,v 12.6 2007/05/17 15:14:54 bostic Exp $
*/

#include <stdio.h>
#include <string.h>
#include "getopt.h"

int    opterr = 1,        /* if error message should be printed */
       optind = 1,            /* index into parent argv vector */
       optopt,                /* character checked for validity */
       optreset = 0;        /* reset getopt */
char const * optarg;    /* argument associated with option */

#undef    BADCH
#define    BADCH    (int)'?'
#undef    BADARG
#define    BADARG    (int)':'
#undef    EMSG
#define    EMSG    ""

int
getopt(int nargc, char * const * nargv, const char * ostr)
{
    static char *progname = *nargv;
    static char const * place = EMSG;        /* option letter processing */
    const char *oli;                /* option letter list index */

    if (optreset || !*place) {
        /* update scanning pointer */
        optreset = 0;
        if (optind >= nargc || *(place = nargv[optind]) != '-') {
            place = EMSG;
            return (EOF);
        }
        if (place[1] && *++place == '-') {
            /* found "--" */
            ++optind;
            place = EMSG;
            return (EOF);
        }
    }                    /* option letter okay? */
    if ((optopt = (int)*place++) == (int)':' || !(oli = strchr(ostr, optopt))) {
        /*
        * if the user didn't specify '-' as an option,
        * assume it means EOF.
        */
        if (optopt == (int)'-')
            return (EOF);
        if (!*place)
            ++optind;
        if (opterr && *ostr != ':')
            (void)fprintf(stderr,"%s: illegal option -- %c\n", progname, optopt);
        return (BADCH);
    }
    if (*++oli != ':') {
        /* don't need argument */
        optarg = NULL;
        if (!*place)
            ++optind;
    } else {
        /* need an argument */
        if (*place)            /* no white space */
            optarg = place;
        else if (nargc <= ++optind) {    /* no arg */
            place = EMSG;
            if (*ostr == ':')
                return (BADARG);
            if (opterr)
                (void)fprintf(stderr,"%s: option requires an argument -- %c\n",progname, optopt);
            return (BADCH);
        } else {
            /* white space */
            optarg = nargv[optind];
        }
        place = EMSG;
        ++optind;
    }
    return (optopt);            /* dump back option letter */
}

//#define TESTING_GETOPT
#ifdef TESTING_GETOPT

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "getopt.h"

void
testopt(int argc, char **argv)
{
    int aflag = 0;
    int bflag = 0;
    char *cvalue = NULL;
    int index;
    int c;

    opterr = 0;
    optind = 1;    // Only needed for repetitive starts

    printf("Input: testop ");
    for(int i=1; i<argc; i++) {
        printf("%s ",argv[i]);
    }
    printf("\n");

    while ((c = getopt (argc, argv, "abc:")) != -1)
        switch (c) {
        case 'a':
            aflag = 1;
            break;
        case 'b':
            bflag = 1;
            break;
        case 'c':
            cvalue = optarg;
            break;
        case '?':
            if (optopt == 'c')
                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
            return;
        default:
            abort ();
        }

    printf ("aflag = %d, bflag = %d, cvalue = %s\n",aflag, bflag, cvalue);

    for (index = optind; index < argc; index++)
        printf ("Non-option argument %s\n", argv[index]);
    printf("\n");
}

int
main()
{
    char *test1[]  = {"x"};
    char *test2[]  = {"x","-a","-b"};
    char *test3[]  = {"x","-ab"};
    char *test4[]  = {"x","-c","foo"};
    char *test5[]  = {"x","-cfoo"};
    char *test6[]  = {"x","arg1"};
    char *test7[]  = {"x","-a","arg1"};
    char *test8[]  = {"x","-c","foo","arg1"};
    char *test9[]  = {"x","-a","--","-b"};
    char *test10[] = {"x","-a","-"};

    testopt(sizeof(test1)/sizeof(test1[0]),test1);
    testopt(sizeof(test2)/sizeof(test2[0]),test2);
    testopt(sizeof(test3)/sizeof(test3[0]),test3);
    testopt(sizeof(test4)/sizeof(test4[0]),test4);
    testopt(sizeof(test5)/sizeof(test5[0]),test5);
    testopt(sizeof(test6)/sizeof(test6[0]),test6);
    testopt(sizeof(test7)/sizeof(test7[0]),test7);
    testopt(sizeof(test8)/sizeof(test8[0]),test8);
    testopt(sizeof(test9)/sizeof(test9[0]),test9);
    testopt(sizeof(test10)/sizeof(test10[0]),test10);

    return 0;
}

/* This is what should be the output of the testing routine:

% testopt
aflag = 0, bflag = 0, cvalue = (null)

% testopt -a -b
aflag = 1, bflag = 1, cvalue = (null)

% testopt -ab
aflag = 1, bflag = 1, cvalue = (null)

% testopt -c foo
aflag = 0, bflag = 0, cvalue = foo

% testopt -cfoo
aflag = 0, bflag = 0, cvalue = foo

% testopt arg1
aflag = 0, bflag = 0, cvalue = (null)
Non-option argument arg1

% testopt -a arg1
aflag = 1, bflag = 0, cvalue = (null)
Non-option argument arg1

% testopt -c foo arg1
aflag = 0, bflag = 0, cvalue = foo
Non-option argument arg1

% testopt -a -- -b
aflag = 1, bflag = 0, cvalue = (null)
Non-option argument -b

% testopt -a -
aflag = 1, bflag = 0, cvalue = (null)
Non-option argument -

*/

#endif
