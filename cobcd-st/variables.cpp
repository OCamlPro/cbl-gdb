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

#include <iostream>
#include <iomanip>
#include <map>
#include <sstream>
#include "variables.h"
#include "profiler.h"
#include "utils.h"

using namespace std;

#define EQ ==
#define AND &&
#define OR ||

void
VARIABLES::ScanCFile(const string &path, const string &fname)
{
    CPROFILER;
    string full_path = path;
    full_path += fname;
    full_path += ".c";

    FILE *f = fopen(full_path.c_str(),"r");
    if( !f ) {
        cout << "Couldn't open the input file " << full_path << endl;
        exit(1);
    }

    char ach[16384];

    string current_record_type = "X" ;
    while( fgets(ach,sizeof(ach),f) ) {
        if( strstr(ach, ": Entry ") ) {
            string input = ach;

            VSTRING tokens = Split(input," \t");
            if( tokens.size() EQ 9
                    AND tokens[0] EQ "/*"
                    AND tokens[1] EQ "Line:"
                    AND tokens[3] EQ ":"
                    AND tokens[4] EQ "Entry"
                    AND tokens[6] EQ ":"
                    AND tokens[8] EQ "*/\n" ) {
                VARIABLE variable;
                variable.record_type = "P";
                variable.offset = stoi(tokens[2]);
                variable.name = tokens[5];
                variable.f_name = tokens[7];
                Insert(variable);
                variable.record_type = "E";
                Insert(variable);
            }
            continue;
        }

        static const char COB_DUMP_FIELD[] = "cob_dump_field";
        static const char COB_SET_DATA[] = "COB_SET_DATA";
        static const char COB_SET_FLD[] = "COB_SET_FLD";
        char *p = ach;

        // Skip over spaces
        while( *p EQ ' ' OR *p EQ '\t' ) {
            p += 1;
        }
        if( strncmp(p, COB_DUMP_FIELD, strlen(COB_DUMP_FIELD)) EQ 0 ) {
            p += strlen(COB_DUMP_FIELD);

            SkipPast(p,'(');
            string level;
            while( *p != ',' ) {
                level += *p++;
            }

            if( stoi(level) EQ -2 ) {
                continue;
            }

            if( stoi(level) EQ -1 ) {
                if( strstr(p, "FD") ) {
                    current_record_type = "F";
                } else if( strstr(p, "SD") ) {
                    current_record_type = "S";
                } else if( strstr(p, "WORKING") ) {
                    current_record_type = "W";
                } else if( strstr(p, "LOCAL") ) {
                    current_record_type = "L";
                } else if( strstr(p, "LINKAGE") ) {
                    current_record_type = "I";
                } else {
                    cout << "We don't understand this line:" << endl;
                    cout << ach;
                    current_record_type = "X";
                }
                continue;
            }
            VARIABLE variable;
            variable.level = stoi(level);

            SkipPast(p,'\"');

            while( *p != '\"' ) {
                variable.name += *p++ ;
            }

            char *poccurs = strstr(ach," OCCURS ");
            char *redef = strstr(ach," REDEFINES ");
            if( poccurs OR redef ) {
                string s = GetCommentText(ach);
                VSTRING t = Split(s," ");

                // Just make sure what we are looking for is in comment
                // text, and not inside a fiendish text literal or really
                // weird variable name.  This probably isn't necessary; we already
                // know that we are on a cob_dump_field line.  And the spaces that
                // surround OCCURS and REDEFINES will prevent a false positive
                // on a variable named "REDEFINES-OCCURS", for example.
                //
                // But still...

                for( size_t i=0; i<t.size(); i++ ) {
                    if( t[i] EQ "REDEFINES") {
                        variable.redefines = true;
                        continue;
                    }
                    if( t[i] EQ "OCCURS ") {
                        variable.occurs_min = stoi(t[i+1]);
                        variable.occurs_max = stoi(t[i+2]);
                        continue;
                    }
                }
            }

            SkipPast(p,',');
            SkipPast(p,' ');

            if( *p EQ '&' ) {
                // Minimal association: name with f_name
                p += 1;
                while(*p != ',') {
                    variable.f_name += *p++;
                }
                variable.record_type = current_record_type;
                variables.push_back(variable);
            } else if( strncmp(p, COB_SET_DATA, strlen(COB_SET_DATA)) EQ 0 ) {
                p += strlen(COB_SET_DATA);
                SkipPast(p,'(');
                string s;
                while( *p != ')' ) {
                    if( *p != ' ' AND *p != '\t' ) {
                        s += *p;
                    }
                    p += 1;
                }
                VSTRING tokens = Split(s,",");
                if( tokens.size() EQ 2 ) {
                    variable.f_name = tokens[0];

                    VSTRING tokens_base_offset = Split(tokens[1],"+");
                    variable.b_name = tokens_base_offset[0];
                    for( size_t i=1; i<tokens_base_offset.size(); i++) {
                        variable.offset += stoi(tokens_base_offset[i]);
                    }
                    variable.record_type = current_record_type;
                    variables.push_back(variable);
                }
            } else if( strncmp(p, COB_SET_FLD, strlen(COB_SET_FLD)) EQ 0 ) {
                p += strlen(COB_SET_FLD);
                SkipPast(p,'(');
                string s;
                while( *p != ')' ) {
                    if( *p != ' ' AND *p != '\t' ) {
                        s += *p;
                    }
                    p += 1;
                }

                SkipPast(p,',');
                int occurrence = atoi(p);
                if(occurrence EQ 0) {
                    VSTRING tokens = Split(s," ,");

                    VSTRING tokens_base_offset = Split(tokens[2],"+");
                    variable.length = stoi(tokens[1]);
                    variable.b_name = tokens_base_offset[0];
                    for( size_t i=1; i<tokens_base_offset.size(); i++) {
                        variable.offset += stoi(tokens_base_offset[i]);
                    }
                    variable.a_name = tokens[3].substr(1);
                    variable.record_type = current_record_type;
                    variables.push_back(variable);
                }
            }
        }
    }

    ::fclose(f);
}

