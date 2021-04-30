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
# os.environ.get('CPRINT_D', '0')  When non-zero, causes verbose output
# os.environ.get('CPRINT_V', '0')  Default formatting p/v N value
# os.environ.get('CPRINT_R', '6')) Default p/? range
# os.environ.get('CPRINT', '<cmd>'))   When non-blank, registers "cmd" instead of "cprint" as the command

from __future__ import print_function

import re
import sys
import platform
import os
from os import path
import traceback
import subprocess
from inspect import currentframe, getframeinfo
from struct import calcsize
import copy
import gdb

# Version to be printed upon request - Don't mess with it!
# It is updated by a Linux bash script, the text "Version x.x" has to appear in square brackets.
COBCD_VERSION = '[Version 4.26-dev]'

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

#  regexp's for ZERO and SPACE handling
R_ALL_ZERO_SPACE = re.compile(r'ALL|ZERO(S|ES)?|SPACE(S)?', re.IGNORECASE)
R_ZERO_SPACE = re.compile(r'ZERO(S|ES)?|SPACE(S)?', re.IGNORECASE)

R_SPACE = re.compile(r'SPACE(S)?', re.IGNORECASE)
R_ZERO = re.compile(r'ZERO(S|ES)?', re.IGNORECASE)
R_ALLSPACE = re.compile(r'ALL[ ]*SPACE(S)?', re.IGNORECASE)
R_ALLZERO = re.compile(r'ALL[ ]*ZERO(S|ES)?', re.IGNORECASE)

# executable available flags
READELF_AVAILABLE = True
COBCDRW_AVAILABLE = True

# Note: Replace "readelf" with path to binary if it is not in your PATH.
READELF_BINARY = os.getenv('READELF', 'readelf')
COBCDRW_BINARY = os.getenv('COBCDRW', 'cobcd-rw')
COBCRUN_NAME = os.getenv('COBCRUN', 'cobcrun') # _only_ the name

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

def cob_malloc(size):
    # This is the Python-side wrapper for cob_malloc calls.
    # We use the C-side wrapper that cobcd-st put into the generated .C
    command = "call (char *){0}({1})".format(GV_ModuleState.cob_malloc, size)
    hexback = gdb.execute(command, False, True).split()[2]
    retval = int(hexback, 16)
    return retval

def cob_cache_malloc(size):
    command = "call (char *){0}({1})".format(GV_ModuleState.cob_cache_malloc, size)
    hexback = gdb.execute(command, False, True).split()[2]
    retval = int(hexback, 16)
    return retval

def cob_free(location):
    # Wrapper for cob_free
    command = "call (void){0}({1})".format(GV_ModuleState.cob_free, hex(location))
    gdb.execute(command, False, True)

def cob_move(source, destination):
    command = "call (void){0}((cob_field *){1}, (cob_field *){2})".format(GV_ModuleState.cob_move,
                                                                          hex(source),
                                                                          hex(destination))
    gdb.execute(command, False, True)

def cob_get_field_str(field, buff, size):
    # Cast the return to void so that GDB doesn't try to interpret the returned
    # value as a string pointer, which can cause minor nightmares when a COBOL group variable
    # contains non-string binary data.
    command = "call (void){0}((cob_field *){1}, (char *){2}, {3})".format(GV_ModuleState.cob_get_field_str,
                                                                                  hex(field),
                                                                                  hex(buff),
                                                                                  hex(size))
    gdb.execute(command, False, True)

def cob_put_field_str(field, buff):
    command = "call (int){0}((cob_field *){1}, (char *){2})".format(GV_ModuleState.cob_put_field_str,
                                                                    hex(field),
                                                                    hex(buff))
    gdb.execute(command, False, True)

def cob_encode_program_id(name, buff, size, fold):
    command = "call (int){0}(\"{1}\", {2}, {3}, {4})".format(GV_ModuleState.cob_encode_program_id,
                                                             name,
                                                             hex(buff),
                                                             size,
                                                             fold)
    gdb.execute(command, False, True)

class GlobalVariables():
    def __init__(self):
        # Establish the version of GDB
        match = re.search(r'[^0-9]*([0-9]+)[.]([0-9]+)[.]([0-9]+).*$', gdb.VERSION)
        if match:
            self.gdb_version_major = int(match.group(1))
            self.gdb_version_minor = int(match.group(2))
            self.gdb_version_patch = int(match.group(3))
        else:
            match = re.search(r'^[^0-9]*([0-9]+)[.]([0-9]+).*$', gdb.VERSION)
            if match:
                self.gdb_version_major = int(match.group(1))
                self.gdb_version_minor = int(match.group(2))
                self.gdb_version_patch = 0
            else:
                self.gdb_version_major = 0
                self.gdb_version_minor = 0
                self.gdb_version_patch = 0

        self.gdb_8dot3_plus = True
        if self.gdb_version_major < 8 or (self.gdb_version_major == 8 and self.gdb_version_minor < 3):
            self.gdb_8dot3_plus = False

        if not self.gdb_8dot3_plus:
            print("\n !! warning: add-symbol-file-cobol is not available with GDB-{0}.{1} (GDB 8.3+ is required)\n".format(
                                                                    self.gdb_version_major,
                                                                    self.gdb_version_minor)
                                                                    )

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
        self.cprint_a0 = None

        # These are used in PutDataAsVar
        self.VarLeft = None
        self.VarLeftIndex = None
        self.VarRight = None

        # This flag means that every variable will need to be read again.
        # It gets set on GDB.events.stop
        self.IsDirty = True

    def AllocateBuffers(self):
        if not self.cprint_buffer:
            try:
                self.cprint_buffer = cob_malloc(PRINT_BUFFER_SIZE)
                self.cprint_f0 = cob_malloc("sizeof(cob_field)")
                self.cprint_a0 = cob_malloc("sizeof(cob_field_attr)")
            except:
                traceback.print_exc()
                self.cprint_buffer = None
                self.cprint_f0 = None

            if not self.cprint_buffer:
                print("Something is very wrong in GlobalVariables.AllocateBuffers()")
                sys.exit(1)

    def DestroyBuffers(self):
        self.cprint_buffer = None
        self.cprint_f0 = None
        self.cprint_a0 = None

GV_GlobalVariables = GlobalVariables()

def NoNulls(s):
    ss = ""
    for ch in s:
        if ord(ch) < 32:
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

def WithRepeatCount(s):
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

def get_left_right_args(args):
    left_side = []
    right_side = []
    condition = ""

    # This can be an assignment, so check for it.
    leftright = re.split('(![=]*|=[=~<>]*[i]*|~|<[=>]*[i]*|>[=]*)', args)

    # Pick up the left and right sides, and turn them into
    # "normalized" COBOL variable names.
    if len(leftright) > 0:
        if leftright[0].startswith ("if"):
            left_side = TokenizeCanonicalName(leftright[0][2:].strip())
            left_side.insert(0, "if")
        else:
            left_side = TokenizeCanonicalName(leftright[0])

    if len(leftright) > 2 and leftright[2] != '':
        # There is a rightside.
        right_side = leftright[2].strip()
        if right_side[0] in ('"', "'"):  # I do love that construction
            if len(right_side) < 2 or right_side[-1] != right_side[0]:
                print("A syntax error in >>{0}<<.".format(right_side))
                left_side = []
                right_side = []
                return left_side, left_side, ""
            right_side = [right_side]
        elif right_side[0:2].lower().replace('"', "'") == "x'":
            right_side = FromHexAlphaString(right_side)
            right_side = [right_side]
        elif right_side[0:2].lower().replace('"', "'") == "h'":
            right_side = FromHexLiteralNumber(right_side)
            right_side = [right_side]
        else:
            # This is not a quoted string, so canonicalize it.
            right_side = TokenizeCanonicalName(leftright[2])

        condition = leftright[1]
    elif len(leftright) > 2 and leftright[2] == '':
        print("No rightside specified.")
        left_side = []
        right_side = []
        return left_side, left_side, ""

    return left_side, right_side, condition

def PreprocessSubscriptsAndRefmods(subscripts):
    # We know this came from inside parentheses.  Although the
    # the straight CPRINT treats "ONE TWO THREE" as "ONE OF TWO OF THREE",
    # we do not.  The processing after this routine expects commas for
    # subscripts and colons for refmods.  Commas, spaces, and semicolons
    # all get treated the same.
    #
    # The logic below turns ONE TWO THREE into ONE, TWO, THREE
    # and it turns ONE OF TWO THREE into ONE TWO,THREE

    separator = ','
    if subscripts.find(':') > -1:
        # We are doing a refmod.  There won't be a separator in
        # a properly-formed refmod.
        separator = ''

    subscripts = subscripts.upper()
    subscripts = subscripts.replace(';', ' ')
    subscripts = subscripts.replace(',', ' ')
    subscripts = subscripts.replace('/', " IN ")
    subscripts = subscripts.replace('.', " IN ")
    subscripts = subscripts.replace(" OF ", " IN ")
    tokens = subscripts.split()
    i = 1
    while i < len(tokens):
        if tokens[i] != "IN" and tokens[i - 1] != "IN":
            tokens[i - 1] += separator
        i += 1
    subscripts = ' '.join(tokens)
    subscripts = subscripts.replace(" IN ", " ")
    return subscripts

def TokenizeCanonicalName(desired):
    # We expect a string, and we return a list of strings.

    # We allow "A of B in C", where [of|in] is not case sensitive
    # We also allow A.B.C, A/B/C, and "A B C"

    # We assume that anything after a '(' is subscripts and/or refmods.  We
    # peel them off here, and tack them onto the final element of the list
    # of strings for later processing

    suffix = ""
    nparen = desired.find('(')
    if nparen > -1:
        suffix = desired[nparen:]
        desired = desired[0:nparen]

        sq_count = suffix.count("'")
        if sq_count > 0 and sq_count % 2 != 0:
            raise gdb.GdbError("Unmatched single quote.")

    sq_count = desired.count("'")
    if sq_count > 0 and sq_count % 2 != 0:
        raise gdb.GdbError("Unmatched single quote.")

    if desired[0] == "'":
        # This can happen as a result of TAB completion:
        desired = desired[1:]
    if desired[-1] == "'":
        # This can happen as a result of TAB completion:
        desired = desired[:-1]

    # turn IN and OF into spaces
    pattern = re.compile(r"^(.*) (in|In|iN|IN|of|oF|Of|OF) (.*)$")
    while True:
        match = re.search(pattern, desired)
        if not match:
            break
        else:
            desired = match.group(1) + " " + match.group(3)

    desired = desired.replace(".", " ").replace("/", " ").strip()
    desired += suffix
    retval = desired.split()
    return retval

def atoi(s):
    # This isn't very bright.  But in Dubner's opinion, it doesn't need to be.
    # It ignores a leading double quote; it accepts a leading '-' as a minus sign,
    # and it converts ASCII digits to a decimal number until it hits
    # something that isn't a digit
    #
    retval = 0
    sign = 1
    i = 0
    while i < len(s):
        ch = s[i]
        i += 1
        if ch == '"':
            if i > 1:
                # An embedded quote ends the conversion
                break
            # The first character is a double-quote.  Ignore it
            continue
        if ch == '-':
            if retval or sign == -1:
                # An embedded hyphen ends the conversion
                break
            # A minus sign while retval is zero is legitimate.
            # Yes, this means "000-0003" will be interpreted as minus 3.
            # I don't care.
            sign = -1
            continue
        n = ord(ch) - ord('0')
        if n < 0 or n > 9:
            # A non-digit ends the conversion
            break
        # This is a digit 0 through 9, so accumulate it into the result
        retval *= 10
        retval += n
    return retval * sign

def IsInteger(s):
    # This routine is designed to provide an indication of whether the
    # input string is nothing but digits, and return either None or the
    # relevant number.  It's intended for use where the input might be
    # an integer literal, or might be a cobol variable.

    # I don't want to use the Python isdigit(), which goes False on
    # negative numbers.

    # And I have to be careful with my atoi routine, because it will
    # return 10 for "10-DOWNING-STREET", which is a perfectly valid
    # COBOL identifier.

    re_integer = re.compile(r'^[-]?[0-9]+$')
    found = re_integer.match(s)
    if found:
        return True
    return False

def IsNumber(s):
    # positive or negative number, with or without decimal point
    re_integer = re.compile(r'^[-]?[0-9]+$|^([-]?[0-9]+)[.][0-9]*$|^([-]?[0-9]*)[.][0-9]+$')
    found = re_integer.match(s)
    if found:
        return True
    return False

def IntegerOrVariable(s):
    # Let's see if the string s is a simple integer
    if IsInteger(s):
        return atoi(s), False  # False means no limit overrun

    # It is not.  Let's see if it represents a unique in-context COBOL variable

    possible_name = TokenizeCanonicalName(s)
    possibilities, limit_reached = get_sorted_possibilities(possible_name, 1)
    if not possibilities:
        # It's not an integer.  But there are no candidates in the variable
        # list that uniquely match.  We return None, and it is up to the caller
        # to take appropriate action.
        return None, limit_reached

    # We know that there is but one possibility:
    payload = GV_ModuleInformation.var_trie.storage_list[possibilities[0]]
    return payload.NumericValue(), False  # False means no limit overrun

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

        # When processing a line like "05 T-ENTRIES OCCURS 90 INDEXED BY TAB-ADR-IND."
        # it turns out that TAB-ADR-IND shows up the line *before* the 05 T-ENTRIES line
        # in the generated C code.  In order to make the name heirachy work properly,
        # I am stashing that line here until the OCCURS line (presumably the next line)
        # is seen.
        self.stashed_line = None

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

                if tokens[self.IndexType] in ('F', 'X', 'W', "WB", 'L', 'I'):
                    # We are working with a variable.  We need to erode the
                    # stack of names down to the variable who is our parent.

                    if tokens[self.IndexLevel] == "":
                        tokens[self.IndexLevel] = '0'
                    our_level = int(tokens[self.IndexLevel])
                    if our_level in (77, 78):
                        our_level = 1
                    if our_level > 0:  # When our_level is zero, it means we are a table index,
                                       # so we don't touch the stacks.
                                       # We want the element above us to be our parent.
                        while self.StackOfLevels[-1] >= our_level or self.StackOfLevels[-1] == 0:
                            # The guy above us is either our brother(==), or a nephew(>),
                            # or he's our crazy INDEXED inlaw (zero) that nobody wants as a parent:
                            self.StackOfNames.pop(-1)
                            self.StackOfLevels.pop(-1)

                    # There is a caveat:
                    # When the GnuCOBOL compiler encounters:
                    #
                    #      05 T-ENTRIES OCCURS 90 INDEXED BY TAB-ADR-IND.
                    #
                    # The variables generated look like this:

