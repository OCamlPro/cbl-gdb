#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <regex>
#include <sstream>
#include "profiler.h"
#include "utils.h"

#ifndef EQ
#define EQ ==
#define AND &&
#define OR ||
#define NEEDED_TO_DEFINE_EQ
#endif

std::string GetCommentText(const std::string &s);

class GAS_SYMBOL
{
public:
    int line_number;
    std::string gas_section;
    std::string gas_symbol;
    std::string undecorated_symbol;
    std::string field_data_pointer;
    std::string field_data_offset;
    LONGLONG gas_value;

    GAS_SYMBOL()
    {
        line_number = 0;
        gas_value = 0;
    }
};

class GAS_SYMBOLS
{
protected:
    typedef std::vector<GAS_SYMBOL> VGAS_SYMBOLS;

    VGAS_SYMBOLS symbols;
    std::unordered_map<std::string,size_t>gas_undecorated_symbol_location;
    std::unordered_map<std::string,size_t> reverse_gas_symbol_location;

    std::string SectionValueCombiner(const std::string &section,LONGLONG value) const
    {
        std::stringstream ss;
        ss<<value;
        return section + ":" + ss.str();
    }

    void SectionValueSplitter(const std::string &combined,std::string &section, LONGLONG &value)
    {
        section.clear();
        value = 0;
        size_t nfound = combined.find_last_of(':');
        if(nfound != std::string::npos) {
            section = combined.substr(0,nfound);
            value = stoll(combined.substr(nfound+1));
        }
    }

public:
    void GetSymbolsFromFile(const std::string &source, const std::string &scope, std::ifstream &ifs);

    GAS_SYMBOL FindGasSymbol(const std::string &sym) const
    {
        GAS_SYMBOL retval;
        if(!sym.empty()) {
            std::unordered_map<std::string,size_t>::const_iterator it = gas_undecorated_symbol_location.find(sym);
            if(it != gas_undecorated_symbol_location.end()) {
                retval = symbols[it->second];
            }
        }
        return retval;
    }

    GAS_SYMBOL FindGasValue(const std::string &section,LONGLONG value) const
    {
        GAS_SYMBOL retval;
        const std::string sectionvalue = SectionValueCombiner(section,value);

        std::unordered_map<std::string,size_t>::const_iterator it = reverse_gas_symbol_location.find(sectionvalue);
        if(it != reverse_gas_symbol_location.end()) {
            retval = symbols[it->second];
        }
        return retval;
    }

    void UpdateBaseOffset(std::string symbol, std::string base, std::string offset);


    int size() const
    {
        return static_cast<int>(symbols.size());
    }

    const GAS_SYMBOL & operator[](int n) const
    {
        return symbols[n];
    }
};

class COB_LABEL
{
public:
    std::string source_file;
    int line_number;
    int sentence_number;
    std::string program;
    std::string entry;
    std::string section;
    std::string paragraph;
    std::string gas_section;
    std::string gas_symbol;
    LONGLONG   gas_value;

    COB_LABEL()
    {
        line_number = -1;
        sentence_number = -1;
        gas_value = 0;
    }
};
typedef std::vector<COB_LABEL>VCOB_LABEL;

class COB_LABELS
{
protected:
    VCOB_LABEL          symbols;
    std::unordered_map<std::string,int>program_id_lines;

public:
    void GetProgramIdLineNumbers(const std::string &source, const std::string &scope, std::ifstream &ifs, GAS_SYMBOLS &gas_symbol_table);
    void GetSymbolsFromFile(const std::string &source, const std::string &scope, std::ifstream &ifs, GAS_SYMBOLS &gas_symbol_table);
    void MergeGasSymbols(const GAS_SYMBOLS &gas_symbol_table);

    int size() const
    {
        return static_cast<int>(symbols.size());
    }

    const COB_LABEL & operator[](int n) const
    {
        return symbols[n];
    }
};

class DATA_DESCRIPTION
{
public:
    std::string source;
    int line_number;
    std::string program_id;
    std::string storage_type;
    std::string storage_scope;
    std::string cbl_symbol;
    std::string gas_section;
    std::string gas_symbol;
    std::string undecorated_symbol;
    std::string picture_string;
    LONGLONG    gas_value;

    // storage comes with a size parameter:  b_17[21];
    LONGLONG storage_size;

    // See cobc/cobc/libcob/common.h declaration for cob_field_attr
    USHORT attr_type;
    USHORT attr_digits;
    short attr_scale;
    USHORT attr_flags;
    std::string attr_pic_pointer;

    // See cobc/cobc/libcob/common.h declaration for cob_field
    int field_size;
    std::string field_data_pointer;
    std::string field_data_offset;
    std::string field_attr_pointer;

public:
    DATA_DESCRIPTION()
    {
        line_number = 0;
        gas_value = 0;
        storage_size = 0;
        attr_type = 0;
        attr_digits = 0;
        attr_scale = 0;
        attr_flags = 0;
        field_size = 0;
    }
};
typedef std::vector<DATA_DESCRIPTION> VDATA_DESCRIPTION;

class DATA_DESCRIPTIONS
{
private:
    VDATA_DESCRIPTION symbols;
    std::unordered_map<std::string,size_t>location_of_gas_symbol;
    std::unordered_map<std::string,std::string>b_symbol_to_program_id;

    void ProcessDataStorage(std::ifstream &ifs,const std::string &source_file);

public:
    void ScanAllDotHFiles(const std::string &path,const std::string &fname);
    void AppendLinkageInformation(const COB_LABELS &cob_labels);

    void MergeGasSymbols(const GAS_SYMBOLS &gas_symbol_table);

    int size() const
    {
        return static_cast<int>(symbols.size());
    }

    const DATA_DESCRIPTION & operator[](int n) const
    {
        return symbols[n];
    }

    int IndexOfSymbol(const std::string &symbol) const
    {
        std::unordered_map<std::string,size_t>::const_iterator it =
            location_of_gas_symbol.find(symbol);
        if(it == location_of_gas_symbol.end()) {
            return -1;
        }
        return static_cast<int>(it->second);
    }

    std::string WeveSeen(const std::string &b_sym) const
    {
        std::string retval;
        std::unordered_map<std::string,std::string>::const_iterator it
            = b_symbol_to_program_id.find(b_sym);
        if(it != b_symbol_to_program_id.end()) {
            retval = it->second;
        }
        return retval;
    }

};

#ifdef NEEDED_TO_DEFINE_EQ
#undef EQ
#undef AND
#undef OR
#endif