void
VARIABLES::FillInTheBlanks(COB_FIELDS &cob_fields)
{
    for( V_VARIABLES::iterator it = variables.begin(); it!=variables.end(); it++) {
        if( it->a_name.empty() ) {
            const COB_FIELD *pfield = cob_fields.Find(it->f_name);
            if( pfield ) {
                if( it->record_type EQ "L" ) {
                    // Local variables are all referenced against cob_local_ptr
                    it->b_name = "cob_local_ptr";
                } else {
                    if( !pfield->b_name.empty() ) {
                        it->b_name = pfield->b_name;
                    }
                }
                it->a_name = pfield->a_name;
                if( pfield->size) {
                    it->length = pfield->size;
                }
                if( pfield->offset) {
                    it->offset = pfield->offset;
                }
            }
        }
    }
}

void
VARIABLES::Dump() const
{
    CPROFILER;
    cout << "COB_FIELDS:" << endl;
    cout << setw( 3) << "RT" ;
    cout << setw( 4) << "lvl";
    cout << setw(20) << "name" ;
    cout << setw(20) << "f_name";
    cout << setw(20) << "b_name";
    cout << setw(20) << "a_name" ;
    cout << setw( 7) << "offset" ;
    cout << setw( 7) << "length";
    cout << setw( 6) << "o_min";
    cout << setw( 6) << "o_max";
    cout << setw( 6) << "redef";
    cout << endl;

    for( V_VARIABLES::const_iterator variable = variables.begin();
            variable!=variables.end();
            variable++) {
        cout << setw( 3) << variable->record_type;    // One character:
        cout << setw( 4) << variable->level;          // FD, SD, 01-49, 66, 77, 78, 88
        cout << setw(20) << variable->name;           // COBOL variable identifier
        cout << setw(20) << variable->f_name;         // Name of the associated cob_field variable
        cout << setw(20) << variable->b_name;         // Name of the location of the data
        cout << setw(20) << variable->a_name;         // Name of the associated cob_field_attrib variable
        cout << setw( 7) << variable->offset;         // When f_name exists, offset should have the same runtime value as f_name->data - b_name;
        cout << setw( 7) << variable->length;         // When f_name exists, length should be the same as f_name->size
        cout << setw( 6) << variable->occurs_min;     //
        cout << setw( 6) << variable->occurs_max;     // -1 for unbounded
        cout << setw( 6) << (variable->redefines ? "REDEF" : "");
        cout << endl;
    }
    cout << endl;
}

