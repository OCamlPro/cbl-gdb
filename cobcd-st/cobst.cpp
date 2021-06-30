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
#include <string>
#include <fstream>
#include <map>
#include <sstream>
#include <iomanip>
#include <string.h>
#include "params.h"
#include "profiler.h"
#include "cobcd_py.h"
#include "fields.h"
#include "variables.h"
#include "utils.h"
#include "ctype.h"
#define EQ ==
#define AND &&
#define OR ||

using namespace std;

//#define DUMPING

// When this global variable is on, every input line is echoed
bool GV_echo_line = false;

static void
TackVariablesOntoCfile(PARAMETERS &params,
                       const VARIABLES &variables,
                       const INSERT_MODE insert_mode)
{
    PROFILER;
    /*  This routine appends to the C file the cross-reference information, followed
    by the magical construction that puts the cobcd.py script into the .debug_gdb_script
    section of the ELF file.
    */

    string fname = params.cbl_filename.GetFandExt();
    stringstream ssVariables;

    variables.FormatVariablesInfo(ssVariables);

    // At this point, ssVariables contains a potentially huge literal string.
    // The Python code, in order to read it fairly efficiently, needs to know
    // how long it is.  We are going to prepend a nine digit number to the
    // beginning of the string

    char ach[16384];
    sprintf(ach,"%9.9u",(int)ssVariables.str().size()+9);

    string sVariables = ach;
    sVariables += ssVariables.str();

    // Build the C-code for the global variable string
    stringstream ssResult;

    ssResult << "/* COBCD DEBUGGING INFORMATION */" << endl << endl;

    // Start with the name
    string fname2;
    for(size_t i=0; i<fname.size(); i++) {
        char ch = fname[i];
        if( isalpha(ch) OR isdigit(ch) OR ch EQ '_'  ) {
            fname2 += ch ;
        } else {
            char ach[3];
            sprintf(ach,"%2.2X",ch);
            fname2 += ach;
        }
    }
    ssResult << "static char VARIABLE_STRING_" << fname2 << "[]=" << endl;

    size_t walker = 0;
    size_t length = (int)sVariables.length();
    while( walker < length ) {
        size_t squiggle = sVariables.find('~',walker);
        if( squiggle EQ string::npos ) {
            break;
        }
        squiggle +=1;
        ssResult << "\"" << sVariables.substr(walker,squiggle-walker) << "\"" << endl;
        walker = squiggle;
    }
    if( walker < length ) {
        ssResult << "\"" << sVariables.substr(walker) << "\"" << endl;
    }
    ssResult << ";" << endl << endl;

    // cobcd.py uses GDB commands to call libcob functions.  This worked great on
    // Linux/Solaris, but GDB couldn't "find" some LIBCOB functions by name because
    // of the way linkage to .DLL exports is done.  So, we put "wrapper" function calls
    // to LIBCOB routines into the generated .C here.  GDB will be able to find these
    // routines because their names are known, and global:

    ssResult << "static void *DEBUG_cob_malloc_" << fname2 << "(const size_t size)" << endl;
    ssResult << "{return cob_malloc(size);}" << endl;
    ssResult << endl;

    ssResult << "static void DEBUG_cob_free_" << fname2 << "(void *location)" << endl;
    ssResult << "{cob_free(location);}" << endl;
    ssResult << endl;

    ssResult << "static void DEBUG_cob_move_" << fname2 << "(cob_field *f1, cob_field *f2)" << endl;
    ssResult << "{cob_move(f1,f2);}" << endl;
    ssResult << endl;

    ssResult << "static int DEBUG_cob_encode_program_id_" << fname2 << "(const unsigned char * const a, unsigned char * const b, const int c, const int d)" << endl;
    ssResult << "{return cob_encode_program_id(a, b, c, d);}" << endl;
    ssResult << endl;

    ssResult << "static const char	*DEBUG_cob_get_field_str_buffered_" << fname2 << "(const cob_field *p)" << endl;
    ssResult << "{return cob_get_field_str_buffered(p);}" << endl;
    ssResult << endl;

    ssResult << "static int DEBUG_cob_cmp_" << fname2 << "(cob_field *a, cob_field *b)" << endl;
    ssResult << "{return cob_cmp(a, b);}" << endl;
    ssResult << endl;

    ssResult << "static cob_field *DEBUG_cob_intr_upper_case_" << fname2 << "(const int a, const int b, cob_field *c)" << endl;
    ssResult << "{return cob_intr_upper_case(a,b,c);}" << endl;
    ssResult << endl;

    if(insert_mode EQ IM_MODE_TEXT) {
        stringstream ssPython;
        ssPython.str("");
        bool in_copyright = true;
        // Copy over the Python script, line by line, getting rid of comments
        // that appear after the copyright notice
        char *py = new char[___python_cobcd_py_len+1];
        py[0] = '\0';
        size_t index=0;
        while(index < ___python_cobcd_py_len) {
            // Read the next line from the .h file
            string seg;
            while(index < ___python_cobcd_py_len) {
                char ch = ___python_cobcd_py[index++];
                if(ch EQ '\n') {
                    break;
                }
                seg += ch;
            }
            size_t nfound_comment = seg.find("#");
            if(false AND !in_copyright AND nfound_comment != string::npos ) {
                // There is a pound sign, which is the Python comment signal
                size_t nfound_quote = seg.find_first_of("'\"");
                if( nfound_quote > nfound_comment ) {
                    seg = seg.substr(0,nfound_comment);
                    seg = RTrim(seg);
                }
            }

            seg += '\n';
            size_t t = strlen(py);
            strcpy(py+t,seg.c_str());

            if( seg.find("EndOfCopyright") != string::npos ) {
                in_copyright = false;
            }
        }

        //    memcpy(py, ___python_cobcd_py,___python_cobcd_py_len);
        //    py[___python_cobcd_py_len] = '\0';
        ssPython <<  py;
        ssPython << "\n";

        delete[] py;

        /*  As per https://sourceware.org/gdb/current/onlinedocs/gdb/dotdebug_005fgdb_005fscripts-section.html,
        we are going to format that string in a way that reflects this example:

        #include "symcat.h"
        #include "gdb/section-scripts.h"
        asm(
        ".pushsection \".debug_gdb_scripts\", \"MS\",@progbits,1\n"
        ".byte " XSTRING (SECTION_SCRIPT_ID_PYTHON_TEXT) "\n"
        ".ascii \"gdb.inlined-script\\n\"\n"
        ".ascii \"class test_cmd (gdb.Command):\\n\"\n"
        ".ascii \"  def __init__ (self):\\n\"\n"
        ".ascii \"    super (test_cmd, self).__init__ (\\\"test-cmd\\\", gdb.COMMAND_OBSCURE)\\n\"\n"
        ".ascii \"  def invoke (self, arg, from_tty):\\n\"\n"
        ".ascii \"    print (\\\"test-cmd output, arg = %s\\\" % arg)\\n\"\n"
        ".ascii \"test_cmd ()\\n\"\n"
        ".byte 0\n"
        ".popsection\n"
        );
        */

        string sPython = ssPython.str();

        ssResult << "asm(" << endl;
        ssResult << "\".pushsection \\\".debug_gdb_scripts\\\", \\\"MS\\\",@progbits,1\\n\"" << endl;
        ssResult << "\".byte 4\\n\"" << endl;
        ssResult << "\".ascii \\\"gdb.inlined-script\\\\n\\\"\\n\"";

        int char_count = 0;
        size_t i = 0;
        while( i < sPython.size() ) {
            if(char_count EQ 0) {
                ssResult << "\".ascii \\\"";
                char_count = 10 ;
            }
            if( char_count >= 120 ) {
                ssResult << "\"" << endl << "\"";
                char_count = 1;
                continue;
            }
            char ch = sPython[i++];
            if( ch EQ '\n' ) {
                ssResult << "\\\\n\\\"\\n\"" << endl;
                char_count = 0;
                continue;
            }
            if( ch EQ '\\' OR ch EQ '\"' ) {
                ssResult << "\\\\\\";
                char_count += 1;
            }
            ssResult << ch;
            char_count += 1;
        }

        ssResult << "\".byte 0\\n\"" << endl;
        ssResult << "\".popsection\\n\"" << endl;
        ssResult << ");" << endl;
    }

    if(insert_mode EQ IM_MODE_FILE) {

        /*  As per https://sourceware.org/gdb/current/onlinedocs/gdb/dotdebug_005fgdb_005fscripts-section.html,
        we are going to format that string in a way that reflects this example:

        asm("
        .pushsection \".debug_gdb_scripts\", \"MS\",@progbits,1\n
        .byte 1 \n
        .asciz \"cobcd.py\"\n
        .popsection \n
        ");

        */

        ssResult << "asm(" ;
        ssResult << "\".pushsection \\\".debug_gdb_scripts\\\", \\\"MS\\\",@progbits,1\\n\"" ;
        ssResult << "\".byte 1 \\n\"" ;
        ssResult << "\".asciz \\\"cobcd.py\\\"\\n\"" ;
        ssResult << "\".popsection\\n\"" ;
        ssResult << ");" ;

        ssResult << endl;
    }
    ssResult << endl;

    // ssResult is the text we want to tack onto the end of the .C file:

    string c_file_name = params.c_filename.WholePath();
    FILE *fcfile = fopen(c_file_name.c_str(),"r");
    if( !fcfile ) {
        cerr << "Failed to open input file \"" << c_file_name << "\"" << endl;
        cerr << "Error: " << strerror(errno) << endl;
        exit(1);
    }
    string c_file;
    while( fgets(ach,sizeof(ach),fcfile) ) {
        if( strstr(ach,"/* COBCD DEBUGGING INFORMATION */") EQ ach ) {
            // For the sake of idempotency.  This can happen during debugging and
            // development, when running COBST multiple times on the same input file
            break;
        }
        c_file += ach;
    }
    fclose(fcfile);
    fcfile = fopen(c_file_name.c_str(),"w");
    if( !fcfile ) {
        cerr << "Failed to open output file \"" << c_file_name << "\"" << endl;
        cerr << "Error: " << strerror(errno) << endl;
        exit(1);
    }
    fputs(c_file.c_str(),fcfile);
    fputs(ssResult.str().c_str(),fcfile);
    fclose(fcfile);

    return;
}


int
main(int argc, char *argv[])
{
    PROFILER;

    char *noisy=getenv("COBST_NOISY");
    if( noisy ) {
        int nnoisy=atoi(noisy);
        if( nnoisy ) {
            GV_echo_line = true;
        }
    }

    // Decode the command line for our purposes.
    PARAMETERS params;
    params.GetParameters(argc, argv);
    if(!params.quiet) {
        cout << "cobst version " << VERSION << endl;
    }

    COB_FIELDS cob_fields;
    cob_fields.ScanAllDotHFiles(params.c_filename.Path(),
                                params.c_filename.GetFname() );
    cob_fields.ScanCFile(params.c_filename.Path(),
                         params.c_filename.GetFname() );
#if defined(DUMPING)
    cob_fields.Dump();
#endif

    VARIABLES variables;
    variables.ScanCFile(params.c_filename.Path(),
                        params.c_filename.GetFname() );
    variables.FillInTheBlanks(cob_fields);
    variables.FixLocalStorageOffsets();
#if defined(DUMPING)
    variables.Dump();
#endif

    variables.BubbleUpProgramEntries();

    TackVariablesOntoCfile(params,
                           variables,
                           params.insert_mode
                          );

    return 0;

}
