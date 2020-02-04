#include "c_elf.h"
#include "die.h"
#include "profiler.h"

#define EQ ==
#define AND &&
#define OR ||

using namespace std;

string ATTRIBUTE_FORMAT_PAIR::PayloadAsString(const ELF &elf,bool with_offset) const
{
    CPROFILER;
    string retval;

    // Use the size of payload as a default:
    ULONGLONG nLength = payload.size();
    LONGLONG exprloc=0x87654321;
    switch(dw_form) {
    case DW_FORM_addr:
        break;
    case DW_FORM_block2:
        break;
    case DW_FORM_block4:
        break;
    case DW_FORM_data2:
        break;
    case DW_FORM_data4:
        break;
    case DW_FORM_data8:
        break;
    case DW_FORM_string:
        // Zero-terminated string of 8-bit characters
        for(size_t i=0; i<payload.size()-1; i++) {
            retval += payload[i];
        }
        nLength = 0;
        break;
    case DW_FORM_block:
        break;
    case DW_FORM_block1:
        break;
    case DW_FORM_data1:
        break;
    case DW_FORM_flag:
        break;
    case DW_FORM_sdata:
        break;
    case DW_FORM_strp: {
        LONGLONG offset = 0;
        for(size_t i=0; i<payload.size(); i++) {
            offset += payload[i] << (i << 3); // I couldn't resist.  It's just (i*8)
        }

        if( offset EQ 0x1891 ) {
            cout << "Offset is " << hex << offset << endl;
        }

        const char *p = elf.GetSectionDataLocation(".debug_str");
        p += offset;
        if(with_offset) {
            char ach[128];
            sprintf(ach,"(0x%llx) ",(long long)offset);
            retval += ach;
        }
        retval += p;
        nLength = 0;
        break;
    }
    case DW_FORM_udata:
        break;
    case DW_FORM_ref_addr:
        break;
    case DW_FORM_ref1:
        break;
    case DW_FORM_ref2:
        break;
    case DW_FORM_ref4:
        break;
    case DW_FORM_ref8:
        break;
    case DW_FORM_ref_udata:
        break;
    case DW_FORM_indirect:
        // This is whacked out.  The initial payload is a DW_FORM code that
        // has to be processed:
        cerr << "I don't understand how DW_FORM_indirect can be found in AT_FORM_PAIR::PayloadAsString()\n";
        exit(1);
        break;
    case DW_FORM_sec_offset:
        break;
    case DW_FORM_exprloc: {
        if(dw_at EQ DW_AT_location ) {
            CPCHAR p = (CPCHAR)payload.data();
            exprloc = LNS_STATE_MACHINE::ProcessSingleExpression(p,elf.IsTarget64());
        }
        break;
    }
    case DW_FORM_flag_present:
        // This FORM has no actual data
        retval = "flag_present";
        break;
    case DW_FORM_ref_sig8:
        break;
    default:
        cerr << "switch(afp.GetForm()) processing an unknown value in AT_FORM_PAIR::PayloadAsString().\n";
        cerr << "This needs to be understood.\n";
        break;
    }
    if(nLength > 0) {
        retval = "0x";
        for(int i=(int)payload.size()-1; i>=0; i--) {
            UBYTE hi = payload[i]>>4;
            if(hi < 0x0a) {
                retval += '0'+hi;
            } else {
                retval += 'a'-0x0a+hi;
            }
            UBYTE lo = payload[i] & 0xF;
            if(lo < 0x0a) {
                retval += '0'+lo;
            } else {
                retval += 'a'-0x0a+lo;
            }
        }
    }

    if( dw_at EQ DW_AT_location AND dw_form EQ DW_FORM_exprloc ) {
        char ach[128];
        sprintf(ach," (0x%16.16llX)",(long long)exprloc);
        retval += ach;
    }

    return retval;
}

