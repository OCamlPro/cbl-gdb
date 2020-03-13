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
#include "vartree.h"
#include "utils.h"
#include "profiler.h"

using namespace std;

#define EQ ==
#define AND &&
#define OR ||

VAR_NODE::VAR_NODE()
{
    CPROFILER;
    size = 0;
    offset = 0;
    level = LEVEL_ROOT;
    parent = nullptr;

    field_size  = 0;
    attr_type   = 0;
    attr_digits   = 0;
    attr_scale  = 0;
    attr_flags  = 0;
}

void VAR_NODE::AddChild(VAR_NODE *new_node)
{
    CPROFILER;

    /*  Little bit of special processing in order to keep things lined up

        At the present time we are more-or-less ignoring COBOL tables, which
        are present in the .cbl.lst file like this:

        00021 GROUP          01   PERFORM7
        00003 NUMERIC        02   PERFORM8  99V9, OCCURS 7

        Note that the GROUP is properly shown as 21 bytes, which is determined
        by the following line as something three bytes long repeated 7 times.

        This won't really work for us; when we calculate running offsets
        later on, that 3 is going to hurt.  So, we will expand it at this
        time.  Downstream processing, like in gdb when asked to display
        table elements, will have to take appropriate action to find a
        particular required element of a table.

        This decision was made at this time in order to simplify and speed up
        development of code that wasn't even trying to cope with tables.

        RJD 2019-07-25

        */

    if(new_node->occurs != 0) {
        // Note that the size derived from .cbl.lst is being augmented. The
        // field_size attribute, which will be derived from the .h files,
        // and will be the size of each element of the table, is going to be
        // left alone.
        new_node->size *= new_node->occurs;
    }

    // Propogate the program/section from parent to child
    if(!this->program.empty()) {
        new_node->program = this->program;
    }
    if(!this->section.empty()) {
        new_node->section = this->section;
    }

    // And link in the new node
    new_node->parent = this;
    this->children.push_back(new_node);
}

VARIABLE_TREE::VARIABLE_TREE()
{
    root = new VAR_NODE;
}

VARIABLE_TREE::~VARIABLE_TREE()
{
    ChopDownTheTree();
}

void VARIABLE_TREE::ChopDownTheTree(VAR_NODE *node)
{
    CPROFILER;
    if(node) {
        for(VVAR_NODES::iterator child=node->children.begin();
                child!=node->children.end();
                child++) {
            ChopDownTheTree(*child);
        }
        delete node;
        node = nullptr;
    }
}

void VARIABLE_TREE::ChopDownTheTree()
{
    CPROFILER;
    ChopDownTheTree(root);
}

void VARIABLE_TREE::FindParentOf(VVAR_NODES &parents,int level)
{
    CPROFILER;
    // Eliminate parents from the back of the stack until you find one
    // that's an immediate ancestor -- not a sibling, but an ancestor --
    // to 'level'
    while(!parents.empty()) {
        if(parents.back()->level < level) {
            break;
        }
        parents.pop_back();
    }
}

void VARIABLE_TREE::ForceParent(VVAR_NODES &parents,const string &program_id)
{
    CPROFILER;
    // When there is only one PROGRAM-ID in a source module, GnuCOBOL 3.0
    // neglects to include that name in the variables section of the .cbl.lst
    // -T listing.  We call this routine to make sure there is a parent
    // parent when need one

    if(parents.empty()) {
        VAR_NODE *new_node = new VAR_NODE;
        new_node->program = program_id;
        new_node->level = LEVEL_PROGRAM;
        root->AddChild(new_node);
        parents.push_back(new_node);
    }
}

