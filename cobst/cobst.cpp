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
#include <regex>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include "csv.h"
#include "params.h"
#include "input_scan.h"
#include "profiler.h"
#include "vartree.h"
#include "cprint_py.h"

#define EQ ==
#define AND &&
#define OR ||

using namespace std;

void scan_test(const string &filename)
{
    PROFILER;
    ifstream ifs;
    ifs.open(filename,ifstream::in);
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

void CreateFullCSV(PARAMETERS &params,
                   const COB_LABELS &cob_labels,
                   const DATA_DESCRIPTIONS &data_description
                  )
{
    /*  During development, I created this all-encompassing output file that
        includes everything I had found up to the point where I started
        abandoning it.

        As I learned where we were going, and was able to create an output
        file that was more focused, this started to wither on the vine.

        If you are reading this, you might want to ask if it's still needed
        at all.
        */


    CSV_GENERATOR csv;
    enum Columns {
        SOURCE = 1,
        LINE,
        SENTENCE,
        QUALIFIED_PROGRAM,
        PROGRAM,
        ENTRY,
        DIVISION,
        SECTION,
        PARAGRAPH,
        SCOPE,
        GAS_SECTION,
        GAS_SYMBOL,
        UNDECORATED_NAME,
        GAS_LOCATION,
        STORAGE_SIZE,
        FIELD_SIZE,
        FIELD_POINTER,
        FIELD_OFFSET,
        FIELD_ATTR,
        ATTR_TYPE,
        ATTR_DIGITS,
        ATTR_SCALE,
        ATTR_FLAGS,
        ATTR_PIC_POINTER,
        PICTURE_STRING
    };

    csv.AddColumn(SOURCE,          "Source File");
    csv.AddColumn(LINE,            "Line Number");
    csv.AddColumn(SENTENCE,        "Sentence Number");
    csv.AddColumn(PROGRAM,         "Program Name");
    csv.AddColumn(DIVISION,        "Division");
    csv.AddColumn(ENTRY,           "Entry");
    csv.AddColumn(SECTION,         "Section");
    csv.AddColumn(PARAGRAPH,       "Paragraph");
    csv.AddColumn(SCOPE,           "Scope");
    csv.AddColumn(GAS_SECTION,     "Assembly Section");
    csv.AddColumn(GAS_SYMBOL,      "Assembly Symbol");
    csv.AddColumn(UNDECORATED_NAME,"Undecorated Name");
    csv.AddColumn(GAS_LOCATION,    "Relative Address");
    csv.AddColumn(STORAGE_SIZE,    "Storage Size");
    csv.AddColumn(FIELD_SIZE,      "Field Size");
    csv.AddColumn(FIELD_POINTER,   "Field Pointer");
    csv.AddColumn(FIELD_OFFSET,    "Field Offset");
    csv.AddColumn(FIELD_ATTR,      "Field Attribute Pointer");
    csv.AddColumn(ATTR_TYPE,       "Attr Type Code");
    csv.AddColumn(ATTR_DIGITS,     "Attr Digits");
    csv.AddColumn(ATTR_SCALE,      "Attr Scale");
    csv.AddColumn(ATTR_FLAGS,      "Attr Flags");
    csv.AddColumn(ATTR_PIC_POINTER,"Attr Pic Pointer");
    csv.AddColumn(PICTURE_STRING,  "Picture String");

    for(int i=0; i<cob_labels.size(); i++) {
        COB_LABEL cs = cob_labels[i];
        stringstream ss;
        ios_base::fmtflags deflags = ss.flags();

        csv.AddData(SOURCE,              cs.source_file);
        ss.flags(deflags);
        ss.str("");
        ss << cs.line_number;
        csv.AddData(LINE,                ss.str());
        ss.flags(deflags);
        ss.str("");
        ss << cs.sentence_number;
        csv.AddData(SENTENCE,           ss.str());
        csv.AddData(PROGRAM,            cs.program);
        csv.AddData(DIVISION,           "Procedure");
        csv.AddData(ENTRY,              cs.entry);
        csv.AddData(SECTION,            cs.section);
        csv.AddData(PARAGRAPH,          cs.paragraph);
        csv.AddData(GAS_SECTION,        cs.gas_section);
        csv.AddData(GAS_SYMBOL,         cs.gas_symbol);
        ss.flags(deflags);
        ss.str("");
        ss << "0x" << nouppercase << setfill('0') << std::setw(16)
           << std::hex << cs.gas_value;
        csv.AddData(GAS_LOCATION,        ss.str());
        csv.Normalize();
    }

    for(int i=0; i<data_description.size(); i++) {
        DATA_DESCRIPTION ds = data_description[i];
        stringstream ss;
        ios_base::fmtflags deflags = ss.flags();

        csv.AddData(SOURCE,                ds.source);
        ss.flags(deflags);
        ss.str("");
        ss << ds.line_number;
        csv.AddData(LINE,               ss.str());
        csv.AddData(PROGRAM,            ds.program_id);
        csv.AddData(DIVISION,           "Data");
        csv.AddData(SECTION,            ds.storage_type);
        csv.AddData(SCOPE,              ds.storage_scope);
        csv.AddData(ENTRY,              ds.cbl_symbol);
        csv.AddData(GAS_SECTION,        ds.gas_section);
        csv.AddData(GAS_SYMBOL,         ds.gas_symbol);
        csv.AddData(UNDECORATED_NAME,   ds.undecorated_symbol);
        csv.AddData(PICTURE_STRING,     ds.picture_string);

        ss.flags(deflags);
        ss.str("");
        ss << "0x" << nouppercase << setfill('0') << std::setw(16)
           << std::hex << ds.gas_value;
        csv.AddData(GAS_LOCATION, ss.str());

        if(ds.storage_type EQ "Storage") {
            ss.flags(deflags);
            ss.str("");
            ss << ds.storage_size;
            csv.AddData(STORAGE_SIZE,       ss.str());
        }

        if(         ds.storage_type EQ "Field"
                    OR  ds.storage_type EQ "Constant"
                    OR  ds.storage_type EQ "String"
                    OR  ds.storage_type EQ "Linkage") {
            ss.flags(deflags);
            ss.str("");
            ss << ds.field_size;
            csv.AddData(FIELD_SIZE,          ss.str());
        }

        csv.AddData(FIELD_POINTER,           ds.field_data_pointer);
        csv.AddData(FIELD_OFFSET,            ds.field_data_offset);
        csv.AddData(FIELD_ATTR,              ds.field_attr_pointer);

        if(ds.storage_type EQ "Attribute") {
            ss.flags(deflags);
            ss.str("");
            ss  << "0x" << nouppercase << setfill('0')
                << std::setw(2) << std::hex << ds.attr_type;
            csv.AddData(ATTR_TYPE,           ss.str());
            ss.flags(deflags);
            ss.str("");
            ss << ds.attr_digits;
            csv.AddData(ATTR_DIGITS,         ss.str());
            ss.flags(deflags);
            ss.str("");
            ss << ds.attr_scale;
            csv.AddData(ATTR_SCALE,          ss.str());
            ss.flags(deflags);
            ss.str("");
            ss  << "0x" << nouppercase << setfill('0') << std::setw(4)
                << std::hex << ds.attr_flags;
            csv.AddData(ATTR_FLAGS,          ss.str());
            csv.AddData(ATTR_PIC_POINTER,    ds.attr_pic_pointer);
        }

        csv.Normalize();
    }

    csv.Generate(params.full_csv);
}

static void
CreateCSV(PARAMETERS &params,
          const COB_LABELS &cob_labels,
          const VARIABLE_TREE &var_tree)
{
    /*  This file, originally designed to be a .CSV file, hence the
        function name, and now a tab-delineated file, contains the
        information needed downstream of us.  There are two sections,
        combined in order to avoid potential confusion.

        The first section contains line/address information, specifically
        .CBL source code line numbers associated with .text assembler
        relative addresses.

        The second section is read by the cprint.py Python extension to
        gdb.  It contains the information needed to map canonical COBOL
        identifiers ( A of B of C ) to the assembly language symbols so
        that COBOL variables can be displayed by gdb.

        The structure of the columns can be changed by moving them
        around or adding columns.  But the column header text is
        read by the downstream programs, so if you change the headers
        you'll have to change the matching code in cprint.py
        */


    CSV_GENERATOR csv;
    enum Columns {
        SOURCE = 1,
        LINE,
        SENTENCE,
        PROGRAM,
        DIVISION,
        SECTION,
        ENTRY,
        PARAGRAPH,
        GAS_SECTION,
        GAS_LOCATION,

        FULL_NAME,
        LEVEL,
        BASE_SYMBOL,
        FIELD_SYMBOL,
        ATTRIBUTE_SYMBOL,
        OFFSET,
        LIST_TYPE,
        LIST_SIZE,
        LIST_PICTURE,
        LIST_USAGE,
        OCCURS,
        FIELD_SIZE,
        ATTR_TYPE,
        ATTR_DIGITS,
        ATTR_SCALE,
        ATTR_FLAGS,
    };

    // This group describes line numbers and their relative addresses
    csv.AddColumn(SOURCE,          "Source File");
    csv.AddColumn(LINE,            "Line Number");
    csv.AddColumn(SENTENCE,        "Sentence Number");
    csv.AddColumn(PROGRAM,         "Program Name");     // Also by cprint
    csv.AddColumn(DIVISION,        "Division");         // Also by cprint
    csv.AddColumn(SECTION,         "Section");          // Also by cprint
    csv.AddColumn(ENTRY,           "Entry");
    csv.AddColumn(PARAGRAPH,       "Paragraph");
    csv.AddColumn(GAS_SECTION,     "Assembly Section");
    csv.AddColumn(GAS_LOCATION,    "Relative Address");

    // This group is used by the cprint Python extension to gdb.  It describes
    // COBOL variable contents, so that cprint can find and display them.
    //
    // The base_symbol is used by gdb/cprint to find the contents of the
    // variable.  We include field_ and attribute_ symbol for reference, but
    // cprint doesn't use them.  Instead, we populate the remaining fields
    // with everything we know (or can divine) so that cprint can figure
    // out as early as possible what variables it is dealing with.

    csv.AddColumn(FULL_NAME,       "Full Name");    // From .cbl.lst
    csv.AddColumn(LEVEL,           "Level");        // From .cbl.lst
    csv.AddColumn(BASE_SYMBOL,     "Base Symbol");  // From .h files
    csv.AddColumn(FIELD_SYMBOL,    "Field Symbol"); // From .h files
    csv.AddColumn(ATTRIBUTE_SYMBOL,"Attribute Symbol"); // From .h files
    csv.AddColumn(OFFSET,          "Offset");       // Offset into base_symbol
    csv.AddColumn(LIST_TYPE,       "List Type");    // From .cbl.lst
    csv.AddColumn(LIST_SIZE,       "List Size");    // From .cbl.lst
    csv.AddColumn(LIST_PICTURE,    "Picture");      // From .cbl.lst
    csv.AddColumn(LIST_USAGE,      "Usage");        // From .cbl.lst
    csv.AddColumn(OCCURS,          "Occurs");       // From .cbl.lst
    csv.AddColumn(FIELD_SIZE,      "Field Size");   // From .h files
    csv.AddColumn(ATTR_TYPE,       "Attr Type");    // From .h files
    csv.AddColumn(ATTR_DIGITS,     "Attr Digits");  // From .h files
    csv.AddColumn(ATTR_SCALE,      "Attr Scale");   // From .h files
    csv.AddColumn(ATTR_FLAGS,      "Attr Flags");   // From .h files

    // Do the line/address information from the .text
    for(int i=0; i<cob_labels.size(); i++) {
        COB_LABEL cs = cob_labels[i];

        if( cs.section EQ "Linkage" ) {
            // This information was previously transferred to the
            // data_description information, so don't put it out here because
            // it's potentially confusing.
            continue;
        }

        stringstream ss;
        ios_base::fmtflags deflags = ss.flags();

        csv.AddData(SOURCE,             cs.source_file);
        ss.flags(deflags);
        ss.str("");
        ss << cs.line_number;
        csv.AddData(LINE,               ss.str());
        ss.flags(deflags);
        ss.str("");
        ss << cs.sentence_number;
        csv.AddData(SENTENCE,           ss.str());
        csv.AddData(PROGRAM,            cs.program);
        csv.AddData(DIVISION,           "Procedure");
        csv.AddData(ENTRY,              cs.entry);
        csv.AddData(SECTION,            cs.section);
        csv.AddData(PARAGRAPH,          cs.paragraph);
        csv.AddData(GAS_SECTION,        cs.gas_section);
        ss.flags(deflags);
        ss.str("");
        ss << "0x" << nouppercase << setfill('0') << std::setw(16)
           << std::hex << cs.gas_value;
        csv.AddData(GAS_LOCATION,        ss.str());
        csv.Normalize();
    }

    /// Do the variable identifier information for gdb/cprint
    for(int i=0; i<var_tree.size(); i++) {
        const VAR_NODE var = var_tree[i];
        stringstream ss;

        csv.AddData(PROGRAM,           var.GetProgram());
        csv.AddData(DIVISION,          "Data");
        csv.AddData(SECTION,           var.GetSection());
        csv.AddData(FULL_NAME,         var.GetCanonicalName());
        ss.str("");
        ss << var.GetLevel();
        csv.AddData(LEVEL,             ss.str());
        csv.AddData(BASE_SYMBOL,       var.GetBaseSymbol());
        csv.AddData(FIELD_SYMBOL,      var.GetFieldSymbol());
        csv.AddData(ATTRIBUTE_SYMBOL,  var.GetAttributeSymbol());
        ss.str("");
        ss << var.GetOffset();
        csv.AddData(OFFSET,            ss.str());
        csv.AddData(LIST_TYPE,         var.GetType());
        ss.str("");
        ss << var.GetSize();
        csv.AddData(LIST_SIZE,         ss.str());
        csv.AddData(LIST_PICTURE,      var.GetPicture());
        csv.AddData(LIST_USAGE,        var.GetUsage());
        ss.str("");
        ss << var.GetOccurs();
        csv.AddData(OCCURS,            ss.str());
        ss.str("");
        ss << var.GetFieldSize();
        csv.AddData(FIELD_SIZE,        ss.str());
        ss.str("");
        ss << var.GetAttrType();
        csv.AddData(ATTR_TYPE,         ss.str());
        ss.str("");
        ss << var.GetAttrDigits();
        csv.AddData(ATTR_DIGITS,       ss.str());
        ss.str("");
        ss << var.GetAttrScale();
        csv.AddData(ATTR_SCALE,        ss.str());
        ss.str("");
        ss << var.GetAttrFlags();
        csv.AddData(ATTR_FLAGS,        ss.str());

        csv.Normalize();
    }

    csv.Generate(params.csv);
}

static string ZeroIsNull(const LONGLONG &v)
{
    stringstream ss;
    if( v != 0 ) {
        ss << v;
    }
    return ss.str();
}

static void
CreateSymc(PARAMETERS &params,
           const COB_LABELS &cob_labels,
           const VARIABLE_TREE &var_tree)
{
    /*  This routine creates the .sym.c file, which contains a global
        variable comprising the cross-reference information, followed
        by the magical construction that puts the cprint.py script
        into the .debug_script section of the object file created by
        compiling .sym.c
    */

    // First, the cross-reference string:

    string fname = params.cbl_filename.GetFname();

    stringstream ss;

    // Do the line/address information from the .text in the minimal form
    // used by cprint.py to figure out which program a trap has occurred in.
    string current_program = "";
    for(int i=0; i<cob_labels.size(); i++) {
        COB_LABEL cs = cob_labels[i];
        if( cs.section EQ "Linkage" ) {
            // This information was previously transferred to the
            // data_description information, so don't put it out here because
            // it's potentially confusing.
            continue;
        }

        if( cs.program != current_program) {
            current_program = cs.program;
            ss << "P|";
            ss << cs.line_number << "|";
            ss << cs.program;
            ss << "~";
            continue;
        }
    }
    for(int i=0; i<var_tree.size(); i++) {
        VAR_NODE cs = var_tree[i];
        ss << "D|";
        ss << cs.GetSection().substr(0,2) << "|";
        ss << ZeroIsNull(cs.GetLevel()) << "|" ;
        ss << cs.GetCanonicalName() << "|" ;
        ss << cs.GetBaseSymbol() << "|" ;
        ss << ZeroIsNull(cs.GetOffset()) << "|" ;
        ss << cs.GetType().substr(0,1) << "|" ;
        ss << ZeroIsNull(cs.GetSize()) << "|" ;
        ss << cs.GetPicture() << "|" ;
        ss << cs.GetUsage() << "|" ;
        ss << ZeroIsNull(cs.GetOccurs()) << "|" ;
        ss << ZeroIsNull(cs.GetAttrType()) << "|" ;
        ss << ZeroIsNull(cs.GetAttrDigits()) << "|" ;
        ss << ZeroIsNull(cs.GetAttrScale()) << "|" ;
        ss << ZeroIsNull(cs.GetAttrFlags()) << "~";
    }
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
    // Copy over the Python script, line by line, getting rid of comments
    char *py = new char[___python_cprint_py_len+1];
    py[0] = '\0';
    size_t index=0;
    while(index < ___python_cprint_py_len) {
        string seg;
        while(index < ___python_cprint_py_len) {
            char ch = ___python_cprint_py[index++];
            if(ch EQ '\n') {
                break;
            }
            seg += ch;
        }
        size_t nfound_comment = seg.find("#");
        if( nfound_comment != string::npos ) {
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
        }
    }

//    memcpy(py, ___python_cprint_py,___python_cprint_py_len);
//    py[___python_cprint_py_len] = '\0';
    ss <<  py;
    ss << "\n";

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

    return;
}

int main(int argc, char *argv[])
{
    PROFILER;

    // Decode the command line for our purposes.
    PARAMETERS params = GetParameters(argc, argv);
    if(!params.quiet) {
        cout << "cobst version " << VERSION << endl;
    }

    VARIABLE_TREE variable_tree;
    GAS_SYMBOLS gas_symbol_table;
    COB_LABELS cob_labels;
    DATA_DESCRIPTIONS data_description;

    string source = params.basename;
    const string GLOBAL = "GLOBAL";
    const string LOCAL = "LOCAL";
    const string MAIN = "MAIN";
    const string GAS = "GAS";

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

    // Read the symbol table from the end of the assembler listing file.
    gas_symbol_table.GetSymbolsFromFile(source,GAS,params.lst);

    // Next we scan the GnuCOBOL-generated .i file, which is a cleaned up
    // (de-commented) version of the .CBL source file.  We use it to find the
    // line numbers associated with PROGRAM-ID declarations.
    cob_labels.GetProgramIdLineNumbers(source,GAS,params.i,gas_symbol_table);

    // Walk the mixed C/assembler listing file.  Assign relative
    // addresses to .CBL line numbers.  We pass along the gas_symbol_table
    // so that gas_symbols can be assigned to line numbers.
    cob_labels.GetSymbolsFromFile(source,GAS,params.lst,gas_symbol_table);

    // Scan the .h files for storage locations
    data_description.ScanAllDotHFiles(params.cbl_filename.Path(),
                                      params.cbl_filename.GetFname());

    // Here's a bit of a kludge for handling LINKAGE.  cob_labels has linkage
    // information extracted from the cob_check_linkage() debugging statements
    // found in .lst.  But we need that information in data_descriptions.
    // Force it in here:
    data_description.AppendLinkageInformation(cob_labels);

    // We end up with two pieces.  The .text information, and a little bit of
    // Linkage information, ends up in cob_labels.  Here we assign gas symbols
    // (if there are any) to the relative addresses assigned to the COBOL
    // line numbers:
    cob_labels.MergeGasSymbols(gas_symbol_table);

    // And here we look at the data description data names (as found by
    // scanning the .h files) and updating those records with information in
    // the gas_symbol table:
    data_description.MergeGasSymbols(gas_symbol_table);

    // The most complete description of the COBOL variable identifiers is
    // found int variable_tree.  But as of right now, it knows nothing about
    // where those variables can be found in the assembly output.
    // data_description has some COBOL/assembler cross references, but it is
    // incomplete, because it doesn't have the canonical COBOL names

    // data_description does have b_? base address information for every
    // WORKING-STORAGE entry, but it only has f_ field entries for variables
    // actually referenced in the program.  So, we're going to merge the
    // data_description information into variable_tree

    variable_tree.Merge(data_description);
    //variable_tree.Dump();

    // Create our original .TAB file.
    CreateFullCSV(params,cob_labels,data_description);

    // Create our actual useful .TAB file.
    CreateCSV(params,cob_labels,variable_tree);

    // Create the .sym.c file that will be compiled and linked into the
    // executable, providing the text that will become the executable's
    // .debug_gdb_scripts section
    CreateSymc(params,cob_labels,variable_tree);

    return 0;
}

/*

#if defined (__STDC__) || defined (ALMOST_STDC) || defined (HAVE_STRINGIZE)
#define STRINGX(s) #s
#else
#define STRINGX(s) "s"
#endif

#define XSTRING(s) STRINGX(s)
#define SECTION_SCRIPT_ID_PYTHON_TEXT 4




*/