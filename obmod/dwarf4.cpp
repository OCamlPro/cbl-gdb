
#include "dwarf4.h"
#include "profiler.h"
#include "utils.h"

#define EQ ==
#define AND &&
#define OR ||

using namespace std;

LONGLONG LEB128Decode(CPCHAR &p)
{
    LONGLONG result = 0;
    int shift = 0;
    const int size = 63;
    ULONGLONG byte;
    while(true) {
        byte = *p++;

        result |= (byte & 0x7F) << shift;
        shift += 7;
        /* sign bit of byte is second high order bit (0x40) */
        if((byte & 0x80) == 0)
            break;
    }
    if ((shift < size) && ((byte & 0x40) != 0)) {
        /* sign extend */
        result |= -(1 << shift);
    }
    return result;
}

ULONGLONG ULEB128Decode(CPCHAR &p)
{
    ULONGLONG result = 0;
    int shift = 0;
    ULONGLONG byte;
    for(;;) {
        byte = *p++;
        result |= (byte & 0x7F) << shift;
        shift += 7;
        if((byte & 0x80) == 0)
            break;
    }
    return result;
}

ULONGLONG GetSectionLength(CPCHAR &p,bool &bDwarfIs64)
{
    ULONGLONG retval = -1;
    bDwarfIs64 = *(ULONG *)p EQ 0xFFFFFFFF;
    if(bDwarfIs64) {
        p += 4;
        retval = *(ULONGLONG *)p;
        p += 8;
    } else {
        retval = *(ULONG *)p;
        p += 4;
    }
    return retval;
}

void SetSectionLength(PCHAR &p,bool bDwarfIs64,ULONGLONG length_of_entire_section)
{
    if(bDwarfIs64) {
        length_of_entire_section -= 12;
        *(ULONG *)p = 0xFFFFFFFF;
        p += 4;
        *(ULONGLONG *)p = length_of_entire_section;
        p += 8;
    } else {
        length_of_entire_section -= 4;
        *(ULONG *)p = (ULONG)length_of_entire_section;
        p += 4;
    }
}

vector<UBYTE> ULEB128Encode(ULONGLONG uval)
{
    vector<UBYTE>retval;
    do {
        UBYTE byte = uval & 0x7F;
        uval >>= 7;
        if (uval != 0) { /* more bytes to come */
            byte |= 0x80;
        }
        retval.push_back(byte);
    } while (uval != 0);
    return retval;
}

void ULEB128Encode(ULONGLONG val,PCHAR &p)
{
    vector<UBYTE>bytes = ULEB128Encode(val);
    for( UBYTE byte : bytes ) {
        *p++ = (char)byte;
    }
}

vector<UBYTE> LEB128Encode(LONGLONG val)
{
    vector<UBYTE>retval;

    bool negative = val < 0;
    bool more = true;

    while(more) {
        UBYTE byte = val & 0x7F;
        val >>= 7;

        if(     (val EQ  0 AND (byte & 0x40) EQ 0)
                OR  (val EQ -1 AND (byte & 0x40) != 0) ) {
            more = false;
        } else {
            byte |= 0x80;
        }
        retval.push_back(byte);
    }
    return retval;
}

void LEB128Encode(LONGLONG val,PCHAR &p)
{
    vector<UBYTE>bytes = LEB128Encode(val);
    for( UBYTE byte : bytes ) {
        *p++ = (char)byte;
    }
}


UBYTE GetUbyte(CPCHAR &p)
{
    return *p++;
}

BYTE GetByte(CPCHAR &p)
{
    return *p++;
}

USHORT GetUshort(CPCHAR &p)
{
    USHORT retval = *(USHORT *)p;
    p += 2;
    return retval;
}

ULONG GetUlong(CPCHAR &p)
{
    ULONG retval = *(ULONG *)p;
    p += 4;
    return retval;
}

ULONGLONG GetUlonglong(CPCHAR &p)
{
    ULONGLONG retval = *(ULONGLONG *)p;
    p += 8;
    return retval;
}

ULONGLONG GetOffset(bool bIs64,CPCHAR &p)
{
    return bIs64 ? GetUlonglong(p) : GetUlong(p);
}

