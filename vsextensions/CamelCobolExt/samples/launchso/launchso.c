#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <libcob.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include <libgen.h>
#include <fcntl.h>

#define LAUNCH_DEBUGGER 1
#define DONT_LAUNCH_DEBUGGER 0
#define IS_COBOL 1
#define ISNT_COBOL 0
#define FORK 1
#define DONT_FORK 0

static char debug_dir[PATH_MAX] = ".prim_debug";
static const char pid_filename[] = "pid_to_debug";
static const char gdb_filename[] = "gdb_launcher";
static const char vsc_filename[] = "vsc_launcher";

void ShowOurPid()
    {
    printf("Our PID is %d\n",getpid());
    }

char * const BuildFilename(const char *fname)
    {
    // Caller must free the malloc()ed pointer
    char * const retval = (char * const)malloc(PATH_MAX);

    strcpy(retval,getenv("HOME"));
    strcat(retval,"/");
    strcat(retval,debug_dir);
    strcat(retval,"/");
    if( mkdir(retval,0777) == -1 && errno != EEXIST )
        {
        printf("\x1B[31m");
        printf("   error number %d\n",errno);
        perror("   mkdir() "); 
        printf("\x1B[0m");
        exit(EXIT_FAILURE);
        }
    strcat(retval,fname);
    return retval;
    }

FILE *OpenOrFail(const char *filename)
    {    
    FILE *retval = fopen(filename,"w");
    if( !retval )
        {
        printf("\x1B[31m");
        printf("   error number %d\n",errno);
        perror("   fopen() "); 
        printf("\x1B[0m");
        exit(EXIT_FAILURE);
        }
    return retval;
    }

void CreatePidVariableFile(int pid,
                           const char *directory_of_library,
                           const char *breakpoint )
    {
    // Create a file that sets a couple of environment variables; we
    // will be needing them to launch a Visual Studio Code session in
    // attach mode.

    char * const filename = BuildFilename(pid_filename);
    FILE *f = OpenOrFail(filename);
    fprintf(f,"#! /bin/bash\n");
    fprintf(f,"export PRIM_SOLIB_TO_DEBUG=%s\n",directory_of_library);
    fprintf(f,"export PRIM_PID_TO_DEBUG=%d\n",pid);
    fprintf(f,"export PRIM_BREAKPOINT_TO_DEBUG=%s\n",breakpoint);
    fclose( f );
    chmod(filename,0775);
    free(filename);
    }

void CreateGdbLaunchFile(int pid,
                         const char *directory_of_library, 
                         const char *breakpoint)
    {
        {
        // Create a text file that will be fed to a gdb session.
        char * const filename = BuildFilename(gdb_filename);
        FILE *f = OpenOrFail(filename);
        fprintf(f,"set solib-search-path %s\n",directory_of_library);
        fprintf(f,"attach %d\n",pid);
        fprintf(f,"b %s\n",breakpoint);     // Set a breakpoint in the .so
        fprintf(f,"signal SIGCONT\n");      // Release the executable
        fclose( f );
        printf("Attach command-line debugger with `gdb -q -x %s`\n",filename);
        free(filename);
        }

        {
        // Create a FIFO that will get picked up by the gdb launcher
        const char *fifo_name = getenv("PRIM_DEBUG_PID");
        if( fifo_name )
            {
            // NOTE: this open() will block until somebody else opens the
            // FIFO for reading.
            printf("Or you can use the %s fifo to get the debug information\n",fifo_name);
            int fd = open(fifo_name, O_WRONLY);
            if( fd != -1 )
                {
                // Wipe out the final underscore; the debugger launch will
                // put it back.
                char appname[PATH_MAX];
                strcpy(appname,breakpoint);
                int n = strlen(appname);
                if( n )
                    {
                    appname[n-1] = '\0';
                    }
                dprintf(fd, "%d\t%s\n", getpid(), appname);
                close(fd);
                }
            }
        else
            {
            printf("There isn't any environment variable PRIM_DEBUG_PID.n");
            printf("There really should be.\n");
            exit(1);
            }
        }
    }

void *DlopenOrFail(const char *library_name, int flag)
    {
    dlerror();    /* Clear any existing error */
    void * handle = dlopen(library_name, RTLD_NOW);
    if (!handle)
        {
        fprintf(stderr, "%s\n", dlerror());
        exit(EXIT_FAILURE);
        }
    return handle;
    }

typedef void (*CHAR_RETURNING_VOID)(char *);

