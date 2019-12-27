#include <iostream>
#include <sstream>
#include <math.h>

#include "params.h"
#include "utils.h"

#define EQ ==
#define AND &&
#define OR ||

using namespace std;

static const char PROGRAM_NAME[] = "program_under_test";
static const char PROGRAM_UNDER_TEST[] = "program_under_test.cbl";
static const char GDB_SCRIPT[] = "gdb_script.txt";
static const char OUR_INPUT_FILE_NAME[] = "inputs.tab";
static const char DUT_INPUT_FILE_NAME[] = "inputs.txt";
static const char INTERMEDIATE_RESULTS[] = "results.txt";
static const char RESULTS_FILE_NAME[] = "results.tab";
static const char ERRORS_FILE_NAME[] = "errors.txt";

static const char DEBUGGING_ROOT[] = "/home/bob/projects/cobol/cbl-gdb/selfcheck/";

void ExecuteCommand(const char *cmd)
{
#ifdef _WIN32
    string command = "ssh bob@doobie ";
    command += "cd \"";
    command += DEBUGGING_ROOT;
    command += "\";";
    command += cmd;
    int result = system(command.c_str());
    if( result != 0) {
        exit(1);
    }
#else
    int result = system(cmd);
    if( result != 0) {
        exit(1);
    }
#endif
}

char RandomDigit()
{
    char digit = '0' + (char)(10.0 *rand() / (RAND_MAX + 1.0));
    return digit;
}

string PictureExpand(const string &p)
{
    string retval;
    char lch;
    for( size_t i=0; i<p.size(); i++ ) {
        char ch = p[i];
        if( ch EQ '(' ) {
            i += 1; // Skip past parenthesis
            int ncount = 0;
            for(;;) {
                ch = p[i++];
                if( ch EQ ')' ) {
                    break;
                }
                ncount = ncount*10 + ch-'0';
            }
            ncount -= 1;
            retval += string(ncount,lch);
            i -= 1;
            continue;
        }

        lch = ch;
        retval += ch;
    }
    return retval;
}

void Scenario(FILE *script,
              FILE *inputs,
              const string &Name,
              const string &value,
              const string &Picture)
{
    // Generate the formalized X(1)9(18)XV9(18) line for the
    // inputs.txt file:

    const int MAX_DIGITS = 18;

    // Split the input value on a decimal point:
    VSTRING lr = Split(value,".");
    if( lr.size() < 2 ) {
        // There was no right side, so create one.
        lr.push_back("0");
    }
    string left = lr[0];
    string right = lr[1];

    // Pick up the minus sign, if there is one:
    char minus = ' ';
    if( left.size() > 0 AND left[0] EQ '-' ) {
        // Flag this number as negative
        minus = '-';
        // strip off the minus sign
        left = left.substr(1);
    }

    // Build the right side out to be 18 digits:
    while( right.size() < MAX_DIGITS ) {
        right += '0';
    }
    // While making sure it's no more than that:
    right = right.substr(0,MAX_DIGITS);

    // Make sure the left side isn't empty:
    if( left.empty() ) {
        left = "0";
    }

    // In the case of a Picture string with 'P' decimal scaling, we
    // have to make sure the values are not out of range:

    string picture = PictureExpand(Picture);

    size_t first_p = picture.find("P");
    size_t first_9 = picture.find("9");

    if( first_p != string::npos ) {
        // We have P-scaling

        // Count up how many P's there are
        int pcount = 0;
        for(int i=0; i<picture.size(); i++) {
            if( picture[i] EQ 'P' ) {
                pcount += 1;
            }
        }

        // Compare the location of the first P to the first 9
        if( first_p < first_9 ) {
            // This is right-of-decimal scaling
            left = "0";
            int i=0;
            // Zero out the leftmost characters
            while( i<pcount AND i<right.size() ) {
                right[i] = '0';
                i += 1;
            }
        } else {
            // This is left-of-decimal scaling
            right = "0";
            int i=0;
            // Zero out the rightmost characters
            while( i<pcount AND i<left.size() ) {
                left[left.size()-1-i] = '0';
                i += 1;
            }
        }
    }

    long long nleft = stoll(left);
    long long nright = stoll(right);

    // This shouldn't be necessary, but limit the left and right sides
    // to MAX_DIGITS characters
    nleft  = nleft  % 999999999999999999LL;
    nright = nright % 999999999999999999LL;

    // Output in the X(1)9(18)XV9(18) format expected by the COBOL program.
    fprintf(inputs,"%c%018lld.%018lld\n",minus,nleft,nright);

    // Now, built the value for the GDB command:
    string pvalue;
    if( minus EQ '-' ) {
        pvalue += '-';
    }
    pvalue += to_string(nleft);  // This is the left side, sans leading zeroes
    pvalue += '.';               // We need a decimal
    pvalue += right;             // This is the MAX_DIGITS right side

    // Strip off trailing zeroes
    while(pvalue[pvalue.length()-1] EQ '0') {
        pvalue = pvalue.substr(0,pvalue.length()-1);
    }

    // Generate the GDB commands
    fprintf(script,"p %s=%s\n",Name.c_str(),pvalue.c_str());
    fprintf(script,"c\n");
}

