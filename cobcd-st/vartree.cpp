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
    occurs = 0;
    parent = nullptr;
    data_description_index = NO_INDEX;

    best_size = 0;
    attr_type = 0;
    attr_digits = 0;
    attr_scale = 0;
    attr_flags = 0;
}

void
VAR_NODE::AddChild(VAR_NODE *new_node)
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

void
VARIABLE_TREE::ChopDownTheTree(VAR_NODE *node)
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

void
VARIABLE_TREE::ChopDownTheTree()
{
    CPROFILER;
    ChopDownTheTree(root);
}

void
VARIABLE_TREE::FindParentOf(VVAR_NODES &parents,int level)
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

void
VARIABLE_TREE::ForceParent(VVAR_NODES &parents,const string &program_id)
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

void
VARIABLE_TREE::ReadFromFile(std::ifstream &ifs)
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

        // At some point GLOBAL was added, which mucks up my heuristics which
        // were based on a non-GLOBAL universe.  Just hack it off.

        // It would make more sense to search for all of the USAGE possibilities
        // and process things that way.  But I'm going for quick and dirty in the
        // hope and anticipation that the compiler will eventually generate all the
        // data we need in a formal way.  RJD, just before version cobc 3.1 was
        // released, 2020-06-26

        if( tokens.back() EQ "GLOBAL" ) {
            tokens.pop_back();
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
            size_t index = it - tokens.begin();

            // Get rid of any comma at the end of index-1:
            size_t n = tokens[index-1].find_last_of(',');
            if( n != string::npos ) {
                tokens[index-1] = tokens[index-1].substr(0,n);
            }

            // There are at least two possibilities
            // OCCURS 4
            // OCCURS 1 TO 15


            if( tokens.size() > index+2 AND tokens[index+2] EQ "TO" ) {
                // Pick up the maximum number of occurrences
                new_node->occurs = stoi(tokens[index+3]);

                // Remove "OCCURS 1 TO 15" from the tokens
                vector<string>::iterator it1 = tokens.begin() + index;
                vector<string>::iterator it2 = tokens.begin() + index + 4;
                tokens.erase(it1,it2);
            } else {
                // Assume "OCCURS 10"
                // Pick up the number of occurrences
                new_node->occurs = stoi(tokens[index+1]);

                // And now remove "OCCURS 10" from the tokens:
                vector<string>::iterator it1 = tokens.begin() + index;
                vector<string>::iterator it2 = tokens.begin() + index + 2;
                tokens.erase(it1,it2);
            }


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
            cerr << "cobcd-st: VARIABLE_TREE::ReadFromFile() doesn't know how to handle";
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

void
VARIABLE_TREE::DumpNode(ostream &sout, const VAR_NODE *node)
{
    if(node) {
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
                        OR  (node->level EQ LEVEL_77 AND current_level_in_dump != 77) ) {
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

                if(node->occurs) {
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
                sout << "] ";

                if( node->data_description_index != NO_INDEX ) {
                    sout << "[";
                    sout << node->data_description_index;
                    sout << "] ";
                } else {
                    sout << "[no_index] ";
                }

                sout << endl;
            }
        }
        for( VVAR_NODES::const_iterator it=node->children.begin();
                it!=node->children.end();
                it++) {
            DumpNode(sout,*it);
        }
    }
}

void
VARIABLE_TREE::Dump(ostream &sout)
{
    // Primarily for debugging.
    // Dump the whole tree, depth first:
    current_program_in_dump.clear();
    current_section_in_dump.clear();
    cout << "VARIABLE_TREE:" << endl;
    DumpNode(sout, root);
    cout << endl;
}

static VAR_NODE *
FindNameIn(VAR_NODE *node,const string &name)
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

