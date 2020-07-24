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

#include <string.h>
#include <set>
#include "input_scan.h"
#include "profiler.h"
#include "params.h"
#include "vartree.h"

#ifdef WIN32
#define strcasecmp _stricmp
#endif

using namespace std;

#define EQ ==
#define AND &&
#define OR ||

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

static void
ProcessDataStorage(ifstream &ifs,
                   const string &source,
                   COB_PIC_SYMBOLS &cob_pic_symbols,
                   COB_DATA &cob_data,
                   COB_FIELD_ATTRIBUTES &cob_field_attributes,
                   COB_FIELDS &cob_fields
                  )
{
    ifs.clear();
    ifs.seekg(0);

    static const string lswitch = "/* Program local variables for ";
    static const string END_OF_FIELDS = "/* End of fields */";

    string storage = "Working"; // This is the default
    string program_id;

    map<std::string,std::string>b_symbol_to_program_id;

    // Variables for building a picture string
    bool inpic = false;
    string picture_symbol;
    string picture_string;

    int current_line_number = 0;
    string input;
    for(;;) {
        getline(ifs,input);
        if(ifs.eof()) {
            break;
        }
        current_line_number += 1;

        if(inpic) {
            // We are in the state of building up a picture string,
            // which looks like this:
//                static const cob_pic_symbol p_1[] = {
//                  {'-', 1},
//                  {'9', 9},
//                  {'.', 1},
//                  {'9', 9},
//                  {'\0', 1}
//                };


            size_t nfound = input.find("\\0");
            if( nfound != string::npos ) {
                // We have processed every element
                cob_pic_symbols.Insert(picture_symbol,picture_string);
                inpic = false;
            } else {
                // We are working on an element:
                size_t nstart = input.find("{");
                if( nstart != string::npos ) {
                    nstart += 2;
                    if( nstart < input.length() ) {
                        picture_string += input.substr(nstart,1);
                        nstart += 3;
                        if( nstart < input.length() ) {
                            int n = STOI( input.substr(nstart) );
                            char ach[24];
                            sprintf(ach,"%d",n);
                            if(n > 1) {
                                picture_string += '(';
                                picture_string += ach;
                                picture_string += ')';
                            }
                        }
                    }
                }
            }
            continue;
        }

        if(input EQ END_OF_FIELDS) {
            storage = "Working";
            program_id.clear();
            continue;
        }

        // Look for external storage

        const string EXTERNAL_PREAMBLE = "static unsigned char\t*b_";
        if( input.substr(0,EXTERNAL_PREAMBLE.size()) EQ EXTERNAL_PREAMBLE ) {
            string c_symbol = "b_";
            size_t nfound = EXTERNAL_PREAMBLE.size();
            while( isdigit(input[nfound]) ) {
                c_symbol += input[nfound++];
            }
            string cbl_symbol = GetCommentText(input);
            int size = 0;
            cob_data.Insert(c_symbol, size, program_id, storage, cbl_symbol);
            continue;
        }

        vector<string> tokens = Split(input," \t");
        if(input.substr(0,lswitch.size()) EQ lswitch) {
            //storage = "Local";
            program_id = StripQuotes(tokens[5]);
            continue;
        }
        if(tokens.size() EQ 5 AND tokens[0] EQ "/*" AND tokens[1] EQ "PROGRAM-ID") {
            // This is seen in global storage sections, where multiple program_id sections
            // can establish global storage
            program_id = tokens[3];
            continue;
        }

        // Look for b_xx[size] variables, found in "Data Storage" subsections:

        //    static cob_u8_t b_10[120] __attribute__((aligned));     /* PRINT-FILE Record */
        //    static cob_u8_t b_11[1] __attribute__((aligned));       /* WRK-DU-1V0-1 */

        size_t nfound = input.find(" b_");
        if( nfound EQ string::npos ) {
            nfound = input.find("\tb_");
        }
        if( nfound != string::npos ) {
            nfound += 1;
            string c_symbol;
            char ch;
            while( nfound < input.length() ) {
                ch = input[nfound++];
                if( ch EQ '[' ) {
                    break;
                }
                c_symbol += ch;
            }
            if( ch EQ '[' ) {
                int size = STOI( input.substr(nfound) );
                string cbl_symbol = GetCommentText(input);
                size_t nfound = cbl_symbol.find(" Record");
                if( nfound != string::npos ) {
                    cbl_symbol = cbl_symbol.substr(0,nfound);
                }

                cob_data.Insert(c_symbol, size, program_id, storage, cbl_symbol);

                // It turns out that the program kinda lies when it gets
                // around to f_ field attribute variables.  In some places,
                // it defines working-storage b_ variables in one program
                // module, maybe with the GLOBAL attribute.  But later on
                // it defines the matching f_ variable in another program-id.

                // We need them in the same program module in order to be
                // able to associate them with the variable listing at the
                // end of the .cbl.lst file.

                // To facilitate that, we build a <string,string> map.
                b_symbol_to_program_id[c_symbol] = program_id;
                continue;
            }
        }

        // Look for a_xxx attributes:
        // ==>static const cob_field_attr a_3 = {0x21,   0,   0, 0x0000, NULL};<<=

        static const char SZ_COB_FIELD_ATTR[] = "cob_field_attr ";
        nfound = input.find(SZ_COB_FIELD_ATTR);
        if( nfound != string::npos ) {
            size_t nstart = nfound + strlen(SZ_COB_FIELD_ATTR);
            string c_symbol;
            char ch;
            // Pick up the symbol
            while( nstart < input.length() AND (ch = input[nstart++]) != ' '  ) {
                c_symbol += ch;
            }
            // Skip past the opening brace
            while( nstart < input.length() AND (ch = input[nstart++]) != '{'  ) {
            }
            // Pick up the hexadecimal attr_type
            char *endptr;
            USHORT attr_type   = static_cast<USHORT>(strtol(input.c_str()+nstart,&endptr,16));
            // Pick up the decimal number of digits:
            USHORT attr_digits = static_cast<USHORT>(strtol(endptr+1,&endptr,10));
            // Pick up the decimal scale
            USHORT attr_scale = static_cast<USHORT>(strtol(endptr+1,&endptr,10));
            // Pick up the hexadecimal flags
            USHORT attr_flags   = static_cast<USHORT>(strtol(endptr+1,&endptr,16));
            // And pick up the pic_pointer
            string pic_pointer;
            // Skip past the ", "
            if( *endptr != '\0' ) {
                endptr += 1;
            }
            if( *endptr != '\0' ) {
                endptr += 1;
            }
            while( *endptr != '\0' AND *endptr != '}' ) {
                pic_pointer += *endptr++;
            }
            if(pic_pointer EQ "NULL") {
                pic_pointer.clear();
            }
            cob_field_attributes.Insert(c_symbol,
                                        attr_type,
                                        attr_digits,
                                        attr_scale,
                                        attr_flags,
                                        pic_pointer,
                                        program_id);
            continue;
        }

        // Look for f_xxx fields:
        // static cob_field f_8    = {120, b_10 + 10, &a_10};   /* PRINT-REC */
        // static cob_field f_294	= {5, b_291 + 15, &a_10};	/* LOW-VAL */
        static const char SZ_COB_FIELD_F[] = "cob_field f_";
        nfound = input.find(SZ_COB_FIELD_F);
        // we can be fooled at this point.  For example, if there is an EBCDIC
        // alphabet involved, then we can have cob_field f_native.

        if( nfound != string::npos ) {
            nfound += strlen(SZ_COB_FIELD_F);

            // Pick up the c_symbol
            string f_name = "f_" ;
            while( nfound < input.length() ) {
                char ch = input[nfound++];
                if( !isdigit(ch) ) {
                    break;
                }
                f_name += ch;
            }

            // Let's refuse to be fooled:
            if( f_name != "f_" ) {
                // Skip past the '{'
                while( nfound < input.length() ) {
                    char ch = input[nfound++];
                    if( ch EQ '{' ) {
                        break;
                    }
                }

                // Pick up the size:
                int size = STOI(input.substr(nfound));

                // Skip past the next space:
                while( nfound < input.length() ) {
                    char ch = input[nfound++];
                    if( ch EQ ' ' ) {
                        break;
                    }
                }
                // Skip to the name_of_data
                string b_name;
                if( nfound < input.length() ) {
                    // Pick up the b
                    b_name += input[nfound++];
                }
                if( nfound < input.length() ) {
                    // Pick up the _
                    b_name += input[nfound++];
                }
                while( nfound < input.length() ) {
                    char ch = input[nfound++];
                    if( !isdigit(ch) ) {
                        break;
                    }
                    b_name += ch;
                }

                if(b_name EQ "NU") {
                    b_name = "";
                }

                // Skip past the ampersand:
                while( nfound < input.length() ) {
                    char ch = input[nfound++];
                    if( ch EQ '&' ) {
                        break;
                    }
                }

                // Pick up the pointer to the attribute
                string a_name;
                if( nfound < input.length() ) {
                    // Pick up the a
                    a_name += input[nfound++];
                }
                if( nfound < input.length() ) {
                    // Pick up the _
                    a_name += input[nfound++];
                }
                while( nfound < input.length() ) {
                    char ch = input[nfound++];
                    if( !isdigit(ch) ) {
                        break;
                    }
                    a_name += ch;
                }

                // See if there is an offset:
                int offset = 0;
                nfound = input.find(" + ");
                if( nfound != string::npos ) {
                    offset = STOI( input.substr(nfound+3) );
                }

                string cbl_name = GetCommentText(input);

                size_t nfound = cbl_name.find(" Record");
                if( nfound != string::npos ) {
                    cbl_name = cbl_name.substr(0,nfound);
                }

                string mapped_program_id = program_id;
                // As mentioned in the b_ processing, the program_id at
                // this point can be a lie.  When a b_ is declared GLOBAL,
                // its definition is in one program-id.  But if it's
                // accessed in program-id2, that's where the .h file says
                // the f_ variable that references b_ is.  We built a map
                // to get around that, which we have to do to get these
                // variables lined up with the .cbl.lst variable list.

                map<string,string>::const_iterator it =
                    b_symbol_to_program_id.find(b_name);
                if(it != b_symbol_to_program_id.end()) {
                    mapped_program_id = it->second;
                }

                string parent;
                const COB_DATUM *pdatum = cob_data.GetCobDatum(b_name);
                if( pdatum ) {
                    parent =  pdatum->cbl_name;
                }

                cob_fields.Insert(f_name,
                                  b_name,
                                  offset,
                                  size,
                                  a_name,
                                  mapped_program_id,
                                  parent,
                                  cbl_name);
                continue;
            }
        }

        // Check for a PICTURE symbol
        if(!inpic) {
            static const char look_for[] = "cob_pic_symbol ";
            size_t nfound = input.find(look_for);
            if( nfound != string::npos ) {
                size_t nstart = nfound + strlen(look_for);
                nfound = input.find("[");
                if( nfound != string::npos ) {
                    picture_symbol = input.substr(nstart,nfound-nstart);
                    picture_string.clear();
                    inpic = true;
                    continue;
                }
            }
        }
    }
}

