#!/usr/bin/python3

################################################################################
# Copyright (c) 2019-2020 COBOLworx
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
################################################################################

#   This module implements the gdb extension commands cprint and cwatch

from __future__ import print_function

import re
import sys
import os
import traceback
import datetime
import subprocess
import ctypes
from ctypes import *

#   This module implements the gdb extension commands cprint and cwatch
#
#   This script is read by the COBST program and incorporated into a
#   C module that gets compiled and linked into the final executable
#
#   Decoding a COBOL variable will require the following information,
#   taken from ...\gnucobol-3.0-rc1\libcob\common.h
#
#   This structure will have an f_ symbol pointing to it.
#
#       typedef struct __cob_field
#           {
#           size_t                  size;       /* Field size */
#           unsigned char           *data;      /* Pointer to field data */
#           const cob_field_attr    *attr;      /* Pointer to attribute */
#           } cob_field;
#
#   cob_field.attr points to this structure, which will have an 'a_' symbol
#
#       typedef struct __cob_field_attr
#           {
#           unsigned short          type;       /* Field type */
#           unsigned short          digits;     /* Digit count */
#           signed short            scale;      /* Field scale */
#           unsigned short          flags;      /* Field flags */
#           const cob_pic_symbol    *pic;       /* Pointer to picture string */
#           } cob_field_attr;
#
#   cob_field_attr.pic, if present, will have a 'p_' symbol pointing to
#
#       typedef struct __cob_pic_symbol
#           {
#           char    symbol;
#           int     times_repeated;
#           } cob_pic_symbol;

#
#   /* Field types */
#
COB_TYPE_UNKNOWN             =  0x00    # 0
COB_TYPE_GROUP               =  0x01    # 1
COB_TYPE_BOOLEAN             =  0x02    # 2
COB_TYPE_NUMERIC             =  0x10    # 16
COB_TYPE_NUMERIC_DISPLAY     =  0x10    # 16
COB_TYPE_NUMERIC_BINARY      =  0x11    # 17
COB_TYPE_NUMERIC_PACKED      =  0x12    # 18
COB_TYPE_NUMERIC_FLOAT       =  0x13    # 19
COB_TYPE_NUMERIC_DOUBLE      =  0x14    # 20
COB_TYPE_NUMERIC_L_DOUBLE    =  0x15    # 21
COB_TYPE_NUMERIC_FP_DEC64    =  0x16    # 22
COB_TYPE_NUMERIC_FP_DEC128   =  0x17    # 23
COB_TYPE_NUMERIC_FP_BIN32    =  0x18    # 24
COB_TYPE_NUMERIC_FP_BIN64    =  0x19    # 25
COB_TYPE_NUMERIC_FP_BIN128   =  0x1A    # 26
COB_TYPE_NUMERIC_COMP5       =  0x1B    # 27
COB_TYPE_ALNUM               =  0x20    # 32
COB_TYPE_ALPHANUMERIC        =  0x21    # 33
COB_TYPE_ALPHANUMERIC_ALL    =  0x22    # 34
COB_TYPE_ALPHANUMERIC_EDITED =  0x23    # 35
COB_TYPE_NUMERIC_EDITED      =  0x24    # 36
COB_TYPE_NATIONAL            =  0x40    # 48
COB_TYPE_NATIONAL_EDITED     =  0x41    # 49

#
#   /* Field flags */
#
COB_FLAG_HAVE_SIGN      = (1 << 0)   # /* 0x0001 */
COB_FLAG_SIGN_SEPARATE  = (1 << 1)   # /* 0x0002 */
COB_FLAG_SIGN_LEADING   = (1 << 2)   # /* 0x0004 */
COB_FLAG_BLANK_ZERO     = (1 << 3)   # /* 0x0008 */
COB_FLAG_JUSTIFIED      = (1 << 4)   # /* 0x0010 */
COB_FLAG_BINARY_SWAP    = (1 << 5)   # /* 0x0020 */
COB_FLAG_REAL_BINARY    = (1 << 6)   # /* 0x0040 */
COB_FLAG_IS_POINTER     = (1 << 7)   # /* 0x0080 */
COB_FLAG_NO_SIGN_NIBBLE = (1 << 8)   # /* 0x0100 */
COB_FLAG_IS_FP          = (1 << 9)   # /* 0x0200 */
COB_FLAG_REAL_SIGN      = (1 << 10)  # /* 0x0400 */
COB_FLAG_BINARY_TRUNC   = (1 << 11)  # /* 0x0800 */
COB_FLAG_CONSTANT       = (1 << 12)  # /* 0x1000 */

class GlobalVariables() :
    def __init__(self) :
        ## Let's figure out if we are a 32-bit or 64-bit machine:
        self.SizeofPointer = 4     # Assume 32-bit
        arch = gdb.execute("show architecture",False,True)
        nfound = arch.find("x86-64")
        if nfound != -1 :
            self.SizeofPointer = 8

        ## Establish the size of a c-side int
        self.SizeofInt = ctypes.sizeof(ctypes.c_int)

        self.libraries_loaded = False

        self.ShortTermMemory = []
        self.pModule = c_void_p(0)
        self.pCobGlobPtr = c_void_p(0)
        self.repeat_count = 11  # Default value for "'0' <repeats 11 times>" logic

    def CobModuleGlobalEnter(self) :
        # We need to call
        #cob_module_global_enter (&module,       /* cob_module **module,  */
        #                         &cob_glob_ptr, /* cob_global **mglobal, */
        #                         1,             /* const int auto_init,  */
        #                         0,             /* const int entry,      */
        #                         0);            /* const unsigned int *name_hash) */
        try :
            ppModule = pointer(self.pModule)
            ppCobGlobPtr = pointer(self.pCobGlobPtr)
            One = c_int(1)
            Zero = c_int(0)

            self.LIBCOB.cob_module_global_enter(ppModule,
                                                ppCobGlobPtr,
                                                One,
                                                Zero,
                                                Zero)

            # We now have a cob_module and a cob_global set up in the GDB
            # process.  There's some information we need to copy over
            # from the target workspace:
            cob_current_module = None
            cobglobptr = GetPointerAt("cobglobptr")
            if cobglobptr :
                ## The beginning of cob_global looks like this (libcob/common.h)
                #    typedef struct __cob_global {
                #        cob_file     *cob_error_file;       /* Last error file */
                #        cob_module   *cob_current_module;   /* Current module */
                ## We need cob_module:
                cob_current_module = GetDataAt(cobglobptr+GV_GlobalVariables.SizeofPointer,GV_GlobalVariables.SizeofPointer)

                ## The information I need from the target, so that I can copy it into the
                ## GDB process space, are the last four char values seen below.  To find them
                ## I assume that and int is the same size as a pointer.  This is true for
                ## Linux/GCC.  It's not always true.  (WIN32, for instance, has them at 8 and 4 bytes in 64-bit mode)
                #
                #    typedef struct __cob_module {
                #        struct __cob_module    *next;          /* Next pointer */
                #        cob_field              **cob_procedure_params; /* Arguments */
                #        const char             *module_name;       /* Module name */
                #        const char             *module_formatted_date; /* Module full date */
                #        const char             *module_source;     /* Module source */
                #        cob_call_union         module_entry;       /* Module entry */
                #        cob_call_union         module_cancel;      /* Module cancel */
                #        const unsigned char    *collating_sequence;    /* COLLATING */
                #        cob_field              *crt_status;        /* CRT STATUS */
                #        cob_field              *cursor_pos;        /* CURSOR */
                #        unsigned int           *module_ref_count;  /* Module ref count */
                #        const char             **module_path;      /* Module path */
                #
                #        unsigned int       module_active;      /* Module is active */
                #        unsigned int       module_date;        /* Module num date */
                #        unsigned int       module_time;        /* Module num time */
                #        unsigned int       module_type;        /* Module type (program = 0, function = 1) */
                #        unsigned int       module_param_cnt;   /* Module param count */
                #        unsigned int       module_returning;   /* Module return type, currently unset+unused */
                #        int                module_num_params;  /* Module arg count */
                #
                #        unsigned char      ebcdic_sign;        /* DISPLAY SIGN */
                #        unsigned char      decimal_point;      /* DECIMAL POINT */
                #        unsigned char      currency_symbol;    /* CURRENCY */
                #        unsigned char      numeric_separator;  /* Separator */
                #
                ## Calculate offset of decimal_point
                offset = 12*GV_GlobalVariables.SizeofPointer + 7*GV_GlobalVariables.SizeofInt + 1
                characters = GetBytesAt(cob_current_module+offset,3)
                characters = bytearray(characters)

                decimal_point = characters[0]
                currency_symbol = characters[1]
                numeric_separator = characters[2]

                ## We got those parameters, through GDB, from the COBOL process.
                ## We now have to shove them into the cob_module we established
                ## with cob_module_global_enter() here in the GDB process.

                pp = cast(self.pModule,POINTER(c_byte))
                pp[offset+0] = decimal_point
                pp[offset+1] = currency_symbol
                pp[offset+2] = numeric_separator

        except :
            traceback.print_exc()
            sys.exit(1)

    def LoadLibcob(self) :
        if not self.libraries_loaded :
            shared_libraries = gdb.execute("info sharedlibrary",False,True)
            shared_libraries = shared_libraries.split('\n')

            self.LIBCOB = None
            libcob_name = None
            for slib in shared_libraries :
                # match "libcob.so" or "libcob-4.dll"
                m = re.match(r'.*[ ](.*libcob[.|-].*)$',slib)
                if m :
                    libcob_name = m.group(1)
                    break
            if libcob_name :
                self.LIBCOB = ctypes.CDLL(libcob_name)
                # And while we are in the neighborhood:
                ##self.LIBCOB.bob_standalone_initializer()
                self.CobModuleGlobalEnter()
                self.libraries_loaded = True

GV_GlobalVariables = GlobalVariables()

class COB_PIC_SYMBOL(Structure) :
    _fields_ = [("symbol"        , c_char),
                ("times_repeated", c_int)]

