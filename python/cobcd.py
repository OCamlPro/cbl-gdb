#######################################################################
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
#    * Neither the name of the COBOLworx nor the names of its
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
#   This module implements the gdb extension commands cprint and cwatch
#EndOfCopyright########################################################

from __future__ import print_function

import re
import sys
import os
import traceback
import datetime
import subprocess

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

def NoNulls(s) :
    ss = ""
    for ch in s:
        if ord(ch) < 32 or ord(ch) >= 128 :
            ch = '.'
        ss += ch
    return ss

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
    #print("MachineInterfaceBody 1:", s)
    ### convert display_body for machine interface:
    came_with_quotes = False
    if len(s) >= 2 and s[0] == '"' and s[-1] == '"' :
        s = s[1:-1]
        came_with_quotes = True

    # Replace double-quotes and backslashes
    retval = ""
    for ch in s :
        if ch == '"' :
            retval += '\\\\\\\"'
        elif ch == '\\' :
            retval += '\\\\\\\\'
        else :
            retval += ch

    if came_with_quotes :
        retval = '\\"' + retval + '\\"'
    #print("MachineInterfaceBody 2:", retval)
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
        if rabbit - fox >= 29 : ## This is a magic number
            ## Because the contraction will be shorter than the length
            ## of the replaced characters, we build the <repeats> clause
            if in_a_quoted_run:
                ## We have to end the current ongoing string
                ## and start this contraction with a prelude
                retval += '", '
            ## and now we can start the contraction:
            retval += '\''
            retval += char
            retval += "' <repeats "
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
                retval += '"'
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
        retval += '"'

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
        if rabbit - fox >= 14 : ## This is a magic number
            ## Because the contraction will be shorter than the length
            ## of the replaced characters, we build the <repeats> clause
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
        if s and s[0] == '"' :
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

def GetAddressOf(variable) :
    if 'gdb' in sys.modules :
        try :
            command = "info address " + variable
            address_string = gdb.execute(command,False,True)

            nfound = address_string.find(" 0x")
            if nfound == -1 :
                print("No match in GetAddressOf()", command)
                return None
            else :
                address_string = address_string[nfound+3:]
                nfound = address_string.find(' ')
                address_string = address_string[0:nfound]
                address = int(address_string,16)
                return address
        except gdb.error:
            print("gdb.execute failed:",command)
            return None
    else :
        # We are running off-line
        return None

class VarTrie() :
    def __init__(self):
        self.storage_list = []
        self.storage_trie = VarTrieNode()
        self.program_ids = set()

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
        if len(variable_line) >= 1 and variable_line[0] == "D" :
            indexSection=1
            indexLevel=2
            indexName=3
            indexField=4
            indexBase=5
            indexOffset=6
            indexListType=7
            indexListSize=8
            indexPicture=9
            indexUsage=10
            indexOccurs=11
            indexAttrType=12
            indexAttrDigits=13
            indexAttrScale=14
            indexAttrFlags=15

            tokens = variable_line.split("|")
            name_tokens =  tokens[indexName].split("/")
            if len(name_tokens) >= 1 :
                payload = CobolVariable()

                payload.Program    = name_tokens[-1]
                payload.Section    = tokens[indexSection]
                payload.Name       = tokens[indexName]
                payload.Level      = int(BlankIsZero(tokens[indexLevel]))
                payload.Base       = tokens[indexBase]
                payload.Field      = tokens[indexField]
                payload.Offset     = int(BlankIsZero(tokens[indexOffset]))
                payload.ListType   = tokens[indexListType]
                payload.ListSize   = int(BlankIsZero(tokens[indexListSize]))
                payload.Picture    = tokens[indexPicture]
                payload.Usage      = tokens[indexUsage]
                payload.Occurs     = int(BlankIsZero(tokens[indexOccurs]))
                payload.FieldSize  = int(BlankIsZero(tokens[indexListSize]))
                payload.AttrType   = int(BlankIsZero(tokens[indexAttrType]))
                payload.AttrDigits = int(BlankIsZero(tokens[indexAttrDigits]))
                payload.AttrScale  = int(BlankIsZero(tokens[indexAttrScale]))
                payload.AttrFlags  = int(BlankIsZero(tokens[indexAttrFlags]))

                if payload.Section == "IN" :
                    payload.Section = "INPUT-OUTPUT"
                elif payload.Section == "WO" :
                    payload.Section = "WORKING-STORAGE"
                elif payload.Section == "LO" :
                    payload.Section = "LOCAL-STORAGE"
                elif payload.Section == "LI" :
                    payload.Section = "LINKAGE"

                if payload.ListType == "A" :
                    payload.ListType == "ALPHANUMERIC"
                if payload.ListType == "N" :
                    payload.ListType == "NUMERIC"
                if payload.ListType == "G" :
                    payload.ListType == "GROUP"
                if payload.ListType == "F" :
                    payload.ListType == "FILE"

                # If the COBOL source code doesn't refer to a variable,
                # then we don't have much information about it.  But
                # we can look at the payload.ListType and at least
                # identify ALPHANUMERICS:

                if payload.ListType in ("ALPHABETIC","ALPHANUMERIC") :
                    payload.AttrType = COB_TYPE_ALPHANUMERIC ;

                # Save the location of this payload in the _storage
                # list.  It's ordered, so we can make use of that when
                # displaying the contents of grouped variables.  We also
                # only keep the payloads themselves in the
                # storage list, and otherwise deal in indexes into that
                # list.
                payload.payload_index = len(self.storage_list)
                self.storage_list.append(payload)
                self.AddToTrie(self.storage_trie,
                                name_tokens,
                                payload.payload_index)
                self.program_ids.add(payload.Program)

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

    def GetListOfPossibilities(self,desired) :
        if isinstance(desired,list) :
            # We were handed a bunch of pieces.  Let's
            # glue them back together into a single space-separated string:
            desired = " ".join(desired)

        # Go handle our various "A of B" possibilities:
        name_tokens = self.TokenizeCanonicalName(desired)

        # Wander through the forest, looking for possible paths:
        paths = []

        if len(name_tokens) == 0 :
            # if there's nothing, there is nothing
            return paths

        self.NameMatcher(name_tokens,0,self.storage_trie,paths)

        if len(paths) == 1 :
            index = paths[0]

        if len(paths) == 0 :
            # A strict attempt at matching what we were given has come up
            # with bupkis.  Let's start again with the very first element (if
            # there was more than one) and give him some options:

            fragment = desired.split()[0]

            pattern1 = re.compile("^"  + fragment)
            pattern2 = re.compile(".+" + fragment)
            pattern3 = re.compile("^"  + fragment,re.IGNORECASE)
            pattern4 = re.compile(".+" + fragment,re.IGNORECASE)

            set1 = set()  # same case, from the beginning
            set2 = set()  # case-insensitive, from the beginning
            set3 = set()  # same case, anywhere
            set4 = set()  # case-insensitive, anywhere
            for index in range(len(self.storage_list)) :
                if re.search(pattern1,self.storage_list[index].Name) :
                    set1.add(index)
                if re.search(pattern2,self.storage_list[index].Name) :
                    set2.add(index)
                if re.search(pattern3,self.storage_list[index].Name) :
                    set3.add(index)
                if re.search(pattern4,self.storage_list[index].Name) :
                    set4.add(index)

            paths = list(set1)
            for index in set2 :
                if index not in paths :
                    paths.append(index)
            for index in set3 :
                if index not in paths :
                    paths.append(index)
            for index in set4 :
                if index not in paths :
                    paths.append(index)

        return paths

    def GetListOfExactPossibilities(self,token) :
        # We are going to return variables that contain token in its
        # exact, case-insensitive, form

        # Wander through the forest, looking for possible paths:
        paths = []
        self.NameMatcher([token],0,self.storage_trie,paths)

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
                line_number = int(tokens[1])
                self.Insert(line_number,program_id)

