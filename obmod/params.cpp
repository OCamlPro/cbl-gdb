#include <iostream>
#include <regex>
#include "params.h"
#include "utils.h"
#ifdef _WIN32
#include "getopt.h"
#include <io.h>
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#include <dirent.h>
#endif

#define EQ ==
#define AND &&
#define OR ||

using namespace std;

void FILENAME::Decode(const string &filename_)
{
    drive.clear();
    path.clear();
    fname.clear();
    ext.clear();

    // This strangely complex code is handling both Windows C:\path\fname.ext
    // Linux /this/and.that possibilities.

    // I also handle the Windows \\server\share Universal Naming Convention possibility,
    // more fool I.

    char cwd_[512];
    string default_path;
    string filename = filename_;
    char delimiter = '/';

#ifdef _WIN32
    string cwd = getcwd(cwd_,sizeof(cwd_));
    if(cwd.size() > 0 AND cwd[cwd.size()-1] != '\\') {
        cwd += '\\';
    }
    drive = cwd.substr(0,2);
    default_path = Replace(cwd.substr(2),'\\',delimiter);
    if(filename_.size() >= 2) {
        if(filename_.substr(0,2) EQ "\\\\") {
            //This is the UNC \\server case:
            size_t nslash = filename_.find('\\',2);
            if(nslash != string::npos) {
                drive = filename_.substr(0,nslash);
                default_path = '\\';
                filename = filename_.substr(nslash+1);
            } else {
                // There is nothing but a server name...maybe
                drive = filename_;
                default_path = '\\';
                filename.clear();
            }
        } else {
            if(filename_[1] EQ ':') {
                // He is specifying a drive.  Make sure we are on the same page
                // for any necessary default path:
                int starting_drive = _getdrive();
                if(_chdrive(toupper(filename_[0]) - 'A' + 1) EQ 0) {
                    default_path = getcwd(cwd_,sizeof(cwd_));
                    drive = default_path.substr(0,2);
                    default_path = default_path.substr(2);
                } else {
                    // He specified a bum drive letter.  He'll get what he deserves:
                    // set the default to just that letter, and let the chips fall
                    // where they may
                    drive = filename_.substr(0,2);
                    default_path = '/';
                }
                _chdrive(starting_drive);
                filename = filename.substr(2);
            }
        }
    }
#else
    // The Linux world will not need a drive:
    drive.clear();
    default_path = getcwd(cwd_,sizeof(cwd_));
    if(default_path.size() >=1 AND default_path[default_path.size()-1] != delimiter) {
        default_path += delimiter;
    }
#endif
    // We now have a drive (which might be empty), and
    // a default path for that drive, in case we need one.  Let's continue:

    // Convert backslashes to frontslashes in the file name
    filename = Replace(filename,'\\',delimiter);

    if(filename.size() EQ 0 OR filename.size() >= 1 AND filename[0] != delimiter) {
        // The filename doesn't start with a slash, so it is a relative path.
        // Incorporate the default path:
        filename = default_path + filename;
    }

    size_t nfirst_slash = filename.find_first_of(delimiter);
    size_t nlast_slash = filename.find_last_of(delimiter);
    if(nfirst_slash != string::npos) {
        nlast_slash += 1;

        // The path will be delimited, front and back, by slashes.  For
        // the root, it'll just be the single slash:
        path = filename.substr(nfirst_slash,nlast_slash-nfirst_slash);

        size_t nextension_dot = filename.find_last_of('.');
        if(nextension_dot != string::npos AND nextension_dot >= nlast_slash) {
            fname = filename.substr(nlast_slash,nextension_dot-nlast_slash);
            ext = filename.substr(nextension_dot);
        } else {
            fname = filename.substr(nlast_slash);
        }
    }
}

