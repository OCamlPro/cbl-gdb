#pragma once

#include <vector>
#include "dwarf4_tags.h"
#include "c_elf.h"

LONGLONG LEB128Decode(CPCHAR &p);
ULONGLONG ULEB128Decode(CPCHAR &p);

std::vector<UBYTE> LEB128Encode(LONGLONG val);
std::vector<UBYTE> ULEB128Encode(ULONGLONG uval);
void LEB128Encode(LONGLONG val,PCHAR &p);
void ULEB128Encode(ULONGLONG uval,PCHAR &p);


ULONGLONG GetSectionLength(CPCHAR &p,bool &bIs64);
void SetSectionLength(PCHAR &p,bool bDwarfIs64,ULONGLONG length);

BYTE  GetByte(CPCHAR &p);
UBYTE GetUbyte(CPCHAR &p);
USHORT GetUshort(CPCHAR &p);
ULONG GetUlong(CPCHAR &p);
ULONGLONG GetUlonglong(CPCHAR &p);
ULONGLONG GetOffset(bool bIs64,CPCHAR &p);




