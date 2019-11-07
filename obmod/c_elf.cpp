#include <iostream>
#include <cstring>
#include <unordered_map>
#include "c_elf.h"
#include "params.h"
#include "profiler.h"
#include "dwarf4.h"
#include "cprint_py.h"

#define EQ ==
#define AND &&
#define OR ||

using namespace std;

void DIE::DecodeThyself(int &nLevel_,CPCHAR &pdata,const ELF &elf, bool bDwarfIs64)
{
    CPROFILER;

    Initialize();

    bool bTargetIs64 = elf.IsTarget64();

    nlevel = nLevel_;
    char const * const pdata_start = elf.GetSectionDataLocation(".debug_info");
    location_offset = pdata - pdata_start;
    abbreviation_index = ULEB128Decode(pdata);

    if(abbreviation_index != 0) {
        char const * pabbrev = elf.GetAbbreviationLocation(abbreviation_index);
        if(pabbrev EQ nullptr) {
            cerr << "GetAbbreviationLocation() returned NULL in DIE::DecodeThyself().\n";
            cerr << "This needs to be understood.\n";
            exit(1);
        }
        ULONGLONG abbrev_from_pabbrev = ULEB128Decode(pabbrev);
        if(abbreviation_index != abbrev_from_pabbrev) {
            cerr << "abbreviation != abbrev_from_pabbrev in DIE::DecodeThyself().\n";
            cerr << "This needs to be understood.\n";
            exit(1);
        }

        // First element is a DW_TAG
        abbreviation.dw_tag = static_cast<DW_TAG>(ULEB128Decode(pabbrev));

        // The second element is DW_CHILDREN
        DW_CHILDREN dw_children = static_cast<DW_CHILDREN>(ULEB128Decode(pabbrev));
        switch(dw_children) {
        case DW_CHILDREN_no:
            abbreviation.has_children = false;
            break;
        case DW_CHILDREN_yes:
            abbreviation.has_children = true;
            nLevel_ += 1;
            break;
        default:
            cerr << "In DIE::DecodeThyself(), the second abbreviation\n";
            cerr << "element is neither DW_CHILDREN_no nor DW_CHILDREN_yes.\n";
            cerr << "This needs to be understood.\n";
            exit(1);
        }

        for(;;) {
            ATTRIBUTE_FORMAT_PAIR afp;
            DW_AT dw_at = (DW_AT)ULEB128Decode(pabbrev);
            DW_FORM dw_form = (DW_FORM)ULEB128Decode(pabbrev);
            if( (int)dw_at EQ 0 AND (int)dw_form EQ 0) {
                break;
            }
            afp.SetAT(dw_at);
            afp.SetForm(dw_form);

            /*  Here's where it gets exciting.

                Although the DWARF documention is coy about describing the actual data structure mechanics,
                it works like this:

                .debug.info has an abbreviation code.

                That code points to an abbreviation in .debug.abbrev

                After a DW_TAG value and a DW_CHILDREN value, the abbreviation then
                consists of ATTRIBUTE_TYPE/FORM pairs.

                The FORM values describes how the actual attribute is stored back in the .debug_info
                data stream, which starts immediately after the abbreviation value.

                We now must extract the attribute payload.  We have to do the extraction at this point
                in order to keep the pdata pointer updated.

                */

            // This value gets set before we pick up any bytes of the payload.
            afp.SetPayloadLocation(pdata-pdata_start);

IndirectFormProcessing:
            // Note that the indirect form means that the first part of the payload is
            // a DW_FORM code itself. That second one also needs processing.  Hence this goto target
            // address has to appear after the SetPayloadLocation call.
            dw_form = afp.GetForm(); // In case this is an IndirectFormProcessing sequence

            ULONGLONG nLength = 0;
            switch(dw_form) {
            case DW_FORM_addr:
                nLength = bTargetIs64 ? 8 : 4;
                break;
            case DW_FORM_block2:
                nLength = GetUshort(pdata);
                break;
            case DW_FORM_block4:
                nLength = GetUlong(pdata);
                break;
            case DW_FORM_data2:
                nLength = 2;
                break;
            case DW_FORM_data4:
                nLength = 4;
                break;
            case DW_FORM_data8:
                nLength = 8;
                break;
            case DW_FORM_string:
                // Zero-terminated string of 8-bit characters
                for(;;) {
                    UBYTE b = GetUbyte(pdata);
                    afp.PayloadPush(b);
                    if(b EQ 0) {
                        break;
                    }
                }
                break;
            case DW_FORM_block:
                nLength = ULEB128Decode(pdata);
                break;
            case DW_FORM_block1:
                nLength = GetUbyte(pdata);
                break;
            case DW_FORM_data1:
                nLength = 1;
                break;
            case DW_FORM_flag:
                nLength = 1;
                break;
            case DW_FORM_sdata:
                // The payload is a signed LEB128 number.  Each byte
                // except the last has the 0x80 bit ON.
                for(;;) {
                    UBYTE b = GetUbyte(pdata);
                    afp.PayloadPush(b);
                    if((b & 0x80) EQ 0) {
                        break;
                    }
                }
                break;
            case DW_FORM_strp: {
                nLength = bDwarfIs64 ? 8 : 4;
                break;
            }
            case DW_FORM_udata:
                // The payload is an unsigned LEB128 number.  Each byte
                // except the last has the 0x80 bit ON.
                for(;;) {
                    UBYTE b = GetUbyte(pdata);
                    afp.PayloadPush(b);
                    if((b & 0x80) EQ 0) {
                        break;
                    }
                }
                break;
            case DW_FORM_ref_addr:
                nLength = bDwarfIs64 ? 8 : 4;
                break;
            case DW_FORM_ref1:
                nLength = 1;
                break;
            case DW_FORM_ref2:
                nLength = 2;
                break;
            case DW_FORM_ref4:
                nLength = 4;
                break;
            case DW_FORM_ref8:
                nLength = 8;
                break;
            case DW_FORM_ref_udata:
                for(;;) {
                    UBYTE b = GetUbyte(pdata);
                    afp.PayloadPush(b);
                    if((b & 0x80) EQ 0) {
                        break;
                    }
                }
                break;
            case DW_FORM_indirect:
                // This is whacked out.  The initial payload is a DW_FORM code that
                // has to be processed:
            {
                DW_FORM form = static_cast<DW_FORM>(ULEB128Decode(pdata));
                afp.SetForm(form);
                goto IndirectFormProcessing;
                break;
            }
            case DW_FORM_sec_offset:
                nLength = bDwarfIs64 ? 8 : 4;
                break;
            case DW_FORM_exprloc:
                nLength = ULEB128Decode(pdata);
                break;
            case DW_FORM_flag_present:
                // This FORM has no actual data
                break;
            case DW_FORM_ref_sig8:
                nLength = 8;
                break;
            default:
                cerr << "switch(afp.GetForm()) processing an unknown value in DIE::DecodeThyself().\n";
                cerr << "This needs to be understood.\n";
                break;
            }
            for(ULONGLONG i=0; i<nLength; i++) {
                afp.PayloadPush(GetUbyte(pdata));
            }

            // We've built this AT_FORM_PAIR; push it onto our list
            abbreviation.at_form_pairs.push_back(afp);
        }
    }
}