class COB_FIELD_ATTR(Structure) :
    _fields_ = [("type"  , c_ushort),
                ("digits", c_ushort),
                ("scale" , c_short) ,
                ("flags" , c_ushort),
                ("pic"   , POINTER(COB_PIC_SYMBOL))]

class COB_FIELD(Structure) :
    _fields_ = [("size", c_size_t),
                ("data", c_void_p),
                ("attr", POINTER(COB_FIELD_ATTR))]

    def __init__(self) :
        self.cob_field_attr = COB_FIELD_ATTR()

def NoNulls(s) :
    ss = ""
    for ch in s:
        if ord(ch) < 32 or ord(ch) >= 128 :
            ch = '.'
        ss += ch
    return ss

def OurPrint(s) :
    ## Use this routine when GDB might halt with a
    ##    --Type <RET> for more, q to quit, c to continue without paging--

    ## The following code catches the 'q' Quit option
    retval = None
    try :
        print(s)
    except KeyboardInterrupt as ki:
        print("Quit")
        retval = str(ki)
    except :
        traceback.print_exc()
    return retval

def LeftRight(s,splitchar) :
    """Splits what is presumably a numeric string with a possible
    splitting character into left and right pieces"""
    left = ""
    right = ""
    i = 0
    while i < len(s) :
        ch = s[i]
        i += 1
        if ch == splitchar :
            break
        left += ch
    while i < len(s) :
        ch = s[i]
        i += 1
        right += ch
    return left, right

def Picture(s) :
    """ Expands a 9(3)V9(2) to 999V999 """
    r = ""
    lch = ''
    ch = ''
    i = 0
    ncount = 0
    while i < len(s) :
        ch = s[i].upper()
        i += 1
        if ch == '(' :
            ncount = 0
            while i < len(s) :
                ch = s[i]
                i += 1
                if ch == ')' :
                    break;
                ncount *= 10
                ncount += ord(ch) - ord('0')
            ncount -= 1
            while ncount > 0 :
                ncount -= 1
                r += lch
        else :
            lch = ch
            r += ch
    # To avoid certain types of twitching on the part of your poor beleagured
    # programmer, replace the digrams 'CR' and 'DB' with 'c' and 'd'.  Note
    # that CR or DB, if present, are the final characters of the string.

    n = r.find("DB")
    if n > -1 :
        r = r[:n] + 'd'
    n = r.find("CR")
    if n > -1 :
        r = r[:n] + 'c'

    return r

def MachineInterfaceBody(s) :

#   This sequence:
#   variables=[{name="L00L",value="{_groupvalue=\"Sn\\\"ap!\",fullname=\"Robert\\\"Dubner\",L02={_groupvalue=\"Crackle!\",firstname = \"Robert\", lastname = \"\\\"Dubner\\\"\",L03={_groupvalue=\"Pop!\",firstname = \"Judy\", lastname = \"Ruderman\"}}}"}]
#
#   Results in a heirarchical display:
#
#    L00L: Sn"ap!
#        fullname : Robert"Dubner
#        L02: Crackle!
#            firstname: Robert
#            lastname: "Dubner"
#            L03: Pop!
#                firstname: Judy
#                lastname: Ruderman
#
#   This one:  variables=[{name="L01",value="===\">\\/<\"==="}]
#
#   Results in      ===">\/<"===
#
#   Counting up the backslashes is, to say the least, tricky

    # Replace double-quotes and backslashes
    retval = ""
    for ch in s :
        if ch == '"' :
            retval += r'\\\"'
        elif ch == '\\' :
            retval += r'\\\\'
        else :
            retval += ch

    return retval

def MachineInterfaceBodyAlt(s) :

    # Replace double-quotes and backslashes
    retval = ""
    for ch in s :
        if ch == '"' :
            retval += r'\"'
        elif ch == '\\' :
            retval += r'\\'
        else :
            retval += ch

    return retval


def ReplaceWithRepeatCountQuoted(s) :
    # We know this guy has a leading quote, and presumably a trailing one
    #print("ReplaceWithRepeatCountQuoted(1)", s)
    retval = ""
    ## Strip off the leading and trailing quotes
    s  = s[1:-1]
    rabbit = 0
    fox = 0
    count = 0
    prior_contraction = False
    in_a_quoted_run = False
    number_of_repeated_runs = 0
    while rabbit < len(s) :
        ## Arriving here means:
        ## We're here for the first time, or we just finished a run
        ## So, we are going to pick up 'char', which is a candidate
        ## for the start of a new run

        fox = rabbit
        char = s[rabbit]
        rabbit += 1
        while rabbit < len(s) and s[rabbit] == char :
            ## This is where we proceed in a run.  Swallow up characters
            ## that are the same as our candidate:
            rabbit += 1

        ## We have either run out of characters, or else
        ## rabbit points to a character other than char
        if rabbit - fox >= GV_GlobalVariables.repeat_count : ## This is a magic number
            ## At 29, the <repeats> clause will be shorter than the replaced text
            ## At 11, the string will be longer.  But 11 matches what gdb does
            if in_a_quoted_run:
                ## We have to end the current ongoing string
                ## and start this contraction with a prelude
                retval += "\", "
            ## and now we can start the contraction:
            retval += "\'"
            retval += char
            retval += "\' <repeats "
            retval += str(rabbit-fox)
            retval += ' times>, '
            prior_contraction = True
            in_a_quoted_run = False
            number_of_repeated_runs += 1
        else :
            ## The run is short enough that we just put the characters
            ## into place
            if not in_a_quoted_run :
                ## We have to start the quote:
                retval += "\""
                in_a_quoted_run = True
            ## Copy over characters until the fox catches up with the rabbit
            while fox < rabbit :
                fox += 1
                retval += char
            prior_contraction = False

    ## We have reached the end
    if prior_contraction :
        ## strip off the unnecessary ', '
        retval = retval[:-2]
    if in_a_quoted_run :
        retval += "\""

    #print("ReplaceWithRepeatCountQuoted(2)", retval)
    return retval

