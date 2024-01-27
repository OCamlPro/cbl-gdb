/* #######################################################################
# Copyright (c) 2019-2024 COBOLworx Corporation
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in
#      the documentation and/or other materials provided with the
#      distribution.
#    * Neither the name of the COBOLworx Corporation nor the names of its
#      contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
############################################################################ */

#include <iostream>
#include <iomanip>
#include <map>
#include <set>
#include <deque>
#include <sstream>
#include <assert.h>
#include "variables.h"
#include "profiler.h"
#include "utils.h"

using namespace std;

#define EQ ==
#define AND &&
#define OR ||

#if 0

static string
ExtractVariableNameFrom(const string &p)
  {
  string retval = Trim(p);
  size_t nfound=retval.find("_");
  if( nfound != string::npos )
    {
    // Extract the b_10 from around the '_' character
    size_t nleft  = nfound;
    size_t nright = nfound+1;

    while( nleft >= 1 )
      {
      if( !isalpha(retval[nleft-1]) )
        {
        break;
        }
      nleft -= 1;
      }
    while( nright < retval.size() )
      {
      if( !isdigit(retval[nright]) )
        {
        break;
        }
      nright += 1;
      }
    retval = retval.substr(nleft,nright-nleft);
    }
  return retval;
  }

#endif

static int
CalculateParam(int type, bool signable, bool big_endian, int size)
  {
  int param = type;
  param *= 2;
  if( signable )
    {
    param += 1;
    }
  param *= 2;
  if( big_endian )
    {
    param += 1;
    }
  param *= 100;
  param += size;
  return param;
  }

static string
NormRepl( string input,
          size_t nfound,
          string lookfor,
          int type,
          bool big_endian,
          bool signable,
          bool size)
  {
  string retval;
  size_t nstart = nfound + lookfor.size();
  size_t p1 = input.find("(", nstart) + 1;
  size_t p2 = input.find(")", nstart);
  string b_val = input.substr(p1, p2-p1);
  int param = CalculateParam(type, signable, big_endian, size);
  char ach[64];
  snprintf( ach,
            sizeof(ach),
            "Q(%s,%d)",
            b_val.c_str(),
            param);
  retval =    input.substr(0, nfound)
            + ach
            + input.substr(p2+1);
  return retval;
  }