#ifdef _DEBUG
void DecodeTest()
{
    vector<string>tests = {
        "",
        ".",
        "/",
        "/.",
        "/.ext",
        "fname",
        "fname.",
        "fname.ext",
        "path/fname",
        "path/fname.",
        "path/fname.ext",
        "path/path2/fname",
        "path/path2/fname.",
        "path/path2/fname.ext",
        "/path/fname",
        "/path/fname.",
        "/path/fname.ext",
        "/path/path2/fname",
        "/path/path2/fname.",
        "/path/path2/fname.ext",
        "d:fname",
        "d:fname.",
        "d:name.ext",
        "d:path/fname",
        "d:path/fname.",
        "d:path/fname.ext",
    };

    FILENAME fn;
    for(vector<string>::const_iterator it=tests.begin(); it!=tests.end(); it++) {
        fn.Decode(*it);
        cout << "\"" << *it      << "\"" << " ==> ";
        cout << "\"" << fn.drive << "\"" << " + ";
        cout << "\"" << fn.path  << "\"" << " + ";
        cout << "\"" << fn.fname << "\"" << " + ";
        cout << "\"" << fn.ext   << "\"" << " ==> ";
        cout << "\"" << fn.WholePath()   << "\"" << "  ";
        cout << endl;
    }
}
#endif

static void Usage()
{
    cerr << "obmod version " VERSION "\n";
    cerr << "\n";
    cerr << "usage: obmod -q -V -i input.o -o output.o -t cobst.tab\n";
    cerr << "\n";
    cerr << "Modifies a cobc+gcc-generated .o ELF file with cobst-generated information.\n";
    cerr << "   -i <path to the .o input file\n";
    cerr << "   -o <path to the .o output file\n";
    cerr << "   -t <path to the cobst-generated symbol table information\n";
    cerr << "   -q quiet (suppresses version display on entry)\n";
    cerr << "   -v displays version information and exits\n";
}

static void Version()
{
    cerr << "obmod version " VERSION "\n";
    exit(1);
}

void OpenOrFail(std::ifstream &ifs,const std::string &fname,std::ios_base::openmode mode/* = std::ifstream::in*/)
{
    ifs.open(fname,mode);
    if(!ifs.is_open()) {
        cerr << "Failed to open input file \"" << fname << "\"" << endl;
        exit(1);
    }
}

void OpenOrFail(std::ofstream &ofs,const std::string &fname,std::ios_base::openmode mode/* = std::ofstream::out*/)
{
    ofs.open(fname,mode);
    if(!ofs.is_open()) {
        cerr << "Failed to open output file \"" << fname << "\"" << endl;
        exit(1);
    }
}

PARAMETERS GetParameters(int argc, char *argv[])
{
    if(argc < 2) {
        Usage();
        exit(1);
    }

    PARAMETERS retval;

    string o_input_parameter;
    string o_output_parameter;
    string cobst_parameter;
    int c;
    retval.quiet = false;
    while ((c = getopt (argc, argv, "i:o:t:vq")) != -1) {
        switch (c) {
        case 'v':
            Version();
            break;
        case 'i':
            o_input_parameter = optarg;
            break;
        case 'o':
            o_output_parameter = optarg;
            break;
        case 't':
            cobst_parameter = optarg;
            break;
        case 'q':
            retval.quiet = true;
            break;

        case '?':
            fprintf (stderr, "Unknown option '-%c'.\n", optopt);
            exit(1);
            break;
        default:
            fprintf (stderr, "Default option?  How did *that* happen?\n");
            exit(1);
            break;
        }
    }

    retval.o_input_filename.Decode(o_input_parameter);
    retval.o_output_filename.Decode(o_output_parameter);
    retval.cobst_filename.Decode(cobst_parameter);

    return retval;
}

vector<string>DirectoryScan(const string &path,const string &fname,const string &wildcard)
{
    vector<string>retval;
#ifdef _WIN32
    _finddata_t c_file;
    intptr_t hFile;

    string find_me = path + fname + wildcard;
    hFile = _findfirst( find_me.c_str(), &c_file );
    if(hFile != -1L) {
        do {
            string found = path;
            found += c_file.name;
            retval.push_back(found);
        } while( _findnext( hFile, &c_file ) == 0 );

        _findclose(hFile);
    }
#else
    DIR* dirp = opendir(path.c_str());
    struct dirent * dp;
    while ((dp = readdir(dirp)) != NULL) {
        string f = dp->d_name;

        size_t nlength = f.size();
        if( nlength > 2 AND f[nlength-2] EQ '.' AND f[nlength-1] EQ 'h' ) {
            string full_name = path + f;
            retval.push_back(full_name);
        }
    }
    closedir(dirp);

#endif
    return retval;
}
