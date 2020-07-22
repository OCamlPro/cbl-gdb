/* #######################################################################
# Copyright (c) 2019-2020 COBOLworx Corporation
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

#include <stdlib.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "profiler.h"
#include "utils.h"
#include "vartree.h"

#ifndef EQ
#define EQ ==
#define AND &&
#define OR ||
#define NEEDED_TO_DEFINE_EQ
#endif

#if __GNUC__ < 7
#define nullptr NULL
#endif


//////////////////////////////////////////////////////////////////////////
// These data structures are based on from GnuCOBOL3.1-dev/libcob/common.h
//////////////////////////////////////////////////////////////////////////

/*  It should be noted that we have two, or perhaps three, sources of
    of data size.  There is the summary in source.cbl.list, there is
    the size in COB_DATA, and there is the size field in COB_FIELD.

    I've seen cases where they don't quite match.  There will be code to
    check that, but wariness is indicated.  RJD, 2020-05-12
    */

class COB_PIC_SYMBOLS;
class COB_PIC_SYMBOL
{
public:
    std::string         name;       /* name of pic_symbol */
    std::string         pic_symbol;
public:
    COB_PIC_SYMBOL(const std::string &n, const std::string &p)
    {
        name = n;
        pic_symbol = p;
    }
    friend COB_PIC_SYMBOLS;
};
typedef std::vector<COB_PIC_SYMBOL> V_COB_PIC_SYMBOLS;
typedef std::map<std::string,size_t> M_COB_PIC_SYMBOLS;

