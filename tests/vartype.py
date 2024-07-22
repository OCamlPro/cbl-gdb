#!/usr/bin/python3

"""
This program generates a number of variables covering many definition
constructs for the GnuCOBOL compiler. This was written primarily to
create variables that are displayed by the CBL-GDB debugging extension.
Specifically, the generated program become part of the CBL-GDB
regression test suite.
"""

"""
vartype is the list of dictionary entries that are used to create the
COBOL variables.
"""
vartypes = \
  [
  {'usage':'DISPLAY','neg':'PICS','pic':'PIC','value':'V'},
  {'usage':'BINARY','neg':'PICS','pic':'PIC','value':'V'},
  {'usage':'BINARY-C-LONG','neg':'Yes','pic':'','value':'I'},
  {'usage':'BINARY-C-LONG SIGNED','neg':'Yes','pic':'','value':'I'},
  {'usage':'BINARY-C-LONG UNSIGNED','neg':'','pic':'','value':'I'},
  {'usage':'BINARY-CHAR','neg':'Yes','pic':'','value':'I'},
  {'usage':'BINARY-CHAR SIGNED','neg':'Yes','pic':'','value':'I'},
  {'usage':'BINARY-CHAR UNSIGNED','neg':'','pic':'','value':'I'},
  {'usage':'BINARY-DOUBLE','neg':'Yes','pic':'','value':'I'},
  {'usage':'BINARY-DOUBLE SIGNED','neg':'Yes','pic':'','value':'I'},
  {'usage':'SIGNED-LONG','neg':'Yes','pic':'','value':'I'},
  {'usage':'BINARY-DOUBLE UNSIGNED','neg':'','pic':'','value':'I'},
  {'usage':'UNSIGNED-LONG','neg':'','pic':'','value':'I'},
  {'usage':'BINARY-INT','neg':'Yes','pic':'','value':'I'},
  {'usage':'BINARY-LONG','neg':'Yes','pic':'','value':'I'},
  {'usage':'BINARY-LONG SIGNED','neg':'Yes','pic':'','value':'I'},
  {'usage':'SIGNED-INT','neg':'Yes','pic':'','value':'I'},
  {'usage':'BINARY-LONG UNSIGNED','neg':'','pic':'','value':'I'},
  {'usage':'UNSIGNED-INT','neg':'','pic':'','value':'I'},
  {'usage':'BINARY-LONG-LONG','neg':'Yes','pic':'','value':'I'},
  {'usage':'BINARY-SHORT','neg':'Yes','pic':'','value':'I'},
  {'usage':'BINARY-SHORT SIGNED','neg':'Yes','pic':'','value':'I'},
  {'usage':'SIGNED-SHORT','neg':'Yes','pic':'','value':'I'},
  {'usage':'BINARY-SHORT UNSIGNED','neg':'','pic':'','value':'I'},
  {'usage':'UNSIGNED-SHORT','neg':'','pic':'','value':'I'},
  {'usage':'COMP','neg':'PICS','pic':'PIC','value':'V'},
  {'usage':'COMPUTATIONAL','neg':'PICS','pic':'PIC','value':'V'},
  {'usage':'COMP-1','neg':'Yes','pic':'','value':'V'},
  {'usage':'COMPUTATIONAL-1','neg':'Yes','pic':'','value':'V'},
  {'usage':'FLOAT-SHORT','neg':'Yes','pic':'','value':'V'},
  {'usage':'COMP-2','neg':'Yes','pic':'','value':'V'},
  {'usage':'COMPUTATIONAL-2','neg':'Yes','pic':'','value':'V'},
  {'usage':'FLOAT-LONG','neg':'Yes','pic':'','value':'V'},
  {'usage':'COMP-3','neg':'Yes','pic':'PIC','value':'V'},
  {'usage':'COMPUTATIONAL-3','neg':'Yes','pic':'PIC','value':'V'},
  {'usage':'PACKED-DECIMAL','neg':'Yes','pic':'PIC','value':'V'},
  {'usage':'COMP-4','neg':'PICS','pic':'PIC','value':'V'},
  {'usage':'COMPUTATIONAL-4','neg':'PICS','pic':'PIC','value':'V'},
  {'usage':'COMP-5','neg':'PICS','pic':'PIC','value':'V'},
  {'usage':'COMPUTATIONAL-5','neg':'PICS','pic':'PIC','value':'V'},
  {'usage':'COMP-6','neg':'','pic':'PIC','value':'V'},
  {'usage':'COMPUTATIONAL-6','neg':'','pic':'PIC','value':'V'},
  {'usage':'COMP-X','neg':'PICS','pic':'PIC','value':'V'},
  {'usage':'COMPUTATIONAL-X','neg':'PICS','pic':'PIC','value':'V'},
  {'usage':'FLOAT-DECIMAL-16','neg':'Yes','pic':'','value':'V'},
  {'usage':'FLOAT-DECIMAL-34','neg':'Yes','pic':'','value':'V'},
  {'usage':'INDEX','neg':'','pic':'','value':'I'},
  {'usage':'POINTER','neg':'','pic':'','value':''},
  {'usage':'PROCEDURE-POINTER','neg':'','pic':'','value':''},
  {'usage':'PROGRAM-POINTER','neg':'','pic':'','value':''},
  ]

