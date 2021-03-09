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
#include <assert.h>
#include "variables.h"
#include "profiler.h"
#include "utils.h"

using namespace std;

#define EQ ==
#define AND &&
#define OR ||

static string
ExtractVariableNameFrom(const string &p)
{
    string retval = Trim(p);
    size_t nfound=retval.find("_");
    if( nfound != string::npos ) {
        // Extract the b_10 from around the '_' character
        size_t nleft  = nfound;
        size_t nright = nfound+1;

        while( nleft >= 1 ) {
            if( !isalpha(retval[nleft-1]) ) {
                break;
            }
            nleft -= 1;
        }
        while( nright < retval.size() ) {
            if( !isdigit(retval[nright]) ) {
                break;
            }
            nright += 1;
        }
        retval = retval.substr(nleft,nright-nleft);
    }
    return retval;
}

static string
HackAwayCobGetNumdisp(const string &p_)
{
    string p = p_;
    // cob_dump_field ( 1, "G-1", COB_SET_FLD(f0, cob_get_numdisp (b_8, 1) + cob_get_numdisp (b_8, 1) + cob_get_numdisp (b_8, 1), b_9, &a_1), 0, 0);
    // Let's hack at that last one first. Anyplace we see "cob_get_numdisp (b_8, 1)", we want to replace it with, simply, "b_8"

    string retval;
    while(!p.empty()) {
        size_t nfound1 = p.find("cob_get_numdisp");
        if( nfound1 EQ string::npos ) {
            nfound1 = p.size();
        }
        // nfound1 is where cob_get_numdisp starts
        size_t nfound2 = p.find(')',nfound1);
        string replacement = "";
        if( nfound2 != string::npos ) {
            size_t nfoundm = nfound1 + strlen("cob_get_numdisp");
            nfound2 += 1;
            string fragment = p.substr(nfoundm,nfound2-nfoundm);
            // Fragment looks like
            //      " (b_8, 1)"
            replacement = " " + ExtractVariableNameFrom(fragment) + " ";
        } else {
            nfound2 = nfound1;
        }

        retval += p.substr(0,nfound1);
        retval += replacement;
        p = p.substr(nfound2);
    }
    return retval;
}

static string
SquishAwayStarIntStar(const string &s_)
{
    string s = s_;
    string retval;

    while( !s.empty() ) {
        size_t nfound1 = s.find( "(*(int *)" );
        size_t nfound2;
        string stripped;
        if( nfound1 != string::npos ) {
            nfound2 = s.find(')',nfound1 + strlen("(*(int *)") );
            if( nfound2 != string::npos ) {
                nfound2 += 2;
                string param = ExtractVariableNameFrom( s.substr(nfound1 + strlen("(*(int *)")) );
                retval = s.substr(0,nfound1);
                retval += ' ';
                retval += param;
                retval += ' ';
            }
        } else {
            nfound1 = s.size();
            nfound2 = nfound1;
            retval += s;
        }
        s = s.substr(nfound2);
    }

    return retval;
}

