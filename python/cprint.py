#   This module implements the gdb extension command cprint
#
#   It reads symbol table data from the .tab file generated
#   by the cobst program, and then uses that to script its
#   way through gdb commands to get the actual data so that
#   the contents of COBOL-NAMED variables (complete with
#   hyphens) can be displayed by gdb .
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

import re
import sys
import os

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

def FlagLeadingZeroes(s) :
    """ Replace leading zeroes with 'z' """
    r = ""
    z = 'z'
    for ch in s :
        if ch == '0' :
            r += z
        else :
            r += ch
            z = '0'
    return r

def MachineInterfaceBody(s) :
    ### convert display_body for machine interface:
    body = s
    body_length = 0
    is_quoted = False

    # Strip off leading and trailing square brackets, which is what sometimes
    # get before some variable types (e.g., file records) are initialized
    #if len(body) >=2 and body[0] == '[' :
    #    body = body[1:-1]   

    # Strip off leading and trailing double-quotes
    if len(body) >=2 and body[0] == '"' :
        is_quoted = True
        body = body[1:-1]   

    # Whatever is left is now the actual number of characters
    body_length = len(body)

    # Prefix double-quotes and backslashes with backslashes:
    temp = ""
    for ch in body :
        if ch == '"'  or ch == '\\':
            temp += '\\'
        temp += ch

    return body , body_length

