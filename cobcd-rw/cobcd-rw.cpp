// cobcd-rw.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <set>
#include <string>

#include "../include/version.h"

using namespace std;

#define EQ ==
#define AND &&
#define OR ||

extern set<string>CobolReservedWords;

void GetRidOfComments(char *s)
    {
    char *p;
    if( (p = strstr(s, "*>")) )
        {
        // Get rid of free-style comments:
        *p = '\0';
        }

    if( strlen(s) >= 7 AND s[6] EQ '*' )
        {
        // Live dangerously and assume this is a fixed-style comment.
        *s = '\0';
        }
    }

void GetRidOfTextLiterals(char *s)
    {
    // Do some ham-handed processing of quotes:
    for(;;)
        {
        char *pq = strchr(s,'\'');
        char *pqq = strchr(s,'\"');
        if( !pq AND !pqq )
            {
            // There are no quotes
            break;
            }
        // There is at least one quote or apostrophe.  The leftmost one
        // governs:
        if( !pq )
            {
            // There is only a double-quote
            pq = pqq + 1;
            }
        else if( !pqq )
            {
            // There is only an apostrophe
            pqq = pq +1;
            }
        if( pqq < pq )
            {
            // We'll use pq as the pointer
            pq = pqq;
            }
        // Pick up the delimiting character
        char q = *pq;
        *pq++ = ' ' ; // Kill the leading delimiter

                      // If this is a B" N" X" BX" or NX" literal, wipe out those
                      // leading characters
        if( pq - s > 1 )
            {
            char prior = *(pq-1) ;
            if( prior EQ 'N' OR prior EQ 'B' )
                {
                *(pq-1) = ' ';
                }
            else if( prior EQ 'X' )
                {
                char priorprior = '\0';
                if( pq - s > 2 )
                    {
                    priorprior = *(pq-2);
                    if( priorprior EQ 'N' OR priorprior EQ 'B' )
                        {
                        *(pq-2) = ' ';
                        }
                    }
                *(pq-1) = ' ';
                }
            }

        // Now, look for the ending apostophe, sowing destruction as we go
        while( *pq )
            {
            if( *pq EQ q AND *(pq+1) EQ q )
                {
                // We have a double-delimiter.  This is supposed to be inside the
                // literal, so we'll just blank them both out and then keep going
                *pq++ = ' ';
                *pq++ = ' ';
                continue;
                }
            if( *pq EQ q )
                {
                // This is a delimiter that we know is not followed by another one.  Thus,
                // this is the ending delimiter.  Wipe it out, and we're done
                *pq++ = ' ';
                break;
                }
            // This is just some random character inside the text literal.  Wipe it out,
            // and keep going
            *pq++ = ' ';
            }
        // We have wiped out a text literal inside the line.  Go back and do it again,
        // in case there are two text literals in this single line.
        continue;
        }
    }

void CheckWord(set<string>&results, const string &token)
    {
    if( token.empty() )
        {
        // Not sure how this can happen
        return;
        }

    bool okay = false;
    for( size_t i=0; i<token.size(); i++ )
        {
        if( isalpha(token[i]) )
            {
            okay = true;
            break;
            }
        }
    if( !okay )
        {
        // There has to be at least one [A-Z] character
        return;
        }

    // The leading and trailing characters can't be underscore or hyphen
    if( token[0] EQ '-' )
        {
        return;
        }
    if( token[0] EQ '_' )
        {
        return;
        }
    if( token[token.size()-1] EQ '-' )
        {
        return;
        }
    if( token[token.size()-1] EQ '_' )
        {
        return;
        }

    if( CobolReservedWords.find(token) EQ CobolReservedWords.end() )
        {
        // It's not a COBOL reserved word
        results.insert(token);
        }
    }

void FindTokensIn(char *s,set<string> &results)
    {
    // This routine feels free to destroy the string it's been handed.
    // You Have Been Warned

    _strupr(s);  // See? We've already started.

    GetRidOfComments(s);
    GetRidOfTextLiterals(s);

    // At this point, comments have been removed.  Text literals, along with their
    // bounding apostrophes or quotation marks, have been turned to spaces

    // We are going to scan through the line, getting rid of extraneous characters.  

    // We are operating under the assumption that the valid character set for variables (the 
    // attentive student will recall that we are actually looking for variables) 
    // is [A-Z0-9].  Embedded '-' and '_' are valid, but valid data names can't start 
    // or end with them.  All other characters, therefore, are slated for destruction:

    char *p = s;
    while( *p )
        {
        bool keep = false;
        keep = keep OR isdigit(*p);
        keep = keep OR isalpha(*p);
        keep = keep OR *p EQ '-';
        keep = keep OR *p EQ '_';
        if( !keep )
            {
            *p = ' ';
            }

        p += 1;
        }

    // We can now tokenize the line, looking for things in between spaces:

    string token;
    p = s;

    while(*p)
        {
        char ch = *p++;
        if( ch EQ ' ' )
            {
            if( !token.empty() )
                {
                CheckWord(results,token);
                token.clear();
                }
            }
        else
            {
            token += ch;
            }
        }
    if( !token.empty() )
        {
        CheckWord(results,token);
        }
    }

int main(int argc, char **argv)
    {
    if( argc > 1 AND (strcmp(argv[1],"--version") EQ 0 OR strcmp(argv[1],"-v") EQ 0 ) )   
        {
        cout << "cobcd-rw Version " << VERSION << endl;
        return 1;
        }

    if( argc < 4 )
        {
        cout << "cobcd-rw Version " << VERSION << endl;
        cout << "" << endl;
        cout << "Usage:" <<endl;
        cout << "" << endl;
        cout << "   cobcd-rw <file> <line_number> <range>" <<endl;
        cout << "   Reads a COBOL source file <file>.  Looks at <line_number-range> through" << endl;
        cout << "   <line_number+range>.  Filters out COBOL reserved words, and returns the" << endl;
        cout << "   tokens that are probably names of variables." << endl;
        cout << "" << endl;
        cout << "It is envisioned that nobody will ever actually use this program manually, but rather" << endl;
        cout << "it will be invoked by the COBOLworx cobcd.py GDB debugging extension." << endl;
        return 1;
        }

    int line_number = atoi(argv[2]);
    int range       = atoi(argv[3]);

    range = range < 0 ? 0 : range;

    int low_line = line_number - range;
    int high_line = line_number + range;

    low_line = low_line < 1 ? 1 : low_line;
    high_line = high_line < 1 ? 1 : high_line;

    FILE *f = fopen(argv[1],"r");
    if( !f )
        {
        cout << "Unable to open the input file: " << argv[1] << endl;
        return 1;
        }

    int current_line = 0;
    char ach[16384];
    set<string>results;
    while( fgets(ach,sizeof(ach),f) )
        {
        if( strlen(ach) )
        ach[ strlen(ach)-1 ] = '\0' ;
        current_line += 1;
        if( current_line > high_line )
            {
            break;
            }
        if( current_line < low_line )
            {
            continue;
            }
        FindTokensIn(ach,results);
        }

    for( set<string>::const_iterator it=results.begin(); it != results.end(); it++ )
        {
        cout << *it << " ";
        }
    results.clear();
    cout << endl;

    fclose(f);
    return 0;
    }