void DIE::Show(const ELF &elf)
{
    CPROFILER;
    cout << "<" << nlevel << ">";
    cout << "<" << hex << location_offset << ">";

    cout << " Abbrev: " << dec << abbreviation_index;
    cout << " (" << DW_TagToString(abbreviation.dw_tag) << ")";
    cout << endl;

    for(size_t i=0; i<abbreviation.at_form_pairs.size(); i++) {
        cout << "   ";
        cout << "<" << hex << abbreviation.at_form_pairs[i].GetLocationOfPayload() << ">";
        cout.width(30);
        cout << left << DW_TagToString(abbreviation.at_form_pairs[i].GetAT());
        cout.width(22);
        cout << left << DW_TagToString(abbreviation.at_form_pairs[i].GetForm());
        string payload = abbreviation.at_form_pairs[i].PayloadAsString(elf);
        cout << payload;
        cout << endl;
    }
}

DIE ELF::GetNextDie(int &nLevel, CPCHAR &pdata, bool bDwarfIs64)
{
    DIE die;
    die.DecodeThyself(nLevel, pdata, *this, bDwarfIs64);

    if( die.GetTag() EQ DW_TAG_NONE ) {
        // This was a null entry, with an abbreviation code of zero.  Such entries
        // are described in the standard at sections 2.3 and as 7.5.3 as
        // "terminating a chain of sibling entries".
        //
        // I don't know what that means.
        //
        // The DWARF-4 documentation is, to my eye, inconsistent.
        //
        // Section 2.3 seems to be pretty clear:
        //
        //      The tree itself is represented by flattening it in prefix order. Each debugging information entry is
        //      defined either to have child entries or not to have child entries (see Section 7.5.3). If an entry is
        //      defined not to have children, the next physically succeeding entry is a sibling. If an entry is
        //      defined to have children, the next physically succeeding entry is its first child. Additional
        //      children are represented as siblings of the first child. A chain of sibling entries is terminated by a
        //      null entry
        //
        // Section 7.5.2 has this confusing paragraph:
        //
        //      On some architectures, there are alignment constraints on section boundaries. To make it easier
        //      to pad debugging information sections to satisfy such constraints, the abbreviation code 0 is
        //      reserved. Debugging information entries consisting of only the abbreviation code 0 are
        //      considered null entries.
        //
        // My confusion comes because "null entries" seems to be two different things: A way of padding sections
        // to alignment boundaries?  Or to end "A chain of siblings"?
        //
        // Now to 7.5.3, which has a number of references to "abbreviation code 0"
        //
        //      ...As described above, the abbreviation code 0 is reserved for null debugging information entries....
        //
        //      ...(As mentioned in Section 2.3, each chain of sibling entries is terminated by a null entry.)...
        //
        //      ...The abbreviations for a given compilation unit end with an entry consisting of a 0 byte for the
        //      abbreviation code....
        //
        // Put that all together, and I don't see how using zero entries for padding is consistent with using
        // zero entries to "terminate a chain of siblings" and to end the abbreviations for a given compilation
        // unit.
        //
        // One possibility is that zero-padding is always done at the end of the section.  In that case, they
        // just get, effectively, ignored if somebody doesn't use the zero as an section-ending signal and
        // keeps going until the data run out based on section length.
        //
        // It also seems as if the authors may be using "ending a chain of siblings" and "ending the compilation
        // unit" as synonymous, and that there is actually no way to return the tree to a prior parent.
        //
        // But I don't believe that.  I believe that there has to be *some* way to describe a tree that looks
        // like this:
        //                                  A
        //                                 /|\                        
        //                                / | \                       
        //                               /  |  \                      
        //                              B   C   D
        //                             / \     / \                        
        //                            E   F*   G   H
        //
        // So, I am going to take one of their comments literally: "The abbreviations for a given compilation unit
        // end with an entry consisting of a 0 byte for the abbreviation code."  Note that all that states is that
        // there will be a zero entry at the end.  It does not state that the presence of the zero ends the entries.
        //
        // So I am setting up a stack. In the above example, A and B and D are flagged as parents. The '*' means that
        // F was followed by a null entry, ending the E-F chain of siblings.  At that point the stack gets popped, and
        // A replaces B as the "current parent.
        //
        // In the fullness of time, either none of this will matter, or else I'll run across relocation objects in the wild
        // that will settle the question.
        //
        nLevel -= 1;
    }
    return die;
}

void ELF::ReadInElfHeader(ifstream &ifs)
{
    CPROFILER;
    // The header is at the beginning of the file:
    ifs.clear();
    ifs.seekg(0);
    ifs.read((char *)(&elf_header),sizeof(elf_header));
    if(memcmp(elf_header.e_ident, ELFMAG, SELFMAG) != 0) {
        cerr << "The specified ELF input file doesn't have an ELF magic number.\n";
        exit(1);
    }
    if(elf_header.e_ident[EI_CLASS] != ELFCLASS64) {
        cerr << "elf_header.e_ident[EI_CLASS] has to be 2, signifying a 64-bit format.\n";
        exit(1);
    }
    if(elf_header.e_type != ET_REL) {
        cerr << "elf_header.e_type has to be ET_REL, signifying a relocatable object file.\n";
        exit(1);
    }
    if(elf_header.e_phnum != 0) {
        cerr << "elf_header.e_phnum is not zero, which means Dubner is wrong in his assumptions about ET_REL file.\n";
        exit(1);
    }
}

void ELF::WriteOutElfHeader(ofstream &ofs)
{
    CPROFILER;
    elf_header.e_shoff = sizeof(Elf64_Ehdr);
    ofs.write((char *)(&elf_header),sizeof(elf_header));
}

void ELF::ReadInSectionHeaders(ifstream &ifs)
{
    CPROFILER;
    SECTION_HEADER section_header;
    if(elf_header.e_shentsize != sizeof(section_header.section_header)) {
        cerr << "elf_header.e_shentsize != sizeof(section_header.sec_header).  That can't be right.\n";
        exit(1);
    }

    // First, read the headers in.
    ifs.clear();
    ifs.seekg(elf_header.e_shoff);
    for(unsigned short i=0; i<elf_header.e_shnum; i++) {
        ifs.read((char *)(&section_header.section_header), elf_header.e_shentsize);
        section_headers.push_back(section_header);
    }

    // With all the headers in, we can find their names.
    for(USHORT i=0; i<elf_header.e_shnum; i++) {
        ifs.clear();
        ifs.seekg(section_headers[elf_header.e_shstrndx].section_header.sh_offset + section_headers[i].section_header.sh_name);
        section_headers[i].section_header_name.clear();

        for(;;) {
            char ch = ifs.get();
            if(ch EQ '\0') {
                break;
            }
            section_headers[i].section_header_name += ch;
        }
        if(location_of_section_header_by_name.find(section_headers[i].section_header_name) != location_of_section_header_by_name.end()) {
            cerr << section_headers[i].section_header_name << " is a duplicate section_header name.  That can't be right.\n";
            exit(1);
        }
        location_of_section_header_by_name[section_headers[i].section_header_name] = i;
    }
}

