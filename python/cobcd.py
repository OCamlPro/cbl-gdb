#!/usr/bin/python3

################################################################################
# Copyright (c) 2019-2021 COBOLworx and contributors
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
# pylint: disable=line-too-long,unused-argument,unused-wildcard-import,import-error
#
# flake8 settings used
# [flake8]
# ignore = E402
# max-line-length = 160
################################################################################

#   This module implements the gdb extension commands cprint and cwatch

# Catalog of environment variables interpreted by this module:
#
# os.environ.get('CPRINT_D','0')   When non-zero, causes verbose output
# os.environ.get('CPRINT_V', '0')  Default formatting p/v N value
# os.environ.get('CPRINT_R', '6')) Default p/? range
# os.environ.get('CPRINT', '<cmd>'))   When non-blank, registers "cmd" instead of "cprint" as the command

from __future__ import print_function

import re
import sys
import platform
import os
import traceback
import subprocess
from inspect import currentframe, getframeinfo
from struct import calcsize
import gdb

# Version to be printed upon request - Don't mess with it!
# It is updated by a Linux bash script, the text "Version x.x" has to appear in square brackets.
COBCD_VERSION = '[Version 4.23-dev]'

#   This module implements the gdb extension commands like cprint and cwatch
#   It is executed by GDB.
#
#   Optionally it is read by the COBST program and incorporated into a
#   C module that gets compiled and linked into the final executable.
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

# Buffer size for printing
PRINT_BUFFER_SIZE = 65536

# These constants are used when calling get_verified_field_location
ALLOCATE = True
DONT_ALLOCATE = False

def GdbExecute(curframe, command):
    # It is up to the caller to handle exceptions
    limiting_debug_level = 1
    if GV_GlobalVariables.debug_level >= limiting_debug_level:
        print("#### ", getframeinfo(curframe).lineno, ": gdb.execute: '", command, "'", sep='')
    retval = gdb.execute(command, False, True)
    if GV_GlobalVariables.debug_level >= limiting_debug_level:
        lines = retval.split('\n')
        for line in lines:
            if len(line) > 0:
                print("####       retval:", line)
    return retval

def GdbExecute2(curframe, command, debug_level):
    # It is up to the caller to handle exceptions
    limiting_debug_level = 1
    if debug_level >= limiting_debug_level:
        print("#### ", getframeinfo(curframe).lineno, ": gdb.execute: '", command, "'", sep='')
    retval = gdb.execute(command, False, True)
    if debug_level >= limiting_debug_level:
        lines = retval.split('\n')
        for line in lines:
            if len(line) > 0:
                print("####       retval:", line)
    return retval

class GlobalVariables():
    def __init__(self):
        # Establish the debugging verbosity level:
        self.debug_level = int(os.environ.get('CPRINT_D', '0'))

        # Let's figure out some stuff about the environment
        self.SizeofPointer = calcsize('P')  # Returns 4 or 8
        self.byteorder = sys.byteorder  # Returns "big" or "little"

        if self.debug_level >= 1:
            print("#### SizeofPointer", self.SizeofPointer)
            print("#### byteorder", self.byteorder)

        self.ShortTermMemory = []
        self.repeat_count = 11  # Default value for "'0' <repeats 11 times>" logic

        self.cprint_buffer = None
        self.cprint_f0 = None

    def AllocateBuffers(self):
        if not self.cprint_buffer:
            try:
                command = "call (char*)cob_malloc({0})".format(PRINT_BUFFER_SIZE)
                self.cprint_buffer = int(gdb.execute(command, False, True).split()[2],16)

                command = "call (char*)cob_malloc(sizeof(cob_field))"
                self.cprint_f0 = int(gdb.execute(command, False, True).split()[2],16)
            except:
                traceback.print_exc()
                self.cprint_buffer = None
                self.cprint_f0 = None
                pass
            if not self.cprint_buffer or not self.cprint_buffer:
                print("Something is very wrong in GlobalVariables.AllocateBuffers()")
                os.exit(1)

    def DestroyBuffers(self):
        self.cprint_buffer = None
        self.cprint_f0 = None

GV_GlobalVariables = GlobalVariables()

def NoNulls(s):
    ss = ""
    for ch in s:
        if ord(ch) < 32 or ord(ch) >= 128:
            ch = '.'
        ss += ch
    return ss

def OurPrint(s):
    # Use this routine when GDB might halt with a
    #    --Type <RET> for more, q to quit, c to continue without paging--

    # The following code catches the 'q' Quit option
    retval = None
    try:
        print(s)
    except KeyboardInterrupt as ki:
        print("Quit")
        retval = str(ki)
    except:
        traceback.print_exc()
    return retval

def LeftRight(s, splitchar):
    """Splits what is presumably a numeric string with a possible
    splitting character into left and right pieces"""
    left = ""
    right = ""
    i = 0
    while i < len(s):
        ch = s[i]
        i += 1
        if ch == splitchar:
            break
        left += ch
    while i < len(s):
        ch = s[i]
        i += 1
        right += ch
    return left, right

def Picture(s):
    """ Expands a 9(3)V9(2) to 999V999"""
    r = ""
    lch = ''
    ch = ''
    i = 0
    ncount = 0
    while i < len(s):
        ch = s[i].upper()
        i += 1
        if ch == '(':
            ncount = 0
            while i < len(s):
                ch = s[i]
                i += 1
                if ch == ')':
                    break
                ncount *= 10
                ncount += ord(ch) - ord('0')
            ncount -= 1
            while ncount > 0:
                ncount -= 1
                r += lch
        else:
            lch = ch
            r += ch
    # To avoid certain types of twitching on the part of your poor beleagured
    # programmer, replace the digrams 'CR' and 'DB' with 'c' and 'd'.  Note
    # that CR or DB, if present, are the final characters of the string.

    n = r.find("DB")
    if n > -1:
        r = r[:n] + 'd'
    n = r.find("CR")
    if n > -1:
        r = r[:n] + 'c'

    return r

def MachineInterfaceBody(s):

#  This sequence:
#  variables=[{name="L00L",value="{_groupvalue=\"Sn\\\"ap!\",fullname=\"Robert\\\"Dubner\",L02={_groupvalue=\"Crackle!\",firstname = \"Robert\", lastname = \"\\\"Dubner\\\"\",L03={_groupvalue=\"Pop!\",firstname = \"Judy\", lastname = \"Ruderman\"}}}"}]
#
#  Results in a heirarchical display:
#
#  L00L: Sn"ap!
#     fullname : Robert"Dubner
#        L02: Crackle!
#            firstname: Robert
#            lastname: "Dubner"
#            L03: Pop!
#                firstname: Judy
#                lastname: Ruderman
#
#  This one:  variables=[{name="L01",value="===\">\\/<\"==="}]
#
#  Results in      ===">\/<"===
#
#  Counting up the backslashes is, to say the least, tricky

    # Replace double-quotes and backslashes
    retval = ""
    for ch in s:
        if ch == '"':
            retval += r'\\\"'
        elif ch == '\\':
            retval += r'\\\\'
        else:
            retval += ch

    return retval

def MachineInterfaceBodyAlt(s):

    # Replace double-quotes and backslashes
    retval = ""
    for ch in s:
        if ch == '"':
            retval += r'\"'
        elif ch == '\\':
            retval += r'\\'
        else:
            retval += ch

    return retval

def ReplaceWithRepeatCountQuoted(s):
    # We know this guy has a leading quote, and presumably a trailing one
    # print("ReplaceWithRepeatCountQuoted(1)", s)
    retval = ""
    # Strip off the leading and trailing quotes
    s = s[1:-1]
    rabbit = 0
    fox = 0
    count = 0
    prior_contraction = False
    in_a_quoted_run = False
    number_of_repeated_runs = 0
    while rabbit < len(s):
        # Arriving here means:
        # We're here for the first time, or we just finished a run
        # So, we are going to pick up 'char', which is a candidate
        # for the start of a new run

        fox = rabbit
        char = s[rabbit]
        rabbit += 1
        while rabbit < len(s) and s[rabbit] == char:
            # This is where we proceed in a run.  Swallow up characters
            # that are the same as our candidate:
            rabbit += 1

        # We have either run out of characters, or else
        # rabbit points to a character other than char
        if rabbit - fox >= GV_GlobalVariables.repeat_count:  # This is a magic number
            # At 29, the <repeats> clause will be shorter than the replaced text
            # At 11, the string will be longer.  But 11 matches what gdb does
            if in_a_quoted_run:
                # We have to end the current ongoing string
                # and start this contraction with a prelude
                retval += "\", "
            # and now we can start the contraction:
            retval += "\'"
            retval += char
            retval += "\' <repeats "
            retval += str(rabbit - fox)
            retval += ' times>, '
            prior_contraction = True
            in_a_quoted_run = False
            number_of_repeated_runs += 1
        else:
            # The run is short enough that we just put the characters
            # into place
            if not in_a_quoted_run:
                # We have to start the quote:
                retval += "\""
                in_a_quoted_run = True
            # Copy over characters until the fox catches up with the rabbit
            while fox < rabbit:
                fox += 1
                retval += char
            prior_contraction = False

    # We have reached the end
    if prior_contraction:
        # strip off the unnecessary ', '
        retval = retval[:-2]
    if in_a_quoted_run:
        retval += "\""

    # print("ReplaceWithRepeatCountQuoted(2)", retval)
    return retval