""" This is the list of variable declarations """
variable_declarations = list()

""" This is the list of DISPLAY statements """
display_statements = list()

""" These are the tab stops for making pretty COBOL lines """
tab_1 = 40
tab_2 = 53
tab_3 = 75

""" This is the line under construction """
line_being_built = ""

def tabstop(tab):
  """ Extends line_being_built out to the tabstop """
  global line_being_built
  while len(line_being_built) < tab:
    line_being_built += " "

def variable_type_1(varroot, vartype):
  """ Builds an unsigned variable """
  global line_being_built
  line_being_built = "        01 " + varroot
  if vartype["pic"]:
    tabstop(tab_1)
    line_being_built += " PIC  999V99"
  tabstop(tab_2)
  line_being_built += vartype["usage"]
  if vartype["value"]:
    tabstop(tab_3)
    if vartype["value"] == 'V':
      line_being_built += " VALUE  123.45"
    if vartype["value"] == 'I':
      line_being_built += " VALUE  123"
  line_being_built += "."
  variable_declarations.append(line_being_built)
  display_statements.append("        DISPLAY " + varroot + ".")

def variable_type_2(varroot, vartype):
  """ Builds a positive signed variable """
  global line_being_built
  if vartype["neg"]:
    varroot += "-P"
    line_being_built = "        01 " + varroot
    if vartype["pic"]:
      tabstop(tab_1)
      line_being_built += " PIC S999V99"
    tabstop(tab_2)
    line_being_built += vartype["usage"]
    if vartype["value"]:
      tabstop(tab_3)
      if vartype["value"] == 'V':
        line_being_built += " VALUE  123.45"
      if vartype["value"] == 'I':
        line_being_built += " VALUE  123"
    line_being_built += "."
    variable_declarations.append(line_being_built)
    display_statements.append("        DISPLAY " + varroot + ".")

def variable_type_3(varroot, vartype):
  """ Builds a negative signed variable """
  global line_being_built
  if vartype["neg"]:
    varroot += "-N"
    line_being_built = "        01 " + varroot
    if vartype["pic"]:
      tabstop(tab_1)
      line_being_built += " PIC S999V99"
    tabstop(tab_2)
    line_being_built += vartype["usage"]
    if vartype["value"]:
      tabstop(tab_3)
      if vartype["value"] == 'V':
        line_being_built += " VALUE -123.45"
      if vartype["value"] == 'I':
        line_being_built += " VALUE -123"
    line_being_built += "."
    variable_declarations.append(line_being_built)
    display_statements.append("        DISPLAY " + varroot + ".")

def variables():
  """ Generates the WORKING-STORAGE portion of the COBOL program """
  global line_being_built
  for vartype in vartypes:
    varroot = "VAR-" + vartype["usage"].replace(' ', '-')
    variable_type_1(varroot, vartype)
    variable_type_2(varroot, vartype)
    variable_type_3(varroot, vartype)

def header():
  print(
        "        >>SOURCE FREE\n"
        "        identification division.\n"
        "        program-id. test.\n"
        "        data division.\n"
        "        working-storage section.")

def middle():
  print("        procedure division.")

def trailer():
  print("        end program test.")

def main():
  """ main entry point of the application """
  variables()

  header()

  for variable_declaration in variable_declarations:
    print(variable_declaration)

  middle()

  for display_statement in display_statements:
    print(display_statement)

  trailer()

if __name__ == "__main__":
    """ This is executed when this file is invoked from the command line """
    main()