def ReplaceWithRepeatCountBracketed(s) :
    if (len(s) % 2) == 1 :
        ## We don't know what to do with an odd-length string.  We just don't
        return s
    # We know this guy has a leading bracket, and presumably a trailing one
    # In between are a stream of undecorated hex characters.  So, this is
    # a lot like the quoted version, except we operate in pairs, rather than characters
    #print("ReplaceWithRepeatCountBracketed(1)", s)
    retval = "["
    ## Strip off the leading and trailing brackets
    s  = s[1:-1]
    rabbit = 0
    fox = 0
    count = 0
    prior_contraction = False
    in_a_quoted_run = False
    number_of_repeated_runs = 0
    while rabbit < len(s) :
        ## Arriving here means:
        ## We're here for the first time, or we just finished a run
        ## So, we are going to pick up 'char', which is a candidate
        ## for the start of a new run

        fox = rabbit
        char = s[rabbit:rabbit+2]
        rabbit += 2
        while rabbit < len(s) and s[rabbit:rabbit+2] == char :
            ## This is where we proceed in a run.  Swallow up characters
            ## that are the same as our candidate:
            rabbit += 2

        ## We have either run out of characters, or else
        ## rabbit points to a character other than char
        if (rabbit - fox)//2 >= GV_GlobalVariables.repeat_count : ## This is a magic number.  See the
                                     ## comment in ReplaceWithRepeatCountQuoted()
            if in_a_quoted_run:
                ## We have to end the current ongoing string
                ## and start this contraction with a prelude
                retval += '", '
            ## and now we can start the contraction:
            retval += char
            retval += " <repeats "
            retval += str((rabbit-fox)//2)
            retval += ' times>, '
            prior_contraction = True
            in_a_quoted_run = False
            number_of_repeated_runs += 1
        else :
            ## The run is short enough that we just put the characters
            ## into place
            if not in_a_quoted_run :
                in_a_quoted_run = True
            ## Copy over characters until the fox catches up with the rabbit
            while fox < rabbit :
                fox += 2
                retval += char
            prior_contraction = False

    ## We have reached the end
    if prior_contraction :
        ## strip off the unnecessary ', '
        retval = retval[:-2]
    retval += ']'

    #print("ReplaceWithRepeatCountBracketed(2)", retval)
    return retval

def ReplaceWithRepeatCount(s) :
    try :
        if s and (s[0] == '"' or s[0] == '<' ):
            return ReplaceWithRepeatCountQuoted(s)
        if s and s[0] == '[' :
            return ReplaceWithRepeatCountBracketed(s)
        return s
    except :
        traceback.print_exc()
        sys.exit(1)

class VarTrieNode() :
    def __init__(self):
        self.parent = None
        self.children = {}
        self.piece_of_name = ""
        self.count = 0
        self.payload_index = None

    def AddCanonicalName(self,name) :
        pass

def BlankIsZero(v) :
    if len(v) == 0 :
        v = "0"
    return int(v)

def GetDataAt(address,bytes) :
    # This routine takes an address and returns the
    # value there.  'bytes' can be 1, 2, 4, or 8
    try :
        if bytes == 1 :
            u = 'b'
        elif bytes == 2 :
            u = 'h'
        elif bytes == 4 :
            u = 'w'
        elif bytes == 8 :
            u = 'g'
        else :
            return None

        command = "x/1" + u + "d " + str(address)
        value_back = gdb.execute(command,False,True)

        nfound = value_back.find(":")
        if nfound == -1 :
            return None
        value = int(   value_back[nfound+1:] )
        return value

    except gdb.error:
        print("gdb.execute failed in GetDataAt:",command)
        return None

def GetBytesAt(address,N) :
    # Pick up N bytes from address, return them as a list of ints
    retval = []
    command = "x/" + str(N) + "xb " + hex(address)
    all_the_bytes = gdb.execute(command,False,True)
    lines = all_the_bytes.split('\n')
    for line in lines :
        nfound = line.find(':')
        if nfound != -1 :
            bytes = line[nfound+1:].split( )
            for byte in bytes :
                retval.append( int(byte,16) )
    return retval;

def ExtractHexValueFrom(s) :
    ## Use this routine to extract an 0x123abc value from a string.
    ## Needed because gdb returns such information in slightly different ways
    retval = None
    match = re.match(".*(0x[0-9a-fA-F]+).*",s)
    if match :
        retval = int(match.group(1),16)
    return retval;

def GetPointerAt(variable) :
    # Use this form when you know variable is the name of a pointer.
    # For example, I wrote this when I needed to pick up cobglobptr.  I
    # didn't really care where cobglobptr was; I cared about the memory
    # cob_global block it pointed to.
    try :
        return ExtractHexValueFrom(gdb.execute("x/1db (char *) " + variable,False,True))
    except :
        return None

def GetAddressOf(variable) :
    # Use this form when you know variable is the name of a block.
    retval = None
    if variable :
        try :
            command = "info address " + variable
            address_string = gdb.execute(command,False,True)
            val_info = ExtractHexValueFrom(address_string)
        except gdb.error:
            val_info = None

        try :
            command = "x/1db " + variable  ## Notice d for decimal, not hex, because
                                           ## a second 0x messes up ExtractHexValue
            address_string = gdb.execute(command,False,True)
            val_x1db = ExtractHexValueFrom(address_string)
        except :
            val_x1db = None

        retval = val_x1db
        if val_x1db == None :
            retval = val_info
    return retval

class VarTrie() :
    def __init__(self):
        self.storage_list = []
        self.storage_trie = VarTrieNode()
        self.program_ids = set()

        self.IndexType = 0
        self.IndexLevel = 1
        self.IndexName = 2
        self.IndexFname = 3
        self.IndexBname = 4
        self.IndexAname = 5
        self.IndexOffset = 6
        self.IndexLength = 7
        self.IndexOccursMin = 8
        self.IndexOccursMax = 9

        self.StackOfNames = []
        self.StackOfLevels = []

    def AddToTrie(self,trie,name_tokens,payload_index) :
        our_node = trie

        for name in name_tokens :
            if name not in our_node.children :
                new_child = VarTrieNode()
                new_child.piece_of_name = name
                new_child.parent = our_node
                our_node.children[name] = new_child
            our_node = our_node.children[name]

        # We're all the way at the end:
        our_node.payload_index = payload_index
        our_node.count = 1

        # Propogate the count back up to the root:
        while our_node.parent :
            our_node.parent.count += our_node.count
            our_node = our_node.parent

        # and we are done

    def LoadFromLine(self,variable_line) :
        tokens = variable_line.split("|")
        try :
            if len(tokens) == 11 :
                ## We have enough in the way of tokens, so proceed
                if tokens[self.IndexType] == 'P' :
                    ## We are entering a new PROGRAM-ID, so reset our stacks
                    self.StackOfNames = [ tokens[self.IndexName] ]
                    self.StackOfLevels = [-2]

                if len(self.StackOfNames) == 0 :
                    print("Please save your program for Bob Dubner at support@COBOLworx.com")
                    print("self.StackOfNames is empty, and it shouldn't be")
                    sys.exit(1)

                if tokens[self.IndexType] in ('F','W','L','I') :
                    ## We are working with a variable.  We need to erode the
                    ## stack of names down to the variable who is our parent:
                    if tokens[self.IndexLevel] == "" :
                        tokens[self.IndexLevel] = '0'
                    our_level = int(tokens[self.IndexLevel])
                    if our_level in (77,78) :
                        our_level = 1
                    while self.StackOfLevels[-1] >= our_level :
                        self.StackOfNames.pop(-1)
                        self.StackOfLevels.pop(-1)

                    ## and now we put ourselves into the position of
                    ## most favored child
                    self.StackOfNames.append(tokens[self.IndexName])
                    self.StackOfLevels.append(our_level)

                    reversed = self.StackOfNames[:]
                    reversed.reverse()

                    payload = CobolVariable()
                    payload.Program    = self.StackOfNames[0]
                    payload.Section    = tokens[self.IndexType]
                    payload.Level      = int(tokens[self.IndexLevel])
                    payload.Name       = '/'.join(reversed)
                    payload.Fname      = tokens[self.IndexFname]
                    payload.Bname      = tokens[self.IndexBname]
                    payload.Aname      = tokens[self.IndexAname]
                    payload.Offset     = int(tokens[self.IndexOffset] or "0")
                    payload.Length     = int(tokens[self.IndexLength] or "0")
                    payload.OccursMin  = int(tokens[self.IndexOffset] or "0")
                    payload.OccursMax  = int(tokens[self.IndexLength] or "0")

                    payload.payload_index = len(self.storage_list)
                    self.storage_list.append(payload)
                    self.AddToTrie(self.storage_trie,
                                   reversed,
                                   payload.payload_index)
                    self.program_ids.add(payload.Program)
        except :
            traceback.print_exc()
            sys.exit(1)

    def TokenizeCanonicalName(self,desired) :
        # We allow "A of B in C", where [of|in] is not case sensitive
        # We also allow A.B.C, A/B/C, and "A B C"

        # turn IN and OF into spaces
        pattern = re.compile(r"^(.*) (in|In|iN|IN|of|oF|Of|OF) (.*)$")
        while True :
            match = re.search(pattern,desired)
            if not match :
                break
            else :
                desired = match.group(1) + " " + match.group(3)

        desired = desired.replace("."," ")
        desired = desired.replace("/"," ")
        retval = desired.split()
        return retval

    def NameMatcher(self,
                    boys,
                    boy_index,
                    trie,
                    paths) :
        # Nomenclature:  We have a list of lost_boys: A/B/C
        # We have a forest, where some of the trees are labeled.
        # The labels might be
        # A.B.C (Just one valid path)
        # X.A.B.C.Y (Just one valid path)
        # A.B.C.X.Y.Z (Just one valid path)
        # A.X.B.Y.C.Z (Just one path)
        # A.B.C.X and A.B.C.Y (two valid paths)
        # A.B.X.C and A.B.Y.C (two valid paths)
        # And so on.
        #
        # We want to wander through the forest, creating a list of paths that
        # leave off each boy at his own tree.
        #
        # In classic recursive-entry style, this routine took a lot of thinking
        # but ends up with very little code that is swamped by explanatory
        # comments.
        #
        # The intent is for this to be unimportant to the user.  It should
        # just work *right*.  If the user asks "show me FRED" and there is
        # only one possibility, then the user gets that.  If there are two
        # possibilities, then the user gets shown those possibilities with a
        # simple option for selecting one.
        #
        if boy_index < len(boys) :
            # we still have at least one boy to leave off at his tree
            boy = boys[boy_index]
            #print("DEBUG boy",boy)
            #print("DEBUG trie.children.keys",trie.children.keys())
            # Do a case-insensitive match:
            bFound = False;
            for child in trie.children :
                if boy.upper() == child.upper() :
                    bFound = True
                    boy = child
                    break
            if bFound :
                # We have found this boy's tree.  Advance both the boy
                # and the trie.
                next_trie = trie.children[boy]
                boy_index += 1
                # Continue along that path.
                self.NameMatcher(boys,boy_index,next_trie,paths)
                # and terminate.
                return
            else :
                # We can't leave that boy at this node.  So, we have to
                # check for that boy on all possible paths from here:
                child_tries = list(trie.children.values())
                for next_trie in child_tries :
                    self.NameMatcher(boys,boy_index,next_trie,paths)
                # and terminate.
                return
        else :
            # we have dropped off the whole string of boys.  Now it's a
            # question of finding every possible path to the edge of the
            # forest from that point.
            if len(trie.children) > 0 :
                # We have not yet reached the edge of the forest:
                child_tries = list(trie.children.values())
                for next_trie in child_tries :
                    self.NameMatcher(boys,boy_index,next_trie,paths)
                # and terminate.
                return
            else :
                # This node has no children, which means we have reached the
                # edge of the forest.

                # Log it:
                paths.append(trie.payload_index)
                #
                # and terminate
                return

    def GetAllPossibilities(self) :
        # Typically in response to a 'print *'
        paths = []
        for index in range(len(self.storage_list)) :
            paths.append(index)
        return paths

    def GetListOfExactPossibilities(self,token) :
        # We are going to return variables that contain the given token
        # in its exact, case-insensitive, form.  This is typically used
        # by the 'print ?' form

        # Wander through the forest, looking for possible paths:
        paths = []
        self.NameMatcher([token],0,self.storage_trie,paths)

        return paths

    def GetListOfPossibilities(self,desired) :
        if isinstance(desired,list) :
            # We were handed a bunch of pieces.  Let's
            # glue them back together into a single space-separated string:
            desired = " ".join(desired).upper()

        # Go handle our various "A of B" possibilities:
        name_tokens = self.TokenizeCanonicalName(desired)

        # Wander through the forest, looking for possible paths:
        paths = []

        if len(name_tokens) == 0 :
            # if there's nothing, there is nothing
            return paths

        self.NameMatcher(name_tokens,0,self.storage_trie,paths)

        if len(paths) != 1 :
            # A strict attempt at matching what we were given has come up
            # with bupkis.  So far we have either no possibilities, or
            # more than one possibility.

            # Put what we have so far into a set:
            set1 = set(paths)

            ## When given something like FIRST IN HOLDER, we want
            ## to find 10 FIRSTNAME/NAME/ACCOUNTHOLDER
            ##
            ## But we don't want that request to pull up
            ##  10 HOLDER/NAME/FIRST
            ##
            ## So, we are going to march through all of the names
            ## in storage_list, and then we will make sure all of
            ## the pieces show up in left-to-right order

            for index in range(len(self.storage_list)) :
                whole_name = self.storage_list[index].Name.upper()
                prior_index = -1
                for fragment in name_tokens :
                    fragment = fragment.upper()
                    current_index = whole_name.find(fragment,prior_index+1)
                    if current_index == -1 :
                        ## This fragment isn't in that name, so, sayonara
                        prior_index = -1
                        break
                    if current_index < prior_index :
                        ## Going backward isn't permitted
                        prior_index = -1
                        break;
                    ## Rightward, ho!
                    prior_index = current_index
                if prior_index != -1 :
                    ## We have found a contender!
                    set1.add(index)

            ## Turn the set back into a list:
            paths = list(set1)

        return paths

class LineList() :
    """List of starting lines of ProgramNames"""

    # The point of this list is to be able to figure out, when a trap occurs,
    # which COBOL routine the trap occurred in.  We can't just look at the
    # information from the gdb "frame" command, because the .c code doesn't
    # use the exact same name. It's similar, but it's transmogrified.  But
    # we can know the line number where the trap occurred, and scanning
    # the .tab file lets us convert line numbers to the original COBOL
    # program-id

    def __init__(self) :
        self.line_list = [] # This will be a list of (int,str) tuples
        self.EndOfTheWorld = 10000000

    def Insert(self,line_number , program_id ) :
        self.line_list.append( (line_number,program_id) )

    def Find(self,line_number ) :
        # returns the program-id that covers line_number

        if len(self.line_list) == 0 :
            return ""

        if line_number < self.line_list[0][0] or line_number >= self.line_list[-1][0] :
            return ""

        left = 0
        right = len(self.line_list)

        while True :
            middle = (right+left)//2
            if self.line_list[middle][0] <= line_number \
                    and line_number < self.line_list[middle+1][0]:
                break
            if self.line_list[middle][0] > line_number :
                right = middle
            else :
                left = middle

        return self.line_list[middle][1]

    def LoadFromLine(self,variable_line) :
        if len(variable_line) > 0 and variable_line[0] == 'P' :
            tokens = variable_line.split("|")
            if len(tokens) >= 3 :
                program_id = tokens[2]
                line_number = int(tokens[6])
                self.Insert(line_number,program_id)

class CobolVariable() :
    """Manages the field, attributes, flags, and pointers of a GnuCOBOL COBOL variable"""
    def __init__ (self) :
        self.index      = None
        self.Program    = ""
        self.Section    = ""
        self.Level      = -1
        self.Name       = ""
        self.Fname      = ""
        self.Bname      = ""
        self.Aname      = ""
        self.Offset     = 0
        self.Length     = 0
        self.OccursMin  = 0
        self.OccursMax  = 0

        self.AttrType   = 0
        self.AttrDigits = 0
        self.AttrScale  = 0
        self.AttrFlags  = 0

        self.location_of_field     = 0  # value of Fname
        self.location_of_data      = 0  # value of Fname->data
        self.location_of_attribute = 0  # value of Fname->attr
        self.data       = []            # list of Length bytes

        self.decimal_point = '.'        # Eventually we are going to have to
        self.comma_separator = ','      # handle DECIMAL-POINT IS COMMA
        self.currency_symbol = '$'

        # Here is the string information decoded from the attributes and
        # the data we get from gdb:
        self.not_in_context = False
        self.display_name = ""
        self.display_body = ""
        self.display_pic  = ""

        # This section is used for "StringToxxx processing
        self.rvalue = ""           # Original user string, before cleaning
        self.cleaned_rvalue = ""   # This will have exactly AttrDigits, zero-filled
        #                          # as necessary to the left and right
        self.is_negative = False   # True if there was a minus sign
        self.expanded_picture = "" # "9(4)" becomes "9999" here
        self.ldigits = -1          # this integer is AttrDigits - AttrScale
        self.rdigits = -1          # this integer is AttrScale...
        #                          # ...but see self.CleanAndScale

    def CleanTokens(self,execute_result) :
        # (This was created during initial development.  During refactoring
        #  to use cobst data rather than collecting field and attribute
        #  information via gdb.execute, it became superfluous.  I am leaving
        #  it here in case it becomes needed again.  RJD 2019-07-24 v0.1)

        # cgdb returns wildly different results from dbg.execute("print ...")
        # than the same function call in straight gdb .  This routine normalizes
        # the two, returning a list that always has the data we care about
        # right after the keyword

        # Because, as per Finagle, the perversity of the universe *does*
        # tend towards a maximum, and because users are agents of chaos,
        # I am going to strip out anything between two double-quotes,
        # because otherwise some user *will* create a string that will
        # confuse the living crap out of this routine!

        left = execute_result.find('"')
        right = execute_result.rfind('"')
        if left != -1 and right != -1 :
            execute_result = execute_result[:left] + execute_result[right+1:]

        # Turn extraneous characters into spaces, to mollify split()
        execute_result = execute_result.replace("'",' ')
        execute_result = execute_result.replace("{",' ')
        execute_result = execute_result.replace("}",' ')
        execute_result = execute_result.replace(",",' ')
        execute_result = execute_result.replace("<",' ')
        execute_result = execute_result.replace(">",' ')

        ex_tokens = execute_result.split()

        # Get rid of superfluous tokens. cgdb has a bunch whose initial
        # characters are \x1a characters; we don't want them.  Nor do we
        # want 0x55555555, although we will pass through 0x0 (because that's
        # what we see for a picture pointer when there is no picture)
        tokens = []
        for xtoken in ex_tokens :
            if      ((ord(xtoken[0]) > 32
                    and xtoken != '='
                    and xtoken != '*'
                    and xtoken != '-'
                    and xtoken[0] != '0')
                    or xtoken == "0"
                    or xtoken == "0x0") :
                tokens.append(xtoken)
        return tokens

    def GetPicSymbols(self) :
        retval = []
        # Picture data is a variable-sized list of
        #   typedef struct __cob_pic_symbol
        #        {
        #        char	symbol;
        #        int 	times_repeated;
        #        } cob_pic_symbol;
        #
        # The final one is flagged by a symbol value of '\0'
        # The only slightly tricky thing is that the structure can be
        # padded
        #
        if self.location_of_pic :
            p = self.location_of_pic
            sizeof_pic_symbol = sizeof(COB_PIC_SYMBOL)
            while True :
                psym = GetBytesAt(p,sizeof_pic_symbol)
                retval = retval + psym
                if psym[0] == 0 :
                    break;
                p += sizeof_pic_symbol
        return retval

    def GetWorkingCopyOfCobField(self) :
        # This routine returns, here in GDB/Python space, a duplicate
        # of this CobolVariable's cob_field variable as defined in the
        # the target COBOL executable's space

        cob_field = COB_FIELD()

        ## Copy over the data into the COB_FIELD object
        pp = bytearray(self.data)
        p_variable_data = create_string_buffer(len(self.data))
        p_variable_data = cast(p_variable_data,POINTER(c_byte))
        for i in range(len(self.data)) :
            p_variable_data[i] = pp[i]

        ## Get the picture string, if there is one, as a list of integers
        picture_data = self.GetPicSymbols()
        p_picture_data = 0
        if len(picture_data) :
            p_picture_data = create_string_buffer(len(picture_data))
            for i in range(len(picture_data)) :
                p_picture_data[i] = picture_data[i]

        cob_field_attr = COB_FIELD_ATTR()
        cob_field_attr.type   = self.AttrType
        cob_field_attr.digits = self.AttrDigits
        cob_field_attr.scale  = self.AttrScale
        cob_field_attr.flags  = self.AttrFlags
        cob_field_attr.pic    = cast(p_picture_data, POINTER(COB_PIC_SYMBOL))

        cob_field = COB_FIELD()
        cob_field.size = self.Length
        cob_field.data = cast(p_variable_data,c_void_p)
        cob_field.attr = pointer(cob_field_attr)
        return cob_field

    def GetDataAsString(self) :
        retval = ""

        if False :  # For development
            print("GetData AsString          Name:"   ,self.Name);
            print("GetData AsString       Section:"   ,self.Section);
            print("GetData AsString         Fname:"   ,self.Fname);
            print("GetData AsString         Bname:"   ,self.Bname);
            print("GetData AsString         Aname:"   ,self.Aname);
            print("GetData AsString        Offset:"   ,self.Offset);
            print("GetData AsString        Length:"   ,self.Length);
            print("GetData AsString len(self.data)"  ,len(self.data));
            print("GetData AsString      AttrType:"   ,hex(self.AttrType));
            print("GetData AsString    AttrDigits:"   ,self.AttrDigits);
            print("GetData AsString     AttrScale:"   ,self.AttrScale);
            print("GetData AsString     AttrFlags:"   ,hex(self.AttrFlags));
            print("GetData AsString   loc_of_data:"   ,hex(self.location_of_data));
            print("GetData AsString   loc_of_attr:"   ,hex(self.location_of_attribute));
            print("GetData AsString    loc_of_pic:"   ,hex(self.location_of_pic));

        ## Something we need to know:
        is_alphanumeric = False
        if self.AttrType in ( COB_TYPE_UNKNOWN,
                              COB_TYPE_GROUP,
                              COB_TYPE_BOOLEAN,
                              COB_TYPE_ALNUM,
                              COB_TYPE_ALPHANUMERIC,
                              COB_TYPE_ALPHANUMERIC_ALL,
                              COB_TYPE_ALPHANUMERIC_EDITED,
                              COB_TYPE_NUMERIC_EDITED,
                              COB_TYPE_NATIONAL,
                              COB_TYPE_NATIONAL_EDITED ) :
            is_alphanumeric = True

        ## By the time we get here, we have pulled all the information we
        ## can from the data we got from the compiler, and also from the
        ## run-time information that the compiler pointed us to.

        ## We can't feed those pointers directly to libcob from here, because they
        ## came from the memory space of a different process, and thus
        ## we get segmentation faults.  So, we are going to build a local
        ## copy that we can send to libcob using ctypes

        cob_field = self.GetWorkingCopyOfCobField()

        if is_alphanumeric :
            # It's not impossible for there to be bum characters in what we call
            # an alphanumeric field.  If a GROUP has a mixture of numeric and alphanumeric
            # characters, for example.  Or if the COBOL is trapped in a place before a
            # variable has been initialized.  Let's give the cob_get_field_string a
            # helping hand with that
            pd = cast(cob_field.data,POINTER(c_byte))
            for i in range(self.Length) :
                # Assume ASCII
                if pd[i] < 32 or pd[i] >= 127 :
                    pd[i] = ord('.')

        ## cob_field is now a duplicate, in our GDB/Python memory space, of the
        ## relevant f_ (or synthesized f0 equivalent) cob_field variable that
        ## we used GDB to pick up from the COBOL memory space.

        length = self.Length + 128
        #buffer = ctypes.create_string_buffer(b'\000' * length)
        buffer = ctypes.create_string_buffer(length)
        size = ctypes.c_size_t(length)

        #print("About to call cob_get_field_str")
        GV_GlobalVariables.LIBCOB.cob_get_field_str.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_size_t]
        GV_GlobalVariables.LIBCOB.cob_get_field_str(byref(cob_field),buffer,size)

        try :
            retval = buffer.value.decode("utf-8")
            if is_alphanumeric :
                retval = '"' + retval + '"'
        except :
            # While 3.1 was still in the release candidate stage,
            # an attempt to get at some variables resulted in
            # utf-8 failures.  Whether or this is still the case as you
            # read this... Reply hazy, try again.
            retval = "<non-utf-8 return from cob_get_field_str>"
            self.not_in_context = True

        return retval

    def PutDataAsString(self,rside) :
        cob_field = self.GetWorkingCopyOfCobField()

        string_length = len(rside)
        buffer = ctypes.create_string_buffer(b'\000' * string_length)
        rbytes = rside.encode("utf-8")
        for i in range(string_length) :
            buffer[i] = rbytes[i];

        #print("About to call cob_put_field_str")
        GV_GlobalVariables.LIBCOB.cob_put_field_str.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
        retval = GV_GlobalVariables.LIBCOB.cob_put_field_str(byref(cob_field),buffer)
        if retval == 0 :
            # We got a SUCCESS response from cob_put_field
            p = self.location_of_data
            p_cob_field_data = cast(cob_field.data,POINTER(c_byte))

            ## Take the bytes we got back from cob_put_field_str and
            ## shove them into place in the COBOL process's memory:
            for i in range(self.Length) :
                self.data[i] = p_cob_field_data[i]
                command = "set {char}" + hex(p) + "=" + hex(self.data[i])
                gdb.execute(command,False,True)
                p += 1

    def FetchVariableData(self, line_list , show_failure ) :
        #print("FetchVariableData:",self.Name,self.Section)
        self.location_of_field = 0;
        self.location_of_data = 0;
        self.location_of_attribute = 0;
        self.data = []
        self.display_body = ""
        self.display_pic = ""
        self.display_name = self.NameToString()
        self.not_in_context = False

        ## Let's update everything we might need:

        ## First, check to see if the compiler provided us with the
        ## the name of the field.  If so, use the run-time values in that named
        ## field to override anything we might have gotten from compile-time

        if self.Fname :
            # We have a field name
            #print("FetchVariableData Fname", self.Fname)
            self.location_of_field = GetAddressOf(self.Fname)
            if self.location_of_field == None :
                self.not_in_context = True
                return
            # The cob_field structure looks like this:
            #   typedef struct __cob_field
            #       {
            #       size_t                size;
            #       unsigned char        *data;
            #       const cob_field_attr *attr;
            #       } cob_field;
            self.Length = GetDataAt(self.location_of_field,GV_GlobalVariables.SizeofPointer)
            if self.Length == None :
                self.not_in_context = True
                return
            possible_location_of_data = GetDataAt(self.location_of_field+GV_GlobalVariables.SizeofPointer,GV_GlobalVariables.SizeofPointer)
            if possible_location_of_data :
                self.location_of_data = possible_location_of_data
            self.location_of_attribute = GetDataAt(self.location_of_field+2*GV_GlobalVariables.SizeofPointer,GV_GlobalVariables.SizeofPointer)
        else :
            #print("There is no self.Fname")
            pass

        if self.location_of_data == 0 :
            # When the COBOL code doesn't reference a variable, there is no f_ cob_field.  But
            # sometimes we are able to divine a b_ + offset reference.  Likewise, sometimes
            # there is an f_value, but at this point in the code the cob_field->data value hasn't yet
            # been set up.  But, again, sometimes we have advance knowledge of what is going to be
            # there because we got information from the compiler.
            if self.Bname :
                #print("FetchVariableData Fname", self.Bname)
                self.location_of_data  = GetAddressOf(self.Bname)
                if self.location_of_data == None :
                    self.not_in_context = True
                    return
                self.location_of_data += self.Offset
            else :
                #print("There is no self.Bname")
                pass

        if self.location_of_attribute == 0 :
            if self.Aname :
                self.location_of_attribute = GetAddressOf(self.Aname)
                if self.location_of_attribute == None :
                    self.not_in_context = True
                    return
            else :
                #print("There is no self.Aname")
                pass

        if self.location_of_attribute != 0 :
            #   typedef struct __cob_field_attr
            #       {
            #       unsigned short      type;       /* Field type */
            #       unsigned short      digits;     /* Digit count */
            #       signed short        scale;      /* Field scale */
            #       unsigned short      flags;      /* Field flags */
            #       const cob_pic_symbol    *pic;       /* Pointer to picture string */
            #       } cob_field_attr;
            self.AttrType   = GetDataAt(self.location_of_attribute+0,2)
            self.AttrDigits = GetDataAt(self.location_of_attribute+2,2)
            self.AttrScale  = GetDataAt(self.location_of_attribute+4,2)
            self.AttrFlags  = GetDataAt(self.location_of_attribute+6,2)
            self.location_of_pic = GetDataAt(self.location_of_attribute+8,GV_GlobalVariables.SizeofPointer)

        if self.Section in('F','W','I') :
            pass
        elif self.Section == 'L' :
            try :
                # We are looking for something in LOCAL-STORAGE.  Local
                # storage is kept in a calloc-ed memory location named
                # cob_local_ptr.  The complication is that if we have
                # several program-ids that each have their own cob_local_ptr
                # we have to make sure we know which one we are dealing
                # with.

                # Pick up the decimal line number where we are trapped
                line_number = -1
                frame_string = gdb.execute("frame",False,True)
                # Look for a number after a colon
                nfound = frame_string.find(':')
                if nfound > -1 :
                    frame_string = frame_string[nfound+1:]
                    frame_string = frame_string.split()
                    try :
                        line_number = int(frame_string[0])
                    except :
                        line_number = -1

                #print("Trapped at ",line_number)
                # We know we are trapped at line_number

                # Look up the program-id of that line_number
                program_id = line_list.Find(line_number)
                #print("in :", program_id)

                #
                # program-id is where the program-under-test is
                # trapped.  Let's see if that program-id matches
                # the final place of the desired variable:
                #
                #print( self.Name )
                split_name = self.Name.split('/')
                if split_name[-1] != program_id :
                    self.not_in_context = True
                    return

                # To recap:  We are trapped in the routine of the desired
                # LOCAL-STORAGE variable, so we can look for it in cob_local_ptr
                # we need GDB's help finding out where cob_local_ptr + offset is:
                command = "x " + "cob_local_ptr + " + str(self.Offset)
                address_string = gdb.execute(command,False,True)
                nfound = address_string.find(':')
                if nfound != -1 :
                    self.location_of_data = int( address_string[:nfound], 16 )

            except gdb.error:
                self.not_in_context = True
        else :
            print("We don't know how to handle",self.Section, self.Name)
            self.not_in_context = True
            return

        if self.location_of_data != 0 :
            self.data = GetBytesAt(self.location_of_data,self.Length)

        self.display_body = self.GetDataAsString()
        self.display_body = ReplaceWithRepeatCount(self.display_body)

        if self.Length != len(self.data) :
            ## This probably should never happen.  But when it did, during debugging,
            ## it actually corrupted memory during the process of creating a
            ## cfields.cob_field.
            ##
            ## So, let's put up a fence:
            self.not_in_context = True
        return

    def NameToString(self) :
        storage_type = ""
        if self.Section == "F" :
            storage_type = "I/O"
        elif self.Section == "W" :
            storage_type = "W-S"
        elif self.Section == "L" :
            storage_type = "L-S"
        elif self.Section == "I" :
            storage_type = "LNK"

        s = "{1:02} {2} [{0}]".format(storage_type,self.Level,self.Name)
        return s

    def ShowFields(self) :
        bname_location = GetAddressOf(self.Bname) or 0
        bname_text = ""
        if bname_location :
            bname_text = "( %s )" % hex(self.location_of_field)

        if self.Section == 'L' :
            field_loc = "cob_local_ptr (" + hex(self.location_of_data) + ")"
        else :
            field_loc = hex(self.location_of_data)

        print("Name:       " , self.display_name    )
        print("Field:      " , self.Fname,'   (',hex(self.location_of_field),')' )
        print("Base:       " , self.Bname,'  ',bname_text     )
        print("Attr:       " , self.Aname,'   (',hex(self.location_of_attribute),')' )
        print("Offset:     " , self.Offset          )
        print("Field.loc:  " , field_loc );
        print("Length:     " , self.Length          )

        print("AttrType:   " , self.AttrType ,end="")
        print(" (",hex(self.AttrType),") ", sep='', end="")
        if self.AttrType == 0 :
            print(" (Unknown)", end="")
        elif self.AttrType == COB_TYPE_UNKNOWN             :
            print("UNKNOWN            ", end="")
        elif self.AttrType == COB_TYPE_GROUP               :
            print("GROUP              ", end="")
        elif self.AttrType == COB_TYPE_BOOLEAN             :
            print("BOOLEAN            ", end="")
        elif self.AttrType == COB_TYPE_NUMERIC_DISPLAY     :
            print("NUMERIC_DISPLAY    ", end="")
        elif self.AttrType == COB_TYPE_NUMERIC_BINARY      :
            print("NUMERIC_BINARY     ", end="")
        elif self.AttrType == COB_TYPE_NUMERIC_PACKED      :
            print("NUMERIC_PACKED     ", end="")
        elif self.AttrType == COB_TYPE_NUMERIC_FLOAT       :
            print("NUMERIC_FLOAT      ", end="")
        elif self.AttrType == COB_TYPE_NUMERIC_DOUBLE      :
            print("NUMERIC_DOUBLE     ", end="")
        elif self.AttrType == COB_TYPE_NUMERIC_L_DOUBLE    :
            print("NUMERIC_L_DOUBLE   ", end="")
        elif self.AttrType == COB_TYPE_NUMERIC_FP_DEC64    :
            print("NUMERIC_FP_DEC64   ", end="")
        elif self.AttrType == COB_TYPE_NUMERIC_FP_DEC128   :
            print("NUMERIC_FP_DEC128  ", end="")
        elif self.AttrType == COB_TYPE_NUMERIC_FP_BIN32    :
            print("NUMERIC_FP_BIN32   ", end="")
        elif self.AttrType == COB_TYPE_NUMERIC_FP_BIN64    :
            print("NUMERIC_FP_BIN64   ", end="")
        elif self.AttrType == COB_TYPE_NUMERIC_FP_BIN128   :
            print("NUMERIC_FP_BIN128  ", end="")
        elif self.AttrType == COB_TYPE_NUMERIC_COMP5       :
            print("NUMERIC_COMP5      ", end="")
        elif self.AttrType == COB_TYPE_ALNUM               :
            print("ALNUM              ", end="")
        elif self.AttrType == COB_TYPE_ALPHANUMERIC        :
            print("ALPHANUMERIC       ", end="")
        elif self.AttrType == COB_TYPE_ALPHANUMERIC_ALL    :
            print("ALPHANUMERIC_ALL   ", end="")
        elif self.AttrType == COB_TYPE_ALPHANUMERIC_EDITED :
            print("ALPHANUMERIC_EDITED", end="")
        elif self.AttrType == COB_TYPE_NUMERIC_EDITED      :
            print("NUMERIC_EDITED     ", end="")
        elif self.AttrType == COB_TYPE_NATIONAL            :
            print("NATIONAL           ", end="")
        elif self.AttrType == COB_TYPE_NATIONAL_EDITED     :
            print("NATIONAL_EDITED    ", end="")
        print()

        print("AttrDigits: " , self.AttrDigits      )
        print("AttrScale:  " , self.AttrScale       )

        print("AttrFlags:  " , hex(self.AttrFlags)  , end = '')
        if (self.AttrFlags & COB_FLAG_HAVE_SIGN     ) != 0 :
            print(" HAVE_SIGN", end = '')
        if (self.AttrFlags & COB_FLAG_SIGN_SEPARATE ) != 0 :
            print(" SIGN_SEPARATE", end = '')
        if (self.AttrFlags & COB_FLAG_SIGN_LEADING  ) != 0 :
            print(" SIGN_LEADING", end = '')
        if (self.AttrFlags & COB_FLAG_BLANK_ZERO    ) != 0 :
            print(" BLANK_ZERO", end = '')
        if (self.AttrFlags & COB_FLAG_JUSTIFIED     ) != 0 :
            print(" FLAG_JUSTIFIED", end = '')
        if (self.AttrFlags & COB_FLAG_BINARY_SWAP   ) != 0 :
            print(" BINARY_SWAP", end = '')
        if (self.AttrFlags & COB_FLAG_REAL_BINARY   ) != 0 :
            print(" REAL_BINARY", end = '')
        if (self.AttrFlags & COB_FLAG_IS_POINTER    ) != 0 :
            print(" IS_POINTER", end = '')
        if (self.AttrFlags & COB_FLAG_NO_SIGN_NIBBLE) != 0 :
            print(" NO_SIGN_NIBBLE", end = '')
        if (self.AttrFlags & COB_FLAG_IS_FP         ) != 0 :
            print(" IS_FP", end = '')
        if (self.AttrFlags & COB_FLAG_REAL_SIGN     ) != 0 :
            print(" REAL_SIGN", end = '')
        if (self.AttrFlags & COB_FLAG_BINARY_TRUNC  ) != 0 :
            print(" BINARY_TRUNC", end = '')
        if (self.AttrFlags & COB_FLAG_CONSTANT      ) != 0 :
            print(" CONSTANT ", end = '')
        print()

        print("data: (HEX)   [" , end= " ")

        ## At this point, if BINARY_SWAP is on, our data are reversed.
        ## We want to unreverse them for this display:
        output_bytes = self.data
        if (self.AttrFlags & COB_FLAG_BINARY_SWAP) != 0 :
            output_bytes.reverse()

        for byte in output_bytes :
            hexout = hex(byte)[-2:]
            if hexout[0:1] == 'x' :
                hexout = '0' + hexout[-1:]
            print(hexout,end=" ")
        print("]")
        print("data: (ASCII) \"", end = "")
        for byte in output_bytes :
            if 32 <= byte < 128 :   # valid ASCII characters
                print(chr(byte),end="")
            else :
                print('.',end="")
        print('"')