void
VARIABLE_TREE::BuildCanonicalName(VAR_NODE *var_node)
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

            // We need to eliminate ourself from our parent's level.
            // We will find ourself, and NULL us out:

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
        for(size_t i=0; i<var_node->children.size(); i++ ) {
            VAR_NODE *child = var_node->children[i];
            if( child != NULL ) {
                BuildCanonicalName(child);
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

void
VARIABLE_TREE::BuildCanonicalName()
{
    BuildCanonicalName(root);
}

void
VARIABLE_TREE::BuildFlatList(VAR_NODE *node,VVAR_NODES &flat)
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

VAR_NODE *
DiveForVariable(        VAR_NODE *node,
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

void
VARIABLE_TREE::AdjustFlatList()
{
    /*  When there is, after an 00 FILE record, an 01 variable that isn't
        referenced in the code, you end up with that 01 not being associated
        with a b_ base symbol.

        This code scans for that situation in the flat_list, and borrows the
        b_ base_name from the 00 parent and assigns it to the 01 child.  */

    for( size_t i=1; i<flat_list.size(); i++ ) {
        if( flat_list[i-1]->level EQ 0
                AND flat_list[i]->base_symbol.empty()
                AND flat_list[i-1]->program EQ flat_list[i]->program
                AND flat_list[i]->level EQ 1 ) {
            // This is the situation: 01 has no base_symbol, but its 00
            // parent does.

            flat_list[i]->base_symbol = flat_list[i-1]->base_symbol;
        }
    }
}

void
VARIABLE_TREE::Consolidive(VAR_NODE *node,const DATA_DESCRIPTIONS &data_description,string &base_symbol)
{
    if( node ) {
        VSTRING tokens = Split(node->GetCanonicalName(),"/");
        if( tokens.size() >= 2 ) {
            if( node->level <= 1 OR node->level EQ 77 ) {
                // We're starting work on a brand-new level-one(ish) variable.  We need to
                // clear out the base_symbol:
                base_symbol = "" ;
            }

            string program_id = tokens.back();
            string cbl_symbol = tokens[0];
            int offset = node->GetOffset();

            // See if we have a data_descriptions entry for that cbl_symbol
            size_t index = NO_INDEX;
            if( data_description.IsInDs(program_id,cbl_symbol,offset) ) {
                // We do!  Save the index
                index = data_description.GetIndex(program_id,cbl_symbol,offset);
                node->data_description_index = index;
            } else if( data_description.IsInDs(program_id,cbl_symbol) ) {
                // We didn't find one specifically at that offset, but there is
                // non-specific entry (which happens when COBOL code doesn't
                // actually reference the variable
                index = data_description.GetIndex(program_id,cbl_symbol);
                node->data_description_index = index;
            }

            if( index EQ NO_INDEX AND !node->redefines.empty() ) {
                // We couldn't find ourself with our own name.  Perhaps we
                // can borrow our base_symbol from an older sibling.
                VSTRING tokes = Split(node->redefines,"/");

                if( data_description.IsInDs(program_id,tokes[0],offset) ) {
                    // We can!  Save the index
                    index = data_description.GetIndex(program_id,tokes[0],offset);
                } else if ( data_description.IsInDs(program_id,tokes[0]) ) {
                    index = data_description.GetIndex(program_id,tokes[0],offset);
                }
                if( index != NO_INDEX ) {
                    DATA_DESCRIPTION symbol = data_description[index];
                    if( base_symbol.empty() and !symbol.c_symbol.empty() ) {
                        // Replace our empty base_symbol with the one from data_descriptions
                        base_symbol = data_description[index].c_symbol;
                    }
                    node->base_symbol = data_description[index].c_symbol;
                }

                // We can't take anything except the base_symbol; none of the
                // rest of our sibling's fields are relevant:
                index = NO_INDEX;
            }

            if( index != NO_INDEX ) {
                DATA_DESCRIPTION symbol = data_description[index];
                if( base_symbol.empty() and !symbol.c_symbol.empty() ) {
                    // Replace our empty base_symbol with the one from data_descriptions
                    base_symbol = data_description[index].c_symbol;
                }
                // Here is where we combine the VARIABLE_TREE data with the DATA_DESCRIPTIONS
                // data to create everything we need:

                if( symbol.storage_size EQ 0 ) {
                    node->best_size = symbol.field_size;
                } else if( symbol.field_size EQ 0 ) {
                    node->best_size = symbol.storage_size;
                } else {
                    node->best_size = symbol.field_size;
                }

                if( node->picture.empty() ) {
                    node->best_picture = symbol.picture_string;
                } else if ( symbol.picture_string.empty() ) {
                    node->best_picture = node->picture;
                } else {
                    // If they're both on, go with the one from the listing file
                    node->best_picture = node->picture;
                }

                if( node->level EQ 66 ) {
                    // The .cbl.lst file doesn't have good offset information
                    // for 66 REDEFINES.  But the symbol *might* have good information,
                    // which it gets from the f_ field_attribute record when the 66
                    // variable is referenced in the code.

                    node->offset = symbol.field_data_offset ;
                }

                node->base_symbol  = symbol.c_symbol    ;

                node->attr_type    = symbol.attr_type   ;
                node->attr_digits  = symbol.attr_digits ;
                node->attr_scale   = symbol.attr_scale  ;
                node->attr_flags   = symbol.attr_flags  ;
            } else {
                node->best_size = node->size;
                node->best_picture = node->picture;
            }

            // We might have a new base_symbol at this point; if we do, apply it to
            // this node:

            if( node->base_symbol.empty() AND !base_symbol.empty() ) {
                node->base_symbol = base_symbol;
            }

            // And our work here is done:
        }

        for( auto child : node->children ) {
            // Check our children
            Consolidive(child,data_description,base_symbol);

            if( !base_symbol.empty() AND node->base_symbol.empty() ) {
                // One of our descendents has a base_address, and we don't.  I'm not really
                // sure that can happen, but just in case, propogate that base_symbol
                // upward:
                node->base_symbol = base_symbol;
            }
        }
    }
}

void
VARIABLE_TREE::Consolidate(const DATA_DESCRIPTIONS &data_description)
{
    // data_description has f_ and b_ entries for working-storage and linkage
    // variables that are referenced in the COBOL code

    // It has b_ entries without f_ entries for level 01 variables that are
    // declared, but not referenced.

    // It has f_ entries without b_ entries for referenced local-storage variables

    // The data_description entries have partial variable name entries; for example
    // The variable 01 HOUSE. 05 GARAGE. 10 FLOOR X(12). has commented entries
    // in the .h file for HOUSE and FLOOR, but nothing for GARAGE or BEDROOM.

    // We sort that out by walking the tree and matching the fully-qualified
    // canonical name already in the variable tree with the partial entries in
    // data_description.

    string base_symbol="";
    Consolidive(root,data_description,base_symbol);
    BuildFlatList(root,flat_list);
    AdjustFlatList();

}

void
VARIABLE_TREE::DumpFlatList() const
{
    cout << "VARIABLE_TREE::FlatList:" << endl;
    cout << setw(20) << "program";
    cout << setw(20) << "section";
    cout << setw( 5) << "size";
    cout << setw(13) << "type";
    cout << setw( 6) << "level";
    cout << setw(42) << "name";
    cout << setw(16) << "picture";
    cout << setw( 8) << "usage";
    cout << setw(20) << "redefines";
    cout << setw( 7) << "occurs";
    cout << setw(42) << "canonical_name";
    cout << setw( 5) << "base";
    cout << setw( 7) << "offset";
    cout << setw( 6) << "bsize";
    cout << setw(16) << "bpicture";
    cout << setw( 6) << "atype";
    cout << setw( 6) << "adigits";
    cout << setw( 7) << "ascale";
    cout << setw( 7) << "aflags";
    cout << endl;
    for( auto node : flat_list ) {
        cout << setw(20) << node->program;
        cout << setw(20) << node->section;
        cout << setw( 5) << node->size;
        cout << setw(13) << node->type;
        cout << setw( 6) << node->level;
        cout << setw(42) << node->name;
        cout << setw(16) << node->picture;
        cout << setw( 8) << node->usage;
        cout << setw(20) << node->redefines;
        cout << setw( 7) << node->occurs;
        cout << setw(42) << node->canonical_name;
        cout << setw( 5) << node->base_symbol;
        cout << setw( 7) << node->offset;
        cout << setw( 6) << node->best_size;
        cout << setw(16) << node->best_picture;
        cout << setw( 6) << node->attr_type;
        cout << setw( 6) << node->attr_digits;
        cout << setw( 7) << node->attr_scale;
        cout << setw( 7) << node->attr_flags;
        cout << endl;
    }
    cout << endl;
}

void
VARIABLE_TREE::FormatVariablesInfo(stringstream &ss) const
{
    for( auto node : flat_list ) {
        ss << "D|";
        ss << node->section.substr(0,2) << "|";
        ss << ZeroIsNull(node->level) << "|" ;
        ss << node->canonical_name << "|" ;
        ss << node->base_symbol << "|" ;
        ss << ZeroIsNull(node->offset) << "|" ;
        ss << node->type.substr(0,1) << "|" ;
        ss << ZeroIsNull(node->best_size) << "|" ;
        ss << node->best_picture << "|" ;
        ss << node->usage << "|" ;
        ss << ZeroIsNull(node->occurs) << "|" ;
        ss << ZeroIsNull(node->attr_type) << "|" ;
        ss << ZeroIsNull(node->attr_digits) << "|" ;
        ss << ZeroIsNull(node->attr_scale) << "|" ;
        ss << ZeroIsNull(node->attr_flags) << "~";
    }
}

