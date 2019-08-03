#include <string.h>
#include <set>
#include "input_scan.h"
#include "profiler.h"
#include "params.h"

#ifdef WIN32
#define strcasecmp _stricmp
#endif

using namespace std;

#define EQ ==
#define AND &&
#define OR ||

string GetCommentText(const string &s)
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

void GAS_SYMBOLS::GetSymbolsFromFile(const string &source, const string &scope, ifstream &ifs)
{
    CPROFILER;
    ifs.clear();
    ifs.seekg(0);

    string input;
    const string defsym = "DEFINED SYMBOLS";
    for(;;) {
        getline(ifs,input);
        if(ifs.eof()) {
            break;
        }
        if(input.substr(0,defsym.size()) EQ defsym) {
            break;
        }
    }
    for(;;) {
        getline(ifs,input);
        if(ifs.eof()) {
            break;
        }

        // These are examples of what we are looking for:

        //  /tmp/ccAi92uI.s:329    .text:0000000000000041 RTEST_
        //  /tmp/ccAi92uI.s:316    .text:0000000000000041 .LFE28
        //  /tmp/ccAi92uI.s:4077   .data.rel.local:0000000000000010 f_12.7368

        // The regex will split out the line number, the section, the hex relative location, and the symbol


        static const regex symbol("[^:]+:([0-9]+)[ ]+([^:]+):([0-9a-gA-F]{16})[ ]+(.*)$");
        smatch sm;
        if(regex_match(input,sm,symbol)) {
            GAS_SYMBOL gs;
            gs.gas_section = sm[2];
            gs.line_number = stoi(sm[1]);
            gs.gas_value = stoll(sm[3],NULL,16);
            gs.gas_symbol = sm[4];
            gs.undecorated_symbol = sm[4];

            // Some f_ and b_ symbols, typically ones that are local to COBOL PROGRAM-ID routines,
            // get decorated with .nnnn suffixes.  We need to be able to find them without their
            // decorations:
            string first_two = gs.undecorated_symbol.substr(0,2);
            if(first_two EQ "f_" OR first_two EQ "b_") {
                size_t ndot = gs.undecorated_symbol.find('.',1);
                if(ndot != string::npos) {
                    gs.undecorated_symbol = gs.undecorated_symbol.substr(0,ndot);
                }

                unordered_map<string,size_t>::const_iterator its = gas_undecorated_symbol_location.find(gs.undecorated_symbol);
                if(its != gas_undecorated_symbol_location.end()) {
                    cout << "Disaster in GAS_SYMBOLS::GetSymbolsFromFile()!\n";
                    cout << "The symbol \"" << gs.undecorated_symbol << "\" has been seen more than once!\n" ;
                    cout << "This violates uniqueness in an unexpected way; two symbols must have the same\n";
                    cout << "root but different decorations.\n";
                    cout << "This has to be addressed.\n";
                    exit(1);
                }
            }

            symbols.push_back(gs);
            gas_undecorated_symbol_location[gs.undecorated_symbol] = symbols.size()-1;

            string find_me = SectionValueCombiner(gs.gas_section,gs.gas_value);
            unordered_map<string,size_t>::const_iterator it = reverse_gas_symbol_location.find(find_me);

            // Some symbols will get both .Local assignments along with the ones generated more directly
            // from the C code.  The map can only have one string associated with a relative location;
            // prefer one that is generated from the C code.

            string already = ".";
            if(it != reverse_gas_symbol_location.end()) {
                size_t n = it->second;
                already = symbols[n].gas_symbol;
            }
            if(already[0] EQ '.') {
                // If a previous symbol is not a local symbol, keep it.
                reverse_gas_symbol_location[find_me] = symbols.size()-1;
            }
        }
    }
}

void GAS_SYMBOLS::UpdateBaseOffset(string symbol, string base, string offset)
{
    unordered_map<string,size_t>::iterator it = gas_undecorated_symbol_location.find(symbol);
    if(it != gas_undecorated_symbol_location.end()) {
        size_t index = it->second;
        symbols[index].field_data_pointer = base;
        symbols[index].field_data_offset = offset;
    }
}