void VARIABLE_TREE::ReadFromFile(std::ifstream &ifs)
{
    CPROFILER;
    ifs.clear();
    ifs.seekg(0);

    // When a module has only one PROGRAM-ID, GnuCOBOL 3.0 neglects to include
    // that information in the variables section, dammit.  So, we have to
    // waste some cycles finding it ourselves.

    bool free_form = false ;
    string first_program_id;
    bool program_id_triggered = false;

    string input;
    VVAR_NODES parents;         // This is a stack for keeping track
    // of parent nodes
    bool processing = false;
    for(;;) {
        getline(ifs,input);
        if(ifs.eof()) {
            break;
        }

        if(!processing) {
            if(input.size() > 4 AND input.substr(0,4) EQ "SIZE") {
                // We have reached the beginning of the parsed variables
                processing = true;
                continue;
            }
            if(input.size() > 4 AND input.substr(0,4) EQ "LINE") {
                vector<string>tokens = Split(input," \t");
                free_form = tokens.size() EQ 2 ;
                continue;
            }

            if(first_program_id.empty()) {
                // Here's where we look for "PROGRAM-ID. <program-id>."
                // Ain't simple.  We need to deal with fixed and free input,
                // and we need to strip out comments, and we need to ignore
                // anything in between 'quoted text' and "quoted text".

                // Only after those are gone can we look for paydirt.

                // Most of the following is pure magic, and is based on the
                // actual structure of the files.  Just live with it.
                if(free_form AND input.length() >= 8) {
                    // Trim away the leading "dddddd  "
                    input = input.substr(8);
                }
                if(!free_form AND input.length() > 80) {
                    // Trim away the SEQUENCE information at column 80
                    input = input.substr(0,80);
                }
                if(!free_form AND input.length() > 9) {
                    // Trim away the leading "ddddd  "
                    input = input.substr(9);
                    // We should be left with a standard COBOL 72-line input
                    // Wipe out the first six line number characters
                    input[0] = ' ';
                    input[1] = ' ';
                    input[2] = ' ';
                    input[3] = ' ';
                    input[4] = ' ';
                    input[5] = ' ';
                }

                // We now need to eliminate anything inside of paired single-
                // or double-quotes, because we don't want to be confused by
                // any crap some evil programmer put in there.  Probably in
                // an attempt to screw me up.
                bool dequoting = true;
                while(dequoting) {
                    dequoting = false;
                    size_t nfound;

                    // Find and eliminate consecutive double-quotes
                    nfound = input.find("\"\"");
                    if(nfound != string::npos) {
                        dequoting = true;
                        input[nfound] = ' ';
                        input[nfound+1] = ' ';
                        continue;
                    }

                    // and consecutive single-quotes
                    nfound = input.find("''");
                    if(nfound != string::npos) {
                        dequoting = true;
                        input[nfound] = ' ';
                        input[nfound+1] = ' ';
                        continue;
                    }

                    nfound = input.find_first_of("\"'");
                    if(nfound != string::npos) {
                        // Eliminate everything between the quote and its
                        // mate:
                        char endquote = input[nfound];
                        dequoting = true;
                        size_t i = nfound;
                        while(input[i] != endquote) {
                            input[i++] = ' ';
                            if(i>=input.length()) {
                                break;
                            }
                        }
                        if(i<input.length()) {
                            input[i++] = ' ';
                        }

                        continue;
                    }
                }
                //  And now, we have to consider comments:
                if(free_form) {
                    size_t nfound;

                    // Strip off leading spaces
                    nfound = input.find_first_not_of(" ");
                    if(nfound != string::npos) {
                        input = input.substr(nfound);
                    }

                    // Find the *> and >>D comment types:
                    nfound = input.find("*>");
                    if(nfound != string::npos) {
                        input = input.substr(0,nfound);
                    }
                    nfound = input.find(">>D");
                    if(nfound != string::npos) {
                        input = input.substr(0,nfound);
                    }
                } else {
                    if(     input.length() >= 7
                            AND (
                                input[7-1] EQ '*'
                                OR
                                input[7-1] EQ '/'
                                OR
                                input[7-1] EQ 'D'
                            )   ) {
                        // Ignore comment lines
                        input.clear();
                    }
                    size_t nfound = input.find("*>");
                    if(nfound != string::npos) {
                        input = input.substr(0,nfound);
                    }
                }

                // We now have input free of comments and text literals,
                // so we can split what's left and look for gold:
                vector<string>tokens = Split(input," \t");
                size_t ntoken = 0;
                if(!program_id_triggered) {
                    for(ntoken=0; ntoken<tokens.size(); ntoken++) {
                        string token = tokens[ntoken];
                        string lc;
                        for(char s : token) {
                            lc += tolower(s);
                        }
                        if(lc EQ "program-id" OR lc EQ "program-id.") {
                            // Given our comment processing and the elimination
                            // of text strings, the next token we see will
                            // be the value we need
                            ntoken += 1;
                            program_id_triggered = true;
                            break;
                        }
                    }
                }
                if(program_id_triggered) {
                    if(ntoken < tokens.size()) {
                        first_program_id = tokens[ntoken];
                        size_t nfound = first_program_id.find(".");
                        if(nfound != string::npos) {
                            first_program_id =
                                first_program_id.substr(0,nfound);
                        }
                    }
                }

            }

            continue;
        }

        vector<string>tokens = Split(input," \t");
        if(tokens.size() EQ 0) {
            // Ignore blank lines
            continue;
        }
        if(tokens.size() EQ 2 AND tokens[0] EQ "PROGRAM") {
            // We are starting a brand new PROGRAM section, which has the
            // root as a parent:

            parents.clear();
            VAR_NODE *new_node = new VAR_NODE;
            new_node->program = tokens[1];
            new_node->level = LEVEL_PROGRAM;
            root->AddChild(new_node);
            parents.push_back(new_node);
            continue;
        }

        if(tokens.size() EQ 2 AND tokens[1] EQ "SECTION") {
            // Sections get their own node, and they have to have the
            // level-00 program-id node as a parent:
            ForceParent(parents,first_program_id);
            FindParentOf(parents,LEVEL_SECTION);
            VAR_NODE *new_node = new VAR_NODE;
            new_node->section = tokens[0];
            new_node->level = LEVEL_SECTION;
            parents.back()->AddChild(new_node);
            parents.push_back(new_node);
            continue;
        }

        VAR_NODE *new_node = new VAR_NODE;

        // We also need some special processing for ','.  Commas can appear
        // In PICTURE strings, so we need to keep them there.  But if a ','
        // appears at the end of the token before a REDEFINES token, we have to
        // remove it.

        vector<string>::const_iterator it = find(tokens.begin(),
                                            tokens.end(),
                                            "REDEFINES");
        if( it != tokens.end() ) {
            // 'it' points to REDEFINES:
            auto index = it - tokens.begin();

            // Get rid of the comma at the end of index-1:
            size_t n = tokens[index-1].find_last_of(',');
            if( n != string::npos ) {
                tokens[index-1] = tokens[index-1].substr(0,n);
            }

            // Pick up the name of the referenced variable:
            new_node->redefines = tokens[index+1];

            // And now remove index, index-1 from the tokens:
            vector<string>::iterator it1 = tokens.begin() + index;
            vector<string>::iterator it2 = tokens.begin() + index + 2;
            tokens.erase(it1,it2);

            // And now we can proceed
        }

        it = find(tokens.begin(),tokens.end(),"OCCURS");
        if( it != tokens.end() ) {
            // 'it' points to OCCURS:
            auto index = it - tokens.begin();

            // Get rid of the comma at the end of index-1:
            size_t n = tokens[index-1].find_last_of(',');
            if( n != string::npos ) {
                tokens[index-1] = tokens[index-1].substr(0,n);
            }

            // Pick up the name of the referenced variable:
            new_node->occurs = stoi(tokens[index+1]);

            // And now remove index, index-1 from the tokens:
            vector<string>::iterator it1 = tokens.begin() + index;
            vector<string>::iterator it2 = tokens.begin() + index + 2;
            tokens.erase(it1,it2);

            // And now we can proceed
        }

        if(tokens.size() EQ 3 AND tokens[1] EQ "FILE") {
            // Special case:  FILE records have a size and a name, but
            // no level.  So, we give it the special level of 00, and treat
            // the whole section with its unique needs.  In particular,
            // the LEVEL 00 name is the parent to all the level 01 entries
            // that follow, and the second 01 entry on implicitly redefines
            // the first 01 entry, as per the COBOL spec.

            // Make sure we actually have a parent program node:
            ForceParent(parents,first_program_id);

            // Back down to the program node
            FindParentOf(parents,LEVEL_PROGRAM+1);   // Back down to the PROGRAM node

            // Create the LEVEL_SECTION entry the FILE record needs
            new_node->size = 0;
            new_node->type = "";
            new_node->level = LEVEL_SECTION;
            new_node->section = SECTION_INPUT_OUTPUT;
            new_node->name = "";
            parents.back()->AddChild(new_node);
            parents.push_back(new_node);

            new_node = new VAR_NODE;

            // And having forced that in, create the level 00 entry for
            // the FILE record itself:
            new_node->size = stoi(tokens[0]);
            new_node->type = tokens[1];         // This is, of course, "FILE"
            new_node->level = LEVEL_FILE;       // Should be 00; special handling
            new_node->section = "";
            new_node->name = tokens[2];
            parents.back()->AddChild(new_node);
            parents.push_back(new_node);

            continue;
        } else if(tokens.size() EQ 4) {
            // Usually a GROUP record.  As of this writing, it can also
            // be a FLOAT-DECIMAL-16 or -34, because those don't have
            // PICTURE field descriptions, although they ought to.
            new_node->size = stoi(tokens[0]);
            new_node->type = tokens[1];
            new_node->level = stoi(tokens[2]);
            new_node->name = tokens[3];
        } else if( tokens.size() EQ 5 AND tokens[1] EQ "warnings" ) {
            // We've reached the end of the parsed tokens with a line:
            // "0 warnings in compilation group"
            delete new_node;
            break;
        } else if( tokens.size() EQ 2 AND tokens[0] EQ "Error/Warning" ) {
            // We've reached the end of the parsed tokens with a line:
            // "0 warnings in compilation group"
            delete new_node;
            break;
        } else if(tokens.size() EQ 5 OR tokens.size() EQ 6) {
            if(tokens.size() EQ 6) {
                // If size is 6, then there is a PIC field followed by a
                // variable type.  Combine them:
                tokens[5-1] += ' ';
                tokens[5-1] += tokens[6-1];
                // And get rid of the position 6
                tokens.pop_back();
            }
            // There are now five elements:

            new_node->size = stoi(tokens[0]);
            new_node->type = tokens[1];
            new_node->level = stoi(tokens[2]);
            new_node->name = tokens[3];
            vector<string>usage = Split(tokens[4]," ");
            if(usage.size() >= 1) {
                new_node->picture = usage[0];
            }
            if(usage.size() >= 2) {
                new_node->usage = usage[1];
            }
        } else if( tokens.size() EQ 3 AND tokens[2] EQ "defined.") {
            // Just ignore "No fields defined."
            delete new_node;
            continue;
        } else if(tokens.size() EQ 3
                  AND (       tokens[1] EQ "88"
                              OR  tokens[2] EQ "66"
                              OR  tokens[3] EQ "78")
                 ) {
            // Just ignore types we can't handle
            delete new_node;
            continue;
        } else {
            cerr << "VARIABLE_TREE::ReadFromFile() doesn't know how to handle";
            cerr << endl;
            cerr << "\"" << input << "\"" << endl;
            delete new_node;
            exit(1);
        }

        if(!new_node->name.empty()) {

            // Special processing for the oddball LEVEL 77 variable type.
            // Level 77 variables have to be atomic, which we don't care
            // about here.  But they also are children of the section, rather
            // than of a lower-leveled variable:
            if( new_node->level EQ LEVEL_77) {
                // Make sure that parent is the section:
                FindParentOf(parents,LEVEL_SECTION+1);
            } else {
                // Make sure that parent is an *immediate* ancestor
                FindParentOf(parents,new_node->level);
            }

            // Add us as a child of that parent
            ForceParent(parents,first_program_id);
            parents.back()->AddChild(new_node);

            // And perhaps we will become a parent
            parents.push_back(new_node);
        } else {
            cerr << "new_node->name is empty in VARIABLE_TREE::ReadFromFile()";
            cerr << endl;
            cerr << "\"" << input << "\"" << endl;
            delete new_node;
            exit(1);
        }
    }
}

