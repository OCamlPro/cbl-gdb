#pragma once

#include <vector>
#include <string>
#include "fields.h"

class VARIABLES;
class VARIABLE
{
private:
    // These fields correspond to the VARIABLE_STRING_xxx that we will
    // be generating

    std::string record_type;    // One character:
    //                          // E for PROGRAM-ID
    //                          // F for File Data
    //                          // W for WORKING-STORAGE
    //                          // L for LOCAL-STORAGE
    //                          // I for LINKAGE
    int         level;          // 0, 1-49, 66, 77, 78, 88
    // 78 is a MicroFocus "read-only" memory extension
    // 88 probably won't appear at all
    std::string name;           // COBOL variable identifier
    std::string f_name;         // Name of the associated cob_field variable
    std::string b_name;         // Name of the location of the data
    std::string a_name;         // Name of the associated cob_field_attrib variable
    int         offset;         // When f_name exists, offset should have the same runtime value as f_name->data - b_name;
    int         length;         // When f_name exists, length should be the same as f_name->size
    int         occurs_min;     //
    int         occurs_max;     // -1 for unbounded
    //                          // When f_name exists, b_name, a_name, offset, and length, won't be available
    //                          // When f_name doesn't exist, the others should be present, and can be used
    //                          // to synthesize an f0 cob_field structure to hand to the cob_get routine

public:
    VARIABLE()
    {
        level = 0;
        offset = 0;
        length = 0;
        occurs_min = 0;
        occurs_max = 1;
    }

    friend class VARIABLES;
};

typedef std::vector<VARIABLE> V_VARIABLES;

class VARIABLES
{
private:
    V_VARIABLES variables;

public:
    void ScanCFile(const std::string &path, const std::string &fname);
    void FillInTheBlanks(const COB_FIELDS_2 &cob_fields_2);
    void Dump() const;
    void
    Insert(const VARIABLE &v)
    {
        variables.push_back(v);
    }
    void FormatVariablesInfo(std::stringstream &ss) const;
};