class TabFileInformation() :
    def __init__(self) :
        self.var_trie = VarTrie()
        self.old_dogs = set()
        self.line_list = LineList()
        self.current_variable_string = ""
        self.current_trapped_routine = ""
        self.current_trapped_file = ""
        self.current_trapped_line = 0
        self.question_mark_range = 6
        self.current_full_path = ""

    def ReadVariableString(self, string_name) :
        retval = False

        # We are going to use the x/ns "memory as string" command, which reads
        # in n blocks of some number of characters.  On gdb 8.3, each block seems
        # to be 201 characters. I can't know if that's universal, so I am going
        # to make this code adaptive

        # We are going to read a single line of the string, in order to figure
        # how how many characters per line we're going to get:

        the_address = GetAddressOf(string_name)
        if the_address :
            command = "x/1s " + str(the_address)
            try :
                the_data = gdb.execute(command,False,True)
                index1 = the_data.find('"')
                if index1 != -1 :
                    index2 = the_data.find('"',index1+1)
                    if( index2 != -1 ) :
                        total_length = int(the_data[index1+1:index1+1+9])
                        chars_each_line = index2-(index1+1);

                        # We know the total length, and we know the number of
                        # chars we will get in each block.  So, we can calculate
                        # the number of blocks we need to ask for, and build
                        # that command:
                        blocks = (total_length + chars_each_line-1) // chars_each_line
                        command = "x/" + str(blocks) + "s " + str(the_address)

                        # And we can get that data
                        the_data = gdb.execute(command,False,True)

                        # And now we loop through the data, extracting each chunk
                        is_first_line = True
                        index2 = 0
                        lines = []
                        while True :
                            index1 = the_data.find('"',index2+1)
                            if index1 == -1 :
                                break;
                            index2 = the_data.find('"',index1+1)
                            if index2 == -1 :
                                break;
                            if is_first_line :
                                index1 += 9
                                is_first_line = False
                            lines.append(the_data[index1+1:index2])

                        # and now that we have all the chunks, we can assemble
                        # them into the VARIABLE_STRING_xxx
                        self.current_variable_string = "".join(lines)
                        retval = True
            except :
                pass
        return retval

    def ProcessInfoStack(self, info_stack) :
        """ Figures out which routine we are currently trapped in"""

        # What we need to know is on the very first line:
        m = info_stack.find('\n')
        if m > -1 :
            info_stack = info_stack[:m]
        tokens = info_stack.split(' ')

        # That line looks like this:
        #
        # #0  optfde01_ (entry=0) at optfde01.cbl:59
        #
        # We need the information from the final token:

        file_and_line = tokens[-1].split(':')
        self.current_trapped_line = int(file_and_line[1])

        try :
            if file_and_line[0] != self.current_trapped_file :
                # We are in a new file
                self.current_trapped_file = file_and_line[0]

                ## Because it's possible that we are trapped in a copybook,
                ## we can't assume that VARIABLE_STRING has the name
                ## of the current_trapped_file.  We need to work our
                ## way through the stack:

                # Find the address of the VARIABLE_STRING for this trap point

                backtrace = gdb.execute("bt",False,True).split('\n')
                for possibility in backtrace :
                    # Pull out the file name, which is before the colon in the last token
                    possible_file = possibility.split(' ')[-1].split(':')[0]
                    name_root = possible_file
                    name_root = name_root.split('/')
                    name_root = name_root[-1]
                    name_root = name_root.replace('.','_')
                    string_name = "VARIABLE_STRING_" + name_root.upper()

                    if self.ReadVariableString(string_name) :
                        break

                # We have us a new variable string
                # Clear out the tables:
                # Load the new tables
                self.var_trie = VarTrie()
                self.line_list = LineList()

                # Pull apart the variable_string
                variable_lines = self.current_variable_string.split("~")
                for variable_line in variable_lines :
                    self.line_list.LoadFromLine(variable_line)
                    self.var_trie.LoadFromLine(variable_line)

                # Put a fencepost at the end of the line_list
                self.line_list.Insert(self.line_list.EndOfTheWorld,"Ragnarok")

                # Get the full path for the source file
                info_source = gdb.execute("info source",False,True)
                info_source = info_source.split('\n')
                for line in info_source :
                    pattern = re.compile(r"Located in (.*)$")
                    match = re.search(pattern,line)
                    if match :
                        self.current_full_path = match.group(1)
                        break
        except :
            traceback.print_exc()

    def ReadSymbolTableText(self) :

        # Figure out where we are
        try :
            original_info_stack = gdb.execute("info stack",False,True)
            self.ProcessInfoStack(original_info_stack)

            # And now figure out what routine we are trapped in, from
            # the line number
            for line in self.line_list.line_list :
               if line[0] > self.current_trapped_line :
                   # We've gotten past self.current_trapped_line, so the prior entry
                   # was the one we wanted
                   break;
               # Keep looking
               self.current_trapped_routine = line[1]

        except :
            #traceback.print_exc()
            print( "No stack.")

