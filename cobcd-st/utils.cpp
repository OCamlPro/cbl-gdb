/* #######################################################################
# Copyright (c) 2019-2020 COBOLworx Corporation
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in
#      the documentation and/or other materials provided with the
#      distribution.
#    * Neither the name of the COBOLworx Corporation nor the names of its
#      contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
############################################################################ */

#include <stdlib.h>
#include <sstream>
#include "utils.h"
#include "profiler.h"

#define EQ ==
#define AND &&
#define OR ||

using namespace std;

string
Trim(const string &s)
{
    PROFILER;
    string retval=s;
    static const string WHITESPACE = " \t\r\n\f";
    size_t nfound = retval.find_last_not_of(WHITESPACE);
    if(nfound != string::npos) {
        nfound += 1;
        retval = retval.substr(0,nfound);
    }
    nfound = retval.find_first_not_of(WHITESPACE);
    if(nfound != string::npos) {
        retval = retval.substr(nfound);
    } else {
        retval = "";
    }
    return retval;
}

string
RTrim(const string &s)
{
    PROFILER;
    string retval=s;
    static const string WHITESPACE = " \t\r\n\f";
    size_t nfound = retval.find_last_not_of(WHITESPACE);
    if(nfound != string::npos) {
        nfound += 1;
        retval = retval.substr(0,nfound);
    }
    return retval;
}

string
StripQuotes(const string &s)
{
    PROFILER;
    string retval=s;
    if( (retval.size() >= 1 AND retval[retval.size()-1] EQ '"') OR retval[retval.size()-1] EQ '\'' ) {
        retval = retval.substr(0,retval.size()-1);
    }
    if( (retval.size() >= 1 AND retval[0] EQ '"') OR retval[0] EQ '\'') {
        retval = retval.substr(1);
    }
    return retval;
}

vector<string>
Split(const string &s,const string &delimiters)
{
    PROFILER;
    vector<string>retval;

    size_t next = 0;
    string snext;
    while(next < s.size()) {
        char ch = s[next++];
        for(size_t i=0; i<delimiters.size(); i++) {
            if(ch EQ delimiters[i]) {
                if(!snext.empty()) {
                    retval.push_back(snext);
                    snext.clear();
                }
                goto next_c;
            }
        }
        snext += ch;
next_c:
        ;
    }
    if(!snext.empty()) {
        retval.push_back(snext);
    }

    return retval;
}

string
Replace(const string &s,const char was, const char is)
{
    PROFILER;
    string retval;

    for(size_t i=0; i<s.size(); i++) {
        char ch = s[i];
        if(ch EQ was) {
            ch = is;
        }
        retval += ch;
    }
    return retval;
}

string
Upper(const string &s)
{
    PROFILER;
    string retval;
    for(size_t i=0; i<s.size(); i++) {
        char ch = s[i];
        retval += toupper(ch);
    }
    return retval;
}

string
ZeroIsNull(const LONGLONG &v)
{
    PROFILER;
    string retval;
    stringstream ss;
    if( v != 0 ) {
        ss << v;
    }
    return ss.str();
}

int
STOI(const string &s)
{
    PROFILER;
    return atoi(s.c_str());
}

string
GetCommentText(const string &s)
{
    PROFILER;
    string retval;
    size_t comment = 0;
    for(;;) {
        size_t nfound = s.find("/*",comment);
        if(nfound EQ string::npos) {
            break;
        }
        comment = nfound+2;
    }
    if(comment > 0) {
        size_t end_comment = s.find("*/",comment);
        if(end_comment != string::npos) {
            retval = Trim(s.substr(comment,end_comment-comment));
        }
    }
    return retval;
}

void
SkipPast(PCHAR &p,char ch)
{
    while( *p++ != ch ) {
    }
}

char *
FindMatchingParenthesis(char *p)
{
    // Pretty simple-minded.  Assumes there are no comments involved, and
    // it assumes the string at p is well-formed.
    char *retval = p ;
    int pcount = 0;
    if( *p++ EQ '(' ) {
        pcount = 1;
    }
    while( pcount && *p ) {
        char ch = *p;
        if( ch EQ '(' ) {
            pcount += 1;
        } else if (ch EQ ')') {
            pcount -= 1;
            retval = p;
        }
        p += 1;
    }
    return retval;
}