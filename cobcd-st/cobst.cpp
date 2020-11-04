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
#define EQ ==
#define AND &&
#define OR ||

using namespace std;

//#define DUMPING

static void
CreateSymc(PARAMETERS &params,
           const VARIABLES &variables,
           bool tack_on_python,
           bool tack_on_filename)
{
    PROFILER;
    /*  This routine creates the .sym.c file, which contains a global
        variable comprising the cross-reference information, followed
        by the magical construction that puts the cobcd.py script
        into the .debug_script section of the object file created by
        compiling .sym.c
    */

    string fname = params.cbl_filename.GetFandExt();
    stringstream ss;

    variables.FormatVariablesInfo(ss);

    // At this point, ss contains a potentially huge literal string.
    // The Python code, in order to read it fairly efficiently, needs to know
    // how long it is.  We are going to prepend a nine digit number to the
    // beginning of the string

    char ach[16];
    sprintf(ach,"%9.9u",(int)ss.str().size()+9);

    string s = ach;
    s += ss.str();

    // Build the C-code for the global variable string
    ss.str("");

    // Start with the name
    for(size_t i=0; i<fname.size(); i++) {
        if( fname[i] EQ '.' ) {
            fname[i] = '_' ;
        }
    }
    ss << "char VARIABLE_STRING_" << Upper(fname) << "[]=" << endl;

    // Break up the actual data into smaller pieces, so that it will compile.
#if 0
    const int SEGMENT_LENGTH = 78;
    int walker = 0;
    int remaining = (int)s.length();
    while( remaining > SEGMENT_LENGTH ) {
        ss << "\"" << s.substr(walker,SEGMENT_LENGTH) << "\"" << endl;
        walker    += SEGMENT_LENGTH;
        remaining -= SEGMENT_LENGTH;
    }
    if( remaining ) {
        ss << "\"" << s.substr(walker) << "\"" << endl;
    }
#endif
    size_t walker = 0;
    size_t length = (int)s.length();
    while( walker < length ) {
        size_t squiggle = s.find('~',walker);
        if( squiggle EQ string::npos ) {
            break;
        }
        squiggle +=1;
        ss << "\"" << s.substr(walker,squiggle-walker) << "\"" << endl;
        walker = squiggle;
    }
    if( walker < length ) {
        ss << "\"" << s.substr(walker) << "\"" << endl;
    }
    ss << ";" << endl << endl;
    params.symc << ss.str();

    ss.str("");
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

        //if( !Trim(seg).empty() )
        {
            seg += '\n';
            size_t t = strlen(py);
            strcpy(py+t,seg.c_str());

            if( seg.find("EndOfCopyright") != string::npos ) {
                in_copyright = false;
            }
        }
    }

//    memcpy(py, ___python_cobcd_py,___python_cobcd_py_len);
//    py[___python_cobcd_py_len] = '\0';
    ss <<  py;
    ss << "\n";

    delete[] py;

    if(tack_on_python) {

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

        s = ss.str();

        params.symc << "asm(" << endl;
        params.symc << "\".pushsection \\\".debug_gdb_scripts\\\", \\\"MS\\\",@progbits,1\\n\"" << endl;
        params.symc << "\".byte 4\\n\"" << endl;
        params.symc << "\".ascii \\\"gdb.inlined-script\\\\n\\\"\\n\"";

        int char_count = 0;
        size_t i = 0;
        while( i < s.size() ) {
            if(char_count EQ 0) {
                params.symc << "\".ascii \\\"";
                char_count = 10 ;
            }
            if( char_count >= 120 ) {
                params.symc << "\"" << endl << "\"";
                char_count = 1;
                continue;
            }
            char ch = s[i++];
            if( ch EQ '\n' ) {
                params.symc << "\\\\n\\\"\\n\"" << endl;
                char_count = 0;
                continue;
            }
            if( ch EQ '\\' OR ch EQ '\"' ) {
                params.symc << "\\\\\\";
                char_count += 1;
            }
            params.symc << ch;
            char_count += 1;
        }

        params.symc << "\".byte 0\\n\"" << endl;
        params.symc << "\".popsection\\n\"" << endl;
        params.symc << ");" << endl;
    }

    if(tack_on_filename) {

        /*  As per https://sourceware.org/gdb/current/onlinedocs/gdb/dotdebug_005fgdb_005fscripts-section.html,
        we are going to format that string in a way that reflects this example:

         asm("
        .pushsection \".debug_gdb_scripts\", \"MS\",@progbits,1\n
        .byte 1 \n
        .asciz \"cobcd.py\"\n
        .popsection \n
        ");

        */

        s = ss.str();

        params.symc << "asm(" ;
        params.symc << "\".pushsection \\\".debug_gdb_scripts\\\", \\\"MS\\\",@progbits,1\\n\"" ;
        params.symc << "\".byte 1 \\n\"" ;
        params.symc << "\".asciz \\\"cobcd.py\\\"\\n\"" ;
        params.symc << "\".popsection\\n\"" ;
        params.symc << ");" ;

        params.symc << endl;
    }

    return;
}

int
main(int argc, char *argv[])
{
    PROFILER;

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

    bool tack_on_python = params.insert_python;
    bool tack_on_filename = params.insert_filename;
#if defined(_WIN32) || defined(__MINGW32__) || defined(__MINGW64__)
    tack_on_python = false;
#endif
    CreateSymc(params,
               variables,
               tack_on_python,
               tack_on_filename);

    return 0;

}
