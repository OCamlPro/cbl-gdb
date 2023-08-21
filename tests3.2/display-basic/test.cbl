        >>SOURCE FREE
        ID                  DIVISION.
        PROGRAM-ID.         test.
        DATA                DIVISION.
        WORKING-STORAGE     SECTION.
        01  VAR-999         PIC         999         VALUE   123.
        01  VAR-S999        PIC         999         VALUE   123.
        01  VAR-S999N       PIC         S999        VALUE   -123.
        
        PROCEDURE DIVISION.
        GDB-START.
        DISPLAY             VAR-999  
        DISPLAY             VAR-S999 
        DISPLAY             VAR-S999N.
        GDB-END.
        GOBACK.
        END PROGRAM test.