void
ScanAllDotHFiles(const string &path,
                 const string &fname,
                 COB_PIC_SYMBOLS &cob_pic_symbols,
                 COB_DATA &cob_data,
                 COB_FIELD_ATTRIBUTES &cob_field_attributes,
                 COB_FIELDS &cob_fields
                )
{
    vector<string>AllDotHFiles = DirectoryScan(path,fname,"*.h");
    for(vector<string>::const_iterator filename = AllDotHFiles.begin(); filename != AllDotHFiles.end(); filename++) {
        ifstream ifs;
        OpenOrFail(ifs,*filename);

        string source_file = *filename;
        size_t nfound = (*filename).find_last_of("/\\");
        if(nfound != string::npos) {
            source_file = (*filename).substr(nfound+1);
        }

        ProcessDataStorage(ifs,
                           source_file,
                           cob_pic_symbols,
                           cob_data,
                           cob_field_attributes,
                           cob_fields
                          );

        ifs.close();
    }
}

void
ScanForLinkage(ifstream &ifs,
               const string &current_c_source,
               COB_PROGRAM_LABELS &program_labels,
               COB_DATA &cob_data,
               COB_FIELDS &cob_fields
              )
{
    // This routine actually does two things, but we put them together here so that
    // we have to scan the .c file only once.

    // We pull out the line numbers of PROGRAM-ID, ENTRY, SECTION, and PARAGRAPHS
    // We also look for b_xx / f_yy assocations for LINKAGE-SECTION variables


    CPROFILER;
    ifs.clear();
    ifs.seekg(0);

    string input;
    string current_source;
    int current_line_number=0;
    int current_sentence_number=0;
    string current_program;
    string current_entry;
    string current_section;
    string current_paragraph;
    set< pair<string,string> >linkage_weve_seen;
    for(;;) {
        getline(ifs,input);
        if(ifs.eof()) {
            break;
        }

        if( input.find("COB_SOURCE_FILE") != string::npos ) {
            vector<string> tokens = Split(input," \t");
            if( tokens.size() >= 3 AND tokens[0] EQ "#define" AND tokens[1] EQ "COB_SOURCE_FILE" ) {
                current_source = StripQuotes(tokens[2]);
                continue;
            }
        }

        if( input.find("PROGRAM-ID") != string::npos ) {
            vector<string> tokens = Split(input," \t");
            if( tokens.size() >= 4 AND tokens[0] EQ "/*" AND tokens[1] EQ "PROGRAM-ID" ) {
                current_program = StripQuotes(tokens[2]);
                current_entry = "";
                current_section = "";
                current_paragraph = "";
                continue;
            }
        }

        if( input.substr(0,5) EQ "#line" ) {
            vector<string> tokens = Split(input," \t");
            if( tokens.size() >= 3 AND StripQuotes(tokens[2]) EQ current_source ) {
                current_line_number = STOI(tokens[1]);
                continue;
            }
        }

        static char SZ_ENTRY[] = "ENTRY_";
        size_t nfound = input.find(SZ_ENTRY);
        if( nfound != string::npos ) {
            nfound += strlen(SZ_ENTRY);

            program_labels.AddEntry('P',
                                    current_source,
                                    current_line_number,
                                    current_sentence_number,
                                    current_program,
                                    current_entry,
                                    current_section,
                                    current_paragraph   );

            current_entry.clear();
            while(nfound < input.length() ) {
                char ch = input[nfound++];
                if( ch EQ ':' ) {
                    break;
                }
                current_entry += ch;
            }

            current_section = "";
            current_paragraph = "";
            program_labels.AddEntry('E',
                                    current_source,
                                    current_line_number,
                                    current_sentence_number,
                                    current_program,
                                    current_entry,
                                    current_section,
                                    current_paragraph   );
            continue;
        }

        static char SZ_SECTION[] = "SECTION_";
        nfound = input.find(SZ_SECTION);
        if( nfound != string::npos ) {
            nfound += strlen(SZ_SECTION);
            current_section.clear();
            while(nfound < input.length() ) {
                char ch = input[nfound++];
                if( ch EQ ':' ) {
                    break;
                }
                current_section += ch;
            }
            current_paragraph = "";
            program_labels.AddEntry('S',
                                    current_source,
                                    current_line_number,
                                    current_sentence_number,
                                    current_program,
                                    current_entry,
                                    current_section,
                                    current_paragraph   );
            continue;
        }

        if( input.find( "Paragraph") != string::npos ) {
            vector<string> tokens = Split(input," \t");
            if( tokens.size() >= 9 AND tokens[0] EQ "/*" AND tokens[1] EQ "Line:" AND tokens[4] EQ "Paragraph" ) {
                current_line_number = STOI(tokens[2]);
                current_paragraph = tokens[5];
                program_labels.AddEntry('G',
                                        current_source,
                                        current_line_number,
                                        current_sentence_number,
                                        current_program,
                                        current_entry,
                                        current_section,
                                        current_paragraph   );
                continue;
            }
        }
        // We can now continue looking for the Linkage information we need:

        // The compiler doesn't give us specific information for linkage variables, which
        // show up only in the code as function parameters.

        // So, the main purpose of this routine is to look for internal cobc calls in
        // the .c source that allow us to create a cross-reference of b_xx and f_yy
        // identifiers.

        // cout << input << endl;
        // There is information to be gleaned from various C calls.
        static const char SZ_COB_CHECK_LINKAGE[] = "cob_check_linkage (";
        nfound = input.find(SZ_COB_CHECK_LINKAGE);
        if( nfound != string::npos ) {
            pair<string,string> cobol_c;
            string c_symbol;
            nfound += strlen(SZ_COB_CHECK_LINKAGE);
            while( nfound < input.length() ) {
                char ch = input[nfound++];
                if( ch EQ ',' ) {
                    break;
                }
                c_symbol += ch;
            }

            string cbl_symbol;
            while( nfound < input.length() ) {
                char ch = input[nfound++];
                if( ch EQ '\'' ) {
                    break;
                }
            }
            while( nfound < input.length() ) {
                char ch = input[nfound++];
                if( ch EQ '\'' ) {
                    break;
                }
                cbl_symbol += ch;
            }

            cobol_c.first  = c_symbol;
            cobol_c.second = cbl_symbol;
            if( linkage_weve_seen.find(cobol_c) EQ linkage_weve_seen.end() ) {
                linkage_weve_seen.insert(cobol_c);
                cob_data.Insert(c_symbol,0,current_program,"Linkage",cbl_symbol);
            }
            // No continue statement here, because the line might also
            // contain COB_SET_DATA
        }

        size_t ncob = 0;
        static const char SZ_COB_SET_DATA[] = "COB_SET_DATA (";
        while((ncob = input.find(SZ_COB_SET_DATA,ncob)) != string::npos) {
            //   We might be working with a line like this:
            //   cob_add (COB_SET_DATA (f_40, cob_local_ptr), COB_SET_DATA (f_44, b_41 + 8), 0);
            //   cob_display (0, 1, 1, COB_SET_DATA (f_13, cob_local_ptr + 32 + 23));

            size_t nstart = ncob + strlen(SZ_COB_SET_DATA);
            size_t nfound = input.find( ",", nstart );
            string f_symbol = input.substr(nstart,nfound - nstart);
            string base;
            int    offset=0;

            nstart = nfound+2;
            nfound = input.find( ")", nstart );
            string param2 = input.substr(nstart,nfound-nstart);

            //   We ignore cob_local_ptr types; locals are handled elsewhere
            if( param2.find("cob_local_ptr") EQ string::npos ) {
                // It could be SOMETHING or SOMETHING + decimal
                nfound = param2.find(" + ");
                if( nfound != string::npos ) {
                    base = param2.substr(0,nfound);
                    offset = STOI(param2.substr(nfound+3));
                } else {
                    nfound = param2.find(")",nstart);
                    base = param2;
                    if( nfound != string::npos ) {
                        base = param2.substr(0,nfound);
                    }
                }
                cob_fields.Modify(f_symbol,base,offset);
            } else {
                size_t nfound = param2.find(" + ");
                if( nfound != string::npos ) {
                    offset += STOI( param2.substr(nfound+3) );
                    nfound = param2.find(" + ",nfound+3);
                    if( nfound != string::npos ) {
                        offset += STOI( param2.substr(nfound+3) );
                        nfound = param2.find(" + ",nfound+3);
                    }
                }
                cob_fields.Modify(f_symbol,"",offset);
            }

            // Go see if there are more on this line
            ncob += strlen(SZ_COB_SET_DATA);
        }
    }
    return;
}