void COB_LABELS::GetSymbolsFromFile(const string &source, const string &scope, ifstream &ifs, GAS_SYMBOLS &gas_symbol_table)
{
    CPROFILER;
    ifs.clear();
    ifs.seekg(0);

    // We assign a .text relative address to each line of the original COBOL
    // code.  We can do that because each line of COBOL code appears in the
    // generated C code as a /* Line : nnn */ comment.

    // The second thing we do is parse for cob_check_linkage() calls and COB_SET_DATA
    // macros.  We are able to extract some information from them about linkage to
    // information that is stored on the C-generated stack, and on the gas-generated
    // stack.  In actuality, this isn't useful, and doing what I can is basically a stunt.
    //
    // That's because critical information about LOCAL-STORAGE data names never finds its
    // way into the output files.  The generated C code just implements stuff without telling
    // us about it in the generated files.
    //
    // So, although this stunt is fun, it's not a complete solution.  We're going to have to
    // modify the GnuCOBOL compiler to emit the data we need.

    /*

    This routine will cover a lot of ground.  The basic principle is that it will look for
    **** comments in the assembly language and associate them with GAS-generated symbols
    and pre-link relative addresses in the non-commented code.

    Some of those assembler comments will contain /* Line: xxxx *-/ C comments from the
    C code generated by the GnuCOBOL compiler.  In that case, xxxx refers back to the
    line number in the original .CBL COBOL source code.

    There are a bunch of variations:

    Here is a paragraph that is referenced by a PERFORM WRT-LN statement elsewhere
    in the COBOL code

    1024:NC101A.c      ****
    1025:NC101A.c      ****   /* Line: 403       : Paragraph WRT-LN                  : NC101A.CBL *-/
    1026:NC101A.c      ****   l_20:;
    5804                    .loc 2 1026 0
    5805 2ea8 90            nop
    5806                .L114:
    1027:NC101A.c      ****   module->module_stmt = 0x00100193;
    5807                    .loc 2 1027 0
    5808 2ea9 488B0500      movq    module.7676(%rip), %rax

    The PERFORM code puts the return address into a C structure named frame_ptr, and then, in the C code,
    does a "goto l_20:"  The assembler doesn't use the code location addresses from the C code; it instead
    did its own mapping.  So, the PERFORM invocation gets compiled and assembled into a "jmp .L114".  In the
    symbol table, that jmp goes to relative location 0x0000000000002ea9.

    Note how the "nop" instruction is ignored here.

    Here is a paragraph that is never referenced by a PERFORM, but is the target of a GO TO

    6090                .L144:
    1140:NC101A.c      ****
    1141:NC101A.c      ****   /* Line: 417       : Paragraph FAIL-ROUTINE-WRITE      : NC101A.CBL *-/
    1142:NC101A.c      ****   l_23:;
    1143:NC101A.c      ****   module->module_stmt = 0x001001A1;
    6091                    .loc 2 1143 0
    6092 336f 488B0500      movq    module.7676(%rip), %rax

    In this case, even though the C code uses the l_23: label, the gcc compiler didn't assign
    a symbolic location after the Paragraph declaration.  Nonetheless, the GO TO code elsewhere
    in the listing shows that a "jmp .L144" was generated; the symbol table shows that .L144 was
    assigned the value 0x000000000000336f

    Here is a paragraph that is not the target of a GO TO or of a simple perform, but is
    included in PERFORM END-ROUTINE THRU END-ROUTINE-13.  There are five paragraphs involved:

    END-ROUTINE
    END-RTN-EXIT
    END-ROUTINE-1
    END-ROUTINE-12
    END-ROUTINE-13

    END-ROUTINE-1 is not the target of a GO TO or PERFORM statements; it only ever gets
    executed because of that PERFORM ... THROUGH ... statement.

    4569 1836 7F97          jg  .L89
    592:NC101A.c      ****   }
    593:NC101A.c      ****
    594:NC101A.c      ****   /* Line: 358       : Paragraph END-ROUTINE-1           : NC101A.CBL *-/
    595:NC101A.c      ****   module->module_stmt = 0x00100166;
    4570                    .loc 2 595 0 is_stmt 1
    4571 1838 488B0500 .    movq    module.7676(%rip), %rax

    Note that it doesn't have a GAS symbol either before, or after, the Line: and Paragraph information.
    But it is important to us to know that the relevant code is located at relative address
    0x0000000000001838.
    */

    string input;
    vector<size_t>cbl_symbols_needing_addresses;
    vector<size_t>cbl_lines_needing_addresses;
    vector<string>program_id_stack;
    string current_entry;
    string current_section;
    string current_paragraph;
    int current_line_number = -1;
    int current_sentence_number = 1;

    set<string>linkage_weve_seen;
    for(;;) {
        getline(ifs,input);
        if(ifs.eof()) {
            break;
        }
        vector<string>tokens = Split(input," \t");
        bool is_comment = tokens.size() >= 2 AND tokens[1] EQ "****";
        if(is_comment) {
            // This is the big divide.  Four-star comments are the GnuCOBOL-generated C
            // source code lines.  They can contain information we need, in particular
            // about PROGRAM-ID, SECTION, ENTRY, and PARAGRAPH names that we want to associate
            // with relative addresses in the machine code.
            //
            // Note: I am aware that four stars really means just "the C part of the mixed C/assembler
            // output.  But "comment" is how I started thinking about it, and it stuck.
            //
            // Try for   ===>  49:rtest.c       **** /* PROGRAM-ID 'RTEST' */ <===
            //
            if(tokens.size() EQ 6 AND tokens[3] EQ "PROGRAM-ID") {
                // Pick up the program-id by removing the single-quotes that bracket it:
                string program_id = tokens[4].substr(1);
                program_id = program_id.substr(0,program_id.size()-1);
                program_id_stack.push_back(program_id);

                // We should have an entry for this program-id in program_id_lines:

                unordered_map<string,int>::const_iterator it = program_id_lines.find(program_id);
                if( it != program_id_lines.end() ) {
                    int line_number = it->second;

                    /*  Here's our situation:  We have found a point in the assembly language that
                        corresponds to ==>ID DIVISION. PROGRAM-ID. program_id.<==  In order to make
                        stepping through COBOL code with a debugger make sense, we need to
                        get this line_number into the line_numbers_needing_addresses stack.  But that
                        has to be done in an appropriate way.  So, we're going to modify our list
                        of tokens so that we co-opt the code down below to serve our purposes:

                    */
                    current_entry.clear();
                    current_section.clear();
                    current_paragraph.clear();

                    tokens.clear();
                    tokens.push_back("000:dummy.c");    //  0
                    tokens.push_back("****");           //  1
                    tokens.push_back("/*");             //  2
                    tokens.push_back("Line:");          //  3
                    stringstream ss;
                    ss << line_number;
                    tokens.push_back(ss.str());         //  4
                    tokens.push_back(":");              //  5
                    tokens.push_back("PROGRAM-ID");     //  6
                    tokens.push_back(program_id);       //  7
                }
                // And fall through to process that pseudo-line
            }

            //
            // Try for   ===>  223:rtest.c       **** /* End PROGRAM-ID 'RTEST' */ <===
            //
            if(tokens.size() EQ 7 AND tokens[3] EQ "End" AND tokens[4] EQ "PROGRAM-ID") {
                current_entry.clear();
                current_section.clear();
                current_paragraph.clear();
                program_id_stack.pop_back();
                continue;
            }

            if(tokens.size() >= 6 AND tokens[3] EQ "Line:") {
                int line_number = stoi(tokens[4]);
                if(line_number EQ current_line_number) {
                    // We are dealing with multiple sentences on a single line
                    current_sentence_number += 1;
                } else {
                    current_line_number = line_number;
                    current_sentence_number = 1;
                    if(line_number EQ 0) {
                        current_paragraph.clear();
                        current_section.clear();
                        current_entry.clear();
                    }
                }

                bool symbol_needing_target = false;
                if(tokens[6] EQ "Paragraph") {
                    /*
                    // Why this code existed, except to concatenate
                    // "Default Error Handler", I don't remember
                    // RJD 2019-07-23, cobc 3.0 rc1
                    int i=7;
                    current_paragraph = tokens[i++];
                    while( !tokens[i].empty() AND tokens[i] != ":")
                        {
                        current_paragraph += " ";
                        current_paragraph += tokens[i++];
                        }
                    symbol_needing_target = true;
                    */
                    current_paragraph = tokens[7];
                    size_t nfound = current_paragraph.find(":");
                    if(nfound != string::npos) {
                        current_paragraph = current_paragraph.substr(0,nfound);
                    }


                    symbol_needing_target = true;
                } else if(tokens[6] EQ "Section") {
                    current_paragraph.clear();
                    current_section = tokens[7];
                    symbol_needing_target = true;
                } else if(tokens[6] EQ "Entry") {
                    current_paragraph.clear();
                    current_entry = tokens[7];
                    symbol_needing_target = true;
                }

                // We will create an entry for every line number. By doing so, we are
                // building up a symbol table entry for every line of COBOL code.
                COB_LABEL cs;
                cs.line_number = current_line_number;
                cs.sentence_number = current_sentence_number;
                if(program_id_stack.size() >= 1) {
                    cs.program = program_id_stack.back();
                }
                cs.entry = current_entry;
                cs.section = current_section;
                cs.paragraph = current_paragraph;
                cs.gas_section = ".text";
                symbols.push_back(cs);

                if(symbol_needing_target) {
                    cbl_symbols_needing_addresses.push_back(symbols.size()-1);
                } else {
                    cbl_lines_needing_addresses.push_back(symbols.size()-1);
                }
                continue;
            }

            if( tokens.size() EQ 5
                    AND tokens[2] EQ "#define"
                    AND tokens[3] EQ "COB_SOURCE_FILE" ) {
                source_file = tokens[4].substr(1);
                source_file = source_file.substr(0,source_file.size()-1);
                continue;
            }

            // There is information to be gleaned from various C calls.
            bool to_be_continued = false;
            if( tokens.size() >= 3 AND tokens[2] EQ "cob_check_linkage" ) {
                smatch sm;
                static const regex linkage(".*\\(([^,]+),[^']+[']([^']+).*$");
                if(regex_match(input,sm,linkage)) {
                    if( linkage_weve_seen.find(sm[1]) EQ linkage_weve_seen.end() ) {
                        linkage_weve_seen.insert(sm[1]);
                        COB_LABEL cs;
                        cs.line_number = current_line_number;
                        cs.sentence_number = current_sentence_number;
                        if(program_id_stack.size() >= 1) {
                            cs.program = program_id_stack.back();
                        }
                        cs.entry = sm[2];
                        cs.section = "Linkage";
                        cs.paragraph = current_paragraph;
                        // Linkage isn't in .text, but rather .data.rel.local
                        cs.gas_section = ".data.rel.local";
                        cs.gas_symbol = sm[1];
                        symbols.push_back(cs);
                    }
                }
                // No continue statement here, because the line might also
                // contain COB_SET_DATA
                to_be_continued = true;
            }
            size_t ncob = 0;
            while((ncob = input.find("COB_SET_DATA",ncob)) != string::npos) {
                // We know we'll get at least one hit, so we can do the expensive regex_match with confidence:
                smatch sm;
                static const regex csd("COB_SET_DATA [(]([^,]+), ([^)]+).*$");
                string spiece = input.substr(ncob);
                if(regex_match(spiece,sm,csd)) {
                    string target = sm[1];
                    string source = sm[2];

                    string base = sm[2];
                    string offset;
                    static const regex offset_match("(.*) [+] (.*)");
                    smatch sm2;
                    if(regex_match(source,sm2,offset_match)) {
                        base = sm2[1];
                        offset = sm2[2];
                    }
                    gas_symbol_table.UpdateBaseOffset(target,base,offset);
                }

                ncob += 12;
                to_be_continued = true;
            }
            if(to_be_continued) {
                continue;
            }

            if(     tokens.size() EQ 7
                    AND tokens[3] EQ "Implicit"
                    AND tokens[4] EQ "PERFORM"
                    AND tokens[5] EQ "return") {
                /*

                More hacking.  I can't think of a clean way to do any of this; until we can
                fix -- or Lord help us, replace -- the compiler, I'll just have to just keep hacking.

                You see, gdb seems to think that it's going to be working with subroutines:  You call
                them, they set up a stack frame, they operate, and they return.  But GnuCOBOL implements,
                of course, the ability to PERFORM sections or paragraphs, and to PERFORM THROUGH paragraphs.

                This is implemented in C through a software frame.  At the point of the PERFORM, a series
                of instructions are coded:

                        frame_ptr->return_address_ptr = &&l_38
                        goto l_18;  // This is address of the paragraph code
                        l_38:

                The paragraph code ends with:

                        goto *frame_ptr->return_address_ptr;

                Full disclosure: I didn't know you could do that.

                Anyway, gdb seems to be able to handle this situation.  But when the last thing
                in a code module is a PARAGRAPH that can be performed, it seems to lose track of
                the single-stepping line numbers when executing an implicit return.

                This attempts to stick in a pseudo line number in order to fix that.


                */
                if(cbl_lines_needing_addresses.empty()) {
                    // Only do this if nobody else is in there already
                    COB_LABEL cs;
                    cs.line_number = current_line_number+1;
                    cs.sentence_number = current_sentence_number;
                    if(program_id_stack.size() >= 1) {
                        cs.program = program_id_stack.back();
                    }
                    cs.entry = current_entry;
                    cs.section = current_section;
                    cs.paragraph = current_paragraph;
                    cs.gas_section = ".text";
                    symbols.push_back(cs);
                    cbl_lines_needing_addresses.push_back(symbols.size()-1);
                }
            }
        } else {
            //  Here is where we process an assembly language line
            //  of the mixed C/assembler output.

            static const regex decimal("[\\d]+");
            static const regex hex("[0-9a-fA-F]+");

            if(input[0] EQ ' ' AND tokens.size() >= 4) {
                // A typical line is
                //          3257       01f0        488B8570      movq   -4240(%rbp), %rax
                // That is LineNumber HexLocation MachineCode   InstructionMnemonics
                // We are interested in the second one, the hex relative address
                if(regex_match(tokens[0],decimal) AND regex_match(tokens[1],hex) AND regex_match(tokens[2],hex) ) {
                    LONGLONG gas_value = stoll(tokens[1],NULL,16);
                    for(size_t i=0; i<cbl_lines_needing_addresses.size(); i++) {
                        symbols[cbl_lines_needing_addresses[i]].gas_value = gas_value;
                    }
                    cbl_lines_needing_addresses.clear();
                    if(tokens[3] != "nop") {
                        for(size_t i=0; i<cbl_symbols_needing_addresses.size(); i++) {
                            symbols[cbl_symbols_needing_addresses[i]].gas_value = gas_value;
                        }
                        cbl_symbols_needing_addresses.clear();
                    }
                }
            }

            if(tokens.size() EQ 2 AND tokens[1].substr(tokens[1].length()-2) EQ "_:") {
                /*

                More hacking.  Make sure every user-defined symbol (they end with '_' has
                a relative address associated with it.  Otherwise single-stepping gets confused.

                Note that this code goes right *after* code that may have cleared out cbl_lines_needing_addresses

                */
                COB_LABEL cs;
                current_line_number += 1;
                cs.line_number = current_line_number;
                cs.sentence_number = current_sentence_number;
                if(program_id_stack.size() >= 1) {
                    cs.program = program_id_stack.back();
                }
                cs.entry = current_entry;
                cs.section = current_section;
                cs.paragraph = current_paragraph;
                cs.gas_section = ".text";
                symbols.push_back(cs);
                cbl_lines_needing_addresses.push_back(symbols.size()-1);
            }
        }
    }
}