class VarTrieNode() :
    def __init__(self):
        self.parent = None
        self.children = {}
        self.piece_of_name = ""
        self.count = 0
        self.payload_index = None

    def AddCanonicalName(self,name) :
        pass

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

    def ReadFromFile(self,filename) :
        # Clear us out
        self.storage_list = []
        self.storage_trie = VarTrieNode()
        self.program_ids = set()
        
        # current_source_file is probably something.cbl
        # convert it to something.tab

        # bust up the filename on "."
        split_file_name = filename.split('.')
        split_symbol_file = split_file_name

        # throw away the final extension
        split_symbol_file.pop()

        # replace it with .tab
        split_symbol_file.append("tab")
        symbol_table_file = ".".join(split_symbol_file)

        try :
            tab_file = open(symbol_table_file,"r")
        except :
            print(symbol_table_file, "doesn't exist")
            return

        line = tab_file.readline()

        # Break up the header line by the separating tabs:
        headers = line.rstrip('\n').split("\t")

        # Find the column numbers from the header names:
        # These have to match the code in cobst.

        indexLineNumber = headers.index("Line Number")
        indexProgram    = headers.index("Program Name")
        indexDivision   = headers.index("Division")
        indexSection    = headers.index("Section")
        indexName       = headers.index("Full Name")
        indexLevel      = headers.index("Level")
        indexBase       = headers.index("Base Symbol")
        indexField      = headers.index("Field Symbol")
        indexAttr       = headers.index("Attribute Symbol")
        indexOffset     = headers.index("Offset")
        indexListType   = headers.index("List Type")
        indexListSize   = headers.index("List Size")
        indexPicture    = headers.index("Picture")
        indexUsage      = headers.index("Usage")
        indexOccurs     = headers.index("Occurs")
        indexFieldSize  = headers.index("Field Size")
        indexAttrType   = headers.index("Attr Type")
        indexAttrDigits = headers.index("Attr Digits")
        indexAttrScale  = headers.index("Attr Scale")
        indexAttrFlags  = headers.index("Attr Flags")
        
        current_program = ""
        while True :
            line = tab_file.readline()
            if line == "" :
                break;

            tokens = line.rstrip('\n').split("\t")

            if tokens[indexDivision] == "Data" :
                name_tokens =  tokens[indexName].split("/")
                if len(name_tokens) >= 1 :
                    payload = CobolVariable()

                    payload.Program    = tokens[indexProgram       ]
                    payload.Section    = tokens[indexSection       ]
                    payload.Name       = tokens[indexName          ]
                    payload.Level      = int(tokens[indexLevel     ])
                    payload.Base       = tokens[indexBase          ]
                    payload.Field      = tokens[indexField         ]
                    payload.Attr       = tokens[indexAttr          ]
                    payload.Offset     = int(tokens[indexOffset    ])
                    payload.ListType   = tokens[indexListType      ]
                    payload.ListSize   = int(tokens[indexListSize  ])
                    payload.Picture    = tokens[indexPicture       ]
                    payload.Usage      = tokens[indexUsage         ]
                    payload.Occurs     = int(tokens[indexOccurs    ])
                    payload.FieldSize  = int(tokens[indexFieldSize ])
                    payload.AttrType   = int(tokens[indexAttrType  ])
                    payload.AttrDigits = int(tokens[indexAttrDigits])
                    payload.AttrScale  = int(tokens[indexAttrScale ])
                    payload.AttrFlags  = int(tokens[indexAttrFlags ])

                    # If the COBOL source code doesn't refer to a variable,
                    # then we don't have much information about it.  But
                    # we can look at the payload.ListType and at least 
                    # identify ALPHANUMERICS:

                    if payload.ListType in ("ALPHABETIC","ALPHANUMERIC") :
                        payload.AttrType = 33 ;

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
                    name_pieces = payload.Name.split("/")
                    if len(name_pieces) >= 1 :
                        name_piece = name_pieces.pop()
                        self.program_ids.add(name_piece)

        tab_file.close()

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
                    boys:[],
                    boy_index:int,
                    trie:VarTrieNode,
                    paths:[]) :
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
            #print(boy)
            #print(trie.children.keys())
            if boy in trie.children :
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
# The following code was commented out because Marty Heyman wants to see
# the whole group when a group is selected by name.  I'm leaving the commented
# code in for now in case the decision gets refined.  If we revisit, it'll
# save a few minutes if we want to expand on GROUPness. Bob Dubner, 2019-08-01
#
#            if self.storage_list[index].ListType == "GROUP" :
#                level = self.storage_list[index].Level
#                index += 1
#                while index < len(self.storage_list) :
#                    if self.storage_list[index].Level <= level \
#                             or self.storage_list[index].Level > 49 :
#                        break
#                    # Note that the list is sorted by heirarchy
#                    paths.append(index)
#                    index += 1

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
        self.EndOfTheWorld = 1000000

    def Insert(self,line_number : int, program_id : str) :
        self.line_list.append( (line_number,program_id) )

    def Find(self,line_number : int) :
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

    def ReadFromFile(self,filename) :
        # current_source_file is probably something.cbl
        # convert it to something.tab

        # bust up the filename on "."
        split_file_name = filename.split('.')
        split_symbol_file = split_file_name

        # throw away the final extension
        split_symbol_file.pop()

        # replace it with .tab
        split_symbol_file.append("tab")
        symbol_table_file = ".".join(split_symbol_file)

        try :
            tab_file = open(symbol_table_file,"r")
        except :
            print(symbol_table_file, "doesn't exist")
            return

        line = tab_file.readline()

        # Break up the header line by the separating tabs:
        headers = line.rstrip('\n').split("\t")

        # Find the column numbers from the header names:
        # These have to match the code in cobst.

        indexLineNumber = headers.index("Line Number")
        indexProgram    = headers.index("Program Name")
        indexDivision   = headers.index("Division")
        indexSection    = headers.index("Section")
        indexName       = headers.index("Full Name")
        indexLevel      = headers.index("Level")
        indexBase       = headers.index("Base Symbol")
        indexField      = headers.index("Field Symbol")
        indexAttr       = headers.index("Attribute Symbol")
        indexOffset     = headers.index("Offset")
        indexListType   = headers.index("List Type")
        indexListSize   = headers.index("List Size")
        indexPicture    = headers.index("Picture")
        indexUsage      = headers.index("Usage")
        indexOccurs     = headers.index("Occurs")
        indexFieldSize  = headers.index("Field Size")
        indexAttrType   = headers.index("Attr Type")
        indexAttrDigits = headers.index("Attr Digits")
        indexAttrScale  = headers.index("Attr Scale")
        indexAttrFlags  = headers.index("Attr Flags")

        current_program = ""        
        while True :
            line = tab_file.readline()
            if line == "" :
                break;

            tokens = line.rstrip('\n').split("\t")

            if tokens[indexDivision] == "Procedure" :
                program_id = tokens[indexProgram]
                line_number = int(tokens[indexLineNumber])
                if program_id != current_program :
                    self.Insert(line_number,program_id)
                    current_program = program_id

        self.Insert(self.EndOfTheWorld,"Ragnarök")

        tab_file.close()

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
        self.Attr       = ""    
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

    def SetToRside(self, line_list : LineList(), rside) :
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
                        print(command)
                        print(address_string)
                        address_string = gdb.execute(command,False,True)

                        match = re.search("address 0x([0-9a-f]+)\.",address_string)
                        if not match :
                            print("No match")
                        else :
                            print(match.group(1))
                            address = int(match.group(1),16)
                            address += self.Offset
                            return address
                    else :
                        return None

                except gdb.error:
                    return None
        else :
            return None
        
    def FetchVariableData(self, line_list : LineList(), show_failure ) :
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
                    #print(command)
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

                        #print(command)
                        data_string = gdb.execute(command,False,True)
                        #print(data_string)
                    else :
                        if show_failure :
                            print("Not in context ( we are stopped in", program_id,")")
                        self.not_in_context = True

                except gdb.error:
                    print("Not in context (gdb.error)")
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
        print("Attr:       " , self.Attr            )
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
        for byte in self.data :
            hexout = hex(byte)[-2:]
            if hexout[0:1] == 'x' :
                hexout = '0' + hexout[-1:]
            print(hexout,end=" ")
        print("]")
        print("pic_data: (ASCII) \"", end = "")
        for byte in self.data :
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

        return value, pic;

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
        self.var_trie = VarTrie()
        self.current_source_file = ""
        self.current_trapped_routine = ""
        self.short_term_memory = []
        self.line_list = LineList()
        self.sticky_display_mode = os.environ.get('CPRINT_V', '0')
        self.current_display_mode = self.sticky_display_mode
        self.machine_interface_mode = False

    def ProcessInfoSource(self, info_source) :
        # This routine is given the results of the gdb "info source" command

        info_source = info_source.replace('\n',' |')
        # A valid response starts with "Located in /path/to/NC101A.cbl"
        result = re.search("\|Located in (.+?) ", info_source)

        if result:
            source_file = result.group(1)
            if source_file != self.current_source_file:
                # We've been given a new source file name, so look up
                # the COBOL identifiers in it:
                self.current_source_file = source_file

                # Load the new tables
                self.var_trie = VarTrie()
                self.var_trie.ReadFromFile(source_file)

                # Load the new line list:
                self.line_list = LineList()
                self.line_list.ReadFromFile(source_file)

        else :
            # There isn't a current source file.
            self.current_source_file = ""

    def ProcessInfoStack(self, info_stack) :
        """ Figures out which routine we are currently trapped in"""

        # Info line should start with "Line xxx "
        self.current_trapped_routine = ""

        m = info_stack.find('\n')
        if m > -1 :
            info_stack = info_stack[:m]

        m = info_stack.find(':')
        if(m > -1) :
            # So far we are well formed
            line_number = int(info_stack[m+1:])

            for xxx in self.line_list.line_list :
                if xxx[0] > line_number :
                    # We've gotten past line_number, so the prior entry
                    # was the one we wanted
                    break;
                # Keep looking
                self.current_trapped_routine = xxx[1]

    def ProcessDisplayMode(self,args) :
        self.machine_interface_mode = False
        # Make sure the current display mode is the same as the session mode
        self.current_display_mode = self.sticky_display_mode 

        # Now see if he wants to change it
        if len(args) >= 1 :
            if args[0] == '/h' :
                print("COBOL print command usage:")
                print("'print/h' prints this message.")
                print("'print <arg>' shows <arg> formatted as specified below")
                print("'print/v<n>' formats print results:")
                print("'print/v<n>' with no arguments sets the session formatting")
                print("'print/v<n> <arg>' displays <arg> with a one-time formatting override")
                print("'print/v0' : just the variable  (this is the default)")
                print("'print/v1' : variable and picture-string")
                print("'print/v2' : variable and '01 full/name'")
                print("'print/v3' : variable, pic-string, and '01 full/name'")
                print("'print/v4' : two-line display with name, variable, and pic-string")
                print("'print/v'  : same as /v4")
                print("'print/vd' or print/d : debug output")
                print("'print/m'  : generate output compatible with '-stack-list-variables --simple-values'")
                print("default v-level can be set with the environment variable CPRINT_V=<n>")
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
                self.current_display_mode = '4'
                args.pop(0)
            elif len(args[0]) >= 3 and args[0][0:2] == '/v' :
                # This is a display v-code:
                v_code = args[0][2]
                args.pop(0)
                if v_code in "01234d" :
                    # It's a valid display_mode
                    self.current_display_mode = v_code

        # if len(args) is down to zero, then any /v code established here
        # becomes the new session default.  Otherwise, it'll apply only
        # once
        if len(args) == 0 :
            self.sticky_display_mode = self.current_display_mode

        return args

    def FormattedDisplay(self,payload : CobolVariable()) :
        # see the comments in ProcessDisplayMode()
        if self.current_display_mode == '0' :
            print(payload.display_body)
            pass
        elif self.current_display_mode ==  '1' :
            print(payload.display_body," : ",payload.display_pic)
            pass
        elif self.current_display_mode ==  '2' :
            print(payload.display_body," : ",payload.display_name)
            pass
        elif self.current_display_mode ==  '3' :
            print(payload.display_body," : ",
                  payload.display_pic," : ",
                  payload.display_name)
            pass
        elif self.current_display_mode ==  '4' :
            print(payload.display_name)
            print(payload.display_body," : ",payload.display_pic)
            pass
        else : # self.current_display_mode ==  'd' :
            payload.ShowFields()
            pass

    def ProcessArguments(self, arguments) :
        # The command is typically "cp A B C" or "cp/d A B C"
        # There is also the possibility of an '=' character, whereby the
        # value after the '=' is assigned to the variable described to the left
        
        preargs = arguments.split('=',1)
        
        rside = None
        if len(preargs) > 1  :
            rside = preargs[1]
        
        # The variable name side is to the left of any '=' character
        args = preargs[0].split()

        args = self.ProcessDisplayMode(args)

        payload_number = None
        pstar = False
        if len(args) == 1 :
            if args[0] == '*' :
                pstar = True
                # If the single argument is an asterisk, then
                # he is asking for a list of variables and values:
                args[0] = self.current_trapped_routine
            
            elif args[0].isdigit() :
                # if the single argument is a number, then he is looking
                # for a short_term_memory entry.  If the short-term entry is 
                # alphabetic, then it is a reference to a program-id.  If it
                # is numeric, it's a payload number

                # Pick up the value indexed by the argument:
                index = int(args[0])

                if index > 0 and index <= len(self.short_term_memory) :
                    # If index is valid, pick up the short-term memory entry
                    short_term_entry = self.short_term_memory[index-1]
                    if isinstance(short_term_entry, int) :
                        # It is an integer; look up the payload number:
                        payload_number = self.short_term_memory[index-1]
                    else :
                        # It is text, meaning it is a program-id.  Pass it
                        # along for expansion
                        args[0] = short_term_entry

        if len(args) >= 1 :
            if payload_number == None :
                # There are one or more arguments.  Use our clever routine
                # to come up with the possibilities:
                possibilities = self.var_trie.GetListOfPossibilities(args)
                if len(possibilities) == 1 and not pstar:
                    # There's only one possibility, and it wasn't a star. 
                    # This is nice and simple.
                    # We'll just move ahead and process it.
                    payload_number = possibilities[0]
                elif len(possibilities) > 1 or (len(possibilities) > 0 and pstar) :
                    # There are multiple possibilities, or there is one 
                    # possibility found via a 'p *'.
                    # Load them into short-term memory:
                    self.short_term_memory = possibilities

                    if self.machine_interface_mode :
                        # This is machine interface mode
                        # Our mission is to emulate the 
                        # '-stack-list-variables --simple-values'

                        s = "variables=["
                        for payload_index in possibilities :
                            payload = self.var_trie.storage_list[payload_index]
                            payload.FetchVariableData(self.line_list , False)

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

                            body , body_length = MachineInterfaceBody(payload.display_body)

                            s += '{name="'
                            s += name
                            s += '"'

                            s += ',type="char *"'

                            s += r',value="'
                            s += payload.location 
                            s += r' \"'
                            s += body
                            s += r'\""},'
                        if s[-1] == ',' :
                            s = s[:-1]  # Strip off final comma
                        s += ']'
                        print(s)

                    else :
                        # This is human-interactive mode
                        # and show them to our user
                        line_counter = 1
                        for payload_index in possibilities :
                            payload = self.var_trie.storage_list[payload_index]
                            payload.FetchVariableData(self.line_list , False)
                            s = "{0:2} : ".format(line_counter)
                            s += payload.display_name
                            s += " : "
                            s += payload.display_body
                            print(s)
                            line_counter += 1
                    payload_number = None

        # At this point, if we have a single payload, it's time to show
        # that to our user:
        if payload_number != None :
            # print("Payload number is ", payload_number)

            # Pick up the payload from the list
            payload = self.var_trie.storage_list[payload_number]
            
            if rside :
                payload.SetToRside(self.line_list, rside)

            # Fetch the data again, just in case he executed some instructions
            # since short-term-memory was established
            payload.FetchVariableData(self.line_list , True)

            if self.machine_interface_mode :
                body , body_length = MachineInterfaceBody(payload.display_body)

                s = payload.display_name + " set to " + body
                print(s)
            else :
                # Human-interface:
                # And display the variable according to the current list v-level
                self.FormattedDisplay(payload)

