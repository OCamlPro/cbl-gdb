#pragma once

#include <vector>
#include "dwarf4_tags.h"
#include "c_elf.h"

LONGLONG LEB128Decode(PCHAR &p);
ULONGLONG ULEB128Decode(PCHAR &p);

std::vector<UBYTE> LEB128Encode(LONGLONG val);
std::vector<UBYTE> ULEB128Encode(ULONGLONG uval);

ULONGLONG GetSectionLength(PCHAR &p,bool &bIs64);

BYTE  GetByte(PCHAR &p);
UBYTE GetUbyte(PCHAR &p);
USHORT GetUshort(PCHAR &p);
ULONG GetUlong(PCHAR &p);
ULONGLONG GetUlonglong(PCHAR &p);
ULONGLONG GetOffset(bool bIs64,PCHAR &p);