void COB_LABELS::GetProgramIdLineNumbers(const std::string &source, const std::string &scope, std::ifstream &ifs, GAS_SYMBOLS &gas_symbol_table)
{
    CPROFILER;
    ifs.clear();
    ifs.seekg(0);

    /*  It could be possible that at some point in the future, we're going to
        create a more fully-capable parser of the COBOL source code, although
        that probably, ultimately, be the province of a COBOL compiler.

        But for now, what I need is the line number of the identifier of
        the PROGRAM-ID paragraph of an IDENTIFICATION DIVISION.

        Typically,

            IDENTIFICATION DIVISION. PROGRAM-ID. {program-name}.

        is just one line of code.  But it turns out that in free-form mode,
        GnuCOBOL is perfectly happy with

        ************************************

            ID
            DIVISION
            .
            PROGRAM-ID
            .
            {program-name}
            .

        ************************************

        So, in this version of this code, I am going to be pretty brain-dead
        about the whole thing.

        */

    bool in_program_id = false;
    int line_number = 0;
    string program_name;
    char ch;
    while(ifs.get(ch)) {
        if( ch <= ' ' ) { // Cheap way of testing for space, tab, vertical tab, newline, carriage return, and form feed
            // We got whitespace.
            if(!program_name.empty()) {
                // We are at a whitespace boundary, and we have something in program_name
                if(program_name[program_name.length()-1] EQ '.') {
                    // Strip off one final period:
                    program_name = program_name.substr(0,program_name.length()-1);
                }
                // If, having stripped off that final period, we are now empty, that
                // means it was a stand-alone period, and we just don't care about it.
                if(!program_name.empty()) {
                    if(in_program_id) {
                        // We have arrived.  We have achieved our purpose in life, our *raison d'etre*, our meaning.
                        // Stash the results.  And try some more.  (So much for fulfillment.)
                        program_id_lines[program_name] = line_number;
                        in_program_id = false;
                    } else {
                        // We got something.  See if we care about it:
                        static const char PROGRAM_ID[] = "program-id";
                        if(strcasecmp(PROGRAM_ID,program_name.c_str()) EQ 0) {
                            // We have found the COBOL reserved word PROGRAM-ID.  The next thing
                            // we find will be the {program_name} we're looking for
                            in_program_id = true;
                        }
                    }
                    // No matter what happened, at this point we're done with that value
                    // of program_name:
                    program_name.clear();
                }
            }
            if(ch EQ '\n') {
                line_number += 1;
            }
        } else {
            // It isn't whitespace; just add it to program_name
            program_name += ch;
        }
    }
}