void RandomScenario(FILE *script,
                    FILE *results,
                    const string &Name,
                    int ldigits,
                    int rdigits,
                    const string &Picture)
{
    if(ldigits EQ 0 AND rdigits EQ 0) {
        // If nothing is wanted, nothing is what is gotten.
        return;
    }

    string value = "";

    if( rand() > RAND_MAX/2 ) {
        value += '-';
    }

    string lsegment;
    string rsegment;
    char ch;

    if( ldigits ) {
        while( (ch = RandomDigit()) EQ '0' ) {
            // Make sure first digit is non-zero
        }
        lsegment = ch;
        while( lsegment.length() < ldigits) {
            lsegment += RandomDigit();
        }
    }

    if( rdigits ) {
        while( rsegment.length() < rdigits) {
            rsegment += RandomDigit();
        }
    }

    value += lsegment;
    if( rsegment.size() > 0 ) {
        value += '.';
        value += rsegment;
    }

    Scenario(script,results,Name,value,Picture);
}

string Digits(ULONGLONG v, int ndigits)
{
    if(ndigits <= 0) {
        return "";
    }
    string retval = string(ndigits,'0');
    while(--ndigits >= 0) {
        retval[ndigits] = (char)(v%10 + '0');
        v /= 10;
    }
    return retval;
}

void GenerateGdbScript(int line_number,
                       const string & Name,
                       int major_digits,
                       int minor_digits,
                       const string &Picture)
{
    FILE *script;
    script = fopen(GDB_SCRIPT,"w");
    if(script EQ NULL) {
        cout << "Couldn't open " << GDB_SCRIPT << endl;
        exit(1);
    }

    // This next name is confusing.  DUT means "Device Under Test", and this
    // output file, named "inputs", contains values read by the generated
    // COBOL file.
    FILE *inputs;
    inputs = fopen(DUT_INPUT_FILE_NAME,"w");
    if(inputs EQ NULL) {
        cout << "Couldn't open " << DUT_INPUT_FILE_NAME << endl;
        exit(1);
    }

    // Generate the GDB commands that will load the program, and launch it,
    // stopping it the first time it sees the DISPLAY "After:" line.
    string output = "file ";
    output += PROGRAM_NAME;
    fprintf(script,"%s\n",output.c_str());
    fprintf(script,"b %d\n",line_number);
    fprintf(script,"r\n");

    // At this point, when the script runs, program_under_test will be
    // trapped just before the statement 'DISPLAY <name>'

    // We will start with some canned scenarios:
    // Minimum:
    Scenario(script,inputs,Name,"0",Picture);
    Scenario(script,inputs,Name,"0.",Picture);
    Scenario(script,inputs,Name,".0",Picture);

    // +/- 1:
    Scenario(script,inputs,Name,"1",Picture);
    Scenario(script,inputs,Name,"1.",Picture);
    Scenario(script,inputs,Name,"-1",Picture);
    Scenario(script,inputs,Name,"-1.",Picture);

    // Maximum:
    double d1 = pow(10,major_digits);
    d1 = floor(d1-0.5);
    ULONGLONG n1 = (ULONGLONG)d1;
    double d2 = pow(10,minor_digits);
    d2 = floor(d2-0.5);
    ULONGLONG n2 = (ULONGLONG)d2;
    string value = Digits(n1,major_digits);
    value += ".";
    value += Digits(n2,minor_digits);
    Scenario(script,inputs,Name,value,Picture);

    if( minor_digits > 0) {
        // Minimum
        value = string(minor_digits,'0');
        value[minor_digits-1] = '1';
        value = "." + value;
        Scenario(script,inputs,Name,value,Picture);
    }

    // Negative maximum:
    value = '-' + value;
    Scenario(script,inputs,Name,value,Picture);

    for(int l=0; l <= major_digits; l++) {
        for(int r=0; r <= minor_digits; r++) {
            RandomScenario(script,inputs,Name,l,r,Picture);
            if( minor_digits > 6 AND r EQ 3) {
                r = minor_digits - 4 ;
            }
        }
        if( major_digits > 6 AND l EQ 3) {
            l = major_digits - 4 ;
        }
    }

    // And generate the instruction that will cause GDB to exit
    fprintf(script,"q\n");
    fclose(script);
    fclose(inputs);

    // Having built the script (and inputs), execute it:
    string command = "gdb <";
    command += GDB_SCRIPT;
    command += " >";
    command += INTERMEDIATE_RESULTS;

    ExecuteCommand(command.c_str());
}

