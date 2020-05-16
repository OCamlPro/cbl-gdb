/* #######################################################################
# Copyright (c) 2019-2020 Symas Corporation
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
#    * Neither the name of the Symas Corporation nor the names of its
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

#include <iostream>
#include <map>
#include <set>
#include <string.h>

#include "profiler.h"
#include "params.h"
#include "utils.h"

using namespace std;

#define EQ ==
#define AND &&
#define OR ||

typedef vector<size_t> VINT;

map<string,int>
GetInputFileMappings(const VSTRING &input_file, const string &primary_file)
{
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

    for(string s : input_file) {
        s = Trim(s);
        if( s.find(".file ") != string::npos) {
            VSTRING tokens = Split(s,"\t ");
            if( tokens.size() >= 3 AND tokens[0] EQ ".file" AND isdigit(tokens[1][0]) ) {
                int nfile = stoi(tokens[1]);
                string file = StripQuotes(tokens[2]);
                retval[file] = nfile;
            }
        }
    }
    return retval;
}

VINT
FindFunctionReferences(const VSTRING &input_file)
{
    set<int>cheating;

    VINT retval;
    int nline = 0;
    for(string s : input_file) {
        if( s.find(".globl") != string::npos) {
            VSTRING tokens = Split(s,"\t ");
            if( tokens[1] EQ ".globl" ) {
                cheating.insert(nline+4);  // This is where our synthetic .loc will go
            }
        }

        if( s.find("@function") != string::npos) {
            VSTRING tokens = Split(s,"\t ");
            if( tokens[3] EQ "@function" ) {
                cheating.insert(nline+3);  // This is where our synthetic .loc will go
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

VSTRING
ReadEntireFile(string filename)
{
    VSTRING retval;
    FILE *f = fopen(filename.c_str(), "r");
    if( !f ) {
        cerr << "Couldn't open input file " << filename << endl;
    }

    char ach[16384];
    while(fgets(ach,sizeof(ach),f)) {
        if(strlen(ach) AND ach[strlen(ach)-1] EQ '\n') {
            ach[strlen(ach)-1] = '\0';
        }
        retval.push_back(ach);
    }
    fclose(f);
    return retval;
}


int
main(int argc, char **argv)
{
    PROFILER;

    PARAMETERS params = GetParameters(argc, argv);
    if(!params.quiet) {
        cout << "sfix version " << VERSION << endl;
    }

    VSTRING input_file = ReadEntireFile(params.s_input_filename.WholePath());
    map<string,int>input_file_mappings = GetInputFileMappings(
            input_file
            , params.cbl_filename);
    map<int,int>replacements = FigureOutReplacements(params,input_file_mappings);
    VINT function_references = FindFunctionReferences(input_file);

    // Let's hang around here and do the hard work:

    stringstream ss;

    // Start the file off with our filename:
    ss << "\t.file\t\"" << params.cbl_filename << "\"" << endl;

    // And then, let's put the modified list of filenames right at the beginning:

    for(map<string,int>::const_iterator it=input_file_mappings.begin();
            it != input_file_mappings.end();
            it++
       ) {
        int oldnum = it->second;
        int newnum = replacements[oldnum];
        if( newnum ) {
            ss << "\t.file " << newnum << " \"" << it->first << "\"" << endl;
        }
    }

    int next_function_reference_index = 0;

    // Start at 1, rather than zero; we need to get rid of the original
    // "  .file blah.c " line.
    for( size_t i=1; i<input_file.size(); i++ ) {
        // We have learned that the GDB trapping can get confused if there isn't
        // a .loc reference in the first few lines after a function declaration.

        // When we created the function_references array, we added offset we
        // need to put a .loc into the right place.  So, when i = function_references,
        // it's time to put a .loc in.

        if(next_function_reference_index < (int)function_references.size()
                && i EQ function_references[next_function_reference_index] ) {
            // We have found a match.
            next_function_reference_index += 1;

            // We are going to look ahead for the next valid .loc, and put
            // that same loc in right here.  The duplications don't seem to
            // cause GDB any problem.

            size_t ii = i+4;
            while(ii < input_file.size() ) {
                VSTRING tokens = Split(input_file[ii]," \t");
                if( tokens.size() > 1 AND tokens[1] EQ ".loc" ) {
                    // We need to replace the old file number with the new one
                    int old_num = stoi(tokens[2]);
                    int new_num = replacements[old_num];
                    if( new_num != 0 ) {
                        // Build the replacement line:
                        int new_line = stoi(tokens[3])-1;

                        ss << "\t.loc";
                        ss << " " << new_num ;
                        ss << " " << new_line ;
                        size_t index = 5;
                        while( index < tokens.size() ) {
                            ss << " ";
                            ss << tokens[index++];
                        }
                        ss << endl;

                        break;
                    }
                }

                ii += 1;
            }
        }

        VSTRING tokens = Split(Trim(input_file[i])," \t");
        if( !tokens.empty() ) {
            if( tokens[0] EQ ".file" ) {
                // We've already taken care of all .file lines
                continue;
            }
            if( tokens[0] EQ ".loc" ) {
                // We need to replace the old file number with the new one
                int oldnum = stoi(tokens[1]);
                int newnum = replacements[oldnum];
                if( newnum != 0 ) {
                    // Rebuild the .loc line
                    ss << "\t";
                    ss << tokens[0];
                    ss << " ";
                    ss << newnum;
                    size_t index = 2;
                    while( index < tokens.size() ) {
                        ss << " ";
                        ss << tokens[index++];
                    }
                    ss << endl;
                }
                continue;
            }
        }

        if( input_file[i].find("endbr64") EQ string::npos ) {
            // When gcc went from version 8 to version 9, the compiler started
            // inserting endbr64 assembly language instructions.  It's safer, says
            // the documentation.
            //
            // They screw up our efforts to trick GDB into doing our bidding -- GDB loses
            // track of what lines to display after subroutine calls -- so I took the easy
            // way out and just removed the suckers.
            ss << input_file[i] << endl;
        }
    }

    FILE *f = fopen(params.s_output_filename.WholePath().c_str(),"w");
    if( !f ) {
        cerr << "Couldn't open input file " << params.s_output_filename.WholePath() << endl;
        exit(1);
    }
    fprintf(f,"%s",ss.str().c_str());
    fclose(f);
    return 0;
}

