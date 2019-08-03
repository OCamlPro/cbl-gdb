
#include "dwarf4.h"
#include "profiler.h"

#define EQ ==
#define AND &&
#define OR ||

using namespace std;

LONGLONG LEB128Decode(PCHAR &p)
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

ULONGLONG ULEB128Decode(PCHAR &p)
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

ULONGLONG GetSectionLength(PCHAR &p,bool &bIs64)
{
    ULONGLONG retval = -1;
    bIs64 = *(ULONG *)p EQ 0xFFFFFFFF;
    if(bIs64) {
        p += 4;
        retval = *(ULONGLONG *)p;
        p += 8;
    } else {
        retval = *(ULONG *)p;
        p += 4;
    }
    return retval;
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


UBYTE GetUbyte(PCHAR &p)
{
    return *p++;
}

BYTE GetByte(PCHAR &p)
{
    return *p++;
}

USHORT GetUshort(PCHAR &p)
{
    USHORT retval = *(USHORT *)p;
    p += 2;
    return retval;
}

ULONG GetUlong(PCHAR &p)
{
    ULONG retval = *(ULONG *)p;
    p += 4;
    return retval;
}

ULONGLONG GetUlonglong(PCHAR &p)
{
    ULONGLONG retval = *(ULONGLONG *)p;
    p += 8;
    return retval;
}

ULONGLONG GetOffset(bool bIs64,PCHAR &p)
{
    return bIs64 ? GetUlonglong(p) : GetUlong(p);
}