#  /* cob_dump_field_ext ( 0, "TAB-ADR-IND", COB_SET_FLD(f0, 4, (cob_u8_t *)&b_40, &a_2), 0, 0); */
#  {
#    /* int max_1 = 90; */
#    {
#      /* cob_dump_field_ext ( 5, "T-ENTRIES", COB_SET_FLD(f0, 1037, b_36 + 10, &a_4), 0, 1, i_1, 1037UL); OCCURS 1 90 */
                    #
                    # We need to somehow associate  TAB-ADR-IND with T-ENTRIES
                    #
                    # I am going to do this by stashing the LEVEL 00 line and not processing
                    # it until right after the OCCURS line
                    #

                    if our_level == 0 and self.stashed_line == None:
                        # This is a LEVEL 00 indexed variable, and we aren't processing it yet.
                        self.stashed_line = variable_line
                        return

                    # and now we put ourselves into the position of
                    # most favored child
                    name_element = tokens[self.IndexName]
                    self.StackOfNames.append(name_element)
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

                    # We need to find our parent.  We will ignore the accursed
                    # level 66, and anything above.

                    # For now we are ignoring the possibility that a level 01
                    # variable might have an FD parent.

                    if payload.Level > 1 and payload.Level <= 49:
                        # We start looking at the variable in the list right where we
                        # will go.
                        i = len(self.storage_list) - 1
                        while i >= 0:
                            candidate_level = self.storage_list[i].Level
                            if candidate_level == payload.Level:
                                # Our older relative has the same level we do.  That means we
                                # share a parent.  That value might be None, but that's okay.
                                payload.index_of_parent = self.storage_list[i].index_of_parent
                                break
                            elif candidate_level > 0 and candidate_level < payload.Level:
                                # Because it is smaller than we are, this has to be our parent:
                                # Unless, of course, it is a level 00 INDEXED BY variable, in which
                                # case it can't be anybody's parent.
                                payload.index_of_parent = i
                                break
                            else:  # candidate_level > payload.Level:
                                   # This has to be a child of an older sibling.  They can't be
                                   # our parent, but we keep looking until we find that older sibling.
                                pass
                            i -= 1
                    # print("Loading", payload.Level, payload.Name, payload.index_of_parent)
                    payload.payload_index = len(self.storage_list)
                    self.storage_list.append(payload)
                    self.AddToTrie(self.storage_trie,
                                   reversed,
                                   payload.payload_index)
                    self.program_ids.add(payload.Program)

                    # We just added a line.  Was it an OCCURS line?
                    if payload.OccursMax:
                        # It was an OCCURS line.
                        # Do we have a stashed LEVEL 00 INDEXED BY line?
                        if self.stashed_line:
                            # We do.  It's time to process it:
                            # (Recursion is a beautiful thing)
                            self.LoadFromLine(self.stashed_line)
                            self.stashed_line = None
        except:
            traceback.print_exc()
            sys.exit(1)

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
            # print("DEBUG boy", boy)
            # print("DEBUG trie.children.keys", trie.children.keys())
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

    def GetListOfPossibilities(self, cobol_variable_name):
        # We know that cobol_variable_name is a list of tokens that has already
        # been through TokenizeCanonicalName
        # We were handed a bunch of pieces.  Let's
        # glue them back together into a single space-separated string:

        # TODO: also handle GDB escaping of symbols here:
        recombined = " ".join(cobol_variable_name).upper()

        # This is the return value:
        paths = []

        # There is the possibility array subscripts (contained in parentheses,
        # and not having any colons, and/or refmods (two values, in parentheses,
        # separated by a colon.)

        subscripts = None
        refmod     = None

        while True:
            n1 = recombined.find('(')
            n2 = recombined.find(')')
            if n1 == -1 and n2 == -1:
                # No more paired parentheses
                break
            if n1 != -1 and n2 < n1 or n1 == -1 and n1 != -1:
                # The parentheses are either mismatched, or in the wrong order
                print("A syntax error in >>{0}<<".format('/'.join(cobol_variable_name)))
                return paths

            # We have a pair of parenthesis.  Extract them and their contents:
            parenthesized = recombined[n1:n2+1]

            # and remove them from the recombined string:
            recombined = recombined[0:n1] + ' ' + recombined[n2+1:]

            if parenthesized.find(':') != -1:
                # This must be a refmod.
                if refmod:
                    # There can be only one refmod
                    print("Too many 'refmods' in >>{0}<<".format('/'.join(cobol_variable_name)))
                    return paths
                refmod = parenthesized
            else:
                # This must be a subscripts specification.
                if subscripts:
                    # There can be only one subscripts specification
                    print("Too many table references in >>{0}<<".format('/'.join(cobol_variable_name)))
                    return paths
                subscripts = parenthesized

        # At this point, we have set aside any table subscripts or refmods from the cobol_variable_name list

        name_tokens = recombined.split()

        # Wander through the forest, looking for possible paths:

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
                    # Make sure that there is no subscripts or refmod for this variable:
                    self.storage_list[index].Subscripts = None
                    self.storage_list[index].Refmod = None
                    # And add it to the set
                    set1.add(index)

            # Turn the set back into a list:
            paths = list(set1)

        # If, and only if, the length of paths is 1, then
        # we append any modifiers back onto the variable:

        # Although this might be wasteful in doing unnecessary fetches,
        # we have to flag the variable as IsDirty if there is a subscript
        # or a refmod.  That's because it's possible for the
        # the subscript or refmod to include variables, and if any of
        # those elements have changed, we need to re-fetch:

        if len(paths) == 1:
            if subscripts or self.storage_list[paths[0]].Subscripts:
                self.storage_list[paths[0]].IsDirty = True
            self.storage_list[paths[0]].Subscripts = subscripts

            if refmod or self.storage_list[paths[0]].Refmod:
                self.storage_list[paths[0]].IsDirty = True
            self.storage_list[paths[0]].Refmod = refmod
        return paths

    def FlagAllAsDirty(self):
        i = 0
        while i < len(self.storage_list):
            self.storage_list[i].IsDirty = True
            i += 1

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
        # This block of data comes from the VariableString
        self.index_of_parent = None
        self.Program     = ""
        self.Section     = ""
        self.Level       = -1
        self.Name        = ""
        self.Fname       = ""
        self.Bname       = ""
        self.Aname       = ""
        self.Offset      = 0
        self.Length      = 0
        self.OccursMin   = 0
        self.OccursMax   = 0
        self.AttrType    = 0
        self.AttrDigits  = 0
        self.AttrScale   = 0
        self.AttrFlags   = 0

        # These are established at the run-time executable instance level.
        # Eventually we'll have to pay attention to what the run-time libcob
        # says about them, and change them accordingly.
        self.decimal_point = '.'
        self.comma_separator = ','
        self.currency_symbol = '$'

        # These elements are based on parenthesized subscripts after the
        # name:  name(array_index1 , array_index2 , ...)(refmod start : refmod run)
        # Note the commas and colons, which determine which is which
        self.Subscripts          = None  # This is the user's requested subscripts, integers or variables
        self.ResolvedSubscripts  = None  # These are the resolved values (offset is one-based)
        self.TableOffset  = 0    # These are additional offset bytes based on the table Subscripts
        self.Refmod       = None
        self.RefmodOffset = 0   # This is the zero-based Offset in (Offset:Length)
        self.RefmodLength = 0   # This is the Length in (Offset:Length)

        # These values are calculated at runtime from the information up above
        self.location_of_field     = 0  # location of Fname, when available
        self.location_of_data      = 0  # value of Fname->data, when available
        self.location_of_attribute = 0  # value of Fname->attr
        self.data       = []            # list of Length bytes

        self.ActualLocation = 0         # This is the location_of_data qualified by table
                                        # subscripts and refmod offset
        self.ActualLength = 0           # This is the length after applying refmod

        # Here is the string information decoded from the attributes and
        # the data we get from gdb:
        self.not_in_context = False
        self.display_body = ""

        self.SubscriptFailure = False  # Set to True when subscripts don't match the number of tables
        self.RefmodFailure    = False  # Set to True when something is wrong with the refmod length

        # When a variable is accessed for the first time, or when the Subscript changes,
        # or when the refmod changes, or when a previously-read variable is displayed through
        # through a "cprint <short_term_index>, or when the value is changed through a
        # "cprint var=<something>, we flag it as IsDirty so that FetchVariableValue is executed
        # when the variable is next accessed.
        self.IsDirty = True

    def get_verified_field_location(self, allocate):
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
        GV_GlobalVariables.AllocateBuffers()
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
            retval = cob_malloc("sizeof(cob_field)")

            # Having allocated a synthetic cob_field, initialize it with the data
            # Unlike the DONT_ALLOCATE situation, where we aren't allowed to mess with the
            # cob_field->data pointer, we are synthesizing this cob_field.  So, we can
            # add in any table offset:
            command = "set var *(cob_field *){0}".format(hex(retval)) + " = {"
            command += "{0}, (char *){1}, &{2}".format(self.Length,
                                                       hex(self.location_of_data + self.TableOffset),
                                                       self.Aname)
            command += "}"
            gdb.execute(command, False, True)
            return retval

        except gdb.error:
            traceback.print_exc()
            print("Command failed in get_verified_field_location:", command)
            retval = None
        return retval

    def free_field_location(self, field_location):
        cob_free(field_location)

    def DumpJustAboutEverything(self):
        location_of_field = self.get_verified_field_location(DONT_ALLOCATE)
        print(    "           Name:", self.NameToStringFull())
        if self.OccursMax:
            print(    " OCCURS min/max: {0}/{1}".format(self.OccursMin, self.OccursMax))
        if self.Subscripts:
            print("     Subscripts:", self.Subscripts)
        resolved = "(" + ",".join(self.ResolvedSubscripts) + ")"
        if resolved != self.Subscripts:
            print("     (resolved):", resolved)
        if self.Refmod:
            print("         refmod:", self.Refmod)
        print(    "        Section:", self.Section)
        print(    "        Program:", self.Program)
        print(    "          Fname:", self.Fname or "f0")
        print(    "          Bname:", self.Bname)
        print(    "          Aname:", self.Aname)
        print(    "         Offset:", self.Offset)
        if self.TableOffset:
            print("    TableOffset:", self.TableOffset)
        if self.Refmod:
            print("   RefmodOffset:", self.RefmodOffset)
            print("   RefmodLength:", self.RefmodLength)
        print(    "         Length:", self.Length)
        print(    " len(self.data):", len(self.data))
        print(    "       AttrType:", hex(self.AttrType))
        print(    "     AttrDigits:", self.AttrDigits)
        print(    "      AttrScale:", self.AttrScale)
        print(    "      AttrFlags:", hex(self.AttrFlags))
        print(    "   loc_of_field:", hex(location_of_field))
        print(    "    loc_of_data:", hex(self.location_of_data))
        print(    "    loc_of_attr:", hex(self.location_of_attribute))
        print(    "     loc_of_pic:", hex(self.location_of_pic))

    def GetDataAsString(self):
        retval = ""
        GV_GlobalVariables.AllocateBuffers()

        if False:  # For development
            self.DumpJustAboutEverything()

        # Something we need to know:
        is_alphanumeric = False
        if self.AttrType in (COB_TYPE_UNKNOWN,
                             COB_TYPE_GROUP,
                             COB_TYPE_BOOLEAN,
                             COB_TYPE_ALNUM,
                             COB_TYPE_ALPHANUMERIC,
                             COB_TYPE_ALPHANUMERIC_ALL,
                             COB_TYPE_ALPHANUMERIC_EDITED,
                             COB_TYPE_NUMERIC_EDITED,
                             COB_TYPE_NATIONAL,
                             COB_TYPE_NATIONAL_EDITED):
            is_alphanumeric = True

        try:
            # We will make our own cob_field in cprint_f0, with location of
            # data modified by .TableOffset
            field_loc = GV_GlobalVariables.cprint_f0
            command = "set var *(cob_field *){0}".format(hex(field_loc)) + " = {"
            command += "{0}, (char *){1}, &{2}".format(self.ActualLength,
                                                       hex(self.ActualLocation),
                                                       self.Aname)
            command += "}"
            # print("command", command)
            gdb.execute(command, False, True)

            buffer = GV_GlobalVariables.cprint_buffer

            # If the data to be displayed aren't valid, the cob routine
            # will sometimes return without having changed the buffer at all.
            # To prevent stale data from the prior call polluting the results,
            # zero the buffer:

            command = "call (void)memset((char *){0},0,{1})".format(hex(buffer),PRINT_BUFFER_SIZE)
            gdb.execute(command, False, True)

            cob_get_field_str(field_loc, buffer, PRINT_BUFFER_SIZE)
        except:
            # This can happen in Windows when cob_get_field_str returns an 0xFF
            # for a GROUP level variabvle thas has LEVEL 88 variables in it.
            traceback.print_exc()
            return "<Error in GetDataAsString>"

        # The formatted variable string is in buffer
        buf_ptr = buffer

        if is_alphanumeric:
            retval = '"'
            for i in range(self.Length):
                # It's possible for cob_get_field_str() to return non-string
                # data, so sanitize it a bit:
                char = GetDataAt(buf_ptr, 1)
                if char == 0:
                    break
                if char < 32:
                    char = ord('.')
                retval += chr(char)
                buf_ptr += 1
            retval += '"'
        else:
            while True:
                char = GetDataAt(buf_ptr, 1)
                if char == 0:
                    break
                retval += chr(char)
                buf_ptr += 1

        return retval

    def PutDataAsString(self, rside, p_startloc, p_length):
        return_value = 1
        cb_move = 0

        location_of_field = self.get_verified_field_location(DONT_ALLOCATE)
        if not location_of_field:
            return 0

        if R_ALLSPACE.match(rside):
            attr_type = COB_TYPE_ALPHANUMERIC_ALL
            rside = " "
            cb_move = 1
        elif R_ALLZERO.match(rside):
            attr_type = COB_TYPE_ALPHANUMERIC_ALL
            rside = "0"
            cb_move = 1
        elif R_SPACE.match(rside):
            attr_type = COB_TYPE_ALPHANUMERIC
            rside = " "
            cb_move = 1
        elif R_ZERO.match(rside):
            attr_type = COB_TYPE_NUMERIC_DISPLAY
            rside = "0"
            cb_move = 1

        try:
            # Let's leverage GDB to set our buffer to rside:
            buffer = GV_GlobalVariables.cprint_buffer

            buf_ptr = buffer
            for index in range(len(rside)):
                n = rside[index]
                command = "set *(char *){0} = {1}".format(hex(buf_ptr), hex(ord(n)))
                gdb.execute(command, False, True)
                buf_ptr += 1
            command = "set *(char *){0} = '\\0'".format(hex(buf_ptr))
            gdb.execute(command, False, True)

            # Fetch a copy of the cob_field_attr
            if cb_move == 1:
                command = "set var ((cob_field_attr *){0})->type = {1}".format(hex(GV_GlobalVariables.cprint_a0), attr_type)
                data_value = hex(buffer)
            else:
                command = "set var *(cob_field_attr *){0} = {1}".format(hex(GV_GlobalVariables.cprint_a0), self.Aname)
                data_value = hex(self.location_of_data + self.TableOffset + p_startloc - 1)
            gdb.execute(command, False, True)

            if self.Refmod:
                # Modify the attribute type.  If it was already COB_TYPE_GROUP, this is obviously harmless.
                # But for other types, this modification makes the debugger behave in the same perverse way
                # that GnuCOBOL does: it just crams whatever ASCII sequence you've specified into
                # whatever the target happens to be.
                command = "set var ((cob_field_attr *){0})->type = {1}".format(hex(GV_GlobalVariables.cprint_a0), COB_TYPE_GROUP)
                gdb.execute(command, False, True)

            # We will make our own cob_field in cprint_f0, using our own cprint_a0
            field_loc = GV_GlobalVariables.cprint_f0
            command = "set var *(cob_field *){0}".format(hex(field_loc)) + " = {"
            command += "{0}, (char *){1}, (cob_field_attr *){2}".format(p_length,
                                                                        data_value,
                                                                        hex(GV_GlobalVariables.cprint_a0))
            command += "}"
            gdb.execute(command, False, True)

            if cb_move == 1:
                cob_move(field_loc, location_of_field)
            else:
                cob_put_field_str(field_loc, buffer)
            self.IsDirty = True
        except gdb.error:
            traceback.print_exc()
            print("COMMAND failed in PutDataAsString: ", command)
            return_value = 0

        return return_value

    @staticmethod
    def PutDataAsVar():
        """Sets one variable's contents to another

        The use of global variables is a solution to the problem of wanting
        to assign "table(2) = table(3)"

        The lists of possibilities have only one entry for a table.  When naively
        setting table(2) = table(3), the .Subscripts for the first value get overwritten
        by the second one, and you end up doing "table(3) = table(3).

        In this solution, I create copies of the CobolVariables when ProcessArguments
        encounters the "cp var1=var2.  Those copies go into GV_GlobalVariables.

        For now, this handles the basic user-interface case.  If more complex cases
        crop up, this will obviously need to be changed."""
        return_value = 1

        # We use the ALLOCATE option because of the possibility that one
        # or both of the source and destination might be, weirdly, COBOL
        # variables that don't have valid f_xx cob_fields, but for some
        # reason the developer wants use them as source or destination

        var_left = GV_GlobalVariables.VarLeft
        var_right = GV_GlobalVariables.VarRight

        if var_left.SubscriptFailure or var_right.SubscriptFailure or var_left.RefmodFailure or var_left.RefmodFailure:
            return 0

        destination = var_left.get_verified_field_location(ALLOCATE)
        if not destination:
            return 0

        source = var_right.get_verified_field_location(ALLOCATE)
        if not source:
            var_left.free_field_location(destination)
            return 0

        try:
            cob_move(source, destination)
            var_left.IsDirty = True

            # Recall that var_left is a copy of the GlobalVariable in var_trie.storage_list.  It has the same
            # data location as the original.  We just changed that data, so we need to flag the entry in the
            # list as dirty.  This is why we've been maintaining .VarLeftIndex
            GV_ModuleInformation.var_trie.storage_list[GV_GlobalVariables.VarLeftIndex].IsDirty = True

        except gdb.error:
            print("COMMAND failed in PutDataAsVar: ", command)
            return_value = 0

        var_left.free_field_location(source)
        var_right.free_field_location(destination)

        return return_value

    def GetDataAsHex(self):
        retval = ""
        if len(self.data) > 16:
            return "<see below>"
        if len(self.data) > 0:
            retval = "0x"

            output_bytes = self.data[:]
            if GV_GlobalVariables.byteorder == "little":
                if (self.AttrFlags & COB_FLAG_BINARY_SWAP) == 0:  # If this is ON, then it is already in display order
                    if (self.AttrFlags & (COB_FLAG_REAL_BINARY | COB_FLAG_IS_POINTER | COB_FLAG_IS_FP)) != 0:
                        # These three types are little-endian; flip them so they display with the big byte to the left
                        output_bytes.reverse()
            for byte in output_bytes:
                hexout = hex(byte)[-2:]
                if hexout[0:1] == 'x':
                    hexout = '0' + hexout[-1:]
                retval += hexout
        return retval

    def CalculateTableOffset(self):
        self.ResolvedSubscripts = []
        self.TableOffset = 0
        self.SubscriptFailure = False
        if self.Subscripts:
            # Strip off the leading parenthesis (we know it is there
            # from GetListOfPossibilities
            subscripts = self.Subscripts[1:]
            # If there is a trailing parenthesis, get rid of it.  It ought
            # to be there, but if somebody typed "(1,2,3" we are going to be
            # magnamimous about it.
            nfound = subscripts.find(')')
            if nfound != -1:
                subscripts = subscripts[:nfound]

            subscripts = PreprocessSubscriptsAndRefmods(subscripts)
            list_of_subscripts = subscripts.split(',')

            # The list of subscripts is just that: a list of subscripts.

            # With that list in hand, we are going to walk the heirarchical tree
            # of this variable back to the root.  Everywhere we hit an OCCURS, we
            # will apply the current list_of_subscripts[-1] to that portion of the
            # heirarchy.

            # We'll keep track along the way, to make sure the number of subscripts
            # is the same as the number of subscripted variables:

            needed = 0
            available = len(list_of_subscripts)
            variable = self
            while variable:
                if variable.OccursMax:
                    # This is a table variable:
                    needed += 1
                    if len(list_of_subscripts) == 0 or not list_of_subscripts[-1]:
                        # Empty becomes 1.  It's convenient for debugging
                        subscript = 1
                    else:
                        subscript, limit_reached = IntegerOrVariable(list_of_subscripts[-1].strip())
                        if subscript is None and limit_reached:
                            print("Multiple symbols match \"{0}\"".format(list_of_subscripts[-1].strip()))
                            self.SubscriptFailure = True
                            break
                        if subscript is None:
                            print("No symbol matches \"{0}\"".format(list_of_subscripts[-1].strip()))
                            self.SubscriptFailure = True
                            break
                    self.ResolvedSubscripts = [subscript] + self.ResolvedSubscripts
                    list_of_subscripts = list_of_subscripts[0:-1]
                    if subscript < 1 or subscript > variable.OccursMax:
                        print("The subscript '{0}({1})' is out of range (1 through {2})".format(variable.Name,
                                                                                                subscript,
                                                                                                variable.OccursMax))
                        available = 0  # This will prevent an additional superfluous "too many subscripts" message
                        needed = 0
                        self.SubscriptFailure = True
                        break
                    self.TableOffset += (subscript - 1) * variable.Length
                if not variable.index_of_parent:
                    break
                variable = GV_ModuleInformation.var_trie.storage_list[variable.index_of_parent]

            available -= needed
            if available:
                if needed == 1:
                    print("{0} needs {1} subscript".format(self.Name, needed))
                else:
                    print("{0} needs {1} subscripts".format(self.Name, needed))
                self.SubscriptFailure = True
        if self.SubscriptFailure:
            self.ResolvedSubscripts = []
            self.TableOffset = 0
        return

    def CalculateRefmodValues(self):
        self.RefmodOffset = 0
        self.RefmodLength = 0
        self.RefmodFailure = False
        if self.Refmod:
            # There is a refmod
            # we know it starts and ends with parentheses, so strip them off
            refmod = self.Refmod[1:-1]
            refmod = PreprocessSubscriptsAndRefmods(refmod)
            refmod = refmod.split(':')
            if len(refmod) != 2:
                # There have to be two, and only two elements:
                print("The refmod {0} has to have two elements".format(self.Refmod))
                self.RefmodFailure = True
                return

            if refmod[0]:
                offset, limit_reached = IntegerOrVariable(refmod[0].strip())
                if offset is None and limit_reached:
                    print("Multiple symbols match \"{0}\"".format(refmod[0].strip()))
                    self.RefmodFailure = True
                    return
                if offset is None:
                    print("No symbol matches \"{0}\"".format(refmod[0].strip()))
                    self.RefmodFailure = True
                    return
            else:
                offset = 1

            if offset is None:
                offset = 1

            if offset < 1:
                print("The refmod {0} offset has to be greater than zero".format(self.Refmod))
                self.RefmodFailure = True
                return

            if refmod[1]:
                length, limit_reached = IntegerOrVariable(refmod[1].strip())
                if length is None and limit_reached:
                    print("Multiple symbols match \"{0}\"".format(refmod[1].strip()))
                    self.RefmodFailure = True
                    return
                if length is None:
                    print("No symbol matches \"{0}\"".format(refmod[1].strip()))
                    self.RefmodFailure = True
                    return
            else:
                # No value was specified, so make it the maximum
                length = self.Length - offset + 1

            if length < 1 or length > self.Length:
                print("The refmod {0} length has to be between 1 and {1}".format(self.Refmod, self.Length))
                self.RefmodFailure = True
                return

            if offset - 1 >= self.Length:
                print("The refmod {0} offset has to be less than {1}".format(self.Refmod, self.Length))
                self.RefmodFailure = True
                return
            if offset - 1 + length > self.Length:
                # The requested refmod would overrun the end of the variable.
                # Silently trim it for the user:
                length = self.Length - (offset - 1)

            self.RefmodOffset = offset - 1  # Note that RefmodOffset is zero-based
            self.RefmodLength = length
        return

    def FetchVariableData(self, show_failure):
        # print("FetchVariableData:", self.Name, self.Section)
        # print("\nFetchVariableData", self.Name, self.IsDirty, self.Offset, self.Length)
        if self.IsDirty:
            # We do this work only if the IsDirty flag indicates that we have to do the work.
            self.location_of_field = 0
            self.location_of_data = 0
            self.location_of_attribute = 0
            self.data = []
            self.display_body = ""
            self.display_body_hex = ""
            self.not_in_context = False

            # Let's update everything we might need:
            # Calculate the additional table offsets
            self.CalculateTableOffset()

            # Calculate refmod offset:length
            self.CalculateRefmodValues()

            if self.SubscriptFailure or self.RefmodFailure:
                # One of the Calculate... routines ran into trouble.  They
                # displayed an error message.  It's at this point we bail out.
                self.Subscripts = None
                self.Refmod = None
                return False

            # First, check to see if the compiler provided us with the
            # the name of the field.  If so, use the run-time values in that named
            # field to override anything we might have gotten from compile-time

            if self.Fname:
                # We have a field name
                # print("FetchVariableData Fname", self.Fname)
                self.location_of_field = GetAddressOf(self.Fname)
                if self.location_of_field is None:
                    self.not_in_context = True
                    return False
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
                    return False
                possible_location_of_data = GetDataAt(self.location_of_field + GV_GlobalVariables.SizeofPointer, GV_GlobalVariables.SizeofPointer)
                if possible_location_of_data:
                    self.location_of_data = possible_location_of_data
                self.location_of_attribute = GetDataAt(self.location_of_field + 2 * GV_GlobalVariables.SizeofPointer, GV_GlobalVariables.SizeofPointer)
            else:
                # print("There is no self.Fname")
                pass

            if self.location_of_data == 0:
                # When the COBOL code doesn't reference a variable, there is no f_ cob_field.  But
                # often the cobcd-st routine was able to determine the b_ + offset reference.  Likewise, sometimes
                # there is an f_value, but at this point in the code the cob_field->data value hasn't yet
                # been set up.  But, again, sometimes we have advance knowledge of what is going to be
                # there because we got information from the compiler.
                if self.Bname:
                    # print("FetchVariableData Fname", self.Bname)
                    if self.Section == 'X' or self.Section == 'WB':
                        # EXTERNAL variable
                        # WORKING-STORAGE BASED variable
                        # These are declared in the generated C
                        # as a "static int *b_13" pointer
                        self.location_of_data = GetPointerAt(self.Bname)
                    else:
                        # self.Bname is the name of something like "static unsigned char b_13[24]"
                        # So, we need to pick up the address of self.Bname
                        self.location_of_data = GetAddressOf(self.Bname)
                    if self.location_of_data is None:
                        self.not_in_context = True
                        return False
                    self.location_of_data += self.Offset
                    # print("  self.location_of_data + offset", hex(self.location_of_data), self.Offset)
                else:
                    # print("There is no self.Bname")
                    pass

            if self.location_of_attribute == 0:
                if self.Aname:
                    self.location_of_attribute = GetAddressOf(self.Aname)
                    if self.location_of_attribute is None:
                        self.not_in_context = True
                        return False
                else:
                    # print(" There is no self.Aname")
                    pass

            if self.location_of_attribute != 0:
                #   typedef struct __cob_field_attr
                #       {
                #       unsigned short       type;       /* Field type */
                #       unsigned short       digits;     /* Digit count */
                #       signed short         scale;      /* Field scale */
                #       unsigned short       flags;      /* Field flags */
                #       const cob_pic_symbol *pic;       /* Pointer to picture string */
                #       } cob_field_attr;
                self.AttrType   = GetDataAt(self.location_of_attribute + 0, 2)
                self.AttrDigits = GetDataAt(self.location_of_attribute + 2, 2)
                self.AttrScale  = GetDataAt(self.location_of_attribute + 4, 2)
                self.AttrFlags  = GetDataAt(self.location_of_attribute + 6, 2)
                self.location_of_pic = GetDataAt(self.location_of_attribute + 8, GV_GlobalVariables.SizeofPointer)

            if self.Section in ('F', 'X', 'W', "WB", 'I'):
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

                    # print("Trapped at ", line_number)
                    # We know we are trapped at line_number

                    # Look up the program-id of that line_number
                    program_id = GV_ModuleInformation.line_list.Find(line_number)
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
                        return False

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
                    return False
            else:
                print("We don't know how to handle", self.Section, self.Name)
                self.not_in_context = True
                return False

            if self.location_of_data != 0:
                # With those calculations in hand, we can calculate the actuals:
                self.ActualLocation = self.location_of_data + self.TableOffset + self.RefmodOffset
                self.ActualLength = self.RefmodLength or self.Length

                # Unlikely as it seems, arriving here means having successfully
                # run the gauntlet of all the reasons up above as to why you
                # can't read this data.  So, get the data:

                self.data = GetBytesAt(self.ActualLocation, self.ActualLength)
                self.display_body_hex = self.GetDataAsHex()
                self.display_body = self.GetDataAsString()
                self.IsDirty = False
        return True

    def NameWithSubscripts(self):
        # Start with full name.
        # Remove the program name from the right side:
        name = self.Name.split('/')
        name = '/'.join(name[0:-1])

        # Tack on table subscripts:
        if self.Subscripts:
            name += "(" + ",".join(map(str, self.ResolvedSubscripts)) + ")"
        elif self.OccursMax:
            # This is a table, but no subscript was supplied.  Assume 1:
            name += "(1)"
        # Tack on reference modifications:
        if self.Refmod:
            name += "({0}:{1})".format(self.RefmodOffset+1, self.RefmodLength)
        return name

    def NameToStringFull(self):
        storage_type = ""
        if self.Section == 'F':
            storage_type = "I/O"
        elif self.Section == 'X':
            storage_type = "EXT"
        elif self.Section == 'W':
            storage_type = "W-S"
        elif self.Section == "WB":
            storage_type = "W-B"
        elif self.Section == 'L':
            storage_type = "L-S"
        elif self.Section == 'I':
            storage_type = "LNK"
        return "{1:02} {2} [{0}]".format(storage_type, self.Level, self.NameWithSubscripts())

    def NameToString(self):
        if GV_ModuleState.current_display_mode in ('3', '4', '5'):
            n_str = self.NameToStringFull()
        else:
            n_str = self.NameWithSubscripts()
        return n_str

    def ShowFields(self):
        bname_location = GetAddressOf(self.Bname) or 0
        bname_text = ""
        if bname_location:
            bname_text = "( %s )" % hex(bname_location)

        print("Name:       ", self.NameToStringFull())
        if self.OccursMax:
            print("OCCURS:      min: {0} max: {1}".format(self.OccursMin, self.OccursMax))

        if self.Subscripts:
            print("Subscripts: ", self.Subscripts, end='')
            resolved = "(" + ",".join(map(str, self.ResolvedSubscripts)) + ")"
            if resolved != self.Subscripts:
                print("; resolves to {0}".format(resolved), end='')
            print("")

        if self.Refmod:
            print("Refmod:     ", self.Refmod, end='')
            resolved = "({0}:{1})".format(self.RefmodOffset + 1, self.RefmodLength)
            if resolved != self.Refmod:
                print("; resolves to {0}".format(resolved), end='')
            print("")

        print("Display:    ", WithRepeatCount(self.display_body))
        print("Hex:        ", self.display_body_hex)

        if self.Section == 'L':
            # This is local-storage
            print("Field:      ", "f0")
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
        if self.Subscripts:
            print("TableOffset:", self.TableOffset)
        if self.Refmod:
            print("RefmodOff:  ", self.RefmodOffset)
            print("RefmodRun:  ", self.RefmodLength)
        print("DataLoc:    ", hex(self.ActualLocation))
        print("Length:     ", self.ActualLength)

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
            if 32 <= byte:   # valid ASCII characters
                print(chr(byte), end="")
            else:
                print('.', end="")
        print('"')

    def NumericValue(self):
        return atoi(self.display_body)

