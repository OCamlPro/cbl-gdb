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


map<string,int>
GetInputFileMappings(const string &filename)
    {
    map<string,int> retval;

    FILE *f = fopen(filename.c_str(), "r");
    if( !f )
        {
        cerr << "Couldn't open input file " << filename << endl;
        }

    char ach[16384];
    while(fgets(ach,sizeof(ach),f))
        {
        if( strstr(ach,".file") )
            {
            string s = Trim(ach);
            VSTRING tokens = Split(s,"\t ");
            if( tokens[0] EQ ".file" AND isdigit(tokens[1][0]) )
                {
                int nfile = stoi(tokens[1]);
                string file = StripQuotes(tokens[2]);
                retval[file] = nfile;
                }
            }
        }
    fclose(f);
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
        it ++)
        {
        if( it->first EQ params.c_filename )
            {
            retval[it->second] = 0;
            continue;
            }
        if( it->first EQ params.cbl_filename )
            {
            retval[it->second] = 1;
            continue;
            }
        retval[it->second] = next++;
        }

    return retval;
    }

int 
main(int argc, char **argv)
    {
    PROFILER;

    PARAMETERS params = GetParameters(argc, argv);
    if(!params.quiet) 
        {
        cout << "sfix version " << VERSION << endl;
        }

    map<string,int>input_file_mappings = GetInputFileMappings(params.s_input_filename.WholePath());

    map<int,int>replacements = FigureOutReplacements(params,input_file_mappings);

    // Let's hang around here and do the hard work:

    stringstream ss;

    // Start the file off with our filename:
    ss << "\t.file\t\"" << params.cbl_filename << "\"" << endl;

    FILE *f = fopen(params.s_input_filename.WholePath().c_str(),"r");

    // Skip the first line
    char ach[16384];
    fgets(ach,sizeof(ach),f);

    // Now, do the line-by-line stuff:
    while(fgets(ach,sizeof(ach),f))
        {
        if( strstr(ach,".file ") OR strstr(ach,".loc ") )
            {
            string s = Trim(ach);
            VSTRING tokens = Split(s,"\t ");
            int nfile = stoi(tokens[1]);
            map<int,int>::const_iterator it = replacements.find(nfile);
            if( it != replacements.end() )
                {
                int newfile = it->second;
                if( newfile != 0 )
                    {
                    ss << "\t";
                    ss << tokens[0];
                    ss << " ";
                    ss << newfile;
                    size_t index = 2;
                    while( index < tokens.size() )
                        {
                        ss << " ";
                        ss << tokens[index++];
                        }
                    ss << endl;
                    }
                }
            }
        else
            {
            ss << ach;
            }
        }
    fclose(f);

    f = fopen(params.s_output_filename.WholePath().c_str(),"w");
    if( !f )
        {
        cerr << "Couldn't open input file " << params.s_output_filename.WholePath() << endl;
        exit(1);
        }
    fprintf(f,"%s",ss.str().c_str());
    fclose(f);
    return 0;
    }