static string
NormalizeIntegerExtraction(COB_FIELDS &cob_fields, string s)
  {
  string retval = s;
    /*
    // These are 1, 2, 4, 8 byte signed little-endian integers

    *(cob_s8_ptr) (b_17)
    *(short *)(b_20)
    *(int *)(b_23)
    *(cob_s64_ptr)(b_26)

    // These are 1, 2, 4, 8 byte unsigned little-endian integers

    *(cob_u8_ptr) (b_29)
    *(unsigned short *)(b_32)
    *(unsigned int *)(b_35)
    *(cob_u64_ptr)(b_38)

    // These are 2-byte big-endian integers

    (short)COB_BSWAP_16(*(short *)(b_44))
    (unsigned short)COB_BSWAP_16(*(short *)(b_56))

    // These are 4-byte big-endian integers

    (int)COB_BSWAP_32(*(int *)(b_47))
    (unsigned int)COB_BSWAP_32(*(int *)(b_59))

    // These are 8-byte big-endian integers

    (cob_s64_t)COB_BSWAP_64(*(cob_s64_t *)(b_50))
    (cob_u64_t)COB_BSWAP_64(*(cob_s64_t *)(b_62))

    // These are N-character Numeric-Display variables

    cob_get_numdisps (b_65, N)

    // These are oddball variables that are less specifically described.
    cob_get_int (&f_78)

    // In a sample program we had

    // numeric-display with a 16 characters:
              05 TABD          PIC  s9(16) display value 5.
                static cob_field f_78   = {16, b_77, &a_37};    // TABD
    // packed-decimal, both COMP-3 and COMP-6
              05 TABD          PIC  s9(2) COMP-3 value 5.
                static cob_field f_96   = {2, b_95, &a_18};     // TABD
    // comp-1 32-bit floating point:
              05 TABD           COMP-1 value 5.
                static cob_field f_132  = {4, b_131, &a_29};    // TABD
    // comp-1 64-bit floating point:
              05 TABD           COMP-2 value 5.
                static cob_field f_135  = {8, b_134, &a_30};    // TABD

    decoding those requires looking at the specified a_xxx attribute values:
    static const cob_field_attr a_37 =  {0x10,   8,   0, 0x0001, NULL};
    static const cob_field_attr a_18 =  {0x12,   2,   0, 0x0001, NULL}; 
    static const cob_field_attr a_29 =  {0x13,  15,   8, 0x0201, NULL};
    static const cob_field_attr a_30 =  {0x14,  34,  17, 0x0201, NULL};

    where (from libcob/common.h)
    
    */

    typedef struct  
        {
        unsigned short        type;       /* Field type [TODO GC4: enum] */
        unsigned short        digits;     /* Digit count */
        signed short          scale;      /* Field scale */
        unsigned short        flags;      /* Field flags */
        const char           *pic;        /* Pointer to picture string */
        } cob_field_attr;

    typedef struct
        {
        size_t           size;      /* Field size */
        unsigned char   *data;      /* Pointer to field data */
        cob_field_attr  *attr;      /* Pointer to attribute */
        } cob_field;

    /* Field types */
    
    #define COB_TYPE_UNKNOWN               0x00    // 0
    #define COB_TYPE_GROUP                 0x01    // 1
    #define COB_TYPE_BOOLEAN               0x02    // 2
    #define COB_TYPE_NUMERIC               0x10    // 16
    #define COB_TYPE_NUMERIC_DISPLAY       0x10    // 16
    #define COB_TYPE_NUMERIC_BINARY        0x11    // 17
    #define COB_TYPE_NUMERIC_PACKED        0x12    // 18
    #define COB_TYPE_NUMERIC_FLOAT         0x13    // 19
    #define COB_TYPE_NUMERIC_DOUBLE        0x14    // 20
    #define COB_TYPE_NUMERIC_L_DOUBLE      0x15    // 21
    #define COB_TYPE_NUMERIC_FP_DEC64      0x16    // 22
    #define COB_TYPE_NUMERIC_FP_DEC128     0x17    // 23
    #define COB_TYPE_NUMERIC_FP_BIN32      0x18    // 24
    #define COB_TYPE_NUMERIC_FP_BIN64      0x19    // 25
    #define COB_TYPE_NUMERIC_FP_BIN128     0x1A    // 26
    #define COB_TYPE_NUMERIC_COMP5         0x1B    // 27
    #define COB_TYPE_ALNUM                 0x20    // 32
    #define COB_TYPE_ALPHANUMERIC          0x21    // 33
    #define COB_TYPE_ALPHANUMERIC_ALL      0x22    // 34
    #define COB_TYPE_ALPHANUMERIC_EDITED   0x23    // 35
    #define COB_TYPE_NUMERIC_EDITED        0x24    // 36
    #define COB_TYPE_NATIONAL              0x40    // 48
    #define COB_TYPE_NATIONAL_EDITED       0x41    // 49

    /* Field flags */
    
    #define COB_FLAG_HAVE_SIGN      = (1 << 0)   // /* 0x0001 */
    #define COB_FLAG_SIGN_SEPARATE  = (1 << 1)   // /* 0x0002 */
    #define COB_FLAG_SIGN_LEADING   = (1 << 2)   // /* 0x0004 */
    #define COB_FLAG_BLANK_ZERO     = (1 << 3)   // /* 0x0008 */
    #define COB_FLAG_JUSTIFIED      = (1 << 4)   // /* 0x0010 */
    #define COB_FLAG_BINARY_SWAP    = (1 << 5)   // /* 0x0020 */
    #define COB_FLAG_REAL_BINARY    = (1 << 6)   // /* 0x0040 */
    #define COB_FLAG_IS_POINTER     = (1 << 7)   // /* 0x0080 */
    #define COB_FLAG_NO_SIGN_NIBBLE = (1 << 8)   // /* 0x0100 */
    #define COB_FLAG_IS_FP          = (1 << 9)   // /* 0x0200 */
    #define COB_FLAG_REAL_SIGN      = (1 << 10)  // /* 0x0400 */
    #define COB_FLAG_BINARY_TRUNC   = (1 << 11)  // /* 0x0800 */
    #define COB_FLAG_CONSTANT       = (1 << 12)  // /* 0x1000 */
  // This is going to be far more tedious than is going to seem reasonable.
  // There really isn't much we can do except hack at it.
  
  string lookfor;
  size_t nfound;
  int type;
  bool big_endian;
  bool signable;
  int size;
  
  for(;;)
    {
    type = COB_TYPE_UNKNOWN;

    lookfor = "(short)COB_BSWAP_16(*(short*)";
    nfound=retval.find(lookfor);
    if(nfound != string::npos)
      {
      type        = COB_TYPE_NUMERIC_BINARY;
      big_endian  = true;
      signable    = true;
      size        = 2;
      retval = NormRepl(retval, nfound, lookfor, type, big_endian, signable, size);
      continue;
      }
    lookfor = "(int)COB_BSWAP_32(*(int*)";
    nfound=retval.find(lookfor);
    if(nfound != string::npos)
      {
      type        = COB_TYPE_NUMERIC_BINARY;
      big_endian  = true;
      signable    = true;
      size        = 4;
      retval = NormRepl(retval, nfound, lookfor, type, big_endian, signable, size);
      continue;
      }
    lookfor = "(cob_s64_t)COB_BSWAP_64(*(cob_s64_t*)";
    nfound=retval.find(lookfor);
    if(nfound != string::npos)
      {
      type        = COB_TYPE_NUMERIC_BINARY;
      big_endian  = true;
      signable    = true;
      size        = 8;
      retval = NormRepl(retval, nfound, lookfor, type, big_endian, signable, size);
      continue;
      }
    lookfor = "(unsignedshort)COB_BSWAP_16(*(short*)";
    nfound=retval.find(lookfor);
    if(nfound != string::npos)
      {
      type        = COB_TYPE_NUMERIC_BINARY;
      big_endian  = true;
      signable    = false;
      size        = 2;
      retval = NormRepl(retval, nfound, lookfor, type, big_endian, signable, size);
      continue;
      }
    lookfor = "(unsignedint)COB_BSWAP_32(*(int*)";
    nfound=retval.find(lookfor);
    if(nfound != string::npos)
      {
      type        = COB_TYPE_NUMERIC_BINARY;
      big_endian  = true;
      signable    = false;
      size        = 4;
      retval = NormRepl(retval, nfound, lookfor, type, big_endian, signable, size);
      continue;
      }
    lookfor = "(cob_u64_t)COB_BSWAP_64(*(cob_s64_t*)";
    nfound=retval.find(lookfor);
    if(nfound != string::npos)
      {
      type        = COB_TYPE_NUMERIC_BINARY;
      big_endian  = true;
      signable    = false;
      size        = 8;
      retval = NormRepl(retval, nfound, lookfor, type, big_endian, signable, size);
      continue;
      }
    lookfor = "*(cob_s8_ptr)";
    nfound=retval.find(lookfor);
    if(nfound != string::npos)
      {
      type        = COB_TYPE_NUMERIC_BINARY;
      big_endian  = false;
      signable    = true;
      size        = 1;
      retval = NormRepl(retval, nfound, lookfor, type, big_endian, signable, size);
      continue;
      }
    lookfor = "*(short*)";
    nfound=retval.find(lookfor);
    if(nfound != string::npos)
      {
      type        = COB_TYPE_NUMERIC_BINARY;
      big_endian  = false;
      signable    = true;
      size        = 2;
      retval = NormRepl(retval, nfound, lookfor, type, big_endian, signable, size);
      continue;
      }
    lookfor = "*(int*)";
    nfound=retval.find(lookfor);
    if(nfound != string::npos)
      {
      type        = COB_TYPE_NUMERIC_BINARY;
      big_endian  = false;
      signable    = true;
      size        = 4;
      retval = NormRepl(retval, nfound, lookfor, type, big_endian, signable, size);
      continue;
      }
    lookfor = "*(cob_s64_ptr)";
    nfound=retval.find(lookfor);
    if(nfound != string::npos)
      {
      type        = COB_TYPE_NUMERIC_BINARY;
      big_endian  = false;
      signable    = true;
      size        = 8;
      retval = NormRepl(retval, nfound, lookfor, type, big_endian, signable, size);
      continue;
      }
    lookfor = "*(cob_u8_ptr)";
    nfound=retval.find(lookfor);
    if(nfound != string::npos)
      {
      type        = COB_TYPE_NUMERIC_BINARY;
      big_endian  = false;
      signable    = false;
      size        = 1;
      retval = NormRepl(retval, nfound, lookfor, type, big_endian, signable, size);
      continue;
      }
    lookfor = "*(unsignedshort*)";
    nfound=retval.find(lookfor);
    if(nfound != string::npos)
      {
      type        = COB_TYPE_NUMERIC_BINARY;
      big_endian  = false;
      signable    = false;
      size        = 2;
      retval = NormRepl(retval, nfound, lookfor, type, big_endian, signable, size);
      continue;
      }
    lookfor = "*(unsignedint*)";
    nfound=retval.find(lookfor);
    if(nfound != string::npos)
      {
      type        = COB_TYPE_NUMERIC_BINARY;
      big_endian  = false;
      signable    = false;
      size        = 4;
      retval = NormRepl(retval, nfound, lookfor, type, big_endian, signable, size);
      continue;
      }
    lookfor = "*(cob_u64_ptr)";
    nfound=retval.find(lookfor);
    if(nfound != string::npos)
      {
      type        = COB_TYPE_NUMERIC_BINARY;
      big_endian  = false;
      signable    = false;
      size        = 8;
      retval = NormRepl(retval, nfound, lookfor, type, big_endian, signable, size);
      continue;
      }
    lookfor = "cob_get_numdisp";
    nfound=retval.find(lookfor);
    if(nfound != string::npos)
      {
      size_t p1 = retval.find("(", nfound) +1;
      size_t p2 = retval.find(")", p1);

      VSTRING pp = Split(retval.substr(p1, p2-p1), ",");
      type        = COB_TYPE_NUMERIC_DISPLAY;
      big_endian  = true;
      signable    = true;
      size        = stoi(Trim(pp[1]));
      
      int param = CalculateParam(type, signable, big_endian, size);

      char ach[64];
      snprintf( ach,
                sizeof(ach),
                "Q(%s,%d)",
                Trim(pp[0]).c_str(),
                param);
      string input = retval;
      retval =    input.substr(0, nfound)
                + ach
                + input.substr(p2+1);
      continue;
      }
    lookfor = "cob_get_int";
    nfound=retval.find(lookfor);
    if(nfound != string::npos)
      {
      size_t p1 = retval.find("(", nfound) +2;
      size_t p2 = retval.find(")", p1);
      string f_name = retval.substr(p1, p2-p1);
      cout << "f_name is " << f_name << endl;
      COB_FIELD *cob_field = cob_fields.Find(f_name);
      assert(cob_field);
      string b_name = cob_field->b_name;
      int size = cob_field->size;
      string a_name = cob_field->a_name;

      COB_ATTR *cob_attr = cob_fields.FindAttr(a_name);
      assert(cob_attr);
      
      
      exit(1);
      continue;
      }
    break;
    }

  return retval;
  }