void VARIABLE_TREE::DumpNode(ostream &sout, const VAR_NODE *node)
{
    if(node !=root) {
        if(     node->program != current_program_in_dump
                OR  node->section != current_section_in_dump) {
            current_program_in_dump = node->program;
            current_section_in_dump = node->section;

            if(node->section.empty()) {
                sout << endl;
                sout << "PROGRAM-ID";
                sout << " : ";
                sout << node->program;
                sout << endl;
            } else {
                sout << endl;
                sout << node->program;
                sout << " : ";
                sout << node->section;
                sout << endl;
            }
        } else if(node->level >= LEVEL_FILE) // Without this, the program tries
            // to dump LEVEL_SECTION.  It's ugly, so we stop it.
        {
            if(     node->level EQ LEVEL_01
                    OR  node->level EQ LEVEL_FILE    // Special processing for FILE record
                    OR  node->level EQ LEVEL_77 AND current_level_in_dump != 77 ) {
                sout << endl;
            }
            sout.width(5);
            sout.fill('0');
            sout << right << node->size;

            sout << " ";
            sout.width(15);
            sout.fill(' ');
            sout << left << node->type;

            sout << " ";
            sout.width(2);
            sout.fill('0');
            sout << right << node->level;
            current_level_in_dump = node->level;

            sout << " ";
            sout.width(30);
            sout.fill(' ');
            sout << left << node->name;

            if(!node->picture.empty()) {
                sout << node->picture;
                sout << " ";
            }
            if(!node->usage.empty()) {
                sout << node->usage;
                sout << " ";
            }

            if(!node->redefines.empty()) {
                sout << "REDEFINES ";
                sout << node->redefines;
                sout << " ";
            }

            if(!node->occurs != 0) {
                sout << "OCCURS ";
                sout << node->occurs;
                sout << " ";
            }

            sout << "[";
            sout << node->canonical_name;
            if( !node->base_symbol.empty() OR node->offset > 0 ) {
                sout << " ";
            }
            sout << node->base_symbol;
            if(node->offset > 0) {
                sout << "+";
                sout << node->offset;
            }
            sout << ':';
            sout << node->field_symbol;
            sout << ':';
            sout << node->attr_symbol;
            sout << "] ";

            sout << endl;
        }
    }
    for( VVAR_NODES::const_iterator it=node->children.begin();
            it!=node->children.end();
            it++) {
        DumpNode(sout,*it);
    }
}