void ELF::WriteOutSectionHeaders(ofstream &ofs)
{
    CPROFILER;
    ULONGLONG running_offset = sizeof(Elf64_Ehdr);
    running_offset += elf_header.e_shnum * elf_header.e_shentsize;
    for(USHORT i=0; i<elf_header.e_shnum; i++) {
        Elf64_Shdr *psh = &(section_headers[i].section_header);

        // Because the sizes of some sections have changed, we have to
        // establish the new offsets
        if(psh->sh_type != SHT_NULL) {
            psh->sh_offset = running_offset;
        }
        running_offset += psh->sh_size;
        ofs.write((char *)psh, elf_header.e_shentsize);
    }
}

void ELF::DisplaySectionHeaders(ostream &ofs)
{
    for(USHORT i=0; i<elf_header.e_shnum; i++) {
        ofs.width(3);
        ofs << left << i;

        ofs.width(21);
        ofs << section_headers[i].section_header_name;
        switch(section_headers[i].section_header.sh_type) {
            // *INDENT-OFF*
            case SHT_NULL            : ofs << "SHT_NULL             Section header table entry unused "; break;
            case SHT_PROGBITS        : ofs << "SHT_PROGBITS         Program data "; break;
            case SHT_SYMTAB          : ofs << "SHT_SYMTAB           Symbol table "; break;
            case SHT_STRTAB          : ofs << "SHT_STRTAB           String table "; break;
            case SHT_RELA            : ofs << "SHT_RELA             Relocation entries with addends "; break;
            case SHT_HASH            : ofs << "SHT_HASH             Symbol hash table "; break;
            case SHT_DYNAMIC         : ofs << "SHT_DYNAMIC          Dynamic linking information "; break;
            case SHT_NOTE            : ofs << "SHT_NOTE             Notes "; break;
            case SHT_NOBITS          : ofs << "SHT_NOBITS           Program space with no data (bss) "; break;
            case SHT_REL             : ofs << "SHT_REL              Relocation entries, no addends "; break;
            case SHT_SHLIB           : ofs << "SHT_SHLIB            Reserved "; break;
            case SHT_DYNSYM          : ofs << "SHT_DYNSYM           Dynamic linker symbol table "; break;
            case SHT_INIT_ARRAY      : ofs << "SHT_INIT_ARRAY       Array of constructors "; break;
            case SHT_FINI_ARRAY      : ofs << "SHT_FINI_ARRAY       Array of destructors "; break;
            case SHT_PREINIT_ARRAY   : ofs << "SHT_PREINIT_ARRAY    Array of pre-constructors "; break;
            case SHT_GROUP           : ofs << "SHT_GROUP            Section group "; break;
            case SHT_SYMTAB_SHNDX    : ofs << "SHT_SYMTAB_SHNDX     Extended section indeces "; break;
            // *INDENT-ON*
        }
        ofs << endl;

        for(int f=0; f<12; f++) {
            ULONGLONG flag = section_headers[i].section_header.sh_flags & (1ULL<<f);
            int indent = 24;
            switch(flag) {
                // *INDENT-OFF*
                case SHF_WRITE            : ofs << string(indent,' ') << "SHF_WRITE            Writable "<< endl; break;
                case SHF_ALLOC            : ofs << string(indent,' ') << "SHF_ALLOC            Occupies memory during execution "<< endl; break;
                case SHF_EXECINSTR        : ofs << string(indent,' ') << "SHF_EXECINSTR        Executable "<< endl; break;
                case SHF_MERGE            : ofs << string(indent,' ') << "SHF_MERGE            Might be merged "<< endl; break;
                case SHF_STRINGS          : ofs << string(indent,' ') << "SHF_STRINGS          Contains nul-terminated strings "<< endl; break;
                case SHF_INFO_LINK        : ofs << string(indent,' ') << "SHF_INFO_LINK        'sh_info' contains SHT index "<< endl; break;
                case SHF_LINK_ORDER       : ofs << string(indent,' ') << "SHF_LINK_ORDER       Preserve order after combining "<< endl; break;
                case SHF_OS_NONCONFORMING : ofs << string(indent,' ') << "SHF_OS_NONCONFORMING Non-standard OS specific handling required "<< endl; break;
                case SHF_GROUP            : ofs << string(indent,' ') << "SHF_GROUP            Section is member of a group.  "<< endl; break;
                case SHF_TLS              : ofs << string(indent,' ') << "SHF_TLS              Section hold thread-local data.  "<< endl; break;
                case SHF_COMPRESSED       : ofs << string(indent,' ') << "SHF_COMPRESSED       Section with compressed data. "<< endl; break;
                // *INDENT-ON*
            }
        }
        cout << string(24,' ') << "Fileloc:   0x" << hex << section_headers[i].section_header.sh_offset << endl;
        cout << string(24,' ') << "Size:      " << section_headers[i].section_header.sh_size << endl;
        cout << string(24,' ') << "sh_link:   " << section_headers[i].section_header.sh_link << endl;
        cout << string(24,' ') << "sh_info:   " << section_headers[i].section_header.sh_info << endl;

        ofs << endl;
    }
}

void ELF::ReadInRawSectionData(ifstream &ifs)
{
    CPROFILER;
    for(USHORT i=0; i<elf_header.e_shnum; i++) {
        if(section_headers[i].section_header.sh_size > 0) {
            ifs.clear();
            ifs.seekg(section_headers[i].section_header.sh_offset);

            size_t raw_size = (size_t)section_headers[i].section_header.sh_size;
            char *raw_data = new char[raw_size];
            ifs.read(raw_data,raw_size);

            section_headers[i].SetRawDataPointer(raw_data,raw_size);
        }
    }
}

void ELF::WriteOutRawSectionData(ofstream &ofs)
{
    CPROFILER;
    for(USHORT i=0; i<elf_header.e_shnum; i++) {
        if(section_headers[i].section_header.sh_size > 0) {
            ofs.write(section_headers[i].RawData(),section_headers[i].section_header.sh_size);
        }
    }
}

void ELF::BuildDebugInfo()
{
    CPROFILER;

    debug_info_dies.clear();

    size_t debug_info_section = GetSectionHeaderIndex(".debug_info");
    if(debug_info_section EQ string::npos) {
        cerr << "No \".debug_info\" section in BuildDebugInfo()???\n";
        exit(1);
    }

    CPCHAR pdata = section_headers[debug_info_section].RawData();

    bool bDwarfIs64; // This will be established by GetSectionLength
    ULONGLONG section_length = GetSectionLength(pdata,bDwarfIs64);

    CPCHAR pdata_end = pdata + section_length;
    USHORT version = GetUshort(pdata);
    ULONGLONG debug_abbrev_offset = GetOffset(bDwarfIs64,pdata);
    // In the single Computation Unit that we
    // should see in a .o relocation module (that is:
    // from a -c gcc compilation), debug_abbrev_offset should
    // be zero.
    if(debug_abbrev_offset != 0) {
        cerr << "debug_abbrev_offset is non-zero in ELF::BuildDebugInfo(). Something\n";
        cerr << "is going on that I don't understand.  R.J.Dubner 2019-06-21\n";
        exit(1);
    }

    UBYTE debug_info_address_size = GetUbyte(pdata);

    int nLevel = 0;

    while(pdata < pdata_end) {
        DIE die = GetNextDie(nLevel,pdata,bDwarfIs64);
        debug_info_dies.push_back(die);
    }
}

