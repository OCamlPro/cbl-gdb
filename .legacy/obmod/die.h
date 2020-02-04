#pragma once

#include "profiler.h"

class ELF;

class ATTRIBUTE_FORMAT_PAIR
{
private:
    DW_AT dw_at;                // Decoded as per abbreviation table
    DW_FORM dw_form;            // Decoded as per abbreviation table

public:
    std::vector<UBYTE> payload; // Actual payload from .debug_info data,
    //                          // decoded as per DW_FORM

private:
    size_t location_of_payload; // Offset of payload in .debug_info

public:
    ATTRIBUTE_FORMAT_PAIR()
    {
        location_of_payload = 0;
        dw_at = (DW_AT)0;
        dw_form = (DW_FORM)0;
    }
    void SetAT(DW_AT a)
    {
        dw_at = static_cast<DW_AT>(a);
    }
    void SetForm(DW_FORM f)
    {
        dw_form = static_cast<DW_FORM>(f);
    }
    DW_AT GetAT() const
    {
        return dw_at;
    }
    DW_FORM GetForm() const
    {
        return dw_form;
    }
    size_t GetLocationOfPayload() const
    {
        return location_of_payload;
    }
    void SetPayloadLocation(size_t l)
    {
        location_of_payload = l;
    }
    void PayloadPush(char n)
    {
        payload.push_back(n);
    }
    size_t PayloadBytesSize() const
    {
        return payload.size();
    }
    const UBYTE *PayloadBytes() const
    {
        return payload.data();
    }
    std::string PayloadAsString(const ELF &elf,bool with_offset=true) const;

};
typedef std::vector<ATTRIBUTE_FORMAT_PAIR>VATTRIBUTE_FORMAT_PAIRS;

class ABBREVIATION
{
private:
    size_t  sequence_number;
    DW_TAG  dw_tag;
    bool    has_children;
    VATTRIBUTE_FORMAT_PAIRS at_form_pairs;

public:
    ABBREVIATION()
    {
        Initialize();
    }
    void Initialize()
    {
        CPROFILER;
        sequence_number = 0;
        dw_tag = DW_TAG_NONE;
        has_children = false;
        at_form_pairs.clear();
    }
    friend class DIE;
    friend class ELF;
};

class DIE
{
    // This is a Debugging Information Entry
private:
    size_t location_offset;     // Location offset in .debug_info
    size_t abbreviation_index;  // Index into .debug_abbrev
    int    nlevel;              // What generation we are

    ABBREVIATION abbreviation;  // From the abbreviation table

    void Initialize()
    {
        abbreviation.Initialize();
    }
public:
    DIE()
    {
        Initialize();
    }
    void DecodeThyself(int &nLevel_,
                       CPCHAR &debug_section_pointer,
                       const ELF &elf,
                       bool bIs64);
    bool HasChildren() const
    {
        return abbreviation.has_children;
    }
    DW_TAG GetTag() const
    {
        return abbreviation.dw_tag;
    }
    void Show(const ELF &elf);
    size_t Size() const
    {
        return abbreviation.at_form_pairs.size();
    }
    const ATTRIBUTE_FORMAT_PAIR & operator[](size_t i) const
    {
        return abbreviation.at_form_pairs[i];
    }
    size_t OffsetIntoDebugInfo() const
    {
        return location_offset;
    }

};
typedef std::vector<DIE>DIES;