#if 0

static string
HackAwayCobGetNumdisp(const string &p_)
  {
  return p_;
  string p = p_;
  // cob_dump_field ( 1, "G-1", COB_SET_FLD(f0, cob_get_numdisp (b_8, 1) + cob_get_numdisp (b_8, 1) + cob_get_numdisp (b_8, 1), b_9, &a_1), 0, 0);
  // Let's hack at that last one first. Anyplace we see
  //    "cob_get_numdisp (b_8, 1)",
  // we want to extract, simply "b_8"

  string retval;
  while(!p.empty())
    {
    size_t nfound1 = p.find("cob_get_numdisp");
    if( nfound1 EQ string::npos )
      {
      nfound1 = p.size();
      }
    // nfound1 is where cob_get_numdisp starts
    size_t nfound2 = p.find(')',nfound1);
    string replacement = "";
    if( nfound2 != string::npos )
      {
      size_t nfoundm = nfound1 + strlen("cob_get_numdisp");
      nfound2 += 1;
      string fragment = p.substr(nfoundm,nfound2-nfoundm);
      // Fragment looks like
      //      " (b_8, 1)"
      replacement = " " + ExtractVariableNameFrom(fragment) + " ";
      }
    else
      {
      nfound2 = nfound1;
      }

    retval += p.substr(0,nfound1);
    retval += replacement;
    p = p.substr(nfound2);
    }
  return retval;
  }

static string
SquishAwayStarIntStar(const string &s_)
  {
  string s = s_;
  string retval;

  while( !s.empty() )
    {
    size_t nfound1 = s.find( "(*(int *)" );
    size_t nfound2;
    string stripped;
    if( nfound1 != string::npos )
      {
      nfound2 = s.find(')',nfound1 + strlen("(*(int *)") );
      if( nfound2 != string::npos )
        {
        nfound2 += 2;
        string param = ExtractVariableNameFrom( s.substr(nfound1 + strlen("(*(int *)")) );
        retval = s.substr(0,nfound1);
        retval += ' ';
        retval += param;
        retval += ' ';
        }
      }
    else
      {
      nfound1 = s.size();
      nfound2 = nfound1;
      retval += s;
      }
    s = s.substr(nfound2);
    }

  return retval;
  }
  
#endif

VSTRING
ParametersInsideParentheses(const string &s, size_t &final)
  {
  VSTRING retval;
  final = 0;
  string next;
  const char *p = s.c_str();

  int pcount = 0;
  if( *p++ EQ '(' )
    {
    pcount = 1;
    }
  while( pcount && *p )
    {
    char ch = *p++;
    final += 1;
    if( ch EQ '(' )
      {
      pcount += 1;
      next += ch;
      continue;
      }
    if( ch EQ ')' )
      {
      if( --pcount )
        {
        next += ch;
        }
      continue;
      }
    if( pcount > 1 )
      {
      next += ch;
      continue;
      }
    // Arriving here means pcount is 1
    if( ch EQ ',' )
      {
      retval.push_back(Trim(next));
      next.clear();
      continue;
      }
    next += ch;
    }
  if( !retval.empty() )
    {
    retval.push_back(Trim(next));
    }

  return retval;
  }

