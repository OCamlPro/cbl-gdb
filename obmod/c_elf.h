#pragma once

#ifndef __C_ELF_H
#define __C_ELF_H

#include <string>
#include <iostream>
#include <fstream>
#include <istream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <string.h>
#include <stdarg.h>

#include "utils.h"
#include "dwarf4.h"
#include "dwarf4_tags.h"
#include "die.h"
#include "elf.h"
#include "line_machine.h"
#include "cobst.h"
#include "profiler.h"

class ELF;

class SECTION_HEADER
{
private:
    char *raw_data;                     // Local copy of raw section data.
    //                                  // section_header.size *always*
    //                                  // reflects the data size of this block.
    Elf64_Xword allocated_size;         // Used for expanding the block

public:
    Elf64_Shdr section_header;          // section header from ELF file
    std::string section_header_name;    // section name as indexed in .shstrtab section

    SECTION_HEADER()
    {
        CPROFILER;
        raw_data = nullptr;
        allocated_size = 0;
    }
    ~SECTION_HEADER()
    {
        CPROFILER;
        DeleteRawData();
    }
    char *RawData() const
    {
        CPROFILER;
        return raw_data;
    }
    void SetRawDataPointer(char *p,ULONGLONG size)
    {
        CPROFILER;
        DeleteRawData();
        raw_data = p;
        section_header.sh_size = size;
        allocated_size = size;
    }
    void DeleteRawData()
    {
        CPROFILER;
        delete[] raw_data;
        raw_data = nullptr;
        section_header.sh_size = 0;
        allocated_size = 0;
    }
    std::string GetSymbolByIndex(int index)
    {
        CPROFILER;
        std::string retval = "<" "?" "?" ">" ;
        if( index < section_header.sh_size ) {
            retval = std::string(raw_data + index);
        }
        return retval;
    }
    void TrimRawData(size_t nbytes)
    {
        CPROFILER;
        // Sometimes it is necessary, like when adding abbreviations to
        // .debug_abbrev, to overwrite something like a final zero.

        nbytes = std::min(nbytes,section_header.sh_size);
        section_header.sh_size -= nbytes;
    }
    void AppendToRawData(const char *p, ULONGLONG size)
    {
        CPROFILER;
        // Append p to raw_data
        while( section_header.sh_size + size > allocated_size ) {
            allocated_size = std::max(section_header.sh_size + size,allocated_size*2);
            char *newp = new char[allocated_size];
            memcpy(newp,raw_data,section_header.sh_size);
            std::swap(raw_data,newp);
            delete[] newp;
        }
        memcpy(raw_data + section_header.sh_size,p,size);
        section_header.sh_size += size;

        // Some sections have an initial length field.

        if( section_header_name == ".debug_info" ) {
            // Odd.  This is in dwarf4.h, but doesn't seem to get picked up?
            // So I don't know why I had to put this here.
            extern void SetSectionLength(PCHAR &p,bool bDwarfIs64,ULONGLONG length);

            bool bIsDwarf64 = *(ULONG *)raw_data == 0xFFFFFFFF;
            PCHAR p = raw_data;
            SetSectionLength(p,bIsDwarf64,section_header.sh_size);
        }
    }
};
typedef std::vector<SECTION_HEADER> VSECTION_HEADER;

class ELF
{
private:
    Elf64_Ehdr elf_header;

    USHORT section_version;
    bool   section_64;
    bool   bTargetIs64;

    VSECTION_HEADER section_headers;
    std::unordered_map<std::string,size_t> location_of_section_header_by_name;
    std::unordered_map<size_t,size_t>location_of_abbreviation_element;
    LNS_STATE_MACHINE lns_state_machine;
    COBST_SYMBOLS cobst_symbols;
    DIES debug_info_dies;
    std::unordered_map<Elf64_Addr,size_t>hash_by_r_offset;

    void ReadInElfHeader(std::ifstream &ifs);
    void ReadInSectionHeaders(std::ifstream &ifs);
    void ReadInRawSectionData(std::ifstream &ifs);
    DIE  GetNextDie(int &nLevel, CPCHAR &pdata, bool bIs64);
    size_t AddToAbbreviations(ABBREVIATION &abbreviation);
    size_t AddToAbbreviations(DW_TAG dw_tag,
                              bool bHasChildren,
                              ...  );
    size_t BuildDie(PCHAR  &pdata,
                    size_t &form_strp_offset,
                    size_t &form_exprloc_offset,
                    size_t abbreviation_index,
                    bool   bDwarfIs64,
                    va_list valist);
    size_t BuildAndAppendDie( size_t &form_strp_offset,
                              size_t &form_exprloc_offset,
                              SECTION_HEADER *debug_info,
                              SECTION_HEADER *relocs,
                              ULONGLONG abbreviation_index_of_base_type,
                              ...);
    void CreateRelocationFor(SECTION_HEADER *debug_info,
                             SECTION_HEADER *relocs,
                             size_t location_of_at_location,
                             size_t form_exprloc_offset,
                             size_t additional_offset);

    ULONGLONG AddToStringTable(const std::string &sec_name,
                               const std::string &text);
    void CreateSection(const std::string &sec_name,
                       ULONGLONG type,
                       ULONGLONG flags );

    void WriteOutElfHeader(std::ofstream &ofs);
    void WriteOutSectionHeaders(std::ofstream &ofs);
    void WriteOutRawSectionData(std::ofstream &ofs);

public:
    ELF()
    {
        bTargetIs64 = true;
    }
    ~ELF()
    {
    }

    bool IsTarget64() const
    {
        return bTargetIs64;
    }

    void ReadCobolSymbolTable(const std::string &cobst_filename);
    void ReadFrom(std::string object_filename);
    void WriteTo(std::string object_filename);
    void DisplaySectionHeaders(std::ostream &ofs=std::cout);

    void ApplyRelocations(int plus_or_minus);
    size_t GetAbbreviationOffset(ULONGLONG abbreviation) const;
    const char *GetAbbreviationLocation(ULONGLONG abbreviation) const;
    void BuildAbbreviationIndex();
    void BuildDebugInfo();
    void DumpEntireDebugInfoDieTree();
    void RunLineStateMachine(bool show_your_work);
    void ReplaceDebugLineSection();
    void InsertDebugScript(const std::string &python_script);
    void AugmentSymbolTable();

    SECTION_HEADER *GetSectionHeaderDangerous(const std::string secname)
    {
        CPROFILER;
        SECTION_HEADER *retval = nullptr;
        size_t index = GetSectionHeaderIndex(secname);
        if(index != std::string::npos) {
            retval = &section_headers[index];
        }
        return retval;
    }

    size_t GetSectionHeaderIndex(const std::string secname) const
    {
        CPROFILER;
        std::unordered_map<std::string,size_t>::const_iterator it = location_of_section_header_by_name.find(secname);
        if(it == location_of_section_header_by_name.end()) {
            return std::string::npos;
        }
        return it->second;
    }

    const char *GetSectionDataLocation(const std::string secname) const
    {
        CPROFILER;
        size_t index = GetSectionHeaderIndex(secname);
        return section_headers[index].RawData();
    }
    char *GetSectionDataLocationDangerous(const std::string secname)
    {
        CPROFILER;
        size_t index = GetSectionHeaderIndex(secname);
        return section_headers[index].RawData();
    }

    void ReplaceASymbol(const std::string &section, const std::string &was, const std::string &is, bool whole_symbol);
    void BruteForceReplace(const std::string &section, const std::string &was, const std::string &is);

};


#endif