def MachineInterfaceFlat(TabFile,possibilities) :
    # This is the machine interface mode for returning the
    # the variables in a flat manner.
    # Our mission is to emulate
    # '-stack-list-variables --simple-values'
    # e.g.,    variables=[{name="b_70",arg="1",type="unsigned char *",value="0x555555759df0 <b_39> \"James     K Lowden    \""}]
    s = "variables=["
    for payload_index in possibilities :
        payload = TabFile.var_trie.storage_list[payload_index]
        payload.FetchVariableData(TabFile.line_list , False)
        if not payload.not_in_context :
            name = ""
            name += payload.display_name;
            n = name.find(' ')
            if n > -1 :
                name = name[:n] + '-' + name[n+1:]
            n = name.find(' ')
            if n > -1 :
                name = name[:n]

            # Everything after the final slash is the program
            # name, which is redundant and takes up valuable
            # screen real estate.  Clear it out.

            n = name.rfind('/')
            if n > -1 :
                name = name[:n]

            body = MachineInterfaceBody(payload.display_body)

            s += '{name="'
            s += name
            s += '",type="char *",value="'
            s += body
            s += '"},'
    if s[-1] == ',' :
        s = s[:-1]  # Strip off final comma
    s += ']'
    s = NoNulls(s)
    return s