void ELF::BuildAbbreviationIndex()
{
    CPROFILER;

    // Build the index into the .debug_abbrev entries.  This is
    // *not* premature optimization.  With the index, accessing
    // .debug_abbrev will be a linear-time operation.  Without it,
    // the time will go up as the square of the number of entries.

    location_of_abbreviation_element.clear();

    size_t section_index = GetSectionHeaderIndex(".debug_abbrev");
    SECTION_HEADER *section = &(section_headers[section_index]);

    CPCHAR pdata = section->RawData();
    ULONGLONG data_length = section->section_header.sh_size;
    CPCHAR pdata_end = pdata + data_length;

    // This code assumes that there is one, and only one, Computation Unit (CU)
    // involved here, which means that abbreviation identifiers will not be
    // re-used.  The single CU model is what we see in relocation modules.
    // (Executable modules can have more than one.)
    while(pdata < pdata_end) {
        CPCHAR pstart = pdata;
        ULONGLONG abbreviation = ULEB128Decode(pdata);
        if(abbreviation EQ 0) {
            // We've hit the end of the list
            break;
        }
        // Save the value we came to get:  The abbreviation's starting element

        unordered_map<size_t,size_t>::const_iterator it = location_of_abbreviation_element.find(abbreviation);
        if(it != location_of_abbreviation_element.end()) {
            cerr << ".debug_abbrev is reusing abbreviation ID values: " << abbreviation << endl;
            cerr << "We need to recode to handle that.\n";
            exit(1);
        }

        location_of_abbreviation_element[abbreviation] = pstart - section->RawData();
        // Skip along to the end of this element
        while(pdata < pdata_end) { // Shouldn't be necessary to do this test
            ULONGLONG attribute_type = ULEB128Decode(pdata);
            ULONGLONG form = ULEB128Decode(pdata);
            if(attribute_type EQ 0 AND form EQ 0) {
                break;
            }
        }
    }
}

char const *ELF::GetAbbreviationLocation(ULONGLONG abbreviation) const
{
    const char *retval = nullptr;
    std::unordered_map<size_t,size_t>::const_iterator it = location_of_abbreviation_element.find(abbreviation);
    if(it != location_of_abbreviation_element.end()) {
        retval = it->second + GetSectionDataLocation(".debug_abbrev");
    }
    return retval;
}

void ELF::DumpEntireDebugInfoDieTree()
{
    for( DIE die : debug_info_dies ) {
        die.Show(*this);
    }
}

void ELF::RunLineStateMachine(bool show_your_work)
{
    lns_state_machine.Run(section_headers[GetSectionHeaderIndex(".debug_line")].RawData(),show_your_work);
}

void ELF::ApplyRelocations(int plus_or_minus)
{
    bool show_your_work = false;
    /*  As a convenience, I am including the relevant relocation information from

    System V Application Binary Interface
    AMD64 Architecture Processor Supplement
    (With LP64 and ILP32 Programming Models)
    Version 1.0
    AMD64 ABI 1.0 – December 22, 2018 – 6:43

    A   Represents the addend used to compute the value of the relocatable field.
    B   Represents the base address at which a shared object has been loaded into memory
    during execution. Generally, a shared object is built with a 0 base virtual address,
    but the execution address will be different.
    G   Represents the offset into the global offset table at which the relocation entry’s symbol
    will reside during execution.
    GOT Represents the address of the global offset table.
    L   Represents the place (section offset or address) of the Procedure Linkage Table entry
    for a symbol.
    P   Represents the place (section offset or address) of the storage unit being relocated (computed
    using r_offset).
    S   Represents the value of the symbol whose index resides in the relocation entry.
    Z   Represents the size of the symbol whose index resides in the relocation entry.

    Note that wordclass means either 32-bit or 64-bit depending on the model. We are
    assuming 64 bits.

    Table 4.9: Relocation Types
    Name                    Value   Field       Calculation
    R_X86_64_NONE           0       none        none
    R_X86_64_64             1       word64      S + A
    R_X86_64_PC32           2       word32      S + A - P
    R_X86_64_GOT32          3       word32      G + A
    R_X86_64_PLT32          4       word32      L + A - P
    R_X86_64_COPY           5       none        none
    R_X86_64_GLOB_DAT       6       wordclass   S
    R_X86_64_JUMP_SLOT      7       wordclass   S
    R_X86_64_RELATIVE       8       wordclass   B + A
    R_X86_64_GOTPCREL       9       word32      G + GOT + A - P
    R_X86_64_32             10      word32      S + A
    R_X86_64_32S            11      word32      S + A
    R_X86_64_16             12      word16      S + A
    R_X86_64_PC16           13      word16      S + A - P
    R_X86_64_8              14      word8       S + A
    R_X86_64_PC8            15      word8       S + A - P
    R_X86_64_DTPMOD64       16      word64      See reference for thread-local storage
    R_X86_64_DTPOFF64       17      word64
    R_X86_64_TPOFF64        18      word64
    R_X86_64_TLSGD          19      word32
    R_X86_64_TLSLD          20      word32
    R_X86_64_DTPOFF32       21      word32
    R_X86_64_GOTTPOFF       22      word32
    R_X86_64_TPOFF32        23      word32
    R_X86_64_PC64 y         24      word64      S + A - P
    R_X86_64_GOTOFF64 y     25      word64      S + A - GOT
    R_X86_64_GOTPC32        26      word32      GOT + A - P
    R_X86_64_SIZE32         32      word32      Z + A
    R_X86_64_SIZE64 y       33      word64      Z + A
    R_X86_64_GOTPC32_TLSDESC 34     word32
    R_X86_64_TLSDESC_CALL   35      none
    R_X86_64_TLSDESC        36      word64 x 2
    R_X86_64_IRELATIVE      37      wordclass   indirect (B + A)
    R_X86_64_RELATIVE64 yy  38      word64      B + A
    Deprecated              39
    Deprecated              40
    R_X86_64_GOTPCRELX      41 word32           G + GOT + A - P
    R_X86_64_REX_GOTPCRELX  42 word32           G + GOT + A - P
    y This relocation is used only for LP64.
    yy This relocation only appears in ILP32 executable files or shared objects.

    */
    CPROFILER;
    if(plus_or_minus EQ -1) {
        //return;
    }
    if(plus_or_minus != 1 AND plus_or_minus != -1) {
        cerr << "plus_or_minus in ELF::ApplyRelocations() has to be either +1 or -1\n";
        exit(1);
    }

    for(USHORT i=0; i<elf_header.e_shnum; i++) {
        if(section_headers[i].section_header.sh_type EQ SHT_RELA OR section_headers[i].section_header.sh_type EQ SHT_REL ) {
            SECTION_HEADER *sreloc    = &section_headers[i];                                // Relocation info
            SECTION_HEADER *ssymbols  = &section_headers[  sreloc->section_header.sh_link]; // Symbol table
            SECTION_HEADER *starget   = &section_headers[  sreloc->section_header.sh_info]; // Target
            SECTION_HEADER *sstrings  = &section_headers[ssymbols->section_header.sh_link]; // strings

            Elf64_Sym *symbol_table = (Elf64_Sym *)ssymbols->RawData();

            if( starget->section_header_name EQ ".debug_info" ) {
                show_your_work = false;
            } else {
                show_your_work = false;
            }

            if( show_your_work ) {
                cout << sreloc->section_header_name << " applies ";
                cout << ssymbols->section_header_name << " to ";
                cout << starget->section_header_name;
                cout << "; The symbol strings are in ";
                cout << sstrings->section_header_name;
                cout << endl;
            }

            bool first_time = true;
            for(int rel_entry = 0; rel_entry<sreloc->section_header.sh_size/sreloc->section_header.sh_entsize; rel_entry++) {
                // Converting .rel entries to .rela will simplify subsequent coding; we just have to
                // do this once:
                Elf64_Rela relocation_entry;
                if(sreloc->section_header.sh_type = SHT_RELA) {
                    // Pick up the relative entry
                    relocation_entry = *(Elf64_Rela *)(sreloc->RawData() + rel_entry * sizeof(Elf64_Rela));
                } else {
                    // Pick up the absolute entry
                    Elf64_Rel rel = *(Elf64_Rel *)(sreloc->RawData() + rel_entry * sizeof(Elf64_Rel));
                    relocation_entry.r_offset = rel.r_offset;
                    relocation_entry.r_info = rel.r_info;
                    relocation_entry.r_addend = 0;
                }

                int symbol_index = ELF64_R_SYM(relocation_entry.r_info);

                Elf64_Sym symbol = symbol_table[symbol_index];

                int relocation_type = ELF64_R_TYPE(relocation_entry.r_info);

                if( show_your_work ) {
                    if(first_time) {
                        first_time = false;
                        cout << "Relocation Offset Symbol Index Relocation Type Addend\n";
                    }
                    cout.width(strlen("Relocation Offset"));
                    cout << hex << right << relocation_entry.r_offset;
                    cout.width(strlen(" Symbol Index"));
                    cout << right << symbol_index;
                    cout.width(strlen(" Relocation Type"));
                    cout << right << relocation_type;
                    cout.width(strlen(" Addend"));
                    cout << hex << right << relocation_entry.r_addend;
                    cout << "   ";
                    cout << sstrings->GetSymbolByIndex(symbol.st_name);
                    cout << endl;
                }

                LONGLONG A = relocation_entry.r_addend;
                LONGLONG S = symbol_table[symbol_index].st_value;
                LONGLONG P = relocation_entry.r_offset;

                LONGLONG value = 0;

                char *ptarget = starget->RawData() + relocation_entry.r_offset;

                switch(relocation_type) {
                case 1:
                    value = S + A;      // 64-bit
                    value *= plus_or_minus;
                    *(LONGLONG *)ptarget += value;
                    break;
                case 2:
                    // Not needed by this program, because we are debugging-focused.
                    break;
                case 4:
                    // Not needed by this program, because we are debugging-focused.
                    break;
                case 10:
                    value = S + A;      // 32-bit
                    value *= plus_or_minus;
                    *(LONG *)ptarget += static_cast<LONG>(value);
                    break;
                case 42:
                    // Not needed by this program, because we are debugging-focused.
                    break;
                default:
                    cerr << "Unknown relocation_type seen in ELF::ApplyRelocations()\n";
                    cerr << "Appropriate action must be taken by the developers.\n";
                    exit(1);
                    break;
                }
            }
        }
    }
}

