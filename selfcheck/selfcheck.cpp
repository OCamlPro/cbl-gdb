#include <iostream>

#include "params.h"
#include "utils.h"

#define EQ ==
#define AND &&
#define OR ||

using namespace std;

static const char INPUT_FILE_NAME[] = "inputs.tab";
static const char RESULTS_FILE_NAME[] = "results.tab";
static const char PROGRAM_UNDER_TEST[] = "program_under_test.cbl";
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
    cout << command << endl;
    system(command.c_str());
    printf("Done!\n");
#else

#endif
    }

int GenerateCobolProgram(const string &Name,
                          const string &Picture,
                          const string &Usage,
                          const string &Value)
    {
    FILE *program;
    program = fopen(PROGRAM_UNDER_TEST,"w");
    if(program EQ NULL)
        {
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
    if( !Picture.empty() )
        {
        output += " PICTURE ";
        output += Picture;
        }
    if( !Usage.empty() )
        {
        output += " ";
        output += Usage;
        }
    if( !Value.empty() )
        {
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
    output = "EXIT PARAGRAPH.";
    fprintf(program,"%06d %s\n",line_number++,output.c_str());

    fclose(program);

    ExecuteCommand("make -f CobolMakefile");


    return line_number;
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

    GenerateCobolProgram(Name,Picture,Usage,Value);



    }

void ProcessInputsAndOutputs()
    {
    FILE *inputs;
    inputs = fopen(INPUT_FILE_NAME,"r");
    if(inputs EQ NULL)
        {
        cout << "Couldn't open " << INPUT_FILE_NAME << endl;
        exit(1);
        }
    FILE *results;
    results = fopen(RESULTS_FILE_NAME,"w");
    if( results EQ NULL )
        {
        cout << "Couldn't open " << RESULTS_FILE_NAME << endl;
        exit(1);
        }
    char ach[1024];

    //The first line has column headers
    fgets(ach,sizeof(ach),inputs);
    fputs(ach,results);

    while( fgets(ach,sizeof(ach),inputs) )
        {
        ProcessInputLine(ach,results);
        }
    fclose(inputs);
    fclose(results);
    }

int main()
    {
    ProcessInputsAndOutputs();
    return 0;
    }