int GenerateCobolProgram(const string &Name,
                         const string &Picture,
                         const string &Usage,
                         const string &Value)
{
    FILE *program;
    program = fopen(PROGRAM_UNDER_TEST,"w");
    if(program EQ NULL) {
        cout << "Couldn't open " << PROGRAM_UNDER_TEST << endl;
        exit(1);
    }

    const char *program_body[] = {
        "IDENTIFICATION DIVISION.",
        "PROGRAM-ID. SELF-TEST.",
        "ENVIRONMENT DIVISION.",
        "INPUT-OUTPUT SECTION.",
        "FILE-CONTROL.",
        "SELECT PGM-INPUT ASSIGN TO 'inputs.txt' ",
        "ORGANIZATION IS LINE SEQUENTIAL.",
        "DATA DIVISION.",
        "FILE SECTION.",
        "FD PGM-INPUT.",
        "01 INPUT-RECORD.",
        "02 SIGNCHAR PICTURE X(1).",
        "02 HIPART PICTURE 9(18)V.",
        "02 FILLER PICTURE X(1) VALUE '.' .",
        "02 LOPART PICTURE V9(18).",
        "WORKING-STORAGE SECTION.",
        "01 EOF PICTURE X VALUE '0'.",
        "01 VARx PIC S9999V9999. *>declaration",
        "PROCEDURE DIVISION.",
        "OPEN INPUT PGM-INPUT.",
        "PERFORM UNTIL EOF = '1'",
        "READ PGM-INPUT AT END MOVE '1' TO EOF",
        "END-READ",
        "IF EOF = '0' ",
        "ADD HIPART TO LOPART GIVING VARx",
        "IF SIGNCHAR = '-'",
        "MULTIPLY -1 BY VARx GIVING VARx",
        "END-IF",
        "DISPLAY 'Before:' VARx",
        "DISPLAY 'After:' VARx *> PICTURE USAGE",
        "MOVE value TO VARx",
        "END-IF",
        "END-PERFORM.",
        "CLOSE PGM-INPUT.",
        "STOP RUN.",
    };
    const size_t program_body_lines = sizeof(program_body)/sizeof(program_body[0]);

    int retval = -1;    // This is the line number of the trap
    int line_number = 1;// This is the counter for the 000001 prefix on each line
    string output;

    for(size_t i=0; i<program_body_lines; i++ ) {
        string body = program_body[i];
        if( body.find("declaration") != string::npos ) {
            stringstream ss;
            ss << "01 " << Name ;
            if( !Picture.empty() ) {
                ss << " PIC " << Picture ;  // PIC instead of PICTURE to keep some lines
                //                          // from growing past character position 72
            }
            if( !Usage.empty() ) {
                ss << " USAGE " << Usage ;
            }
            if( false AND !Value.empty() ) {
                ss << " VALUE " << Value ;
            }
            ss << " .";
            body = ss.str();
        }

        const string VARx="VARx";
        const string value="value";
        size_t n;
        while( (n = body.find(VARx)) != string::npos ) {
            body = body.replace(n,VARx.size(),Name);
        }

        if( body.find("After:") != string::npos ) {
            retval = line_number;
            body = "DISPLAY 'After:' ";
            body += Name;
            body += " *> ";
            body += Picture.empty() ? "<none>" : Picture.c_str();
            body += " ";
            body += Usage.empty() ? "<none>" : Usage.c_str();
        }

        n = body.find(value);
        if( n != string::npos ) {
            body = body.replace(n,value.size(),Value);
        }

        if( body.size() + 7 >= 72 ) {
            size_t n = body.find("VALUE");
            if(n EQ string::npos) {
                printf("This should never happen in GenerateCobolProgram()\n");
                fprintf(program,"%06d %s\n",line_number++,body.c_str());
            } else {
                string body1 = body.substr(0,n);
                string body2 = body.substr(n);
                fprintf(program,"%06d %s\n",line_number++,body1.c_str());
                fprintf(program,"%06d     %s\n",line_number++,body2.c_str());
            }
        } else {
            fprintf(program,"%06d %s\n",line_number++,body.c_str());
        }
    }

    fclose(program);

    ExecuteCommand("make -f CobolMakefile");
    return retval;
}

