/* #######################################################################
# Copyright (c) 2019-2024 COBOLworx Corporation
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
#    * Neither the name of the COBOLworx Corporation nor the names of its
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

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <string.h>

#ifndef EQ
#define EQ ==
#define AND &&
#define OR ||
#define NEEDED_TO_DEFINE_EQ
#endif

class COB_FIELDS;

class COB_FIELD
{
public:
    std::string  f_name;       /* name of field         */
    std::string  b_name;       /* name of data field    */
    int          offset;       /* any internal + offset */
    int          size;         /* Field size            */
    std::string  a_name;       /* name of attribute     */

public:
    COB_FIELD(std::string f_name_,
              std::string b_name_,
              int         offset_,
              int         size_,
              std::string a_name_)
    {
        f_name       = f_name_     ;
        b_name       = b_name_     ;
        offset       = offset_     ;
        size         = size_       ;
        a_name       = a_name_     ;
    }

    friend COB_FIELDS;
} ;
typedef std::vector<COB_FIELD> V_COB_FIELDS_2;
typedef std::map<std::string, size_t> M_COB_FIELDS_2;
typedef std::map<std::string, size_t> M_COB_DATA;

class COB_ATTR
{
public:
    std::string     a_name;     /* name of field         */
    unsigned short  type;       /* Field type [TODO GC4: enum] */
    unsigned short  digits;     /* Digit count */
    signed short    scale;      /* Field scale */
    unsigned short  flags;      /* Field flags */
    const char     *pic;        /* Pointer to picture string */

public:
    COB_ATTR (std::string     a_name_,
              unsigned short  type_,  
              unsigned short  digits_,
              signed short    scale_, 
              unsigned short  flags_, 
              const char     *pic_)
    {
    a_name       = a_name_  ;
    type         = type_    ;
    digits       = digits_  ;
    scale        = scale_   ;
    flags        = flags_   ;
    pic          = pic_     ;
    }

    friend COB_FIELDS;
} ;
typedef std::vector<COB_ATTR> V_COB_ATTR_2;
typedef std::map<std::string, size_t> M_COB_ATTR_2;


class COB_FIELDS
{
private:
    M_COB_FIELDS_2  m_cob_fields;
    V_COB_FIELDS_2  v_cob_fields;
    M_COB_DATA      m_cob_data;
    V_COB_ATTR_2    v_cob_attr;
    M_COB_ATTR_2    m_cob_attr;

private:
    void ScanDotHFile(std::ifstream &ifs, const std::string &source);
    void Insert(std::string f_name,
                std::string b_name,
                int         offset,
                int         size,
                std::string a_name);
    void InsertAttr(COB_ATTR &cob_attr);
public:
    void ScanAllDotHFiles(const std::string &path, const std::string &fname);
    void ScanCFile(const std::string &path, const std::string &fname);

    COB_FIELD *Find(const std::string &f_name) ;
    COB_ATTR  *FindAttr(const std::string &a_name) ;
    size_t     GetBSize(const std::string b_name) const;
    void Dump() const ;
};

#ifdef NEEDED_TO_DEFINE_EQ
#undef EQ
#undef AND
#undef OR
#endif