class CobolVariable() :
    """Manages the field, attributes, flags, and pointers of a GnuCOBOL COBOL variable"""
    def __init__ (self) :
        self.index      = None
        self.Program    = ""
        self.Section    = ""
        self.Name       = ""
        self.Level      = -1
        self.Base       = ""
        self.Field      = ""
        self.Offset     = 0
        self.ListType   = ""
        self.ListSize   = 0
        self.Picture    = ""
        self.Usage      = ""
        self.Occurs     = 0
        self.FieldSize  = 0
        self.AttrType   = 0
        self.AttrDigits = 0
        self.AttrScale  = 0
        self.AttrFlags  = 0

        self.location   = ""            # This is Base + Offset as reported by GDB
        self.data       = []            # list of ListSize bytes

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

    def CleanAndScale(self,rside) :
        """ Returns False if the number could not be CleanedAndScaled """
        # rside is the raw data from the user.
        self.rvalue = rside

        # Expand 9(6)V9(5) to 999999V99999
        self.expanded_picture = Picture(self.Picture)

        pcount = 0
        for ch in self.expanded_picture :
            if ch == 'P' :
                pcount += 1

        # If there is a minus sign, flag this value as negative, and flush
        # out everything to the left of the minus sign
        nhyphen = rside.find('-')
        if nhyphen > -1 :
            self.is_negative = True
            rside = rside[nhyphen+1:]
        else :
            self.is_negative = False

        # A negative number without the HAVE_SIGN is not valid
        if self.is_negative and (self.AttrFlags & COB_FLAG_HAVE_SIGN) == 0 :
            print("Error: Minus sign not allowed on unsigned variable")
            return False

        # Let's flush out everything but numeric stuff, so that 1,000 becomes
        # 1000 and things like 1953/02/02 become 19530202
        self.cleaned_rvalue = ""
        for ch in rside :
            if "0123456789-.".find(ch) > -1 :
                self.cleaned_rvalue += ch

        # Figure out the initial left/right digit count
        self.rdigits = self.AttrScale
        self.ldigits = self.AttrDigits - self.rdigits

        # We need to handle the possibility of
        # scaling 'P' characters in the PICTURE clause.

        # Canonical examples:
        # PICTURE 9PPP yields DIGITS 4, SCALE -3, and an input of
        # 3000 gets converted to self.data 3

        # PICTURE PPP9 yields DIGITS 4, SCALE 4, and an input of
        # 0.0003 gets converted to self.data 3

        if pcount > 0 :
            # Sigh.  Well, we have to do it.  Let's count on GnuCOBOL having
            # figured out whether this is a left-side or right-side scaling

            lstr, rstr = LeftRight(rside,self.decimal_point)

            if self.rdigits < 0 :
                self.ldigits = self.AttrDigits

                # This is a 9PPP style picture.  Whether implied or explicit,
                # the decimal point will be at the right edge of the number

                # There is, therefore, no right string
                self.rdigits = 0
                rstr = ""

                # We lop off the rightmost -ldigits from lstr
                for i in range(pcount) :
                    self.ldigits -= 1
                    lstr = '0' + lstr
                    lstr = lstr[0:-1];

            elif self.rdigits > 0 :
                # This is a PPP9 style picture

                # There is, therefore, no left string
                self.ldigits = 0
                lstr = ""
                # And we lop off the leftmost digits of rstr
                self.rdigits = self.AttrScale
                for i in range(pcount) :
                    self.rdigits -= 1
                    rstr = rstr[1:]
                    rstr = rstr + '0'

            # Put the number back together for further processing:
            self.cleaned_rvalue = lstr + self.decimal_point + rstr

        # Let's turn cleaned_rvalue into a string of exactly AttrDigits
        # characters, zero-filled to the left and right, as necessary

        lstr, rstr = LeftRight(self.cleaned_rvalue,self.decimal_point)

        # Trim off leading zeroes
        while len(lstr) > 0 and lstr[0] == '0' :
            lstr = lstr[1:]

        # Trim off any trailing zeroes:
        while len(rstr)>0 and rstr[len(rstr)-1] == '0' :
            rstr = rstr[:len(rstr)-1]

        # If there are too many ldigits flag this as an error
        if len(lstr) > self.ldigits :
            print("Error: Magnitude too large")
            return False

        # If there are too many rdigits, just grab the leftmost
        if len(rstr) > self.rdigits :
            print("Warning: Least significant digits truncated")
            rstr = rstr[:self.rdigits]

        # Make sure there are exactly ldigits:
        while len(lstr) < self.ldigits :
            lstr = '0' + lstr

        # Make sure there are exactly rdigits:
        while len(rstr) < self.rdigits :
            rstr = rstr + '0'

        self.cleaned_rvalue = lstr + rstr
        if int(self.cleaned_rvalue) == 0 :
            self.is_negative = False

        return True

    def SetToRside(self, line_list , rside) :
        #print( "Inside SetToRside", rside)

        GotData = True

        if self.AttrType == COB_TYPE_NUMERIC_DISPLAY :
            self.StringToNumericDisplay(rside)
        elif self.AttrType == COB_TYPE_NUMERIC_BINARY :
            self.StringToNumericBinary(rside)
        elif self.AttrType == COB_TYPE_NUMERIC_PACKED :
            self.StringToPackedDecimal(rside)
        elif self.AttrType == COB_TYPE_NUMERIC_FLOAT :
            self.StringToIeeeBinary(rside)
        elif self.AttrType == COB_TYPE_NUMERIC_DOUBLE :
            self.StringToIeeeBinary(rside)
        elif self.AttrType == COB_TYPE_NUMERIC_FP_DEC64 :
            GotData = False
            print("Need to implement: COB_TYPE_NUMERIC_FP_DEC64")
        elif self.AttrType == COB_TYPE_NUMERIC_FP_DEC128 :
            GotData = False
            print("Need to implement: COB_TYPE_NUMERIC_FP_DEC128")
        elif self.AttrType == COB_TYPE_NUMERIC_FP_BIN32 :
            GotData = False
            print("Need to implement: COB_TYPE_NUMERIC_FP_BIN32")
        elif self.AttrType == COB_TYPE_NUMERIC_FP_BIN64 :
            GotData = False
            print("Need to implement: COB_TYPE_NUMERIC_FP_BIN64")
        elif self.AttrType == COB_TYPE_NUMERIC_FP_BIN128 :
            GotData = False
            print("Need to implement: COB_TYPE_NUMERIC_FP_BIN128")
        elif self.AttrType == COB_TYPE_ALPHANUMERIC :
            self.StringToAlphanumeric(rside)
        elif self.AttrType == COB_TYPE_NUMERIC_EDITED :
            self.StringToNumericEdited(rside)
        else :
            GotData = False
            print("Probably a POINTER type.  Not yet implemented")

        if GotData and len(self.data) > 0:
            #print("We've GotData: self.data")
            #print(' '.join(format(x, '02x') for x in self.data))

            # We've got self.ListSize bytes in self.data
            # Put them into memory

            the_address = self.GetTheAddress(line_list)

            if the_address :
                i = 0
                while i < len(self.data) :
                    command = "set {char}" + hex(the_address+i) + "=" + hex(self.data[i])
                    if 'gdb' in sys.modules :
                         gdb.execute(command,False,True)
                    i += 1

    def GetTheAddress(self,line_list) :
        if self.Section in ("WORKING-STORAGE",
                            "INPUT-OUTPUT",
                            "LINKAGE") :
            # We can find this variable's address at base_symbol + offset:
            command = "info address " + self.Base

            if 'gdb' in sys.modules :
                try :
                    #print(command)
                    address_string = gdb.execute(command,False,True)

                    match = re.search("address 0x([0-9a-f]+)\.",address_string)
                    if not match :
                        print("No match")
                    else :
                        address = int(match.group(1),16)
                        address += self.Offset
                        return address

                except gdb.error:
                    # This exception is thrown when you ask for a LINKAGE
                    # variable and it's not available because you aren't
                    # trapped inside the referring routine.
                    self.not_in_context = True
            else :
                # We are running off-line
                return None

        elif self.Section == "LOCAL-STORAGE" :
            if 'gdb' in sys.modules :
                try :
                    # Okay.
                    #
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

                    # Okay again.
                    #
                    # program-id is where the program-under-test is
                    # trapped.  Let's see if that program-id matches
                    # the final place of the desired variable:
                    #
                    #print( self.Name )
                    split_name = self.Name.split('/')
                    #print("Match? :",split_name[-1],program_id)
                    if split_name[-1] == program_id :
                        # Everything seems to match up, so we can try
                        # to get this variable's data:

                        command = "info address cob_local_ptr"
                        address_string = gdb.execute(command,False,True)
                        #print(command)
                        #print(address_string)
                        address_string = gdb.execute(command,False,True)

                        match = re.search("address 0x([0-9a-f]+)\.",address_string)
                        if not match :
                            print("No match")
                        else :
                            #print(match.group(1))
                            address = int(match.group(1),16)
                            address += self.Offset
                            return address
                    else :
                        return None

                except gdb.error:
                    return None
        else :
            return None

    def FetchVariableData(self, line_list , show_failure ) :
        data_string = None
        self.location = ""
        self.data = []
        self.display_body = ""
        self.display_pic = ""
        self.display_name = ""
        self.not_in_context = False

        if self.Section in ("WORKING-STORAGE",
                            "INPUT-OUTPUT",
                            "LINKAGE") :
            # We can find this variable's contents by asking for the bytes
            # at base_symbol + offset:
            command = "x/" + str(self.ListSize) + "xb " + self.Base
            if self.Offset > 0 :
                command += "+"
                command += str(self.Offset)

            if 'gdb' in sys.modules :
                try :
                    data_string = gdb.execute(command,False,True)
                except gdb.error:
                    # This exception is thrown when you ask for a LINKAGE
                    # variable and it's not available because you aren't
                    # trapped inside the referring routine.
                    self.not_in_context = True
            else :
                # We are running off-line, so simulate a return of just zeroes
                data_string = ""
                for i in range(self.ListSize) :
                    data_string += "0x00 "
                # print("Simulated", command, "returns", self.ListSize, "zeroes")
        elif self.Section == "LOCAL-STORAGE" :
            if 'gdb' in sys.modules :
                try :
                    # Okay.
                    #
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

                    # Okay again.
                    #
                    # program-id is where the program-under-test is
                    # trapped.  Let's see if that program-id matches
                    # the final place of the desired variable:
                    #
                    #print( self.Name )
                    split_name = self.Name.split('/')
                    #print("Match? :",split_name[-1],program_id)
                    if split_name[-1] == program_id :
                        # Everything seems to match up, so we can try
                        # to get this variable's data:

                        command = "x/" + str(self.ListSize) + "xb "
                        command += "cob_local_ptr"
                        if self.Offset > 0 :
                            command += "+"
                            command += str(self.Offset)

                        data_string = gdb.execute(command,False,True)
                        #print(data_string)
                    else :
                        if show_failure :
                            print("Not in context ( we are stopped in", program_id,")")
                        self.not_in_context = True

                except gdb.error:
                    self.not_in_context = True
            else :
                # print("Can't do LOCAL-STORAGE in debug mode")
                data_string = ""
                for i in range(self.ListSize) :
                    data_string += "0x00 "
        else :
            print("We don't know how to handle",self.Section)
            return

        if data_string != None :
            bytes = data_string.split()

            if len(bytes) > 0 :
                self.location = bytes[0]
                n = self.location.find(':')
                if n > -1 :
                    self.location = self.location[0:n]

            for byte in bytes :
                if len(byte) != 4 :
                    # We are ignoring anything that's not a single-byte 0x..
                    # hex value
                    continue
                self.data.append(int(byte,0))

        # Our caller thinks we have data, so we're going to be asked to
        # display it.  Might as well get a jump on that.
        self.display_name = self.NameToString();
        if self.not_in_context :
            self.display_body = "Not in context"
        else :
            self.display_body,self.display_pic = self.DataToString()
            self.display_body = ReplaceWithRepeatCount(self.display_body)

    def NameToString(self) :
        storage_type = ""
        if self.Section == "INPUT-OUTPUT" :
            storage_type = "I/O"
        elif self.Section == "WORKING-STORAGE" :
            storage_type = "W-S"
        elif self.Section == "LOCAL-STORAGE" :
            storage_type = "L-S"
        elif self.Section == "LINKAGE" :
            storage_type = "LNK"

        s = "{1:02} {2} [{0}]".format(storage_type,self.Level,self.Name)
        return s

    def ShowFields(self) :
        print("Name:       " , self.display_name    )
        print("Base:       " , self.Base            )
        print("Offset:     " , self.Offset          )
        #print("Attr:       " , self.Attr            )
        print("ListType:   " , self.ListType        )

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
        #elif self.AttrType == COB_TYPE_NUMERIC             :   # NUMERIC and NUMERIC_DISPLAY are the same
        #    print("NUMERIC            ", end="")
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

        print("Picture:    " , self.Picture         )
        print("Usage:      " , self.Usage           )
        print("Size:       " , self.ListSize        )
        print("pic_data: (HEX)   [" , end= " ")

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
        print("pic_data: (ASCII) \"", end = "")
        for byte in output_bytes :
            if 32 <= byte < 128 :   # valid ASCII characters
                print(chr(byte),end="")
            else :
                print('.',end="")
        print('"')

    def IeeeBinaryToString(self) :
        bytes = self.data
        length = len(bytes)
        if length == 10 :
            return self.X86_extended_precision_from_bytes(bytes)

        exponent_bits = 0
        fraction_bits = 0
        bias = 0
        valid_digits = 0

        if length == 2 :
            # binary16
            exponent_bits = 5
            fraction_bits = 10
            bias = 15
            valid_digits = 4
        elif length == 4 :
            # binary32
            exponent_bits = 8
            fraction_bits = 23
            bias = 127
            valid_digits = 8
        elif length == 8 :
            # binary64
            exponent_bits = 11
            fraction_bits = 52
            bias = 1023
            valid_digits = 16
        elif length == 16 :
            # binary128
            exponent_bits = 15
            fraction_bits = 112
            bias = 16383
            valid_digits = 34
        elif length == 32 :
            # binary256
            exponent_bits = 19
            fraction_bits = 236
            bias = 262143
            valid_digits = 72
        else :
            message = "Don't know how to handle IEEE 754 floating point with a length of "
            message += str(length)
            message += " bytes"
            raise Exception (message)

        type = "  [ IEEE binary"
        type += str(length*8)
        type += " ]"

        exponent_mask = ((1<<exponent_bits)-1)
        unit_point = 1<<fraction_bits
        ten_point = unit_point * 10
        fraction_mask = unit_point-1

        # combine the bytes into a single value
        value = 1   # This is a guard bit; it ensure that, for example,
                    # a bunch of bytes of zeros have at least the number
                    # of bits we're going to need.

        for i in range(length-1,-1,-1) :
            value <<= 8
            value += bytes[i]

        # extract the various IEEE 754 fields:
        sign = (value >> exponent_bits + fraction_bits) & 1
        exponent = (value >> fraction_bits) & exponent_mask
        fraction = value & fraction_mask

        # start creating the output string:

        body = ""
        pic = ""

        if sign == 1 :
            body += '-'

        if ((exponent+1) & exponent_mask) == 0 :
            # exponent is the maximum possible value
            if fraction == 0 :
                body += "Inf"
                body += type
                return body , pic
            body = "NaN"
            body += type
            return body , pic

        if exponent == 0 :
            if fraction == 0 :
                body += "0.0"
                body += type
                return body , pic
            else :
                # faction is != 0, so this is a denormal number
                # fraction stays the same, with the implied binary
                # point fraction_bits to the left of the whole value
                # exponent becomes -N, where N is -(bias-1)
                exponent = -(bias-1)
                # now, adjust fraction to look like a regular number:
                while (fraction & (1<<fraction_bits)) == 0 :
                    fraction <<= 1
                    exponent -= 1
        else :
            # At this point, the exponent is in the range of 1 - exponent_mask-1,
            # meaning that this is an ordinary floating point number.
            # subtract bias from the exponent, as per the specification
            exponent -= bias
            # and add in the implied "1" bit to the left edge of the fraction
            fraction += unit_point

        # Now it gets interesting.  We have an exponent, which multiplies fraction
        # by a power of 2^exponent.  And we have fraction, which currently has
        # an implied binary point between bits fraction_bits+1 and fraction_bits
        #
        # our rather daunting goal is to convert this to 1.234E10
        #

        # fraction is a value ranging from 1.0000 to 1.999999, and
        #
        # Take advantage of python's unlimited integer capability, and find the
        # decimal equivalent of the unit place of the fraction:
        decimal_exponent = 0
        while exponent != 0 :
            if exponent < 0 :
                # We want to increase the exponent, which means we want to divide
                # the fraction by two.  To avoid losing bits, we're going to
                # do that by multiplying it by 5, and dividing it by ten via
                # the decimal exponent:
                exponent += 1;  # increase exponent towards zero
                fraction *= 5;  # multiply by five
                decimal_exponent -= 1; # divide by ten
            else :
                exponent -= 1;
                fraction <<= 1;

            # Multiply or divide by ten to normalize fraction to be
            # between unit_point <= fraction < ten_point

            while fraction > ten_point :
                fraction //= 10
                decimal_exponent += 1;
            while fraction < unit_point :
                fraction *= 10
                decimal_exponent -= 1;

        # when you pop out here, you have a big number
        # whose left-most decimal digit is 1-9 (and is the
        # units place) and the correct decimal_exponent
        # We can now peel of the digits, largest to smallest:

        digit_count = 1
        digit = fraction // unit_point;
        fraction -= digit * unit_point
        fraction *= 10
        body += chr( ord('0') + digit)
        body += '.'
        while fraction != 0 :
            digit = fraction // unit_point;
            fraction -= digit * unit_point
            fraction *= 10
            body += chr( ord('0') + digit)
            digit_count += 1
            if digit_count >= valid_digits :
                break;

        while body[len(body)-1] == '0' :
            body = body[:len(body)-1]

        if body[len(body)-1] == '.' :
            body += '0';

        body += 'e'
        body += str(decimal_exponent)
        body += type
        return body , pic

    def StringToIeeeBinary(self,rside) :
        self.data = bytearray(self.ListSize)
        length = self.ListSize

        if length == 2 :
            # binary16
            exponent_bits = 5
            fraction_bits = 10
            bias = 15
            valid_digits = 4
        elif length == 4 :
            # binary32
            exponent_bits = 8
            fraction_bits = 23
            bias = 127
            valid_digits = 8
        elif length == 8 :
            # binary64
            exponent_bits = 11
            fraction_bits = 52
            bias = 1023
            valid_digits = 16
        elif length == 16 :
            # binary128
            exponent_bits = 15
            fraction_bits = 112
            bias = 16383
            valid_digits = 34
        elif length == 32 :
            # binary256
            exponent_bits = 19
            fraction_bits = 236
            bias = 262143
            valid_digits = 72
        else :
            message = "Don't know how to handle IEEE 754 floating point with a length of "
            message += str(length)
            message += " bytes"
            raise Exception (message)

        rside = rside.lower()

        if rside == "inf" or rside == "+inf" :
            # Positive infinity
            sign = 0
            exponent = (1<<exponent_bits)-1
            significand = 0
        elif rside == "-inf" :
            # Negative infinity
            sign = 1
            exponent = (1<<exponent_bits)-1
            significand = 0
        elif rside == "nan" or rside == "snan" :
            # signalling NaN
            sign = 0
            exponent = (1<<exponent_bits)-1
            significand = 1
        elif rside == "qnan" :
            # quiet NaN
            sign = 0
            exponent = (1<<exponent_bits)-1
            significand = (1<<(fraction_bits-1)) + 1
        else :
            try :
                fpnumber = float(rside)
            except :
                print("Ill-formed floating point input")
                fpnumber = 0

            if fpnumber == 0 :
                sign = 0
                exponent = 0
                significand = 0
            else :
                # We have run out of dodges; we have to actually encode
                # the floating point number
                if fpnumber < 0 :
                    sign = 1
                    fpnumber = -fpnumber
                else:
                    sign = 0

                astring = "{0:24.18e}".format(fpnumber)

                eloc = astring.find("e")
                point10 = -1 # Just to the right of the leftmost digit
                lside = astring[0]
                lside += astring[2:eloc]

                exp10 = int(astring[eloc+1:])

                while exp10 < 0 :
                    point10 += 1
                    if point10 > 0 :
                        lside = '0' + lside
                    exp10 += 1

                while exp10 > 0 :
                    lside = lside + '0'
                    lside = lside
                    point10 -= 1
                    exp10 -= 1

                if point10 < 0 :
                    intpart = lside[0:-point10]
                    fracpart = lside[-point10:]
                else :
                    intpart = ""
                    fracpart = lside

                while len(fracpart) > 0 :
                    if  fracpart[-1:] != '0' : # Just the last char
                        break
                    fracpart = fracpart[0:-1] # All but last char

                # Convert the integer part to a string of bytes:
                intba = bytearray(intpart.encode("utf8"))
                for i in range(len(intba)) :
                    intba[i] -= 0x30    # "0"

                # Convert the integer part to a string of bits:
                intbits = ""
                while len(intba) > 0 :
                    # divide intba by two
                    borrow = 0
                    for i in range(len(intba)) :
                        newborrow = intba[i] & 0x01
                        intba[i] = (borrow*10 + intba[i]) // 2
                        borrow = newborrow
                    # Push the lowest order bit onto the bitstring
                    if borrow ==  1 :
                        intbits += "1"
                    else :
                        intbits += "0"
                    if intba[0] == 0x00 :
                        intba = intba[1:]

                # Reverse the order of intbits:
                intbits = intbits[::-1]

                # Convert the fractional part to a string of bytes:
                fracba = bytearray(fracpart.encode("utf8"))
                for i in range(len(fracba)) :
                    fracba[i] -= 0x30    # "0"

                # Convert the fractional part to a string of bits.  We
                # we will create one more than we can possibly need

                fracbits = ""
                while len(fracbits) < fraction_bits+1+1 :
                    # Multiply fracbits by two:
                    carry = 0
                    for i in range(len(fracba)-1,-1,-1) :
                        fracba[i] = fracba[i]*2 + carry
                        carry = fracba[i] // 10
                        fracba[i] = fracba[i] % 10
                    # Pick up the bit that means >= 0.5
                    if carry == 1 :
                        fracbits += '1'
                    else :
                        fracbits += '0'

                sigbits = intbits + fracbits
                binary_point = len(intbits)

                # Round to the actual number of places we'll need
                carry = sigbits[fraction_bits+1]
                sigbits = sigbits[0:fraction_bits+1]

                i = len(sigbits)-1
                while(i >= 0) :
                    if carry == '0' :
                        break;
                    if sigbits[i] == '0' :
                        sigbits = sigbits[:i] + '1' + sigbits[i+1:]
                        break;
                    sigbits = sigbits[:i] + '0' + sigbits[i+1:]
                    carry = 1
                    i -= 1

                if carry == 1 :
                    # Well, this is improbable.  Somebody must've entered
                    # 0.999999999999999999999999999999999, but in a way
                    # that snuck past the Python floating point input
                    # conversion.
                    #
                    # What a jerk
                    #
                    sigbits = '1' + sigbits
                    sigbits = sigbits[0:fraction_bits+1]
                    binary_point += 1

                # If the number is >= 1, it's already normalized.

                # Otherwise, strip off leading zeroes while adjusting the
                # binary point:
                while sigbits[0] == '0' :
                    sigbits = sigbits[1:] + '0'
                    binary_point -= 1;

                # calculate the power_of_two from the binary_point position
                power_of_two = binary_point - 1

                # Convert the power_of_two using the bias:

                exponent = power_of_two + bias

                if exponent <= 0 :
                    # I am not going to mess with sub-normal numbers.  This
                    # is, therefore, just too damned small.  Make it zero
                    sign = 0
                    exponent = 0
                    significand = 0
                elif exponent >= (1<<exponent_bits)-1 :
                    # The number is too big for this IEEE format.
                    # Make it infinite
                    exponent = (1<<exponent_bits)-1
                    significand = 0
                else :
                    # We have the sign and exponent.  Build the signicand:
                    significand = 0
                    # Trim off the implicit high-order bit:
                    sigbits = sigbits[1:]
                    for ch in sigbits :
                        significand <<= 1
                        if ch == '1' :
                            significand += 1;

        # We can now build the self.data bytearray:

        mask = 0x80
        offset = 0;
        if sign == 1:
            self.data[offset] |= mask
        mask >>= 1
        if mask == 0 :
            mask = 0x80
            offset += 1

        for i in range(exponent_bits) :
            if exponent & 1<<(exponent_bits-1-i) != 0 :
                self.data[offset] |= mask
            mask >>= 1
            if mask == 0 :
                mask = 0x80
                offset += 1

        for i in range(fraction_bits) :
            if significand & 1<<(fraction_bits-1-i) != 0 :
                self.data[offset] |= mask
            mask >>= 1
            if mask == 0 :
                mask = 0x80
                offset += 1
        self.data.reverse()

    def FpNormalize(self,coefficient,exponent) :
        # convert anything to 1.234E5
        if coefficient == 0 :
            exponent = 0

        str_coefficient = str(coefficient)
        # make sure it's at least two characters
        if len(str_coefficient) == 1 :
            str_coefficient += '0'
            exponent -= 1
        places = len(str_coefficient) - 1   # One digit to the left of the
                                            # decimal point; places to the right
        exponent += places
        # insert the decimal point:
        str_coefficient = str_coefficient[0:1] + '.' + str_coefficient[1:]

        # strip off trailing zeroes
        while str_coefficient[-1:] == '0' :
            str_coefficient = str_coefficient[:len(str_coefficient)-1]

        # when left with "3." make it "3.0"
        if len(str_coefficient) == 2 :
            str_coefficient += '0'

        return str_coefficient,exponent

    def GnuCobolDecimal128ToString(self) :
        # This decodes the GnuCOBOL version of the decimal64 format.  This is
        # based on a misreading of the actual IEEE format:
        # See https://en.wikipedia.org/wiki/Decimal64_floating-point_format
        # Either that, or they were using an early draft version.  Anyway,
        # follow along to see how it works:

        # Convert the bytes of data to a single 128-bit value:
        our_bytes = self.data
        if self.AttrFlags & COB_FLAG_BINARY_SWAP :
            our_bytes.reverse()
        value = 1   # This is a guard bit.  Without it, we wouldn't get a
                    # 128-bit zero, for example
        for i in range(self.ListSize-1,-1, -1) :
            value <<= 8
            value += our_bytes[i]

        if self.ListSize == 16 :
            # decimal128
            # The format is 1 S, 4 combination,  14 exponent, 110 coefficient
            type = "decimal64"
            coefficient_bits1 = 111  # Top 4 are 100x
            coefficient_bits2 = 113  # Top 4 are 0xxx
            exponent_loc1 = 128 - 1 - 2 - 14
            exponent_loc2 = 128 - 1 - 0 - 14
            exponent_bits = 14
            bias = 6176

        sign = (value >> (128-1)) & 1
        combination = (value >> (128-5)) & 0xF;

        coefficient1 = value & ((1<<coefficient_bits1)-1)
        coefficient2 = value & ((1<<coefficient_bits2)-1)
        exponent1 = (value >> exponent_loc1) & ((1<<exponent_bits)-1)
        exponent2 = (value >> exponent_loc2) & ((1<<exponent_bits)-1)

        body = ""
        pic = ""
        if combination == 0b1111 : # It's NaN/Inf
            body = "NaN/Inf"
        else :
            # It's a number
            if sign == 1 :
                body += '-'
            if combination >= 0b1100 :
                # Mode 1:
                exponent = exponent1
                coefficient = coefficient1 + (1<<113)
            else :
                exponent = exponent2
                coefficient = coefficient2

            exponent -= bias
            str_coefficient, exponent = self.FpNormalize(coefficient,exponent)
            body += str_coefficient

            body += 'e'
            body += str(exponent)

            pic += "  [ GnuCOBOL "
            pic += type
            pic += " ]"
        return body,pic


    def GnuCobolDecimal64ToString(self) :
        # This decodes the GnuCOBOL version of the decimal64 format.  This is
        # based on a misreading of the actual IEEE format:
        # See https://en.wikipedia.org/wiki/Decimal64_floating-point_format
        # Either that, or they were using an early draft version.  Anyway,
        # follow along to see how it works:

        # Convert the bytes of data to a single 64-bit value:
        our_bytes = self.data
        if self.AttrFlags & COB_FLAG_BINARY_SWAP :
            our_bytes.reverse()
        value = 1   # This is a guard bit.  Without it, we wouldn't get a
                    # 64-bit zero, for example
        for i in range(self.ListSize-1,-1, -1) :
            value <<= 8
            value += our_bytes[i]

        if self.ListSize == 8 :
            # decimal64
            # The format is 1 S, 4 combination, 8 exponent, 50 coefficient
            type = "decimal64"
            coefficient_bits1 = 51  # Top 4 are 100x
            coefficient_bits2 = 53  # Top 4 are 0xxx
            exponent_loc1 = 64 - 1 - 2 - 10
            exponent_loc2 = 64 - 1 - 0 - 10
            exponent_bits = 10
            bias = 398

        sign = (value >> (64-1)) & 1
        combination = (value >> (64-5)) & 0xF;

        coefficient1 = value & ((1<<coefficient_bits1)-1)
        coefficient2 = value & ((1<<coefficient_bits2)-1)
        exponent1 = (value >> exponent_loc1) & ((1<<exponent_bits)-1)
        exponent2 = (value >> exponent_loc2) & ((1<<exponent_bits)-1)

        body = ""
        pic = ""
        if combination == 0b1111 : # It's NaN/Inf
            body = "NaN/Inf"
        else :
            # It's a number
            if sign == 1 :
                body += '-'
            if combination >= 0b1100 :
                # Mode 1:
                exponent = exponent1
                coefficient = coefficient1 + (1<<53)
            else :
                exponent = exponent2
                coefficient = coefficient2

            exponent -= bias
            str_coefficient, exponent = self.FpNormalize(coefficient,exponent)
            body += str_coefficient

            body += 'e'
            body += str(exponent)

            pic += "[ GnuCOBOL "
            pic += type
            pic += " ]"
        return body, pic

    def IeeeDecimal64ToString(self) :
        # NOTE: This code is more-or-less untested.  This is legitimate code
        # for IEEE decimal64.  As of this writing, version 3.0- of GnuCOBOL
        # has a flawed implementation.  It is internally consistent, but it
        # isn't IEEE.  See GnuCobolDecimal64ToString

        # This is the IEEE decimal64/128 format
        # See https://en.wikipedia.org/wiki/Decimal64_floating-point_format
        # GnuCOBOL uses the Binary Integer Decimal option, rather than
        # the Densely Packed Decimal option.

        # Convert the bytes of data to a single 64-bit value:
        our_bytes = self.data
        if self.AttrFlags & COB_FLAG_BINARY_SWAP :
            our_bytes.reverse()
        value = 1   # This is a guard bit.  Without it, we wouldn't get a
                    # 64-bit zero, for example
        for i in range(self.ListSize-1,-1, -1) :
            value <<= 8
            value += our_bytes[i]

        if self.ListSize == 8 :
            # decimal64
            # The format is 1 S, 5 combination, 8 exponent, 50 coefficient
            type = "binary64"
            coefficient_bits = 50
            exponent_bits = 8
            combination_bits = 5
            bias = 398

        coefficient = value & ((1<<coefficient_bits)-1)
        exponent = (value >> coefficient_bits) & ((1<<exponent_bits)-1)
        combination = (value >> (coefficient_bits+exponent_bits)) \
                              & ((1<<combination_bits)-1)
        sign = (value >> (coefficient_bits+exponent_bits+combination_bits)) & 1

        print( hex(value) )
        print( bin(combination) )
        print( hex(exponent) )
        print( hex(coefficient) )

        body = ""
        pic = ""
        if combination == 0b11111 : # It's NaN
            if (exponent & (1<<(exponent_bits-1))) != 0 :
                body += 's'
            else :
                body += 'q'
            body += "NaN"
        elif combination == 0b11110 : # It's Infinity
            if sign == 1 :
                body += '-'
            else :
                body += '+'
            body += "Inf"
        else :
            # It's a number
            if sign == 1 :
                body += '-'
            if combination >= 0b11000 :
                ms_exponent = (combination>>1)&0x03
                ms_coefficient = 8 + (combination&1)
            else :
                ms_exponent = (combination>>3)&0x03
                ms_coefficient = combination & 0x07

            exponent = (ms_exponent<<exponent_bits) + exponent
            exponent -= bias
            #coefficient = (ms_coefficient<<coefficient_bits) + coefficient
            body += str(coefficient)
            body += 'e'
            body += str(exponent)

            pics += "[ IEEE "
            pics += type
            pics += " ]"
        return body, pic

    def PackedDecimalToString(self) :
        body = ""
        pic = ""

        pcount = 0
        digit_count = 0

        for ch in Picture(self.Picture) :
            if ch == '9' :
                digit_count += 1
            if ch == 'P' :
                pcount += 1

        # The sign nybble: 0x0C = positive
        #                  0x0D = negative
        #                  0x0F = unsigned

        has_sign_nybble = (self.AttrFlags & COB_FLAG_NO_SIGN_NIBBLE) == 0
        signed = (self.AttrFlags & COB_FLAG_HAVE_SIGN) != 0

        negative = False
        if has_sign_nybble :
            sign_nybble = self.data[self.ListSize-1] & 0x0F
            if sign_nybble in (0xf, 0xa, 0xc, 0xe) : # one learns not to ask silly questions
                negative = False
            else :
                negative = True

        nybble = 0
        if has_sign_nybble and (digit_count & 1) == 0 :
            nybble += 1;
        if not has_sign_nybble and (digit_count & 1) != 0 :
            nybble += 1;

        # Pull out the digits
        dcount = 0
        while dcount < digit_count :
            if (nybble & 1) == 0 :
                digit = self.data[nybble>>1] >> 4
            else :
                digit = self.data[nybble>>1] & 0x0F
            digit += ord('0')
            body += chr(digit)
            nybble += 1
            dcount += 1

        if pcount > 0 :
            if self.AttrScale < 0 :
                # Scaling to the left of decimal point
                for i in range(pcount) :
                    body += '0'
            else :
                # Scaling to the right of decimal point
                for i in range(pcount) :
                    body = body + '0'
                body = '.' + body
        elif self.AttrScale > 0 :
            body = body[:self.AttrDigits-self.AttrScale] + \
                    self.decimal_point + \
                    body[self.AttrDigits-self.AttrScale:]

        if negative :
            body = '-' + body

        if signed and not negative :
            body = '+' + body

        pic += "[ "
        if signed :
            pic += "SIGNED"
        else :
            pic += "UNSIGNED"
        pic += " PACKED DECIMAL ("
        pic += str(self.AttrDigits - self.AttrScale)
        pic += ','
        pic += str(self.AttrScale)
        pic += ") ]"
        return body, pic

    def StringToPackedDecimal(self,rside) :
        self.data = bytearray()
        if self.CleanAndScale(rside) :
            # The sign nybble: 0x0C = positive
            #                  0x0D = negative
            #                  0x0F = unsigned
            # Note: Those values are from the internet, and match a number of
            # documents.  I haven't strictly verified what GNUCobol does, except
            # that I have seen 0x0D for negative numbers and I have seen 0x0C
            # for positives.

            if self.is_negative :
                sign_nybble = 0x0D  # Indicates negative
            else :
                sign_nybble = 0x0C  # Indicates positive

            if (self.AttrFlags & COB_FLAG_HAVE_SIGN) == 0 :
                sign_nybble = 0x0F  # Indicates unsigned

            # Reverse the combined string, because we will build it up
            # from low-order to high-order
            rvalue = self.cleaned_rvalue[::-1]

            # We can now put together the self.data package:
            self.data = bytearray(self.ListSize)
            offset = len(self.data)-1
            mask = 0
            if (self.AttrFlags & COB_FLAG_NO_SIGN_NIBBLE) == 0 :
                self.data[offset] |= ((sign_nybble&0xF) << (mask*4))
                #print(''.join(format(x, '02x') for x in self.data))
                mask += 1;
                offset -= mask//2
                mask %= 2

            for ch in rvalue :
                nch = ord(ch) - ord('0');
                nch &= 0x0F
                nch <<= (mask*4)
                self.data[offset] |= nch;
                #print(''.join(format(x, '02x') for x in self.data))
                mask += 1;
                offset -= mask//2
                mask %= 2

    def NumericDisplayToString(self) :
        # numeric digits, but with a decimal point:
        body = ""
        pic = ""

        # Count up any P-scalers in the Picture string:
        pcount = 0
        for ch in Picture(self.Picture) :
            if ch == 'P' :
                pcount += 1;

        negative = False
        if self.ListSize > 0 :
            # DISPLAY keeps the negative bit in an oddball place:
            if (self.data[self.ListSize-1] & 0x40) != 0 :
                negative = True

            # Convert the binary value in self.data to a string
            for i in range(self.ListSize) :
                digit = self.data[i] & 0x3F
                body += chr(digit)

            if pcount > 0 :
                # There is scaling involved
                if self.AttrScale < 0 :
                    # scaling to the left of the decimal point
                    for i in range(pcount) :
                        body += '0'
                else :
                    # scaling to the right of the decimal point
                    for i in range(pcount) :
                        body = '0' + body
                    body = self.decimal_point + body
            elif self.AttrScale > 0 :
                # No scaling
                radix_point = self.ListSize - self.AttrScale
                body = body[:radix_point] + self.decimal_point + body[radix_point:]

            if negative :
                body = '-' + body
            else :
                if (self.AttrFlags & COB_FLAG_HAVE_SIGN) != 0 :
                    body = '+' + body

            pic += "[ DISPLAY ("
            pic += str(self.AttrDigits - self.AttrScale)
            pic += ','
            pic += str(self.AttrScale)
            pic += ") ]"
        return body , pic

    def StringToNumericDisplay(self,rside) :
        self.data = bytearray()

        if self.CleanAndScale(rside) :
            self.data = bytearray(self.ListSize)
            for i in range(self.ListSize) :
                self.data[i] = ord(self.cleaned_rvalue[i])

            if self.is_negative :
                self.data[self.ListSize-1] |= 0x40

    def NumericBinaryToStringComp5(self) :
        # Special processing for COMP-5, which as of this writing
        # behaves differently when compared to other Generic Binary variables

        pcount = 0
        for ch in Picture(self.Picture) :
            if ch == 'P' :
                pcount += 1

        # we can be handed binary data in either little-endian
        # or big-endian.  We will operated little-endian
        our_bytes = self.data

        #our_bytes = [0x00, 0xb4, 0x21, 0x50, 0x1f, 0x01, 0x00, 0x00]
        #our_bytes.reverse();

        if (self.AttrFlags & COB_FLAG_BINARY_SWAP) != 0 :
            our_bytes.reverse()

        # we need to figure out if we are dealing with a signed
        # we need to figure out if we are dealing with a signed
        # and negative number.

        negative = False
        if (self.AttrFlags & COB_FLAG_HAVE_SIGN) != 0 :
            # This is a binary number that can be signed; let's
            # see if it is.
            if (our_bytes[self.ListSize-1] & 0x80) != 0 :
                # and it is negative
                negative = True

                # we need to do a complement & increment.  We will
                # be fighting Python the whole way:

                carry = 1
                for i in range(self.ListSize) :
                    our_bytes[i] = ~our_bytes[i];
                    our_bytes[i] &= 0xFF
                    our_bytes[i] += carry;
                    carry = our_bytes[i] >> 8
                    our_bytes[i] &= 0xFF

        # at this point, the negative flag has been set, and
        # our_bytes is the absolute value of the output
        # build up the little-endian value:

        value = 0
        for i in range(self.ListSize-1,-1, -1) :
            value <<= 8
            value += our_bytes[i]

        # we should be in business:

        body = ""
        pic = ""

        # Do an initial conversion to a string
        body += str(value)

        # We need to expand it by adding leading zeroes to get up
        # to the appropriate length

        length = 0
        if self.ListSize <= 1 :
            length = 3
        elif self.ListSize <= 2 :
            length = 5
        elif self.ListSize <= 4 :
            length = 10
        else :
            length = 20

        while len(body) < length :
            body = '0' + body

        pic += '['
        if (self.AttrFlags & COB_FLAG_HAVE_SIGN) != 0 :
            if negative :
                body = '-' + body
            else :
                body = '+' + body
            pic += " signed "
        else :
            pic += " unsigned "

        pic += "binary-"
        pic += str(self.ListSize*8)
        if self.AttrScale != 0 :
            pic += " ("
            pic += str(self.AttrDigits - self.AttrScale)
            pic += ','
            pic += str(self.AttrScale)
            pic += ")"

        pic += ' ]'
        return body, pic

    def NumericBinaryToString(self) :
        if self.Usage == "COMP-5" :
            body, pic = self.NumericBinaryToStringComp5()
            return body, pic

        # Generic Binary

        pcount = 0
        for ch in Picture(self.Picture) :
            if ch == 'P' :
                pcount += 1

        # we can be handed binary data in either little-endian
        # or big-endian.  We will operated little-endian
        our_bytes = self.data

        #our_bytes = [0x00, 0xb4, 0x21, 0x50, 0x1f, 0x01, 0x00, 0x00]
        #our_bytes.reverse();

        if (self.AttrFlags & COB_FLAG_BINARY_SWAP) != 0 :
            our_bytes.reverse()

        # we need to figure out if we are dealing with a signed
        # and negative number.

        negative = False
        if (self.AttrFlags & COB_FLAG_HAVE_SIGN) != 0 :
            # This is a binary number that can be signed; let's
            # see if it is.
            if (our_bytes[self.ListSize-1] & 0x80) != 0 :
                # and it is negative
                negative = True

                # we need to do a complement & increment.  We will
                # be fighting Python the whole way:

                carry = 1
                for i in range(self.ListSize) :
                    our_bytes[i] = ~our_bytes[i];
                    our_bytes[i] &= 0xFF
                    our_bytes[i] += carry;
                    carry = our_bytes[i] >> 8
                    our_bytes[i] &= 0xFF

        # at this point, the negative flag has been set, and
        # our_bytes is the absolute value of the output
        # build up the little-endian value:

        value = 0
        for i in range(self.ListSize-1,-1, -1) :
            value <<= 8
            value += our_bytes[i]

        # we should be in business:

        body = ""
        pic = ""

        # Do an initial conversion to a string, keeping in mind that because of
        # COBOL truncation issues, value may have more digits to the left than
        # we actually want to see.
        body += str(value)
        body = body[-self.AttrDigits:]

        # We need to expand it by adding leading zeroes to get up
        # to the appropriate length

        length = self.AttrDigits

        while len(body) < length :
            body = '0' + body

        if pcount > 0 :
            if self.AttrScale < 0 :
                for i in range(pcount) :
                    body = body[1:] + '0'
            else :
                for i in range(pcount) :
                    body = '0' + body[1:]
                body = '.' + body
        else :
            # Let's put in a decimal point, if necessary
            if  self.AttrScale > 0 :
                breakat = len(body) - self.AttrScale
                body = body[:breakat] + self.decimal_point + body[breakat:]

        pic += '['
        if (self.AttrFlags & COB_FLAG_HAVE_SIGN) != 0 :
            if negative :
                body = '-' + body
            else :
                body = '+' + body
            pic += " signed "
        else :
            pic += " unsigned "

        pic += "binary-"
        pic += str(self.ListSize*8)
        if self.AttrScale != 0 :
            pic += " ("
            pic += str(self.AttrDigits - self.AttrScale)
            pic += ','
            pic += str(self.AttrScale)
            pic += ")"

        pic += ' ]'
        return body, pic;


    def StringToNumericBinary(self,rside) :
        # Generic binary
        self.data = bytearray()
        if self.CleanAndScale(rside) :
            value = int(self.cleaned_rvalue,10)
            if self.is_negative :
                value = -value

            # Build up that value as little-endian
            self.data = bytearray(self.ListSize)
            for i in range(self.ListSize) :
                self.data[i] += value & 0xFF
                value = value // 256

            if self.AttrFlags & COB_FLAG_BINARY_SWAP :
                self.data.reverse()

    def AlphanumericToString(self) :
        # ALPHANUMERIC
        body = ""
        pic = ""
        body += '"'
        for i in range(len(self.data)) :
            if self.data[i] < 32 or self.data[i] >= 128 :
                body += self.decimal_point
            else :
                body += chr(self.data[i])
        body += '"'
        pic = '[ alphanumeric-'
        pic += str(self.ListSize)
        pic += ' ]'
        return body, pic;

    def StringToAlphanumeric(self,rside):
        # String from string:
        self.data = rside[0:self.ListSize]
        while len(self.data) < self.ListSize :
            self.data += ' '
        # convert to bytearray:
        self.data = bytearray(self.data,"utf8")

    def NumericEditedToString(self) :
        # NUMERIC(EDITED)
        body = ""
        pic = ""

        for i in range(self.ListSize) :
            if self.data[i] < 32 or self.data[i] >= 128 :
                body += '.'
            else :
                body += chr(self.data[i])

        if False :  # False means "Match observed GnuCOBOL v3.00rc1 behavior"

            # There seems to be an inconsistency in GnuCOBOL's behavior.

            # Given the value 1234.56,
            # a variable declared 9,999V99 will display as 1,23456
            # a variable declared 9,999.99 will display as 1,234.56
            # Both of those are represented internally as NUMERIC-EDITED

            # Meanwhile,
            # a variable declared 9999V99 will display as 1234.56
            # a variable declared 9999.99 will display as 1234.56
            # Both of those are represented internally as NUMERIC-DISPLAY

            # I haven't asked Simon Sobisch about this, and I haven't found
            # anything definitive in documentation avaialable on the Web; the
            # topic just seems too danged complex and there are too many
            # possibilities to enumerate them all.

            # I put in the "if False" up above to make cprint match cobc
            # behavior, mainly so that my automated selfcheck program can
            # work properly.  Making it "if True" will put the decimal point
            # back on display, which really seems like the right thing. I
            # have the feeling cobc should be changed, or at least the
            # inconsistency explained.

            # If self.Picture does not contain the decimal point character,
            # AND self.AttrScale indicates there are characters to the right
            # of a decimal point, then we need to insert one.

            if self.Picture.find(self.decimal_point) == -1 and self.AttrScale > 0 :
                body = body[:len(body)-self.AttrScale] + self.decimal_point + body[-self.AttrScale:]

        pic += '[ PIC '
        pic += self.Picture
        pic += ' ]'
        return body, pic;

    def StringToNumericEdited(self,rside) :
        self.data = bytearray()
        if self.CleanAndScale(rside) :
            # We have to look for are "floating insertion" symbols
            # A floating insertion range is indicated by duplicated '$', '+', or
            # '-' characters.  One caveat: the floating range can have embedded
            # commas in it.

            pic_without_commas = ""
            for ch in self.expanded_picture :
                if ch != self.comma_separator :
                    pic_without_commas += ch

            floating_insertion = None
            floating_insertion_start = None
            floating_insertion_end = None

            currency_doubled = self.currency_symbol + self.currency_symbol
            if pic_without_commas.find(currency_doubled) > -1 :
                floating_insertion = self.currency_symbol
            elif pic_without_commas.find("++") > -1 :
                floating_insertion = '+'
            elif pic_without_commas.find("--") > -1 :
                floating_insertion = '-'

            # find the start and end of the floating sequence in self.expanded_picture
            if floating_insertion :
                for i in range(len(self.expanded_picture)) :
                    if self.expanded_picture[i] == floating_insertion :
                        if floating_insertion_start == None:
                            floating_insertion_start = i
                        floating_insertion_end = i

            # We need to convert leading zeroes to the 'z' character:
            input = self.cleaned_rvalue

            # zero-flag the integer portion
            i = 0
            while i < self.ldigits :
                if input[i] == '0' :
                    input = input[:i] + 'z' + input[i+1:]
                else :
                    break;
                i += 1
            if i >= self.ldigits :
                # The entire integer part is zero.  How about the fractional part?
                if self.rdigits > 0 :
                    if int(input[-self.rdigits:]) == 0 :
                        # The fractional part is also zero; flag them, too
                        input = input[0:self.ldigits]
                        for i in range(self.rdigits) :
                            input += 'z'

            # Recap:  input is now abs(numeric input), with leading
            # zeroes to the left of the decimal position replaced with 'z'
            # (Or everthing is 'z' if the value was zero to begin with)

            # We will now walk the picture and input strings from right
            # to left, building the resulting string as we go

            pindex = len(self.expanded_picture)-1
            dindex = len(input)-1

            # This is, perforce, a state machine.  At this stage of development
            # is is mostly an ad hoc state machine.

            body = ""

            rightmost = True
            did_floating = False

            while pindex >= 0 :
                pch = self.expanded_picture[pindex]
                pindex -= 1

                if rightmost :
                    rightmost = False

                    if pch == '+' :
                        if self.is_negative :
                            body += '-'
                        else :
                            body += '+'
                        continue

                    if pch == '-' :
                        if self.is_negative :
                            body += '-'
                        else :
                            body += '+'
                        continue

                    if pch == 'c' :
                        # This is a CR (See the Picture() function)
                        if self.is_negative :
                            body += 'RC'    # We are building body backwards
                        else :
                            body += "  "
                        continue

                    if pch == 'd'  :
                        # This is a DB
                        if self.is_negative :
                            body += 'BD'
                        else :
                            body += "  "
                        continue

                if did_floating :
                    # Everything to the left of the floating_insertion is
                    # supposed to be a blank
                    body += ' '
                    continue;

                if not did_floating and pindex+1 == floating_insertion_start :
                    # The leftmost character of the floating_insertion has
                    # to be set, even at the cost of truncating the input
                    did_floating = True
                    if floating_insertion == '-' :
                        if self.is_negative :
                            body += '-'
                        else :
                            body += ' '
                        continue
                    if floating_insertion == '-' :
                        if self.is_negative :
                            body += '-'
                        else :
                            body += '+'
                        continue
                    if floating_insertion == self.currency_symbol :
                        body += self.currency_symbol
                        continue

                if "PV".find(pch) > -1 :
                    # We ignore scaling P characters here, because we processed
                    # them earlier by scaling the input string
                    # We also ignore the implicate decimal point 'V'
                    continue

                inside_floating = False
                if floating_insertion_start != None :
                    inside_floating = (pindex+1 >= floating_insertion_start and pindex+1 <= floating_insertion_end)

                include_literal = "B0/,."
                if not inside_floating:
                    include_literal += self.currency_symbol
                if include_literal.find(pch) > -1 :
                    body += pch
                    continue

                # After this point, we need to pull out an input character
                dch = input[dindex]
                dindex -= 1

                if inside_floating :
                    if dch == 'z' :
                        did_floating = True
                        body += floating_insertion
                        continue

                if pch == '9' or inside_floating:
                    # Explicit characters are expressed explicitly
                    if dch == 'z' :
                        dch = '0'
                    body += dch
                    continue

                if pch == 'Z' :
                    # Convert leading zeroes to blanks
                    if dch == 'z' :
                        dch = ' '
                    body += dch
                    continue

                if pch == '*' :
                    # Convert leading zeroes to asterisks
                    if dch == 'z' :
                        dch = '*'
                    body += dch
                    continue

            # We've built body.  Flip it right-side around
            body = body[::-1]

            # Depending on the numbers, we could have some interesting
            # entries.  Let's just power through them.

            # Get rid of commas preceded by spaces
            wipe_me = ' ' + self.comma_separator
            while True :
                i = body.find(wipe_me)
                if i == -1 :
                    break;
                body = body[:i] + "  " + body[i+2:]

            # Get rid of commas preceded by asterisks:
            wipe_me = '*' + self.comma_separator
            while True :
                i = body.find(wipe_me)
                if i == -1 :
                    break;
                body = body[:i] + "**" + body[i+2:]

            # Get rid of commas preceded by currency symbols:
            wipe_me = self.currency_symbol + self.comma_separator
            while True :
                i = body.find(wipe_me)
                if i == -1 :
                    break;
                body = body[:i] + ' ' + self.currency_symbol + body[i+2:]

            if (body[-2:] == 'CR' or body[-2:0] == 'DB') and body[-3] == ' '  :
                body = body[:-2] + "  "

            if (body[-2:] == 'CR' or body[-2:0] == 'DB') and body[-4:-2] == " ."  :
                body = body[:-2] + "  "

            if (body[-1:] == '+' or body[-1] == '-') and body[-2] == ' '  :
                body = body[:-1] + " "

            if (body[-1:] == '-' or body[-1] == '+') and body[-3:-1] == " ."  :
                body = body[:-1] + " "

            # If we ended up with just a decimal point, get rid of that:
            if body.strip() == self.decimal_point :
                i = body.find(self.decimal_point)
                body = body[:i] + ' ' + body[i+1:]

            self.data = bytearray(body.encode("utf8"))

    def DataToString(self) :
        noisy = False
        value = ""
        pic = "Unknown"
        if False :  # dummy code that can be followed by elifs
            pass
        elif self.AttrType == COB_TYPE_NUMERIC_DISPLAY :
            if noisy :
                print("COB_TYPE_NUMERIC_DISPLAY")
            value, pic = self.NumericDisplayToString()
        elif self.AttrType == COB_TYPE_NUMERIC_BINARY :
            if noisy :
                print("COB_TYPE_NUMERIC_BINARY")
            value, pic = self.NumericBinaryToString()
        elif self.AttrType == COB_TYPE_NUMERIC_PACKED :
            if noisy :
                print("COB_TYPE_NUMERIC_PACKED")
            value, pic = self.PackedDecimalToString()
        elif self.AttrType == COB_TYPE_NUMERIC_FLOAT :
            if noisy :
                print("COB_TYPE_NUMERIC_FLOAT")
            value, pic = self.IeeeBinaryToString()
        elif self.AttrType == COB_TYPE_NUMERIC_DOUBLE :
            if noisy :
                print("COB_TYPE_NUMERIC_DOUBLE")
            value, pic = self.IeeeBinaryToString()
        elif self.AttrType == COB_TYPE_NUMERIC_FP_DEC64 :
            if noisy :
                print("COB_TYPE_NUMERIC_FP_DEC64")
            value, pic = self.GnuCobolDecimal64ToString()
        elif self.AttrType == COB_TYPE_NUMERIC_FP_DEC128 :
            if noisy :
                print("")
            value, pic = self.GnuCobolDecimal128ToString()
        elif self.AttrType == COB_TYPE_NUMERIC_FP_BIN32 :
            if noisy :
                print("")
            value, pic = self.IeeeBinaryToString()
        elif self.AttrType == COB_TYPE_NUMERIC_FP_BIN64 :
            if noisy :
                print("")
            value, pic = self.IeeeBinaryToString()
        elif self.AttrType == COB_TYPE_NUMERIC_FP_BIN128 :
            if noisy :
                print("")
            value, pic = self.IeeeBinaryToString()
        elif self.AttrType == COB_TYPE_ALPHANUMERIC :
            if noisy :
                print("")
            value, pic = self.AlphanumericToString()
        elif self.AttrType == COB_TYPE_NUMERIC_EDITED :
            if noisy :
                print("COB_TYPE_NUMERIC_EDITED")
            value, pic = self.NumericEditedToString()
        else :
            # We were handed an unknown type.  Let's look at the data:
            non_printable = 0;
            for i in range(len(self.data)) :
                if self.data[i] < 32 or self.data[i] >= 128 :
                    non_printable += 1

            if non_printable >= 1 :
                value = "["
                for byte in self.data :
                    hexout = hex(byte)[-2:]
                    if hexout[0:1] == 'x' :
                        hexout = '0' + hexout[-1:]
                    value += hexout
                value += "]"
            else :
                value = '"'
                for byte in self.data :
                    if 32 <= byte < 128 :   # valid ASCII characters
                        value += chr(byte)
                    else :
                        value += '.'
                value += '"'

        pcount = 0
        for ch in Picture(self.Picture) :
            if ch == 'P' :
                pcount += 1
        pcount = 0

        if pcount > 0 :
            if self.AttrScale < 0 :
                for i in range(pcount) :
                    value += '0'
            else :
                stash = ''
                if value[0] == '+' or value[0] == '-' :
                    stash = value[0]
                    value = value[1:]
                for i in range(pcount) :
                    value = '0' + value
                value = stash + self.decimal_point + value

        ## Because we are in COBOL land, we replace any embedded quotes
        ## with doubled quotes:
        if value and value[0] == '"' :
            value = '"' + value[1:-1].replace('"','""') + '"'
        return value, pic;

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

    def ReadVariableString(self, string_name) :
        retval = ""

        # We are going to use the x/ns "memory as string" command, which reads
        # in n blocks of some number of characters.  On gdb 8.3, each block seems
        # to be 201 characters. I can't know if that's universal, so I am going
        # to make this code adaptive

        # We are going to read a single line of the string, in order to figure
        # how how many characters per line we're going to get:

        the_address = GetAddressOf(string_name)
        command = "x/1s " + str(the_address)
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
        return

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

        if file_and_line[0] != self.current_trapped_file :
            # We are in a new file
            self.current_trapped_file = file_and_line[0]

            # Find the address of the VARIABLE_STRING for this file

            name_root = self.current_trapped_file
            name_root = name_root.split('/')
            name_root = name_root[-1]
            name_root = name_root.replace('.','_')
            string_name = "VARIABLE_STRING_" + name_root.upper()

            self.ReadVariableString(string_name)

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

