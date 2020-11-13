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
COB_FIELDS::Insert(std::string f_name_,
                   std::string b_name_,
                   int         offset_,
                   int         size_,
                   std::string a_name_)
{
    M_COB_FIELDS_2::const_iterator it = m_cob_fields.find(f_name_);
    if( it != m_cob_fields.end() ) {
        std::cout << "Creating the symbol " << f_name_ << " more than once in COB_FIELDS::Insert()\n" ;
        exit(1);
    }
    COB_FIELD cf(f_name_,
                 b_name_,
                 offset_,
                 size_,
                 a_name_);
    m_cob_fields[f_name_] = v_cob_fields.size();
    v_cob_fields.push_back(cf);
}

void
COB_FIELDS::ScanDotHFile(ifstream &ifs, const string &source)
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
                if( !isdigit(ch) OR ch != '_' ) {
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

static VSTRING
GetHfilesFrom(const std::string &path, const std::string &fname)
{
    VSTRING retval;

    string fn = path;
    fn += fname;
    fn += ".c";

    FILE *f = fopen(fn.c_str(),"r");
    if( f ) {
        char ach[16384];
        while( fgets(ach,sizeof(ach),f) ) {
            if( strstr(ach,"#include") ) {
                string s = ach;
                size_t n1 = s.find('"');
                if( n1 != string::npos ) {
                    size_t n2 = s.find('"',n1+1);
                    if( n2 != string::npos ) {
                        string hname = s.substr(n1+1,n2-(n1+1));
                        if( hname.find(fname) != string::npos ) {
                            retval.push_back(path + hname);
                        }
                    }
                }
            }
        }

        fclose(f);
    }

    fn = path;
    fn += fname;
    fn += ".delete_us";

    f = fopen(fn.c_str(),"w");
    if( f ) {
        for( size_t i=0; i<retval.size(); i++ ) {
            fprintf(f,"%s\n",retval[i].c_str());
        }
        fclose(f);
    }
    return retval;
}

void
COB_FIELDS::ScanAllDotHFiles(const std::string &path, const std::string &fname )
{
    CPROFILER;
    vector<string>AllDotHFiles = GetHfilesFrom(path,fname);
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

COB_FIELD *
COB_FIELDS::Find(const std::string &f_name_)
{
    COB_FIELD *retval = NULL;

    map<std::string,size_t>::const_iterator it =  m_cob_fields.find(f_name_);
    if( it != m_cob_fields.end() ) {
        retval = &v_cob_fields[it->second];
    }

    return retval;
}

void
COB_FIELDS::Dump() const
{
    CPROFILER;
    std::cout << "COB_FIELDS:" << std::endl;
    std::cout << std::setw(20) << "f_name" ;
    std::cout << std::setw(20) << "b_name";
    std::cout << std::setw(20) << "offset" ;
    std::cout << std::setw(20) << "size";
    std::cout << std::setw(20) << "a_name";
    std::cout << std::endl;

    for( V_COB_FIELDS_2::const_iterator it = v_cob_fields.begin();
            it!=v_cob_fields.end();
            it++) {
        COB_FIELD field = *it;
        std::cout << std::setw(20) << field.f_name ;
        std::cout << std::setw(20) << field.b_name;
        std::cout << std::setw(20) << field.offset ;
        std::cout << std::setw(20) << field.size;
        std::cout << std::setw(20) << field.a_name;
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void
COB_FIELDS::ScanCFile(const string &path, const string &fname)
{
    // We specifically are looking for COB_SET_DATA for LOCAL-STORAGE, because
    // that's the only way we can find offsets into cob_local_ptr:

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
    static const char COB_SET_DATA[] = "COB_SET_DATA";
    int line_number = 1;
    while( fgets(ach,sizeof(ach),f) ) {
        extern bool GV_echo_line;
        if( GV_echo_line ) {
            printf("B-%d %s",line_number++,ach);
        }
        char *pcsd;
        if( (pcsd=strstr(ach,COB_SET_DATA)) ) {
            // This line has COB_SET_DATA in it.

            // I can't believe I am about to do this.  But I couldn't let it go. Users
            // are an evil bunch, just evil, and I found I couldn't ignore the possibility
            // that some evil -- I did say they are evil, right? -- user would write some
            // COBOL code that has in it " MOVE 'COB_SET_DATA' TO CONFUSE-DUBNER", which
            // would end up with a "memmove(dest,"COB_SET_DATA",12)" in the C code we
            // are currently examining.

            // So, I am going to put in some simple screening code that will get rid
            // of the simplest form of that problem.

            // Did I say I can't believe I am doing this?

            char *p1 = strchr(ach,'\"');
            char *p2 = strchr(pcsd,'\"');

            if( p1 AND p2 AND p1 < pcsd ) {
                // Our COB_SET_DATA is in a double-quote sandwich.  Assume it's
                // part of a literal, and just skip it
                continue;
            }

            char *p = pcsd + strlen(COB_SET_DATA);
            // Find the first character after the opening parenthesis
            SkipPast(p,'(');

            // Build up a string of non-space characters up to the
            // closing parenthesis
            string s;
            while( *p != ')' ) {
                if( *p != ' ' AND *p != '\t' ) {
                    s += *p;
                }
                p += 1;
            }
            // Bust it up on commas:
            VSTRING tokens = Split(s,",");
            if( tokens.size() EQ 2 ) {
                // The first element is the f_name:
                string f_n = tokens[0];

                // Which is followed by a base name.  It'll be cob_local_ptr
                // for LOCAL-STORAGE, or b_xxx otherwise
                VSTRING tokens_base_offset = Split(tokens[1],"+");
                string b_n = tokens_base_offset[0];

                // And if there is anything after that, separated by plus signs,
                // that'll be the offset
                int off = 0;
                for( size_t i=1; i<tokens_base_offset.size(); i++) {
                    off += stoi(tokens_base_offset[i]);
                }

                // Okay, we have f_ b_ off
                // Use them to modify what should be an already-existing f_ record:
                COB_FIELD *pfield = Find(f_n);
                if( pfield ) {
                    pfield->b_name = b_n;
                    pfield->offset = off;
                }
            }
        }
    }

    ::fclose(f);
}


