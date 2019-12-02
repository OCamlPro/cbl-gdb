#pragma once

#include <string>
#include <vector>

typedef char *PCHAR;
typedef char const *CPCHAR;

typedef  int8_t  BYTE;
typedef uint8_t UBYTE;

typedef  int16_t  SHORT;
typedef uint16_t USHORT;

typedef  int32_t  LONG;
typedef uint32_t ULONG;

typedef  int64_t  LONGLONG;
typedef uint64_t ULONGLONG;

typedef std::vector<std::string>VSTRING;

std::string Trim(const std::string &s);
std::string StripQuotes(const std::string &s);
VSTRING Split(const std::string &s,const std::string &delimiters);
std::string Replace(const std::string &s,const char was, const char is);

std::string FgetsRtrim(const std::string &ach);