void VARIABLE_TREE::Dump(ostream &sout)
{
    // Primarily for debugging.
    // Dump the whole tree, depth first:
    current_program_in_dump.clear();
    current_section_in_dump.clear();
    DumpNode(sout, root);
}

static VAR_NODE *FindNameIn(VAR_NODE *node,const string &name)
{
    if( node->GetLevel() != LEVEL_66 AND node->GetName() EQ name ) {
        return node;
    }
    VAR_NODE *retval = NULL;
    for( size_t i=0; i<node->GetChildren().size(); i++ ) {
        retval = FindNameIn(node->GetChildren()[i],name);
        if( retval ) {
            break;
        }
    }
    return retval;
}

void VARIABLE_TREE::BuildCanonicalName(VAR_NODE *var_node)
{
    /*  We are going to take our variable tree, with its basic structure

            PROGRAM
            SECTION   [of PROGRAM];
            01 VAR01  [of SECTION of PROGRAM]
            02 VAR02A [of VAR01 of SECTION of PROGRAM]
            02 VAR02B [of VAR01 of SECTION of PROGRAM]

        and use it to build up the canonical COBOL identifier of
            VAR02A of VAR01 of SECTION of PROGRAM
            VAR02B of VAR01 of SECTION of PROGRAM

        But!  Before we walk the tree, we need to calculate the offset of
        this var_node.

        var_node identifies a COBOL identifier.  It could be a level 00 FILE
        description; it could be a level 77 atomic; it could be an
        01-48 GROUP, or it could be an 01-49 atomic.

        Each node contains a size attribute, which is the size of itself in
        bytes.  (For a GROUP or FILE node, it's the collected size of all
        of its descendents.)

        At this point in the depth-first search, we can calculate the offset
        to the beginning of this node's data.  There are various possibilities:

        1)  If we are a REDEFINES record, then we have to have an older
            brother and our offset is his offset.

        2)  If our parent is a LEVEL_FILE record, and we have an older
            brother, then we implicitly redefine that older brother, which
            means that his offset is our offset

        3)  If we are the first-born, then our offset is our parent's
            offset.

        4)  If our parent is LEVEL_SECTION, then we are a first-level (either
            77 or what's usually level 01) variable, and our offset is our
            parent's offset: zero. This is because first-level variables all
            have b_? symbols that we ultimately want to be referencing.

        5)  Otherwise, the logic chain means that our parent is another
            variable, and we have an older brother.  Our offset is
            his size added to his offset.

        6)  All bets are off if the selection we are processing is
            LOCAL-STORAGE.  In that case, everything is relative to a malloced
            memory block at cob_local_data.  That means even when our parent
            is LEVEL_SECTION, we don't reset our offset to zero, and even
            level 01 variables are older brothers to the next level 01
            variable.

        Grab your abacus; we're gonna do some calculating:
        */

#if 0
    cout << var_node->program << " " ;
    cout << var_node->section << " " ;
    cout << var_node->type << " " ;
    cout << var_node->level << " " ;
    cout << var_node->name << " " ;
    if( !var_node->redefines.empty() ) {
        cout << "REDEFINES:" << var_node->redefines << " " ;
    }
    cout << endl;
#endif

    if(var_node->level >= LEVEL_FILE) {
        VAR_NODE *parent = var_node->parent;
        int brother_index = -1;

        // Check all of dad's children
        for(int i=1; i<(int)parent->children.size(); i++) {
            // When we find ourself, then we've also found our immediately
            // older sibling:
            if(parent->children[i] EQ var_node) {
                brother_index = i-1 ;
                break;
            }
        }

        if( var_node->level EQ LEVEL_66 ) {
            // A level 66 variable is like an 01 or 77: it's a top-level
            // variable that comes right off of the root.  It points, however,
            // to a previously-defined memory location within the same
            // level 01 object that we are in.
            //
            // We need to find that memory location.  It might be one of
            // our parent's siblings.
            //
            /* We have a theoretical problem.  Consider this code:

            01 TESTER.
              02 TESTER-02.
                10 TEST-1.
                   20 BOB  PIC 9(9).
                10 TEST-2.
                   20 BOB  PIC 9(9).
                      66 BILL RENAMES BOB OF TEST-2.

            The .cbl.lst output for the case of "OF TEST-2" and "OF-TEST1"
            is the same in both cases:

            00018 GROUP          01   TESTER
            00018 GROUP          02   TESTER-02
            00009 GROUP          10   TEST-1
            00009 NUMERIC        20   BOB                            9(9)
            00009 GROUP          10   TEST-2
            00009 NUMERIC        20   BOB                            9(9)
            00009 ALPHANUMERIC   66   BILL                           9(9), REDEFINES BOB

            From a theoretical standpoint, one could argue that the COBC output
            should be refined.  Another argument is that COBST should parse
            the original code, rather than the COBC output.

            From a practical standpoint, when I first encountered this and discussed
            it with Marty, he had this to say:

            “The RENAMES [level 66] clause is rarely used and it is not covered in any detail in this book.”
            (SAMS Teach Yourself COBOL.) THE ONLY REFERENCE in 3 books and the IBM COBOL F (1960s) manual.

            So, I am just going to look through our ancestors (in reverse order on
            each level) and snag the first one with a matching name.

            */

            // back up to level 1 :
            VAR_NODE *grampa = parent->parent;
            while( grampa->level > LEVEL_01 ) {
                grampa = grampa->parent;
            }

            VAR_NODE *source = FindNameIn(grampa,var_node->redefines);
            if( !source ) {
                cerr << "HULL BREACH in cobst! A RENAMES variable doesn't seem to have an ancestor!\n";
                cerr << "This is in VARIABLE_TREE::BuildCanonicalName()\n";
                exit(1);
            }

            // We have now found the source of our RENAME:
            var_node->offset = source->offset;

            // Let the source know they will have to update us later, once
            // the base_names start getting assigned:
            source->renamed_by.push_back(var_node);

            // This is a level 66 variable, so it needs to be moved up to
            // the same level that grampa is on.

            var_node->parent = grampa->parent;
            grampa->parent->children.push_back(var_node);

            // We need to eliminate ourself one from our parent's level.
            // We will find ourself, and just null out the pointer:

            for(size_t i=0; i<var_node->parent->children.size(); i++) {
                if( var_node->parent->children[i] EQ var_node ) {
                    var_node->parent->children[i] = NULL;
                }
            }
        } else if( !var_node->redefines.empty() ) {
            // This is case 1:  We are a REDEFINES record.
            if( brother_index EQ -1) {
                cerr << "HULL BREACH in cobst! A REDEFINES variable has no older brother!\n";
                cerr << "This is in VARIABLE_TREE::BuildCanonicalName()\n";
                exit(1);
            }
            // And our brother's offset is our offset
            var_node->offset = parent->children[brother_index]->offset;

            // We want to replace ->redefines with the canonical identifier
            // of the redefined variable.  We will try our brother's
            // redefined first, which will handle a chained redefined
            if(!parent->children[brother_index]->redefines.empty()) {
                var_node->redefines = parent->children[brother_index]->redefines;
            } else {
                var_node->redefines = parent->children[brother_index]->canonical_name;
            }
        } else if( parent->level EQ LEVEL_FILE and brother_index != -1 ) {
            // This is case 2: an implicit redefines in the
            // INPUT-OUTPUT section
            var_node->offset = parent->children[brother_index]->offset;
            var_node->redefines = parent->children[brother_index]->canonical_name;
        } else if( brother_index EQ -1 ) {
            // This is case 3.  Not a REDEFINES, and we have no older sibling.
            // Our offset thus has to be the same as our parent's offset:
            var_node->offset = parent->offset;
        } else if( parent->level EQ LEVEL_SECTION
                   AND var_node->section != "LOCAL-STORAGE" ) {
            // This is case 4:  We are a level 77 or level 01 identifier,
            // and so our offset should be zero.
            if( parent->offset != 0) {
                cerr << "HULL BREACH in cobst! A LEVEL_SECTION parent has a non-zero\n";
                cerr << "offset in VARIABLE_TREE::BuildCanonicalName\n";
                exit(1);
            }
            var_node->offset = parent->offset;
        } else {
            // This is case 5 or 6:  "All other cases".
            const int BOUNDARY = 16;

            int size = parent->children[brother_index]->size;

            // Special processing:  LOCAL-STORAGE is implemented through
            // a block of memory allocated with calloc().  The c-symbolic
            // name of that block of memory is cob_local_ptr.

            // It is observed from the .lst file that Level 01 and 77 variables
            // inside the cob_local_ptr block are allocated on 16-byte
            // boundaries
            if( parent->level EQ LEVEL_SECTION
                    AND var_node->section EQ "LOCAL-STORAGE" ) {
                // Make sure that size is a multiple of BOUNDARY:
                size += (BOUNDARY-1);
                size &= ~(BOUNDARY-1);
            }

            var_node->offset = parent->children[brother_index]->offset
                               + size;
        }
    }

    if( var_node->level != LEVEL_66 ) {
        // Now it's time to build the canonical name
        // Walk down to the bottom of the variable nodes:
        for(VVAR_NODES::const_iterator child=var_node->children.begin();
                child!=var_node->children.end();
                child++) {
            if( *child != NULL ) {
                BuildCanonicalName(*child);
            }
        }
    }

    //  Look back up through our ancestors, building up the canonical
    //  COBOL name:

    const VAR_NODE *walker = var_node;
    var_node->canonical_name = "";
    if(walker->level >= LEVEL_SECTION) {
        // Only start processing if we've above the program level.
        for(;;) {
            bool add_IN = false;
            if( walker->level > LEVEL_SECTION ) {
                var_node->canonical_name += walker->name;
                add_IN = true;
            }

            if( walker->level EQ LEVEL_PROGRAM ) {
                var_node->canonical_name += walker->program;
                //  We just added the PROGRAM name, so we're done walking back
                //  this node:
                break;
            }

            // There is more to come
            if(add_IN) {
                var_node->canonical_name += "/";
            }
            walker = walker->parent;
        }
    }
}

