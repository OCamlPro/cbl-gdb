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
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string.h>

#include "profiler.h"
#include "params.h"
#include "utils.h"

using namespace std;

#define EQ ==
#define AND &&
#define OR ||

typedef vector<size_t> VINT;

typedef struct {
    size_t size;
    size_t current_index;
    char * data;
} WHOLEFILE;

#define OVERSIZED 16384

bool
fgets(char *buf,size_t max, WHOLEFILE &fd)
{
    // Like ::fgets, except it strips off the final '\n'
    bool retval = false;
    size_t index = 0;
    if( fd.current_index < fd.size ) {
        retval = true;
        while( index < max-1 ) {
            char ch = fd.data[fd.current_index++];
            if( ch EQ '\n' ) {
                break;
            }
            buf[index++] = ch;
        }
        buf[index++] = '\0';
    }
    return retval;
}

map<string,int>
GetInputFileMappings(WHOLEFILE &fd, const string &primary_file)
{
    PROFILER;
    map<string,int> retval;

    // Prime the pump with the primary COBOL source file name.  We absolutely
    // want this reference to be in there, but it might not actually be in the
    // .s file, as can happen when the primary COBOL source file doesn't
    // actually have any executable instructions in it, but is merely a
    // container for copybook includes.
    //
    // We'll start it with a value of negative one.  When (as is usually the
    // case) there is a .file reference to primary_file in the .s module,
    // the -1 will be replaced, and all will be well.
    //
    // Otherwise, FigureOutReplacements will end up replacing the -1 with
    // 1.  The generated .s file will have primary_file as ".file 1", which
    // will be peachy even if there aren't any actual ".loc 1" references.

    retval[primary_file] = -1;

    char ach[OVERSIZED];
    while( fgets(ach,sizeof(ach),fd) ) {
        if( strstr(ach,".file") ) {
            string s = ach;
            s = Trim(s);
            VSTRING tokens = Split(s,"\t ");
            if( tokens.size() >= 3 AND tokens[0] EQ ".file" AND isdigit(tokens[1][0]) ) {
                int nfile = STOI(tokens[1]);
                string file = StripQuotes(tokens[2]);
                retval[file] = nfile;
            }
        }
    }
    return retval;
}

VINT
FindFunctionReferences(WHOLEFILE &fd)
{
    PROFILER;
    set<int>cheating;

    VINT retval;
    int nline = 0;
    fd.current_index = 0;

    char ach[OVERSIZED];
    while( fgets(ach,sizeof(ach),fd) ) {
        if( strstr(ach,".globl") ) {
            string s = ach;
            VSTRING tokens = Split(s,"\t ");
            if( tokens[1] EQ ".globl" ) {
                cheating.insert(nline+4);  // This is where our synthetic .loc will go
            }
        }

        if( strstr(ach,"function") ) {
            string s = ach;
            VSTRING tokens = Split(s,"\t ");
            if( tokens[3] EQ "@function") {
                cheating.insert(nline+3);  // This is where our synthetic .loc will go; intel and arm compilers
            }
            if( tokens[3] EQ "#function" ) {
                cheating.insert(nline+4);  // This is where our synthetic .loc will go; sparc compiler
            }
        }
        nline += 1;
    }
    for( set<int>::const_iterator it=cheating.begin(); it!=cheating.end(); it++) {
        retval.push_back(*it);
    }

    return retval;
}

map<int,int>
FigureOutReplacements(const PARAMETERS &params,
                      const map<string,int> &input_file_mappings)
{
    PROFILER;
    map<int,int> retval;

    int next=2;
    for( map<string,int>::const_iterator it=input_file_mappings.begin();
            it != input_file_mappings.end();
            it ++) {
        if( it->first EQ params.c_filename ) {
            // Skip over the cobc-generated .c file
            retval[it->second] = 0;
            continue;
        }
        if( it->first.substr(it->first.length()-2) EQ ".h"
                OR it->first.substr(it->first.length()-2) EQ ".c") {
            // Skip over any xxxx.h file
            retval[it->second] = 0;
            continue;
        }
        if( it->first EQ params.cbl_filename ) {
            // Force the COBOL source file to be #1
            retval[it->second] = 1;
            continue;
        }
        retval[it->second] = next++;
    }

    return retval;
}

void
ReadEntireFile(const string &filename,WHOLEFILE &fd)
{
    PROFILER;
    FILE *f = fopen(filename.c_str(), "r");
    if( !f ) {
        cerr << "Couldn't the open input file " << filename << endl;
        exit(1);
    }
    // Get the length of the file
    fseek(f,0,SEEK_END);
    long physical_size = fd.size = ftell(f);
    fseek(f,0,SEEK_SET);

    // Leave room for a possible final '\n'
    fd.data = new char[physical_size+1];

    // Read it in
    size_t nread = fread(fd.data,1,fd.size,f);
    fd.size = nread; // This handles both Windows \r\n and Unix \n line endings.  With
    //               // Windows, the physical size is bigger than the number of bytes read,
    //               // because the read() process eliminates the '\r' characters.

    if( fd.data[fd.size-1] != '\n' ) {
        // Put in the final \n that isn't there.
        fd.size +=1 ;
        fd.data[fd.size-1] = '\n';
    }
    if(nread >= 0) {
        // This is always true, but it suppresses compiler warnings.
        fclose(f);
    }

    fd.current_index=0;
}