CHAR_RETURNING_VOID DlsymOrFailCharReturningVoid(void * handle, const char *entry_point)
    {
    CHAR_RETURNING_VOID CharReturningVoid;

    dlerror();    /* Clear any existing error */
    CharReturningVoid = (CHAR_RETURNING_VOID) dlsym(handle, entry_point);
    char * error = dlerror();
    if (error != NULL)
        {
        fprintf(stderr, "%s\n", error);
        exit(EXIT_FAILURE);
        }

    return CharReturningVoid;
    }

void LaunchSo(const char *library_name,
              const char *entry_point_,
              int is_cobol,
              int launch_debugger
              )
    {
    char path_to_library[PATH_MAX];
    char directory_of_library[PATH_MAX];
    char entry_point[128];
    char breakpoint[128];
    char Yes[1] = "Y";
    char No[1]  = "N";

    // Get the full canonical name of the library file
    realpath(library_name,path_to_library);

    // Get the directory of the library file
    strcpy(directory_of_library,path_to_library);
    strcpy(directory_of_library,dirname(directory_of_library));

    strcpy(entry_point,entry_point_);
    strcpy(breakpoint,entry_point_);

    if( is_cobol )
        {
        // Modify entry_point by replacing each hyphen with two underscores,
        // the same way that GnuCOBOL does

        char *d = entry_point;
        const char *s = entry_point_;
        for(;;)
            {
            char ch = *s++;
            if( ch == '-' )
                {
                ch = '_';
                *d++ = ch;
                }
            *d++ = ch;
            if( ch == '\0' )
                break;
            }
        strcpy(breakpoint,entry_point);

        // The global entry_point is actually a preamble in the cobc-generated
        // code.  The place that the user expects the trap to occur is the
        // target of a second call.  That call is to the same name with an 
        // appended underscore:
        strcat(breakpoint,"_");

        // If in module.so, the entry_point program calls another subprogram
        // in that same module, the COBOL run-time reports an error:
        //
        //      libcob: module.cbl:x: error: module 'subprogram' not found
        //
        // This is prevented with a cob_resolve() call.  Why this is necessary
        // is, as-yet, unknown.

        cob_resolve(entry_point);
        }

    if( !launch_debugger )
        {
        // This is just a straight load-and-execute
        void *handle;
        CHAR_RETURNING_VOID CharReturningVoid;

        handle = DlopenOrFail(library_name,RTLD_NOW);
        CharReturningVoid = DlsymOrFailCharReturningVoid(handle, entry_point);

        (*CharReturningVoid)(No);
        dlclose(handle);
        }
    else
        {
        // For debugging, we create some scripts that facilitate the attachment
        // of a debugging session, load the shared library, and execute a 
        // pause().  The debugger will send us a SIGCONT when it's ready
        
        void *handle;
        CHAR_RETURNING_VOID CharReturningVoid;
        int pid;

        // Get the process id.  
        pid = getpid();
        ShowOurPid();

        CreatePidVariableFile(pid,directory_of_library,breakpoint);
        CreateGdbLaunchFile(pid,directory_of_library,breakpoint);

        handle = DlopenOrFail(library_name, RTLD_NOW);

        // Get the actual entry point
        CharReturningVoid = DlsymOrFailCharReturningVoid(handle, entry_point);

        // Stop in your tracks until gdb executes a `signal SIGCONT`
        pause();

        // And now launch the routine in the dynamic library.
        printf("Calling %s(\"N\"):\n",breakpoint);
        (*CharReturningVoid)(No);
        printf("back from %s()\n",breakpoint);
        dlclose(handle);    // We're done with the handle
        }
    }

void HandleSigcont()
    {
    signal(SIGCONT,HandleSigcont);
    }

int main()
    {
    cob_init(0,NULL);

    signal(SIGCONT,HandleSigcont);

    const int test_cobol = 1;

    if( test_cobol )
        {
        printf("*********** without debugging ***********\n");
        LaunchSo("elbarco.so","elbarco",IS_COBOL,DONT_LAUNCH_DEBUGGER);
        printf("*********** with debugging ***********\n");
        LaunchSo("elbarco.so","elbarco",IS_COBOL,LAUNCH_DEBUGGER);
        }
    else
        {
        printf("*********** without debugging ***********\n");
        LaunchSo("libboat.so","RowYourBoat",ISNT_COBOL,DONT_LAUNCH_DEBUGGER);
        printf("*********** with debugging ***********\n");
        LaunchSo("libboat.so","RowYourBoat",ISNT_COBOL,LAUNCH_DEBUGGER);
        }

    cob_stop_run(0);
    }
