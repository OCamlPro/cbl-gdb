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

    unordered_map<std::string,std::string>b_symbol_to_program_id;

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
            // We are in the state of building up a picture string.
            smatch sm;
            static const regex re_picture_payload("[^{]+[{]'(.+)', ([0-9]+).*$");
            if(regex_match(input,sm,re_picture_payload)) {
                if(sm[1] EQ "\\0") {
                    cob_pic_symbols.Insert(picture_symbol,picture_string);
                    inpic = false;
                } else {
                    string ch = sm[1].str();
                    picture_string += ch[0];
                    int n = stoi(sm[2].str());
                    if(n > 1) {
                        picture_string += '(';
                        picture_string += sm[2].str();
                        picture_string += ')';
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
        smatch sm;
        // Scans for not 'b' followed by b_(something)[<number)]
        static const regex re_storage(".+(b_[0-9]+)[[]([0-9]+).*$");
        if(regex_match(input,sm,re_storage)) {
            if(sm.size() EQ 3) {
                string c_symbol   = sm[1].str();
                int size          = stoi(sm[2].str());
                string cbl_symbol = GetCommentText(input);

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
            }
            continue;
        }

        // Look for a_xxx attributes:
        // ==>static const cob_field_attr a_3 = {0x21,   0,   0, 0x0000, NULL};<<=
        static const regex attribute(".*cob_field_attr ([^ ]+).+[{]0x([^,]+),([^,]+),([^,]+), 0x([^,]+), ([^}]+).*$");
        if(regex_match(input,sm,attribute)) {
            string c_symbol    =  sm[1].str();
            USHORT attr_type   = static_cast<USHORT>(stoul(sm[2],NULL,16));
            USHORT attr_digits = static_cast<USHORT>(stoul(sm[3]));
            SHORT  attr_scale  = static_cast<short>(stol(sm[4]));
            USHORT attr_flags  = static_cast<USHORT>(stoul(sm[5],NULL,16));
            string pic_pointer = sm[6];
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
        if(input.find("cob_field f_") != string::npos) {
            // This is a variable field, generate by a storage level of 02 or more
            // ==>static cob_field f_17 = {22, b_17, &a_2}; /* WS-NAME */<<==
            // cob_field is found in cobc/libcob/common.h

            static const regex field(".*cob_field f_.*$");
            if(regex_match(input,field)) {
                // This is a variable field

                smatch sm;
                static const regex field_match(".*cob_field ([^ \t]+)[^{]+[{]([^,]+), ([^,]+), [&]([^}]+).*$");
                if(regex_match(input,sm,field_match)) {
                    string c_symbol;
                    int    size=0;
                    string name_of_data;
                    int    offset=0;
                    string name_of_attr;
                    string mapped_program_id = program_id;
                    string cbl_name;

                    // look for b_17 + 10
                    smatch sm2;
                    static const regex offset_match("(.*) [+] (.*)");
                    string base_offset = sm[3].str();
                    name_of_data = sm[3].str();
                    if(regex_match(base_offset,sm2,offset_match)) {
                        name_of_data = sm2[1].str();
                        offset = stoi(sm2[2].str());
                    }

                    // As mentioned in the b_ processing, the program_id at
                    // this point can be a lie.  When a b_ is declared GLOBAL,
                    // its definition is in one program-id.  But if it's
                    // accessed in program-id2, that's where the .h file says
                    // the f_ variable that references b_ is.  We built a map
                    // to get around that, which we have to do to get these
                    // variables lined up with the .cbl.lst variable list.

                    unordered_map<string,string>::const_iterator it =
                        b_symbol_to_program_id.find(name_of_data);
                    if(it != b_symbol_to_program_id.end()) {
                        mapped_program_id = it->second;
                    }
                    c_symbol = sm[1].str();
                    size = stoi(sm[2].str());
                    if(name_of_data EQ "NULL") {
                        name_of_data.clear();
                    }
                    name_of_attr = sm[4].str();
                    cbl_name = GetCommentText(input);

                    cob_fields.Insert(c_symbol,
                                      size,
                                      name_of_data,
                                      offset,
                                      name_of_attr,
                                      mapped_program_id,
                                      cbl_name);
                }
            }
            continue;
        }

        // Check for a PICTURE symbol
        if(!inpic) {
            smatch sm;
            static const regex re_picture(".*cob_pic_symbol ([^[]+).*");
            if(regex_match(input,sm,re_picture)) {
                picture_symbol = sm[1].str();
                picture_string.clear();
                inpic = true;
                continue;
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
    set<pair<string,string>>linkage_weve_seen;
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
                current_line_number = stoi(tokens[1]);
                continue;
            }
        }

        if( input.find("ENTRY_") != string::npos ) {
            smatch sm;
            static const regex re_entry("^[ \t]*ENTRY_(.*):.*$");
            if(regex_match(input,sm,re_entry)) {
                program_labels.AddEntry('P',
                                        current_source,
                                        current_line_number,
                                        current_sentence_number,
                                        current_program,
                                        current_entry,
                                        current_section,
                                        current_paragraph   );

                current_entry = sm[1];
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
        }

        if( input.find("SECTION_") != string::npos ) {
            smatch sm;
            static const regex re_entry("^[ \t]*SECTION_(.*):.*$");
            if(regex_match(input,sm,re_entry)) {
                current_section = sm[1];
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
        }

        if( input.find( "Paragraph") != string::npos ) {
            vector<string> tokens = Split(input," \t");
            if( tokens.size() >= 9 AND tokens[0] EQ "/*" AND tokens[1] EQ "Line:" AND tokens[4] EQ "Paragraph" ) {
                current_line_number = stoi(tokens[2]);
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
        if( input.find("cob_check_linkage") != string::npos ) {
            //  We are processing something like
            //
            //  cob_check_linkage (b_41, "'LK-MATH-PROBLEM' (accessed by 'LK-PARAM-1')", 1);

            smatch sm;
            static const regex linkage(".*\\(([^,]+),[^']+[']([^']+).*$");
            if(regex_match(input,sm,linkage)) {
                pair<string,string> cobol_c;
                string c_symbol = sm[1].str();
                string cbl_symbol = sm[2].str();
                cobol_c.first  = c_symbol;
                cobol_c.second = cbl_symbol;

                if( linkage_weve_seen.find(cobol_c) EQ linkage_weve_seen.end() ) {
                    linkage_weve_seen.insert(cobol_c);

                    cob_data.Insert(c_symbol,0,current_program,"Linkage",cbl_symbol);

                }
            }
            // No continue statement here, because the line might also
            // contain COB_SET_DATA
        }

        size_t ncob = 0;
        while((ncob = input.find("COB_SET_DATA",ncob)) != string::npos) {
            //   We might be working with a line like this:
            //   cob_add (COB_SET_DATA (f_40, cob_local_ptr), COB_SET_DATA (f_44, b_41 + 8), 0);

            // We know we'll get at least one hit, so we can do the expensive regex_match with confidence:
            smatch sm;
            static const regex csd("COB_SET_DATA [(]([^,]+), ([^)]+).*$");
            string spiece = input.substr(ncob);
            if(regex_match(spiece,sm,csd)) {
                string f_symbol = sm[1].str();
                string source = sm[2].str();
                string base = sm[2].str();
                int offset=0;
                static const regex offset_match("(.*) [+] (.*)");
                smatch sm2;
                if(regex_match(source,sm2,offset_match)) {
                    base = sm2[1].str();
                    offset = stoi(sm2[2].str());
                }
                if( base.find("cob_local_ptr") EQ string::npos ) {
                    cob_fields.Modify(f_symbol,base,offset);
                }
            }

            ncob += 12;
        }
    }
    return;
}

void
DATA_DESCRIPTIONS::Flatten(const COB_FIELDS &cob_fields,
                           const COB_FIELD_ATTRIBUTES &cob_field_attributes,
                           const COB_PIC_SYMBOLS &cob_pic_symbols,
                           const COB_DATA &cob_data
                          )
{
    // Run through the f_ fields first

    for(auto field : cob_fields.v_cob_fields) {
        DATA_DESCRIPTION ds;
        ds.field_data_name     = field.name        ;
        ds.field_size          = field.size        ;
        ds.c_symbol            = field.name_of_data;
        ds.field_data_offset   = field.offset      ;
        ds.field_attr_name     = field.name_of_attr;
        ds.program_id          = field.program_id  ;
        ds.cbl_symbol          = field.cbl_name    ;

        const COB_FIELD_ATTRIBUTE *cfa = cob_field_attributes.GetFieldAttribute(field.name_of_attr);
        if( cfa ) {
            ds.attr_type          = cfa->type;
            ds.attr_digits        = cfa->digits;
            ds.attr_scale         = cfa->scale;
            ds.attr_flags         = cfa->flags;
            ds.attr_pic_name   = cfa->name_of_pic;

            const COB_PIC_SYMBOL *pic_pointer = cob_pic_symbols.GetPicSymbol(ds.attr_pic_name);
            if( pic_pointer ) {
                ds.picture_string = pic_pointer->pic_symbol;
            }
        }

        const COB_DATUM *cd = cob_data.GetCobDatum(ds.c_symbol);
        if( cd ) {
            ds.storage_size = cd->size;
            ds.storage_type = cd->storage_type;
        }

        if( ds.storage_type.empty() AND ds.c_symbol.empty() ) {
            ds.storage_type = "Local";
        }

        DsInsert(ds);
    }

    // Now walk through the b_ data fields.  In general, these will already
    // be in the symbols array.  But in cases where a working-storage variable
    // is declared, but never used, there won't be an f_ entry.  This means that
    // the debugger won't be able to show you properly formatted data, but it
    // will, at least, know where it is and show you the byte values.

    for(auto cob_data : cob_data.v_data) {
        if( !IsInDs(cob_data.program_id,cob_data.cbl_name) ) {
            // It's not already there.  So, put it there.
            DATA_DESCRIPTION ds;
            ds.program_id = cob_data.program_id;
            ds.cbl_symbol = cob_data.cbl_name;
            ds.c_symbol = cob_data.name;
            ds.storage_size = cob_data.size;
            ds.storage_type = cob_data.storage_type;
            DsInsert(ds);
        }
    }

}

void
COB_PROGRAM_LABELS::FormatProgramInfo(stringstream &ss) const
{
    for(auto label : program_labels) {
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



void
DATA_DESCRIPTIONS::Dump() const
{
    cout << "DATA_DESCRIPTION:" << endl ;
    cout << setw(20) << "program_id"            ;
    cout << setw(20) << "cbl_symbol"            ;
    cout << setw(20) << "c_symbol"            ;
    cout << setw( 7) << "offset"   ;
    cout << setw(13) << "storage_type" ;
    cout << setw(16) << "picture"       ;
    cout << setw( 6) << "ssize"           ;
    cout << setw( 6) << "fsize"           ;
    cout << setw(10) << "attr_type"           ;
    cout << setw(12) << "attr_digits"         ;
    cout << setw(11) << "attr_scale"          ;
    cout << setw(11) << "attr_flags"          ;
    cout << endl;
    for( auto symbol : symbols ) {
        cout << setw(20) << symbol.program_id            ;
        cout << setw(20) << symbol.cbl_symbol            ;
        cout << setw(20) << symbol.c_symbol            ;
        cout << setw( 7) << symbol.field_data_offset   ;
        cout << setw(13) << symbol.storage_type ;
        cout << setw(16) << symbol.picture_string       ;
        cout << setw( 6) << symbol.storage_size           ;
        cout << setw( 6) << symbol.field_size           ;
        cout << setw(10) << symbol.attr_type           ;
        cout << setw(12) << symbol.attr_digits         ;
        cout << setw(11) << symbol.attr_scale          ;
        cout << setw(11) << symbol.attr_flags          ;
        cout << endl;
    }
    cout << endl;
}