class ShortTermMemory() :
    def __init__(self) :
        self.short_term_memory = []

GV_ShortTermMemory = ShortTermMemory()

def MetaPossibilities(args,TabFile,ShortTermMemory,machine_interface_mode,rside) :
    possibilities = []
    if len(args) >= 1 :
        if args[0] == '*' :
            # If the first, presumably single, argument is an asterisk, then
            # he is asking for a list of variables and values:
            possibilities = TabFile.var_trie.GetAllPossibilities()
        elif args[0] == '?' :
            # If the first, presumably single, argument is a question mark,
            # then we need to round up the usual suspects.

            # But first, we are going to keep a rogue's gallery of the current list
            # suspects.  The idea here is that sometimes MOVE A TO B is the last line
            # of a PARAGRAPH, so as soon as it is executed control jumps elsewhere and
            # B falls out of context.  Or perhaps the next GDB command is a CONTINUE,
            # rather than NEXT.  By keeping one stage of stickiness, the display of variables
            # will be a bit more like what the user probably expects.

            if len(TabFile.current_trapped_file) > 0 :
                range = TabFile.question_mark_range;
                if range < 0 :
                    range = 1000000000
                return_string = subprocess.check_output(["cobcd-rw", \
                                                          TabFile.current_trapped_file, \
                                                          str(TabFile.current_trapped_line), \
                                                          str(range) ])
                tokens = return_string.split()
                new_dogs = set()
                for token in tokens :
                    token = token.decode("utf-8")
                    new_dogs = new_dogs.union(set(TabFile.var_trie.GetListOfExactPossibilities(token)))
                possibilities = list(TabFile.old_dogs.union(new_dogs))
                TabFile.old_dogs = new_dogs
        elif args[0].isdigit() :
            # if the first, presumably single, argument is a number, then he is looking
            # for a short_term_memory entry.

            # Pick up the 1-based value indexed by the argument:
            index = int(args[0])-1
            if index >= 0 and index < len(ShortTermMemory.short_term_memory) :
                # If index is valid, pick up the short-term memory entry
                possibilities = [ShortTermMemory.short_term_memory[index]]
        else :
            # There are one or more arguments.  Use our clever routine
            # to come up with the possibilities:
            possibilities = TabFile.var_trie.GetListOfPossibilities(args)

    # What we do now depends on how many possibilities there are
    possibilities.sort()

    if len(possibilities) == 1 and machine_interface_mode and not rside and args[0] != '*' and args[0] != '?' :
        # Very special case.  This was a `p/m something`, which is what happens when
        # the -data-evaluate-expression command is issued by Visual Studio Code
        payload_index = possibilities[0]
        payload = TabFile.var_trie.storage_list[payload_index]
        payload.FetchVariableData(TabFile.line_list , False)
        s = 'value="' + payload.display_body.replace(r'"',r'\"') + '"'
        print(s)
        possibilities = []
        return possibilities

    if len(possibilities) > 1 or machine_interface_mode :
        # There is more than one possibility:
        if machine_interface_mode :
            # This is machine interface mode
            # Our mission is to emulate the
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
            print(NoNulls(s))

        else :
            ShortTermMemory.short_term_memory = []

            # This is human-interactive mode
            # and show them to our user
            line_counter = 1
            for payload_index in possibilities :
                payload = TabFile.var_trie.storage_list[payload_index]
                payload.FetchVariableData(TabFile.line_list , False)
                if not payload.not_in_context :
                    ShortTermMemory.short_term_memory.append(payload_index)
                    s = "{0:2} : ".format(line_counter)
                    s += payload.display_name
                    s += " : "
                    s += payload.display_body
                    print(NoNulls(s))
                    line_counter += 1

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
        self.machine_interface_mode = False

    def ProcessDisplayMode(self,args) :
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

    def ProcessArguments(self, TabFile, ShortTermMemory, arguments) :
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
        args = self.ProcessDisplayMode(args)
        TabFile.question_mark_range = self.question_mark_range

        possibilities = MetaPossibilities(args,TabFile,ShortTermMemory,self.machine_interface_mode,rside)
        if len(possibilities) == 1 :
            # There is but one possibility
            # Pick up the payload from the list
            payload = TabFile.var_trie.storage_list[possibilities[0]]

            if rside :
                payload.SetToRside(TabFile.line_list, rside)

                if self.machine_interface_mode :
                    body = MachineInterfaceBody(payload.display_body)

                    s = payload.display_name + " set to " + body
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
        the_address = payload.GetTheAddress(TabFile.line_list)

        if not the_address and not 'gdb' in sys.modules :
            the_address = 0x12345678

        #print("SetTheWatchpoint", hex(the_address))

        command = "watch *(char(*)"
        command += '[' + str(payload.ListSize) +'])'
        command += '(' + hex(the_address) + ')'

        if 'gdb' in sys.modules :
            #print("The command is:",command)
            watch_text = gdb.execute(command,False,True)
            print(watch_text)
        else :
            print(command)

    def ProcessWArgumentsW(self, TabFile, ShortTermMemory, arguments) :
        # The command is typically "cw A

        args = arguments.split()

        possibilities = MetaPossibilities(args,TabFile,ShortTermMemory, False, False)

        if len(possibilities) == 1 :
            payload_number = possibilities[0]
            #print("Watch Payload number is ", payload_number)

            # Pick up the payload from the list
            payload = TabFile.var_trie.storage_list[payload_number]

            # And go set the Watchpoint
            self.SetTheWatchpoint(TabFile, payload)