VSTRING
ParametersInsideParentheses(const string &s, size_t &final)
{
    VSTRING retval;
    final = 0;
    string next;
    const char *p = s.c_str();

    int pcount = 0;
    if( *p++ EQ '(' ) {
        pcount = 1;
    }
    while( pcount && *p ) {
        char ch = *p++;
        final += 1;
        if( ch EQ '(' ) {
            pcount += 1;
            next += ch;
            continue;
        }
        if( ch EQ ')' ) {
            if( --pcount ) {
                next += ch;
            }
            continue;
        }
        if( pcount > 1 ) {
            next += ch;
            continue;
        }
        // Arriving here means pcount is 1
        if( ch EQ ',' ) {
            retval.push_back(Trim(next));
            next.clear();
            continue;
        }
        next += ch;
    }
    if( !retval.empty() ) {
        retval.push_back(Trim(next));
    }

    return retval;
}

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

    bool looking_for_perform_return_location = false;
    string current_record_type = "X" ;
    int line_number = 1;
    while( fgets(ach,sizeof(ach),f) ) {
        extern bool GV_echo_line;
        if( GV_echo_line ) {
            printf("A-%d %s",line_number++,ach);
        }
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
                variable.offset = tokens[2];
                variable.name = tokens[5];
                variable.f_name = tokens[7];
                Insert(variable);
                variable.record_type = "E";
                Insert(variable);
            }
            continue;
        }

        if( strstr(ach, ": PERFORM ") ) {
            string input = ach;

            VSTRING tokens = Split(input," \t");
            if( tokens.size() EQ 8
                    AND tokens[0] EQ "/*"
                    AND tokens[1] EQ "Line:"
                    AND tokens[3] EQ ":"
                    AND tokens[4] EQ "PERFORM"
                    AND tokens[5] EQ ":"
                    AND tokens[7] EQ "*/\n" ) {
                VARIABLE variable;
                variable.record_type = "p";
                variable.f_name = tokens[6] + ':' + tokens[2];
                Insert(variable);
                looking_for_perform_return_location = true;
            }
            continue;
        }

        if( looking_for_perform_return_location ) {
            // We are looking for a line like this:
            //     frame_ptr->return_address_ptr = &&l_54;
            //
            // When found, we want to make the name member of the
            // variable we just pushed to be "l_54"

            if( strstr(ach,"for (") ) {
                // If you see the for(, it means that this was a
                // PERFORM N times.

                // We ignore those
                variables.pop_back();
                looking_for_perform_return_location = false;
                continue;
            }

            char *p = strstr(ach,"&&");
            if( p ) {
                p += 2;
                char *p2 = strstr(ach,";");
                if( p2 ) {
                    *p2 = '\0';
                }
                variables.back().name = p;
                looking_for_perform_return_location = false;
            }
            continue;
        }

        static const char COB_DUMP_FILE[]   = "cob_dump_file";
        static const char COB_DUMP_FIELD[]  = "cob_dump_field";
        static const char COB_DUMP_OUTPUT[] = "cob_dump_output";
        static const char COB_SET_DATA[] = "COB_SET_DATA";
        static const char COB_SET_FLD[] = "COB_SET_FLD";
        char *p = ach;

        // Skip over leading spaces
        while( *p EQ ' ' OR *p EQ '\t' ) {
            p += 1;
        }

        // The lines we are looking for can be commented out.
        // So, we just ignore the "/*" if it's the first thing on a line:
        if( *p EQ '/' AND *(p+1) EQ '*' ) {
            p += 2;
        }

        // Skip over spaces
        while( *p EQ ' ' OR *p EQ '\t' ) {
            p += 1;
        }

        if( strncmp(p, COB_DUMP_OUTPUT, strlen(COB_DUMP_OUTPUT)) EQ 0 ) {
            /* cob_dump_output("WORKING-STORAGE"); */
            /* cob_dump_output("LINKAGE"); */
            /* cob_dump_output("LOCAL-STORAGE"); */
            p += strlen(COB_DUMP_OUTPUT) + 2;
            if( strstr(p,"WORKING-STORAGE") EQ p ) {
                current_record_type = "W";
            } else if( strstr(p,"LOCAL-STORAGE") EQ p ) {
                current_record_type = "L";
            } else if( strstr(p,"LINKAGE") EQ p ) {
                current_record_type = "I";
            }
            // There are other types: REPORT and SCREEN.  We don't care about them
            continue;
        }

        if( strncmp(p, COB_DUMP_FILE, strlen(COB_DUMP_FILE)) EQ 0 ) {
            /* cob_dump_file ("FD pgm-input", h_PGM_INPUT); */
            /* cob_dump_file ("SD pgm-input", h_PGM_INPUT); */
            p += strlen(COB_DUMP_FILE) + 3;
            if( strstr(p,"SD ") EQ p ) {
                current_record_type = "S";
            } else if( strstr(p,"FD ") EQ p ) {
                current_record_type = "F";
            }
            continue;
        }

        if( strncmp(p, COB_DUMP_FIELD, strlen(COB_DUMP_FIELD)) EQ 0 ) {
            // As time has gone on, and I have been exposed to steadily more complex COBOL
            // constructions, this routine has been forced to become more and more complex.
            // The cob_dump_field routine takes these parameters
            //   cob_dump_field (const int level, const char *name, cob_field *f_addr, const int offset, const int indexes, ...)
            // What makes this generally useful is that the field pointer gets built at dump time using the
            // COB_SET_FLD() macro
            //   COB_SET_FLD(f_name, length, bname + offset, aname)
            //
            // Where it gets complicated is when the length and/or the offset are themselves variable

            // So, examples:

            //  /* cob_dump_field ( 1, "WRK-DU-1V0-1", COB_SET_FLD(f0, 1, b_52, &a_7), 0, 0); */
            //  /* cob_dump_field (10, "datyr", COB_SET_FLD(f0, 4, b_15 + 17, &a_7), 0, 0); */
            //  /* cob_dump_field ( 5, "new-dat1", COB_SET_FLD(f0, 8, b_15 + 17, &a_3), 0, 0); REDEFINES */
            //  /* cob_dump_field ( 2, "ELMT", &f_278, 0, 1, i_1, 1); OCCURS 1 3 */
            //  /* cob_dump_field ( 2, "G-2", COB_SET_FLD(f0, cob_get_numdisp (b_8, 1), b_9, &a_1), 0, 0); */
            //  /* cob_dump_field ( 1, "result", COB_SET_FLD(f0, (*(int *)(b_8)), b_10, &a_3), 0, 0); */
            //  /* cob_dump_field ( 1, "G-1", COB_SET_FLD(f0, cob_get_numdisp (b_8, 1) + cob_get_numdisp (b_8, 1) + cob_get_numdisp (b_8, 1), b_9, &a_1), 0, 0); */
            //  /* cob_dump_field ( 2, "G-3", COB_SET_FLD(f0, cob_get_numdisp (b_8, 1) + cob_get_numdisp (b_8, 1), b_9 + 0 + cob_get_numdisp (b_8, 1), &a_1), 0, 0); */
            //  /* cob_dump_field ( 3, "G-4", COB_SET_FLD(f1, cob_get_numdisp (b_8, 1), b_9 + 0 + cob_get_numdisp (b_8, 1), &a_1), 0, 0); */
            //  /* cob_dump_field ( 4, "X", COB_SET_FLD(f0, 1, b_9 + 0 + cob_get_numdisp (b_8, 1), &a_4), 0, 1, i_1, 1); OCCURS 1 3 */
            //  /* cob_dump_field ( 3, "G-5", COB_SET_FLD(f2, cob_get_numdisp (b_8, 1), b_9 + 0 + cob_get_numdisp (b_8, 1) + 0 + cob_get_numdisp (b_8, 1), &a_1), 0, 0); */
            //  /* cob_dump_field ( 4, "X", COB_SET_FLD(f0, 1, b_9 + 0 + cob_get_numdisp (b_8, 1) + 0 + cob_get_numdisp (b_8, 1), &a_4), 0, 1, i_1, 1); OCCURS 1 3 */
            //  /* cob_dump_field ( 2, "WS-LASTNAME", COB_SET_DATA (f_88, b_84 + 12), 0, 0); EXTERNAL */
            //
            // Arriving here means that the first thing on the line after removing "/*" is "cob_dump_field"

            string cdf = p;
            size_t nfound = cdf.find('(');
            if( nfound != string::npos ) {
                cdf = cdf.substr(nfound);
                VARIABLE variable;
                variable.record_type = current_record_type;

                // If this variable is a REDEFINES, that word, flanked by spaces, will be on the line.
                variable.redefines = strstr(p," REDEFINES ") != NULL;

                // If this variable is an OCCURS, that word, flanked by spaces, will be on the line:

                char *poccurs = strstr(ach," OCCURS ");
                if( poccurs ) {
                    VSTRING t = Split(poccurs," ");
                    if( t.size() >= 3 ) {
                        variable.occurs_min = stoi(t[1]);
                        variable.occurs_max = stoi(t[2]);
                    }
                }


                // Hack away at cob_get_numdisp() entries:
                cdf = HackAwayCobGetNumdisp(cdf);
                // Squish away any (*(int *)b_8) entries:
                cdf = SquishAwayStarIntStar(cdf);

                size_t final_paren;
                VSTRING cdf_parameters = ParametersInsideParentheses(cdf,final_paren);
                if( cdf_parameters.size() >= 5  ) {
                    variable.level = stoi(Trim(cdf_parameters[0]));
                    variable.name  = StripQuotes(cdf_parameters[1]);
                    string field_pointer = Trim(cdf_parameters[2]);
                    string occurs_offset = Trim(cdf_parameters[3]);
                    string occurs_indexes = Trim(cdf_parameters[4]);

                    if( !field_pointer.empty() && field_pointer[0] EQ '&') {
                        // Minimal association: COBOL_name with f_name
                        variable.f_name = field_pointer.substr(1);
                    } else {
                        if( field_pointer.find(COB_SET_FLD) != string::npos ) {
                            nfound = field_pointer.find( '(' );
                            if( nfound != string::npos ) {
                                field_pointer = field_pointer.substr(nfound);
                                VSTRING csf_parameters = ParametersInsideParentheses(field_pointer,final_paren);
                                if( csf_parameters.size() EQ 4 ) {
                                    //   COB_SET_FLD(f_name, length, bname + offset, aname)
                                    // fname is meaningless in this context
                                    // length might be combinations of integer constants and/or text :

                                    for(size_t i=0; i<csf_parameters[1].size(); i++ ) {
                                        char ch = csf_parameters[1][i];
                                        if( ch != ' ' ) {
                                            variable.length += ch;
                                        }
                                    }

                                    // The third parameter is the b_name with, possibly, an offset added to it.
                                    VSTRING base_offset = Split(csf_parameters[2],"+");
                                    if( base_offset.size() ) {
                                        variable.b_name = Trim(base_offset[0]);
                                    }
                                    size_t i = 1;
                                    while( i < base_offset.size() ) {
                                        string offset = Trim(base_offset[i++]);
                                        if( offset != "0" ) {
                                            if( !variable.offset.empty() ) {
                                                variable.offset += '+';
                                            }
                                            variable.offset += offset ;
                                        }
                                    }
                                    variable.a_name = csf_parameters[3].substr(1);
                                } else {
                                    cerr << "VARIABLES::ScanCFile() doesn't know how to handle the COB_SET_FLD macro in" << endl ;
                                    cerr << "   " << ach ;
                                    cerr << "Please alert Bob Dubner at support@cobolworx.com" << endl;
                                    cerr << "Thank you." << endl;
                                    continue;
                                }
                            }
                        } else if( field_pointer.find(COB_SET_DATA) != string::npos ) {
                            nfound = field_pointer.find( '(' );
                            if( nfound != string::npos ) {
                                field_pointer = field_pointer.substr(nfound);
                                VSTRING csd_parameters = ParametersInsideParentheses(field_pointer,final_paren);
                                if(csd_parameters.size() == 2) {
                                    variable.f_name = csd_parameters[0];
                                }

                                // The second parameter is the b_name with, possibly, an offset added to it.
                                VSTRING base_offset = Split(csd_parameters[1],"+");
                                if( base_offset.size() ) {
                                    variable.b_name = Trim(base_offset[0]);
                                }
                                size_t i = 1;
                                while( i < base_offset.size() ) {
                                    string offset = Trim(base_offset[i++]);
                                    if( offset != "0" ) {
                                        if( !variable.offset.empty() ) {
                                            variable.offset += '+';
                                        }
                                        variable.offset += offset ;
                                    }
                                }
                            }
                        } else {
                            cerr << "VARIABLES::ScanCFile() doesn't know how to handle the field pointer in" << endl ;
                            cerr << "   " << ach ;
                            cerr << "Please alert Bob Dubner at support@cobolworx.com" << endl;
                            cerr << "Thank you." << endl;
                            continue;
                        }
                    }
                    Insert(variable);
                }
            }
        }
    }

    ::fclose(f);
}