void ELF::ReadCobolSymbolTable(const std::string &cobst_filename)
{
    CPROFILER;
    cobst_symbols.ReadFrom(cobst_filename);
}

void ELF::ReplaceDebugLineSection()
{
    CPROFILER;

    /*  Assuming we've already read in the cobst symbols, let's use the line data
        from there to build a new .debug_line section: */

    LNS_BUILDER lns_builder;

    // Empirically, I have noted that COBOL programs end up with the delta line
    // usually being 1, and there are fairly large jumps in relative address.
    // Hence the choice of 0,2, below, which gives us operation_advance of 0 through 121
    // with a line advance of 0 or 1
    lns_builder.SetParams(  1,      // minimum_instruction_length;
                            1,      // maximum_operations_per_instruction
                            0,      // line_bound 0
                            2,      // line_range 2
                            lns_state_machine.GetOpcodeBase(),        // opcode_base
                            true,                                     // default_is_stmt
                            lns_state_machine.StandardOpcodeLengths() // standard_opcode_lengths
                         );

    for(size_t i=0; i<cobst_symbols.size(); i++) {
        const COBST_SYMBOL cs = cobst_symbols[i];

        ULONGLONG address = cs.address;
        ULONGLONG line = cs.line_number;

        /*  In some programs, the GnuCOBOL compiler generates code ostensibly for "Line: 0". So
            far, such code seems to be the very last "Line:" generated.  It's at the end of the
            file for a "default error handler".  When allowed through, it messes things up.  And
            it shouldn't be there, anyway; how would gdb, for example, display the code at
            "Line 0"?  So, we filter it out here.  */

        if(line != 0) {
            int file_number = lns_builder.AddFile(cobst_symbols[i].file);
            lns_builder.AddToAddressLineList(address,file_number,line);
        }
    }

    // this next call creates an entire .debug_line section from
    // the parameters and address/line pairs we've provided.
    vector<UBYTE>line_number_program = lns_builder.BuildProgram();

    // Okey dokey.  We have a brand spanking new .debug_line data section.
    // Let's use it to replace our old one:

    size_t debug_line_section_index = GetSectionHeaderIndex(".debug_line");
    SECTION_HEADER *dls = &section_headers[debug_line_section_index];

    const char  *newdata = lns_builder.GetLineProgramData();
    const size_t newsize = (size_t)lns_builder.GetLineProgramSize();

    char *newprogram = new char[newsize];
    memcpy(newprogram,newdata,newsize);
    dls->SetRawDataPointer(newprogram,newsize);

    // Okay, the .debug_line section is modified and ready to go.

    // One last thing: we need to adjust the single relocation entry in
    // .rela_debug_line

    size_t rela_debug_line_section_index = GetSectionHeaderIndex(".rela.debug_line");
    SECTION_HEADER *rdl = &section_headers[rela_debug_line_section_index];

    char *prdl = rdl->RawData();
    ULONGLONG relocation_address_offset = lns_builder.GetRelocationAddressOffset();

    // Assume there is one, and only one, Elf64 .RELA entry.  The first eight bytes
    // are the r_offset value, and that's what we need to change:

    *(ULONGLONG *)prdl = relocation_address_offset;

    // And, that's that.  To recap, we have completely replaced the incoming
    // .debug_line section with one derived from the output of the 'cobst'
    // COBOL Symbol Table program, and we just updated the .rela.debug_line
    // relocation information that modifies the very first SET_ADDRESS
    // instruction in the Line Number Program state machine.
}