class ModuleInformation():
    """Information about the source code module being debugged.

    Much of the information here is extracted from the VARIABLE_STRING global
    variable that was compiled into the .c module generated by GnuCOBOL.
    """
    def __init__(self):
        # These variables contain information extracted from the VARIABLE_STRING_xxx
        self.current_variable_string = ""
        self.var_trie = VarTrie()
        self.line_list = LineList()
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

                        # We have a new variable string
                        # Clear out the tables:
                        self.var_trie = VarTrie()
                        self.line_list = LineList()
                        self.perform_commands = {}

                        # Load the new tables
                        # Pull apart the variable_string
                        variable_lines = self.current_variable_string.split("~")
                        for variable_line in variable_lines:
                            self.line_list.LoadFromLine(variable_line)
                            self.var_trie.LoadFromLine(variable_line)
                            self.LoadPerformFromLine(variable_line)

                        # Put a fencepost at the end of the line_list
                        self.line_list.Insert(self.line_list.EndOfTheWorld, "Ragnarok")
                        retval = True
            except:
                pass
        return retval

    def LoadPerformFromLine(self, variable_line):
        if len(variable_line) > 0 and variable_line[0] == 'p':
            tokens = variable_line.split("|")
            if len(tokens) > 3:
                self.perform_commands[tokens[3]] = tokens[2]

    def FlagAllAsDirty(self):
        self.var_trie.FlagAllAsDirty()

