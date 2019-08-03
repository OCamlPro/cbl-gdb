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

        self.Insert(1000000,"Ragnarök")

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

        self.data           = []            # list of ListSize bytes

        # Here is the string information decoded from the attributes and
        # the data we get from gdb:

        self.not_in_context = False
        self.display_name = ""
        self.display_body = ""
        self.display_pic  = ""

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
        
    def FetchVariableData(self, line_list : LineList(), show_failure ) :
        data_string = None
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
                # We are running off-line, so simulate the return of zeroes
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

        #print("Here is our data_string: ", data_string)
        if data_string != None :
            bytes = data_string.split()
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
        if self.AttrType == 0 :
            print(" (Unknown)", end="")
        print("")
        print("AttrDigits: " , self.AttrDigits      )
        print("AttrScale:  " , self.AttrScale       )
        print("AttrFlags:  " , hex(self.AttrFlags)  )
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
        signed = (self.AttrFlags & COB_FLAG_NO_SIGN_NIBBLE) == 0

        body = ""
        pic = ""
        if signed :
            sign = self.data[self.ListSize-1] & 0x0F
            if sign in (0xf, 0xa, 0xc, 0xe) : # one learns not to ask silly questions
                body = '+';
            else :
                body = '-'

        nybble = 0
        if signed and (self.AttrDigits & 1) == 0 :
            nybble += 1;
        if not signed and (self.AttrDigits & 1) != 0 :
            nybble += 1;

        radix_point = self.AttrDigits - self.AttrScale
        dcount = 0
        while dcount < self.AttrDigits :
            if dcount == radix_point :
                body += '.'
            if (nybble & 1) == 0 :
                digit = self.data[nybble>>1] >> 4
            else :
                digit = self.data[nybble>>1] & 0x0F
            digit += ord('0')
            body += chr(digit)
            nybble += 1
            dcount += 1

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

    def NumericDisplayToString(self) :
        # numeric digits, but with a decimal point:
        body = ""
        pic = ""

        radix_point = self.ListSize - self.AttrScale

        for i in range(self.ListSize) :
            if i == radix_point :
                body += '.'
            digit = self.data[i]
            body += chr(digit)

        pic += "[ DISPLAY ("
        pic += str(self.AttrDigits - self.AttrScale)
        pic += ','
        pic += str(self.AttrScale)
        pic += ") ]"
        return body , pic

    def NumericBinaryToString(self) :
        # Generic Binary

        # we can be handed binary data in either little-endian
        # or big-endian.  We will operated little-endian
        our_bytes = self.data
        if self.AttrFlags & COB_FLAG_BINARY_SWAP :
            our_bytes.reverse()

        # we need to figure out if we are dealing with a signed
        # and negative number.

        negative = False
        if (self.AttrFlags & COB_FLAG_HAVE_SIGN) == 1 :
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

        if negative :
            value *= -1;

        # we should be in business:

        body = ""
        pic = ""

        body += str(value)
        if  self.AttrScale != 0 :
            breakat = len(body) - self.AttrScale
            body = body[:breakat] + '.' + body[breakat:]

        pic += '['
        if (self.AttrFlags & COB_FLAG_HAVE_SIGN) == 1 :
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

    def AlphanumericToString(self) :
        # ALPHANUMERIC
        body = ""
        pic = ""
        body += '"'
        for i in range(len(self.data)) :
            if self.data[i] < 32 or self.data[i] >= 128 :
                body += '.'
            else :
                body += chr(self.data[i])
        body += '"'
        pic = '[ alphanumeric-'
        pic += str(self.ListSize)
        pic += ' ]'
        return body, pic;

    def NumericEditedToString(self) :
        # NUMERIC(EDITED)
        body = ""
        pic = ""

        for i in range(self.ListSize) :
            if self.data[i] < 32 or self.data[i] >= 128 :
                body += '.'
            else :
                body += chr(self.data[i])
        pic += '[ PIC '
        pic += self.Picture
        pic += ' ]'
        return body, pic;

    def DataToString(self) :
        value = ""
        pic = "Unknown"
        if False :  # dummy code that can be followed by elifs
            pass
        elif self.AttrType == COB_TYPE_NUMERIC_DISPLAY :
            value, pic = self.NumericDisplayToString()
        elif self.AttrType == COB_TYPE_NUMERIC_BINARY :
            value, pic = self.NumericBinaryToString()
        elif self.AttrType == COB_TYPE_NUMERIC_PACKED :
            value, pic = self.PackedDecimalToString()
        elif self.AttrType == COB_TYPE_NUMERIC_FLOAT :
            value, pic = self.IeeeBinaryToString()
        elif self.AttrType == COB_TYPE_NUMERIC_DOUBLE :
            value, pic = self.IeeeBinaryToString()
        elif self.AttrType == COB_TYPE_NUMERIC_FP_DEC64 :
            value, pic = self.GnuCobolDecimal64ToString()
        elif self.AttrType == COB_TYPE_NUMERIC_FP_DEC128 :
            value, pic = self.GnuCobolDecimal128ToString()
        elif self.AttrType == COB_TYPE_NUMERIC_FP_BIN32 :
            value, pic = self.IeeeBinaryToString()
        elif self.AttrType == COB_TYPE_NUMERIC_FP_BIN64 :
            value, pic = self.IeeeBinaryToString()
        elif self.AttrType == COB_TYPE_NUMERIC_FP_BIN128 :
            value, pic = self.IeeeBinaryToString()
        elif self.AttrType == COB_TYPE_ALPHANUMERIC :
            value, pic = self.AlphanumericToString()
        elif self.AttrType == COB_TYPE_NUMERIC_EDITED :
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
        self.short_term_memory = []
        self.line_list = LineList()
        self.sticky_display_mode = os.environ.get('CPRINT_V', '0')
        self.current_display_mode = self.sticky_display_mode

    def ProcessInfoSource(self, info_source) :
        # This routine is given the results of the gdb "info source" command

        # A valid response starts with "Current source file is NC101A.cbl"
        result = re.search("^Current source file is (.+)\s", info_source)

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

    def ProcessDisplayMode(self,args) :
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
                print("default v-level can be set with the environment variable CPRINT_V=<n>")
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
        args = arguments.split()

        args = self.ProcessDisplayMode(args)

        payload_number = None
        if len(args) == 1 :
            if args[0] == '*' :
                # If the single argument is an asterisk, then
                # he's looking for help with program names
                if len(self.var_trie.program_ids) == 1 :
                    # There is exactly one program-id in our list of 
                    # program-ids, so show him those variables
                    args[0] = list(self.var_trie.program_ids)[0]
                else :
                    # Make short_term_memory a list of program-ids
                    self.short_term_memory = sorted(self.var_trie.program_ids)

                    # And display those program-ids:
                    line_counter = 1
                    for prog_id in self.short_term_memory :
                        print(line_counter, prog_id)
                        line_counter += 1
                    # Make sure we don't process the '*' any further
                    args = []
            
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
            # print("Payload number is ", payload_number)

            # Pick up the payload from the list
            payload = self.var_trie.storage_list[payload_number]

            # Fetch the data again, just in case he executed some instructions
            # since short-term-memory was established
            payload.FetchVariableData(self.line_list , True)

            # And display the variable according to the current list v-level
            self.FormattedDisplay(payload)