class COB_PIC_SYMBOLS
{
private:
    V_COB_PIC_SYMBOLS v_pic_symbols;
    M_COB_PIC_SYMBOLS m_pic_symbols;

public:
    COB_PIC_SYMBOLS()
    {
    }
    void
    Insert(const std::string &name, const std::string &pic_symbol)
    {
        M_COB_PIC_SYMBOLS::const_iterator it = m_pic_symbols.find(name);
        if( it != m_pic_symbols.end() ) {
            std::cout << "Creating the symbol " << name << " more than once in COB_PIC_SYMBOLS::Insert()\n" ;
            exit(1);
        }

        COB_PIC_SYMBOL cps(name, pic_symbol);
        m_pic_symbols[name] = v_pic_symbols.size();
        v_pic_symbols.push_back(cps);
    }
    void
    Dump() const
    {
        std::cout << "COB_PIC_SYMBOLS:" << std::endl;
        std::cout << std::setw(20) << "name" ;
        std::cout << " ";
        std::cout << std::setw(20) << "pic_symbol";
        std::cout << std::endl;

        for( V_COB_PIC_SYMBOLS::const_iterator it = v_pic_symbols.begin();
                it != v_pic_symbols.end();
                it++) {
            const COB_PIC_SYMBOL symbol = *it;
            std::cout << std::setw(20) << symbol.name ;
            std::cout << " ";
            std::cout << std::setw(20) << symbol.pic_symbol;
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
    const COB_PIC_SYMBOL *
    GetPicSymbol(std::string const &pic_name) const
    {
        const COB_PIC_SYMBOL *retval = nullptr;
        M_COB_PIC_SYMBOLS::const_iterator it = m_pic_symbols.find(pic_name);
        if( it != m_pic_symbols.end() ) {
            retval = &v_pic_symbols[it->second];
        }
        return retval;
    }
};

class COB_DATA;
class COB_DATUM
{
public:
    std::string         name;         /* name of data field */
    int                 size;         /* size of the field  */
    std::string         program_id;   /* From a header comment*/
    std::string         storage_type; /* From .h file structure*/
    std::string         cbl_name;     /* From the comment in the .h code */

public:
    COB_DATUM(const std::string &name_,
              int size_,
              const std::string &program_id_,
              const std::string &storage_,
              const std::string &cbl_name_)
    {
        name = name_;
        size = size_;
        program_id = program_id_;
        storage_type = storage_;
        cbl_name = cbl_name_;
    }
    friend COB_DATA;
} ;
typedef std::vector<COB_DATUM> V_COB_DATUM;
typedef std::map<std::string,size_t> M_COB_DATUM;

class COB_DATA
{
public:
    V_COB_DATUM v_data; // The list of elements
    M_COB_DATUM m_data_from_b_field;
    M_COB_DATUM m_data_from_cbl_name;

public:
    COB_DATA()
    {
    }
    void
    Insert(const std::string &name,
           int size,
           const std::string &program_id,
           const std::string &storage,
           const std::string &cbl_name)
    {
        M_COB_DATUM::const_iterator it = m_data_from_b_field.find(name);
        if( it != m_data_from_b_field.end() ) {
            std::cout << "Creating the symbol " << name << " more than once in COB_DATA::Insert()\n" ;
            exit(1);
        }

        COB_DATUM cd(name, size, program_id, storage, cbl_name);
        m_data_from_b_field[name] = v_data.size();

        std::string cobol_name = program_id;
        cobol_name += "|";
        cobol_name += cbl_name;
        m_data_from_cbl_name[cobol_name] = v_data.size();

        v_data.push_back(cd);
    }

    void
    Dump() const
    {
        std::cout << "COB_DATA:" << std::endl;
        std::cout << std::setw(20) << "name" ;
        std::cout << std::setw(20) << "storage";
        std::cout << std::setw(20) << "size";
        std::cout << std::setw(20) << "program_id";
        std::cout << std::setw(20) << "cbl_name";
        std::cout << std::endl;


        for( V_COB_DATUM::const_iterator it=v_data.begin();
                it!= v_data.end();
                it++) {
            COB_DATUM symbol = *it;
            std::cout << std::setw(20) << symbol.name ;
            std::cout << std::setw(20) << symbol.storage_type;
            std::cout << std::setw(20) << symbol.size;
            std::cout << std::setw(20) << symbol.program_id;
            std::cout << std::setw(20) << symbol.cbl_name;
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    const COB_DATUM *
    GetCobDatum(const std::string &b_name) const
    {
        const COB_DATUM *retval = nullptr;
        M_COB_DATUM::const_iterator it = m_data_from_b_field.find(b_name);
        if( it != m_data_from_b_field.end() ) {
            retval = &v_data[it->second];
        }

        return retval;
    }

    const COB_DATUM *
    GetCobDatumFromProgAndName(const std::string &prog,
                               const std::string &name) const
    {
        std::string cobol_name = prog;
        cobol_name += "|";
        cobol_name += name;

        const COB_DATUM *retval = nullptr;
        M_COB_DATUM::const_iterator it = m_data_from_cbl_name.find(cobol_name);
        if( it != m_data_from_cbl_name.end() ) {
            retval = &v_data[it->second];
        }

        return retval;
    }


};

class COB_FIELD_ATTRIBUTES;
class COB_FIELD_ATTRIBUTE
{
public:
    std::string         a_name;     /* name of attribute */
    unsigned short      type;       /* Field type        */
    unsigned short      digits;     /* Digit count       */
    signed short        scale;      /* Field scale       */
    unsigned short      flags;      /* Field flags       */
    std::string         p_name;     /* picture string    */
    std::string         program_id; /* picture string    */

public:
    COB_FIELD_ATTRIBUTE(std::string         name_,
                        unsigned short      type_,
                        unsigned short      digits_,
                        signed short        scale_,
                        unsigned short      flags_,
                        std::string         name_of_pic_,
                        std::string         program_id_)
    {
        a_name      = name_          ;
        type        = type_          ;
        digits      = digits_        ;
        scale       = scale_         ;
        flags       = flags_         ;
        p_name      = name_of_pic_   ;
        program_id  = program_id_    ;
    }
    friend COB_FIELD_ATTRIBUTES;
} ;
typedef std::vector<COB_FIELD_ATTRIBUTE> V_COB_FIELD_ATTRIBUTE;
typedef std::map<std::string,size_t> M_COB_FIELD_ATTRIBUTES;

class COB_FIELD_ATTRIBUTES
{
public:
    V_COB_FIELD_ATTRIBUTE  v_cob_field_attributes;
    M_COB_FIELD_ATTRIBUTES m_cob_field_attributes;

public:
    COB_FIELD_ATTRIBUTES()
    {
    }
    void
    Insert(std::string         a_name,     /* name of attribute */
           unsigned short      type,       /* Field type        */
           unsigned short      digits,     /* Digit count       */
           signed short        scale,      /* Field scale       */
           unsigned short      flags,      /* Field flags       */
           std::string         p_name,     /* picture string    */
           std::string         program_id) /* picture string    */
    {
        M_COB_FIELD_ATTRIBUTES::const_iterator it = m_cob_field_attributes.find(a_name);
        if( it != m_cob_field_attributes.end() ) {
            std::cout << "Creating the symbol " << a_name << " more than once in COB_FIELD_ATTRIBUTES::Insert()\n" ;
            exit(1);
        }

        COB_FIELD_ATTRIBUTE cfa(a_name,
                                type,
                                digits,
                                scale,
                                flags,
                                p_name,
                                program_id);
        m_cob_field_attributes[a_name] = v_cob_field_attributes.size();
        v_cob_field_attributes.push_back(cfa);
    }

    void
    Dump() const
    {
        std::cout << "COB_FIELD_ATTRIBUTES:" << std::endl;
        std::cout << std::setw(20) << "a_name" ;
        std::cout << std::setw(20) << "type";
        std::cout << std::setw(20) << "digits";
        std::cout << std::setw(20) << "scale" ;
        std::cout << std::setw(20) << "flags";
        std::cout << std::setw(20) << "p_name";
        std::cout << std::setw(20) << "program_id";
        std::cout << std::endl;
        for(V_COB_FIELD_ATTRIBUTE::const_iterator it=v_cob_field_attributes.begin();
                it!=v_cob_field_attributes.end();
                it++) {
            COB_FIELD_ATTRIBUTE attr = *it;
            std::cout << std::setw(20) << attr.a_name ;
            std::cout << std::setw(20) << attr.type;
            std::cout << std::setw(20) << attr.digits;
            std::cout << std::setw(20) << attr.scale ;
            std::cout << std::setw(20) << attr.flags;
            std::cout << std::setw(20) << attr.p_name;
            std::cout << std::setw(20) << attr.program_id;
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    const COB_FIELD_ATTRIBUTE *
    GetFieldAttribute(const std::string &a_name) const
    {
        const COB_FIELD_ATTRIBUTE *retval = nullptr;
        M_COB_FIELD_ATTRIBUTES::const_iterator it = m_cob_field_attributes.find(a_name);
        if( it != m_cob_field_attributes.end() ) {
            retval = &v_cob_field_attributes[it->second];
        }

        return retval;
    }
};

class COB_FIELDS;
class COB_FIELD
{
public:
    std::string  f_name;       /* name of field         */
    std::string  b_name;       /* name of data field    */
    int          offset;       /* any internal + offset */
    int          size;         /* Field size            */
    std::string  a_name;       /* name of attribute     */
    std::string  program_id;   /* Where defined         */
    std::string  cbl_name;     /* From the comment in the .h code */

public:
    COB_FIELD(std::string f_name_,
              std::string b_name_,
              int         offset_,
              int         size_,
              std::string a_name_,
              std::string program_id_,
              std::string cbl_name_)
    {
        f_name       = f_name_      ;
        b_name       = b_name_      ;
        offset       = offset_    ;
        size         = size_      ;
        a_name       = a_name_      ;
        program_id   = program_id_;
        cbl_name     = cbl_name_  ;
    }

    friend COB_FIELDS;
} ;
typedef std::vector<COB_FIELD> V_COB_FIELDS;
typedef std::map<std::string,size_t> M_COB_FIELDS;

class COB_FIELDS
{
private:
    M_COB_FIELDS m_cob_fields;

public:
    V_COB_FIELDS v_cob_fields;

public:
    void
    Insert(std::string f_name,
           std::string b_name,
           int         offset,
           int         size,
           std::string a_name,
           std::string program_id,
           std::string cbl_name)
    {
        M_COB_FIELDS::const_iterator it = m_cob_fields.find(f_name);
        if( it != m_cob_fields.end() ) {
            std::cout << "Creating the symbol " << f_name << " more than once in COB_FIELDS::Insert()\n" ;
            exit(1);
        }
        COB_FIELD cf(f_name,
                     b_name,
                     offset,
                     size,
                     a_name,
                     program_id,
                     cbl_name);
        m_cob_fields[f_name] = v_cob_fields.size();
        v_cob_fields.push_back(cf);
    }

    void
    Modify(std::string f_name_,
           std::string b_name_,
           int         offset_)
    {
        M_COB_FIELDS::iterator it = m_cob_fields.find(f_name_);
        if( it EQ m_cob_fields.end() ) {
            std::cout << "Couldn't find symbol " << f_name_ << " COB_FIELDS::Modify()\n" ;
            exit(1);
        }
        size_t i = it->second;
        v_cob_fields[i].b_name = b_name_;
        v_cob_fields[i].offset = offset_;
    }

    void
    Dump() const
    {
        std::cout << "COB_FIELDS:" << std::endl;
        std::cout << std::setw(20) << "f_name" ;
        std::cout << std::setw(20) << "b_name";
        std::cout << std::setw(20) << "offset" ;
        std::cout << std::setw(20) << "size";
        std::cout << std::setw(20) << "a_name";
        std::cout << std::setw(20) << "program_id";
        std::cout << std::setw(20) << "cbl_name";
        std::cout << std::endl;

        for( V_COB_FIELDS::const_iterator it = v_cob_fields.begin();
                it!=v_cob_fields.end();
                it++) {
            COB_FIELD field = *it;
            std::cout << std::setw(20) << field.f_name ;
            std::cout << std::setw(20) << field.b_name;
            std::cout << std::setw(20) << field.offset ;
            std::cout << std::setw(20) << field.size;
            std::cout << std::setw(20) << field.a_name;
            std::cout << std::setw(20) << field.program_id;
            std::cout << std::setw(20) << field.cbl_name;
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
};

class COB_PROGRAM_LABELS;
class COB_PROGRAM_LABEL
{
private:
    char type;
    // P for PROGRAM-ID
    // E for ENTRY
    // S for SECTION
    // G for PARAGRAPH
    std::string source_file;
    int line_number;
    int sentence_number;
    std::string program;
    std::string entry;
    std::string section;
    std::string paragraph;

public:
    COB_PROGRAM_LABEL(char type_,
                      const std::string &source_file_,
                      int line_number_,
                      int sentence_number_,
                      const std::string &program_,
                      const std::string &entry_,
                      const std::string &section_,
                      const std::string &paragraph_)
    {
        type            = type_           ;
        source_file     = source_file_    ;
        line_number     = line_number_    ;
        sentence_number = sentence_number_;
        program         = program_        ;
        entry           = entry_          ;
        section         = section_        ;
        paragraph       = paragraph_      ;
    }

    friend COB_PROGRAM_LABELS;
};
typedef std::vector<COB_PROGRAM_LABEL>VCOB_PROGRAM_LABEL;

class COB_PROGRAM_LABELS
{
private:
    VCOB_PROGRAM_LABEL program_labels;

public:
    COB_PROGRAM_LABELS()
    {
    }
    void
    AddEntry(char type,
             const std::string &source_file,
             int line_number,
             int sentence_number,
             const std::string &program,
             const std::string &entry,
             const std::string &section,
             const std::string &paragraph)
    {
        COB_PROGRAM_LABEL cpl(  type,
                                source_file,
                                line_number,
                                sentence_number,
                                program,
                                entry,
                                section,
                                paragraph);
        program_labels.push_back(cpl);
    }
    void FormatProgramInfo(std::stringstream &ss) const;

    void
    Dump() const
    {
        std::cout << "COB_PROGRAM_LABELS:\n";
        std::cout << std::setw( 5) << "t";
        std::cout << std::setw(15) << "source_file";
        std::cout << std::setw( 6) << "line";
        std::cout << std::setw(30) << "program";
        std::cout << std::setw(30) << "entry";
        std::cout << std::setw(30) << "section";
        std::cout << std::setw(30) << "paragraph";
        std::cout << std::endl;

        for( VCOB_PROGRAM_LABEL::const_iterator it=program_labels.begin();
                it!=program_labels.end();
                it++) {
            COB_PROGRAM_LABEL label = *it;
            std::cout << std::setw( 5) << label.type;
            std::cout << std::setw(15) << label.source_file;
            std::cout << std::setw( 6) << label.line_number;
            std::cout << std::setw(30) << label.program;
            std::cout << std::setw(30) << label.entry;
            std::cout << std::setw(30) << label.section;
            std::cout << std::setw(30) << label.paragraph;
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
};

class DATA_DESCRIPTIONS;
class DATA_DESCRIPTION
{
public:
    std::string program_id;
    std::string cobol_symbol;
    std::string storage_type;
    std::string b_name;
    int         offset;
    int         size;
    std::string f_name;
    std::string picture_string;

public:
    DATA_DESCRIPTION()
    {
        offset = 0;
        size = 0;
    }
    friend DATA_DESCRIPTIONS;
};
typedef std::vector<DATA_DESCRIPTION> VDATA_DESCRIPTION;

typedef std::map<std::string,size_t> M_VKEY;

class DATA_DESCRIPTIONS
{
private:
    VDATA_DESCRIPTION symbols;
    // We are going to key that table by (program-id, cbl_symbol,offset)
    M_VKEY symbols_key;

    std::map<std::string,size_t>location_of_c_symbol;

public:
    int
    size() const
    {
        return static_cast<int>(symbols.size());
    }

    const DATA_DESCRIPTION &
    operator[](size_t n) const
    {
        return symbols[n];
    }

    int
    IndexOfSymbol(const std::string &symbol) const
    {
        std::map<std::string,size_t>::const_iterator it =
            location_of_c_symbol.find(symbol);
        if(it == location_of_c_symbol.end()) {
            return -1;
        }
        return static_cast<int>(it->second);
    }

    std::string
    MakeVKey(std::string const &program_id, std::string const &c_symbol,int offset) const
    {
        char ach[20];
        sprintf(ach,"%d",offset);
        return program_id + "|" + c_symbol + "|" + ach;
    }

    void
    DsInsert(const DATA_DESCRIPTION &ds )
    {
        symbols_key[MakeVKey(ds.program_id,ds.cobol_symbol,ds.offset)] = symbols.size();
        symbols.push_back(ds);
    }

    bool
    IsInDs(std::string const &program_id, std::string const &cobol_symbol,int offset) const
    {
        std::string dskey=MakeVKey(program_id,cobol_symbol,offset);
        M_VKEY::const_iterator it = symbols_key.find(dskey);
        return it != symbols_key.end();
    }

    size_t
    GetIndex(std::string const &program_id, std::string const &c_symbol,int offset) const
    {
        size_t retval = (size_t)(-1);
        std::string dskey=MakeVKey(program_id,c_symbol,offset);
        M_VKEY::const_iterator it = symbols_key.find(dskey);
        if(it != symbols_key.end()) {
            retval = it->second;
        }
        return retval;
    }

    void Dump() const;

    void Flatten(const COB_FIELDS &cob_fields,
                 const COB_FIELD_ATTRIBUTES &cob_field_attributes,
                 const COB_PIC_SYMBOLS &cob_pic_symbols,
                 const COB_DATA &cob_data
                );

};

void ScanAllDotHFiles(const std::string &path,
                      const std::string &fname,
                      COB_PIC_SYMBOLS &cob_pic_symbols,
                      COB_DATA &cob_data,
                      COB_FIELD_ATTRIBUTES &cob_field_attributes,
                      COB_FIELDS &cob_fields
                     );
void ScanForLinkage(std::ifstream &ifs,
                    const std::string &current_c_source,
                    COB_PROGRAM_LABELS &program_labels,
                    COB_DATA &cob_data,
                    COB_FIELDS &cob_fields
                   );


#ifdef NEEDED_TO_DEFINE_EQ
#undef EQ
#undef AND
#undef OR
#endif