void
VARIABLES::FormatVariablesInfo(stringstream &ss) const
{
    CPROFILER;
    for( V_VARIABLES::const_iterator variable = variables.begin();
            variable!=variables.end();
            variable++) {
        if( variable->name EQ "FILLER" ) {
            continue;
        }
        ss << variable->record_type             << "|" ;
        ss << ZeroIsNull(variable->level)       << "|" ;
        ss << variable->name                    << "|" ;
        ss << variable->f_name                  << "|" ;
        ss << (variable->b_name EQ "cob_local_ptr" ? "" : variable->b_name) << "|" ;
        ss << variable->a_name                  << "|" ;
        ss << ZeroIsNull(variable->offset)      << "|" ;
        ss << ZeroIsNull(variable->length)      << "|" ;
        ss << ZeroIsNull(variable->occurs_min)  << "|" ;
        if( variable->occurs_min EQ 0 AND variable->occurs_max EQ 1 ) {
            // This is the default, so output nothing
            ss << "|" ;
        } else {
            ss << variable->occurs_max;
            ss << "|" ;
        }
        ss << "~";
    }
}

void
VARIABLES::Insert(const VARIABLE &v)
{
    variables.push_back(v);
}

static int
VARIABLE_LEVEL(int a)
{
    return ((a EQ 77 OR a EQ 78) ? 1 : a);
}

void
VARIABLES::FixLocalStorageOffsets()
{
    // The purpose of this routine is to cope with the fact that
    // GnuCOBOL doesn't generate LOCAL-VARIABLE offset information
    // for variables that are not referenced in the body of the code.

    // So, we have to infer the locations within cob_local_ptr by the
    // lengths of each variable.

    // This is further complicated by observation that 01/77 variables are
    // placed on 16-byte boundaries.

    // We also have to take into account the possibility of REDEFINES, which
    // reset a variable to the same offset as the previous one on the same
    // level

    // Use a <map> to prevent O(N-squared) in the highly-theoretical and extremely
    // unlikely limit when handling REDEFINES.
    //
    // It would take an extremely long time to become N-squared, and even that
    // would require a huge input file with a lot of REDEFINES and other odd
    // circumstances.  But good habits are good habits.

    map<int,V_VARIABLES::iterator>elder_sibling;

    for( V_VARIABLES::iterator variable = variables.begin();
            variable!=variables.end();
            variable++) {
        if( variable->level EQ 66 ) {
            // We can't handle Level 66 here
            continue;
        }

        if( variable->record_type EQ "L" ) {
            int our_level = VARIABLE_LEVEL(variable->level);

            if( variable->redefines ) {
                // Make sure our offset is the same as our elder sibling's offset
                map<int,V_VARIABLES::iterator>::iterator elder = elder_sibling.find(our_level);
                if( elder != elder_sibling.end() ) {
                    variable->offset = elder->second->offset;
                }
            }

            elder_sibling[our_level] = variable;

            if( variable->offset EQ 0 ) {
                // We have a possible rework here:
                // Find somebody older than us.  And, yes, this, too, is O(N-squared) in
                // the insane limit.  I half-thought of a couple of Nlog(N) ways of avoiding the N-squared,
                // and wouldn't bet that there isn't an O(N) possibilitiy.
                // But it doesn't matter.  It just doesn't.  And unlike the simple map for handling
                // redefines, this would require real work and testing.  Screw it.

                V_VARIABLES::iterator it = variable - 1;
                while( it->record_type EQ "L" AND our_level < VARIABLE_LEVEL(it->level) ) {
                    it--;
                }

                // our_level is >= it->level
                if( it->record_type EQ "L" AND our_level EQ VARIABLE_LEVEL(it->level) ) {
                    // We have found our immediately elder sibling
                    // Replace us with his offset plus his length:
                    variable->offset = it->offset + it->length;

                    // cob_local_ptr idiosyncracy:  Level 01 variables are allocated on
                    // 16-byte boundaries:
                    if( our_level EQ 1 ) {
                        variable->offset = (variable->offset + 15) & ~15;
                    }
                } else if( it->record_type EQ "L" AND our_level > VARIABLE_LEVEL(it->level) ) {
                    // We have found our parent.  Our offset is their offset
                    variable->offset = it->offset;
                }
            }

        }
    }
}