if 'gdb' in sys.modules :
    class CPrint (gdb.Command) :
        """Prints COBOL-NAMED variables when debugging cobst processed object files."""

        def __init__ (self):
            super (CPrint, self).__init__ ("print", gdb.COMMAND_USER)
            ###########################################################################
            ## The version number in the next line is updated by a Linux bash script. #
            ## The text "Version x.x" has to appear in square brackets.               #
            ## Don't mess with it!                                                    #
            ###########################################################################
            print('registering CPrint (Usage is "print  <COBOL identifier>") [Version 3.12]')
            self.Worker = CPrintWorker()
            self.TabFile = TabFileInformation();
            self.TabFile.question_mark_range = self.Worker.question_mark_range

        def invoke (self, arguments, from_tty):
            # pick up the name of the current source file
            try :
                self.TabFile.ReadSymbolTableText()
                self.Worker.ProcessArguments(self.TabFile,GV_ShortTermMemory,arguments)
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
            try :
                self.TabFile.ReadSymbolTableText()
                self.Worker.ProcessWArgumentsW(self.TabFile,GV_ShortTermMemory,arguments)
            except :
                traceback.print_exc()

if 'gdb' not in sys.modules:
    # I debug with Visual Studio in Windows, which doesn't provide a gdb object.
    # Test code goes here
    #Worker = CPrintWorker()
    #Worker.ProcessInfoSource("|Located in C:\projects\1-symas\cbl-gdb\samples\ref_test_3\rtest.cbl ")
    pass
else:
    CPrint()
    CWatch()