void
VARIABLES::ScanCFile( COB_FIELDS &cob_fields,
                      const string &path,
                      const string &fname)
  {
  CPROFILER;
  string full_path = path;
  full_path += fname;
  full_path += ".c";

  FILE *f = fopen(full_path.c_str(),"r");
  if( !f )
    {
    cout << "Couldn't open the input file " << full_path << endl;
    exit(1);
    }

  set<string>externals;

  char ach[16384];

  string current_perform_file;
  int current_perform_line=-1;

  enum
    {
    PR_LOOKING,
    PR_FIRST_COMMENT,
    PR_RETURN_ADDRESS,
    PR_SECOND_COMMENT,
    } perform_return = PR_LOOKING;

  string current_record_type = "-" ;
  int line_number = 0;
  string input;

  bool looking_for_cstart = false;
  bool found_cstart = false;

  deque<VARIABLE> list_of_indexes;

  while( fgets(ach,sizeof(ach),f) )
    {
    input = ach;
    line_number += 1;
    extern bool GV_echo_line;
    if( GV_echo_line )
      {
      printf("A-%d %s",line_number,ach);
      }
    if( strstr(ach, ": Entry ") )
      {
      VSTRING tokens = Split(input," \t");
      if( tokens.size() EQ 9
          AND tokens[0] EQ "/*"
          AND tokens[1] EQ "Line:"
          AND tokens[3] EQ ":"
          AND tokens[4] EQ "Entry"
          AND tokens[6] EQ ":"
          AND tokens[8] EQ "*/\n" )
        {
        VARIABLE variable;
        variable.record_type = "P";
        variable.offset = tokens[2];
        variable.name = tokens[5];
        variable.f_name = tokens[7];
        Insert(variable);
        variable.record_type = "E";
        Insert(variable);
        }
      continue;
      }

    if( strstr(ach, " PROGRAM-ID ") )
      {
      //  When a PROCEDURE DIVISION starts off with a DECLARATIVES section,
      //  it's possible to have executable code before the ENTRY point
      //  of the PROGRAM-ID.  The CBL-GDB Python, however, requires
      //  the "P" record_type to be the first entry for a program block.

      //  We're handling this by putting a marker in when we see a PROGRAM-ID
      //  line.  Later on, before we output the VARIABLE_STRING, we'll
      //  bubble each "P" entry up to its parent PROGRAM-ID entry.
      VSTRING tokens = Split(input," \t");
      if( tokens.size() EQ 4
          AND tokens[0] EQ "/*"
          AND tokens[1] EQ "PROGRAM-ID"
          AND tokens[3] EQ "*/\n" )
        {
        VARIABLE variable;
        variable.record_type = "PROGRAM-ID";
        Insert(variable);
        }

      looking_for_cstart = true;
      continue;
      }

    if( looking_for_cstart AND !found_cstart )
      {
      // Our assumption is that the first return after the first
      // PROGRAM-ID is where we want CSTART to break:
      VSTRING tokens = Split(input," \t");

      if( tokens.size() >= 3 && tokens[0] == "return")
        {
        // We have found that first return
        VARIABLE variable;
        variable.record_type = "C";
        variable.name = tokens[1];
        Insert(variable);
        found_cstart = true;
        }
      }


check_for_another_perform:
    switch(perform_return)
      {
      case PR_LOOKING:
        {
        if( strstr(ach, ": PERFORM ") )
          {
          VSTRING tokens = Split(input," \t");
          if( tokens.size() EQ 8
              AND tokens[0] EQ "/*"
              AND tokens[1] EQ "Line:"
              AND tokens[3] EQ ":"
              AND tokens[4] EQ "PERFORM"
              AND tokens[5] EQ ":"
              AND tokens[7] EQ "*/\n" )
            {
            VARIABLE variable;
            variable.record_type = "p";
            variable.f_name = tokens[6] + ':' + tokens[2];
            current_perform_file = tokens[6];
            current_perform_line = stoi(tokens[2]);
            Insert(variable);
            perform_return = PR_FIRST_COMMENT;
            continue;
            }
          }
        }
      break;

      case PR_FIRST_COMMENT:
        if( strstr (ach,"/*") )
          {
          // This is the first comment after the PERFORM.
          VSTRING tokens = Split(input," \t");
          if( tokens.size() > 1
              AND tokens[0] == "/*"
              AND tokens[1] == "PERFORM")
            // This is a PERFORM A
            {
            perform_return = PR_RETURN_ADDRESS;
            continue;
            }
          else
            {
            //
            // This is a PERFORM N TIMES or PERFORM UNTIL

            // We ignore those
            variables.pop_back();
            perform_return = PR_LOOKING;
            goto check_for_another_perform;
            }
          }
        break;

      case PR_RETURN_ADDRESS:
        {
        // When looking for a return location, the first thing we
        // find is the default:
        //     frame_ptr->return_address_ptr = &&l_50;
        // This might be our only choice, when, for example, this
        // PERFORM is itself followed by an implicit return, which can
        // happen when the PERFORM statement is itself the last statement
        // of a paragraph that ends a section or a program.

        char *p = strstr(ach,"&&");
        if( p )
          {
          p += 2;
          char *p2 = strstr(ach,";");
          if( p2 )
            {
            *p2 = '\0';
            }
          variables.back().name = p;
          perform_return = PR_SECOND_COMMENT;
          }
        continue;
        }
      break;

      case PR_SECOND_COMMENT:
        // But what we prefer to find is the /* Line: nn that follows
        // the PERFORM.  If we can find that line number, it makes for
        // a cleaner-looking sequence in GDB

        // From something like
        //      /* Line: 109       : Paragraph PARTEND-001             : rtest.cbl */
        // we will build rtest.cbl:109 as the target:

        // Discovered a problem, though.
        //
        //64        PERFORM until end-of-file
        //65            PERFORM 02-process
        //66            PERFORM 01-read
        //67            END-PERFORM.
        //68        DISPLAY my-counters.

        // It turns out that the END-PERFORM line doesn't generate any code
        // at all in the generated C file.  So, the next line after 66 PERFORM
        // was getting picked up as line 68 -- which isn't what we had in mind.

        // So, I only issue the "nice" line number if it is the one right after
        // the perform; otherwise we go through the until/step sequenct that
        // doesn't look as good in GDB when it happens, but is more accurate
        // in this case.

        char *p = strstr(ach,"/*");
        if( p )
          {
          VSTRING tokens = Split(ach," \t");
          if( tokens.size() > 2 AND tokens[1] == "Line:")
            {
            size_t i;
            for( i=5; i<tokens.size(); i++ )
              {
              if( tokens[i] EQ ":" )
                {
                break;
                }
              }
            i += 1;
            if( i < tokens.size() )
              {
              if( tokens[i] EQ current_perform_file
                  AND (current_perform_line+1) EQ stoi(tokens[2]) )
                {
                variables.back().name = tokens[i] + ':' + tokens[2];
                }
              }
            }
          perform_return = PR_LOOKING;
          goto check_for_another_perform;
          }
        break;
      }

    static const char COB_EXTERNAL_ADDR[]   = "cob_external_addr";
    static const char COB_DUMP_FILE[]       = "cob_dump_file";
    static const char COB_DUMP_FIELD[]      = "cob_dump_field";
    static const char COB_DUMP_OUTPUT[]     = "cob_dump_output";
    static const char COB_SET_DATA[]        = "COB_SET_DATA";
    static const char COB_SET_FLD[]         = "COB_SET_FLD";
    char *p = ach;

    // Skip over leading spaces
    while( *p EQ ' ' OR *p EQ '\t' )
      {
      p += 1;
      }

    // The lines we are looking for can be commented out.
    // So, we just ignore the "/*" if it's the first thing on a line:
    if( *p EQ '/' AND *(p+1) EQ '*' )
      {
      p += 2;
      }

    // Skip over spaces
    while( *p EQ ' ' OR *p EQ '\t' )
      {
      p += 1;
      }

    if( strstr(p,COB_EXTERNAL_ADDR) )
      {
      // We are looking for
      //  b_13 = cob_external_addr ("Z_TABLE", 3160);
      VSTRING t = Split(p, " \t");
      if( t[1] EQ "=" AND t[2] EQ COB_EXTERNAL_ADDR )
        {
        // That's good enough for me
        externals.insert(t[0]);
        }
      }

    if( strncmp(p, COB_DUMP_OUTPUT, strlen(COB_DUMP_OUTPUT)) EQ 0 )
      {
      /* cob_dump_output("WORKING-STORAGE"); */
      /* cob_dump_output("LINKAGE"); */
      /* cob_dump_output("LOCAL-STORAGE"); */
      p += strlen(COB_DUMP_OUTPUT);
      if( strstr(p,"WORKING-STORAGE") )
        {
        current_record_type = "W";
        }
      else if( strstr(p,"LOCAL-STORAGE") )
        {
        current_record_type = "L";
        }
      else if( strstr(p,"LINKAGE") )
        {
        current_record_type = "I";
        }
      // There are other types: REPORT and SCREEN.  We don't care about them
      continue;
      }

    if( strncmp(p, COB_DUMP_FILE, strlen(COB_DUMP_FILE)) EQ 0 )
      {
      /* cob_dump_file ("FD pgm-input", h_PGM_INPUT); */
      /* cob_dump_file ("SD pgm-input", h_PGM_INPUT); */
      p += strlen(COB_DUMP_FILE);
      if( strstr(p,"SD ") )
        {
        current_record_type = "S";
        }
      else if( strstr(p,"FD ") )
        {
        current_record_type = "F";
        }
      continue;
      }

    if( strncmp(p, COB_DUMP_FIELD, strlen(COB_DUMP_FIELD)) EQ 0 )
      {
      // As time has gone on, and I have been exposed to steadily more complex COBOL
      // constructions, this routine has been forced to become more and more complex.
      // The cob_dump_field routine takes these parameters
      //   cob_dump_field (const int level, const char *name, cob_field *f_addr, const int offset, const int indexes, ...)
      // What makes this generally useful is that the field pointer gets built at dump time using the
      // COB_SET_FLD() macro
      //   COB_SET_FLD(f_name, length, bname + offset, aname)
      //
      // Where it gets complicated is when the length and/or the offset are themselves variable

      // So, examples:

      //  /* cob_dump_field ( 1, "WRK-DU-1V0-1", COB_SET_FLD(f0, 1, b_52, &a_7), 0, 0); */
      //  /* cob_dump_field (10, "datyr", COB_SET_FLD(f0, 4, b_15 + 17, &a_7), 0, 0); */
      //  /* cob_dump_field ( 5, "new-dat1", COB_SET_FLD(f0, 8, b_15 + 17, &a_3), 0, 0); REDEFINES */
      //  /* cob_dump_field ( 2, "ELMT", &f_278, 0, 1, i_1, 1); OCCURS 1 3 */
      //  /* cob_dump_field ( 2, "G-2", COB_SET_FLD(f0, cob_get_numdisp (b_8, 1), b_9, &a_1), 0, 0); */
      //  /* cob_dump_field ( 1, "result", COB_SET_FLD(f0, (*(int *)(b_8)), b_10, &a_3), 0, 0); */
      //  /* cob_dump_field ( 1, "G-1", COB_SET_FLD(f0, cob_get_numdisp (b_8, 1) + cob_get_numdisp (b_8, 1) + cob_get_numdisp (b_8, 1), b_9, &a_1), 0, 0); */
      //  /* cob_dump_field ( 2, "G-3", COB_SET_FLD(f0, cob_get_numdisp (b_8, 1) + cob_get_numdisp (b_8, 1), b_9 + 0 + cob_get_numdisp (b_8, 1), &a_1), 0, 0); */
      //  /* cob_dump_field ( 3, "G-4", COB_SET_FLD(f1, cob_get_numdisp (b_8, 1), b_9 + 0 + cob_get_numdisp (b_8, 1), &a_1), 0, 0); */
      //  /* cob_dump_field ( 4, "X", COB_SET_FLD(f0, 1, b_9 + 0 + cob_get_numdisp (b_8, 1), &a_4), 0, 1, i_1, 1); OCCURS 1 3 */
      //  /* cob_dump_field ( 3, "G-5", COB_SET_FLD(f2, cob_get_numdisp (b_8, 1), b_9 + 0 + cob_get_numdisp (b_8, 1) + 0 + cob_get_numdisp (b_8, 1), &a_1), 0, 0); */
      //  /* cob_dump_field ( 4, "X", COB_SET_FLD(f0, 1, b_9 + 0 + cob_get_numdisp (b_8, 1) + 0 + cob_get_numdisp (b_8, 1), &a_4), 0, 1, i_1, 1); OCCURS 1 3 */
      //  /* cob_dump_field ( 2, "WS-LASTNAME", COB_SET_DATA (f_88, b_84 + 12), 0, 0); EXTERNAL */
      //  /* cob_dump_field_ext ( 1, "WS-BASED", COB_SET_DATA (f_55, b_55), 0, 0); BASED */
      //  /* cob_dump_field_ext (88, "FLAG", &f_21, 0, 0); VALUE (cob_field *)&c_3 OR (cob_field *)&c_4 */
      //
      // Arriving here means that the first thing on the line after removing "/*" is "cob_dump_field"

      string cdf = p;
      size_t nfound = cdf.find('(');
      if( nfound != string::npos )
        {
        cdf = cdf.substr(nfound);
        VARIABLE variable;
        variable.record_type = current_record_type;

        // If this variable is a REDEFINES, that word, flanked by spaces, will be on the line.
        variable.redefines = strstr(p," REDEFINES ") != NULL;

        // If this variable is an OCCURS, that word, flanked by spaces, will be on the line:

        char *poccurs = strstr(ach," OCCURS ");
        if( poccurs )
          {
          VSTRING t = Split(poccurs," ");
          if( t.size() >= 3 )
            {
            variable.occurs_min = stoi(t[1]);
            variable.occurs_max = stoi(t[2]);
            }
          }

        // This is yet another hack:
        bool is_based = !!strstr(ach,"; BASED */"); // Becomes true if non-NULL, false if NULL

        size_t final_paren;
        VSTRING cdf_parameters = ParametersInsideParentheses(cdf,final_paren);
        if( cdf_parameters.size() >= 5  )
          {
          variable.level = stoi(Trim(cdf_parameters[0]));
          variable.name  = StripQuotes(cdf_parameters[1]);
          string field_pointer = Trim(cdf_parameters[2]);
          string occurs_offset = Trim(cdf_parameters[3]);
          string occurs_indexes = Trim(cdf_parameters[4]);

          if( !field_pointer.empty() && field_pointer[0] EQ '&')
            {
            // Minimal association: COBOL_name with f_name
            variable.f_name = field_pointer.substr(1);
            }
          else
            {
            if( field_pointer.find(COB_SET_FLD) != string::npos )
              {
                nfound = field_pointer.find( '(' );
                if( nfound != string::npos )
                  {
                  field_pointer = field_pointer.substr(nfound);
                  VSTRING csf_parameters = ParametersInsideParentheses(field_pointer,final_paren);
                  if( csf_parameters.size() EQ 4 )
                    {
                    //   COB_SET_FLD(f_name, length, bname + offset, aname)
                    // fname is meaningless in this context
                    // length might be combinations of integer constants and/or text :

                    for(size_t i=0; i<csf_parameters[1].size(); i++ )
                      {
                      char ch = csf_parameters[1][i];
                      if( ch != ' ' )
                        {
                        variable.length += ch;
                        }
                      }

                    // The third parameter is the b_name with, possibly, an offset added to it.
                    VSTRING base_offset = Split(csf_parameters[2],"+");
                    if( base_offset.size() )
                      {
                      variable.b_name = Trim(base_offset[0]);
                      }
                    size_t i = 1;
                    while( i < base_offset.size() )
                      {
                      string offset = Trim(base_offset[i++]);
                      if( offset != "0" )
                        {
                        if( !variable.offset.empty() )
                          {
                          variable.offset += '+';
                          }
                        variable.offset += offset ;
                        }
                      }
                    variable.a_name = csf_parameters[3].substr(1);
                    }
                  else
                    {
                    cerr << "VARIABLES::ScanCFile() doesn't know how to handle the COB_SET_FLD macro in" << endl ;
                    cerr << "   " << ach << endl;
                    cerr << "   " << cdf << endl;
                    cerr << "   " << csf_parameters[2] << endl;
                    cerr << "Please alert Bob Dubner at support@cobolworx.com" << endl;
                    cerr << "Thank you." << endl;
                    continue;
                    }
                  }
              }
            else if( field_pointer.find(COB_SET_DATA) != string::npos )
              {
              nfound = field_pointer.find( '(' );
              if( nfound != string::npos )
                {
                field_pointer = field_pointer.substr(nfound);
                VSTRING csd_parameters = ParametersInsideParentheses(field_pointer,final_paren);
                if(csd_parameters.size() == 2)
                  {
                  variable.f_name = csd_parameters[0];
                  }

                // The second parameter is the b_name with, possibly, an offset added to it.
                VSTRING base_offset = Split(csd_parameters[1],"+");
                if( base_offset.size() )
                  {
                  variable.b_name = Trim(base_offset[0]);
                  }
                size_t i = 1;
                while( i < base_offset.size() )
                  {
                  string offset = Trim(base_offset[i++]);
                  if( offset != "0" )
                    {
                    if( !variable.offset.empty() )
                      {
                      variable.offset += '+';
                      }
                    variable.offset += offset ;
                    }
                  }
                }
              }
            else
              {
              cerr << "VARIABLES::ScanCFile() doesn't know how to handle the "
                      "field pointer in" << endl ;
              cerr << "   " << ach ;
              cerr << "Please alert Bob Dubner at support@cobolworx.com" << endl;
              cerr << "Thank you." << endl;
              continue;
              }
            }
          if( is_based )
            {
            variable.record_type += "B"; // Reflag as BASED
            }

          if( variable.level EQ 88 )
            {
            // This is a conditional variable.
            // /* cob_dump_field_ext (88, "AFLAG", &f_17, 0, 0); VALUE (cob_field *)&c_3 
            //    OR (cob_field *)&c_4 
            //    OR (cob_field *)&c_5 THRU (cob_field *)&c_6 
            //    OR (cob_field *)&c_7 
            //    OR (cob_field *)&c_8 THRU (cob_field *)&c_9 OR (cob_field *)&c_10 */
            nfound = cdf.find(" VALUE ");
            if( nfound != string::npos )
              {
              string vcdf = cdf.substr(nfound + 7);
              for(;;)
                {
                nfound = vcdf.find("(cob_field *)&");
                if( nfound EQ string::npos )
                  {
                  break;
                  }
                vcdf.replace(nfound, 14, "");
                //cout << vcdf << endl;
                }
              VSTRING tokens = Split(vcdf," \t");
              if( tokens.size() && tokens.back() == "*/\n" )
                {
                tokens.pop_back();
                }
              for(size_t i=0; i<tokens.size(); i++)
                {
                if( tokens[i] == "OR" )
                  {
                  // This happens with ...A THRU B OR...
                  continue;
                  }
                string lower = tokens[i];
                string upper = tokens[i];
                if( i+1 < tokens.size() )
                  {
                  if( tokens[i+1] EQ "OR" )
                    {
                    i += 1;
                    }
                  else if( tokens[i+1] == "THRU" )
                    {
                    upper = tokens[i+2];
                    i += 2;
                    }
                  else
                    {
                    fprintf(stderr, "There was a problem parsing LEVEL 88 VALUE\n");
                    fprintf(stderr, "%d: %s\n", line_number, input.c_str());
                    exit(1);
                    }
                  }
                pair<string, string> condition = {lower, upper};
                variable.conditions.push_back(condition);
                }
              }
            }

          // We need to do a bit of thinking, here.  GnuCOBOL 3.2 puts INDEXED
          // BY variables into the C file just before the declaration of the
          // array that uses them.  Such variables have a level of 00.  So,
          // we are going to stash them in a list until we see their parent

          variable.length = NormalizeIntegerExtraction(cob_fields, variable.length);
//          cout << "   Length parameter is " << variable.length << endl;

          if( variable.level == 0 )
            {
            list_of_indexes.push_back(variable);
            }
          else
            {
            Insert(variable);
            while( list_of_indexes.size() )
              {
              Insert(list_of_indexes.front());
              list_of_indexes.pop_front();
              }
            }
          }
        }
      }
    }

  // We have a set of externals.  Let's scan through the variables.  Any
  // variable whose b_name is in externals[] gets reflagged as 'X'

  for( V_VARIABLES::iterator it=variables.begin(); it!=variables.end(); it++ )
    {
    if( externals.find(it->b_name) != externals.end() )
      {
      it->record_type = "X"; // Reflag as EXTERNAL
      }
    }

  ::fclose(f);
  }