class CWatchWorker() :
    """Does most of the non-gdb work of CWatch()"""

    def __init__(self):
        self.var_trie = VarTrie()
        self.current_source_file = ""
        self.current_trapped_routine = ""
        self.short_term_memory = []
        self.line_list = LineList()
        self.current_display_mode = "0"

    def ProcessDisplayModeW(self,args) :
        if len(args) >= 1 :
            if args[0] == '/h' :
                print("COBOL cwatch command usage:")
                print("'cwatch/h' prints this message.")
                print("'cwatch/vd' or cwatch/d : debug output")
                print("'cwatch/v0' back to default")
                args = [""]
                return args

            # if the first argument is /d, turn on our debug flag:
            if args[0] == '/d' :
                self.current_display_mode = 'd'
                args.pop(0)
            elif args[0] == '/v' :
                self.current_display_mode = '4'
                args.pop(0)
            elif len(args[0]) >= 3 and args[0][0:2] == '/v' :
                # This is a display v-code:
                v_code = args[0][2]
                args.pop(0)
                if v_code in "0d" :
                    # It's a valid display_mode
                    self.current_display_mode = v_code

        # if len(args) is down to zero, then any /v code established here
        # becomes the new session default.  Otherwise, it'll apply only
        # once
        if len(args) == 0 :
            self.sticky_display_mode = self.current_display_mode

        return args

    def ProcessInfoSourceW(self, info_source) :
        # This routine is given the results of the gdb "info source" command

        info_source = info_source.replace('\n',' |')
        # A valid response starts with "Located in /path/to/NC101A.cbl"
        result = re.search("\|Located in (.+?) ", info_source)

        if result:
            source_file = result.group(1)
            if source_file != self.current_source_file:
                # We've been given a new source file name, so look up
                # the COBOL identifiers in it:
                self.current_source_file = source_file

                # Load the new tables
                self.var_trie = VarTrie()
                self.var_trie.ReadFromFile(source_file)

                # Load the new line list:
                self.line_list = LineList()
                self.line_list.ReadFromFile(source_file)

        else :
            # There isn't a current source file.
            self.current_source_file = ""

    def ProcessInfoStackW(self, info_stack) :
        """ Figures out which routine we are currently trapped in"""

        # Info line should start with "Line xxx "
        self.current_trapped_routine = ""

        m = info_stack.find('\n')
        if m > -1 :
            info_stack = info_stack[:m]

        m = info_stack.find(':')
        if(m > -1) :
            # So far we are well formed
            line_number = int(info_stack[m+1:])

            for xxx in self.line_list.line_list :
                if xxx[0] > line_number :
                    # We've gotten past line_number, so the prior entry
                    # was the one we wanted
                    break;
                # Keep looking
                self.current_trapped_routine = xxx[1]

    def SetTheWatchpoint(self,payload : CobolVariable()) :
        the_address = payload.GetTheAddress(self.line_list)

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

    def ProcessArgumentsW(self, arguments) :
        # The command is typically "cw A 
        
        args = arguments.split()
        args = self.ProcessDisplayModeW(args)

        payload_number = None
        if len(args) == 1 :
            if args[0] == '*' :
                # If the single argument is an asterisk, then
                # he's looking for a list of variables
                args[0] = list(self.current_trapped_routine)
            
            elif args[0].isdigit() :
                # if the single argument is a number, then he is looking
                # for a short_term_memory entry.  If the short-term entry is 
                # alphabetic, then it is a reference to a program-id.  If it
                # is numeric, it's a payload number

                # Pick up the value indexed by the argument:
                index = int(args[0])

                if index > 0 and index <= len(self.short_term_memory) :
                    # If index is valid, pick up the short-term memory entry
                    short_term_entry = self.short_term_memory[index-1]
                    if isinstance(short_term_entry, int) :
                        # It is an integer; look up the payload number:
                        payload_number = self.short_term_memory[index-1]
                    else :
                        # It is text, meaning it is a program-id.  Pass it
                        # along for expansion
                        args[0] = short_term_entry

        if len(args) >= 1 :
            if payload_number == None :
                # There are one or more arguments.  Use our clever routine
                # to come up with the possibilities:
                possibilities = self.var_trie.GetListOfPossibilities(args)
                if len(possibilities) == 1 :
                    # There's only one possibility, which is nice and simple.
                    # We'll just move ahead and process it.
                    payload_number = possibilities[0]
                elif len(possibilities) > 1 :
                    # There are multiple possibilities.  Load them into
                    # short-term memory:
                    self.short_term_memory = possibilities

                    # and show them to our user
                    line_counter = 1
                    for payload_index in possibilities :
                        payload = self.var_trie.storage_list[payload_index]
                        payload.FetchVariableData(self.line_list , False)
                        s = "{0:2} : ".format(line_counter)
                        s += payload.display_name
                        s += " : "
                        s += payload.display_body
                        print(s)
                        line_counter += 1

        # At this point, if we have a single payload, it's time to show
        # that to our user:
        if payload_number != None :
            #print("Watch Payload number is ", payload_number)

            # Pick up the payload from the list
            payload = self.var_trie.storage_list[payload_number]

            # And go set the Watchpoint
            self.SetTheWatchpoint(payload)