void ProcessInputLine(const char *input)
{
    VSTRING tokens = Split(Trim(input),"\t");
    int i = 0;
    string Name	       = tokens[i++];
    if(Name.empty()) {
        return;
    }
    string Picture	   = StripQuotes(tokens[i++]);
    string Usage	   = tokens[i++];
    string Value	   = tokens[i++];
    int Major	       = stoi(tokens[i++]);
    int Minor	       = stoi(tokens[i++]);

    int line_number = GenerateCobolProgram(Name,Picture,Usage,Value);
    GenerateGdbScript(line_number, Name, Major, Minor, Picture);
}

void ProcessIntermediateResults(FILE *results,FILE *errors,int &case_count)
{
    FILE *inputs;
    inputs = fopen(INTERMEDIATE_RESULTS,"r");
    if(inputs EQ NULL) {
        cout << "Couldn't open " << INTERMEDIATE_RESULTS << endl;
        exit(1);
    }
    FILE *script;
    script = fopen(GDB_SCRIPT,"r");
    if(script EQ NULL) {
        cout << "Couldn't open " << GDB_SCRIPT << endl;
        exit(1);
    }

    // We will walk through the script, and the results, in lockstep, so that
    // we can include the inputs alongside the outputs

    char ach[1024];
    string Name;
    string Picture;
    string Usage;
    string Before;
    string GdbAssign;
    string Sentry;
    string After;
    string GdbDisplay;
    while(fgets(ach,sizeof(ach),inputs)) {
        string input = FgetsRtrim(ach);

        if( input.substr(0,7) EQ "Before:" ) {
            Before = input.substr(7);
            // Skip the line we know is blank
            fgets(ach,sizeof(ach),inputs);
            continue;
        }

        size_t n = input.find("'After:'");
        if( n != string::npos ) {
            input = input.substr(n);
            VSTRING variable_info = Split(input," ");
            Name    = variable_info[1];
            Picture = variable_info[3];
            Usage   = variable_info[4];
            continue;
        }
        if( input.find("Continuing") != string::npos ) {
            continue;
        }

        if( input.find("Warning:") != string::npos ) {
            Sentry += "W";
            continue;
        }

        if( input.find("Error:") != string::npos ) {
            Sentry += "E";
            continue;
        }

        if( input.substr(0,6) EQ "After:" ) {
            After = input.substr(6);

            // Find the script line that generated this output:
            while(fgets(ach,sizeof(ach),script)) {
                // We should never break out of this loop because we run out
                // of data, but caution is always valuable.
                string script = FgetsRtrim(ach);
                VSTRING script_tokens = Split(script," =");
                if( script_tokens.size()>=3 AND script_tokens[0] EQ "p" ) {
                    GdbAssign = script_tokens[2];
                    break;
                }
            }

            // Note:  I want to be able to read the .tab file into Excel.
            // Excel has a serious flaw:  It's too helpful, and you can't
            // shut it the hell up.  So, if you feed it a COBOLish "+001.000",
            // even with the double quotes, Excel converts that to "0", which
            // isn't what we want.

            // There is an obscure fix for this, which is probably itself a
            // bug that got held over for legacy reasons.  By creating the
            // entry
            //              ="+001.000"
            //
            // Excel treats that as text. Another stackoverflow commenter
            // points out that there is an additional bug, at least in
            // Excel 2007, that causes screwball processing if there is
            // a comma in the text field.  Since we will be handling
            // PICTURE strings with commas, I am going with that more
            // complex construction:
            //
            //              "=""2008-10-03"""

            string mismatch = Before != GdbDisplay ? "***" : "" ;

            fprintf(results,"%s\t",Name.c_str());
            fprintf(results,"\"=\"\"%s\"\"\"\t",Picture.c_str());
            fprintf(results,"\"=\"\"%s\"\"\"\t",Usage.c_str());
            fprintf(results,"\"=\"\"%s\"\"\"\t",Before.c_str());
            fprintf(results,"\"=\"\"%s\"\"\"\t",GdbAssign.c_str());
            fprintf(results,"%s\t",Sentry.c_str());
            fprintf(results,"\"=\"\"%s\"\"\"\t",After.c_str());
            fprintf(results,"\"=\"\"%s\"\"\"\t",GdbDisplay.c_str());
            fprintf(results,"%s\n",mismatch.c_str());

            stringstream ss;

            if(Picture.empty()) {
                Picture = "<none>";
            }
            if(Usage.empty()) {
                Usage = "<none>";
            }

            ss <<        Name;
            ss << " " << Picture;
            ss << " " << Usage;
            ss << " " << Before;
            ss << " " << GdbAssign;
            ss << " " << Sentry;
            ss << " " << After;
            ss << " " << GdbDisplay;
            ss << " " << mismatch;
            printf("%s\n",ss.str().c_str());

            if(!mismatch.empty() ) {
                fprintf(errors,"%s\n",ss.str().c_str());
            }

            case_count += 1;

            Name.clear();
            Picture.clear();
            Usage.clear();
            Before.clear();
            GdbAssign.clear();
            Sentry.clear();
            GdbDisplay.clear();
            After.clear();
            mismatch.clear();
            continue;
        }


        // At this point, the line has to be the output of the cprint.py
        // assignment:

        if( input.substr(0,6) EQ "(gdb) " ) {
            input = input.substr(6);
        }
        GdbDisplay = input;
    }

    fclose(script);
    fclose(inputs);
}