void VARIABLE_TREE::BuildCanonicalName()
{
    BuildCanonicalName(root);
}

void VARIABLE_TREE::AssignBaseSymbol(VAR_NODE *node,const MPSS &symmap)
{
    // Walk the whole trie, seeing if node can be modified
    // with the information in symmap

    if( node EQ NULL ) {
        // This one must've been moved by a RENAMES clause
        return;
    }

    PSS prog_var_pair;
    string name_to_try = node->redefines;
    if(name_to_try.empty()) {
        name_to_try = node->canonical_name;
    }

    if(node->base_symbol.empty()) {
        // We do this only once:
        vector<string>names = Split(name_to_try,"/");
        if(names.size() >= 2) {
            // We have at least a root/program at the end of the list
            // Pick up program
            prog_var_pair.first = names.back();
            names.pop_back();
            // Pick up the root name
            prog_var_pair.second = names.back();

            // See if we've got this in our map
            MPSS::const_iterator it = symmap.find(prog_var_pair);
            if(it != symmap.end()) {
                // If so, assign the b_symbol
                node->base_symbol = it->second;

                // If anybody referenced us in a 66/RENAMES variable, now is
                // when we have to apply the base_symbol to that node, as
                // well as ourselves:
                for( size_t i=0; i<node->renamed_by.size(); i++) {
                    node->renamed_by[i]->base_symbol = it->second;
                }
            }
        }
    }

    for(VVAR_NODES::const_iterator child=node->children.begin();
            child!=node->children.end();
            child++) {
        AssignBaseSymbol(*child,symmap);
    }
}