void
VARIABLES::FillInTheBlanks(COB_FIELDS &cob_fields)
  {
  string current_bname = "";
  for( V_VARIABLES::iterator it = variables.begin(); it!=variables.end(); it++)
    {
    if( it->level <= 2 OR it->level EQ 78 OR it->level EQ 77 )
      {
      // This is a root variable, so reset the current name
      current_bname = "";
      }

    if( it->a_name.empty() )
      {
      const COB_FIELD *pfield = cob_fields.Find(it->f_name);
      if( pfield )
        {
        if( it->record_type EQ "L" )
          {
          // Local variables are all referenced against cob_local_ptr
          it->b_name = "cob_local_ptr";
          }
        else
          {
          if( !pfield->b_name.empty() )
            {
            it->b_name = pfield->b_name;
            }
          }
        it->a_name = pfield->a_name;
        if( pfield->size)
          {
          it->length = to_string(pfield->size);
          }
        if( pfield->offset )
          {
          it->offset = to_string(pfield->offset);
          }
        }
      }
    if( it->b_name.empty() )
      {
      it->b_name = current_bname;
      }
    else
      {
      current_bname = it->b_name;
      }

    V_VARIABLES::iterator it2 = it;
    while( it2 > variables.begin() )
      {
      if( it2->b_name.empty() )
        {
        // We have nothing to propogate upward
        break;
        }
      if( it2->level EQ 77 )
        {
        // Level 77 doesn't propogate upward
        break;
        }
      V_VARIABLES::iterator it1 = it2-1;
      if( !it1->b_name.empty() )
        {
        // The one before ours is already populated
        break;
        }
      // When you get here, it1 has no b_name, and it2 does
      if( it1->level <= it2->level AND it2->level >= 2 )
        {
        it1->b_name = it2->b_name;
        }
      it2--;
      }
    }
  }

