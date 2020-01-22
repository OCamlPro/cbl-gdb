#pragma once

#include <string>
#include <fstream>
#include <vector>

#define VERSION "0.10.1"

class FILENAME
{
private:
    std::string drive;   // Empty in a Linux system; "C:" on Windows
    std::string path;    // Starts and ends with a /; might be just the one character if the file is in the root
    std::string fname;   // just the file's base name
    std::string ext;     // starts with a '.' if it was present

public:
    void Decode(const std::string &filename);
    std::string Stem() const
    {
        std::string retval = drive;
        retval += path;
        retval += fname;
        return retval;
    }
    void SetExtension(const std::string &ex)
    {
        if(ex.size() >= 1 && ex[0] != '.') {
            ext = '.';
            ext += ex;
        } else {
            ext = ex;
        }
    }
    std::string GetFname() const
    {
        return fname;
    }
    std::string GetExt() const
    {
        return ext;
    }

    std::string GetFandExt() const
    {
        std::string retval;
        retval = fname + ext;

        return retval;
    }

    std::string WholePath() const
    {
        return drive + path + fname + ext;
    }

    std::string PathToRoot() const
    {
        return drive + path + fname;
    }

    std::string Path() const
    {
        return drive + path;
    }

    friend void DecodeTest();

};

class PARAMETERS
{
public:
    FILENAME    s_input_filename;   // .s input file name
    FILENAME    s_output_filename;  // .s output file name
    std::string c_filename;       // Replace this name...
    std::string cbl_filename;         // ...with this one
    bool        quiet;
};

std::vector<std::string> DirectoryScan(const std::string &path,
                                       const std::string &fname,
                                       const std::string &ext);
void OpenOrFail(std::ifstream &ifs,const std::string &fname,
                std::ios_base::openmode mode = std::ifstream::in);
void OpenOrFail(std::ofstream &ofs,const std::string &fname,
                std::ios_base::openmode mode = std::ofstream::out);
PARAMETERS GetParameters(int argc, char *argv[]);
