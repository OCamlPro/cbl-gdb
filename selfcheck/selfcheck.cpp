#include <iostream>
#include <math.h>

#include "params.h"
#include "utils.h"

#define EQ ==
#define AND &&
#define OR ||

using namespace std;

static const char PROGRAM_NAME[] = "program_under_test";
static const char PROGRAM_UNDER_TEST[] = "program_under_test.cbl";
static const char INPUT_FILE_NAME[] = "inputs.tab";
static const char RESULTS_FILE_NAME[] = "results.tab";
static const char GDB_SCRIPT[] = "gdb_script.txt";

static const char DEBUGGING_ROOT[] = "/home/bob/projects/cobol/cbl-gdb/selfcheck/";

void ExecuteCommand(const char *cmd)
{
#ifdef _WIN32
    string command = "ssh bob@doobie ";
    command += "cd \"";
    command += DEBUGGING_ROOT;
    command += "\";";
    command += cmd;
    system(command.c_str());
#else
    system(cmd);
#endif
}

char RandomDigit()
{
    char digit = '0' + (char)(10.0 *rand() / (RAND_MAX + 1.0));
    return digit;
}

void Scenario(FILE *script,const string &Name,const string &value)
{
    const char *pvalue = value.c_str();
    fprintf(script,"p %s=%s\n",Name.c_str(),pvalue);
    fprintf(script,"c\n");
}

void RandomScenario(FILE *script,const string &Name,int ldigits, int rdigits)
{
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

    if( rdigits) {
        while( (ch = RandomDigit()) EQ '0' ) {
            // Make sure first digit is non-zero
        }
        rsegment = ch;
        while( rsegment.length() < rdigits) {
            rsegment += RandomDigit();
        }
    }

    value = lsegment;
    value += '.';
    value += rsegment;

    Scenario(script,Name,value);
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
                       int random_iteration_count)
{
    FILE *script;
    script = fopen(GDB_SCRIPT,"w");
    if(script EQ NULL) {
        cout << "Couldn't open " << GDB_SCRIPT << endl;
        exit(1);
    }

    string output = "file ";
    output += PROGRAM_NAME;

    fprintf(script,"%s\n",output.c_str());
    fprintf(script,"b %d\n",line_number);
    fprintf(script,"r\n");

    // At this point, when the script runs, program_under_test will be
    // trapped just before the statement 'DISPLAY <name>'

    // We will start with some canned scenarios:
    // Minimum:
    Scenario(script,Name,"0");
    Scenario(script,Name,"0.");
    Scenario(script,Name,".0");

    // Stupid minimum:
    Scenario(script,Name,"1");
    Scenario(script,Name,"1.");
    Scenario(script,Name,"-1");
    Scenario(script,Name,"-1.");

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
    Scenario(script,Name,value);

    // Negative maximum:
    value = '-' + value;
    Scenario(script,Name,value);

    for(int l=0; l <= major_digits+1; l++) {
        for(int r=0; r <= minor_digits+1; r++) {
            RandomScenario(script,Name,l,r);
        }
    }

    fprintf(script,"q\n");
    fclose(script);

    // Having built the script, execute it:
    string command = "gdb <";
    command += GDB_SCRIPT;
    command += " >results.txt";
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

    /*

    IDENTIFICATION DIVISION.
    PROGRAM-ID. SELF-TEST.
    DATA DIVISION.
    WORKING-STORAGE SECTION.
    01 <name> PICTURE <picture> <usage> <value> .
    PROCEDURE DIVISION.
    PERFORM DISPLAY-VARIABLE 10 TIMES
    STOP RUN.
    DISPLAY-VARIABLE.
        DISPLAY <name>.
        EXIT PARAGRAPH.
    */

    int retval = -1;    // This is the line number of the trap
    int line_number = 1;
    string output;

    output = "IDENTIFICATION DIVISION.";
    fprintf(program,"%06d %s\n",line_number++,output.c_str());
    output = "PROGRAM-ID. SELF-TEST.";
    fprintf(program,"%06d %s\n",line_number++,output.c_str());
    output = "DATA DIVISION.";
    fprintf(program,"%06d %s\n",line_number++,output.c_str());
    output = "WORKING-STORAGE SECTION.";
    fprintf(program,"%06d %s\n",line_number++,output.c_str());
    output = "01 ";
    output += Name;
    if( !Picture.empty() ) {
        output += " PICTURE ";
        output += Picture;
    }
    if( !Usage.empty() ) {
        output += " ";
        output += Usage;
    }
    if( !Value.empty() ) {
        output += " VALUE ";
        output += Value;
    }
    output += ".";
    fprintf(program,"%06d %s\n",line_number++,output.c_str());
    output = "PROCEDURE DIVISION.";
    fprintf(program,"%06d %s\n",line_number++,output.c_str());
    output = "PERFORM DISPLAY-VARIABLE 999999999 TIMES.";
    fprintf(program,"%06d %s\n",line_number++,output.c_str());
    output = "STOP RUN.";
    fprintf(program,"%06d %s\n",line_number++,output.c_str());
    output = "DISPLAY-VARIABLE.";
    fprintf(program,"%06d %s\n",line_number++,output.c_str());

    // This is the money shot; this is the reason the whole thing exists!
    retval = line_number;
    output = "DISPLAY ";
    output += Name;
    fprintf(program,"%06d %s\n",line_number++,output.c_str());

    // If a value was specified, re-initialize our variable inside the loop
    if( !Value.empty() ) {
        output = "MOVE ";
        output += Value;
        output += " TO ";
        output += Name;
        fprintf(program,"%06d %s\n",line_number++,output.c_str());
    }

    output = "EXIT PARAGRAPH.";
    fprintf(program,"%06d %s\n",line_number++,output.c_str());

    fclose(program);

    ExecuteCommand("make -f CobolMakefile");
    return retval;
}

void ProcessInputLine(const char *input,FILE *results)
{
    VSTRING tokens = Split(Trim(input),"\t");
    int i = 0;
    string Name	       = tokens[i++];
    string Picture	   = tokens[i++];
    string Usage	   = tokens[i++];
    string Value	   = tokens[i++];
    int Major	       = stoi(tokens[i++]);
    int Minor	       = stoi(tokens[i++]);
    int Iterations     = stoi(tokens[i++]);

    int line_number = GenerateCobolProgram(Name,Picture,Usage,Value);
    GenerateGdbScript(line_number, Name, Major, Minor, Iterations);
}

void ProcessInputsAndOutputs()
{
    FILE *inputs;
    inputs = fopen(INPUT_FILE_NAME,"r");
    if(inputs EQ NULL) {
        cout << "Couldn't open " << INPUT_FILE_NAME << endl;
        exit(1);
    }
    FILE *results;
    results = fopen(RESULTS_FILE_NAME,"w");
    if( results EQ NULL ) {
        cout << "Couldn't open " << RESULTS_FILE_NAME << endl;
        exit(1);
    }
    char ach[1024];

    //The first line has column headers
    fgets(ach,sizeof(ach),inputs);
    fputs(ach,results);

    while( fgets(ach,sizeof(ach),inputs) ) {
        ProcessInputLine(ach,results);
    }
    fclose(inputs);
    fclose(results);
}

int main()
{
    srand(47);
    ProcessInputsAndOutputs();
    return 0;
}

