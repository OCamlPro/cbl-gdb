#include <iostream>
#include <stdarg.h>
#include <string.h>

#include "utils.h"

#define EQ ==
#define AND &&
#define OR ||

using namespace std;

static const int DEFAULT_COUNT = 100;
static const char OUTPUT_FILE_NAME[] = "inputs.tab";

static const int MAX_DIGITS = 18;

int RandomRange(int lower, int upper)
    {
    return lower + (int)((upper-lower) * (rand()/(RAND_MAX + 1.00)));
    }

bool CoinFlip()
    {
    return rand() >= RAND_MAX/2;
    }

string RepRange(char ch,int rep)
    {
    string retval;

    if(rep < 4)
        {
        retval = string(rep,ch);
        }
    else
        {
        retval = ch;
        retval += '(';
        retval += to_string(rep);
        retval += ')';
        }

    return retval;
    }

class ITEM
    {
    private:
        std::string name;
        std::string picture_characters; // 'c' means CR; 'd' means DB
        int max_length;
        VSTRING usage_possibilities;

        const std::string &PickUsage() const
            {
            int n = RandomRange(0,(int)usage_possibilities.size());
            return usage_possibilities[n];
            }

        void BuildNumericEntry(int &nvar_count, FILE *results) const
            {
            std::string picture="";
            std::string usage="";
            int ldigits = 0;
            int rdigits = 0;

            for( int s=0; s<4; s++ )
                {
                if( (s & 0x02) != 0 )
                    {
                    // Build a P-scaled PICTURE
                    for(int pcount=-9; pcount<=9; pcount++)
                        {
                        if(pcount EQ 0)
                            {
                            continue;
                            }
                        for(int nine_count=1; nine_count <= MAX_DIGITS-abs(pcount); nine_count++)
                            {
                            picture = "";
                            if( (s & 0x1) != 0 )
                                {
                                picture += 'S';
                                }
                            if(pcount < 0)
                                {
                                picture += RepRange('P',-pcount);
                                picture += RepRange('9',nine_count);
                                rdigits = -pcount + nine_count;
                                }
                            else
                                {
                                picture += RepRange('9',nine_count);
                                picture += RepRange('P',pcount);
                                ldigits = pcount + nine_count;
                                }
                            // We have the picture, now pick the usage:
                            for( size_t i=0; i<usage_possibilities.size(); i++ )
                                {
                                usage = usage_possibilities[i];
                                if(usage EQ "PACKED-DECIMAL"
                                   OR usage EQ "COMP-3"
                                   OR usage EQ "COMPUTATIONAL-3"
                                   OR usage EQ "COMP-6"
                                   )
                                    {
                                    continue;
                                    }
                                // We can now build the output:
                                std::string output = "\t";  // Skip the "Skip" parameter

                                output += "VAR";
                                output += to_string(nvar_count++);
                                cout << output << endl;

                                output += "\t";
                                output += picture;

                                output += "\t";
                                output += usage;

                                output += "\t";
                                // Build a default value:
                                // As of cobc version 3.1, we need to have
                                // pcount digits to avoid having the runtime
                                // just hang, which it does sometimes.
                                // Example:  01 VAR1 PICTURE 9PP USAGE COMP VALUE 0 .
                                if( pcount < 0 )
                                    {
                                    output += '.';
                                    }
                                output += string(abs(pcount)+nine_count,'0');

                                output += "\t";
                                output += to_string(ldigits);

                                output += "\t";
                                output += to_string(rdigits);

                                //cout << output << endl;
                                fprintf(results,"%s\n",output.c_str());
                                }
                            }
                        }
                    }
                else
                    {
                    // Build a 99V99 style PICTURE
                    for( int ldigits=0; ldigits<=MAX_DIGITS; ldigits++ )
                        {
                        for( int rdigits=0; rdigits<=(MAX_DIGITS-ldigits); rdigits++ )
                            {
                            if( rdigits EQ 0 AND ldigits EQ 0 )
                                {
                                continue;
                                }
                            picture = "";
                            if( (s & 0x1) != 0 )
                                {
                                picture += 'S';
                                }
                            if( ldigits > 0 )
                                {
                                picture += RepRange('9',ldigits);
                                }
                            if( rdigits > 0 OR CoinFlip() )
                                {
                                // If there are no rdigits, only put the V in sometimes
                                picture += 'V' ;
                                }
                            if( rdigits > 0 )
                                {
                                picture += RepRange('9',rdigits);
                                }
                            // We have the picture, now pick the usage:
                            for( size_t i=0; i<usage_possibilities.size(); i++ )
                                {
                                usage = usage_possibilities[i];
                                // We can now build the output:
                                std::string output = "\t";  // Skip the "Skip" parameter

                                output += "VAR";
                                output += to_string(nvar_count++);
                                cout << output << endl;

                                output += "\t";
                                output += picture;

                                output += "\t";
                                output += usage;

                                output += "\t";
                                output += "0";  // Default Value

                                output += "\t";
                                output += to_string(ldigits);

                                output += "\t";
                                output += to_string(rdigits);

                                //cout << output << endl;
                                fprintf(results,"%s\n",output.c_str());
                                }
                            }
                        }
                    }
                }
            }

    public:
        ITEM(const std::string &name_,
             const std::string &picture_characters,
             int max_length_,
             const VSTRING &usage_possiblities_
            ):
            name(name_),
            picture_characters(picture_characters),
            max_length(max_length_),
            usage_possibilities(usage_possiblities_)
            {
            }

        void BuildEntry(int &nvar_count, FILE *results) const
            {
            if( name EQ "Numeric" )
                {
                BuildNumericEntry(nvar_count, results);
                }
            }
        
    };