void
VARIABLES::Dump() const
  {
  CPROFILER;
  cout << "VARIABLES:" << endl;
  cout << setw( 3) << "RT" ;
  cout << setw( 4) << "lvl";
  cout << setw(20) << "name" ;
  cout << setw(20) << "f_name";
  cout << setw(20) << "b_name";
  cout << setw(20) << "a_name" ;
  cout << setw(20) << "offset" ;
  cout << setw(20) << "length";
  cout << setw( 6) << "o_min";
  cout << setw( 6) << "o_max";
  cout << setw( 6) << "redef";
  cout << endl;

  for( V_VARIABLES::const_iterator variable = variables.begin();
       variable!=variables.end();
       variable++)
    {
    cout << setw( 3) << variable->record_type;    // One character:
    cout << setw( 4) << variable->level;          // FD, SD, 01-49, 66, 77, 78, 88
    cout << setw(20) << variable->name;           // COBOL variable identifier
    cout << setw(20) << variable->f_name;         // Name of the associated cob_field variable
    cout << setw(20) << variable->b_name;         // Name of the location of the data
    cout << setw(20) << variable->a_name;         // Name of the associated cob_field_attrib variable
    cout << setw(20) << variable->offset;         // When f_name exists, offset should have the same runtime value as f_name->data - b_name;
    cout << setw(20) << variable->length;      // When f_name exists, length should be the same as f_name->size.  But it might be a b_ variable
    cout << setw( 6) << variable->occurs_min;     //
    cout << setw( 6) << variable->occurs_max;     // -1 for unbounded
    cout << setw( 6) << (variable->redefines ? "REDEF" : "");
    cout << endl;
    }
  cout << endl;
  }

