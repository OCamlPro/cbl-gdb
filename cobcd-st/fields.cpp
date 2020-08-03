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

#include "fields.h"
#include "params.h"
#include "profiler.h"
#include "utils.h"

using namespace std;

#define EQ ==
#define AND &&
#define OR ||

void
COB_FIELDS_2::Insert(std::string f_name_,
                     std::string b_name_,
                     int         offset_,
                     int         size_,
                     std::string a_name_)
{
    M_COB_FIELDS_2::const_iterator it = m_cob_fields.find(f_name_);
    if( it != m_cob_fields.end() ) {
        std::cout << "Creating the symbol " << f_name_ << " more than once in COB_FIELDS_2::Insert()\n" ;
        exit(1);
    }
    COB_FIELD_2 cf(f_name_,
                   b_name_,
                   offset_,
                   size_,
                   a_name_);
    m_cob_fields[f_name_] = v_cob_fields.size();
    v_cob_fields.push_back(cf);
}


void
COB_FIELDS_2::ScanDotHFile(ifstream &ifs, const string &source)
{
    int current_line_number = 0;

    string input;
    for(;;) {
        getline(ifs,input);
        if(ifs.eof()) {
            break;
        }
        current_line_number += 1;

        // Look for f_xxx fields:
        // static cob_field f_8    = {120, b_10 + 10, &a_10};   /* PRINT-REC */
        // static cob_field f_294	= {5, b_291 + 15, &a_10};	/* LOW-VAL */
        static const char SZ_COB_FIELD_F[] = "cob_field f_";
        size_t nfound = input.find(SZ_COB_FIELD_F);
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

                Insert(f_name,
                       b_name,
                       offset,
                       size,
                       a_name);
            }
        }
    }
}

void
COB_FIELDS_2::ScanAllDotHFiles(const std::string &path, const std::string &fname )
{
    CPROFILER;
    vector<string>AllDotHFiles = DirectoryScan(path,fname,"*.h");
    for(vector<string>::const_iterator filename = AllDotHFiles.begin(); filename != AllDotHFiles.end(); filename++) {
        ifstream ifs;
        OpenOrFail(ifs,*filename);

        string source_file = *filename;
        size_t nfound = (*filename).find_last_of("/\\");
        if(nfound != string::npos) {
            source_file = (*filename).substr(nfound+1);
        }

        ScanDotHFile(ifs, source_file);

        ifs.close();
    }
}

const COB_FIELD_2 *
COB_FIELDS_2::Find(const std::string &f_name_) const
{
    const COB_FIELD_2 *retval = NULL;

    map<std::string,size_t>::const_iterator it =  m_cob_fields.find(f_name_);
    if( it != m_cob_fields.end() ) {
        retval = &v_cob_fields[it->second];
    }

    return retval;
}

void
COB_FIELDS_2::Dump() const
{
    CPROFILER;
    std::cout << "COB_FIELDS_2:" << std::endl;
    std::cout << std::setw(20) << "f_name" ;
    std::cout << std::setw(20) << "b_name";
    std::cout << std::setw(20) << "offset" ;
    std::cout << std::setw(20) << "size";
    std::cout << std::setw(20) << "a_name";
    std::cout << std::endl;

    for( V_COB_FIELDS_2::const_iterator it = v_cob_fields.begin();
            it!=v_cob_fields.end();
            it++) {
        COB_FIELD_2 field = *it;
        std::cout << std::setw(20) << field.f_name ;
        std::cout << std::setw(20) << field.b_name;
        std::cout << std::setw(20) << field.offset ;
        std::cout << std::setw(20) << field.size;
        std::cout << std::setw(20) << field.a_name;
        std::cout << std::endl;
    }
    std::cout << std::endl;
}