GV_ModuleInformation = ModuleInformation()

class ModuleState():
    """State of the inferior as it is debugged"""
    def __init__(self):
        # These variables contain state information that changes as the inferior
        # is debugged.
        self.old_dogs = set()  # One level of retained former variables when doing 'cprint ?'
        self.current_trapped_routine = ""
        self.current_trapped_file = ""
        self.current_trapped_frame = -1
        self.current_trapped_line = 0
        self.question_mark_range = int(os.environ.get("CPRINT_R", '6'))
        self.current_full_path = ""
        self.Stack = False
        self.sticky_display_mode = '0'
        self.current_display_mode = '0'
        self.current_display_submode = '0'

        # This code was requested by Simon Sobisch, who wanted
        # to avoid recompiling his source code when the wrapper solution
        # was first deployed.  In Version4.27, this will be removed and everybody
        # will use the wrapper solution.
        self.cob_malloc            = "cob_malloc"
        self.cob_cache_malloc      = "cob_cache_malloc"
        self.cob_free              = "cob_free"
        self.cob_move              = "cob_move"
        self.cob_get_field_str     = "cob_get_field_str"
        self.cob_put_field_str     = "cob_put_field_str"
        self.cob_encode_program_id = "cob_encode_program_id"

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

                        debugged_system = platform.system()
                        if debugged_system == 'Windows':
                            self.cob_malloc            = "DEBUG_cob_malloc_"            + name_root2
                            self.cob_cache_malloc      = "DEBUG_cob_cache_malloc_"      + name_root2
                            self.cob_free              = "DEBUG_cob_free_"              + name_root2
                            self.cob_move              = "DEBUG_cob_move_"              + name_root2
                            self.cob_get_field_str     = "DEBUG_cob_get_field_str_"     + name_root2
                            self.cob_put_field_str     = "DEBUG_cob_put_field_str_"     + name_root2
                            self.cob_encode_program_id = "DEBUG_cob_encode_program_id_" + name_root2

                        if GV_ModuleInformation.ReadVariableString(string_name):
                            break
                        stack_line += 1

                    if stack_line >= len(stack):
                        # For whatever reasons, we couldn't find any information in
                        # the stack above the "frame" point
                        return False

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
        except gdb.error:
            self.current_trapped_routine = ""
            self.current_trapped_file = ""
            self.current_trapped_frame = -1
            self.current_trapped_line = 0
            raise gdb.GdbError("No stack.")
        except:
            self.current_trapped_routine = ""
            self.current_trapped_file = ""
            self.current_trapped_frame = -1
            self.current_trapped_line = 0

        return retval

    def EstablishModuleState(self, announce):
        #
        if GV_GlobalVariables.IsDirty:
            # There was a events.stop.
            GV_ModuleInformation.FlagAllAsDirty()
            GV_GlobalVariables.IsDirty = False

        # Go read the frame and stack, and, if necessary, fetch the VariableStringData
        try:
            self.Stack = self.ProcessStack()
        except gdb.GdbError as user_error:
            if announce:
                # pass user errors unchanged
                raise user_error
            return

        if self.Stack:
            # Figure out what routine we are trapped in, from
            # the line number
            for line in GV_ModuleInformation.line_list.line_list:
                if line[0] > self.current_trapped_line:
                    # We've gotten past self.current_trapped_line, so the prior entry
                    # was the one we wanted
                    break
                # Keep looking
                self.current_trapped_routine = line[1]
        else:
            if announce:
                # CHECKME: _printing_ here looks wrong
                print("No stack.")

GV_ModuleState = ModuleState()

class HNode():
    def __init__(self):
        self.storage_table_index = -1
        self.Level = 0
        self.BaseName = ""  # Just A of A/B/C
        self.display_body = ""
        self.parent = None
        self.children = []

def TrimComma(t_str):
    if t_str and t_str[-1] == ',':
        t_str = t_str[:-1]  # Strip off final comma
    return t_str

def BuildHString(node):
    h_str = ""
    if node.Level == 0:
        # This is the root.  We live only for our children
        for child in node.children:
            h_str += BuildHString(child)
    else:
        if node.Level in (1, 77, 78):
            # Top level variables are treated this way:
            h_str += r'{name="'
            h_str += node.BaseName  # + "/" + str(node.storage_table_index)
            h_str += r'",value="'
            if len(node.children) == 0:
                # There are no children, which means this is just
                # a simple variable:
                h_str += r'\"'
                h_str += MachineInterfaceBody(node.display_body)
                h_str += r'\"'
                h_str += r'"'
            else:
                # This is a top-level node with children:
                h_str += r'{_groupvalue=\"'
                h_str += MachineInterfaceBody(node.display_body)
                h_str += r'\",'
                for child in node.children:
                    h_str += BuildHString(child)
                h_str = TrimComma(h_str)
                h_str += r'}"'
            h_str += '},'
        else:
            # This is a node whose level is 01-49, 66
            if len(node.children) == 0:
                h_str += node.BaseName  # + "/" + str(node.storage_table_index)
                h_str += r'=\"'
                h_str += MachineInterfaceBody(node.display_body)
                h_str += r'\",'
            else:
                h_str += node.BaseName  # + "/" + str(node.storage_table_index)
                h_str += r'={_groupvalue=\"'
                h_str += MachineInterfaceBody(node.display_body)
                h_str += r'\",'
                for child in node.children:
                    h_str += BuildHString(child)
                h_str = TrimComma(h_str)
                h_str += r'},'
    return h_str

def MachineInterfaceHeirarchical(possibilities):
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
        payload = GV_ModuleInformation.var_trie.storage_list[payload_index]
        payload.FetchVariableData(False)
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

            new_node.display_body = WithRepeatCount(payload.display_body)

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
    # retval = r'variables=[{name="L01",value="{_groupvalue=\"Snap!\",fullname=\"RobertDubner\",L02={_groupvalue=\"Crackle!\",firstname = \"Robert\", lastname = \"Dubner\",L03={_groupvalue=\"Pop!\",firstname = \"Judy\", lastname = \"Ruderman\"}}}"}]'
    return retval

def get_sorted_possibilities(args, limit):
    # We take a list of strings as args
    possibilities_1 = []
    possibilities = []

    if len(args) >= 1 and GV_ModuleState.current_trapped_line > 0:
        if args[0].find('*') != -1:
            # If the first, presumably single, argument contains an asterisk, then
            # he is asking for a list of variables and values:
            possibilities_1 = GV_ModuleInformation.var_trie.GetAllPossibilities()
        elif args[0].find('?') != -1:
            if not COBCDRW_AVAILABLE:
                print("Function not available: cobcd-rw not available")
                return possibilities, False

            # If the first, presumably single, argument contains a question mark,
            # then we need to round up the usual suspects.

            # But first, we are going to keep a rogue's gallery of the current list
            # suspects.  The idea here is that sometimes MOVE A TO B is the last line
            # of a PARAGRAPH, so as soon as it is executed control jumps elsewhere and
            # B falls out of context.  Or perhaps the next GDB command is a CONTINUE,
            # rather than NEXT.  By keeping one level of stickiness, the display of '?' variables
            # will be a bit more like what the user probably expects.
            if len(GV_ModuleState.current_full_path) > 0:
                if len(args) > 1 and IsNumber(args[1]):
                    # an explicit, one-time range has been requested
                    qmrange = int(args[1])
                    # Clear out the old_dogs, because the parameter was
                    # specific:
                    GV_ModuleState.old_dogs = set()
                else:
                    # without that request, use the established +/- range
                    qmrange = GV_ModuleState.question_mark_range
                if qmrange == -1:
                    qmrange = 1000000000  # equivalent to "all"
                if qmrange <= -2:
                    # -2 means there should be no "cprint ?" response
                    return_string = ""
                else:
                    return_string = subprocess.check_output([COBCDRW_BINARY,
                                                             GV_ModuleState.current_full_path,
                                                             str(GV_ModuleState.current_trapped_line),
                                                             str(qmrange)])
                tokens = return_string.split()
                new_dogs = set()
                for token in tokens:
                    token = token.decode("utf-8")
                    new_dogs = new_dogs.union(set(GV_ModuleInformation.var_trie.GetListOfExactPossibilities(token)))
                possibilities_1 = list(GV_ModuleState.old_dogs.union(new_dogs))
                GV_ModuleState.old_dogs = new_dogs
        elif args[0].isdigit():
            # if the first, presumably single, argument is a number, then he is looking
            # for a short-term memory entry.

            # Pick up the 1-based value indexed by the argument:
            index = int(args[0]) - 1
            if index >= 0 and index < len(GV_GlobalVariables.ShortTermMemory):
                # If index is valid, pick up the short-term memory entry
                possibilities_1 = [GV_GlobalVariables.ShortTermMemory[index]]
            else:
                print("The specified index ({0}) is out of range (1 to {1})".format(index + 1, len(GV_GlobalVariables.ShortTermMemory)))
                return possibilities, False  # False means there was no 'limit' overflow
        else:
            # There are one or more arguments.  Use our clever routine
            # to come up with the possibilities_1:
            possibilities_1 = GV_ModuleInformation.var_trie.GetListOfPossibilities(args)

    # We now have a list of possibilities_1 based on the names of the variables
    # typically, some of them will not be in context.  The only way of
    # knowing which variables are in context is to try to read them.  If we
    # can't read them, then we don't add them to our list:
    target_limit = 0
    for payload_index in possibilities_1:
        payload = GV_ModuleInformation.var_trie.storage_list[payload_index]
        payload.FetchVariableData(False)
        if not payload.not_in_context:
            possibilities.append(payload_index)
            target_limit = target_limit + 1

            # If there are too many possibilities_1, then return nothing.  limit will usually
            # be zero or one.  Zero means return everything; one means return only the possibilities
            # when there is just one, which is useful in situations like "cprint A = B", which can only
            # work if A and B are unambiguous (we actually return 2 if there are too much to distinguish
            # between "not found" and "too much")

            if limit > 0 and target_limit > limit:
                possibilities = []
                return possibilities, True  # True means there was a 'limit' overflow
                break

    # Sorting them makes the display much more sensible:
    possibilities.sort()

    return possibilities, False # False means there was no 'limit' overflow

def get_completion_possibilities(word):
    GV_ModuleState.EstablishModuleState(False)

    word = word.upper()
    lookup = TokenizeCanonicalName(word)
    # print ("DBG: lookup: <",  lookup,  ">")
    # FIXME: in case of completion "TXT" may complete to TXT TXT-A TXT-B,
    #        but if there's a possibility which is a "complete" match like TXT
    #        only this one is returned (which is correct for printing only)
    # FIXME: for completion "get_sorted_possibilities" itself should work with
    #        names that _start_ with the lookup so we don't need to filter later
    possibilities_1, limit_reached = get_sorted_possibilities(lookup, 0)
    # we now have the requested list, create the completion from its names
    possibilities = []
    for index in possibilities_1:
        whole_name = GV_ModuleInformation.var_trie.storage_list[index].Name.upper()
        if word == "*" or whole_name.startswith(word):
            # make sure returned words are not seen as multiple words by GDB
            whole_name = "'" + whole_name + "'"
            possibilities.append(whole_name)

    return possibilities

def break_completion(text, word, condition):
    try:
        if text == "" and word == "":
            if condition == 1:
                possibilities = []
                for gdb_breakpoint in gdb.breakpoints():
                    possibilities.append(str(gdb_breakpoint.number))
                return possibilities

        if condition == 0:
            # gdb only completes this with "i" typed, disabled for now
            # if len(text.split()) == 1:
            #     return ["if"]

            if len(text.split()) in (3, 4) and word == "":
                return ["=", "!=", "<", "=<", ">", ">=", "<>", "=~"]
        else:
            if len(text.split()) in (2, 3) and word == "":
                return ["=", "!=", "<", "=<", ">", ">=", "<>", "=~"]

        # note: modifier / slash is no word, so in this case word is None
        if word and (word[0] in ('*', '?', '"', '\'') or word[0].isdigit()):
            # if it doesn't look like a word then just return nothing
            return gdb.COMPLETE_NONE

        if word is None or word == "" or word[0] in ('(', ')', ',', ':'):
            # gdb returns everything, do the same here
            word = "*"

        possibilities = get_completion_possibilities(word)
        return possibilities
    except:
        traceback.print_exc()

def FromHexAlphaString(conv_str):
    """Converts a COBOL hexadecimal alphanumeric string to an ordinary string"""
    retval = conv_str
    if conv_str[0:2].lower().replace('"', "'") == "x'" and conv_str[-1] in ("'", '"') and (len(conv_str) % 2) == 1:
        # This is correctly formed
        # Trim off the front and back:
        conv_str = conv_str[2:-1]
        i = 0
        retval = '"'
        while i < len(conv_str):
            try:
                conv_char = int(conv_str[i:i + 2], 16)
            except:
                break
            retval += chr(conv_char)
            i += 2
        retval += '"'
    return retval

def FromHexLiteralNumber(conv_str):
    """Converts a COBOL hexadecimal numeric literal string to an ordinary string"""
    retval = 0
    if conv_str[0:2].lower().replace('"', "'") == "h'" and conv_str[-1] in ("'", '"'):
        # This is correctly formed
        # Trim off the front and back:
        conv_str = conv_str[2:-1]
        i = 0
        while i < len(conv_str):
            try:
                nch = int(conv_str[i], 16)
            except:
                break
            retval *= 16
            retval += nch
            i += 1
    return str(retval)