void COB_LABELS::MergeGasSymbols(const GAS_SYMBOLS &gas_symbol_table)
{
    CPROFILER;
    for(VCOB_LABEL::iterator it=symbols.begin(); it!=symbols.end(); it++) {
        // Don't mess with symbols that already have been found or defined.
        // Symbols set through linkage are an example.
        if( it->gas_symbol.empty()  ) {
            GAS_SYMBOL cob_symbol = gas_symbol_table.FindGasValue(".text",it->gas_value);
            if(!cob_symbol.gas_symbol.empty()) {
                it->gas_symbol = cob_symbol.gas_symbol;
                it->gas_section = cob_symbol.gas_section;
            }
        }
    }
}

void DATA_DESCRIPTIONS::ProcessDataStorage(ifstream &ifs,const string &source)
{
    ifs.clear();
    ifs.seekg(0);

    static const string lswitch = "/* Program local variables for ";
    static const string END_OF_FIELDS = "/* End of fields */";

    string scope = "Global"; // This is the default
    string program_id;

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
                    DATA_DESCRIPTION ds;
                    ds.source = source;
                    ds.line_number = current_line_number;
                    ds.program_id = program_id;
                    ds.storage_type = "Picture";
                    ds.storage_scope = scope;
                    ds.gas_symbol = picture_symbol;
                    ds.picture_string = picture_string;
                    symbols.push_back(ds);
                    inpic = false;
                    picture_string.clear();
                } else {
                    string ch = sm[1];
                    picture_string += ch[0];
                    int n = stoi(sm[2]);
                    if(n > 1) {
                        picture_string += '(';
                        picture_string += sm[2];
                        picture_string += ')';
                    }
                }
            }
            continue;
        }


        if(input EQ END_OF_FIELDS) {
            scope = "Global";
            program_id.clear();
            continue;
        }

        vector<string> tokens = Split(input," \t");
        if(input.substr(0,lswitch.size()) EQ lswitch) {
            scope = "Local";
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
        static const regex storage(".+(b_[0-9]+)[[]([0-9]+).*$");
        if(regex_match(input,sm,storage)) {
            if(sm.size() EQ 3) {
                DATA_DESCRIPTION ds;
                ds.source = source;
                ds.line_number = current_line_number;
                ds.program_id = program_id;
                ds.storage_type = "Storage";
                ds.storage_scope = scope;
                ds.cbl_symbol = GetCommentText(input);
                ds.gas_symbol = sm[1];
                ds.storage_size = stoll(sm[2]);
                symbols.push_back(ds);

                // It turns out that the program kinda lies when it gets
                // around to f_ field attribute variables.  In some places,
                // it defines working-storage b_ variables in one program
                // module, maybe with the GLOBAL attribute.  But later on
                // it defines the matching f_ variable in another program-id.

                // We need them in the same program module in order to be
                // able to associate them with the variable listing at the
                // end of the .cbl.lst file.

                // To facilitate that, we build a <string,string> map.
                b_symbol_to_program_id[ds.gas_symbol] = program_id;
            }
            continue;
        }

        // Look for a_xxx attributes:
        // ==>static const cob_field_attr a_3 = {0x21,   0,   0, 0x0000, NULL};<<=
        static const regex attribute(".*cob_field_attr ([^ ]+).+[{]0x([^,]+),([^,]+),([^,]+), 0x([^,]+), ([^}]+).*$");
        if(regex_match(input,sm,attribute)) {
            DATA_DESCRIPTION ds;
            ds.source = source;
            ds.line_number = current_line_number;
            ds.program_id = program_id;
            ds.storage_type = "Attribute";
            ds.storage_scope = scope;
            ds.gas_symbol = sm[1];

            // cob_field_attr is found in cobc/libcob/common.h
            ds.attr_type = static_cast<USHORT>(stoul(sm[2],NULL,16));
            ds.attr_digits = static_cast<USHORT>(stoul(sm[3]));
            ds.attr_scale = static_cast<short>(stol(sm[4]));
            ds.attr_flags = static_cast<USHORT>(stoul(sm[5],NULL,16));
            string pic_pointer = sm[6];
            if(pic_pointer EQ "NULL") {
                pic_pointer.clear();
            }
            ds.attr_pic_pointer = pic_pointer;
            symbols.push_back(ds);
            continue;
        }

        // Look for cob_field constants:
        if(input.find("cob_field c_") != string::npos) {
            // This is a cob_field constant:
            // ==>static const cob_field c_1    = {15, (cob_u8_ptr)"entering: rtest", &a_1};<==
            // cob_field is found in cobc/libcob/common.h

            smatch sm;
            static const regex cfield(".*cob_field ([^ \t]+)[^{]+[{]([^,]+).*$");
            if(regex_match(input,sm,cfield)) {
                DATA_DESCRIPTION ds;
                ds.source = source;
                ds.line_number = current_line_number;
                ds.program_id = program_id;
                ds.storage_type = "Constant";
                ds.storage_scope = scope;
                ds.gas_symbol = sm[1];
                ds.field_size = stoi(sm[2]);

                // Go after the first and last double-quotes directly, in case
                // the string itself has double-quotes in it.
                // Likewise, look for the '&' and '}' after the data, in case
                // the data contains those.
                size_t start_quote = input.find_first_of("\"");
                if(start_quote != string::npos) {
                    start_quote += 1;
                    size_t end_quote = input.find_last_of("\"");
                    if(end_quote != string::npos) {
                        ds.field_data_pointer = input.substr(start_quote,end_quote-start_quote);
                        size_t ampersand = input.find('&',end_quote);
                        if(ampersand != string::npos) {
                            ampersand += 1;
                            size_t end_brace = input.find_last_of("}");
                            if(end_brace != string::npos) {
                                ds.field_attr_pointer = input.substr(ampersand,end_brace - ampersand);
                            }
                        }
                    }
                }
                symbols.push_back(ds);
                continue;
            }
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
                    // look for b_17 + 10
                    smatch sm2;
                    static const regex offset_match("(.*) [+] (.*)");
                    string base_offset = sm[3];
                    string base = sm[3];
                    string offset;
                    if(regex_match(base_offset,sm2,offset_match)) {
                        base = sm2[1];
                        offset = sm2[2];
                    }

                    // As mentioned in the b_ processing, the program_id at
                    // this point can be a lie.  When a b_ is declared GLOBAL,
                    // its definition is in one program-id.  But if it's
                    // accessed in program-id2, that's where the .h file says
                    // the f_ variable that references b_ is.  We built a map
                    // to get around that, which we have to do to get these
                    // variables lined up with the .cbl.lst variable list.

                    string mapped_program_id = program_id;
                    unordered_map<string,string>::const_iterator it =
                        b_symbol_to_program_id.find(base);

                    if(it != b_symbol_to_program_id.end()) {
                        mapped_program_id = it->second;
                    }

                    DATA_DESCRIPTION ds;
                    ds.source = source;
                    ds.line_number = current_line_number;
                    ds.program_id = mapped_program_id;
                    ds.storage_type = "Field";
                    ds.storage_scope = scope;
                    ds.gas_symbol = sm[1];
                    ds.field_size = stoi(sm[2]);
                    if(base EQ "NULL") {
                        base.clear();
                    }
                    ds.field_data_pointer = base;
                    ds.field_data_offset = offset;
                    ds.field_attr_pointer = sm[4];
                    ds.cbl_symbol = GetCommentText(input);
                    symbols.push_back(ds);
                }
            }
            continue;
        }

        // Check for a PICTURE symbol
        if(!inpic) {
            smatch sm;
            static const regex re_picture(".*cob_pic_symbol ([^[]+).*");
            if(regex_match(input,sm,re_picture)) {
                picture_symbol = sm[1];
                inpic = true;
                continue;
            }
        }
        size_t nfound;
        if((nfound = input.find("const char st_")) != string::npos) {
            nfound += 11; // To the beginning of st_
            string sym;
            while(nfound < input.size() AND input[nfound] != '[') {
                sym += input[nfound++];
            }
            size_t n1 = input.find_first_of("\"");
            size_t n2 = input.find_last_of("\"");
            if(n1 != string::npos) {
                n1 += 1;
                string svalue = input.substr(n1,n2-n1);
                DATA_DESCRIPTION ds;
                ds.source = source;
                ds.line_number = current_line_number;
                ds.program_id = program_id;
                ds.storage_type = "String";
                ds.storage_scope = scope;
                ds.field_size = svalue.size()+1;
                ds.field_data_pointer = svalue;
                ds.gas_symbol = sym;
                symbols.push_back(ds);
            }
        }
    }
}

