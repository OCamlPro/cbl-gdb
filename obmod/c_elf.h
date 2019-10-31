#pragma once

#ifndef __C_ELF_H
#define __C_ELF_H

#include <string>
#include <iostream>
#include <fstream>
#include <istream>
#include <vector>
#include <unordered_map>
#include "utils.h"
#include "dwarf4.h"
#include "die.h"
#include "elf.h"
#include "line_machine.h"
#include "cobst.h"

class ELF;

class SECTION_HEADER
{
private:
    char *raw_data;                     // Local copy of raw section data.
    //                                  // section_header.size *always*
    //                                  // reflects the size of this block.

public:
    Elf64_Shdr section_header;          // section header from ELF file
    std::string section_header_name;    // section name as indexed in .shstrtab section

    SECTION_HEADER()
    {
        raw_data = nullptr;
    }
    ~SECTION_HEADER()
    {
        DeleteRawData();
    }
    char *RawData()
    {
        return raw_data;
    }
    void SetRawDataPointer(char *p,ULONGLONG size)
    {
        if(raw_data != nullptr) {
            // Delete any existing buffer:
            delete raw_data;
            raw_data = nullptr;
            section_header.sh_size = 0;
        }
        raw_data = p;
        section_header.sh_size = size;
    }
    void DeleteRawData()
    {
        delete raw_data;
        raw_data = nullptr;
    }
    std::string GetSymbolByIndex(int index)
    {
        std::string retval = "<??>" ;
        if( index < section_header.sh_size ) {
            retval = std::string(raw_data + index);
        }
        return retval;
    }
};
typedef std::vector<SECTION_HEADER> VSECTION_HEADER;

class ELF
{
private:
    Elf64_Ehdr elf_header;
    VSECTION_HEADER section_headers;
    std::unordered_map<std::string,size_t> location_of_section_header_by_name;
    std::unordered_map<ULONGLONG,char *>location_of_abbreviation_element;
    DIE_TREE *debug_info_die_tree;
    DIE_TREE *debug_line_die_tree;
    UBYTE debug_info_address_size;
    bool bIs64;
    LNS_STATE_MACHINE lns_state_machine;
    COBST_SYMBOLS cobst_symbols;

    void ReadInElfHeader(std::ifstream &ifs);
    void ReadInSectionHeaders(std::ifstream &ifs);
    void ReadInRawSectionData(std::ifstream &ifs);
    void BuildAbbreviationIndex();

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
        debug_info_die_tree = new DIE_TREE(this);
        debug_info_address_size = 0;
    }
    ~ELF()
    {
        delete debug_info_die_tree;
    }

    void ReadCobolSymbolTable(const std::string &cobst_filename);
    void ReadFrom(std::string object_filename);
    void WriteTo(std::string object_filename);
    void DisplaySectionHeaders(std::ostream &ofs=std::cout);

    void ApplyRelocations(int plus_or_minus);
    char *GetAbbreviationLocation(ULONGLONG abbreviation) const;
    void BuildDieTree(std::string section_name);
    void DumpEntireDebugInfoDieTree(std::string section_name);
    void RunLineStateMachine(bool show_your_work);
    void ReplaceDebugLineSection();
    void InsertDebugScript(const std::string &python_script);
    void AugmentSymbolTable();

    ULONGLONG GetAddressSize() const
    {
        return debug_info_address_size;
    }
    bool Is64() const
    {
        return bIs64;
    }

    size_t GetSectionHeaderIndex(const std::string secname)
    {
        std::unordered_map<std::string,size_t>::const_iterator it = location_of_section_header_by_name.find(secname);
        if(it == location_of_section_header_by_name.end()) {
            return std::string::npos;
        }
        return it->second;
    }

    char *GetSectionDataLocation(const std::string secname)
    {
        size_t index = GetSectionHeaderIndex(secname);
        return section_headers[index].RawData();
    }

    void ReplaceASymbol(const std::string &section, const std::string &was, const std::string &is, bool whole_symbol);
    void BruteForceReplace(const std::string &section, const std::string &was, const std::string &is);

};


#endif