class CPrintWorker():
    """Does most of the non-gdb work of CPrint()"""

    # By creating this worker, we can use a Python debugging package to
    # handle everything that doesn't actually need the gdb object
    def __init__(self):
        self.current_source_file = ""
        GV_ModuleState.sticky_display_mode = os.environ.get("CPRINT_V", '0')
        self.use_cprint = os.environ.get("CPRINT", "cprint")
        self.machine_interface_mode = False
        self.completed_names = []

    def ProcessSlashV(self, args):
        # We know the first two characters of args[0] are "/v"
        v_code = None
        if len(args[0]) == 3:
            # This is a single-digit display v-code:
            v_code = args[0][2]
            args.pop(0)
        if v_code and v_code in "012345d":
            # It's a valid display_mode.  Make it the one-time mode:
            GV_ModuleState.current_display_mode = v_code
            if len(args) == 0:
                # There are no additional arguments, so make it the session mode
                GV_ModuleState.sticky_display_mode = GV_ModuleState.current_display_mode
        else:
            print("The /v code is invalid - it must be /v0 through /v5 or /vd")
            args = []
        return args

    def ProcessSlashR(self, args):
        # We know the first two characters of args[0] are "/r"
        #
        # We want "/rxxx" to be the same as "/r xxx":
        first_arg = args[0][2:]  # Might be empty

        args.pop(0)
        if first_arg:
            args = [first_arg] + args

        r_code = ""
        if len(args) == 0:
            # '/r' was the only argument, so it is a request to display the range:
            if int(GV_ModuleState.question_mark_range) <= -2:
                print("The 'cprint ?' range is currently suppressed (-2)")
            elif int(GV_ModuleState.question_mark_range) == -1:
                print("The 'cprint ?' range is currently the same as 'cprint *' (-1)")
            else:
                print("The 'cprint ?' range is currently +/-", GV_ModuleState.question_mark_range, "lines")
            args = []
            return args

        if len(args) > 0:
            r_code = args[0]
            args.pop(0)

        if IsNumber(r_code):
            # This is a request to change the question_mark_range.  We will make
            # the change and reset the display by deleting the old_dogs, even
            # if the value hasn't actually changed:
            GV_ModuleState.question_mark_range = int(r_code)
            GV_ModuleState.old_dogs = set()
        else:
            print("The /r command is malformed")
            args = []

        return args

    def ProcessDisplayMode(self, args):
        args = args.split(' ')
        self.machine_interface_mode = False

        # Start off with the current display mode the same as the session mode
        GV_ModuleState.current_display_mode = GV_ModuleState.sticky_display_mode
        GV_ModuleState.current_display_submode = ''

        # Now see if he wants to change it
        if len(args) >= 1:
            # if the first argument is /d, turn on our debug flag:
            if args[0] == "/d":
                GV_ModuleState.current_display_mode = 'd'
                args.pop(0)
            elif args[0] == "/x":
                GV_ModuleState.current_display_submode = 'x'
                args.pop(0)
            elif args[0] == "/b":
                GV_ModuleState.current_display_submode = 'b'
                args.pop(0)
            elif args[0] == "/m":
                self.machine_interface_mode = True
                args.pop(0)
            elif args[0][0:2] == "/v":
                args = self.ProcessSlashV(args)
            elif args[0][0:2] == "/r":
                args = self.ProcessSlashR(args)
        if args:
            args = ' '.join(args)
        return args

    def GetDisplayBody(self, payload):
        body = ""
        if payload.Refmod:
            if not payload.display_body.startswith('"') or GV_ModuleState.current_display_submode == 'x':
                # We are dealing with something that libcob thinks is not alphabetic.
                body = '['
                for i in range(len(payload.data)):
                    # This is a trick. By adding 256 to the byte, we ensure that
                    # 0, for example, becomes 0x100 instead of 0x0.  Thus we can
                    # strip off the final two characters for our string
                    body += hex(256 + payload.data[i])[-2:]
                body += ']'
            else:
                # We are dealing with something that libcob thinks is alphabetic:
                # Pull out those characters as ASCII.  Unlike GnuCOBOL, we are
                # going to replace characters we don't recognize with dots
                body = '"'
                for i in range(len(payload.data)):
                    nch = payload.data[i]
                    if nch < 32:
                        nch = ord('.')
                    body += chr(nch)
                body += '"'
            body = WithRepeatCount(body)
        else:
            # There is no refmod involved:
            if GV_ModuleState.current_display_submode == 'x' and len(payload.data) <= 16:
                body = payload.display_body_hex
            elif GV_ModuleState.current_display_submode == 'b' and len(payload.data) <= 8:
                # This is a kludge.  Convert the hex to binary:
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
            else:
                body = WithRepeatCount(payload.display_body)

        return body

    def FormattedDisplay(self, payload):
        body = self.GetDisplayBody(payload)

        # The following code catches the 'q' Quit option
        retval = None
        try:
            if GV_ModuleState.current_display_mode in ('0', '3'):
                print(payload.NameToString(), " : ", body)
            elif GV_ModuleState.current_display_mode in ('1', '4'):
                print(body)
            elif GV_ModuleState.current_display_mode in ('2', '5'):
                print(payload.NameToString())
                print(body)
            else:  # GV_ModuleState.current_display_mode ==  'd':
                payload.ShowFields()
        except KeyboardInterrupt as ki:
            retval = str(ki)
            # print("Quit")
            print(retval) # is there a caller that does not like the print?
        except:
            traceback.print_exc()
        return retval

    @staticmethod
    def set_var_value(value_):
        # The left-hand-side of the assignment is found in GV_GlobalVariables.VarLeft
        # Its index in storage_list is .VarLeftIndex.  See ProcessArguments

        # value_ is a list.  It should have only one element.  It will be a literal constant
        # or numerical constant.  If it is a variable name, we don't use it; we use
        # GV_GlobalVariables.VarRight instead.

        # Just a reminder:  We use .VarLeft and .VarRight to handle the situation
        # where the same table name is on the left and right: "cprint A(2)=A(3).  Because
        # storage_list has only one entry for A, the processing for A(3) overwrites the
        # Subscripts (and Refmod) for A(2).  We get around this by processing the left
        # side first, and making a copy of the object in .VarLeft.  Then we process the
        # right side, and make a copy of that in .VarRight.  When we get here, we operate
        # on those.
        return_value = 1

        value = value_[0]
        if R_ALL_ZERO_SPACE.match(value_[0]):
            if len(value_) == 2 and R_ZERO_SPACE.match(value_[1]):
                value = "{0} {1}".format(value_[0], value_[1])
            else:
                value = value_[0]

        # This routine deals with the various cases of "cprint <this>=<that>"
        # The ability to cope with refmods complicates things considerably.  Try to keep up.
        var_left  = GV_GlobalVariables.VarLeft
        var_right = GV_GlobalVariables.VarRight

        if not var_left.Refmod:
            # Because we are not dealing with a refmod situation, we are
            # pretty much going to let the libcob routines handle thing
            # for us
            if var_right and not var_right.Refmod:
                # this is a straight "cprint var1 = var2
                return_value = CobolVariable.PutDataAsVar()
            else:
                if var_right:  # The logic requires that var_right.Refmod exists:
                    # Because var_right has a refmod, we will use the var_right display
                    # body, which at this point is already trimmed to the data we need
                    value = var_right.display_body
                if value.startswith(('"', "'")) and value.endswith(('"', "'")):
                    # Get rid of the quotes around a string:
                    value = value[1:-1]
                return_value = var_left.PutDataAsString(value, 1, var_left.Length)
        else:
            # var_left has a refmod:
            if var_right:
                # Because there is a right-side variable, we will replace 'value' with
                # the human display of that variable
                value = var_right.display_body

            # There is a refmod on the left side, so arrange to replace the
            # data on the left
            p_startloc = var_left.RefmodOffset + 1
            p_length   = var_left.RefmodLength

            if value.startswith(('"', "'")) and value.endswith(('"', "'")):
                value = value[1:-1]
            return_value = var_left.PutDataAsString(value, p_startloc, p_length)

        if return_value == 1:
            payload = GV_ModuleInformation.var_trie.storage_list[GV_GlobalVariables.VarLeftIndex]

            # Adjust the modifiers in the storage_list to match var_left, so that subsequent
            # fetch/display shows what we just changed:
            payload.Subscripts = var_left.Subscripts
            payload.Refmod     = var_left.Refmod

            # Flag the element in storage_list as IsDirty so that the next fetch reflects the change
            payload.IsDirty = True

            #######################################################
            #
            # POOR DESIGN ALERT!
            #
            # What I am about to describe badly needs fixing.
            #
            # As of this writing, FetchVariableData separately reads data for each element
            # of a COBOL variable.  So, if you have this:
            #
            #     01 PERSON
            #       02 LastName  PIC X(10)
            #       02 FirstName PIC X(10)
            #
            # there are CobolVariables for LastName and FirstName.  But
            # There is also one for PERSON.  So, when PERSON.FetchVariableData
            # is called, a 20-byte copy of PERSON's data is stored in that variable.
            # When LastName.FetchVariableData and FirstName.FetchVariableData are
            # called, second copies are made of those 20 bytes of data.
            #
            # This is not just wasteful -- it's screwed up.  When set_var_value
            # is called to change LastName, that IsDirty bit gets set.  But the
            # IsDirty for PERSON is still False -- and so, even though LastName was
            # changed, displaying PERSON will show the stale data from the fetch
            # before the set_var_value.
            #
            # As a purely temporary expedient, I am setting the global dirty
            # flag, which will prevent the display of stale data, but at the
            # cost of a bunch of unnecessary data reads.  So be it, until I
            # get a chance to fix the fundamental design flaw by not reading
            # multiple copies of the data.  Dubner, 2021-04-02
            #
            #######################################################
            GV_GlobalVariables.IsDirty = True
            #######################################################

        return return_value

    def ProcessArgumentsMachineMode(self, leftside, rightside, leftside_possibilities, rightside_possibilities):
        """We arrive here when processing a cprint/m"""
        if len(leftside_possibilities) == 0:
            # just return quietly.  A "variable not found" message
            # confuses Visual Studio Code
            return

        if len(leftside_possibilities) == 1 and not rightside and leftside[0] != '*' and leftside[0] != '?':
            # Very special case.  This was a `cprint/m something`, which is what happens when
            # the -data-evaluate-expression command is issued by Visual Studio Code.  I considered creating
            # a unique 'print/e ' command, but then I realized that VSC never issues a 'cprint/m something' unless there
            # is also an 'rside'.  So, this code is in response to a `-data-evalute-expression` equivalent command
            payload_index = leftside_possibilities[0]
            payload = GV_ModuleInformation.var_trie.storage_list[payload_index]
            disp_str = 'value="' + MachineInterfaceBodyAlt(WithRepeatCount(payload.display_body)) + '"'
            print(disp_str)
            return

        if len(leftside_possibilities) == 1 and rightside and leftside[0] != '*' and leftside[0] != '?':
            payload_index = leftside_possibilities[0]
            payload = GV_ModuleInformation.var_trie.storage_list[payload_index]
            if self.set_var_value(rightside) == 0:
                # The assignment failed
                return

            disp_str = payload.NameToString() + " set to " + WithRepeatCount(payload.display_body)
            print(disp_str)

            # Eliminate stale modifiers, if necessary
            if payload.Subscripts or payload.Refmod:
                payload.Subscripts = None
                payload.Refmod = None
                payload.IsDirty = True

            return

        # In machine_interface mode, we just return the possibility(ies).
        # Because we are going to be showing the data in a heirarchical way,
        # we want to be sure every sub-level of a 01 Level variable is visible:
        ppp = set(leftside_possibilities)
        for i in leftside_possibilities:
            tokens = GV_ModuleInformation.var_trie.storage_list[i].Name.split('/')
            var = tokens[-2]
            prog = tokens[-1]
            u = i - 1
            while u >= 0:
                tokens = GV_ModuleInformation.var_trie.storage_list[u].Name.split('/')
                if tokens[-2] != var or tokens[-1] != prog:
                    break
                ppp.add(u)
                u -= 1
            d = i + 1
            while d < len(GV_ModuleInformation.var_trie.storage_list):
                tokens = GV_ModuleInformation.var_trie.storage_list[d].Name.split('/')
                if tokens[-2] != var or tokens[-1] != prog:
                    break
                ppp.add(d)
                d += 1

        leftside_possibilities = list(ppp)
        leftside_possibilities.sort()

        disp_str = MachineInterfaceHeirarchical(leftside_possibilities)
        print(disp_str)

    def ProcessArgumentsHumanMode(self, leftside_possibilities, rightside):
        if len(leftside_possibilities) > 1:
            GV_GlobalVariables.ShortTermMemory = []

            line_counter = 1
            for payload_index in leftside_possibilities:
                payload = GV_ModuleInformation.var_trie.storage_list[payload_index]
                if not payload.not_in_context:
                    GV_GlobalVariables.ShortTermMemory.append(payload_index)
                    print_str = "{0:2} : ".format(line_counter)
                    line_counter += 1
                    print_str += payload.NameToString()
                    print_str += " : "

                    body = WithRepeatCount(payload.display_body)
                    if GV_ModuleState.current_display_submode == 'x' and len(payload.data) <= 16:
                        body = payload.display_body_hex

                    print_str += body
                    retval = OurPrint(NoNulls(print_str))
                    if retval:
                        # print("Quit")
                        print(retval)
                        break
        elif len(leftside_possibilities) == 1:
            # There is but one possibility
            # Pick up the payload from the list
            payload = GV_ModuleInformation.var_trie.storage_list[leftside_possibilities[0]]

            if rightside:
                if self.set_var_value(rightside) == 0:
                    # The assignment failed
                    return

            # Having done the assignment, if any,
            # now display the left-side data that was changed or requested to be printed:
            if payload.FetchVariableData(True):
                self.FormattedDisplay(payload)

            # And having displayed it, let's reset subscripts and refmods (if necessary) so that
            # subsequent displays don't carry around the stale modifiers
            if payload.Subscripts or payload.Refmod:
                payload.Subscripts = None
                payload.Refmod = None
                payload.IsDirty = True

    def ProcessArguments(self, arguments):
        # Get rid of the bounding apostrophes of completed names:
        for i in range(len(self.completed_names)):
            # Look for each completed_name in the 'arguments'.  If
            # found, remove any bounding apostrophes.  This isn't foolproof; the
            # user could make confusing things happen.  <shrug>  Like the joke
            # says:  "Don't do that".
            completed_name = self.completed_names[i]
            nfound = arguments.find(completed_name)
            if nfound >= 0:
                nlength = len(completed_name)
                beginning = arguments[:nfound]
                middle    = arguments[nfound:nfound+nlength]
                end       = arguments[nfound+nlength:]
                if middle[-1] == "'":
                    middle = middle[:-1]
                if middle[0] == "'":
                    middle = middle[1:]
                arguments = beginning+middle+end

        # Look for /[dxvmvr] switches:
        args = self.ProcessDisplayMode(arguments)

        # At this point, we have args[], which should be
        #    <var>
        # or
        #    <var> = literal
        # or
        #    <var1> = <var2>
        #
        # <var> is a COBOL variable, which can be
        #   A
        #   A of B
        #   A in B
        #   A/B
        #   A.B

        # It is possible for args[] to be empty. The user might have supplied
        # no arguments, or ProcessDisplayMode could have, properly, eaten all
        # of them.  In any case, no arguments means do nothing:

        if not args or not args[0]:
            return

        if not GV_ModuleState.Stack:
            raise gdb.GdbError("No stack.")

        # Initialize for subsequent processing:
        leftside_possibilities = []
        rightside_possibilities = []
        left_side = ""
        right_side = ""

        left_side, right_side, condition = get_left_right_args(args)
        if not left_side:
            return

        # From this point on, the left_side, and right_side (if there
        # is one), are handled as lists of strings

        # But before going general, We need to handle an obscure situation,
        # but one that can happen.
        #
        # We handle array/table entries by creating a CobolVariable using its base symbol,
        # but also carrying along the Subscripts and the TableOffset created by those subscripts.
        #
        # The rare case is when the operator specifies an assignment: "cp table(2) = table(4)", because
        # The second evaluation stomps on the first one. So, we are going to save copies of both of them
        # one at a time, here, so we can use them later, if needed, in PutDataAsVar()

        GV_GlobalVariables.VarLeft = None
        GV_GlobalVariables.VarLeftIndex = None
        GV_GlobalVariables.VarRight = None

        sq_count = left_side[0].count("'")
        if sq_count > 0 and sq_count % 2 != 0:
            raise gdb.GdbError("Unmatched single quote.")

        if left_side[0][0] == "'":
            # This can happen as a result of TAB completion:
            left_side[0] = left_side[0][1:]
        if left_side[-1][-1] == "'":
            # This can happen as a result of TAB completion:
            left_side[-1] = left_side[-1][:-1]

        leftside_possibilities, limit_reached = get_sorted_possibilities(left_side, 0)
        if not leftside_possibilities:
            # There are no leftside possibilities.  Let him know why, and then bug out
            if left_side[0][0] in ('*', '?') or left_side[0].isdigit():
                # When '*' or '?' match nothing, then just return nothing
                return
            print('No symbols match "{0}" in current context'.format('/'.join(left_side)))
            return

        if len(leftside_possibilities) == 1:
            # because there is but a single possibility, we might be in a "cprint var1=var2" situation
            GV_GlobalVariables.VarLeft = copy.copy(GV_ModuleInformation.var_trie.storage_list[leftside_possibilities[0]])
            GV_GlobalVariables.VarLeftIndex = leftside_possibilities[0]
            if GV_GlobalVariables.VarLeft.SubscriptFailure or GV_GlobalVariables.VarLeft.RefmodFailure:
                return
        elif right_side:
            # Our user wants to do an assignment: "cprint leftside=rightside"
            # Alas, we cannot accommodate that request:
            print('Multiple symbols match the left-hand-side "{0}" in current context.'.format('/'.join(left_side)))
            return

        if right_side and right_side[0].find('"') == -1 and right_side[0].find("'") == -1 and not R_ALL_ZERO_SPACE.match(right_side[0]):
            # We have to deal with the possibility that the right_side is a numeric constant,
            # as in "cprint var=1234".  We can't let get_sorted_possibilities mess with it,
            # because that routine will get confused thinking that we are looking for an
            # short_term_index into storage_list
            if len(right_side) != 1 or not IsNumber(right_side[0]):
                rightside_possibilities, limit_reached = get_sorted_possibilities(right_side, 1)
                if not rightside_possibilities and not limit_reached:
                    print('No symbol matches "{0}" in current context'.format('/'.join(right_side)))
                    return
                if limit_reached:
                    print('Multiple symbols match the right-hand-side "{0}" in current context'.format('/'.join(right_side)))
                    return
                GV_GlobalVariables.VarRight = copy.copy(GV_ModuleInformation.var_trie.storage_list[rightside_possibilities[0]])
                if GV_GlobalVariables.VarRight.SubscriptFailure or GV_GlobalVariables.VarRight.RefmodFailure:
                    return
            else:
                # This is a numeric constant; just pass it along
                pass

        if self.machine_interface_mode:
            self.ProcessArgumentsMachineMode(left_side,
                                             right_side,
                                             leftside_possibilities,
                                             rightside_possibilities)
        else:
            self.ProcessArgumentsHumanMode(leftside_possibilities,
                                           right_side)