void
VARIABLES::BubbleUpProgramEntries()
  {
  // When a COBOL program has a DECLARATIVES section in the PROCEDURE division, it's possible
  // to have, for example PERFORM entries in the variables vector before the "P" entry.
  // The cobcd.py Python extension to GDB requires, however, that the "P" entry be
  // the first for each program-id block.
  //
  // This routine looks for "P" entries and bubbles them up to the "PROGRAM-ID" entry
  // that was inserted in the list when the /* PROGRAM-ID */ line was encountered

  for( size_t i=variables.size()-1; i>0; i-- )
    {
    if( variables[i].record_type EQ "P" )
      {
      // This is a "P entry.
      if( variables[i-1].record_type != "PROGRAM-ID" )
        {
        // It needs to be bubbled up:
        swap(variables[i],variables[i-1]);
        }
      }
    }
  }

void
VARIABLES::FormatVariablesInfo(stringstream &ss) const
  {
  CPROFILER;
  for( V_VARIABLES::const_iterator variable = variables.begin();
       variable!=variables.end();
       variable++)
    {
    if( variable->record_type == "PROGRAM-ID" )
      {
      // This is a marker entry, which was needed by BubbleUpProgramEntries().
      // It must not be included in the output
      continue;
      }

    if( variable->record_type == "L" AND variable->level == 66)
      {
      // GnuCOBOL 3.1.2 is not providing enough information for CBL-GDB
      // to find 66 RENAMES variables in LOCAL-STORAGE.  So, until that's
      // fixed, we just skip them here.  The fix will be when the
      // cob_dump_field_ext field provides the offset and length from
      // cob_local_ptr
      continue;
      }

    string name = variable->name;
    if( name.empty() )
      {
      name = "FILLER";
      }

    //cerr << "Fixed lengths " << variable->length << endl;

    ss << variable->record_type             << "|" ;
    ss << ZeroIsNull(variable->level)       << "|" ;
    ss << name                              << "|" ;
    ss << variable->f_name                  << "|" ;
    ss << (variable->b_name EQ "cob_local_ptr" ? "" : variable->b_name) << "|" ;
    ss << variable->a_name                  << "|" ;
    ss << ZeroIsNull(variable->offset)      << "|" ;
    ss << variable->length                  << "|" ;
    ss << ZeroIsNull(variable->occurs_min)  << "|" ;
    if( variable->occurs_min EQ 0 AND variable->occurs_max EQ 1 )
      {
      // This is the default, so output nothing
      ss << "|" ;
      }
    else
      {
      ss << variable->occurs_max;
      ss << "|" ;
      }
    ss << "~";

    // LEVEL88 conditional variableS have ->conditional entries.
    // These values are not of the same form as other entries, because they 
    // contain values determined by the user, and thus they might have the '|'
    // or '~' characters that we use as delimiters.  So, we need to encode
    // those strings differently:
    for(std::vector<std::pair<std::string, std::string>>::const_iterator condition = variable->conditions.begin();
        condition != variable->conditions.end();
        condition++)
      {
      ss << "c"                     << "|" ;  // type
      ss << ""                      << "|" ;  // level
      ss << ""                      << "|" ;  // name
      ss << condition->first        << "|" ;  // f_name
      ss << condition->second       << "|" ;  // b_name
      ss << ""                      << "|" ;  // a_name
      ss << ""                      << "|" ;  // offset
      ss << ""                      << "|" ;  // length
      ss << ""                      << "|" ;  // occurs_min
      ss << ""                      << "|" ;  // occurs_max
      ss << "~";                              // end-of-line
      }
    }
  }