void VARIABLE_TREE::AssignBaseSymbol(VAR_NODE *node,
                                     const MPSSI &symmap,
                                     const DATA_DESCRIPTIONS &data_description)
{
    cout << node->canonical_name << endl;



    PSS prog_var_pair;
    string name_to_try = node->redefines;
    if(name_to_try.empty()) {
        name_to_try = node->canonical_name;
    }

    vector<string>names = Split(name_to_try,"/");
    if(names.size() >= 2) {
        // We have at least a root/program at the end of the list
        // Pick up program
        prog_var_pair.first = names.back();
        names.pop_back();
        // Pick up the root name
        prog_var_pair.second = names[0];

        // See if we've got this in our map
        MPSSI::const_iterator it = symmap.find(prog_var_pair);
        if(it != symmap.end()) {
            // If so, assign the b_symbol
            int dd_index = it->second;

            node->base_symbol = data_description[dd_index].field_data_pointer;
            node->attr_symbol = data_description[dd_index].field_attr_pointer;

            if( node->base_symbol.empty() ) {
                // This must be a "Storage" record with no matching "Field"
                node->base_symbol = data_description[dd_index].undecorated_symbol;
            }
        }
    }
    for(VVAR_NODES::const_iterator child=node->children.begin();
            child!=node->children.end();
            child++) {
        AssignBaseSymbol(*child,symmap,data_description);
    }
}