typedef std::vector<ITEM>VITEMS;

ITEM BuildItem(const std::string &name,
               const std::string &picture_characters,
               int max_length,
               ...)
    {
    VSTRING usage_possibilities;
    va_list valist;
    va_start(valist, max_length);
    for(;;)
        {
        const char *p = va_arg(valist,const char *);
        usage_possibilities.push_back(p);
        if( strlen(p) EQ 0 )
            {
            break;
            }
        }
    va_end(valist);
    return ITEM(name,
                picture_characters,
                max_length,
                usage_possibilities);
    }

VITEMS BuildListOfItems()
    {
    VITEMS retval;

    retval.push_back( BuildItem("Numeric",
                                "S9VP",
                                18,
                                "DISPLAY",
                                "BINARY",
                                "COMP",
//                                "COMPUTATIONAL",
                                "PACKED-DECIMAL",
                                "COMP-3",
//                                "COMPUTATIONAL-3",
                                "COMP-4",
//                                "COMPUTATIONAL-4",
                                "COMP-5",
//                                "COMPUTATIONAL-5",
                                "COMP-6",
                                "COMP-X",
                                "") );
#if 0
    retval.push_back( BuildItem("Numeric-Edited",
                                "*$BPVZ90/,.+-cd",
                                18,
                                "DISPLAY",
                                "" ) );
    retval.push_back( BuildItem("Alphabetic",
                                "A",
                                160,
                                "DISPLAY",
                                "") );
    retval.push_back( BuildItem("Alphanumeric",
                                "AX9",
                                160,
                                "DISPLAY",
                                "" ) );
    retval.push_back( BuildItem("Alphanumeric-Edited",
                                "AX9B0/",
                                160,
                                "DISPLAY",
                                "" ) );
#endif
    return retval;
    }

const ITEM &SelectRandomItem(const VITEMS &items)
    {
    int n = RandomRange(0,(int)items.size());
    return items[n];
    }

void GenerateCobolVariableForSelfCheck(int &nvar_count, FILE *results,const VITEMS &items)
    {
    const ITEM item = items[0];
    item.BuildEntry(nvar_count, results);
    }

int main(int argc, char **argv)
    {
    printf("Test case generator version 0.7\n");
    int ncount = 0;
    if( argc >= 2)
        {
        ncount = atoi(argv[1]);
        }
    if( ncount EQ 0 )
        {
        ncount = DEFAULT_COUNT;
        }

    VITEMS items = BuildListOfItems();
    srand(47);

    FILE *results = fopen(OUTPUT_FILE_NAME,"w");
    if(results EQ NULL)
        {
        cout << "Couldn't open " << OUTPUT_FILE_NAME << endl;
        exit(1);
        }
    fprintf(results,"%s\n","Skip\tName\tPicture\tUsage\tValue\tMajor\tMinor");

    int nvar_count = 1;
    for(int i=0; i<ncount; i++)
        {
        GenerateCobolVariableForSelfCheck(nvar_count, results,items);
        break;
        }
    printf("Generated %d test variables\n",nvar_count-1);

    fclose(results);
    }