def ReplaceWithRepeatCountBracketed(s):
    if (len(s) % 2) == 1:
        # We don't know what to do with an odd-length string.  We just don't
        return s
    # We know this guy has a leading bracket, and presumably a trailing one
    # In between are a stream of undecorated hex characters.  So, this is
    # a lot like the quoted version, except we operate in pairs, rather than characters
    # print("ReplaceWithRepeatCountBracketed(1)", s)
    retval = "["
    # Strip off the leading and trailing brackets
    s = s[1:-1]
    rabbit = 0
    fox = 0
    count = 0
    prior_contraction = False
    in_a_quoted_run = False
    number_of_repeated_runs = 0
    while rabbit < len(s):
        # Arriving here means:
        # We're here for the first time, or we just finished a run
        # So, we are going to pick up 'char', which is a candidate
        # for the start of a new run

        fox = rabbit
        char = s[rabbit:rabbit + 2]
        rabbit += 2
        while rabbit < len(s) and s[rabbit:rabbit + 2] == char:
            # This is where we proceed in a run.  Swallow up characters
            # that are the same as our candidate:
            rabbit += 2

        # We have either run out of characters, or else
        # rabbit points to a character other than char
        if (rabbit - fox) // 2 >= GV_GlobalVariables.repeat_count:  # This is a magic number.  See the
                                                                    # comment in ReplaceWithRepeatCountQuoted()
            if in_a_quoted_run:
                # We have to end the current ongoing string
                # and start this contraction with a prelude
                retval += '", '
            # and now we can start the contraction:
            retval += char
            retval += " <repeats "
            retval += str((rabbit-fox) // 2)
            retval += ' times>, '
            prior_contraction = True
            in_a_quoted_run = False
            number_of_repeated_runs += 1
        else:
            # The run is short enough that we just put the characters
            # into place
            if not in_a_quoted_run:
                in_a_quoted_run = True
            # Copy over characters until the fox catches up with the rabbit
            while fox < rabbit:
                fox += 2
                retval += char
            prior_contraction = False

    # We have reached the end
    if prior_contraction:
        # strip off the unnecessary ', '
        retval = retval[:-2]
    retval += ']'

    # print("ReplaceWithRepeatCountBracketed(2)", retval)
    return retval

def ReplaceWithRepeatCount(s):
    try:
        if s and (s[0] == '"' or s[0] == '<'):
            return ReplaceWithRepeatCountQuoted(s)
        if s and s[0] == '[':
            return ReplaceWithRepeatCountBracketed(s)
        return s
    except:
        traceback.print_exc()
        sys.exit(1)

class VarTrieNode():
    def __init__(self):
        self.parent = None
        self.children = {}
        self.piece_of_name = ""
        self.count = 0
        self.payload_index = None

    def AddCanonicalName(self, name):
        pass

def BlankIsZero(v):
    if len(v) == 0:
        v = "0"
    return int(v)

def GetDataAt(address, bytes):
    # This routine takes an address and returns the
    # value there.  'bytes' can be 1, 2, 4, or 8
    try:
        if bytes == 1:
            u = 'b'
        elif bytes == 2:
            u = 'h'
        elif bytes == 4:
            u = 'w'
        elif bytes == 8:
            u = 'g'
        else:
            return None

        # When an integer is long enough, Python appends a 'L' to the hex
        # number.  This came up in SPARC, with addresses like 0xffffffff7b407700
        command = "x/1" + u + "u " + hex(address).replace('L', '')
        value_back = GdbExecute(currentframe(), command)

        nfound = value_back.find(":")
        if nfound == -1:
            return None
        value = int(value_back[nfound + 1:])
        return value

    except gdb.error:
        print("gdb .execute failed in GetDataAt:", command)
        return None

def GetBytesAt(address, N):
    # Pick up N bytes from address, return them as a list of ints
    retval = []
    command = "x/" + str(N) + "xb " + hex(address)
    all_the_bytes = GdbExecute(currentframe(), command)

    lines = all_the_bytes.split('\n')
    for line in lines:
        nfound = line.find(':')
        if nfound != -1:
            bytes = line[nfound + 1:].split()
            for byte in bytes:
                retval.append(int(byte, 16))
    return retval

def ExtractHexValueFrom(s):
    # Use this routine to extract an 0x123abc value from a string.
    # Needed because gdb returns such information in slightly different ways
    retval = None
    match = re.match(".*(0x[0-9a-fA-F]+).*", s)
    if match:
        retval = int(match.group(1), 16)
    return retval

def GetPointerAt(variable):
    # Use this form when you know variable is the name of a pointer.
    # For example, I wrote this when I needed to pick up cobglobptr.  I
    # didn't really care where cobglobptr was; I cared about the memory
    # cob_global block it pointed to.
    try:
        command = "x/1db (char *) " + variable
        result = GdbExecute(currentframe(), command)
        return ExtractHexValueFrom(result)
    except:
        return None

def GetAddressOf(variable):
    # Use this routine when you know variable is the name of a block.
    retval = None
    if variable:
        try:
            command = "info address " + variable
            address_string = GdbExecute(currentframe(), command)
            retval = ExtractHexValueFrom(address_string)
        except gdb.error:
            retval = None

        if not retval:
            try:
                command = "x/1db " + variable  # Notice d for decimal, not hex, because
                                               # a second 0x messes up ExtractHexValue
                address_string = GdbExecute(currentframe(), command)
                retval = ExtractHexValueFrom(address_string)
            except:
                retval = None

    return retval

def AddEmUp(expression):
    # Originally written when I thought only integers would appear
    # here, I have since learned that expressions can be more complex.
    # For now I am simply punting when when confronted with something
    # that isn't an integer or a simple addition of integers
    #
    # Processing of arrays can involve 123*b_37 + 456, and I've been warned
    # that there might even be parentheses involved.  We'll burn those bridges
    # in times to come.
    expression = expression or "0"
    retval = 0
    tokens = expression.split('+')
    for token in tokens:
        if token[0:1] in "0123456789":  # Python2/3 ecumenical
            try:
                if sys.version[0] == '2':
                    # This is Python2
                    token = token.decode("latin1")
                retval += int(token)
            except:
                # if the latin decode fails, or if token isn't an integer,
                # an exception will be thrown.  As described above, just
                # ignore that for now.
                pass
    return retval

def get_refmod(arg):
    refmod_spec = ""
    clean_arg = ""

    if arg.startswith('(') and arg.endswith(')'):
        refmod_spec = arg[1:-1]
    elif arg.find('(') != -1 and arg.find(')') != -1:
        refmod_spec = arg[arg.index('(') + 1:-1]
        clean_arg = arg[:arg.index('(')]
    else:
        clean_arg = arg

    return clean_arg, refmod_spec

def get_lenspec_refmod(refmod, payload, lenspec_type):
    valid = 1

    if lenspec_type == "p":
        startloc = payload.location_of_data
        length = payload.Length
    elif lenspec_type == "n":
        startloc = 1
        length = payload.Length
    else:
        print("Not a valid lenspec type")
        return 0, 0, 0

    lenspec = refmod.split(':')
    if len(lenspec) != 2:
        print("Offset and length specification not recognized")
        return 0, 0, 0
    else:
        if lenspec[0] != '' and int(lenspec[0]) <= length:
            startloc += int(lenspec[0]) - 1
            length -= int(lenspec[0]) - 1
        elif lenspec[0] != '' and int(lenspec[0]) > length:
            print("Startposition > variable length: {0}".format(length))
            return 0, startloc, length

        if lenspec[1] != '' and int(lenspec[1]) <= length:
            length = int(lenspec[1])
        elif lenspec[1] != '' and int(lenspec[1]) > length:
            print("Length specifier to large, max: {0}".format(length))
            return 0, 0, 0

    return valid, startloc, length

class VarTrie():
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

    def AddToTrie(self, trie, name_tokens, payload_index):
        our_node = trie

        for name in name_tokens:
            if name not in our_node.children:
                new_child = VarTrieNode()
                new_child.piece_of_name = name
                new_child.parent = our_node
                our_node.children[name] = new_child
            our_node = our_node.children[name]

        # We're all the way at the end:
        our_node.payload_index = payload_index
        our_node.count = 1

        # Propogate the count back up to the root:
        while our_node.parent:
            our_node.parent.count += our_node.count
            our_node = our_node.parent

        # and we are done

    def LoadFromLine(self, variable_line):
        tokens = variable_line.split("|")
        try:
            if len(tokens) == 11:
                # We have enough in the way of tokens, so proceed
                if tokens[self.IndexType] == 'P':
                    # We are entering a new PROGRAM-ID, so reset our stacks
                    self.StackOfNames = [tokens[self.IndexName]]
                    self.StackOfLevels = [-2]

                if len(self.StackOfNames) == 0:
                    print("Please save your program for Bob Dubner at support@COBOLworx.com")
                    print("self.StackOfNames is empty, and it shouldn't be")
                    sys.exit(1)

                if tokens[self.IndexType] in ('F', 'W', 'L', 'I'):
                    # We are working with a variable.  We need to erode the
                    # stack of names down to the variable who is our parent:
                    if tokens[self.IndexLevel] == "":
                        tokens[self.IndexLevel] = '0'
                    our_level = int(tokens[self.IndexLevel])
                    if our_level in (77, 78):
                        our_level = 1
                    while self.StackOfLevels[-1] >= our_level:
                        self.StackOfNames.pop(-1)
                        self.StackOfLevels.pop(-1)

                    # and now we put ourselves into the position of
                    # most favored child
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
                    payload.Offset     = AddEmUp(tokens[self.IndexOffset])
                    payload.Length     = AddEmUp(tokens[self.IndexLength])
                    payload.OccursMin  = int(tokens[self.IndexOccursMin] or "0")
                    payload.OccursMax  = int(tokens[self.IndexOccursMax] or "0")

                    payload.payload_index = len(self.storage_list)
                    self.storage_list.append(payload)
                    self.AddToTrie(self.storage_trie,
                                   reversed,
                                   payload.payload_index)
                    self.program_ids.add(payload.Program)
        except:
            traceback.print_exc()
            sys.exit(1)

    def TokenizeCanonicalName(self, desired):
        # We allow "A of B in C", where [of|in] is not case sensitive
        # We also allow A.B.C, A/B/C, and "A B C"

        # turn IN and OF into spaces
        pattern = re.compile(r"^(.*) (in|In|iN|IN|of|oF|Of|OF) (.*)$")
        while True:
            match = re.search(pattern, desired)
            if not match:
                break
            else:
                desired = match.group(1) + " " + match.group(3)

        desired = desired.replace(".", " ")
        desired = desired.replace("/", " ")
        retval = desired.split()
        return retval

    def NameMatcher(self,
                    boys,
                    boy_index,
                    trie,
                    paths):
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
        if boy_index < len(boys):
            # we still have at least one boy to leave off at his tree
            boy = boys[boy_index]
            # print("DEBUG boy",boy)
            # print("DEBUG trie.children.keys",trie.children.keys())
            # Do a case-insensitive match:
            bFound = False
            for child in trie.children:
                if boy.upper() == child.upper():
                    bFound = True
                    boy = child
                    break
            if bFound:
                # We have found this boy's tree.  Advance both the boy
                # and the trie.
                next_trie = trie.children[boy]
                boy_index += 1
                # Continue along that path.
                self.NameMatcher(boys, boy_index, next_trie, paths)
                # and terminate.
                return
            else:
                # We can't leave that boy at this node.  So, we have to
                # check for that boy on all possible paths from here:
                child_tries = list(trie.children.values())
                for next_trie in child_tries:
                    self.NameMatcher(boys, boy_index, next_trie, paths)
                # and terminate.
                return
        else:
            # we have dropped off the whole string of boys.  Now it's a
            # question of finding every possible path to the edge of the
            # forest from that point.
            if len(trie.children) > 0:
                # We have not yet reached the edge of the forest:
                child_tries = list(trie.children.values())
                for next_trie in child_tries:
                    self.NameMatcher(boys, boy_index, next_trie, paths)
                # and terminate.
                return
            else:
                # This node has no children, which means we have reached the
                # edge of the forest.

                # Log it:
                paths.append(trie.payload_index)
                #
                # and terminate
                return

    def GetAllPossibilities(self):
        # Typically in response to a 'cprint *'
        paths = []
        for index in range(len(self.storage_list)):
            paths.append(index)
        return paths

    def GetListOfExactPossibilities(self, token):
        # We are going to return variables that contain the given token
        # in its exact, case-insensitive, form.  This is typically used
        # by the 'cprint ?' form

        # Wander through the forest, looking for possible paths:
        paths = []
        self.NameMatcher([token], 0, self.storage_trie, paths)

        return paths

    def GetListOfPossibilities(self, desired):
        if isinstance(desired, list):
            # We were handed a bunch of pieces.  Let's
            # glue them back together into a single space-separated string:
            desired = " ".join(desired).upper()

        # Go handle our various "A of B" possibilities:
        name_tokens = self.TokenizeCanonicalName(desired)

        # Wander through the forest, looking for possible paths:
        paths = []

        if len(name_tokens) == 0:
            # if there's nothing, there is nothing
            return paths

        self.NameMatcher(name_tokens, 0, self.storage_trie, paths)

        if len(paths) != 1:
            # A strict attempt at matching what we were given has come up
            # with bupkis.  So far we have either no possibilities, or
            # more than one possibility.

            # Put what we have so far into a set:
            set1 = set(paths)

            # When given something like FIRST IN HOLDER, we want
            # to find 10 FIRSTNAME/NAME/ACCOUNTHOLDER
            #
            # But we don't want that request to pull up
            #  10 HOLDER/NAME/FIRST
            #
            # So, we are going to march through all of the names
            # in storage_list, and then we will make sure all of
            # the pieces show up in left-to-right order

            for index in range(len(self.storage_list)):
                whole_name = self.storage_list[index].Name.upper()
                prior_index = -1
                for fragment in name_tokens:
                    fragment = fragment.upper()
                    current_index = whole_name.find(fragment, prior_index + 1)
                    if current_index == -1:
                        # This fragment isn't in that name, so, sayonara
                        prior_index = -1
                        break
                    if current_index < prior_index:
                        # Going backward isn't permitted
                        prior_index = -1
                        break
                    # Rightward, ho!
                    prior_index = current_index
                if prior_index != -1:
                    # We have found a contender!
                    set1.add(index)

            # Turn the set back into a list:
            paths = list(set1)

        return paths

class LineList():
    """List of starting lines of ProgramNames"""

    # The point of this list is to be able to figure out, when a trap occurs,
    # which COBOL routine the trap occurred in.  We can't just look at the
    # information from the gdb "frame" command, because the .c code doesn't
    # use the exact same name. It's similar, but it's transmogrified.  But
    # we can know the line number where the trap occurred, and scanning
    # the .tab file lets us convert line numbers to the original COBOL
    # program-id

    def __init__(self):
        self.line_list = []  # This will be a list of (int, str) tuples
        self.EndOfTheWorld = 10000000

    def Insert(self, line_number, program_id):
        self.line_list.append((line_number, program_id))

    def Find(self, line_number):
        # returns the program-id that covers line_number

        if len(self.line_list) == 0:
            return ""

        if line_number < self.line_list[0][0] or line_number >= self.line_list[-1][0]:
            return ""

        left = 0
        right = len(self.line_list)

        while True:
            middle = (right+left) // 2
            if self.line_list[middle][0] <= line_number \
                    and line_number < self.line_list[middle + 1][0]:
                break
            if self.line_list[middle][0] > line_number:
                right = middle
            else:
                left = middle

        return self.line_list[middle][1]

    def LoadFromLine(self, variable_line):
        if len(variable_line) > 0 and variable_line[0] == 'P':
            tokens = variable_line.split("|")
            if len(tokens) >= 3:
                program_id = tokens[2]
                line_number = int(tokens[6])
                self.Insert(line_number, program_id)

class CobolVariable():
    """Manages the field, attributes, flags, and pointers of a GnuCOBOL COBOL variable"""
    def __init__(self):
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
        self.display_pic = ""

        # This section is used for "StringToxxx processing
        self.rvalue = ""            # Original user string, before cleaning
        self.cleaned_rvalue = ""    # This will have exactly AttrDigits, zero-filled
        #                           # as necessary to the left and right
        self.is_negative = False    # True if there was a minus sign
        self.expanded_picture = ""  # "9(4)" becomes "9999" here
        self.ldigits = -1           # this integer is AttrDigits - AttrScale
        self.rdigits = -1           # this integer is AttrScale...
        #                           # ...but see self.CleanAndScale

    def CleanTokens(self, execute_result):
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
        if left != -1 and right != -1:
            execute_result = execute_result[:left] + execute_result[right + 1:]

        # Turn extraneous characters into spaces, to mollify split()
        execute_result = execute_result.replace("'", ' ')
        execute_result = execute_result.replace("{", ' ')
        execute_result = execute_result.replace("}", ' ')
        execute_result = execute_result.replace(",", ' ')
        execute_result = execute_result.replace("<", ' ')
        execute_result = execute_result.replace(">", ' ')

        ex_tokens = execute_result.split()

        # Get rid of superfluous tokens. cgdb has a bunch whose initial
        # characters are \x1a characters; we don't want them.  Nor do we
        # want 0x55555555, although we will pass through 0x0 (because that's
        # what we see for a picture pointer when there is no picture)
        tokens = []
        for xtoken in ex_tokens:
            if ((ord(xtoken[0]) > 32
                and xtoken != '='
                and xtoken != '*'
                and xtoken != '-'
                and xtoken[0] != '0')
                or xtoken == "0"
                or xtoken == "0x0"):
                tokens.append(xtoken)
        return tokens

    def get_verified_field_location(self,allocate):
        # When calling this routine with ALLOCATE, the caller must do
        # free_field_location(returned_value)
        #
        # Sometimes self.Fname is valid.  But sometimes it's not.
        #
        # working-storage is the easiest; if there is a self.Fname, then
        # everything is okay and you can just return self.location_of_field.
        #
        # Local storage can have self.Fname, but sometimes the data in the
        # cob_field.data pointer isn't loaded up with the data that's actually
        # available in the cob_local_ptr area.  We can fix that.
        #
        # If a variable isn't referenced in the COBOL source code, then it doesn't
        # have self.Fname.  But the data is actually at self.location_of_data.  In
        # that case, if the caller has allocate turned on, we can synthesize a temporary
        # cob_field.
        #
        # If allocate is turned off in that last case, we return None
        #
        GV_GlobalVariables.AllocateBuffers();
        try:
            if not allocate:
                if self.Fname and self.Section not in ('L'):
                    # working storage or linkage, so we are safe to go simple:
                    return self.location_of_field

                if self.Fname and self.Section in ('L'):
                    # This is the case where we update the cob_field data area
                    # from self.location_of_data:
                    command = "set var {0}".format(self.Fname) + " = {"
                    command += "{0}, (char *){1}, &{2}".format(self.Length, hex(self.location_of_data), self.Aname)
                    command += "}"
                    gdb.execute(command, False, True)
                    return self.location_of_field

                # There is no self.Fname.  So, use the one in GlobalVariables:
                retval = GV_GlobalVariables.cprint_f0
                command = "set var *(cob_field *){0}".format(hex(retval)) + " = {"
                command += "{0}, (char *){1}, &{2}".format(self.Length, hex(self.location_of_data), self.Aname)
                command += "}"
                gdb.execute(command, False, True)
                return retval

            # We *must* allocate, because our caller is expected us to, so they
            # will be calling free_field_location

            command = "call (char*)cob_malloc(sizeof(cob_field))"
            retval = int(gdb.execute(command, False, True).split()[2],16)

            # Having allocated a synthetic cob_field, initialize it with the data
            # we have
            command = "set var *(cob_field *){0}".format(hex(retval)) + " = {"
            command += "{0}, (char *){1}, &{2}".format(self.Length, hex(self.location_of_data), self.Aname)
            command += "}"
            gdb.execute(command, False, True)
            return retval

        except gdb.error:
            traceback.print_exc()
            print("Command failed in get_verified_field_location:", command)
            retval = None
        return retval;

    def free_field_location(self,field_location):
        gdb.execute("call (void *)cob_free({0})".format(hex(field_location)), False, True)

    def GetDataAsString(self):
        retval = ""
        location_of_field = self.get_verified_field_location(DONT_ALLOCATE)
        if not location_of_field:
            return retval

        if False:  # For development
            print("GetData AsString          Name:", self.Name)
            print("GetData AsString       Section:", self.Section)
            print("GetData AsString         Fname:", self.Fname or "f0")
            print("GetData AsString         Bname:", self.Bname)
            print("GetData AsString         Aname:", self.Aname)
            print("GetData AsString        Offset:", self.Offset)
            print("GetData AsString        Length:", self.Length)
            print("GetData AsString len(self.data)", len(self.data))
            print("GetData AsString      AttrType:", hex(self.AttrType))
            print("GetData AsString    AttrDigits:", self.AttrDigits)
            print("GetData AsString     AttrScale:", self.AttrScale)
            print("GetData AsString     AttrFlags:", hex(self.AttrFlags))
            print("GetData AsString  loc_of_field:", hex(location_of_field))
            print("GetData AsString   loc_of_data:", hex(self.location_of_data))
            print("GetData AsString   loc_of_attr:", hex(self.location_of_attribute))
            print("GetData AsString    loc_of_pic:", hex(self.location_of_pic))

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

        try:
            buffer = GV_GlobalVariables.cprint_buffer
            command = "call (const char *)cob_get_field_str((cob_field *){0}, (char *){1}, {2})".format(hex(location_of_field),
                                                                                                         hex(buffer), PRINT_BUFFER_SIZE )
            gdb.execute(command, False, True)
        except gdb.error:
            print("Command failed in GetDataAsString:", command)
            return "<Error in GetDataAsString>"

        # The formatted variable string is in buffer
        if is_alphanumeric :
            retval = '"'

        buf_ptr = buffer
        while True:
            char = GetDataAt(buf_ptr, 1)
            if char == 0:
                break
            retval += chr(char)
            buf_ptr += 1
        if is_alphanumeric :
            retval += '"'

        return retval

    def PutDataAsString(self, rside):
        return_value = 1

        location_of_field = self.get_verified_field_location(DONT_ALLOCATE)
        if not location_of_field:
            return 0

        try:
        # Let's leverage GDB to set our buffer to rside:
            buffer = GV_GlobalVariables.cprint_buffer

            buf_ptr = buffer
            for index in range(len(rside)):
                n = rside[index]
                command = "set *(char *){0} = '{1}'".format(hex(buf_ptr),n)
                gdb.execute(command, False, True)
                buf_ptr += 1
            command = "set *(char *){0} = '\\0'".format(hex(buf_ptr))
            gdb.execute(command, False, True)

            # Now use the target executable's libcob to convert the string to the field:
            command = "call (const char *)cob_put_field_str((cob_field *)"
            command += hex(location_of_field)
            command += ",(char *)" + hex(buffer)
            command += ")"
            gdb.execute(command, False, True)
        except gdb.error:
            print("COMMAND failed in PutDataAsString: ", command)
            return_value = 0

        return return_value

    def PutDataAsVar(self, payload_s):
        return_value = 1

        # We use the ALLOCATE option because of the possibility that one
        # or both of the source and destination might be, weirdly, COBOL
        # variables that don't have valid f_xx cob_fields, but for some
        # reason the developer wants use them as source or destination

        destination = self.get_verified_field_location(ALLOCATE)
        if not destination:
            return 0

        source = payload_s.get_verified_field_location(ALLOCATE)
        if not source:
            self.free_field_location(destination)
            return 0

        try:
            command = "call (void)cob_move((cob_field *){0}, (cob_field *){1})".format(hex(source), hex(destination))
            gdb.execute(command, False, True)
            #print("COMMAND succeeded in PutDataAsVar: ", command)
        except gdb.error:
            print("COMMAND failed in PutDataAsVar: ", command)
            return_value = 0

        self.free_field_location(source)
        self.free_field_location(destination)

        return return_value

    def GetDataAsHex(self):
        retval = ""
        if len(self.data) > 16:
            return "<see below>"
        if len(self.data) > 0:
            retval = "0x"

            output_bytes = self.data[:]
            if GV_GlobalVariables.byteorder == "little":
                if (self.AttrFlags & COB_FLAG_BINARY_SWAP) == 0: # If this is ON, then it is already in display order
                    if (self.AttrFlags & (COB_FLAG_REAL_BINARY | COB_FLAG_IS_POINTER | COB_FLAG_IS_FP)) != 0:
                        # These three types are little-endian; flip them so they display with the big byte to the left
                        output_bytes.reverse()
            for byte in output_bytes:
                hexout = hex(byte)[-2:]
                if hexout[0:1] == 'x':
                    hexout = '0' + hexout[-1:]
                retval += hexout
        return retval

    def FetchVariableData(self, line_list, show_failure):
        # print("FetchVariableData:", self.Name, self.Section)
        self.location_of_field = 0
        self.location_of_data = 0
        self.location_of_attribute = 0
        self.data = []
        self.display_body = ""
        self.display_body_hex = ""
        self.display_pic = ""
        self.display_name = self.NameToString()
        self.not_in_context = False

        # Let's update everything we might need:

        # First, check to see if the compiler provided us with the
        # the name of the field.  If so, use the run-time values in that named
        # field to override anything we might have gotten from compile-time

        if self.Fname:
            # We have a field name
            # print("FetchVariableData Fname", self.Fname)
            self.location_of_field = GetAddressOf(self.Fname)
            if self.location_of_field is None:
                self.not_in_context = True
                return
            # The cob_field structure looks like this:
            #   typedef struct __cob_field
            #       {
            #       size_t                size;
            #       unsigned char        *data;
            #       const cob_field_attr *attr;
            #       } cob_field;
            self.Length = GetDataAt(self.location_of_field, GV_GlobalVariables.SizeofPointer)
            if self.Length is None:
                self.not_in_context = True
                return
            possible_location_of_data = GetDataAt(self.location_of_field + GV_GlobalVariables.SizeofPointer, GV_GlobalVariables.SizeofPointer)
            if possible_location_of_data:
                self.location_of_data = possible_location_of_data
            self.location_of_attribute = GetDataAt(self.location_of_field + 2 * GV_GlobalVariables.SizeofPointer, GV_GlobalVariables.SizeofPointer)
        else:
            # print("There is no self.Fname")
            pass

        if self.location_of_data == 0:
            # When the COBOL code doesn't reference a variable, there is no f_ cob_field.  But
            # sometimes we are able to divine a b_ + offset reference.  Likewise, sometimes
            # there is an f_value, but at this point in the code the cob_field->data value hasn't yet
            # been set up.  But, again, sometimes we have advance knowledge of what is going to be
            # there because we got information from the compiler.
            if self.Bname:
                # print("FetchVariableData Fname", self.Bname)
                self.location_of_data = GetAddressOf(self.Bname)
                if self.location_of_data is None:
                    self.not_in_context = True
                    return
                self.location_of_data += self.Offset
            else:
                # print("There is no self.Bname")
                pass

        if self.location_of_attribute == 0:
            if self.Aname:
                self.location_of_attribute = GetAddressOf(self.Aname)
                if self.location_of_attribute is None:
                    self.not_in_context = True
                    return
            else:
                # print(" There is no self.Aname")
                pass

        if self.location_of_attribute != 0:
            #   typedef struct __cob_field_attr
            #       {
            #       unsigned short      type;       /* Field type */
            #       unsigned short      digits;     /* Digit count */
            #       signed short        scale;      /* Field scale */
            #       unsigned short      flags;      /* Field flags */
            #       const cob_pic_symbol    *pic;       /* Pointer to picture string */
            #       } cob_field_attr;
            self.AttrType   = GetDataAt(self.location_of_attribute + 0, 2)
            self.AttrDigits = GetDataAt(self.location_of_attribute + 2, 2)
            self.AttrScale  = GetDataAt(self.location_of_attribute + 4, 2)
            self.AttrFlags  = GetDataAt(self.location_of_attribute + 6, 2)
            self.location_of_pic = GetDataAt(self.location_of_attribute + 8, GV_GlobalVariables.SizeofPointer)

        if self.Section in('F', 'W', 'I'):
            pass
        elif self.Section == 'L':
            try:
                # We are looking for something in LOCAL-STORAGE.  Local
                # storage is kept in a calloc-ed memory location named
                # cob_local_ptr.  The complication is that if we have
                # several program-ids that each have their own cob_local_ptr
                # we have to make sure we know which one we are dealing
                # with.

                # Pick up the decimal line number where we are trapped
                line_number = -1
                command = "frame"
                frame_string = GdbExecute(currentframe(), command)
                # Look for a number after a colon
                nfound = frame_string.find(':')
                if nfound > -1:
                    frame_string = frame_string[nfound + 1:]
                    frame_string = frame_string.split()
                    try:
                        line_number = int(frame_string[0])
                    except:
                        line_number = -1

                # print("Trapped at ",line_number)
                # We know we are trapped at line_number

                # Look up the program-id of that line_number
                program_id = line_list.Find(line_number)
                # print("in :", program_id)

                #
                # program-id is where the program-under-test is
                # trapped.  Let's see if that program-id matches
                # the final place of the desired variable:
                #
                # print( self.Name )
                split_name = self.Name.split('/')
                if split_name[-1] != program_id:
                    self.not_in_context = True
                    return

                # To recap:  We are trapped in the routine of the desired
                # LOCAL-STORAGE variable, so we can look for it in cob_local_ptr
                # we need GDB's help finding out where cob_local_ptr + offset is:
                command = "x " + "cob_local_ptr + " + str(self.Offset)
                address_string = GdbExecute(currentframe(), command)
                nfound = address_string.find(':')
                if nfound != -1:
                    self.location_of_data = int(address_string[:nfound], 16)

            except gdb.error:
                self.not_in_context = True
        else:
            print("We don't know how to handle", self.Section, self.Name)
            self.not_in_context = True
            return

        if self.location_of_data != 0:
            self.data = GetBytesAt(self.location_of_data, self.Length)

        self.display_body_hex = self.GetDataAsHex()
        self.display_body = self.GetDataAsString()
        self.display_body = ReplaceWithRepeatCount(self.display_body)

        if self.Length != len(self.data):
            # This probably should never happen.  But when it did, during debugging,
            # it actually corrupted memory during the process of creating a
            # cfields.cob_field.
            #
            # So, let's put up a fence:
            self.not_in_context = True
        return

    def NameToString(self):
        storage_type = ""
        if self.Section == "F":
            storage_type = "I/O"
        elif self.Section == "W":
            storage_type = "W-S"
        elif self.Section == "L":
            storage_type = "L-S"
        elif self.Section == "I":
            storage_type = "LNK"

        s = "{1:02} {2} [{0}]".format(storage_type, self.Level, self.Name)
        return s

    def ShowFields(self):
        bname_location = GetAddressOf(self.Bname) or 0
        bname_text = ""
        if bname_location:
            bname_text = "( %s )" % hex(bname_location)

        data_ptr = hex(self.location_of_data)

        print("Name:       ", self.display_name)
        print("Display:    ", self.display_body)
        print("Hex:        ", self.display_body_hex)

        if self.Section == 'L':
            # This is local-storage
            print("Field:      ", "f0" )
        else:
            if self.Fname:
                print("Field:      ", self.Fname, '   (', hex(self.location_of_field), ')')
            else:
                print("Field:      ", "<none>")

        if self.Section == 'L':
            # This is local-storage
            print("Base:       ", "cob_local_ptr")
        else:
            print("Base:       ", self.Bname, '  ', bname_text)

        print("Offset:     ", self.Offset)
        print("Data:       ", data_ptr)
        print("Length:     ", self.Length)

        print("Attr:       ", self.Aname, '   (', hex(self.location_of_attribute), ')')
        print("AttrType:   ", self.AttrType, end="")
        print(" (", hex(self.AttrType), ") ", sep='', end="")
        if self.AttrType == 0:
            print(" (Unknown)", end="")
        elif self.AttrType == COB_TYPE_UNKNOWN:
            print("UNKNOWN            ", end="")
        elif self.AttrType == COB_TYPE_GROUP:
            print("GROUP              ", end="")
        elif self.AttrType == COB_TYPE_BOOLEAN:
            print("BOOLEAN            ", end="")
        elif self.AttrType == COB_TYPE_NUMERIC_DISPLAY:
            print("NUMERIC_DISPLAY    ", end="")
        elif self.AttrType == COB_TYPE_NUMERIC_BINARY:
            print("NUMERIC_BINARY     ", end="")
        elif self.AttrType == COB_TYPE_NUMERIC_PACKED:
            print("NUMERIC_PACKED     ", end="")
        elif self.AttrType == COB_TYPE_NUMERIC_FLOAT:
            print("NUMERIC_FLOAT      ", end="")
        elif self.AttrType == COB_TYPE_NUMERIC_DOUBLE:
            print("NUMERIC_DOUBLE     ", end="")
        elif self.AttrType == COB_TYPE_NUMERIC_L_DOUBLE:
            print("NUMERIC_L_DOUBLE   ", end="")
        elif self.AttrType == COB_TYPE_NUMERIC_FP_DEC64:
            print("NUMERIC_FP_DEC64   ", end="")
        elif self.AttrType == COB_TYPE_NUMERIC_FP_DEC128:
            print("NUMERIC_FP_DEC128  ", end="")
        elif self.AttrType == COB_TYPE_NUMERIC_FP_BIN32:
            print("NUMERIC_FP_BIN32   ", end="")
        elif self.AttrType == COB_TYPE_NUMERIC_FP_BIN64:
            print("NUMERIC_FP_BIN64   ", end="")
        elif self.AttrType == COB_TYPE_NUMERIC_FP_BIN128:
            print("NUMERIC_FP_BIN128  ", end="")
        elif self.AttrType == COB_TYPE_NUMERIC_COMP5:
            print("NUMERIC_COMP5      ", end="")
        elif self.AttrType == COB_TYPE_ALNUM:
            print("ALNUM              ", end="")
        elif self.AttrType == COB_TYPE_ALPHANUMERIC:
            print("ALPHANUMERIC       ", end="")
        elif self.AttrType == COB_TYPE_ALPHANUMERIC_ALL:
            print("ALPHANUMERIC_ALL   ", end="")
        elif self.AttrType == COB_TYPE_ALPHANUMERIC_EDITED:
            print("ALPHANUMERIC_EDITED", end="")
        elif self.AttrType == COB_TYPE_NUMERIC_EDITED:
            print("NUMERIC_EDITED     ", end="")
        elif self.AttrType == COB_TYPE_NATIONAL:
            print("NATIONAL           ", end="")
        elif self.AttrType == COB_TYPE_NATIONAL_EDITED:
            print("NATIONAL_EDITED    ", end="")
        print()

        print("AttrDigits: ", self.AttrDigits)
        print("AttrScale:  ", self.AttrScale)

        print("AttrFlags:  ", hex(self.AttrFlags), end='')
        if (self.AttrFlags & COB_FLAG_HAVE_SIGN) != 0:
            print(" HAVE_SIGN", end='')
        if (self.AttrFlags & COB_FLAG_SIGN_SEPARATE) != 0:
            print(" SIGN_SEPARATE", end='')
        if (self.AttrFlags & COB_FLAG_SIGN_LEADING) != 0:
            print(" SIGN_LEADING", end='')
        if (self.AttrFlags & COB_FLAG_BLANK_ZERO) != 0:
            print(" BLANK_ZERO", end='')
        if (self.AttrFlags & COB_FLAG_JUSTIFIED) != 0:
            print(" FLAG_JUSTIFIED", end='')
        if (self.AttrFlags & COB_FLAG_BINARY_SWAP) != 0:
            print(" BINARY_SWAP", end='')
        if (self.AttrFlags & COB_FLAG_REAL_BINARY) != 0:
            print(" REAL_BINARY", end='')
        if (self.AttrFlags & COB_FLAG_IS_POINTER) != 0:
            print(" IS_POINTER", end='')
        if (self.AttrFlags & COB_FLAG_NO_SIGN_NIBBLE) != 0:
            print(" NO_SIGN_NIBBLE", end='')
        if (self.AttrFlags & COB_FLAG_IS_FP) != 0:
            print(" IS_FP", end='')
        if (self.AttrFlags & COB_FLAG_REAL_SIGN) != 0:
            print(" REAL_SIGN", end='')
        if (self.AttrFlags & COB_FLAG_BINARY_TRUNC) != 0:
            print(" BINARY_TRUNC", end='')
        if (self.AttrFlags & COB_FLAG_CONSTANT) != 0:
            print(" CONSTANT ", end='')
        print()

        print("Data: (HEX)   [", end=" ")
        for byte in self.data:
            hexout = hex(byte)[-2:]
            if hexout[0:1] == 'x':
                hexout = '0' + hexout[-1:]
            print(hexout, end=" ")
        print("]")

        print("Data: (ASCII) \"", end="")
        for byte in self.data:
            if 32 <= byte < 128:   # valid ASCII characters
                print(chr(byte), end="")
            else:
                print('.', end="")
        print('"')

class TabFileInformation():
    def __init__(self):
        self.var_trie = VarTrie()
        self.old_dogs = set()
        self.line_list = LineList()
        self.current_variable_string = ""
        self.current_trapped_routine = ""
        self.current_trapped_file = ""
        self.current_trapped_frame = -1
        self.current_trapped_line = 0
        self.question_mark_range = 6
        self.current_full_path = ""
        self.perform_commands = {}

    def ReadVariableString(self, string_name):
        retval = False

        # We are going to use the x/ns "memory as string" command, which reads
        # in n blocks of some number of characters.  On gdb 8.3, each block seems
        # to be 201 characters. I can't know if that's universal, so I am going
        # to make this code adaptive

        # We are going to read a single line of the string, in order to figure
        # how how many characters per line we're going to get:

        the_address = GetAddressOf(string_name)
        if the_address:
            command = "x/1s " + str(the_address)
            try:
                the_data = GdbExecute(currentframe(), command)
                index1 = the_data.find('"')
                if index1 != -1:
                    index2 = the_data.find('"', index1 + 1)
                    if index2 != -1:
                        total_length = int(the_data[index1 + 1:index1 + 1 + 9])
                        chars_each_line = index2 - (index1 + 1)

                        # We know the total length, and we know the number of
                        # chars we will get in each block.  So, we can calculate
                        # the number of blocks we need to ask for, and build
                        # that command:
                        blocks = (total_length + chars_each_line - 1) // chars_each_line
                        command = "x/" + str(blocks) + "s " + str(the_address)

                        # And we can get that data
                        the_data = GdbExecute(currentframe(), command)

                        # And now we loop through the data, extracting each chunk
                        is_first_line = True
                        index2 = 0
                        lines = []
                        while True:
                            index1 = the_data.find('"', index2 + 1)
                            if index1 == -1:
                                break
                            index2 = the_data.find('"', index1 + 1)
                            if index2 == -1:
                                break
                            if is_first_line:
                                index1 += 9
                                is_first_line = False
                            lines.append(the_data[index1 + 1:index2])

                        # and now that we have all the chunks, we can assemble
                        # them into the VARIABLE_STRING_xxx
                        self.current_variable_string = "".join(lines)
                        retval = True
            except:
                pass
        return retval

    def LoadPerformFromLine(self,variable_line):
        if len(variable_line) > 0 and variable_line[0] == 'p':
            tokens = variable_line.split("|")
            if len(tokens) > 3:
                self.perform_commands[tokens[3]] = tokens[2]

    def ProcessStack(self):
        """ Figures out which routine we are currently trapped in"""

        # That's seems innocent enough.  But we're dealing with a multitude of sins.

        # In the simplest incarnation, we have a simple program with a simple trap, in which
        # case the results of the "frame" and "info stack" commands look like this:

        #    (gdb) frame
        #    #0  MAIN_ (entry=0) at MAIN.cbl:11

        #    (gdb) info stack
        #    #0  MAIN_ (entry=0) at MAIN.cbl:11
        #    #1  0x0000555555554cc9 in MAIN () at MAIN.cbl:8
        #    #2  0x0000555555554cb4 in main (argc=1, argv=0x7fffffffe368) at MAIN.cbl:8

        # But we can trap through copybooks.  That can look like this:

        #    (gdb) frame
        #    #0  C_ (entry=0, b_11=0x7ffff35172f0 <b_8> "B   4") at INDENTER.cpy:2

        #    (gdb) info stack
        #    #0  C_ (entry=0, b_11=0x7ffff35172f0 <b_8> "B   4") at INDENTER.cpy:2
        #    #1  0x00007ffff3112ddf in C (b_11=0x7ffff35172f0 <b_8> "B   4") at C.cbl:10
        #    #2  0x00007ffff331622d in B_ (entry=0, b_11=0x555555756130 <b_8> "MAIN2") at B.cbl:18
        #    #3  0x00007ffff3315e3f in B (b_11=0x555555756130 <b_8> "MAIN2") at B.cbl:10
        #    #4  0x0000555555554fa3 in MAIN_ (entry=0) at MAIN.cbl:11
        #    #5  0x0000555555554cc9 in MAIN () at MAIN.cbl:8
        #    #6  0x0000555555554cb4 in main (argc=1, argv=0x7fffffffe368) at MAIN.cbl:8

        # Our problem with that is there is no VARIABLE_STRING_INDENTER_CPY; we have to look down
        # through the stack until we find VARIABLE_STRING_C_CBL in order to have a hope of being
        # able to show the right variables.

        # The third case is when the user wants to look at variables in a different frame.  As
        # an example, suppose the user trapped at INDENTER.cpy:2 issues a "frame 3" command.  Here's
        # what we see:

        #    (gdb) frame
        #    #3  0x00007ffff3315e3f in B (b_11=0x555555756130 <b_8> "MAIN2") at B.cbl:10

        #    (gdb) info stack
        #    #0  C_ (entry=0, b_11=0x7ffff35172f0 <b_8> "B   4") at INDENTER.cpy:2
        #    #1  0x00007ffff3112ddf in C (b_11=0x7ffff35172f0 <b_8> "B   4") at C.cbl:10
        #    #2  0x00007ffff331622d in B_ (entry=0, b_11=0x555555756130 <b_8> "MAIN2") at B.cbl:18
        # --> #3  0x00007ffff3315e3f in B (b_11=0x555555756130 <b_8> "MAIN2") at B.cbl:10
        #    #4  0x0000555555554fa3 in MAIN_ (entry=0) at MAIN.cbl:11
        #    #5  0x0000555555554cc9 in MAIN () at MAIN.cbl:8
        #    #6  0x0000555555554cb4 in main (argc=1, argv=0x7fffffffe368) at MAIN.cbl:8

        # The algorithm that handles those cases seems to be this:
        #   * Pull the file.name from the "frame" results.
        #   * Walk through "info stack" results until the first instance of file.name is found;
        #     that becomes current_trapped_file and current_trapped_line
        #   * Starting at that "info stack" line, walk downward until a valid VARIABLE_STRING
        #     is found.
        # That might give a bogus result.  For example, if the frame is set to a module that
        # wasn't compiled with debugging, then the walk will take you to a module that does
        # have debugging.  That's not what you want, but I don't see how to avoid the problem
        # if you also want to be able to handle code in COPYBOOKs.

        retval = False
        try:
            command = "frame"
            info_frame = GdbExecute(currentframe(), command)
            command = "info stack"
            info_stack = GdbExecute(currentframe(), command)
            if len(info_stack) > 0 and info_stack[0] == '#':
                retval = True

                # Get the frame number from info_frame:
                frame_from_frame = int(info_frame.split(' ')[0][1:])

                # Get the file name from the first line of the "frame" information:
                m = info_frame.find('\n')
                if m > -1:
                    info_frame = info_frame[:m]
                tokens = info_frame.rstrip().split(' ')
                filename = tokens[-1].split(':')[0]

                # We now march through the info_stack lines, looking for the
                # first line whose filename matches filename:

                stack = info_stack.split('\n')
                stack_line = 0
                while stack_line < len(stack):
                    tokens = stack[stack_line].split(' ')

                    frame_from_stack = int(tokens[0][1:])

                    # That line looks like this:
                    #
                    # #0  optfde01_ (entry=0) at optfde01.cbl:59
                    #
                    # We need the information from the final token:

                    file_and_line = tokens[-1].split(':')
                    if file_and_line[0] == filename:
                        break
                    stack_line += 1

                if stack_line >= len(stack):
                    print("Bad things are happening.  Save this program for Dubner, and tell him")
                    print("that \"frame\" and \"info stack\" don't match in ProcessStack()")
                    return False

                # Unexpected and confusing things happen when the frame is set
                # to a PROCEDURE DIVISION line. So, if an adjustment is necessary,
                # we make it here:

                if frame_from_frame != frame_from_stack:
                    command = "select-frame " + str(frame_from_stack)
                    result = GdbExecute(currentframe(), command)

                # At this point, file_and_line[0] matches the file name from "frame"
                self.current_trapped_line = int(file_and_line[1])

                if file_and_line[0] != self.current_trapped_file or frame_from_frame != frame_from_stack:
                    # We are in a new file, or perhaps a new frame
                    self.current_trapped_file = file_and_line[0]
                    self.current_trapped_frame = frame_from_stack

                    # clear out the persistent 'p ?' memory; it has suddenly
                    # become obsolete
                    self.old_dogs = set()

                    # Because it's possible that we are trapped in a copybook,
                    # we can't assume that VARIABLE_STRING has the name
                    # of the current_trapped_file.  We need to work our
                    # way through the stack:

                    # Find the address of the VARIABLE_STRING for this trap point

                    while stack_line < len(stack):
                        possibility = stack[stack_line]
                        # Pull out the file name, which is before the colon in the last token
                        possible_file = possibility.split(' ')[-1].split(':')[0]
                        name_root = possible_file
                        name_root = name_root.split('/')
                        name_root = name_root[-1]

                        name_root2 = ""
                        for ch in name_root:
                            if ch.isalnum():
                                name_root2 += ch
                            else:
                                hh = hex(ord(ch))[-2:]
                                name_root2 += hh.upper()

                        string_name = "VARIABLE_STRING_" + name_root2

                        if self.ReadVariableString(string_name):
                            break
                        stack_line += 1

                    # We have us a new variable string
                    # Clear out the tables:
                    # Load the new tables
                    self.var_trie = VarTrie()
                    self.line_list = LineList()
                    self.perform_commands = {}

                    if stack_line >= len(stack):
                        # For whatever reasons, we couldn't find any information in
                        # the stack above the "frame" point
                        return False

                    # Pull apart the variable_string
                    variable_lines = self.current_variable_string.split("~")
                    for variable_line in variable_lines:
                        self.line_list.LoadFromLine(variable_line)
                        self.var_trie.LoadFromLine(variable_line)
                        self.LoadPerformFromLine(variable_line)

                    # Put a fencepost at the end of the line_list
                    self.line_list.Insert(self.line_list.EndOfTheWorld, "Ragnarok")

                    # Get the full path for the source file
                    command = "info source"
                    info_source = GdbExecute(currentframe(), command)
                    info_source = info_source.split('\n')
                    for line in info_source:
                        pattern = re.compile(r"Located in (.*)$")
                        match = re.search(pattern, line)
                        if match:
                            self.current_full_path = match.group(1)
                            break
        except:
            self.current_trapped_routine = ""
            self.current_trapped_file = ""
            self.current_trapped_frame = -1
            self.current_trapped_line = 0

        return retval

    def ReadSymbolTableText(self):
        # Go read the frame and stack, and fetch the VariableStringData
        if self.ProcessStack():
            # Figure out what routine we are trapped in, from
            # the line number
            for line in self.line_list.line_list:
                if line[0] > self.current_trapped_line:
                    # We've gotten past self.current_trapped_line, so the prior entry
                    # was the one we wanted
                    break
                # Keep looking
                self.current_trapped_routine = line[1]
        else:
            print("No stack.")

def MachineInterfaceFlat(TabFile, possibilities):
    # This is the machine interface mode for returning the
    # the variables in a flat manner.
    # Our mission is to emulate
    # '-stack-list-variables --simple-values'
    # e.g.,    variables=[{name="b_70",arg="1",type="unsigned char *",value="0x555555759df0 <b_39> \"James     K Lowden    \""}]
    s = "variables=["
    for payload_index in possibilities:
        payload = TabFile.var_trie.storage_list[payload_index]
        payload.FetchVariableData(TabFile.line_list, False)
        if not payload.not_in_context:
            name = ""
            name += payload.display_name
            n = name.find(' ')
            if n > -1:
                name = name[:n] + '-' + name[n + 1:]
            n = name.find(' ')
            if n > -1:
                name = name[:n]

            # Everything after the final slash is the program
            # name, which is redundant and takes up valuable
            # screen real estate.  Clear it out.

            n = name.rfind('/')
            if n > -1:
                name = name[:n]

            body = MachineInterfaceBody(payload.display_body)

            s += '{name="'
            s += name
            s += '",type="char *",value="'
            s += body
            s += '"},'
    if s[-1] == ',':
        s = s[:-1]  # Strip off final comma
    s += ']'
    s = NoNulls(s)
    return s

class HNode():
    def __init__(self):
        self.storage_table_index = -1
        self.Level = 0
        self.BaseName = ""  # Just A of A/B/C
        self.display_body = ""
        self.parent = None
        self.children = []

def TrimComma(s):
    if s and s[-1] == ',':
        s = s[:-1]  # Strip off final comma
    return s

def BuildHString(node):
    s = ""
    if node.Level == 0:
        # This is the root.  We live only for our children
        for child in node.children:
            s += BuildHString(child)
    else:
        if node.Level in (1, 77, 78):
            # Top level variables are treated this way:
            s += r'{name="'
            s += node.BaseName  #  + "/" + str(node.storage_table_index)
            s += r'",value="'
            if len(node.children) == 0:
                # There are no children, which means this is just
                # a simple variable:
                s += r'\"'
                s += MachineInterfaceBody(node.display_body)
                s += r'\"'
                s += r'"'
            else:
                # This is a top-level node with children:
                s += r'{_groupvalue=\"'
                s += MachineInterfaceBody(node.display_body)
                s += r'\",'
                for child in node.children:
                    s += BuildHString(child)
                s = TrimComma(s)
                s += r'}"'
            s += '},'
        else:
            # This is a node whose level is 01-49, 66
            if len(node.children) == 0:
                s += node.BaseName  # + "/" + str(node.storage_table_index)
                s += r'=\"'
                s += MachineInterfaceBody(node.display_body)
                s += r'\",'
            else:
                s += node.BaseName  # + "/" + str(node.storage_table_index)
                s += r'={_groupvalue=\"'
                s += MachineInterfaceBody(node.display_body)
                s += r'\",'
                for child in node.children:
                    s += BuildHString(child)
                s = TrimComma(s)
                s += r'},'
    return s

def MachineInterfaceHeirarchical(TabFile, possibilities):
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
    for payload_index in possibilities:
        payload = TabFile.var_trie.storage_list[payload_index]
        payload.FetchVariableData(TabFile.line_list, False)
        if not payload.not_in_context:
            # print("MIH", payload.Level, payload.Name, payload.display_body)
            # First, if the stack has nephews of ours,
            # let's get them off of the stack:
            while stack[-1].Level > payload.Level:
                stack.pop()

            # Create the new node
            new_node = HNode()
            new_node.storage_table_index = payload_index
            new_node.Level = payload.Level
            split_name = payload.Name.split('/')
            if len(split_name) == 2:
                # If it's just the two pieces, then use the whole name/program
                # pair.  (This is a file record or 01/77, and we might need
                # the whole name to accurately handle a "print var=x request")
                new_node.BaseName = payload.Name
            else:
                # Just use the lowest level.  When necessary, the VSC code will
                # be able to walk up through the containers to get the full name
                new_node.BaseName = split_name[0]

            new_node.display_body = payload.display_body

            if payload.Level in (77, 78):
                stack = [root]
            if stack[-1].Level == payload.Level or payload.Level == 66:
                # The element on the stack is our brother, so
                # so we are going to share parents:
                new_node.parent = stack[-1].parent
            else:  # stack[-1].Level < payload.Level
                # The element on the stack is our parent
                new_node.parent = stack[-1]
            # And now we become the final stack entry
            if payload.Level != 66:  # 66 is just a pain in the patootie.
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

def get_sorted_possibilities(args, tab_file):
    possibilities_1 = []
    possibilities = []
    from_digit = False

    if len(args) >= 1:
        if args[0].find('*') != -1:
            # If the first, presumably single, argument contains an asterisk, then
            # he is asking for a list of variables and values:
            possibilities_1 = tab_file.var_trie.GetAllPossibilities()
        elif args[0].find('?') != -1:
            # If the first, presumably single, argument contains a question mark,
            # then we need to round up the usual suspects.

            # But first, we are going to keep a rogue's gallery of the current list
            # suspects.  The idea here is that sometimes MOVE A TO B is the last line
            # of a PARAGRAPH, so as soon as it is executed control jumps elsewhere and
            # B falls out of context.  Or perhaps the next GDB command is a CONTINUE,
            # rather than NEXT.  By keeping one stage of stickiness, the display of variables
            # will be a bit more like what the user probably expects.

            if len(tab_file.current_full_path) > 0:
                # explicit requested range ... or current value
                if  len(args) >= 2 and args[1].isnumeric():
                    qmrange = int(args[1])
                else:
                    qmrange = tab_file.question_mark_range
                if qmrange < 0:
                    qmrange = 1000000000
                if qmrange == 999:
                    # This is an expedient for simply killing any cprint ? response
                    return_string = ""
                else:
                    return_string = subprocess.check_output(["cobcd-rw",
                                                              tab_file.current_full_path,
                                                              str(tab_file.current_trapped_line),
                                                              str(qmrange)])
                tokens = return_string.split()
                new_dogs = set()
                for token in tokens:
                    token = token.decode("utf-8")
                    new_dogs = new_dogs.union(set(tab_file.var_trie.GetListOfExactPossibilities(token)))
                possibilities_1 = list(tab_file.old_dogs.union(new_dogs))
                tab_file.old_dogs = new_dogs
        elif args[0].isdigit():
            from_digit = True
            # if the first, presumably single, argument is a number, then he is looking
            # for a short-term memory entry.

            # Pick up the 1-based value indexed by the argument:
            index = int(args[0]) - 1
            if index >= 0 and index < len(GV_GlobalVariables.ShortTermMemory):
                # If index is valid, pick up the short-term memory entry
                possibilities_1 = [GV_GlobalVariables.ShortTermMemory[index]]
        else:
            # There are one or more arguments.  Use our clever routine
            # to come up with the possibilities_1:
            possibilities_1 = tab_file.var_trie.GetListOfPossibilities(args)

    # We now have a list of possibilities_1 based on the names of the variables
    # typically, some of them will not be in context.  We wish, at this point
    # to eliminate such variables.  The bad news is that this means we
    # have to fetch all the variables data twice.  But, heck.

    for payload_index in possibilities_1:
        payload = tab_file.var_trie.storage_list[payload_index]
        payload.FetchVariableData(tab_file.line_list, False)
        if not payload.not_in_context:
            possibilities.append(payload_index)

    # What we do now depends on how many possibilities there are
    possibilities.sort()

    return possibilities, from_digit


def MetaPossibilities(args, TabFile, machine_interface_mode, rside, submode):
    possibilities = []
    from_digit = False
    if TabFile.current_trapped_line > 0:
        possibilities, from_digit = get_sorted_possibilities(args, TabFile)

    if len(possibilities) == 1 and machine_interface_mode and not rside and args[0] != '*' and args[0] != '?':
        # Very special case.  This was a `cprint/m something`, which is what happens when
        # the -data-evaluate-expression command is issued by Visual Studio Code.  I considered creating
        # a unique 'print/e ' command, but then I realized that VSC never issues a 'cprint/m something' unless there
        # is also an 'rside'.  So, this code is in response to a `-data-evalute-expression` equivalent command
        payload_index = possibilities[0]
        payload = TabFile.var_trie.storage_list[payload_index]
        payload.FetchVariableData(TabFile.line_list, False)
        s = 'value="' + MachineInterfaceBodyAlt(payload.display_body) + '"'
        print(s)
        possibilities = []
        return possibilities

    if len(possibilities) > 1 or machine_interface_mode:
        # In machine_interface mode, we just return the possibility(ies).  In human
        # mode, if there is more than one, we create a numbered list; the user can later
        # specify a variable by the number
        if machine_interface_mode:
            # Because we are going to be showing the data in a heirarchical way,
            # we want to be sure every sub-level of a 01 Level variable is visible:
            ppp = set(possibilities)
            for i in possibilities:
                tokens = TabFile.var_trie.storage_list[i].Name.split('/')
                var = tokens[-2]
                prog = tokens[-1]
                u = i - 1
                while u >= 0:
                    tokens = TabFile.var_trie.storage_list[u].Name.split('/')
                    if tokens[-2] != var or tokens[-1] != prog:
                        break
                    ppp.add(u)
                    u -= 1
                d = i + 1
                while d < len(TabFile.var_trie.storage_list):
                    tokens = TabFile.var_trie.storage_list[d].Name.split('/')
                    if tokens[-2] != var or tokens[-1] != prog:
                        break
                    ppp.add(d)
                    d += 1

            possibilities = list(ppp)
            possibilities.sort()

            # s = MachineInterfaceFlat(TabFile, possibilities)
            s = MachineInterfaceHeirarchical(TabFile, possibilities)
            print(s)

        else:
            GV_GlobalVariables.ShortTermMemory = []

            # This is human-interactive mode
            # and show them to our user
            line_counter = 1
            for payload_index in possibilities:
                payload = TabFile.var_trie.storage_list[payload_index]
                payload.FetchVariableData(TabFile.line_list, False)
                if not payload.not_in_context:
                    GV_GlobalVariables.ShortTermMemory.append(payload_index)
                    s = "{0:2} : ".format(line_counter)
                    line_counter += 1
                    s += payload.display_name
                    s += " : "

                    body = payload.display_body
                    if submode == 'x' and len(payload.data) <= 16:
                        body = payload.display_body_hex

                    s += body
                    retval = OurPrint(NoNulls(s))
                    if retval == "Quit":
                        break
    if len(possibilities) == 1 and not from_digit:
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

class CPrintWorker():
    """Does most of the non-gdb work of CPrint()"""

    # By creating this worker, we can use a Python debugging package to
    # handle everything that doesn't actually need the gdb object
    def __init__(self):
        self.current_source_file = ""
        self.sticky_display_mode = os.environ.get("CPRINT_V", '0')
        self.question_mark_range = int(os.environ.get("CPRINT_R", '6'))
        self.use_cprint = os.environ.get("CPRINT", "cprint")
        self.machine_interface_mode = False

    def ProcessDisplayMode(self, args, TabFile):
        self.machine_interface_mode = False
        # Make sure the current display mode is the same as the session mode
        self.current_display_mode = self.sticky_display_mode

        self.current_display_submode = ''

        # Now see if he wants to change it
        if len(args) >= 1:
            # if the first argument is /d, turn on our debug flag:
            if args[0] == '/d':
                self.current_display_mode = 'd'
                args.pop(0)
            if args[0] == '/x':
                self.current_display_submode = 'x'
                args.pop(0)
            if args[0] == '/b':
                self.current_display_submode = 'b'
                args.pop(0)
            elif args[0] == '/m':
                self.machine_interface_mode = True
                args.pop(0)
            elif args[0] == '/v':
                self.current_display_mode = '0'
                args.pop(0)
            elif len(args[0]) >= 3 and args[0][0:2] == '/v':
                # This is a display v-code:
                v_code = args[0][2]
                args.pop(0)
                if v_code in "012d":
                    # It's a valid display_mode
                    self.current_display_mode = v_code
                    # if len(args) is down to zero, then any /v code established here
                    # becomes the new session default.  Otherwise, it'll apply only once
                    if len(args) == 0:
                        self.sticky_display_mode = self.current_display_mode
            elif len(args) == 2 and args[0] == '/r' and args[1].lstrip('-').isnumeric():
                # This is a request to change the question_mark_range
                self.question_mark_range = int(args[1])
                print("The 'cprint ?' range has been set to +/-", self.question_mark_range, "lines")
                TabFile.old_dogs = set()
                args = ['?']
            elif len(args[0]) > 2 and args[0][0:2] == '/r':
                # This is a request to change the question_mark_range
                range = args[0][2:]
                if range.lstrip('-').isnumeric():
                    self.question_mark_range = int(range)
                if int(self.question_mark_range) < 0:
                    print("The 'cprint ?' range has been set to the same as 'cprint *'")
                else:
                    print("The 'cprint ?' range has been set to", self.question_mark_range)
                TabFile.old_dogs = set()
                args = ['?']
            elif len(args) == 1 and args[0] == '/r':
                if int(self.question_mark_range) < 0:
                    print("The 'cprint ?' range is currently the same as 'cprint *'")
                else:
                    print("The 'cprint ?' range is currently +/-", self.question_mark_range, "lines")
                args = []

        return args

    def FormattedDisplay(self, payload):
        # see the comments in ProcessDisplayMode()
        body = payload.display_body
        if self.current_display_submode == 'x' and len(payload.data) <= 16:
            body = payload.display_body_hex

        if self.current_display_submode == 'b' and len(payload.data) <= 8:
            # kludge.  Convert the hex to binary:
            body = ""
            index = 2
            while index < len(payload.display_body_hex) and index < 8 * 2 + 2:
                hexchar = int(payload.display_body_hex[index])
                mask = 8
                while mask > 0:
                    if hexchar & mask:
                        body += '1'
                    else:
                        body += '0'
                    mask >>= 1
                index += 1

        if self.current_display_mode == '0':
            print(payload.display_name, " : ", body)
            pass
        elif self.current_display_mode == '1':
            print(body)
            pass
        elif self.current_display_mode == '2':
            print(payload.display_name)
            print(body)
            pass
        else:  # self.current_display_mode ==  'd':
            payload.ShowFields()
            pass

    @staticmethod
    def set_var_value(tab_file, payload, value):
        number_regexp = re.compile(r'[0-9]+|[0-9]+[\.,][0-9]+')
        return_value = 1

        if value.startswith(('"', "'")) and value.endswith(('"', "'")):
            if payload.PutDataAsString(value[1:-1]) == 0:
                return_value = 0
        elif re.match(number_regexp, value):
            if payload.PutDataAsString(value) == 0:
                return_value = 0
        #elif isinstance(value, str):   // doesn't apply to t01-some-var
        else:
            possibilities = MetaPossibilities(value, tab_file, False, False, '')
            if len(possibilities) == 1:
                payload_s = tab_file.var_trie.storage_list[possibilities[0]]
                if payload.PutDataAsVar(payload_s) == 0:
                    return_value = 0
            else:
                print("Variable not found: " + str(value))
                return_value = 0
        #else:
        #    print("No valid method found for " + str (value))
        #    return_value = 0

        return return_value

    def ProcessArguments(self, TabFile, arguments):
        # The command is typically "cprint A B C" or "cprint/d A B C"
        # There is also the possibility of an '=' character, whereby the
        # value after the '=' is assigned to the variable described to the left
        preargs = arguments.split('=', 1)

        rside = None
        if len(preargs) > 1:
            rside = preargs[1].strip()

        # The variable name side is to the left of any '=' character
        args = []
        argsargs = preargs[0].split()
        for arg in argsargs:
            args.append(arg)

        # CHECKME: Do we need setting that here at all and in both directions?
        self.question_mark_range = TabFile.question_mark_range
        args = self.ProcessDisplayMode(args, TabFile)
        TabFile.question_mark_range = self.question_mark_range

        # TODO early exit if we just set the range  -> cp /r 4
        #if len(preargs) >= 1 and preargs[0][0:2] == '/r':
        #    return

        possibilities = MetaPossibilities(args, TabFile, self.machine_interface_mode, rside, self.current_display_submode)
        if len(possibilities) == 1:
            # There is but one possibility
            # Pick up the payload from the list
            payload = TabFile.var_trie.storage_list[possibilities[0]]

            if rside:
                if self.set_var_value(TabFile, payload, rside) == 0:
                    return

                if self.machine_interface_mode:
                    # body = MachineInterfaceBody(payload.display_body)

                    disp_str = payload.display_name + " set to " + payload.display_body
                    print(disp_str)

            if not self.machine_interface_mode:
                # Human-interface:
                # Fetch the data again, just in case he executed some instructions
                # since short-term-memory was established
                payload.FetchVariableData(TabFile.line_list, True)
                # And display the variable according to the current list v-level
                self.FormattedDisplay(payload)
        elif not possibilities:
            if self.machine_interface_mode:
                # just return quietly.  The variable not found message
                # confuses Visual Studio Code
                return
            # Happens for example with R = 0 and cprint on PERFORM line
            if len(args) >= 1 and args[0] == '?':
                print("No variable found in range")
                return
            # TODO print("Variable not found: " + str(value))
            print("Variable not found")
        else:
            if rside:
                print("assignment needs exactly one possible variable")

class CWatchWorker():
    """Does most of the non-gdb work of CWatch()"""

    def __init__(self):
        self.current_display_mode = "0"

    @staticmethod
    def set_watchpoint(payload, wp_type, wp_refmod):
        if wp_refmod:
            valid, wp_startloc, wp_length = get_lenspec_refmod(wp_refmod, payload, "p")
            if valid == 0:
                return
        else:
            wp_startloc = payload.location_of_data
            wp_length = payload.Length

        spec = "*(char(*)"
        spec += '[' + str(wp_length) + '])'
        spec += '(' + hex(wp_startloc) + ')'

        if wp_type == 'WPW':
            break_point = gdb.Breakpoint(spec, gdb.BP_WATCHPOINT, gdb.WP_WRITE)
        elif wp_type == 'WPR':
            break_point = gdb.Breakpoint(spec, gdb.BP_WATCHPOINT, gdb.WP_READ)
        elif wp_type == 'WPRW':
            break_point = gdb.Breakpoint(spec, gdb.BP_WATCHPOINT, gdb.WP_ACCESS)
        else:
            print("Internal error: Watchpoint type not recognized")

    def process_watch_arguments(self, tab_file, arguments):
        wp_type = 'WPW'
        wp_refmod = ""
        wp_arg = ""

        args = arguments.split()
        if not args:
            print("No variable given\n")
            return

        if args[0] == '/r':
            wp_type = 'WPR'
            del args[0]
        elif args[0] == '/w':
            wp_type = 'WPW'
            del args[0]
        elif args[0] == '/rw':
            wp_type = 'WPRW'
            del args[0]
        elif args[0].startswith('/'):
            print("Option not recognized\n")
            return

        if len(args) > 2:
            print("Too much arguments")
            return

        if len(args) == 2:
            wp_arg, wp_refmod = get_refmod(args[1])
            if wp_refmod != "":
                del args[1]
            else:
                print("Argument not recognized")
                return
            wp_arg = args[0]
        else:
            wp_arg, wp_refmod = get_refmod(args[0])

        possibilities = MetaPossibilities(wp_arg, tab_file, False, False, '')

        if len(possibilities) == 1:
            payload_number = possibilities[0]
            # print("Watch Payload number is ", payload_number)

            # Pick up the payload from the list
            payload = tab_file.var_trie.storage_list[payload_number]
            payload.FetchVariableData(tab_file.line_list, False)

            # And go set the Watchpoint
            self.set_watchpoint(payload, wp_type, wp_refmod)
        elif not possibilities:
            print("Variable not found")

class CBreakWorker():

    def __init__(self):
        try:
            self.strtype = basestring
        except NameError:
            self.strtype = str

        self.section_buffer = None
        self.module_buffer = None

    @staticmethod
    def set_breakpoint(spec):
        break_point = gdb.Breakpoint(spec, gdb.BP_BREAKPOINT)
        return break_point

    @staticmethod
    def get_data_from_buffer(buffer):
        return_value = ""

        buf_addr = GetAddressOf("{0}".format(buffer))
        while True:
            char = GetDataAt(buf_addr, 1)
            if char == 0:
                break
            return_value += chr(char)
            buf_addr += 1

        return return_value

    @staticmethod
    def set_condition(break_point, condition):
        break_point.condition = condition

    def get_module_name(self, module):
        return_value = None

        try:
            if self.module_buffer is None:
                self.module_buffer = gdb.execute("call (char*)cob_cache_malloc(100)", False, True).split()[2]
        except gdb.error:
            self.module_buffer = None
            try:
                spec = "{0}_".format(module)
                spec_ref = gdb.parse_and_eval(spec)
                return_value = module
            except:
                return_value = None
        else:
            try:
                command = "call (int)cob_encode_program_id(\"{0}\",{1},100,0)".format(module, self.module_buffer)
                gdb.execute(command, False, True)
                return_value = self.get_data_from_buffer(self.module_buffer)
            except gdb.error as error:
                raise gdb.GdbError("The program is not being run: " + str(error))

        return return_value

    def get_section_name(self, section):
        return_value = None

        try:
            if self.section_buffer is None:
                self.section_buffer = gdb.execute("call (char*)cob_cache_malloc(100)", False, True).split()[2]
        except gdb.error as error:
            self.section_buffer = None
            raise gdb.GdbError("The program is not being run: " + str(error))

        command = "call (int)cob_encode_program_id(\"{0}\",{1},100,1)".format(section, self.section_buffer)

        try:
            gdb.execute(command, False, True)
        except gdb.error as error:
            raise gdb.GdbError("execute failed: " + str(error))

        return_value = self.get_data_from_buffer(self.section_buffer)

        return return_value

    @staticmethod
    def get_var_payload(tab_file, search_var):
        payload = None
        possibilities = []

        tab_file.ReadSymbolTableText()
        if tab_file.current_trapped_line > 0:
            possibilities, from_digit = get_sorted_possibilities(search_var, tab_file)

        if len(possibilities) == 1:
            payload_number = possibilities[0]

            # Pick up the payload from the list
            payload = tab_file.var_trie.storage_list[payload_number]
            payload.FetchVariableData(tab_file.line_list, False)
            if search_var.upper() not in payload.Name.upper():
                payload = None

        return payload

    def set_string_breakpoint(self, line_nr, condition, payload, refmod, diff_str, break_point):
        if condition in ("=", "=i", "==", "==i", "<>", "<>i", "=~"):
            if condition[-1:] == "i":
                cmp_func = "strncasecmp"
            else:
                cmp_func = "strncmp"

            cmp_len = len(diff_str)
            bp_startloc = 1
            if refmod:
                valid, bp_startloc, bp_length = get_lenspec_refmod(refmod, payload, "n")
                if valid == 0:
                    return

                if bp_length > cmp_len:
                    print("Length specifier > compare string ({0} > {1})".format(str(bp_length), str(cmp_len)))
                    return

                cmp_len = bp_length

            location_of_field = payload.get_verified_field_location(DONT_ALLOCATE)
            if not location_of_field:
                return

            if break_point is None:
                if line_nr == "-1":
                    break_point = self.set_breakpoint("")
                else:
                    break_point = self.set_breakpoint(line_nr)

            if condition in ("=", "=i", "==", "==i"):
                self.set_condition(break_point, "(int){0}(((char *)cob_get_field_str_buffered({1}) + {2} - 1), \"{3}\", {4}) == 0".format(cmp_func, hex(location_of_field), bp_startloc, diff_str, cmp_len))
            elif condition in ("<>", "<>i"):
                self.set_condition(break_point, "(int){0}(((char *)cob_get_field_str_buffered({1}) + {2} - 1), \"{3}\", {4}) != 0".format(cmp_func, hex(location_of_field), bp_startloc, diff_str, cmp_len))
            else:
                self.set_condition(break_point, "$_regex(((char *)cob_get_field_str_buffered({0} + {1} - 1), \"{2}\")".format(hex(location_of_field), bp_startloc, diff_str))
        else:
            print("Condition not implemented")

    def set_numeric_breakpoint(self, line_nr, condition, payload, diff_num, break_point):
        if condition in ("==", "<", "<=", ">", ">=", "<>"):
            if break_point is None:
                if line_nr == "-1":
                    break_point = self.set_breakpoint("")
                else:
                    break_point = self.set_breakpoint(line_nr)

            location_of_field = payload.get_verified_field_location(DONT_ALLOCATE)
            if not location_of_field:
                return

            self.set_condition(break_point, "(double)atof((char *)cob_get_field_str_buffered({0})) {1} {2}".format(hex(location_of_field), condition, diff_num))
        else:
            print("Condition not implemented")

    def set_section_breakpoint(self, args):
        mod_sec = args[1].split(":")
        if len(mod_sec) == 1:
            section_name = self.get_section_name(mod_sec[0])
            if section_name is None:
                print("Section not found")
                return
            module_name = ""
        elif len(mod_sec) == 2:
            section_name = self.get_section_name(mod_sec[1])
            if section_name is None:
                print("Section not found")
                return
            module_name = self.get_module_name(mod_sec[0])
            if module_name is None:
                print("Module not found")
                return

        if not module_name:
            spec = "SECTION_{0}".format(section_name)
        else:
            spec = "{0}_:SECTION_{1}".format(module_name, section_name)
        break_point = self.set_breakpoint(spec)

    @staticmethod
    def get_cob_cmp_refloc(refmod, payload, cmp_len, second):
        bp_startloc = 1
        location_of_field = None

        valid, bp_startloc, bp_length = get_lenspec_refmod(refmod, payload, "n")
        if valid == 0:
            return location_of_field, cmp_len

        if bp_length > cmp_len:
            print("Length specifier > compare string ({0} > {1})".format(str(bp_length), str(cmp_len)))
            return location_of_field, cmp_len
        elif bp_length < cmp_len and second == 1:
            print("Different length specifiers {0} <> {1}".format(bp_length, cmp_len))
            return location_of_field, cmp_len

        cmp_len = bp_length

        try:
            command = "call (char*)cob_malloc(sizeof(cob_field))"
            location_of_field = int(gdb.execute(command, False, True).split()[2], 16)

            command = "set var *(cob_field *){0}".format(location_of_field) + " = {"
            command += "{0}, (char *){1}, &{2}".format(cmp_len, hex(payload.location_of_data + bp_startloc - 1), payload.Aname)
            command += "}"
            gdb.execute(command, False, True)
        except gdb.error:
            print("set_cob_cmp_breakpoint: Error getting refmod for {0}".format(payload.Aname))
            location_of_field = None

        return location_of_field, cmp_len

    def set_cob_cmp_breakpoint(self, line_nr, condition, payload_f, refmod_f, payload_s, refmod_s, break_point):
        if condition in ("=", "=="):
            condition_str = "=="
        elif condition == "<>":
            condition_str = "!="
        elif condition in (">=", "<=", ">", "<"):
            condition_str = condition
        else:
            print("Condition not implemented")
            return

        if refmod_f:
            location_of_field, cmp_len = self.get_cob_cmp_refloc(refmod_f, payload_f, payload_s.Length, 0)
        else:
            cmp_len = payload_f.Length
            location_of_field = payload_f.get_verified_field_location(DONT_ALLOCATE)

        if not location_of_field:
            return

        if refmod_s:
            other_field, cmp_len = self.get_cob_cmp_refloc(refmod_s, payload_s, cmp_len, 1)
        else:
            other_field = payload_s.get_verified_field_location(DONT_ALLOCATE)

        if not other_field:
            return

        if break_point is None:
            if line_nr == "-1":
                break_point = self.set_breakpoint("")
            else:
                break_point = self.set_breakpoint(line_nr)
        self.set_condition(break_point, "(int)cob_cmp({0}, {1}) {2} 0".format(hex(location_of_field), hex(other_field), condition_str))

    @staticmethod
    def get_var_data(args):
        var_f = ""
        refmod_f = ""
        var_s = ""
        refmod_s = ""

        if len(args) == 4:
            var_f, refmod_f = get_refmod(args[1])
            condition = args[2]
            var_s, refmod_s = get_refmod(args[3])
        elif len(args) == 5:
            #  0  1     2    3     4
            # 52 var1 (1:2)  =    var2
            # 52 var1   =   var2 (1:2)
            if args[2].find('(') != -1:
                var_f, refmod_f = get_refmod(args[2])
                var_f = args[1]
                condition = args[3]
                var_s, refmod_s = get_refmod(args[4])
            elif args[4].find('(') != -1:
                var_f, refmod_f = get_refmod(args[1])
                condition = args[2]
                var_s, refmod_s = get_refmod(args[4])
                var_s = args[3]
            else:
                print("Statement not recognized")
                return var_f, refmod_f, var_s, refmod_s, condition
        elif len(args) == 6:
            var_f, refmod_f = get_refmod(args[2])
            var_f = args[1]
            var_s, refmod_s = get_refmod(args[5])
            var_s = args[4]
            condition = args[3]

        return var_f, refmod_f, var_s, refmod_s, condition

    @staticmethod
    def split_arguments(arguments):
        match_chars = ("=", "==", "=~", "~", "<", "<=", ">", ">=", "<>", "i")
        arg_list = list(filter(lambda e: e not in ('', ' ', '"', '\''), re.split('([^a-zA-Z0-9_:-])', arguments)))

        i = 0
        combine = 0
        while i < len(arg_list) - 1:
            if arg_list[i] in match_chars and arg_list[i + 1] in match_chars:
                arg_list[i] += arg_list.pop(i + 1)
            elif arg_list[i] == "(":
                arg_list[i] += arg_list.pop(i + 1)
                combine = 1
            elif arg_list[i + 1] == ")" or combine == 1:
                if arg_list[i + 1] == ")":
                    combine = 0
                arg_list[i] += arg_list.pop(i + 1)
            else:
                i += 1

        return arg_list

    def set_cond_break(self, tab_file, args, break_point, is_lit):
        var_f = ""
        refmod_f = ""
        var_s = ""
        refmod_s = ""
        payload_s = None

        var_f, refmod_f, var_s, refmod_s, condition = self.get_var_data(args)

        payload_f = self.get_var_payload(tab_file, var_f)
        if payload_f is None:
            print("Variable {0} not found".format(var_f))
            return

        if is_lit == 0 and not var_s.isdigit():
            payload_s = self.get_var_payload(tab_file, var_s)
            if payload_s is None:
                print("Variable {0} not found".format(var_s))
                return

        if payload_s is not None:
            self.set_cob_cmp_breakpoint(args[0], condition, payload_f, refmod_f, payload_s, refmod_s, break_point)
        else:
            if payload_f.AttrType in (33, 34, 35):
                self.set_string_breakpoint(args[0], condition, payload_f, refmod_f, var_s, break_point)
            elif payload_f.AttrType in (16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27) and var_s.isdigit():
                self.set_numeric_breakpoint(args[0], condition, payload_f, var_s, break_point)
            else:
                print("Not yet implemented")
                return

    def process_break_arguments(self, tab_file, arguments):
        is_lit = 0
        if arguments and arguments[-1] in ('"', '\''):
            is_lit = 1

        if arguments:
            args = self.split_arguments(arguments)
        else:
            break_point = self.set_breakpoint("")
            return

        if len(args) == 1 and args[0].isdigit():
            break_point = self.set_breakpoint(args[0])
        elif len(args) == 1 and isinstance(args[0], self.strtype):
            module_name = self.get_module_name(args[0])
            if module_name is None:
                print("Module not found")
            else:
                break_point = self.set_breakpoint("{0}_:ENTRY_{0}".format(module_name))
        elif len(args) == 2 and args[0] == "section" and isinstance(args[1], self.strtype):
            self.set_section_breakpoint(args)
        elif len(args) >= 4 and len(args) <= 7:
            if len(args) >= 4 and not args[0].isdigit():
                args.insert(0, '-1')
            elif len(args) == 4:
                print('Breakpoint parameters not recognized')
                return

            if args[1] != "if":
                print('Second argument should be "if"')
                return

            del args[1]

            self.set_cond_break(tab_file, args, None, is_lit)
        else:
            print('Breakpoint parameters not recognized')
            return

    @staticmethod
    def find_breakpoint(bp_nr):
        return_breakpoint = None

        for gdb_breakpoint in gdb.breakpoints():
            if gdb_breakpoint.number == int(bp_nr):
                return_breakpoint = gdb_breakpoint
                break

        if return_breakpoint is None:
            print("No breakpoint number {0}.format(bp_nr)")

        return return_breakpoint

    def process_condition_arguments(self, tab_file, arguments):
        is_lit = 0
        if arguments[-1] in ('"', '\''):
            is_lit = 1

        args = self.split_arguments(arguments)
        if not args[0].isdigit():
            print('First argument is not a breakpoint number')
            return

        break_point = self.find_breakpoint(args[0])
        if break_point is None:
            return

        if len(args) >= 3 and len(args) <= 5:
            self.set_cond_break(tab_file, args, break_point, is_lit)
        else:
            print('Breakpoint parameters not recognized')
            return


CBLDBG_TABFILE = TabFileInformation()
CBLDBG_CBREAK_WORKER = CBreakWorker()

class CPrint(gdb.Command):
    """Prints COBOL-NAMED variables when debugging cobst processed object files.

    COBOL print command usage:
    cprint <arg>        shows <arg> using session-default formatting
    cprint/v<n> <arg>   displays <arg> with a one-time formatting override
    cprint/v0           name and variable on one line (this is the default)
    cprint/v1           just the variable
    cprint/v2           two-line display with name, then variable
    cprint/x            show numeric value in hexadecima
    cprint/b            show numeric value in binar
    cprint/d            show expanded informatio
    cprint/v<n>         with no arguments sets the session formattin
    cprint/m            generate output compatible with '-stack-list-variables --simple-values
    cprint *            print all variables that are currently in context
    cprint ? [R]        print all variables that are currently in context within R lines of current line,
                        if R is not specified use the range currently setup
    cprint/r N          set the range for 'cprint ?' to plus/minus R (the default is 6; -1 means 'all'
    cprint VARIABLE="STRING"    set variable to string
    cprint VARIABLE='STRING'    set variable to string
    cprint VARIABLE=NUMERIC     set variable to numeric
    cprint VARIABLE=VARIBALE    set variable to variable

    The default v-level can be set with the environment variable CPRINT_V=n
    The default r-level can be set with the environment variable CPRINT_R=n
    The 'cprint' command can be changed to 'anything' with the environment variable CPRINT=anything"""

    def __init__(self):
        self.worker = CPrintWorker()
        self.tab_file = CBLDBG_TABFILE
        self.tab_file.question_mark_range = self.worker.question_mark_range

        print('Registering CPrint as ' + self.worker.use_cprint + ' ' + COBCD_VERSION)
        super(CPrint, self).__init__(self.worker.use_cprint, gdb.COMMAND_DATA)

    def invoke(self, arguments, from_tty):
        # pick up the name of the current source file
        try:
            # Check if this is a special case.  (Enough of these will
            # mean putting this in a separate function.)
            tokens = arguments.split()
            if len(tokens) == 3 and tokens[0] == "set" and tokens[1] == "repeat" and tokens[2].isdigit():
                GV_GlobalVariables.repeat_count = int(tokens[2])
                if GV_GlobalVariables.repeat_count < 1:
                    GV_GlobalVariables.repeat_count = 1000000
                return

            self.tab_file.ReadSymbolTableText()
            self.worker.ProcessArguments(self.tab_file, arguments)
        except:
            traceback.print_exc()

class CWatch(gdb.Command):
    """Used for creating COBOL-NAMED watchpoints when debugging cobst processed object files.

Set a watchpoint for an expression.
Usage: cwatch [/r|/w|/rw] VARIABLE [(start:length)]
A watchpoint stops execution of your program whenever the value of an expression changes.

    cwatch VARIABLE      set a (write) watch on this variable
    cwatch/w VARIABLE    set a write watch on this variable
    cwatch/r VARIABLE    set a read watch on this variable
    cwatch/rw VARIABLE   set a read/write watch on this variable

    if (start:length) is given the VARIABLE is watched with this startpoint and length"""

    def __init__(self):
        super(CWatch, self).__init__("cwatch", gdb.COMMAND_BREAKPOINTS)
        print('Registering CWatch')
        self.worker = CWatchWorker()
        self.tab_file = CBLDBG_TABFILE

    def invoke(self, arguments, from_tty):
        self.dont_repeat()
        try:
            self.tab_file.ReadSymbolTableText()
            self.worker.process_watch_arguments(self.tab_file, arguments)
        except:
            traceback.print_exc()

class CBreak(gdb.Command):
    """Used for creating COBOL-NAMED breakpoints when debugging cobst processed object files.

Set a breakpoint for an expression.
Usage: cbreak
A breakpoint stops execution of your program whenever the expression matches

    cbreak                                       set a breakpoint on the current line
    cbreak MODULE                                set a breakpoint on MODULE entry
    cbreak section SECTON                        set a breakpoint on SECTION entry
    cbreak section MODULE:SECTON                 set a breakpoint on SECTION entry in MODULE
    cbreak LINENR                                set a breakpoint on line number (LINENR)
    cbreak LINENR if VARIABLE =[i] MSTRING       set a breakpoint on line number (LINENR)
                                                 for string VARIABLE equals MSTRING
    cbreak LINENR if VARIABLE <>[i] MSTRING      set a breakpoint on line number (LINENR)
                                                 for string VARIABLE not equal MSTRING
    cbreak LINENR if VARIABLE =~ REGEXP          set a breakpoint on line number (LINENR)
                                                 for string VARIABLE when matching REGEXP
    cbreak LINENR if NUMVAR condition VALUE      set a breakpoint on line number (LINENR)
                                                 for numeric VARIABLE condition and value
                                                 where condition is == < <= > >= <>
    cbreak LINENR if VARIABLE condition VARIABLE set a breakpoint on line numer (LINENR)
                                                 for variable condition and second variable
                                                 where condition is = <> >= <= > <

    if (start:length) is given the VARIABLE is checked with this startpoint and length

    Examples:
    cbreak 28 VARIABLE = "breakstring"
    cbreak 28 VARIABLE (2:3) = "breakstring"
    cbreak 28 VARIABLE =i "breakstring"          option i for ignore case
    cbreak 28 VARIABLE =~ ^break*

    cbreak 29 VARIABLE > 12"""

    def __init__(self):
        self.worker = CBLDBG_CBREAK_WORKER
        self.tab_file = CBLDBG_TABFILE

        print('Registering CBreak')
        super(CBreak, self).__init__("cbreak", gdb.COMMAND_BREAKPOINTS)

    def invoke(self, arguments, from_tty):
        self.dont_repeat()
        self.worker.process_break_arguments(self.tab_file, arguments)


class CCondition(gdb.Command):
    """Specify breakpoint number N to break only if COND is true.

Usage is `ccondition N COND', where N is an integer and COND is a COBOL
expression to be evaluated whenever breakpoint N is reached.
Do "help cbreak" for info on COBOL expressions"""

    def __init__(self):
        self.worker = CBLDBG_CBREAK_WORKER
        self.tab_file = CBLDBG_TABFILE

        print('Registering CCondition')
        super(CCondition, self).__init__("ccondition", gdb.COMMAND_BREAKPOINTS)

    def invoke(self, arguments, from_tty):
        self.dont_repeat()
        self.worker.process_condition_arguments(self.tab_file, arguments)


class CStart(gdb.Command):
    """(Re-)start the debugged program stopping at the beginning of the first COBOL statement.
Any arguments after the cstart command are passed to the executable.  If present,
they override any that might have been passed to GDB with --args."""

    def __init__(self):
        self.tab_file = CBLDBG_TABFILE

        print('Registering CStart')
        super(CStart, self).__init__("cstart", gdb.COMMAND_USER)

    def invoke(self, arguments, from_tty):
        self.dont_repeat()
        try:
            command = "info files"
            info_files = gdb.execute(command, False, True).split('\n')
            line1 = info_files[0]
            line1 = line1.replace('\\','/') # In case of Windows
            main_program = line1.split('/')[-1].split('"')[0]
            if main_program == 'cobcrun':
                command = "show args"
                args = gdb.execute(command, False, True).split('"')[1].split()
                cobol_program = args[0]
                args = ' '.join(args)
                # Arguments passed by CSTART override any that came from the command line:
                arguments = arguments or args
            else:
                cobol_program = main_program

            # If there is no cobol program, just run
            if cobol_program:
                # Turn hyphens into doubled underscores:
                entry_point = cobol_program.replace("-","__") + "_"

                # Learn the current breakpoint pending state
                breakpoint_pending = gdb.execute("show breakpoint pending", False, True).split()[-1].replace('.','')

                # Turn breakpoint pending on, because we might want to set a pending
                # breakpoint if we are launching with cobcrun
                command = "set breakpoint pending on"
                gdb.execute(command, False, True)

                # set a temporary breakpoint there
                command = "tbreak " + entry_point
                gdb.execute(command, False, True)

                # Restore breakpoint pending to the original state:
                command = "set breakpoint pending " + breakpoint_pending
                gdb.execute(command, False, True)

            # And proceed to run the program:
            command = "run " + arguments
            gdb.execute(command, True, False)

        except gdb.error:
            print("cstart failed: " + command)

class CNext(gdb.Command):
    """The cnext command behaves like GDB 'next', but it is smart enough to
attempt to pass over PERFORMs.  It optionally takes a count parameter.

It does the best it can, but occasionally it behaves a little strangely when a
compiler optimization removes the JUMP TO PARAGRAPH and JUMP BACK FROM PARAGRAPH
logic and simply puts the paragraph in-line, which CNEXT doesn't detect."""

    def __init__(self):
        super(CNext, self).__init__("cnext", gdb.COMMAND_BREAKPOINTS)
        print('Registering CNext')
        self.tab_file = CBLDBG_TABFILE

    def invoke(self, arguments, from_tty):
        try:
            self.tab_file.ReadSymbolTableText()
            if arguments:
                ncount = int(arguments)
            else:
                ncount = 1
            while ncount > 0:
                command = "info stack"
                key = gdb.execute(command, False, True).split('\n')[0].split()[-1]
                if key in self.tab_file.perform_commands:
                    command = "until " + self.tab_file.perform_commands[key]
                    gdb.execute(command, True, False)

                    # If we went to a specific line location, then the UNTIL leaves
                    # us in a nice place.  If we target a local address, then
                    # we have stopped at a l_xx: target in the C code that shows
                    # up as being before where the user expects to be.  So,
                    # in that case, we issue an extra step to get GDB and the
                    # COBOL source code line in sync.

                    if self.tab_file.perform_commands[key].find(':') == -1:
                        command = "step"
                        gdb.execute(command, True, False)

                else:
                    command = "next"
                    gdb.execute(command, True, False)
                ncount -= 1
        except:
            #traceback.print_exc()
            print("No stack.")

def exit_handler(event):
    GV_GlobalVariables.DestroyBuffers()
    pass

def stop_handler(event):
    pass

gdb.events.exited.connect(exit_handler)
gdb.events.stop.connect(stop_handler)

CPrint()
CWatch()
CBreak()
CCondition()
CNext()
CStart()