void VARIABLE_TREE::BuildBaseOffsetPointers(VAR_NODE *node,VBOP &vbop)
{
    if( node EQ NULL ) {
        // This one must've been moved by a RENAMES clause
        return;
    }
    BOP base_offset_pointer;

    if(node->canonical_name.find("/") != string::npos) {
        base_offset_pointer.base_symbol = node->base_symbol;
        base_offset_pointer.offset      = node->offset;
        base_offset_pointer.pvar_node   = node;
        vbop.push_back(base_offset_pointer);
    }

    for(VVAR_NODES::const_iterator child=node->children.begin();
            child!=node->children.end();
            child++) {
        BuildBaseOffsetPointers(*child,vbop);
    }
}

void VARIABLE_TREE::BuildFlatList(VAR_NODE *node,VVAR_NODES &flat)
{
    if( node EQ NULL ) {
        // This one must've been moved by a RENAMES clause
        return;
    }
    if(node->level >= LEVEL_FILE) {
        flat.push_back(node);
    }

    for(VVAR_NODES::const_iterator child=node->children.begin();
            child!=node->children.end();
            child++) {
        BuildFlatList(*child,flat);
    }
}

void VARIABLE_TREE::MergeWorkingStorage(const DATA_DESCRIPTIONS &data_description)
{
    /*  Each WORKING-STORAGE root variable, typically with levels 01 or 77,
    and more generally variables who have no parents although 01 is
    generally used, there's actually no requirement that the level be 01),
    has a b_ symbol.

    */

    // Build a list of program/root pairs and their b_ symbols:

    MPSS prog_var_b_symbol;

    for(int i=0; i<data_description.size(); i++) {
        DATA_DESCRIPTION dd = data_description[i];
        if( dd.storage_type EQ "Storage" OR dd.storage_type EQ "Linkage") {
            PSS prog_var;
            prog_var.first  = dd.program_id;
            prog_var.second = dd.cbl_symbol;

            //  Special handling.  File records are of the form
            //  "PRINT-FILE Record".  We have to strip out that egregious
            //  extraneous information:
            size_t nfound = prog_var.second.find(" Record");
            if(nfound != string::npos) {
                prog_var.second = prog_var.second.substr(0,nfound);
            }

            prog_var_b_symbol[prog_var] = dd.undecorated_symbol;
        }
    }

    // Hand that map to a depth-first search that assigns the undecorated
    // base_address symbol to the program/root portion of each identifier.
    AssignBaseSymbol(root,prog_var_b_symbol);

    // Given that we now have a base_symbol to go along with the offset
    // to every canonical identifier, we can now look through the data
    // descriptions for matching base/offset pairs and assign the associated
    // field_symbol to each canonical identifier.

    // The fillip is handling redefines.  The data_descriptions can have
    // multiple identifier sub-pieces that point to the same base+offset,
    // but data_descriptions has no information about the variable level.
    //
    // It turns out we can figure this all out based on the ordering.
    // Although the data_descriptions and our variable_tree were built from
    // separate sources, the order in which the variables were presented
    // to each source was the same.  So, we can actually assign field/attribute
    // symbols to our canonical names on that basis.

    // Build a flat vector of <base/offset , VAR_NODE *> pairs:
    BuildBaseOffsetPointers(root,vbop);

    // With the base/offsets assigned, we can now look to assigning
    // field/attribute symbols to our variable_tree.  The variable_tree has
    // entries for every cobol identifier, since it was built from an
    // listing file close to the source code.  We don't necessarily have
    // a field/attribute entry for every identifier, since that listing
    // was created further downstream, and it appears that GnuCOBOL doesn't
    // create entries for variables that aren't referenced by the source
    // code.

    // The monkey wrench in the works:  Redefines.  Multiple names can
    // use the same base/offset pairing.  Hence this refinement:

    // Using a key of base/offset/name, we'll build a map pointing into
    // the data_description entries.  We'll then walk through vbop, and use
    // that map to make the assignments.

    map<string,int> field_attribute_lookup;

    for(int i=0; i<data_description.size(); i++) {
        if(data_description[i].storage_type EQ "Field") {
            stringstream ss;
            ss << data_description[i].field_data_pointer;
            ss << "/";
            if( data_description[i].field_data_offset.empty() ) {
                ss << "0";
            } else {
                ss << data_description[i].field_data_offset;
            }
            ss << "/";
            ss << data_description[i].cbl_symbol;
            field_attribute_lookup[ss.str()] = i;
        }
    }

    // We can now walk vbop, looking for those base/offset/name entries.

    // When we find them, we copy over the field/attribute information from
    // data_description to the node

    for(size_t i=0; i<vbop.size(); i++) {
        stringstream ss;
        ss << vbop[i].base_symbol;
        ss << "/";
        ss << vbop[i].offset;
        ss << "/";
        ss << vbop[i].pvar_node->name;
        map<string,int>::const_iterator it
            = field_attribute_lookup.find(ss.str());
        if(it != field_attribute_lookup.end()) {
            int j = it->second;
            vbop[i].pvar_node->field_symbol
                = data_description[j].undecorated_symbol;
            vbop[i].pvar_node->attr_symbol
                = data_description[j].field_attr_pointer;

            vbop[i].pvar_node->field_size  = data_description[j].field_size;

            string attr_symbol = vbop[i].pvar_node->attr_symbol;
            if( !attr_symbol.empty() ) {
                // Look up the attribute symbol in data_description:
                int k = data_description.IndexOfSymbol(attr_symbol);
                if(k > -1) {
                    vbop[i].pvar_node->attr_type   = data_description[k].attr_type;
                    vbop[i].pvar_node->attr_digits = data_description[k].attr_digits;
                    vbop[i].pvar_node->attr_scale  = data_description[k].attr_scale;
                    vbop[i].pvar_node->attr_flags  = data_description[k].attr_flags;
                }
            }
        }
    }
}

