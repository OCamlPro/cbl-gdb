#pragma once

#include <string>
#include "utils.h"

class COBST_SYMBOL
{
public:
    std::string file;       //  The COBOL source file
    ULONGLONG   line_number;//  The specified line number
    ULONGLONG   column;     //  The column, if it can be divined

    std::string program;    //  From the COBOL PROGRAM-ID
    std::string division;   //  Procedure vs Data
    std::string entry;      //  ...programs have entries
    std::string section;    //  ...that might have sections
    std::string paragraph;  //  ...that might have paragraphs

    bool    status_change;  //  Means that program.entry.section.paragraph has changed

    ULONGLONG   address;    //  Relative address

    std::string full_name;  //  COBOL symbol name
    int         level;      //  COBOL symbol level
    std::string base_symbol;//  C-code base symbol for the COBOL symbol
    ULONGLONG   offset;     //  offset from the base address
    std::string list_type;  //  
    size_t      list_size;  // Length of data
    std::string picture;
    int         occurs;
    USHORT      attr_type;
    USHORT      attr_digits;
    SHORT       attr_scale;
    USHORT      attr_flags;

    COBST_SYMBOL()
    {
        line_number = 0;
        column = 0;
        address = 0;
        status_change = false;
    }
};
typedef std::vector<COBST_SYMBOL>VCOBST_SYMBOLS;

class COBST_SYMBOLS
{
private:
    VCOBST_SYMBOLS cobst_symbols;

public:
    void ReadFrom(const std::string &cobst_filename);
    size_t size() const
    {
        return cobst_symbols.size();
    }
    const COBST_SYMBOL &operator[](size_t n) const
    {
        return cobst_symbols[n];
    }

};