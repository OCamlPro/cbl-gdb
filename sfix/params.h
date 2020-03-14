/* #######################################################################
# Copyright (c) 2019-2020 Symas Corporation
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in
#      the documentation and/or other materials provided with the
#      distribution.
#    * Neither the name of the Symas Corporation nor the names of its
#      contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
############################################################################ */

#pragma once

#include <string>
#include <fstream>
#include <vector>

#define VERSION "1.1.3"

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
