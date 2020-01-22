#include "utils.h"
#include "profiler.h"

#define EQ ==
#define AND &&
#define OR ||

using namespace std;

string Trim(const string &s)
    {
    PROFILER;
    string retval=s;
    static const string WHITESPACE = " \t\r\n\f";
    size_t nfound = retval.find_last_not_of(WHITESPACE);
    if(nfound != string::npos)
        {
        nfound += 1;
        retval = retval.substr(0,nfound);
        }
    nfound = retval.find_first_not_of(WHITESPACE);
    if(nfound != string::npos && nfound > 0)
        {
        retval = retval.substr(nfound);
        }
    return retval;
    }

string StripQuotes(const string &s)
    {
    PROFILER;
    string retval=s;
    if(retval.size() >= 1 AND retval[retval.size()-1] EQ '"' OR retval[retval.size()-1] EQ '\'')
        {
        retval = retval.substr(0,retval.size()-1);
        }
    if(retval.size() >= 1 AND retval[0] EQ '"' OR retval[0] EQ '\'')
        {
        retval = retval.substr(1);
        }
    return retval;
    }

vector<string> Split(const string &s,const string &delimiters)
    {
    PROFILER;
    vector<string>retval;

    size_t next = 0;
    string snext;
    while(next < s.size())
        {
        char ch = s[next++];
        for(size_t i=0; i<delimiters.size(); i++)
            {
            if(ch EQ delimiters[i])
                {
                retval.push_back(snext);
                snext.clear();
                goto next_c;
                }
            }
        snext += ch;
next_c:
        ;
        }
    if(!snext.empty())
        {
        retval.push_back(snext);
        }

    return retval;
    }

string Replace(const string &s,const char was, const char is)
    {
    PROFILER;
    string retval;

    for(size_t i=0; i<s.size(); i++)
        {
        char ch = s[i];
        if(ch EQ was)
            {
            ch = is;
            }
        retval += ch;
        }
    return retval;
    }