class CobolAfterWatchBreakpoint(gdb.Breakpoint):
    def __init__(self, creating_wp):

        frame = gdb.newest_frame ()
        cob_frame = CFrameWorker.Frame(True, 0)
        if cob_frame and cob_frame != frame:
            frame = cob_frame

        resume_address = "*" + str(hex(frame.pc()))

        if GV_GlobalVariables.gdb_8dot3_plus:
            gdb.Breakpoint.__init__(self, resume_address, internal=True)
        else:
            gdb.Breakpoint.__init__(self, resume_address)

        # we fake the output of the original watchpoint + COBOL in CobolWatchpointHandler
        self.silent = True
        self.creating_wp = creating_wp

        # copy commands as necessary ("real watchpoint" never halts and therefore commands are never executed)
        if creating_wp.commands:
            commands_to_set = "cup 0" + "\n" + creating_wp.commands
        else:
            commands_to_set = "cup 0"
        if GV_GlobalVariables.gdb_8dot3_plus:
            self.commands = commands_to_set
        else:
            command = "commands {0}\n{1}\nend".format(gdb.breakpoints()[-1].number, commands_to_set)
            gdb.execute(command, True, False)

    def stop(self):
        self.creating_wp.handler.output_wp()
        self.creating_wp.activate_stop()
        return True # _now_ we do want to stop

    def out_of_scope (self):
        self.creating_wp.handler.output__wp()
        self.creating_wp.activate_stop()


class CobolWatchpointHandler():
    def __init__(self, creating_wp, payload):
        print ("... for", payload.NameToString())
        self.creating_wp = creating_wp
        self.payload = payload
        self.old_value = self.payload.GetDataAsString()

    def output_wp (self):
        title = {
            # gdb.BP_WATCHPOINT : "Watchpoint",
            gdb.BP_HARDWARE_WATCHPOINT : "Hardware watchpoint",
            gdb.BP_READ_WATCHPOINT : "Hardware read watchpoint",
            gdb.BP_ACCESS_WATCHPOINT : "Hardware access (read/write) watchpoint",
        }
        bp_type = self.creating_wp.type
        title_string = title.get(bp_type, "Unknown watchpoint " + str(bp_type))
        payload = self.payload
        print("{0} {1}: {2} from {3}\n".format(title_string,
                                               self.creating_wp.number,
                                               payload.NameToString(),
                                               payload.Program))
        if bp_type == gdb.BP_HARDWARE_WATCHPOINT:
            new_value = payload.GetDataAsString()
            print("Old value: {0}".format(self.old_value))
            print("New value: {0}".format(new_value))
            self.old_value = new_value
        elif bp_type == gdb.BP_ACCESS_WATCHPOINT:
            new_value = payload.GetDataAsString()
            if new_value != self.old_value:
                print("Old value: {0}".format(self.old_value))
                print("New value: {0}".format(new_value))
                self.old_value = new_value
            else:
                print("Value: {0}".format(self.old_value))
        else:
            print("Value: {0}".format(self.old_value))

class CobolWatchBreakpoint(gdb.Breakpoint):
    def __init__(self, payload, bp_spec, bp_action):
        try:
            gdb.Breakpoint.__init__(self, bp_spec, gdb.BP_WATCHPOINT, bp_action)
        except Exception as ex:
            raise gdb.GdbError(ex)
        if self.type == gdb.BP_WATCHPOINT:
            self.delete()
            raise gdb.GdbError("Expression cannot be implemented as hardware watchpoint.\n... and therefore removed")
        self.handler = CobolWatchpointHandler(self, payload)
        self.after_done = 0

    def stop(self):
        if self.after_done == 0:
            self.after_done = 1
            CobolAfterWatchBreakpoint(self)

        # we never want the COBOL watchpoints to end directly because:
        # * this would interfer internal multi-step-changes like memmove_sse_back
        # * stopping here will also show the "C-like" output
        # * commands would be executed (is done in the AfterWatchBreakPoint)
        return False

    def activate_stop(self):
        self.after_done = 0

class CWatchWorker():
    """Does most of the non-gdb work of CWatch()"""

    def __init__(self):
        pass

    @staticmethod
    def set_watchpoint(payload, wp_type):
        if payload.Refmod:
            wp_startloc = payload.location_of_data + payload.TableOffset + payload.RefmodOffset
            wp_length = payload.RefmodLength
        else:
            wp_startloc = payload.location_of_data + payload.TableOffset
            wp_length = payload.Length

        spec = "*(char(*)"
        spec += '[' + str(wp_length) + '])'
        spec += '(' + hex(wp_startloc) + ')'

        if wp_type == 'WPW':
            break_point = CobolWatchBreakpoint(payload, spec, gdb.WP_WRITE)
        elif wp_type == 'WPR':
            break_point = CobolWatchBreakpoint(payload, spec, gdb.WP_READ)
        elif wp_type == 'WPRW':
            break_point = CobolWatchBreakpoint(payload, spec, gdb.WP_ACCESS)
        else:
            print("Internal error: Watchpoint type not recognized")
            return

    def process_watch_arguments(self, arguments):
        left_side = []
        right_side = []
        condition = ""

        wp_type = 'WPW'

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

        if len(args) > 3:
            print("Too many arguments")
            return

        left_side, right_side, condition = get_left_right_args(''.join(args))
        if not left_side:
            return

        if right_side:
            print('No rightside arguments implemented')
            return

        possibilities, limit_reached = get_sorted_possibilities(left_side, 1)
        if not possibilities and not limit_reached:
            print('No symbol matching "{0}" in current context'.format(''.join(left_side)))
            return

        if limit_reached:
            print('More than one symbol matching "{0}" in current context.'.format(''.join(left_side)))
            return

        # Pick up the payload from the list
        payload = GV_ModuleInformation.var_trie.storage_list[possibilities[0]]
        payload.FetchVariableData(False)

        # And go set the Watchpoint
        self.set_watchpoint(payload, wp_type)


