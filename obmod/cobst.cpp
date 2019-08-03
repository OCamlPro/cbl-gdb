#include <iostream>
#include <fstream>
#include <unordered_map>
#include "cobst.h"
#include "params.h"

#define EQ ==
#define AND &&
#define OR ||

using namespace std;

void COBST_SYMBOLS::ReadFrom(const string &cobst_filename)
{
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
    const string ENTRY              = "Entry";
    const string SECTION            = "Section";
    const string PARAGRAPH          = "Paragraph";
    const string ASSEMBLY_SECTION   = "Assembly Section";
    const string ADDRESS            = "Relative Address";

    unordered_map<string,size_t>header_columns;
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
        if(values[header_columns[ASSEMBLY_SECTION]] EQ ".text") {
            cobst_symbol.file       = values[header_columns[SOURCE_FILE]];
            cobst_symbol.line_number= stoull(values[header_columns[LINE_NUMBER]]);
            cobst_symbol.program    = values[header_columns[PROGRAM_NAME]];
            cobst_symbol.entry      = values[header_columns[ENTRY]];
            cobst_symbol.section    = values[header_columns[SECTION]];
            cobst_symbol.paragraph  = values[header_columns[PARAGRAPH]];
            cobst_symbol.address    = stoull(values[header_columns[ADDRESS]].substr(2),
                                             nullptr,16);

            if(         prior.file      != cobst_symbol.file
                        OR  prior.program   != cobst_symbol.program
                        OR  prior.entry     != cobst_symbol.entry
                        OR  prior.section   != cobst_symbol.section
                        OR  prior.paragraph != cobst_symbol.paragraph  ) {
                cobst_symbol.status_change = true;
            }
            cobst_symbols.push_back(cobst_symbol);
            prior = cobst_symbol;
        }
    }
}