void
VARIABLES::Insert(VARIABLE &v)
  {
  // From the "Department of Dirty Tricks at GnuCOBOL, we need to be able to handle
  // this weirdness:
  //
  //        /* cob_dump_field_ext ( 0, "TAB-ADR-IND", COB_SET_FLD(f0, 4, (cob_u8_t *)&b_25, &a_12), 0, 0); */
  //
  // That's a LEVEL 00 INDEXED variable.  I first encountered it in an OCCURS clause
  // with an "INDEXED BY" subclause.  The pointer cast is because indexed variables, in
  // this case b_25, are declared as "static int" rather than "static cob_u8_t[]", and
  // the cast avoids compiler warnings or errors when the statement is compiled.
  //
  // We, however, don't want the cast:
  static char achCast[] = "(cob_u8_t *)&";
  size_t nfound = v.b_name.find(achCast);
  if( nfound != string::npos )
    {
    v.b_name = v.b_name.replace(nfound,strlen(achCast),"");
    }
  variables.push_back(v);
  }

static int
VARIABLE_LEVEL(int a)
  {
  return ((a EQ 77 OR a EQ 78) ? 1 : a);
  }

static int
AddEmUp(const string &s)
  {
  int retval = 0;

  VSTRING tokens = Split(s,"+");
  for(size_t i=0; i<tokens.size(); i++)
    {
    string token = Trim(tokens[i]);
    if( !token.empty() AND isdigit(token[0]) )
      {
      retval += stoi(token);
      }
    }

  return retval;
  }

void
VARIABLES::FixLocalStorageOffsets()
  {
  // The purpose of this routine is to cope with the fact that
  // GnuCOBOL doesn't generate LOCAL-VARIABLE offset information
  // for variables that are not referenced in the body of the code.

  // So, we have to infer the locations within cob_local_ptr by the
  // lengths of each variable.

  // This is further complicated by observation that 01/77 variables are
  // placed on 16-byte boundaries.

  // We also have to take into account the possibility of REDEFINES, which
  // reset a variable to the same offset as the previous one on the same
  // level

  // Use a <map> to prevent O(N-squared) in the highly-theoretical and extremely
  // unlikely limit when handling REDEFINES.
  //
  // It would take an extremely long time to become N-squared, and even that
  // would require a huge input file with a lot of REDEFINES and other odd
  // circumstances.  But I couldn't make myself skip it.

  map<int,V_VARIABLES::iterator>elder_sibling;

  for( V_VARIABLES::iterator variable = variables.begin();
       variable!=variables.end();
       variable++)
    {
    if( variable->level EQ 66 )
      {
      // We can't handle Level 66 here
      continue;
      }

    if( variable->record_type EQ "L" )
      {
      int our_level = VARIABLE_LEVEL(variable->level);

      if( variable->redefines )
        {
        // Make sure our offset is the same as our elder sibling's offset
        map<int,V_VARIABLES::iterator>::iterator elder = elder_sibling.find(our_level);
        if( elder != elder_sibling.end() )
          {
          variable->offset = elder->second->offset;
          }
        }

      elder_sibling[our_level] = variable;

      if( variable->offset.empty() OR variable->offset EQ "0" )
        {
        // We have a possible rework here:
        // Find somebody older than us.  And, yes, this, too, is O(N-squared) in
        // the insane limit.  I half-thought of a couple of Nlog(N) ways of avoiding the N-squared,
        // and wouldn't bet that there isn't an O(N) possibilitiy.
        // But it doesn't matter.  It just doesn't.  And unlike the simple map for handling
        // redefines, this would require real work and testing.  Screw it.

        V_VARIABLES::iterator it = variable - 1;
        while( it->record_type EQ "L" AND our_level < VARIABLE_LEVEL(it->level) )
          {
          it--;
          }

        // our_level is >= it->level
        if( it->record_type EQ "L" AND our_level EQ VARIABLE_LEVEL(it->level) )
          {
          // We have found our immediately elder sibling
          // Replace us with his offset plus his length:

          // When I learned that offsets and lengths could variable, I turned
          // variable.length and variable.offset into strings, rather than
          // integers.  For this code, assume we have integers and names
          // separated by plus signs.  Ignore names.  That's all I can do
          // without samples.

          int parent_offset = AddEmUp(it->offset);
          int parent_length = AddEmUp(it->length);

          int our_offset = parent_offset + parent_length;

          // cob_local_ptr idiosyncracy:  Level 01 variables are allocated on
          // 16-byte boundaries:
          if( our_level EQ 1 )
            {
            our_offset = (our_offset + 15) & ~15;
            }
          variable->offset = to_string(our_offset);
          }
        else if( it->record_type EQ "L" AND our_level > VARIABLE_LEVEL(it->level) )
          {
          // We have found our parent.  Our offset is their offset
          variable->offset = it->offset;
          }
        }
      }
    }
  }

void
VARIABLES::FixTheLengths(const COB_FIELDS &cob_fields)
  {
  return;
  for( V_VARIABLES::iterator variable = variables.begin();
       variable!=variables.end();
       variable++)
    {
    size_t found1 = 0;
    for(;;)
      {
      size_t found_cob = variable->length.find("cob_", found1);
      found1 = variable->length.find("b_", found1);
      if( found1 == string::npos )
        {
        break;
        }
      // We encounter a conflict when the length is something like
      //        2 * (*(cob_u64_ptr)(b_43))
      if( found_cob != string::npos )
        {
        found1 = found_cob + 4;
        continue;
        }

      size_t found2 = found1+2;
      while( isdigit(variable->length[found2]) )
        {
        found2 += 1;
        }
      string b_name = variable->length.substr(found1, found2-found1);
      // cerr << "XXXXX " << b_name << std::endl;

      // Because there is something like a calculation in the length, the
      // variable is probably a DEPENDING ON variable.  In that case we very
      // might have seen its length in a .h file:

      // cerr << "XXXXX " << variable->name << variable->length << " " << variable->occurs_min << " " << variable->occurs_max << std::endl;

      size_t b_size = cob_fields.GetBSize(variable->b_name);

      if( b_size )
        {
        // For EXTERNAL variables, we see the size directly.
        char ach[32];
        sprintf(ach, "%zd", b_size);
        variable->length = ach;
        }
#if 1
      else
        {
        // This is going to be a bit weird.  This is probably a DEPENDING ON
        // situation.  In that case, we need to replace the b_name in the 
        // variable->length with the OCCURS_MAX value that we haven't seen yet
        V_VARIABLES::iterator itmax = variable + 1;
        while( itmax != variables.end() )
          {
          // cerr << itmax->name << itmax->length << " " << itmax->occurs_min << " " << itmax->occurs_max << std::endl;
          if( itmax->occurs_max > 0 )
            {
            char ach[32];
            sprintf(ach, "%d", itmax->occurs_max);
            variable->length.replace(found1, found2-found1, ach);
            break;
            }
          itmax += 1;
          }
        }
#endif

      found1 = found2;
      }
    }
  }