class CBreakWorker():
    def __init__(self):
        try:
            self.strtype = basestring
        except NameError:
            self.strtype = str

        self.section_buffer = None
        self.module_buffer = None

    @staticmethod
    def set_breakpoint(spec, t_flag, i_flag):
        # note: temporary support in python API only with GDB 8.3+
        if GV_GlobalVariables.gdb_8dot3_plus:
            break_point = gdb.Breakpoint(spec, type=gdb.BP_BREAKPOINT, temporary=t_flag, internal=i_flag)
        else:
            if t_flag:
                # note: we ignore internal here, because temporary is more important...
                command = "tbreak {0}".format(spec)
                gdb.execute(command, True, False)
                break_point = gdb.breakpoints()[-1]
            else:
                break_point = gdb.Breakpoint(spec, type=gdb.BP_BREAKPOINT, internal=i_flag)

        if break_point.pending:
            break_point.delete()
            print("... and therefore removed")
            return None

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

    @staticmethod
    def module_available(module):
        try:
            spec = "{0}_".format(module)
            spec_ref = gdb.parse_and_eval(spec)
            return True
        except gdb.error:
            return False

    def get_module_name(self, module):
        return_value = None

        if module == "":
            return return_value

        # TODO: encode in cobcd.py, without manual malloc (fixes breakpoint before running))
        try:
            if self.module_buffer is None:
                self.module_buffer = cob_cache_malloc(100)
        except gdb.error:
            if self.module_available(module):
                return module
            else:
                return return_value

        try:
            cob_encode_program_id(module, self.module_buffer, 100, 0)
            module = self.get_data_from_buffer(self.module_buffer)
        except gdb.error as error:
            raise gdb.GdbError("The program is not being run: " + str(error))

        if self.module_available(module):
            return_value = module
        else:
            if READELF_AVAILABLE:
                modules_loaded = CBLDBG_CLOAD_WORKER.load_modules(module.split())
                if modules_loaded is not None and self.module_available(module):
                    return_value = module
            else:
                return_value = module

        return return_value

    def get_section_name(self, section):
        return_value = None

        # TODO: encode in cobcd.py, without manual malloc (fixes breakpoint before running))

        try:
            if self.section_buffer is None:
                self.section_buffer = cob_cache_malloc(100)
        except gdb.error as error:
            self.section_buffer = None
            raise gdb.GdbError("The program is not being run: " + str(error))

        try:
            cob_encode_program_id(section, self.section_buffer, 100, 1)
        except gdb.error as error:
            raise gdb.GdbError("execute failed: " + str(error))

        return_value = self.get_data_from_buffer(self.section_buffer)

        return return_value

    @staticmethod
    def get_var_payload(search_args):
        payload = None
        possibilities = []

        GV_ModuleState.EstablishModuleState(True)
        # HACK to let cbreak work until it is refactored
        # TODO do the refactoring or at least document the issue
        possibilities, limit_reached = get_sorted_possibilities(search_args, 1)

        if limit_reached:
            print('More than one symbol matching "{0}" in current context.'.format(''.join(search_args)))
            return payload

        if not possibilities and not limit_reached:
            print('No symbol matching "{0}" in current context'.format(" ".join(search_args)))
            return payload

        payload_number = possibilities[0]

        # Pick up the payload from the list
        payload = GV_ModuleInformation.var_trie.storage_list[payload_number]
        payload.FetchVariableData(False)

        return payload

    @staticmethod
    def get_payload_field_location(payload, pl_startloc, cmp_len):
        location_of_field = None

        try:
            location_of_field = cob_malloc("sizeof(cob_field)")

            command = "set var *(cob_field *){0}".format(location_of_field) + " = {"
            command += "{0}, (char *){1}, &{2}".format(cmp_len, hex(payload.location_of_data + payload.TableOffset + pl_startloc), payload.Aname)
            command += "}"
            gdb.execute(command, False, True)
        except gdb.error:
            print("get_payload_field_location: Error location_of_field {0}".format(payload.Aname))
            location_of_field = None

        return location_of_field

    def set_regexp_breakpoint(self, line_nr, condition, payload, diff_str, break_point, t_flag, i_flag):
        if condition == "=~":
            cmp_len = len(diff_str)
            if payload.Refmod:
                bp_startloc = payload.RefmodOffset
                bp_length = payload.RefmodLength
                if bp_length < cmp_len:
                    print("Length specifier < compare string ({0} < {1})".format(str(bp_length), str(cmp_len)))
                    return

                cmp_len = bp_length
            else:
                bp_startloc = 0
                bp_length = payload.Length

            location_of_field = self.get_payload_field_location(payload, bp_startloc, cmp_len)
            if not location_of_field:
                return

            bp_startloc += payload.TableOffset
            if break_point is None:
                if line_nr == "-1":
                    break_point = self.set_breakpoint("", t_flag, i_flag)
                else:
                    break_point = self.set_breakpoint(line_nr, t_flag, i_flag)

            self.set_condition(break_point, "$_regex(((char *)cob_get_field_str_buffered({0}), \"{1}\")".format(hex(location_of_field), diff_str))
        else:
            print("Condition not implemented")

    def set_section_breakpoint(self, args, t_flag, i_flag):
        module_name = ""

        mod_sec = args[0].split(":")
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

            if mod_sec[0] != "":
                module_name = self.get_module_name(mod_sec[0])
                if module_name is None:
                    # message already from add-symbol-file-cobol
                    # print("Module not found: {0}".format(mod_sec[0]))
                    return

        if not module_name:
            spec = "SECTION_{0}".format(section_name)
        else:
            spec = "{0}_:SECTION_{1}".format(module_name, section_name)
        break_point = self.set_breakpoint(spec, t_flag, i_flag)

    def get_cob_cmp_refloc(self, payload, cmp_len, second):
        bp_startloc = 1
        location_of_field = None

        bp_startloc = payload.RefmodOffset
        bp_length = payload.RefmodLength

        if bp_length > cmp_len:
            print("Length specifier > compare string ({0} > {1})".format(str(bp_length), str(cmp_len)))
            return location_of_field, cmp_len

        if bp_length < cmp_len and second == 1:
            print("Different length specifiers {0} <> {1}".format(bp_length, cmp_len))
            return location_of_field, cmp_len

        cmp_len = bp_length

        location_of_field = self.get_payload_field_location(payload, bp_startloc, cmp_len)

        return location_of_field, cmp_len

    def set_cob_cmp_breakpoint(self, line_nr, condition, payload_l, payload_r, break_point, is_location, t_flag, i_flag):
        case_ins = 0
        if condition[-1:] == "i":
            case_ins = 1
            condition = condition[:-1]

        if condition in ("=", "=="):
            condition_str = "=="
        elif condition in ("<>", "!="):
            condition_str = "!="
        elif condition in (">=", "<=", ">", "<"):
            condition_str = condition
        else:
            print("Condition {0} not implemented".format(condition))
            return

        if payload_l.Refmod:
            if is_location == 1:
                r_length = GetDataAt(payload_r, GV_GlobalVariables.SizeofPointer)
                location_of_field, cmp_len = self.get_cob_cmp_refloc(payload_l, r_length, 0)
            else:
                location_of_field, cmp_len = self.get_cob_cmp_refloc(payload_l, payload_r.Length, 0)
        else:
            cmp_len = payload_l.Length
            location_of_field = self.get_payload_field_location(payload_l, 0, cmp_len)

        if not location_of_field:
            return

        if is_location == 1:
            other_field = payload_r
        else:
            if payload_r.Refmod:
                other_field, cmp_len = self.get_cob_cmp_refloc(payload_r, cmp_len, 1)
            else:
                cmp_len = payload_r.Length
                other_field = self.get_payload_field_location(payload_r, 0, cmp_len)

        if not other_field:
            return

        if break_point is None:
            if line_nr == "-1":
                break_point = self.set_breakpoint("", t_flag, i_flag)
            else:
                break_point = self.set_breakpoint(line_nr, t_flag, i_flag)

        if case_ins == 1 and is_location == 1:
            self.set_condition(break_point, "(int)cob_cmp(cob_intr_upper_case(0,0,{0}), {1}) {2} 0".format(hex(location_of_field), hex(other_field), condition_str))
        elif case_ins == 1 and is_location == 0:
            self.set_condition(break_point, "(int)cob_cmp(cob_intr_upper_case(0,0,{0}), cob_intr_upper_case(0,0,{1})) {2} 0".format(hex(location_of_field), hex(other_field), condition_str))
        else:
            self.set_condition(break_point, "(int)cob_cmp({0}, {1}) {2} 0".format(hex(location_of_field), hex(other_field), condition_str))

    @staticmethod
    def create_cob_field(value, payload_l, case_ins):
        location_of_field = None

        try:
            location_of_field = cob_malloc("sizeof(cob_field)")
            location_of_attribute = cob_malloc("sizeof(cob_field_attr)")

            if R_ALLSPACE.match(value):
                attr_type = COB_TYPE_ALPHANUMERIC_ALL
                value = " "
            elif R_ALLZERO.match(value):
                attr_type = COB_TYPE_ALPHANUMERIC_ALL
                value = "0"
            elif R_SPACE.match(value):
                attr_type = COB_TYPE_ALPHANUMERIC
                value = " "
            elif R_ZERO.match(value):
                attr_type = COB_TYPE_NUMERIC_DISPLAY
                value = "0"
            elif value.isdigit():
                attr_type = COB_TYPE_NUMERIC_DISPLAY
            else:
                attr_type = COB_TYPE_ALPHANUMERIC

            command = "set var ((cob_field_attr *){0})->type = {1}".format(hex(location_of_attribute), attr_type)
            gdb.execute(command, False, True)

            command = "set var ((cob_field_attr *){0})->flags = {1}".format(hex(location_of_attribute), COB_FLAG_CONSTANT)
            gdb.execute(command, False, True)

            buffer = cob_malloc(PRINT_BUFFER_SIZE)

            if case_ins == 1:
                value = value.upper()

            buf_ptr = buffer
            for index in range(len(value)):
                n = value[index]
                command = "set *(char *){0} = {1}".format(hex(buf_ptr), hex(ord(n)))
                gdb.execute(command, False, True)
                buf_ptr += 1
            command = "set *(char *){0} = '\\0'".format(hex(buf_ptr))
            gdb.execute(command, False, True)

            command = "set var *(cob_field *){0}".format(location_of_field) + " = {"
            command += "{0}, (char *){1}, {2}".format(len(value), hex(buffer), hex(location_of_attribute))
            command += "}"
            gdb.execute(command, False, True)
        except gdb.error:
            print("create_cob_field: Error creating cob_field for {0}".format(value))
            return location_of_field

        return location_of_field

    def set_cond_break(self, left_side, right_side, condition, break_point, is_lit, t_flag, i_flag):
        payload_s = None
        is_location = 0
        case_ins = 0

        line_nr = left_side[0]
        del left_side[0]

        if condition[-1:] == "i":
            case_ins = 1

        payload_f = self.get_var_payload(left_side)
        if payload_f is None:
            # note: error handling already in get_var_payload
            return

        payload_l = copy.copy(payload_f)

        if R_ALL_ZERO_SPACE.match(right_side[0]):
            if len(right_side) == 2 and R_ZERO_SPACE.match(right_side[1]):
                value = "{0} {1}".format(right_side[0], right_side[1])
            else:
                value = right_side[0]
            payload_r = self.create_cob_field(value, payload_l, case_ins)
            is_location = 1
        elif is_lit == 0 and not right_side[0].isdigit():
            payload_s = self.get_var_payload(right_side)
            if payload_s is None:
                # note: error handling already in get_var_payload
                return

            payload_r = copy.copy(payload_s)
        elif is_lit == 1 and condition != "=~":
            payload_r = self.create_cob_field(right_side[0][1:-1], payload_l, case_ins)
            is_location = 1
        elif right_side[0].isdigit():
            payload_r = self.create_cob_field(right_side[0], payload_l, case_ins)
            is_location = 1

        if payload_r is not None and condition != "=~":
            self.set_cob_cmp_breakpoint(line_nr, condition, payload_l, payload_r, break_point, is_location, t_flag, i_flag)
        else:
            if payload_l.AttrType in (1, 33, 34, 35):
                self.set_regexp_breakpoint(line_nr, condition, payload_l, right_side[0][1:-1], break_point, t_flag, i_flag)
            else:
                print("Not yet implemented (Attribute type", payload_l.AttrType, ")")
                return

    def process_break_arguments(self, arguments, t_flag, i_flag):
        is_lit = 0
        left_side = []
        right_side = []
        condition = ""

        if arguments and arguments[-1] in ('"', '\''):
            is_lit = 1

        if arguments:
            left_side, right_side, condition = get_left_right_args(arguments)
            if not left_side:
                return
        else:
            break_point = self.set_breakpoint("", t_flag, i_flag)
            return

        if not right_side:
            if len(left_side) == 1 and left_side[0].isdigit():
                break_point = self.set_breakpoint(left_side[0], t_flag, i_flag)
            elif len(left_side) == 1 and isinstance(left_side[0], self.strtype) and left_side[0].find(':') != -1:
                self.set_section_breakpoint(left_side, t_flag, i_flag)
            elif len(left_side) == 1 and isinstance(left_side[0], self.strtype) and (left_side[0].upper() != "SECTION" or left_side[0].upper() != "SEC"):
                module_name = self.get_module_name(left_side[0])
                if module_name is None:
                    print("Module not found: {0}".format(left_side[0]))
                else:
                    break_point = self.set_breakpoint("-function {0}_ -label ENTRY_{1}".format(module_name,module.upper()), t_flag, i_flag)
            elif len(left_side) == 2 and (left_side[0].upper() == "SECTION" or left_side[0].upper() == "SEC") and isinstance(left_side[1], self.strtype):
                del left_side[0]
                self.set_section_breakpoint(left_side, t_flag, i_flag)
            else:
                print('Breakpoint parameters not recognized')
                return
        elif right_side and condition != "":
            if len(left_side) < 2:
                print('Breakpoint parameters not recognized')
                return

            if len(left_side) >= 2 and not left_side[0].isdigit():
                print("No line specifier, using current location")
                left_side.insert(0, '-1')

            if left_side[1] != "if":
                print('Second argument should be "if"')
                return

            del left_side[1]

            self.set_cond_break(left_side, right_side, condition, None, is_lit, t_flag, i_flag)
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

    def process_condition_arguments(self, arguments):
        left_side = []
        right_side = []
        condition = ""
        is_lit = 0

        if arguments[-1] in ('"', '\''):
            is_lit = 1

        if arguments:
            left_side, right_side, condition = get_left_right_args(arguments)
            if not left_side:
                return

        if not left_side[0].isdigit():
            print('First argument is not a breakpoint number')
            return

        break_point = self.find_breakpoint(left_side[0])
        if break_point is None:
            return

        if len(left_side) == 1:
            print("Breakpoint {0} now unconditional.".format(left_side[0]))
            self.set_condition(break_point, None)
        elif len(left_side) >= 2 and len(left_side) <= 4:
            self.set_cond_break(left_side, right_side, condition, break_point, is_lit, False, False)
        else:
            print('Breakpoint parameters not recognized')
            return

CBLDBG_CBREAK_WORKER = CBreakWorker()

class CPrint(gdb.Command):
    """Prints COBOL-NAMED variables when debugging cobst processed object files.

    COBOL cprint command usage:
    cprint <arg>        displays <arg> using session-default formatting
    cprint <arg>(N)     displays the Nth element of table <arg>. Multiple subscripts are allowed
    cprint <arg>(S:R)   reference modification: displays R characters starting at S in <arg>
    cprint/v0           set session formatting to name and value on one line (this is the default)
    cprint/v1           set session formatting to just value
    cprint/v2           set session formatting to two-line display with name, then value
    cprint/v3           like /v0, but with variable Level and Storage Type included
    cprint/v4           same as /v1
    cprint/v5           like /v2, but with variable Level and Storage Type included
    cprint/v<n> <arg>   displays <arg> with a one-time formatting override
    cprint/x            show numeric value in hexadecimal
    cprint/b            show numeric value in binary
    cprint/d            show expanded information
    cprint/v<n>         with no arguments sets the session formatting
    cprint/m            generate output compatible with '-stack-list-variables --simple-values
    cprint *            display all variables that are currently in context
    cprint/r N          set the range for 'cprint ?' to plus/minus N (default 6; -1 means 'all'; -2 'none')
    cprint/r            show the current value of N
    cprint ?            display in-context variables within +/- N lines of current line
    cprint ? R          display in-context variables within +/- R lines of current line
    cprint VARIABLE="STRING"    set variable to string
    cprint VARIABLE='STRING'    set variable to string
    cprint VARIABLE=NUMERIC     set variable to numeric
    cprint VARIABLE=VARIABLE2   set variable to variable2

    cprint set repeat N sets the display <repeats> threshold to N characters

    The default v-level can be set with the environment variable CPRINT_V=n
    The default r-level can be set with the environment variable CPRINT_R=n
    The 'cprint' command can be changed to 'anything' with the environment variable CPRINT=anything"""

    def __init__(self):
        global COBCDRW_AVAILABLE

        try:
            subprocess.check_output([COBCDRW_BINARY, '-v'])
        except subprocess.CalledProcessError as ex:
            # cobcd-rw until 4.26 release returned "1", assume this is fine
            if ex.returncode != 1:
                print("WARNING: \"cprint ?\" not available: {0} not available, consider setting COBCDRW".format(COBCDRW_BINARY))
                COBCDRW_AVAILABLE = False
        # note: specific errors were only added with Python 3.3, so we catch everything...
        # except subprocess.CalledProcessError:
        except:
            print("WARNING: \"cprint ?\" not available: {0} not available, consider setting COBCDRW".format(COBCDRW_BINARY))
            COBCDRW_AVAILABLE = False

        self.worker = CPrintWorker()

        print('Registering CPrint as ' + self.worker.use_cprint + ' ' + COBCD_VERSION)
        super(CPrint, self).__init__(self.worker.use_cprint, gdb.COMMAND_DATA)

        self.worker.completed_names = []

    def invoke(self, arguments, from_tty):
        try:
            # Check if this is a special case.  (Enough of these will
            # mean putting this in a separate function.)
            tokens = arguments.split()
            if len(tokens) == 3 and tokens[0] == "set" and tokens[1] == "repeat" and IsNumber(tokens[2]):
                GV_GlobalVariables.repeat_count = int(tokens[2])
                if GV_GlobalVariables.repeat_count < 1:
                    GV_GlobalVariables.repeat_count = 1000000
                GV_GlobalVariables.IsDirty = True  # To force reformatting
                return

            GV_ModuleState.EstablishModuleState(False)
            self.worker.ProcessArguments(arguments)
        except gdb.GdbError as user_error:
            # pass user errors unchanged
            raise user_error
        except:
            traceback.print_exc()

    def complete(self, text, word):
        # We expect COBOL variables which GDB doesn't have, so the fallback
        # gdb.COMPLETE_SYMBOL can't be used.
        self.completed_names = []

        try:
            # HACK: because of missing support for hypens: in identifiers
            # we have to look at the text for that
            # but the hack doesn't work because GDB won't complete at - either...
            # if text and text.find('-'):
            #    word = text # split, last part

            # note: modifier / slash is no word, so in this case word is None
            if word and (word[0] in ('*', '?', '"', '\'') or word[0].isdigit()):
                # if it doesn't look like a word then just return nothing
                return gdb.COMPLETE_NONE

            if word is None or word == "" or word[0] in ('(', ')', ',', ':'):
                # gdb returns everything, do the same here
                word = "*"

            possibilities = get_completion_possibilities(word)
            if len(possibilities) == 1:
                # There are degenerate cases that this logic can't handle:  For example,
                # the user can TAB out to a completed name, and then backspace and
                # try again.  But we're trying to help out, here.
                self.worker.completed_names.append(possibilities[0])

            return possibilities
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
        self.worker = CWatchWorker()

    def invoke(self, arguments, from_tty):
        self.dont_repeat()
        try:
            GV_ModuleState.EstablishModuleState(True)
            self.worker.process_watch_arguments(arguments)
        except gdb.GdbError as user_error:
            # pass user errors unchanged
            raise user_error
        except:
            traceback.print_exc()

    @staticmethod
    def complete(text, word):
        try:
            # note: modifier / slash is no word, so in this case word is None
            if word and (word[0] in ('*', '?', '"', '\'') or word[0].isdigit()):
                # if it doesn't look like a word then just return nothing
                return gdb.COMPLETE_NONE

            if word is None or word == "" or word[0] in ('(', ')', ',', ':'):
                # gdb returns everything, do the same here
                word = "*"

            possibilities = get_completion_possibilities(word)
            return possibilities
        except:
            traceback.print_exc()

class CBreak(gdb.Command):
    """Used for creating COBOL-NAMED breakpoints when debugging cobst processed object files.

Set a breakpoint for an expression.
Usage: cbreak
A breakpoint stops execution of your program whenever the expression matches

    cbreak                                       set a breakpoint on the current line
    cbreak MODULE                                set a breakpoint on MODULE entry
    cbreak section SECTION                       set a breakpoint on SECTION entry
    cbreak section MODULE:SECTION                set a breakpoint on SECTION entry in MODULE
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
    cbreak LINENR if VARIABLE condition VARIABLE set a breakpoint on line number (LINENR)
                                                 for variable condition and second variable
                                                 where condition is = <> >= <= > <

    if (start:length) is given the VARIABLE is checked with this startpoint and length

    MSTRING may also be: (ALL)SPACE/SPACES, ZERO/ZEROS/ZEROES

    Examples:
    cbreak 28 VARIABLE = "breakstring"
    cbreak 28 VARIABLE (2:3) = "breakstring"
    cbreak 28 VARIABLE =i "breakstring"          option i for ignore case
    cbreak 28 VARIABLE =~ ^break*

    cbreak 29 VARIABLE > 12"""

    def __init__(self):
        self.worker = CBLDBG_CBREAK_WORKER

        print('Registering commands cwatch, cbreak, ctbreak, ccondition')
        super(CBreak, self).__init__("cbreak", gdb.COMMAND_BREAKPOINTS)
        CTBreak()
        CCondition()

    def invoke(self, arguments, from_tty):
        self.dont_repeat()
        self.worker.process_break_arguments(arguments, False, False)

    @staticmethod
    def complete(text, word):
        possibilities = break_completion(text, word, 0)
        return possibilities