ULONGLONG ELF::AddToStringTable(const std::string &sec_name,const std::string &text)
{
    size_t section_index = GetSectionHeaderIndex(sec_name);
    if( section_index EQ string::npos ) {
        cerr << "He's trying to add something to the non-existant section\n";
        cerr << sec_name << " in ELF::AddToStringTable()" << endl;
        exit(1);
    }
    // Pick up the existing buffer size and location
    ULONG old_size = ULONG(section_headers[section_index].section_header.sh_size);

    section_headers[section_index].AppendToRawData(text.c_str(),text.size()+1);

    return old_size;
}


void ELF::CreateSection(const std::string &sec_name,
                        ULONGLONG type,
                        ULONGLONG flags )
{
    size_t script_section_index = GetSectionHeaderIndex(sec_name);
    if( script_section_index EQ string::npos ) {
        ULONGLONG offset = AddToStringTable(".shstrtab",sec_name);
        SECTION_HEADER section_header;

        section_header.section_header.sh_name = (ULONG)offset;
        section_header.section_header.sh_type = (ULONG)type;
        section_header.section_header.sh_flags = flags;
        section_header.section_header.sh_addr = 0;
        section_header.section_header.sh_offset = 0;
        section_header.section_header.sh_size = 0;
        section_header.section_header.sh_link = 0;
        section_header.section_header.sh_info = 0;
        section_header.section_header.sh_addralign = 0;
        section_header.section_header.sh_entsize = 0;

        section_headers.push_back(section_header);
        location_of_section_header_by_name[sec_name] = section_headers.size()-1;

        elf_header.e_shnum += 1;
    }
}

#if 0
void ELF::InsertDebugScript(const std::string &python_script)
{
    // This version puts in just the name
    // We're going to <add python_script> to the .debug_gdb_scripts section
    const string script_sec = ".debug_gdb_scripts";

    // Make sure we have a section
    CreateSection(script_sec,SHT_PROGBITS,0);
    string strange_string = "\x01";
    strange_string += python_script;
    AddToStringTable(script_sec,strange_string);
}
#else
void ELF::InsertDebugScript(const std::string &python_script)
{
    // This version embeds the entire cprint.py program as a text string
    // We're going to <add python_script> to the .debug_gdb_scripts section
    const string script_sec = ".debug_gdb_scripts";

    char *embedded = new char[___python_cprint_py_len + 1];
    memcpy(embedded,___python_cprint_py,___python_cprint_py_len);
    embedded[___python_cprint_py_len] = '\0';

    // Make sure we have a section
    CreateSection(script_sec,SHT_PROGBITS,0);
    string strange_string = "\x04";
    strange_string += python_script;
    strange_string += "\n";
    strange_string += embedded;
    AddToStringTable(script_sec,strange_string);
    delete[] embedded;
}
#endif

void ELF::ReadFrom(string object_filename)
{
    CPROFILER;
    ifstream ifs;
    OpenOrFail(ifs,object_filename,ifstream::in | ifstream::binary);
    ReadInElfHeader(ifs);
    ReadInSectionHeaders(ifs);
    ReadInRawSectionData(ifs);
    ApplyRelocations(+1);   // This has to be done before BuildingDebugInfo
    BuildAbbreviationIndex();
    BuildDebugInfo();
}

void ELF::WriteTo(string object_filename)
{
    CPROFILER;

    /*  ReadFrom() followed immediately by WriteTo() should result in an output
    file that is functionally equivalent to the input file.  That how I tested
    them early in development.

    It won't come out actually identical.  It turns out that a typical gcc-generated .o file's
    data sections in the file are not physically arranged in the same physical order as the
    section headers in the section header table.  In other words: the sh_offset values for
    the various sections do not increase monotonically with the section header index number.

    This program reads them in in section header order, and outputs them in that same
    order.  The resulting_offsets will, therefore, increase monotonically.

    Be apprised that the file lengths may not be the same, either.  One sample gcc-generated file
    had four unexplained bytes in between twp sections, and had close to 3,000 unexplained
    bytes after the last section.  This program will strip all padding out.
    */

    ofstream ofs;
    OpenOrFail(ofs,object_filename,ifstream::out | ifstream::binary);

    ApplyRelocations(-1);   // Setting the parameter to -1 causes the relocations to be undone.
    WriteOutElfHeader(ofs);
    WriteOutSectionHeaders(ofs);
    WriteOutRawSectionData(ofs);
}

void ELF::ReplaceASymbol(const string &section, const string &was, const string &is, bool whole_symbol)
{
    size_t section_header_index = GetSectionHeaderIndex(section);
    char * const symstart = GetSectionDataLocationDangerous(section);
    char *symbols = symstart;

    symbols += 1;    // Skip past the first null

    while((ULONGLONG)(symbols-symstart) < section_headers[section_header_index].section_header.sh_size) {
        if(*symbols EQ '\0') {
            // We are done; never found our symbol
            break;
        }
        string symbol = symbols;

        size_t length = whole_symbol ? symbol.size() : was.size();

        if(symbol.substr(0,length) EQ was) {
            // Replace symbol with new symbol:
            memcpy(symbols,is.c_str(),is.size());
            cout << "ELF::ReplaceASymbol(): replaced "
                 << was << " with " << is << endl;
            break;
        }
        symbols += symbol.size()+1;
    }
}

void ELF::BruteForceReplace(const string &section, const string &was, const string &is)
{
    size_t section_header_index = GetSectionHeaderIndex(section);
    char *const dstart = GetSectionDataLocationDangerous(section);
    char *data = dstart;

    // Pure brute force:
    size_t i=0;
    size_t len=was.length();

    while((ULONGLONG)(data-dstart) < section_headers[section_header_index].section_header.sh_size) {
        char ch = *data++;
        if(ch != was[i++]) {
            // Wrong word.  Reset, keep looking
            i = 0;
            continue;
        }
        if(i != len) {
            // we haven't finished looking
            continue;
        }
        // we have matched the letters of was.  If the next character
        // is a null, then we've found our string
        if(*data++ != '\0') {
            i = 0;
            continue;
        }

        cout << "ELF::BruteForceReplace(): replaced "
             << was << " with " << is << endl;

        memcpy(data - (len+1),is.c_str(),len);
        continue;
    }
}

size_t ELF::AddToAbbreviations(ABBREVIATION &abbreviation)
{
    SECTION_HEADER *abbrevs = GetSectionHeaderDangerous(".debug_abbrev");

    // Figure out the sequence number of this new element.  The elements
    // are numbered from 1, so we have to add 1 at this point
    abbreviation.sequence_number = location_of_abbreviation_element.size()+1;

    // Adjust our finder table to reflect the abbreviation we're about to build
    location_of_abbreviation_element[abbreviation.sequence_number]
        = abbrevs->section_header.sh_size;

    char ach[1024]; // Ain't no abbreviation gonna be this big.

    PCHAR pdata = ach;

    // Turn the abbreviation into a byte stream;
    ULEB128Encode(abbreviation.sequence_number,pdata);
    ULEB128Encode(abbreviation.dw_tag,pdata);
    *pdata++ = (char)(abbreviation.has_children ?
                      DW_CHILDREN_yes
                      : DW_CHILDREN_no);
    for( ATTRIBUTE_FORMAT_PAIR afp : abbreviation.at_form_pairs ) {
        ULEB128Encode(afp.GetAT(),pdata);
        ULEB128Encode(afp.GetForm(),pdata);
    }
    // Put in a pair of zeroes to end the abbreviation
    ULEB128Encode(0,pdata);
    ULEB128Encode(0,pdata);

    // Put in a zero to end the list of abbreviations
    *pdata++ = 0;

    // Calculate the size of this entry
    size_t nbytes = pdata - ach;

    // Arrange to overwrite the zero that ended the existing list
    abbrevs->TrimRawData(1);

    // Append our new entry, complete with terminating zero
    abbrevs->AppendToRawData(ach,nbytes);
    return abbreviation.sequence_number;
}