void DATA_DESCRIPTIONS::AppendLinkageInformation(const COB_LABELS &cob_labels)
{
    for( int i=0;  i<cob_labels.size(); i++ ) {
        COB_LABEL cl = cob_labels[i];
        if( cl.section EQ "Linkage" ) {
            DATA_DESCRIPTION dd;
            dd.program_id = cl.program;
            dd.cbl_symbol = cl.entry;
            dd.storage_type = cl.section;
            dd.storage_scope = "Local";
            dd.gas_section = cl.gas_section;
            dd.gas_symbol = cl.gas_symbol;
            dd.undecorated_symbol = cl.gas_symbol;
            this->symbols.push_back(dd);
        }
    }
}


void DATA_DESCRIPTIONS::MergeGasSymbols(const GAS_SYMBOLS &gas_symbol_table)
{
    CPROFILER;
    // First pass:  Match symbols based on gas_name
    size_t index = 0;
    for(VDATA_DESCRIPTION::iterator it=symbols.begin(); it!=symbols.end(); it++) {
        GAS_SYMBOL gas_symbol = gas_symbol_table.FindGasSymbol(it->gas_symbol);
        if(!gas_symbol.gas_symbol.empty()) {
            it->gas_symbol = gas_symbol.gas_symbol;
            it->undecorated_symbol = gas_symbol.undecorated_symbol;
            it->gas_section = gas_symbol.gas_section;
            it->gas_value = gas_symbol.gas_value;
            if(!gas_symbol.field_data_pointer.empty()) {
                it->field_data_pointer = gas_symbol.field_data_pointer;
                it->field_data_offset = gas_symbol.field_data_offset;
            }
        }

        if(!it->gas_symbol.empty()) {
            location_of_gas_symbol[it->gas_symbol] = index;
        }
        index += 1;
    }
}

void DATA_DESCRIPTIONS::ScanAllDotHFiles(const string &path,const string &fname)
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

        ProcessDataStorage(ifs,source_file);

        ifs.close();
    }
}