class HNode() :
    def __init__(self) :
        self.storage_table_index = -1
        self.Level = 0
        self.BaseName = "" # Just A of A/B/C
        self.display_body = ""
        self.parent = None
        self.children = []

def TrimComma(s) :
    if s and s[-1] == ',' :
        s = s[:-1]  # Strip off final comma
    return s

def BuildHString(node) :
    s = ""
    if node.Level == 0 :
        # This is the root.  We live only for our children
        for child in node.children :
            s += BuildHString(child)
    else :
        if node.Level in (1,77,78) :
            ## Top level variables are treated this way:
            s += r'{name="'
            s += node.BaseName #  + "/" + str(node.storage_table_index)
            s += r'",value="'
            if len(node.children) == 0 :
                ## There are no children, which means this is just
                ## a simple variable:
                s += r'\"'
                s += MachineInterfaceBody(node.display_body)
                s += r'\"'
                s += r'"'
            else :
                ## This is a top-level node with children:
                s += r'{_groupvalue=\"'
                s += MachineInterfaceBody(node.display_body)
                s += r'\",'
                for child in node.children :
                    s += BuildHString(child)
                s=TrimComma(s)
                s += r'}"'
            s += '},'
        else :
            # This is a node whose level is 01-49, 66
            if len(node.children) == 0 :
                s += node.BaseName # + "/" + str(node.storage_table_index)
                s += r'=\"'
                s += MachineInterfaceBody(node.display_body)
                s += r'\",'
            else :
                s += node.BaseName # + "/" + str(node.storage_table_index)
                s += r'={_groupvalue=\"'
                s += MachineInterfaceBody(node.display_body)
                s += r'\",'
                for child in node.children :
                    s += BuildHString(child)
                s=TrimComma(s)
                s += r'},'
    return s