if 'gdb' in sys.modules :
    class CPrint (gdb.Command) :
        """Prints COBOL-NAMED variables when debugging cobst/obmod processed object files."""

        def __init__ (self):
            super (CPrint, self).__init__ ("print", gdb.COMMAND_USER)
            print('registering CPrint (Usage is "print  <COBOL identifier>") [Version 0.8.8]')
            self.Worker = CPrintWorker()

        def invoke (self, arguments, from_tty):
            # pick up the name of the current source file
            info_source = gdb.execute("info source",False,True)
            self.Worker.ProcessInfoSource(info_source)
            info_stack = gdb.execute("info stack",False,True)
            self.Worker.ProcessInfoStack(info_stack)
            self.Worker.ProcessArguments(arguments)

    class CWatch (gdb.Command) :
        """Used for creating COBOL-NAMED watchpoints when debugging cobst/obmod processed object files."""

        def __init__ (self):
            super (CWatch, self).__init__ ("cwatch", gdb.COMMAND_USER)
            print('registering CWatch (Usage is "cwatch <COBOL identifier>")')
            self.Worker = CWatchWorker()

        def invoke (self, arguments, from_tty):
            # pick up the name of the current source file
            info_source = gdb.execute("info source",False,True)
            self.Worker.ProcessInfoSourceW(info_source)
            info_stack = gdb.execute("info stack",False,True)
            self.Worker.ProcessInfoStackW(info_stack)
            self.Worker.ProcessArgumentsW(arguments)
            