void
ExtractLOCData(char *s, int &fileno, char **ppRemainder )
{
    PROFILER;
    // Don't call this unless you know the line has .loc[ \t] in it
    char *p = strstr(s, ".loc");
    p += 4;
    while( *p AND isspace(*p) ) {
        p += 1;
        // skip whitespace
    }
    fileno = atoi(p);
    while( *p AND isdigit(*p) ) {
        p += 1;
        // skip the digits
    }
    *ppRemainder = p;
}

void
build(PARAMETERS &params,
      map<string,int> &input_file_mappings,
      map<int,int> &replacements,
      WHOLEFILE &fd,
      VINT &function_references)
{
    //PROFILER;

    string name = params.s_output_filename.WholePath();
    FILE *f = fopen(name.c_str(),"w");
    if( !f ) {
        cerr << "Couldn't open the output file " << params.s_output_filename.WholePath() << endl;
        exit(1);
    }

    // Start the file off with our filename:
    fprintf(f,"\t.file\t\"%s\"\n",params.cbl_filename.c_str());

    // And then, let's put the modified list of filenames right at the beginning:

    for(map<string,int>::const_iterator it=input_file_mappings.begin();
            it != input_file_mappings.end();
            it++
       ) {
        int oldnum = it->second;
        int newnum = replacements[oldnum];
        if( newnum ) {
            fprintf(f,"\t.file %d \"%s\"\n",newnum,it->first.c_str());
            //ss << "\t.file " << newnum << " \"" << it->first << "\"" << endl;
        }
    }

    int next_function_reference_index = 0;

    char ach[OVERSIZED];
    fd.current_index = 0;
    int i = -1;   // i is the record we are looking at right now

    fgets(ach,sizeof(ach),fd);  // Skip the original ".file blah.c" line
    i += 1;

    while(fgets(ach,sizeof(ach),fd)) {
        i += 1;
        // We have learned that the GDB trapping can get confused if there isn't
        // a .loc reference in the first few lines after a function declaration.

        // When we created the function_references array, we added the offset we
        // need to put a .loc into the right place.  So, when i = function_references,
        // it's time to put a .loc in.

        if(next_function_reference_index < (int)function_references.size()
                && i EQ (int)function_references[next_function_reference_index] ) {
            // We have found a match.
            next_function_reference_index += 1;

            // We are going to look ahead for the next valid .loc, and put
            // that same loc in right here.  The duplications don't seem to
            // cause GDB any problem.

            int remember = fd.current_index;

            char ach2[OVERSIZED];
            for(int i=0; i<4; i++) {
                // Skip the appropriate number of lines
                fgets(ach2,sizeof(ach2),fd);
            }

            while( fgets(ach2,sizeof(ach2),fd) ) {
                if( strstr(ach2,".loc") AND (strstr(ach2,".loc ") OR strstr(ach2,".loc\t")) ) {
                    int old_num;
                    char *premainder;
                    ExtractLOCData(ach2,old_num,&premainder);
                    int new_num = replacements[old_num];
                    if( new_num != 0 ) {
                        // Build the replacement line:
                        int new_line = atoi(premainder) - 1 ;
                        fprintf(f,"\t.loc %d %d\n",new_num,new_line);
                        break;
                    }
                }
            }
            fd.current_index = remember;
        }

        if( strstr(ach,".file") ) {
            // We've already taken care of all .file lines
            continue;
        }

        if( strstr(ach,".loc") AND (strstr(ach,".loc ") OR strstr(ach,".loc\t")) ) {
            int oldnum;
            char *premainder;
            ExtractLOCData(ach,oldnum,&premainder);
            int newnum = replacements[oldnum];
            if( newnum != 0 ) {
                // Rebuild the .loc line
                fprintf(f,"\t.loc %d%s\n",newnum,premainder);
            }
            continue;
        }

        fprintf(f,"%s\n",ach);
    }

    fclose(f);
}

int
main(int argc, char **argv)
{
    PROFILER;

    PARAMETERS params = GetParameters(argc, argv);
    if(!params.quiet) {
        cout << "sfix version " << VERSION << endl;
    }

    WHOLEFILE fd;
    ReadEntireFile(params.s_input_filename.WholePath(),fd);
    map<string,int>input_file_mappings = GetInputFileMappings(fd,params.cbl_filename);
    VINT function_references = FindFunctionReferences(fd);
    map<int,int>replacements = FigureOutReplacements(params,input_file_mappings);
    build(params,input_file_mappings,replacements,fd,function_references);

    return 0;
}

