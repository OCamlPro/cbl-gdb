#include <iostream>
#include <fstream>
#include <unordered_map>
#include "cobst.h"
#include "params.h"
#include "profiler.h"

#define EQ ==
#define AND &&
#define OR ||

using namespace std;

void COBST_SYMBOLS::ReadFrom(const string &cobst_filename)
{
    CPROFILER;
    ifstream ifs;
    OpenOrFail(ifs,cobst_filename);

    string input;

    // First order of business: get a map of column headers.  This means, of
    // course, that any change to the header names in the cobst output will
    // break this file.

    getline(ifs,input);
    vector<string>column_headers = Split(input,"\t");

    const string SOURCE_FILE        = "Source File";
    const string LINE_NUMBER        = "Line Number";
    const string PROGRAM_NAME       = "Program Name";
    const string DIVISION           = "Division";
    const string ENTRY              = "Entry";
    const string SECTION            = "Section";
    const string PARAGRAPH          = "Paragraph";
    const string ASSEMBLY_SECTION   = "Assembly Section";
    const string ADDRESS            = "Relative Address";
    const string FULL_NAME          = "Full Name";
    const string LEVEL              = "Level";
    const string BASE_SYMBOL        = "Base Symbol";
    const string OFFSET             = "Offset";
    const string LIST_TYPE          = "List Type";
    const string LIST_SIZE          = "List Size";
    const string PICTURE            = "Picture";
    const string OCCURS             = "Occurs";
    const string ATTR_TYPE          = "Attr Type";
    const string ATTR_DIGITS        = "Attr Digits";
    const string ATTR_SCALE         = "Attr Scale";
    const string ATTR_FLAGS         = "Attr Flags";

    map<string,size_t>header_columns;
    for(int i=0; i<(int)column_headers.size(); i++) {
        header_columns[column_headers[i]] = i;
    }

    // With that map, we can extract the information of interest to us:
    COBST_SYMBOL prior;
    for(;;) {
        COBST_SYMBOL cobst_symbol;
        getline(ifs,input);
        vector<string>values = Split(input,"\t");

        if(values.size() < 9) {
            break;
        }
        if(  values[header_columns[ASSEMBLY_SECTION]] EQ ".text" ) {
            cobst_symbol.file       = values[header_columns[SOURCE_FILE]];
            cobst_symbol.line_number= stoull(values[header_columns[LINE_NUMBER]]);
            cobst_symbol.program    = values[header_columns[PROGRAM_NAME]];
            cobst_symbol.division   = values[header_columns[DIVISION]];
            cobst_symbol.entry      = values[header_columns[ENTRY]];
            cobst_symbol.section    = values[header_columns[SECTION]];
            cobst_symbol.paragraph  = values[header_columns[PARAGRAPH]];
            cobst_symbol.address    =
                stoull(values[header_columns[ADDRESS]].substr(2),nullptr,16);
            if(     prior.file      != cobst_symbol.file
                    OR  prior.program   != cobst_symbol.program
                    OR  prior.entry     != cobst_symbol.entry
                    OR  prior.section   != cobst_symbol.section
                    OR  prior.paragraph != cobst_symbol.paragraph  ) {
                cobst_symbol.status_change = true;
            }

            cobst_symbols.push_back(cobst_symbol);
            prior = cobst_symbol;
        } else {
            cobst_symbol.section    = values[header_columns[SECTION]];
            if( cobst_symbol.section EQ "WORKING-STORAGE"
                    OR cobst_symbol.section EQ "LOCAL-STORAGE"
                    OR cobst_symbol.section EQ "LINKAGE" ) {
                cobst_symbol.full_name = values[header_columns[FULL_NAME]];
                cobst_symbol.level = stoi(values[header_columns[LEVEL]]);
                cobst_symbol.base_symbol = values[header_columns[BASE_SYMBOL]];
                cobst_symbol.offset = stoull(values[header_columns[OFFSET]]);
                cobst_symbol.list_type = values[header_columns[LIST_TYPE]];
                cobst_symbol.list_size = stoull(values[header_columns[LIST_SIZE]]);
                cobst_symbol.picture = values[header_columns[PICTURE]];
                cobst_symbol.occurs = stoi(values[header_columns[OCCURS]]);
                cobst_symbol.attr_type = (USHORT)stoul(values[header_columns[ATTR_TYPE]]);
                cobst_symbol.attr_digits = (USHORT)stoul(values[header_columns[ATTR_DIGITS]]);
                cobst_symbol.attr_scale = (SHORT)stoi(values[header_columns[ATTR_SCALE]]);
                cobst_symbol.attr_flags = (USHORT)stoul(values[header_columns[ATTR_FLAGS]]);
                }

            cobst_symbols.push_back(cobst_symbol);
            prior = cobst_symbol;
        }
    }
}