def MachineInterfaceHeirarchical(TabFile,possibilities) :
    # This is the machine interface mode for returning the
    # the variables in a flat manner.
    # Our mission is to emulate
    # '-stack-list-variables --all-values'
    # e.g.,
    # variables=[{name="L00",value="\"Standalone\""},{name="L01",value="{fullname = \"RobertDubner\", L02 = {firstname = \"Robert\", lastname = \"Dubner\"}}"},{name="L01_2",value="{fullname = \"JudyRuderman\", L02 = {firstname = \"Judy\", lastname = \"Ruderman\"}}"}]

    # We will use a stack to build a tree from the ordered variables
    root = HNode()
    stack = [root]

    retval = "variables=["
    for payload_index in possibilities :
        payload = TabFile.var_trie.storage_list[payload_index]
        payload.FetchVariableData(TabFile.line_list , False)
        if not payload.not_in_context :
            # print("MIH", payload.Level, payload.Name, payload.display_body)
            # First, if the stack has nephews of ours,
            # let's get them off of the stack:
            while stack[-1].Level > payload.Level :
                stack.pop()

            # Create the new node
            new_node = HNode()
            new_node.storage_table_index = payload_index
            new_node.Level = payload.Level
            split_name = payload.Name.split('/')
            if len(split_name) == 2 :
                # If it's just the two pieces, then use the whole name/program
                # pair.  (This is a file record or 01/77, and we might need
                # the whole name to accurately handle a "print var=x request")
                new_node.BaseName = payload.Name
            else :
                # Just use the lowest level.  When necessary, the VSC code will
                # be able to walk up through the containers to get the full name
                new_node.BaseName = split_name[0]

            new_node.display_body = payload.display_body

            if payload.Level in (77,78) :
                stack = [root]
            if stack[-1].Level == payload.Level or payload.Level == 66 :
                # The element on the stack is our brother, so
                # so we are going to share parents:
                new_node.parent = stack[-1].parent
            else : # stack[-1].Level < payload.Level
                # The element on the stack is our parent
                new_node.parent = stack[-1]
            # And now we become the final stack entry
            if payload.Level != 66 : # 66 is just a pain in the patootie.
                stack.append(new_node)
            # And let's let our parent know they have another child
            new_node.parent.children.append(new_node)

    # We now have a tree that can be used to create the variables string
    retval += BuildHString(root)

    retval = TrimComma(retval)
    retval += ']'
    retval = NoNulls(retval)
    #retval = r'variables=[{name="L01",value="{_groupvalue=\"Snap!\",fullname=\"RobertDubner\",L02={_groupvalue=\"Crackle!\",firstname = \"Robert\", lastname = \"Dubner\",L03={_groupvalue=\"Pop!\",firstname = \"Judy\", lastname = \"Ruderman\"}}}"}]'
    return retval


def MetaPossibilities(args,TabFile,machine_interface_mode,rside) :
    possibilities_1 = []
    from_digit = False
    if len(args) >= 1 :
        if args[0].find('*') != -1 :
            # If the first, presumably single, argument contains an asterisk, then
            # he is asking for a list of variables and values:
            possibilities_1 = TabFile.var_trie.GetAllPossibilities()
        elif args[0].find('?') != -1 :
            # If the first, presumably single, argument contains a question mark,
            # then we need to round up the usual suspects.

            # But first, we are going to keep a rogue's gallery of the current list
            # suspects.  The idea here is that sometimes MOVE A TO B is the last line
            # of a PARAGRAPH, so as soon as it is executed control jumps elsewhere and
            # B falls out of context.  Or perhaps the next GDB command is a CONTINUE,
            # rather than NEXT.  By keeping one stage of stickiness, the display of variables
            # will be a bit more like what the user probably expects.

            if len(TabFile.current_full_path) > 0 :
                qmrange = TabFile.question_mark_range;
                if qmrange < 0 :
                    qmrange = 1000000000
                return_string = subprocess.check_output(["cobcd-rw", \
                                                          TabFile.current_full_path, \
                                                          str(TabFile.current_trapped_line), \
                                                          str(qmrange) ])
                tokens = return_string.split()
                new_dogs = set()
                for token in tokens :
                    token = token.decode("utf-8")
                    new_dogs = new_dogs.union(set(TabFile.var_trie.GetListOfExactPossibilities(token)))
                possibilities_1 = list(TabFile.old_dogs.union(new_dogs))
                TabFile.old_dogs = new_dogs
        elif args[0].isdigit() :
            from_digit = True
            # if the first, presumably single, argument is a number, then he is looking
            # for a short-term memory entry.

            # Pick up the 1-based value indexed by the argument:
            index = int(args[0])-1
            if index >= 0 and index < len(GV_GlobalVariables.ShortTermMemory) :
                # If index is valid, pick up the short-term memory entry
                possibilities_1 = [GV_GlobalVariables.ShortTermMemory[index]]
        else :
            # There are one or more arguments.  Use our clever routine
            # to come up with the possibilities_1:
            possibilities_1 = TabFile.var_trie.GetListOfPossibilities(args)

    # We now have a list of possibilities_1 based on the names of the variables
    # typically, some of them will not be in context.  We wish, at this point
    # to eliminate such variables.  The bad news is that this means we
    # have to fetch all the variable data twice.  But, heck.

    possibilities = []
    for payload_index in possibilities_1 :
        payload = TabFile.var_trie.storage_list[payload_index]
        payload.FetchVariableData(TabFile.line_list , False)
        if not payload.not_in_context :
            possibilities.append(payload_index)

    # What we do now depends on how many possibilities there are
    possibilities.sort()

    if len(possibilities) == 1 and machine_interface_mode and not rside and args[0] != '*' and args[0] != '?' :
        # Very special case.  This was a `p/m something`, which is what happens when
        # the -data-evaluate-expression command is issued by Visual Studio Code.  I considered creating
        # a unique 'print/e ' command, but then I realized that VSC never issues a 'p/m something' unless there
        # is also an 'rside'.  So, this code is in response to a `-data-evalute-expression` equivalent command
        payload_index = possibilities[0]
        payload = TabFile.var_trie.storage_list[payload_index]
        payload.FetchVariableData(TabFile.line_list , False)
        s = 'value="' + MachineInterfaceBodyAlt(payload.display_body) + '"'
        print(s)
        possibilities = []
        return possibilities

    if len(possibilities) > 1 or machine_interface_mode :
        # In machine_interface mode, we just return the possibility(ies).  In human
        # mode, if there is more than one, we create a numbered list; the user can later
        # specify a variable by the number
        if machine_interface_mode :
            # Because we are going to be showing the data in a heirarchical way,
            # we want to be sure every sub-level of a 01 Level variable is visible:
            ppp = set(possibilities);
            for i in possibilities :
                tokens = TabFile.var_trie.storage_list[i].Name.split('/')
                var  = tokens[-2]
                prog = tokens[-1]
                u = i-1;
                while u >= 0 :
                    tokens = TabFile.var_trie.storage_list[u].Name.split('/')
                    if tokens[-2] != var or tokens[-1] != prog :
                        break;
                    ppp.add(u)
                    u -= 1 ;
                d = i+1;
                while d < len(TabFile.var_trie.storage_list) :
                    tokens = TabFile.var_trie.storage_list[d].Name.split('/')
                    if tokens[-2] != var or tokens[-1] != prog :
                        break;
                    ppp.add(d)
                    d += 1 ;

            possibilities = list(ppp)
            possibilities.sort()

            #s = MachineInterfaceFlat(TabFile,possibilities)
            s = MachineInterfaceHeirarchical(TabFile,possibilities)
            print(s)

        else :
            GV_GlobalVariables.ShortTermMemory = []

            # This is human-interactive mode
            # and show them to our user
            line_counter = 1
            for payload_index in possibilities :
                payload = TabFile.var_trie.storage_list[payload_index]
                payload.FetchVariableData(TabFile.line_list , False)
                if not payload.not_in_context :
                    GV_GlobalVariables.ShortTermMemory.append(payload_index)
                    s = "{0:2} : ".format(line_counter)
                    line_counter += 1
                    s += payload.display_name
                    s += " : "
                    s += payload.display_body
                    retval = OurPrint(NoNulls(s))
                    if retval == "Quit" :
                        break;
    if len(possibilities) == 1 and not from_digit :
        GV_GlobalVariables.ShortTermMemory = [possibilities[0]]

    return possibilities