if 'gdb' not in sys.modules:
    # I debug with Visual Studio in Windows, which doesn't provide a gdb object.
    # Test code goes here
    Worker = CPrintWorker()
    #Worker.ProcessInfoSource("|Located in C:\projects\1-symas\cbl-gdb\samples\ref_test_3\rtest.cbl ")
    #Worker.ProcessInfoSource("|Located in Y:\cobol\cbl-gdb\selfcheck\program_under_test.cbl ")
    #Worker.ProcessInfoSource("|Located in Y:\cobol\cbl-gdb\samples\ref_test_1\rtest.cbl ")
    #Worker.ProcessInfoStack("Line 60 ")
    Worker.ProcessInfoSource("|Located in Y:/cobol/cbl-gdb/samples/ref_test_0/rtest.cbl ")
    Worker.ProcessInfoStack("Line:4\n")
    Worker.ProcessArguments("*")
    Worker.ProcessArguments("/m *")

#    Worker.ProcessArguments("*")
#    Worker.ProcessArguments("/m *")
#    Worker.ProcessArguments("/m my-counters/working-data/optfde01")
    #Worker.ProcessArguments("V=.0000999999999")

#    WWorker = CWatchWorker()
#    WWorker.ProcessInfoSourceW("|Located in C:\projects\1-symas\cbl-gdb\samples\ref_test_3\rtest.cbl ")
#    Worker.ProcessInfoStackW("Line 34 ")
#    WWorker.ProcessArgumentsW("*")
    pass
else:                
    CPrint()
    CWatch()

