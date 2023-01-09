        IDENTIFICATION DIVISION.
        PROGRAM-ID. C.
        DATA DIVISION.
        WORKING-STORAGE SECTION.
        01 CALL-BLOCK.
            05 WE-ARE-C PIC X(4) VALUE "C".
            05 WS-INDENT PIC 9.
        LINKAGE SECTION.
        COPY LINK.
        PROCEDURE DIVISION USING CALLED-BLOCK.
        ADD 2 TO INDENT GIVING WS-INDENT
        COPY INDENTER.
        DISPLAY "I am '" WE-ARE-C "', called from '" CALLED-FROM "'"
            WITH NO ADVANCING
        DISPLAY "; I call nobody"
        GOBACK.