void
VARIABLES::FillInTheBlanks(COB_FIELDS &cob_fields)
{
    string current_bname = "";
    for( V_VARIABLES::iterator it = variables.begin(); it!=variables.end(); it++) {
        if( it->level <= 2 OR it->level EQ 78 OR it->level EQ 77 ) {
            // This is a root variable, so reset the current name
            current_bname = "";
        }

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
                    it->length = to_string(pfield->size);
                }
                if( pfield->offset ) {
                    it->offset = to_string(pfield->offset);
                }
            }
        }
        if( it->b_name.empty() ) {
            it->b_name = current_bname;
        } else {
            current_bname = it->b_name;
        }

        V_VARIABLES::iterator it2 = it;
        while( it2 > variables.begin() ) {
            if( it2->b_name.empty() ) {
                // We have nothing to propogate upward
                break;
            }
            if( it2->level EQ 77 OR it2->level EQ 77 ) {
                // Level 77 doesn't propogate upward
                break;
            }
            V_VARIABLES::iterator it1 = it2-1;
            if( !it1->b_name.empty() ) {
                // The one before ours is already populated
                break;
            }
            // When you get here, it1 has no b_name, and it2 does
            if( it1->level <= it2->level AND it2->level >= 2 ) {
                it1->b_name = it2->b_name;
            }
            it2--;
        }

    }
}

