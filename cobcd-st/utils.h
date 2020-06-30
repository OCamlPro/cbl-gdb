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

#if __GNUC__ >= 7
#include <cstdint>
#endif

#include <string>
#include <vector>

typedef char *PCHAR;

#if __GNUC__ >= 7
typedef  int8_t  BYTE;
typedef uint8_t UBYTE;

typedef  int16_t  SHORT;
typedef uint16_t USHORT;

typedef  int32_t  LONG;
typedef uint32_t ULONG;

typedef  int64_t  LONGLONG;
typedef uint64_t ULONGLONG;
#else
    
typedef   signed char  BYTE;
typedef unsigned char UBYTE;

typedef   signed short  SHORT;
typedef unsigned short USHORT;

typedef   signed int  LONG;
typedef unsigned int ULONG;

typedef   signed long long  LONGLONG;
typedef unsigned long long ULONGLONG;

#endif


typedef std::vector<std::string>VSTRING;

std::string Trim(const std::string &s);
std::string RTrim(const std::string &s);
std::string StripQuotes(const std::string &s);
std::vector<std::string>Split(const std::string &s,const std::string &delimiters);
std::string Replace(const std::string &s,const char was, const char is);
std::string Upper(const std::string&s);
std::string ZeroIsNull(const LONGLONG &v);