class CTBreak(gdb.Command):
    """Set a temporary breakpoint.
Like "cbreak" except the breakpoint is only temporary,
so it will be deleted when hit.  Equivalent to "break" followed
by using "enable delete" on the breakpoint number.

Do "help cbreak" for info on COBOL expressions"""

    def __init__(self):
        if GV_GlobalVariables.gdb_version_major > 8 or GV_GlobalVariables.gdb_version_major == 8 and GV_GlobalVariables.gdb_version_minor >= 3:
            self.worker = CBLDBG_CBREAK_WORKER
            super(CTBreak, self).__init__("ctbreak", gdb.COMMAND_BREAKPOINTS)

    def invoke(self, arguments, from_tty):
        self.dont_repeat()
        self.worker.process_break_arguments(arguments, True, False)

    @staticmethod
    def complete(text, word):
        possibilities = break_completion(text, word, 0)
        return possibilities

class CCondition(gdb.Command):
    """Specify breakpoint number N to break only if COND is true.

Usage is `ccondition N COND', where N is an integer and COND is a COBOL
expression to be evaluated whenever breakpoint N is reached.
Do "help cbreak" for info on COBOL expressions"""

    def __init__(self):
        self.worker = CBLDBG_CBREAK_WORKER
        super(CCondition, self).__init__("ccondition", gdb.COMMAND_BREAKPOINTS)

    def invoke(self, arguments, from_tty):
        self.dont_repeat()
        self.worker.process_condition_arguments(arguments)

    @staticmethod
    def complete(text, word):
        possibilities = break_completion(text, word, 1)
        return possibilities

class CLoadWorker():
    """Does most of the non-gdb work of CLoad(),
    managing a list of (likely) COBOL modules and paths found via COB_LIBRARY_PATH
    For remote/cross-debugging: needs local access to the executables along with the readelf binary
    and possibly COB_MODULE_EXT set."""

    def __init__(self):
        # TODO: try to get this "target" info from GDB
        self.objext = os.getenv('COB_MODULE_EXT')
        if self.objext is None:
            # FIXME: this is not necessarily the same as the GDBs that executes this python...
            debugged_system = platform.system()
            if debugged_system == 'Windows':
                self.objext = '.dll'
            else:
                self.objext = '.so'

        # note: try to get the value from libcob won't help for renotte-/cross-debugging, as the local
        #       python iterates over the folders
        library_path = os.getenv('COB_LIBRARY_PATH', '.')
        self.module_paths = library_path.split(path.pathsep)

        self.modules = []
        for path_entry in self.module_paths:
            try:
                if not path.isdir(path_entry):
                    print("CloadWorker warning: entry in COB_LIBRARY_PATH skipped, is not a path:", path_entry)
                    continue
                files = os.listdir(path_entry)
            except:
                print("CloadWorker warning: entry in COB_LIBRARY_PATH is not a path:", path_entry)
                continue
            for f in files:
                parts = path.splitext(f)
                if parts[1] == self.objext:
                    self.modules.append(parts[0])
        self.modules = set(self.modules)  # removing possible duplicates and order

    def get_module_ext(self):
        # returns the module extension
        return self.objext

    def get_module_paths(self):
        # returns the paths in COB_LIBRARY_PATH
        return self.module_paths

    def query_modules(self, filters):
        # returns the modules found on start, but only the ones matching the given regex filter(s)
        if not filters:
            filters = {""}
        result_entries = []
        max_entries = 200  # TODO get configured maximum from GDB (may-completions?)
        for filter_entry in filters:
            pattern = re.compile(filter_entry, re.IGNORECASE)
            for module in self.modules:
                if pattern.match(module) is not None:
                    result_entries.append(module)
                    if len(result_entries) == max_entries:
                        result_entries = set(result_entries)  # removing duplicates
                        if len(result_entries) == max_entries:
                            # FIXME GDB asks: Display all 200 possibilities? (y or n)
                            return "  ".join(result_entries) + "\n*** List may be truncated, max-completions reached. ***"
        # FIXME: GDB returns a nice table
        return "  ".join(set(result_entries))  # removing duplicates and order

    def load_modules(self, module_names):
        return_list = []

        for module_name in module_names:
            solibpath = None
            paths = self.get_module_paths()
            for path_entry in paths:
                module_path = path.join(path_entry, module_name + CBLDBG_CLOAD_WORKER.get_module_ext())
                if path.isfile(module_path):
                    solibpath = module_path
                    break

            if solibpath is None:
                print("{0}: module not found in {1}" .format(module_name, paths))
                continue

            offset = CBLDBG_CLOAD_WORKER.get_text_offset(solibpath)
            try:
                gdb_cmd = "add-symbol-file %s %s" % (solibpath, offset)
                gdb.execute(gdb_cmd, False, True)
                print("Reading symbols {0}...".format(solibpath))
                return_list.append(solibpath)
            except gdb.error:
                print("Error in loading: {0}".format(solibpath))

        return return_list

    @staticmethod
    def get_text_offset(solibpath):
        elfres = subprocess.check_output([READELF_BINARY, "-WS", solibpath])
        for line in elfres.splitlines():
            if "] .text " in str(line):
                return "0x" + str(line.split()[4])
        return ""  # TODO: Raise error when offset is not found?

    def get_module_completion(self, word):
        # returns the modules found on start, but only the ones starting with the given filter
        if not word:
            return self.modules
        self.search = word.upper()
        return filter(self.__filterCompletions, self.modules)

    def __filterCompletions(self, word):
        if word.upper().startswith(self.search):
            return True
        else:
            return False

CBLDBG_CLOAD_WORKER = CLoadWorker()

# actual reading originally taken from https://stackoverflow.com/questions/20380204/how-to-load-multiple-symbol-files-in-gdb/62471062#62471062

class CLoad(gdb.Command):
    """Load symbols from COBOL module in COB_LIBRARY_PATH or query for available modules.
Usage: add-symbol-file-cobol MODULE           try to load given COBOL module into GDB
       add-symbol-file-cobol /q [REGEX ..]    query available modules
       add-symbol-file-cobol /c               re-read modules"""

    def __init__(self):
        global READELF_AVAILABLE

        try:
            subprocess.check_output([READELF_BINARY, '--version'])
        # note: specific errors were only added with Python 3.3, so we catch everything...
        # except subprocess.CalledProcessError:
        except:
            READELF_AVAILABLE = False
            raise gdb.GdbError("{0} not found on path, not registering add-symbol-file-cobol, consider setting READELF".format(READELF_BINARY))

        print('Registering add-symbol-file-cobol')
        super(CLoad, self).__init__("add-symbol-file-cobol", gdb.COMMAND_FILES)
        self.search = ''

    def invoke(self, user_input, from_tty):
        self.dont_repeat()

        if user_input:
            module_names = user_input.split()
        else:
            print("module name missing")
            return

        if module_names[0][0:2] == '/c':
            global CBLDBG_CLOAD_WORKER
            CBLDBG_CLOAD_WORKER = CLoadWorker()
            print("Modules re-read")
            return

        if module_names[0][0:2] == '/q':
            del module_names[0]
            print(CBLDBG_CLOAD_WORKER.query_modules(module_names))
            return

        modules_loaded = CBLDBG_CLOAD_WORKER.load_modules(module_names)
        if modules_loaded is None:
            print("No modules found")

    def complete(self, text, word):
        return CBLDBG_CLOAD_WORKER.get_module_completion(word)

class CStart(gdb.Command):
    """(Re-)start the debugged program stopping at the beginning of the first COBOL statement.
Any arguments after the cstart command are passed to the executable.  If present,
they override any that might have been passed to GDB with --args."""

    def __init__(self):
        print('Registering CStart')
        super(CStart, self).__init__("cstart", gdb.COMMAND_RUNNING)

    def invoke(self, arguments, from_tty):
        self.dont_repeat()
        try:
            command = "info files"
            info_files = gdb.execute(command, False, True).split('\n')
            if not info_files[0]:
                # a remote connection cannot be issued by start, cstart does exactly the same
                # instead, one may use (only for extended-remote) "run", with a previous tbreak
                raise gdb.GdbError("No symbol table loaded.  Use the \"file\" command.")
            line1 = info_files[0]
            line1 = line1.replace('\\', '/') # In case of Windows
            main_program = line1.split('/')[-1].split('"')[0]
            # Note: cobcrun may be renamed or contain version numbers - is there a better approach?
            if main_program == COBCRUN_NAME:
                command = "show args"
                args = gdb.execute(command, False, True).split('"')[1].split()
                cobol_program = args[0]
                args = ' '.join(args)
                # Arguments passed by CSTART override any that came from the command line:
                arguments = arguments or args
            else:
                cobol_program = main_program

            cobol_program = cobol_program.replace(".exe", "")    # This is a hack for Windows

            # If there is no cobol program, just run
            if cobol_program:
                # TODO: complete encoding as soon as this is done in cob_encode_program_id in python

                # set temporary maintenance breakpoint
                if GV_GlobalVariables.gdb_8dot3_plus:
                    # after its TODO is fixed: CBLDBG_CBREAK_WORKER.process_break_arguments(cobol_program, True, True)
                    module = cobol_program.replace("-", "__")
                    gdb.Breakpoint("-function {0}_ -label ENTRY_{1}".format(module,module.upper()), type=gdb.BP_BREAKPOINT, temporary=True, internal=True)
                else:
                    # Turn hyphens into doubled underscores:
                    entry_point = cobol_program.replace("-", "__") + "_"
                    command = "tbreak {0}".format(entry_point)
                    gdb.execute(command, True, False)
                command = "run " + arguments
            else:
                # "start" is not expected to actually "run", so we use start in the unknown case
                command = "start " + arguments

            # And proceed to run the program:
            gdb.execute(command, True, False)

        except gdb.error:
            print("cstart failed: " + command)

        except gdb.GdbError as user_error:
            # pass user errors unchanged
            raise user_error

        except:
            traceback.print_exc()

class CNext(gdb.Command):
    """The cnext command behaves like GDB 'next', but it is smart enough to
attempt to pass over PERFORMs.  It optionally takes a count parameter.

It does the best it can, but occasionally it behaves a little strangely when a
compiler optimization removes the JUMP TO PARAGRAPH and JUMP BACK FROM PARAGRAPH
logic and simply puts the paragraph in-line, which CNEXT doesn't detect."""

    def __init__(self):
        super(CNext, self).__init__("cnext", gdb.COMMAND_BREAKPOINTS)
        print('Registering CNext')

    def invoke(self, arguments, from_tty):
        try:
            GV_ModuleState.EstablishModuleState(True)
            if arguments:
                ncount = int(arguments)
            else:
                ncount = 1
            while ncount > 0:
                command = "info stack"
                key = gdb.execute(command, False, True).split('\n')[0].split()[-1]
                if key in GV_ModuleInformation.perform_commands:
                    command = "until " + GV_ModuleInformation.perform_commands[key]
                    gdb.execute(command, True, False)

                    # If we went to a specific line location, then the UNTIL leaves
                    # us in a nice place.  If we target a local address, then
                    # we have stopped at a l_xx: target in the C code that shows
                    # up as being before where the user expects to be.  So,
                    # in that case, we issue an extra step to get GDB and the
                    # COBOL source code line in sync.

                    if GV_ModuleInformation.perform_commands[key].find(':') == -1:
                        command = "step"
                        gdb.execute(command, True, False)

                else:
                    command = "next"
                    gdb.execute(command, True, False)
                ncount -= 1
        except gdb.error:
            raise gdb.GdbError("No stack.")
        except:
            traceback.print_exc()

class CFrameWorker:
    """Does all of the gdb work of CUp() and CDown()"""

    def __init__(self):
        print('Registering commands cup, cdown, cup-silently, cdown-silently')
        CUp()
        CDown()
        CUpS()
        CDownS()

    @staticmethod
    def Frame(up, number):

        try:
            frame = gdb.selected_frame()
        except gdb.error:
            raise gdb.GdbError("No stack.")

        while True:
            if number != 0:
                if up:
                    frame = frame.older()
                else:
                    frame = frame.newer()
                if frame is None:
                    return None
            else:
                number = -1

            try:
                block = frame.block()
            except:
                # no debugging symbols, just skip
                block = None
                continue

            symbol = gdb.lookup_symbol("module", block)[0]
            if not symbol:
                continue
            if str(symbol.type) != "cob_module *":
                continue

            # quite likely a COBOL frame
            if number <= 0:
                return frame
            number = number - 1

    @staticmethod
    def Go(argument, up, silent):

        number = 1
        if argument:
            if not argument.isdigit():
                raise gdb.GdbError("argument must be a digit, not " + argument)
            number = int(argument)

        frame = CFrameWorker.Frame(up, number)
        if not frame:
            if up:
                print("Initial COBOL frame selected; you cannot go up.")
            else:
                print("Bottom (innermost) COBOL frame selected; you cannot go down.")
            return

        frame.select()
        gdb.execute("frame", (silent == False), silent)

class CUp (gdb.Command):
    """Select and print COBOL stack frame that called this one.
An argument says how many frames up to go, the special value 0
may be used to also stop if the current frame is a COBOL frame.
"""
    def __init__(self):
        super(CUp, self).__init__("cup", gdb.COMMAND_STACK)

    def invoke(self, argument, from_tty):
        CFrameWorker.Go(argument, True, False)

class CDown (gdb.Command):
    """Select and print COBOL stack frame called by this one.
An argument says how many frames down to go, the special value 0
may be used to also stop if the current frame is a COBOL frame.
"""
    def __init__(self):
        super(CDown, self).__init__("cdown", gdb.COMMAND_STACK)
    def invoke(self, argument, from_tty):
        CFrameWorker.Go(argument, False, False)

class CUpS (gdb.Command):
    """Select COBOL stack frame that called this one silenthly.
An argument says how many frames up to go, the special value 0
may be used to also stop if the current frame is a COBOL frame.
"""
    def __init__(self):
        super(CUpS, self).__init__("cup-silently", gdb.COMMAND_STACK)
    def invoke(self, argument, from_tty):
        CFrameWorker.Go(argument, True, True)

class CDownS (gdb.Command):
    """Select COBOL stack frame called by this one silently.
An argument says how many frames down to go, the special value 0
may be used to also stop if the current frame is a COBOL frame.
"""
    def __init__(self):
        super(CDownS, self).__init__("cdown-silently", gdb.COMMAND_STACK)
    def invoke(self, argument, from_tty):
        CFrameWorker.Go(argument, False, True)

def exit_handler(event):
    # With the disappearance of the inferior, we are going
    # to need new malloc-created buffers
    GV_GlobalVariables.DestroyBuffers()

def stop_handler(event):
    # When the inferior has been running, and is now stopped, we
    # need to flag all cobol variables as IsDirty
    GV_GlobalVariables.IsDirty = True

gdb.events.exited.connect(exit_handler)
gdb.events.stop.connect(stop_handler)

CPrint()
CWatch()
CBreak()
CLoad()
CNext()
CStart()
CFrameWorker()