# Note on the program structure:
#
# The Python extension to gdb is plenty powerful, but it is infuriating
# in one regard: Debugging.  I've gotten used to source level debugging, and
# the irony of having to do print-level debugging in an extension to a source-
# level debugger is delicious, but mainly in the sense that some people like
# the taste of beetles.  So, I have jumped through some hoops so that I could
# do at least *some* debugging in Visual Studio even though the gdb object
# isn't available.  Bob Dubner, 2019-07-23

class CPrintWorker() :
    """Does most of the non-gdb work of CPrint()"""

    # By creating this worker, we can use a Python debugging package to
    # handle everything that doesn't actually need the gdb object
    def __init__(self):
        self.current_source_file = ""
        self.sticky_display_mode = os.environ.get('CPRINT_V', '0')
        self.question_mark_range = int(os.environ.get('CPRINT_R', '6'))
        self.use_cprint = int(os.environ.get('CPRINT', '0'))
        self.machine_interface_mode = False

    def ProcessDisplayMode(self,args,TabFile) :
        self.machine_interface_mode = False
        # Make sure the current display mode is the same as the session mode
        self.current_display_mode = self.sticky_display_mode

        # Now see if he wants to change it
        if len(args) >= 1 :
            if args[0] == '/h' :
                print("COBOL print command usage:")
                print("'print/h' prints this message.")
                print("'print <arg>' shows <arg> using session-default formatting")
                print("'print/v<n> <arg>' displays <arg> with a one-time formatting override")
                print("'print/v0' : name and variable on one line (this is the default)")
                print("'print/v1' : just the variable")
                print("'print/v2' : two-line display with name, then variable")
                print("'print/d   : expanded information")
                print("'print/v<n>' with no arguments sets the session formatting")
                print("'print/m'  : generate output compatible with '-stack-list-variables --simple-values'")
                print("'print *'  : print all variables that are currently in context")
                print("'print ?'  : print all variables that are currently in context and within R lines of the breakpoint")
                print("'print/r N': set the range for 'print ?' to plus/minus R (the default is 6; -1 means 'all')")
                print("The default v-level can be set with the environment variable CPRINT_V=n")
                print("The default r-level can be set with the environment variable CPRINT_R=n")
                print("The 'print' command can be changed to 'cprint' with the environment variable CPRINT=1")
                args = [""]
                return args

            # if the first argument is /d, turn on our debug flag:
            if args[0] == '/d' :
                self.current_display_mode = 'd'
                args.pop(0)
            elif args[0] == '/m' :
                self.machine_interface_mode = True
                args.pop(0)
            elif args[0] == '/v' :
                self.current_display_mode = '0'
                args.pop(0)
            elif len(args[0]) >= 3 and args[0][0:2] == '/v' :
                # This is a display v-code:
                v_code = args[0][2]
                args.pop(0)
                if v_code in "012d" :
                    # It's a valid display_mode
                    self.current_display_mode = v_code
                    # if len(args) is down to zero, then any /v code established here
                    # becomes the new session default.  Otherwise, it'll apply only once
                    if len(args) == 0 :
                        self.sticky_display_mode = self.current_display_mode
            elif len(args) == 2 and args[0] == '/r' and args[1].lstrip('-').isnumeric()  :
                # This is a request to change the question_mark_range
                self.question_mark_range = int(args[1])
                print("The 'p ?' range has been set to +/-", self.question_mark_range, "lines")
                TabFile.old_dogs = set()
                args = ['?']
            elif len(args[0]) > 2 and args[0][0:2] == '/r'  :
                # This is a request to change the question_mark_range
                range = args[0][2:]
                if range.lstrip('-').isnumeric() :
                    self.question_mark_range = int(range)
                if int(self.question_mark_range) < 0 :
                    print("The 'p ?' range has been set to the same as 'p *'")
                else :
                    print("The 'p ?' range has been set to", self.question_mark_range)
                args = ['?']
            elif len(args) == 1 and args[0] == '/r' :
                if int(self.question_mark_range) < 0 :
                    print("The 'p ?' range is currently the same as 'p *'")
                else :
                    print("The 'p ?' range is currently +/-", self.question_mark_range,"lines")
                args = []

        return args

    def FormattedDisplay(self,payload ) :
        # see the comments in ProcessDisplayMode()
        if self.current_display_mode == '0' :
            print(payload.display_name," : ",payload.display_body)
            pass
        elif self.current_display_mode ==  '1' :
            print(payload.display_body)
            pass
        elif self.current_display_mode ==  '2' :
            print(payload.display_name)
            print(payload.display_body)
            pass
        else : # self.current_display_mode ==  'd' :
            payload.ShowFields()
            pass

    def ProcessArguments(self, TabFile, arguments) :
        # The command is typically "print A B C" or "print/d A B C"
        # There is also the possibility of an '=' character, whereby the
        # value after the '=' is assigned to the variable described to the left

        preargs = arguments.split('=',1)

        rside = None
        if len(preargs) > 1  :
            rside = preargs[1]

        # The variable name side is to the left of any '=' character
        args = []
        argsargs = preargs[0].split()
        for arg in argsargs :
            args.append(arg)

        self.question_mark_range = TabFile.question_mark_range
        args = self.ProcessDisplayMode(args,TabFile)
        TabFile.question_mark_range = self.question_mark_range

        possibilities = MetaPossibilities(args,TabFile,self.machine_interface_mode,rside)
        if len(possibilities) == 1 :
            # There is but one possibility
            # Pick up the payload from the list
            payload = TabFile.var_trie.storage_list[possibilities[0]]

            if rside :
                payload.PutDataAsString(rside)

                if self.machine_interface_mode :
                    #body = MachineInterfaceBody(payload.display_body)

                    s = payload.display_name + " set to " + payload.display_body
                    print(s)

            if not self.machine_interface_mode :
                # Human-interface:
                # Fetch the data again, just in case he executed some instructions
                # since short-term-memory was established
                payload.FetchVariableData(TabFile.line_list , True)
                # And display the variable according to the current list v-level
                self.FormattedDisplay(payload)

class CWatchWorker() :
    """Does most of the non-gdb work of CWatch()"""

    def __init__(self):
        self.current_display_mode = "0"

    def SetTheWatchpoint(self, TabFile, payload ) :
        command = "watch *(char(*)"
        command += '[' + str(payload.Length) +'])'
        command += '(' + hex(payload.location_of_data) + ')'

        #print("The command is:",command)
        watch_text = gdb.execute(command,False,True)
        print(watch_text)

    def ProcessWArgumentsW(self, TabFile, arguments) :
        # The command is typically "cw A

        args = arguments.split()

        possibilities = MetaPossibilities(args,TabFile, False, False)

        if len(possibilities) == 1 :
            payload_number = possibilities[0]
            #print("Watch Payload number is ", payload_number)

            # Pick up the payload from the list
            payload = TabFile.var_trie.storage_list[payload_number]
            payload.FetchVariableData(TabFile.line_list , False)

            # And go set the Watchpoint
            self.SetTheWatchpoint(TabFile, payload)

class CPrint (gdb.Command) :
    """Prints COBOL-NAMED variables when debugging cobst processed object files."""

    def __init__ (self):
        ###########################################################################
        ## The version number in the next line is updated by a Linux bash script. #
        ## The text "Version x.x" has to appear in square brackets.               #
        ## Don't mess with it!                                                    #
        ###########################################################################
        self.Worker = CPrintWorker()
        self.TabFile = TabFileInformation();
        self.TabFile.question_mark_range = self.Worker.question_mark_range

        actual_command = "print"
        if self.Worker.use_cprint == 1 :
            actual_command = "cprint"
        print('registering CPrint (Usage is "' + actual_command + '" <COBOL identifier>") [Version 4.1]')
        super (CPrint, self).__init__ (actual_command, gdb.COMMAND_USER)

    def invoke (self, arguments, from_tty):
        GV_GlobalVariables.LoadLibcob()
        # pick up the name of the current source file
        try :
            # Check if this is a special case.  (Enough of these will
            # mean putting this in a separate function.)
            tokens = arguments.split()
            if len(tokens) == 3 and tokens[0] == "set" and tokens[1] == "repeat" and tokens[2].isdigit() :
                GV_GlobalVariables.repeat_count = int(tokens[2])
                if GV_GlobalVariables.repeat_count < 1 :
                    GV_GlobalVariables.repeat_count = 1000000
                return

            self.TabFile.ReadSymbolTableText()
            self.Worker.ProcessArguments(self.TabFile,arguments)
        except :
            traceback.print_exc()

class CWatch (gdb.Command) :
    """Used for creating COBOL-NAMED watchpoints when debugging cobst processed object files."""

    def __init__ (self):
        super (CWatch, self).__init__ ("cwatch", gdb.COMMAND_USER)
        print('registering CWatch (Usage is "cwatch <COBOL identifier>")')
        self.Worker = CWatchWorker()
        self.TabFile = TabFileInformation();

    def invoke (self, arguments, from_tty):
        GV_GlobalVariables.LoadLibcob()
        try :
            self.TabFile.ReadSymbolTableText()
            self.Worker.ProcessWArgumentsW(self.TabFile,arguments)
        except :
            traceback.print_exc()


CPrint()
CWatch()
