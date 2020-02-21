// sfix.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <map>
#include <string.h>

#include "profiler.h"
#include "params.h"
#include "utils.h"

using namespace std;

#define EQ ==
#define AND &&
#define OR ||

typedef vector<size_t> VINT;

map<string,int>
GetInputFileMappings(const VSTRING &input_file)
{
    map<string,int> retval;

    for(string s : input_file) {
        if( s.find(".file ") != string::npos) {
            VSTRING tokens = Split(s,"\t ");
            if( tokens[0] EQ ".file" AND isdigit(tokens[1][0]) ) {
                int nfile = stoi(tokens[1]);
                string file = StripQuotes(tokens[2]);
                retval[file] = nfile;
            }
        }
    }
    return retval;
}

VINT
FindFunctionReferences(const VSTRING &input_file)
{
    VINT retval;
    int nline = 0;
    for(string s : input_file) {
        if( s.find("@function") != string::npos) {
            VSTRING tokens = Split(s,"\t ");
            if( tokens[0] EQ ".type" AND tokens[2] EQ "@function" ) {
                retval.push_back(nline);
            }
        }
        nline += 1;
    }
    return retval;
}

map<int,int>
FigureOutReplacements(const PARAMETERS &params,
                      const map<string,int> &input_file_mappings)
{
    map<int,int> retval;

    int next=2;
    for( map<string,int>::const_iterator it=input_file_mappings.begin();
            it != input_file_mappings.end();
            it ++) {
        if( it->first EQ params.c_filename ) {
            // Skip over the cobc-generated .c file
            retval[it->second] = 0;
            continue;
        }
        if( it->first.substr(it->first.length()-2) EQ ".h"
                OR it->first.substr(it->first.length()-2) EQ ".c") {
            // Skip over any xxxx.h file
            retval[it->second] = 0;
            continue;
        }
        if( it->first EQ params.cbl_filename ) {
            // Force the COBOL source file to be #1
            retval[it->second] = 1;
            continue;
        }
        retval[it->second] = next++;
    }

    return retval;
}

VSTRING
ReadEntireFile(string filename)
{
    VSTRING retval;
    FILE *f = fopen(filename.c_str(), "r");
    if( !f ) {
        cerr << "Couldn't open input file " << filename << endl;
    }

    char ach[16384];
    while(fgets(ach,sizeof(ach),f)) {
        string s = Trim(ach);
        retval.push_back(s);
    }
    fclose(f);
    return retval;
}


int
main(int argc, char **argv)
{
    PROFILER;

    PARAMETERS params = GetParameters(argc, argv);
    if(!params.quiet) {
        cout << "sfix version " << VERSION << endl;
    }

    VSTRING input_file = ReadEntireFile(params.s_input_filename.WholePath());
    map<string,int>input_file_mappings = GetInputFileMappings(input_file);
    map<int,int>replacements = FigureOutReplacements(params,input_file_mappings);
    VINT function_references = FindFunctionReferences(input_file);

    // Let's hang around here and do the hard work:

    stringstream ss;

    // Start the file off with our filename:
    ss << "\t.file\t\"" << params.cbl_filename << "\"" << endl;

    // And then, let's put the modified list of filenames right at the beginning:

    for(map<string,int>::const_iterator it=input_file_mappings.begin();
            it != input_file_mappings.end();
            it++
       ) {
        int oldnum = it->second;
        int newnum = replacements[oldnum];
        if( newnum ) {
            ss << "\t.file " << newnum << " \"" << it->first << "\"" << endl;
        }
    }

    int next_function_reference_index = 0;
    size_t interject_line_number = -1;
    string interject_line = "";
    for( size_t i=0; i<input_file.size(); i++ ) {
        if( i EQ interject_line_number ) {
            ss << interject_line;
        }
        VSTRING tokens = Split(input_file[i]," \t");
        if( tokens[0] EQ ".file" ) {
            // We've already taken care of all .file lines
            continue;
        }
        if( tokens[0] EQ ".loc" ) {
            // We need to replace the old file number with the new one
            int oldnum = stoi(tokens[1]);
            int newnum = replacements[oldnum];
            if( newnum != 0 ) {
                // Rebuild the .loc line
                ss << "\t";
                ss << tokens[0];
                ss << " ";
                ss << newnum;
                size_t index = 2;
                while( index < tokens.size() ) {
                    ss << " ";
                    ss << tokens[index++];
                }
                ss << endl;
            }
            continue;
        }
        if(next_function_reference_index < function_references.size()
                && i EQ function_references[next_function_reference_index] ) {
            // We have found a match.
            next_function_reference_index += 1;

            // We are going to look ahead for the next valid .loc:

            size_t ii = i+4;
            while(ii < input_file.size() ) {
                VSTRING tokens = Split(input_file[ii]," \t");
                if( tokens[0] EQ ".loc" ) {
                    // We need to replace the old file number with the new one
                    int old_num = stoi(tokens[1]);
                    int new_num = replacements[old_num];
                    if( new_num != 0 ) {
                        // Build the replacement line:
                        int new_line = stoi(tokens[2])-1;
                        stringstream ss;

                        ss << "\t.loc";
                        ss << " " << new_num ;
                        ss << " " << new_line ;
                        size_t index = 4;
                        while( index < tokens.size() ) {
                            ss << " ";
                            ss << tokens[index++];
                        }
                        ss << endl;

                        interject_line = ss.str();
                        interject_line_number = i+4;
                        break;
                    }
                }

                ii += 1;
            }
        }
        ss << input_file[i] << endl;
    }

    FILE *f = fopen(params.s_output_filename.WholePath().c_str(),"w");
    if( !f ) {
        cerr << "Couldn't open input file " << params.s_output_filename.WholePath() << endl;
        exit(1);
    }
    fprintf(f,"%s",ss.str().c_str());
    fclose(f);
    return 0;
}

