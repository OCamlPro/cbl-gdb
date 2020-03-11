Because, sadly and inevitably, the Makefiles and compilation scripts have
become almost completely incomprehensible, this simplified explanation of the
compile-COBOL-for debugging process is offered:

1)  Invoke GnuCOBOL, with -g and -d debugging specified, to compile the COBOL
    source to create intermediate .c, .h, and mixed-language (C + assembly)
    listings.  This compilation uses the --fgen-c-line-directives switch, which
    puts #line references to the original COBOL code into the generated .c
    code.

2)  Invoke the Symas COBST program.  It reads those intermediate files and
    creates a .sym.c source code module.

3)  Invoke GCC to compile the .sym.c module.  This creates a .o module that
    contains COBOL symbol table information (including information about
    VARIABLES-WITH-HYPHENS) and the Python extension script to GDB.

4)  Invoke GCC to create a .s assembly language file from the intermediate .c
    file created by GnuCOBOL.
    
5)  Invoke the Symas SFIX program, which modifies the .s file by removing all
    internal debugging references to the intermediate .c file.  This is
    necessary because otherwise the assembler intermixes the explicit #line
    references to the COBOL code with the implicit references to the line
    numbers of the C code.  The results, as displayed by GDB, are meaningless;
    getting rid of location references to the .C source fixes the problem.

6)  Invoke the GCC/AS assembler to assemble the modified .s code into a .o
    module.
    
7)  Feed the two .o modules into GnuCOBOL to create the executable.

This sounds worse than it is, because all of that has been incorporated into
a script, only 224 lines long, named "cobcd".