if 'gdb' in sys.modules :
    class CPrint (gdb.Command) :
        """Prints COBOL-NAMED variables when debugging cobst/obmod processed object files."""

    # Because I couldn't get this to work automatically, even though it ought to
    # when put into data-directory/gdb/python:
    #
    # https://sourceware.org/gdb/current/onlinedocs/gdb/Python.html#Python
    #
    # I created a ~/.gdbinit file, which includes
    #       source /path/to/thisfile.py

        def __init__ (self):
            super (CPrint, self).__init__ ("print", gdb.COMMAND_USER)
            print('registering CPrint v0.4.3 , (Usage is "print <COBOL identifier>")')
            self.Worker = CPrintWorker()

        def invoke (self, arguments, from_tty):
            # pick up the name of the current source file
            info_source = gdb.execute("info source",False,True)
            self.Worker.ProcessInfoSource(info_source)
            self.Worker.ProcessArguments(arguments)
            
if 'gdb' not in sys.modules:
    # I debug with Visual Studio in Windows, which doesn't provide a gdb object.
    # Test code goes here
    Worker = CPrintWorker()
    Worker.ProcessInfoSource(r"Current source file is C:\projects\1-symas\batch-demo\gdb\samples\optfde01\optfde01.cbl ")
    Worker.ProcessArguments("*")
    Worker.ProcessArguments("22")
    Worker.ProcessArguments("22")
    Worker.ProcessArguments("/v")
    Worker.ProcessArguments("22")
    Worker.ProcessArguments("22")
    Worker.ProcessArguments("/v0")
    Worker.ProcessArguments("22")
    Worker.ProcessArguments("22")

    Worker.ProcessArguments("/vd 22")

    pass
else:                
    CPrint()