void
COB_PROGRAM_LABELS::FormatProgramInfo(stringstream &ss) const
{
    for(VCOB_PROGRAM_LABEL::const_iterator it=program_labels.begin();
            it != program_labels.end();
            it++ ) {
        COB_PROGRAM_LABEL label = *it;
        /*
        type            = type_           ;
        source_file     = source_file_    ;
        line_number     = line_number_    ;
        sentence_number = sentence_number_;
        program         = program_        ;
        entry           = entry_          ;
        section         = section_        ;
        paragraph       = paragraph_      ; */

        switch(label.type) {
        case 'P':
            ss << label.type << "|" ;
//              ss << label.source_file << "|"  ;
            ss << label.line_number << "|"  ;
            ss << label.program ;
            ss << "~";
            break;
        case 'E':
            //ss << label.type << "|" ;
            //ss << label.source_file << "|"  ;
            //ss << label.line_number << "|"  ;
            //ss << label.entry << "|" ;
            //ss << "~";
            break;
        case 'S':
            //ss << label.type << "|" ;
            //ss << label.source_file << "|"  ;
            //ss << label.line_number << "|"  ;
            //ss << label.section << "|" ;
            //ss << "~";
            break;
        case 'G':
            //ss << label.type << "|" ;
            //ss << label.source_file << "|"  ;
            //ss << label.line_number << "|"  ;
            //ss << label.paragraph << "|" ;
            //ss << "~";
            break;
        }
    }
}
