#pragma once

class ELF;

class ATTRIBUTE_FORMAT_PAIR
{
private:
    ELF *pelf;    // Parent ELF object, needed for PayloadAsString
    DW_AT attribute;
    DW_FORM format;
    char *location_of_payload;
    ULONGLONG total_size_of_payload;
    std::vector<UBYTE> payload;

public:
    ATTRIBUTE_FORMAT_PAIR(ELF *elf)
    {
        pelf = elf;
        location_of_payload = nullptr;
        total_size_of_payload = 0;
    }

    void SetAT(ULONGLONG a)
    {
        attribute = static_cast<DW_AT>(a);
    }
    void SetForm(ULONGLONG f)
    {
        format = static_cast<DW_FORM>(f);
    }
    DW_AT GetAT() const
    {
        return attribute;
    }
    DW_FORM GetForm() const
    {
        return format;
    }
    void SetPayloadLocation(char *p)
    {
        location_of_payload = p;
    }
    void SetPayloadTotalSize(char *p)
    {
        total_size_of_payload = p - location_of_payload;
    }
    void PayloadPush(char n)
    {
        payload.push_back(n);
    }
    std::string PayloadAsString();

};
typedef std::vector<ATTRIBUTE_FORMAT_PAIR>VATTRIBUTE_FORMAT_PAIRS;

class DIE
{
    // This is a Debugging Information Entry
private:
    ELF *pelf;            // Parent ELF object

    char    *offset;            // Offset into debug_<section>
    DW_TAG  dw_tag;
    bool    has_children;       // Redundant: has_children if and only if children.size() is non-zero
    // But it can be meaningful *before* we've had a chance to process those children
    VATTRIBUTE_FORMAT_PAIRS at_form_pairs;
    void Initialize()
    {
        // Dangerous if called with parent != nullptr; you could cause memory
        // leaks unless you know what you are doing.
        offset = nullptr;
        dw_tag = DW_TAG_NONE;
        has_children = false;
        parent = nullptr;
    }
public:
    int  sequence_number;
    DIE *parent;                 // nullptr at root of tree
    std::vector<DIE *>children;

    DIE(ELF *elf)
    {
        pelf = elf;
        Initialize();
    }
    void DecodeThyself(PCHAR &debug_section_pointer);
    bool HasChildren() const
    {
        return has_children;
    }
    DW_TAG GetTag() const
    {
        return dw_tag;
    }
    void Show();
};

class DIE_TREE
{
private:
    ELF *pelf;
    DIE *root;
    std::vector<DIE *>parents;  // parents.back() is the most recent DIE flagged as having children.
    // We need it to establish DIE.parent as we walk through the debug_<section>
    void UprootTheTree(DIE *);
    int  sequence_number;       // Useful for tracking parents

public:
    DIE_TREE(ELF *elf)
    {
        pelf = elf;
        root = nullptr;
        sequence_number = 0;
    }
    ~DIE_TREE()
    {
        UprootTheTree(root);
    }
    void GetNextDie(PCHAR &debug_section_pointer);
    void DisplayEntireTree(DIE *die);
};