void
VARIABLES::Dump() const
{
    CPROFILER;
    cout << "VARIABLES:" << endl;
    cout << setw( 3) << "RT" ;
    cout << setw( 4) << "lvl";
    cout << setw(20) << "name" ;
    cout << setw(20) << "f_name";
    cout << setw(20) << "b_name";
    cout << setw(20) << "a_name" ;
    cout << setw(20) << "offset" ;
    cout << setw(20) << "length";
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
        cout << setw(20) << variable->offset;         // When f_name exists, offset should have the same runtime value as f_name->data - b_name;
        cout << setw(20) << variable->length;      // When f_name exists, length should be the same as f_name->size.  But it might be a b_ variable
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
        ss << variable->length               << "|" ;
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

static int
AddEmUp(const string &s)
{
    int retval = 0;

    VSTRING tokens = Split(s,"+");
    for(size_t i=0; i<tokens.size(); i++) {
        string token = Trim(tokens[i]);
        if( !token.empty() AND isdigit(token[0]) ) {
            retval += stoi(token);
        }
    }

    return retval;
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
    // circumstances.  But I couldn't make myself skip it.

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

            if( variable->offset.empty() OR variable->offset EQ "0" ) {
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

                    // When I learned that offsets and lengths could variable, I turned
                    // variable.length and variable.offset into strings, rather than
                    // integers.  For this code, assume we have integers and names
                    // separated by plus signs.  Ignore names.  That's all I can do
                    // without samples.

                    int parent_offset = AddEmUp(it->offset);
                    int parent_length = AddEmUp(it->length);

                    int our_offset = parent_offset + parent_length;

                    // cob_local_ptr idiosyncracy:  Level 01 variables are allocated on
                    // 16-byte boundaries:
                    if( our_level EQ 1 ) {
                        our_offset = (our_offset + 15) & ~15;
                    }
                    variable->offset = to_string(our_offset);
                } else if( it->record_type EQ "L" AND our_level > VARIABLE_LEVEL(it->level) ) {
                    // We have found our parent.  Our offset is their offset
                    variable->offset = it->offset;
                }
            }
        }
    }
}