void ProcessInputsAndOutputs()
{
    FILE *inputs;
    inputs = fopen(OUR_INPUT_FILE_NAME,"r");
    if(inputs EQ NULL) {
        cout << "Couldn't open " << OUR_INPUT_FILE_NAME << endl;
        exit(1);
    }
    FILE *results;
    results = fopen(RESULTS_FILE_NAME,"w");
    if( results EQ NULL ) {
        cout << "Couldn't open " << RESULTS_FILE_NAME << endl;
        exit(1);
    }
    FILE *errors = fopen(ERRORS_FILE_NAME,"w");
    if( errors EQ NULL ) {
        cout << "Couldn't open " << ERRORS_FILE_NAME << endl;
        exit(1);
    }

    char ach[1024];

    //The first line has column headers
    fgets(ach,sizeof(ach),inputs);

    fprintf(results,"Variable\tPicture\tUsage\tBefore\tAssign\tFlag\tCobolOut\tGdbOut\tMismatch\n");
    fprintf(errors, "Variable\tPicture\tUsage\tBefore\tAssign\tFlag\tCobolOut\tGdbOut\tMismatch\n");

    int case_count = 0;
    while( fgets(ach,sizeof(ach),inputs) ) {
        if( ach[0] != '\t' ) {
            continue;
        }
        ProcessInputLine(ach);
        ProcessIntermediateResults(results,errors,case_count);
    }
    fclose(inputs);
    fclose(results);

    printf("We processed %d individual cases\n",case_count);
}

int main()
{
    printf("selfcheck test case scripter version 0.7\n");
    srand(47);
    ProcessInputsAndOutputs();
    return 0;
}