void SingleExpressionAddress(PCHAR &pdata,LONGLONG address,bool bTarget64)
{
    if(bTarget64 AND address >= 0) {
        *pdata++ = 9;
        *pdata++ = DW_OP_addr;
        *(LONGLONG *)pdata = address;
        pdata += sizeof(LONGLONG);
    } else if(!bTarget64 AND address >= 0) {
        *pdata++ = 5;
        *pdata++ = DW_OP_addr;
        *(LONG *)pdata = (LONG)address;
        pdata += sizeof(LONG);
    } else {
        vector<UBYTE> databytes = LEB128Encode(address);
        *pdata++ = (char)databytes.size()+1;
        *pdata++ = (char)DW_OP_fbreg;
        memcpy(pdata,databytes.data(),databytes.size());
        pdata += databytes.size();
    }
}

void ELF::AugmentSymbolTable()
{
    // These are the section headers we're going to be manipulating

    const char string_section[] = ".debug_str";

    SECTION_HEADER *relocs     = GetSectionHeaderDangerous(".rela.debug_info");
    SECTION_HEADER *strings    = GetSectionHeaderDangerous(string_section);
    SECTION_HEADER *abbrevs    = GetSectionHeaderDangerous(".debug_abbrev");
    SECTION_HEADER *debug_info = GetSectionHeaderDangerous(".debug_info");

    // We need to find a typical relocation entry.  I have
    // been unsuccessful in completely figuring them out, so I am reduced
    // to mimicry.
    Elf64_Rela relocation_prototype;
    for( size_t i=0; i<relocs->section_header.sh_size/relocs->section_header.sh_entsize; i++ ) {
        const Elf64_Rela rela = *(Elf64_Rela *)(relocs->RawData() + i * relocs->section_header.sh_entsize);
        const LONGLONG r_addend = rela.r_addend;
        const ULONG type = ELF64_R_TYPE(rela.r_info);
        if(r_addend != 0 AND type EQ 0xa) {
            relocation_prototype = rela;
            break;
        }
    }

    // Our WORKING-STORAGE and LINKAGE symbols are referenced against a b_xx
    // c_sym symbol.  Let's scan debug_info once and build a map of what
    // we're going to need.

    unordered_map<string,LONGLONG>c_sym_to_location;
    for( DIE die : debug_info_dies ) {
        DW_TAG dw_tag = die.GetTag();
        if( dw_tag EQ DW_TAG_formal_parameter
                OR
                dw_tag EQ DW_TAG_variable ) {
            // We have found a tag or a formal parameter.  Fetch the string
            // associated with it.
            string c_sym;
            LONGLONG location;
            bool gotsym = false;
            bool gotloc = false;
            for( size_t i=0; i<die.Size(); i++ ) {
                const ATTRIBUTE_FORMAT_PAIR afp = die[i];
                const DW_AT dw_at = afp.GetAT();
                const DW_FORM dw_form = afp.GetForm();

                if( dw_at EQ DW_AT_name) {
                    gotsym = true;
                    c_sym = afp.PayloadAsString(*this,false);
                }
                if( dw_at EQ DW_AT_location) {
                    gotloc = true;
                    CPCHAR p = (CPCHAR)afp.PayloadBytes();
                    location = LNS_STATE_MACHINE::ProcessSingleExpression(p,IsTarget64());
                }
                if(gotloc AND gotsym) {
                    c_sym_to_location[c_sym] = location;
                }
            }
        }
    }

    // We need to create a new base type called 'cob_variable'

    // We have to put cob_variable into the string table, which means an
    // associated relocation entry:

    size_t location_of_cob_variable_string = AddToStringTable(string_section,"cob_variable");

    // Now that we have the location of the string, we can create a .debug_abbrev
    // entry that will be used for cob_variable base_type.  This abbreviation
    // undoubtedly already exists in the abbreviation list, but rather than
    // find it, I got lazy and built a new one.

    ABBREVIATION abbreviation;
    ATTRIBUTE_FORMAT_PAIR afp;
    size_t size;

    abbreviation.Initialize();
    abbreviation.dw_tag = DW_TAG_base_type;
    abbreviation.has_children = false;
    //
    afp.SetAT(DW_AT_byte_size);
    afp.SetForm(DW_FORM_data1);
    abbreviation.at_form_pairs.push_back(afp);
    //
    afp.SetAT(DW_AT_encoding);
    afp.SetForm(DW_FORM_data1);
    abbreviation.at_form_pairs.push_back(afp);
    //
    afp.SetAT(DW_AT_name);
    afp.SetForm(DW_FORM_strp);
    abbreviation.at_form_pairs.push_back(afp);
    //
    size_t abbreviation_index_of_base_type = AddToAbbreviations(abbreviation);

    // We now need to create an abbreviation for the cob_variable type for
    // ordinary variables, that is, working-storage or local-storage

    abbreviation.Initialize();
    abbreviation.dw_tag = DW_TAG_variable;
    abbreviation.has_children = false;
    //
    afp.SetAT(DW_AT_name);
    afp.SetForm(DW_FORM_strp);
    abbreviation.at_form_pairs.push_back(afp);
    //
    afp.SetAT(DW_AT_type);
    afp.SetForm(DW_FORM_ref4);
    abbreviation.at_form_pairs.push_back(afp);
    //
    afp.SetAT(DW_AT_location);
    afp.SetForm(DW_FORM_exprloc);
    abbreviation.at_form_pairs.push_back(afp);
    //
    size_t abbreviation_index_of_variable = AddToAbbreviations(abbreviation);

    // We now need to create an abbreviation for the cob_variable type for
    // formal parameters:

    abbreviation.Initialize();
    abbreviation.dw_tag = DW_TAG_formal_parameter;
    abbreviation.has_children = false;
    //
    afp.SetAT(DW_AT_name);
    afp.SetForm(DW_FORM_strp);
    abbreviation.at_form_pairs.push_back(afp);
    //
    afp.SetAT(DW_AT_type);
    afp.SetForm(DW_FORM_ref4);
    abbreviation.at_form_pairs.push_back(afp);
    //
    afp.SetAT(DW_AT_location);
    afp.SetForm(DW_FORM_exprloc);
    abbreviation.at_form_pairs.push_back(afp);
    //
    size_t abbreviation_index_of_formal_parameter = AddToAbbreviations(abbreviation);

    // We can now create the DIE for the cob_variable base type:

    char ach[1024]; // I admit it.  Terrible programming practice.

    // First, trim off the final Abbrev Number 0 entry in the data. We have
    // to do this before calculating the offset into the data for the
    // indirect string relocation.
    debug_info->TrimRawData(1);

    PCHAR pdata = ach;

    // This code is tightly linked to the definition, up above, for
    // abbreviation_index_of_base_type.  Change one; change the other.  If you
    // don't, the entire .debug_info section will become higgledy-piggledy.
    // You Have Been Warned.

    // Start with the abbreviation index
    ULEB128Encode(abbreviation_index_of_base_type,pdata);
    // Follow with the byte size
    *pdata++ = 1;
    // Follow with the code for unsigned char
    *pdata++ = DW_ATE_unsigned_char;
    // And finally the pointer to the string

    // First, calculate where the pointer is going to go in .debug_info; we
    // need it for the relocation table entry we need to create.
    size_t offset_for_relocation = debug_info->section_header.sh_size + (pdata-ach) ;

    // And now incorporate the location
    *(ULONG *)pdata = (ULONG)location_of_cob_variable_string;
    pdata += sizeof(ULONG);

    // We'll need the offset of the base_type for the DW_AT_type variable
    // of the DW_TAG_variable values we'll be creating:
    size_t offset_of_base_type = debug_info->section_header.sh_size;

    // Place a new Abbreviation 0 entry at the end of the data:
    *pdata++ = 0;

    // We can now incorporate this new information into .debug_info
    size = pdata - ach ;
    debug_info->AppendToRawData(ach,size);

    // Along with that .debug_info, which contains an indirect string pointer,
    // we have to create a matching .rela.debug_info entry:

    relocation_prototype.r_offset = offset_for_relocation;
    relocation_prototype.r_addend = location_of_cob_variable_string;
    relocs->AppendToRawData((char *)&relocation_prototype,sizeof(relocation_prototype));

    for( size_t i = 0;
            i < cobst_symbols.size();
            i++ ) {
        const COBST_SYMBOL cobst_symbol = cobst_symbols[i];

        if( cobst_symbol.section EQ "WORKING-STORAGE" ) {
            const string full_name = cobst_symbol.full_name;
            string base_symbol = cobst_symbol.base_symbol;
            const ULONGLONG offset = cobst_symbol.offset;

            unordered_map<string,LONGLONG>::const_iterator it =
                c_sym_to_location.find(base_symbol);

            if( it != c_sym_to_location.end() ) {
                LONGLONG relocatable_location = it->second + offset;

                //cout << cobst_symbol.section;
                //cout << " ";
                //cout << full_name;
                //cout << " ";
                //cout << base_symbol;
                //cout << " ";
                //cout << offset;
                //cout << " ";
                //cout << relocatable_location;
                //cout << endl;

                // We need to add this variable to the debug_str table:
                size_t location_of_variable_name
                    = AddToStringTable(string_section,full_name);

                // We need to create a DW_TAG_variable for this variable:
                debug_info->TrimRawData(1); // Remove Abbrev: 0 at end
                pdata = ach;
                ULEB128Encode(abbreviation_index_of_variable,pdata);
                offset_for_relocation = debug_info->section_header.sh_size + (pdata-ach) ;
                *(ULONG *)pdata = (ULONG)location_of_variable_name;
                pdata += sizeof(ULONG);
                *(ULONG *)pdata = (ULONG)offset_of_base_type;
                pdata += sizeof(ULONG);
                SingleExpressionAddress(pdata,relocatable_location,IsTarget64());
                *pdata++ = 0;   // Replace Abbrev: 0 at end
                size = pdata - ach ;
                debug_info->AppendToRawData(ach,size);

                // And we also need a relocation entry for the variable name:
                relocation_prototype.r_offset = offset_for_relocation;
                relocation_prototype.r_addend = location_of_variable_name;
                relocs->AppendToRawData((char *)&relocation_prototype,sizeof(relocation_prototype));
            }
        }

        if( cobst_symbol.section EQ "LINKAGE" ) {
            const string full_name = cobst_symbol.full_name;
            string base_symbol = cobst_symbol.base_symbol;
            const ULONGLONG offset = cobst_symbol.offset;

            unordered_map<string,LONGLONG>::const_iterator it =
                c_sym_to_location.find(base_symbol);

            if( it != c_sym_to_location.end() ) {
                LONGLONG relocatable_location = it->second + offset;

                //cout << cobst_symbol.section;
                //cout << " ";
                //cout << full_name;
                //cout << " ";
                //cout << base_symbol;
                //cout << " ";
                //cout << offset;
                //cout << " ";
                //cout << relocatable_location;
                //cout << endl;

                // We need to add this variable to the debug_str table:
                size_t location_of_variable_name
                    = AddToStringTable(string_section,full_name);

                // We need to create a DW_TAG_variable for this variable:
                debug_info->TrimRawData(1); // Remove Abbrev: 0 at end
                pdata = ach;
                ULEB128Encode(abbreviation_index_of_formal_parameter,pdata);
                offset_for_relocation = debug_info->section_header.sh_size + (pdata-ach) ;
                *(ULONG *)pdata = (ULONG)location_of_variable_name;
                pdata += sizeof(ULONG);
                *(ULONG *)pdata = (ULONG)offset_of_base_type;
                pdata += sizeof(ULONG);
                SingleExpressionAddress(pdata,relocatable_location,IsTarget64());
                *pdata++ = 0;   // Replace Abbrev: 0 at end
                size = pdata - ach ;
                debug_info->AppendToRawData(ach,size);

                // And we also need a relocation entry for the variable name:
                relocation_prototype.r_offset = offset_for_relocation;
                relocation_prototype.r_addend = location_of_variable_name;
                relocs->AppendToRawData((char *)&relocation_prototype,sizeof(relocation_prototype));
            }
        }
        if( cobst_symbol.section EQ "LOCAL-STORAGE" ) {
            const string full_name = cobst_symbol.full_name;

            LONGLONG relocatable_location = -1; // Flag that this is LOCAL-STORAGE

            //cout << cobst_symbol.section;
            //cout << " ";
            //cout << full_name;
            //cout << " ";
            //cout << relocatable_location;
            //cout << endl;

            // We need to add this variable to the debug_str table:
            size_t location_of_variable_name
                = AddToStringTable(string_section,full_name);

            // We need to create a DW_TAG_variable for this variable:
            debug_info->TrimRawData(1); // Remove Abbrev: 0 at end
            pdata = ach;
            ULEB128Encode(abbreviation_index_of_formal_parameter,pdata);
            offset_for_relocation = debug_info->section_header.sh_size + (pdata-ach) ;
            *(ULONG *)pdata = (ULONG)location_of_variable_name;
            pdata += sizeof(ULONG);
            *(ULONG *)pdata = (ULONG)offset_of_base_type;
            pdata += sizeof(ULONG);
            SingleExpressionAddress(pdata,relocatable_location,IsTarget64());
            *pdata++ = 0;   // Replace Abbrev: 0 at end
            size = pdata - ach ;
            debug_info->AppendToRawData(ach,size);

            // And we also need a relocation entry for the variable name:
            relocation_prototype.r_offset = offset_for_relocation;
            relocation_prototype.r_addend = location_of_variable_name;
            relocs->AppendToRawData((char *)&relocation_prototype,sizeof(relocation_prototype));
        }
    }
}