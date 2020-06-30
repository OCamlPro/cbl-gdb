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
#include <string>
#include <fstream>
#include <map>
#include <sstream>
#include <iomanip>
#include <string.h>
#include "csv.h"
#include "params.h"
#include "input_scan.h"
#include "profiler.h"
#include "vartree.h"
#include "cobcd_py.h"

#define EQ ==
#define AND &&
#define OR ||

using namespace std;

//#define DUMPING

void
scan_test(const string &filename)
{
    PROFILER;
    ifstream ifs;
    ifs.open(filename.c_str(),ifstream::in);
    FILE *f = fopen(filename.c_str(),"r");

    string input;
    int nlines;
    nlines = 0;
    LAPTIME;
    for(;;) {
        getline(ifs,input);
        if(ifs.eof()) {
            break;
        }
        nlines += 1;
    }
    LAPTIME;
    cout << nlines << endl;
    LAPTIME;

    char ach[2048];
    nlines = 0;
    while(fgets(ach,sizeof(ach),f)) {
        nlines += 1;
    }
    LAPTIME;
    cout << nlines << endl;


    ifs.close();
    fclose(f);
}

static void
CreateSymc(PARAMETERS &params,
           const COB_PROGRAM_LABELS &program_labels,
           const VARIABLE_TREE &variable_tree,
           bool tack_on_python)
{
    /*  This routine creates the .sym.c file, which contains a global
        variable comprising the cross-reference information, followed
        by the magical construction that puts the cobcd.py script
        into the .debug_script section of the object file created by
        compiling .sym.c
    */

    string fname = params.c_filename.GetFname();
    stringstream ss_tab;
    stringstream ss;

    // Start off the output with the program section information:
    program_labels.FormatProgramInfo(ss);

    // Continue with the data variables information:
    variable_tree.FormatVariablesInfo(ss);

    // Do the line/address information from the .text in the minimal form
    // used by cobcd.py to figure out which program a trap has occurred in.
    string current_program = "";

    // At this point, ss contains a potentially huge literal string.
    // We're going to bust it up into smaller pieces.

    string s = ss.str();
    ss.str("");

    ss << "char VARIABLE_STRING_" << Upper(fname) << "[]=" << endl;

    const int SEGMENT_LENGTH = 78;
    while(s.length() > SEGMENT_LENGTH) {
        ss << "\"" << s.substr(0,SEGMENT_LENGTH) << "\"" << endl;
        s = s.substr(SEGMENT_LENGTH);
    }
    if( s.length() ) {
        ss << "\"" << s << "\"" << endl;
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
        if( !in_copyright AND nfound_comment != string::npos ) {
            // There is a pound sign, which is the Python comment signal
            size_t nfound_quote = seg.find_first_of("'\"");
            if( nfound_quote > nfound_comment ) {
                seg = seg.substr(0,nfound_comment);
                seg = RTrim(seg);
            }
        }

        if( !Trim(seg).empty() ) {
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

    COB_PROGRAM_LABELS   program_labels;
    VARIABLE_TREE        variable_tree;
    COB_PIC_SYMBOLS      cob_pic_symbols;
    COB_DATA             cob_data;
    COB_FIELD_ATTRIBUTES cob_field_attributes;
    COB_FIELDS           cob_fields;
    DATA_DESCRIPTIONS    data_description;

    string source = params.basename;
    const string GLOBAL = "GLOBAL";
    const string LOCAL = "LOCAL";
    const string MAIN = "MAIN";

    // We embark on a multi-step process, where we scan a number of
    // different files separately.  This is a bit like putting a jigsaw
    // puzzle together.

    // One of the files generated by GnuCOBOL is named .cbl.lst.  This is
    // a listing of the COBOL program, with the PROCEDURE division sort of
    // normalized.  We're interested in the last part of the file, which has
    // a normalized rendition of the parsed STORAGE sections.  We read that
    // in, and build a COBOL identifier tree based on the various storage
    // sections and the COBOL 77 01 02 03 data heirarchy.
    variable_tree.ReadFromFile(params.cbl_lst);
    variable_tree.BuildCanonicalName();
#if defined(DUMPING)
    //variable_tree.Dump();
#endif

    // Scan the .h files for working storage and local storage locations
    ScanAllDotHFiles(params.c_filename.Path(),
                     params.c_filename.GetFname(),
                     cob_pic_symbols,
                     cob_data,
                     cob_field_attributes,
                     cob_fields
                    );

    // Scan the .c file for program identification and linkage information.
    ScanForLinkage(params.c_file,
                   params.c_filename.GetFname(),
                   program_labels,
                   cob_data,
                   cob_fields
                  );
#if defined(DUMPING)
    cout << endl;
    program_labels.Dump();
    cob_pic_symbols.Dump();
    cob_data.Dump();
    cob_field_attributes.Dump();
    cob_fields.Dump();
#endif
    // We now have a nice complete list of COB_DATA, COB_FIELD_ATTRIBUTES, COB_FIELDS
    // Flatten them down into the composite table:

    data_description.Flatten(cob_fields,cob_field_attributes,cob_pic_symbols,cob_data);
#if defined(DUMPING)
    data_description.Dump();
#endif

    // The most complete description of the COBOL variable identifiers is
    // found in variable_tree.  But as of right now, it knows nothing about
    // where those variables can be found in the assembly output.
    // data_description has some COBOL/assembler cross references, but it is
    // incomplete, because it doesn't have the canonical COBOL names

    // The Consolidate() routine puts them together.

    variable_tree.Consolidate(data_description);
#if defined(DUMPING)
    variable_tree.Dump();
    variable_tree.DumpFlatList();
#endif
    // Create the .sym.c file that will be compiled and linked into the
    // executable, providing the text that will become the executable's
    // .debug_gdb_scripts section
#if defined(_WIN32)
    bool tack_on_python = false;
#else
    bool tack_on_python = true;
#endif
    CreateSymc(params,
               program_labels,
               variable_tree,
               tack_on_python);
    return 0;
}