VAR_NODE *DiveForVariable(        VAR_NODE *node,
                                  const string &section,
                                  const string &program_id,
                                  const string &cbl_symbol,
                                  const int offset)
{
    VAR_NODE *retval = nullptr;

    if(     node->GetSection()  EQ section
            AND node->GetProgram()  EQ program_id
            AND node->GetName()     EQ cbl_symbol
            AND node->GetOffset()   EQ offset ) {
        // Son of a gun!  We found it!
        return node;
    }
    VVAR_NODES children = node->GetChildren();
    for(VAR_NODE *child : children) {
        VAR_NODE *maybe = DiveForVariable(child,
                                          section,
                                          program_id,
                                          cbl_symbol,
                                          offset);
        if(maybe != nullptr) {
            return maybe;
        }
    }
    return retval;
}


void VARIABLE_TREE::MergeLocalStorage(const DATA_DESCRIPTIONS &data_description)
{

}

void VARIABLE_TREE::Merge(const DATA_DESCRIPTIONS &data_description)
{
    CPROFILER;
    /*  We are now going to play a nice game of Symbol Table Sudoku

    On what I conceptualize as the left side is the trie we built up from
    the COBOL identifiers list at the end of .cbl.lst

    The characteristic of that trie is that it has the canonical (fully-
    qualified) A/B/C/program-id identifiers, including the size of each
    variable and the level of each variable, and which of the INPUT-OUTPUT,
    WORKING-STORAGE, LOCAL-STORAGE, and LINKAGE sections it came from.

    What it does *not* have is any clue of where in memory it can be found.

    Because the canonical naming is heirarchical, and because we are given
    the length of each named variable in the heirarchy, we are able to
    calculate the offset to each piece of the A/B/C/program-id heirarchy.

    On the right side of my visualization, we have the data_description
    entries.  Those have two pieces of the variable name: a name fragment,
    and the program it came from.  It does not have the whole canonical
    name, and it doesn't tell us the level.

    Here are the rules of the game:

    1)  Every WORKING-STORAGE level-01 identifier has a storage_type =
    "Storage" record with the b_ identifier in the undecorated_name
    field.

    2)  Every variable that is referenced in the program has a storage_type
    = "Field" entry.  That entry has the f_ identifier in the
    undecorated_name field.  It has the number of bytes in the
    field_size filed.  The base address is found in field_data_pointer,
    the attribute address is in the field_attr_pointer, and the
    offset from the base address is found in field_data_offset

    3)  LOCAL-STORAGE variables do not have a b_ symbol for the base
    address.  Those, instead, have 'cob_local_ptr' in the
    field_data_pointer field of a "Field" entry.

    4)  LINKAGE storage does have b_ identifiers in field_data_pointer
    field.  They also have undecorated_symbol entries ultimately created
    by information found in cob_check_linkage() calls found int the .c code
    commented into the .lst file.  So the association there is similar to the
    one for WORKING-STORAGE

    For each entry on the left side (the trie starts at 'root') we
    will work to find the matching entries on the right (data_description)
    side, and copy information from the right to the left.

    We are, actually, able to figure out a lot of things.  Where we can't
    manage is when variables are created that are never used.  If they
    aren't used, no "Field" entries are created.  If none of a heirarchy
    is accessed, then the best we can do is figure out where it is; we
    can't get many of the attributes (which are specified in the Field
    entry.)

    Let's have at it, shall we?

    */

    MergeWorkingStorage(data_description);
    MergeLocalStorage(data_description);


    // With that all done, build a flat list of VAR_NODE pointers
    BuildFlatList(root,flat_list);
}


