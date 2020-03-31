        IDENTIFICATION DIVISION.                                        Col73
        PROGRAM-ID. RTEST.                                              Col73
        DATA DIVISION.                                                  Col73
        WORKING-STORAGE SECTION.                                        Col73
      *> This is the communications buffer between EXEC CICS statements
      *> and the C/C++ code that implements those statements.
      *>
      *> The EXEC CICS preprocessor converts the EXEC CICS statements
      *> into COBOL code that populates this block, which is then
      *> handed to the dfh_execcics() routine in the C/C++ code
      *>
      *> Clearly, the two sides need to agree on the structure and 
      *> of this data block.  Refer to tcp.cpp and tcp.h in the 
      *> CICS system code.
      *>
       01 DFHXBLK.
           05 DFHX-TERMID       PIC X(4) VALUE SPACES.
           05 DFHX-COMMAND-CODE USAGE BINARY-LONG   UNSIGNED VALUE ZERO.
           05 DFHX-FLAGS        USAGE BINARY-DOUBLE UNSIGNED VALUE ZERO.
           05 DFHX-PARAMETERS   OCCURS 12 TIMES.
             10 DFHX-LITERAL-X PIC X(64). *> For literal names
             10 DFHX-LITERAL-NUMERIC REDEFINES DFHX-LITERAL-X.
               15 DFHX-LITERAL-V   
                  USAGE BINARY-DOUBLE UNSIGNED. *> For literal values
               15 FILLER PIC X(56).
             10 DFHX-POINTERS REDEFINES DFHX-LITERAL-X.
               15 DFHX-POINTER USAGE POINTER.
                *> What follows is the length of the buffer referenced
                *> by the pointer, when appropriate.
               15 DFHX-LENGTH BINARY-DOUBLE UNSIGNED.
               15 FILLER PIC X(48).
               
        01  DX-FLAG-BITS.
            05 DX-BIT00 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000000000001'.
               05 DXF_0E0C_ABCODE_N REDEFINES DX-BIT00
                  USAGE BINARY-DOUBLE UNSIGNED.
               05 DXF_1804_ACCUM REDEFINES DX-BIT00
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT01 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000000000002'.
               05 DXF_0E0C_ABEND REDEFINES DX-BIT01
                  USAGE BINARY-DOUBLE UNSIGNED.
               05 DXF_1804_ACTPARTN_N REDEFINES DX-BIT01
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT02 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000000000004'.
               05 DXF_0E0C_CANCEL REDEFINES DX-BIT02
                  USAGE BINARY-DOUBLE UNSIGNED.
               05 DXF_1804_ALARM REDEFINES DX-BIT02
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT03 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000000000008'.
               05 DXF_0E0C_END-EXEC REDEFINES DX-BIT03
                  USAGE BINARY-DOUBLE UNSIGNED.
               05 DXF_1804_ALTERNATE REDEFINES DX-BIT03
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT04 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000000000010'.
               05 DXF_0E0C_NODUMP REDEFINES DX-BIT04
                  USAGE BINARY-DOUBLE UNSIGNED.
               05 DXF_1804_CURSOR REDEFINES DX-BIT04
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT05 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000000000020'.
               05 DXF_1804_CURSOR_DV REDEFINES DX-BIT05
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT06 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000000000040'.
               05 DXF_1804_DATAONLY REDEFINES DX-BIT06
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT07 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000000000080'.
               05 DXF_1804_DEFAULT REDEFINES DX-BIT07
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT08 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000000000100'.
               05 DXF_1804_END-EXEC REDEFINES DX-BIT08
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT09 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000000000200'.
               05 DXF_1804_ERASE REDEFINES DX-BIT09
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT10 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000000000400'.
               05 DXF_1804_ERASEAUP REDEFINES DX-BIT10
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT11 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000000000800'.
               05 DXF_1804_FMHPARM_N REDEFINES DX-BIT11
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT12 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000000001000'.
               05 DXF_1804_FORMFEED REDEFINES DX-BIT12
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT13 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000000002000'.
               05 DXF_1804_FREEKB REDEFINES DX-BIT13
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT14 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000000004000'.
               05 DXF_1804_FROM_DA REDEFINES DX-BIT14
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT15 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000000008000'.
               05 DXF_1804_FRSET REDEFINES DX-BIT15
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT16 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000000010000'.
               05 DXF_1804_HONEOM REDEFINES DX-BIT16
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT17 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000000020000'.
               05 DXF_1804_L40 REDEFINES DX-BIT17
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT18 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000000040000'.
               05 DXF_1804_L64 REDEFINES DX-BIT18
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT19 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000000080000'.
               05 DXF_1804_L80 REDEFINES DX-BIT19
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT20 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000000100000'.
               05 DXF_1804_LAST REDEFINES DX-BIT20
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT21 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000000200000'.
               05 DXF_1804_LDC_N REDEFINES DX-BIT21
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT22 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000000400000'.
               05 DXF_1804_LENGTH_DV REDEFINES DX-BIT22
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT23 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000000800000'.
               05 DXF_1804_MAPONLY REDEFINES DX-BIT23
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT24 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000001000000'.
               05 DXF_1804_MAPSET_N REDEFINES DX-BIT24
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT25 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000002000000'.
               05 DXF_1804_MAP_N REDEFINES DX-BIT25
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT26 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000004000000'.
               05 DXF_1804_MSR_DV REDEFINES DX-BIT26
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT27 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000008000000'.
               05 DXF_1804_NLEOM REDEFINES DX-BIT27
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT28 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000010000000'.
               05 DXF_1804_NOFLUSH REDEFINES DX-BIT28
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT29 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000020000000'.
               05 DXF_1804_OUTPARTN_N REDEFINES DX-BIT29
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT30 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000040000000'.
               05 DXF_1804_PAGING REDEFINES DX-BIT30
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT31 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000080000000'.
               05 DXF_1804_PRINT REDEFINES DX-BIT31
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT32 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000100000000'.
               05 DXF_1804_REQID_N REDEFINES DX-BIT32
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT33 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000200000000'.
               05 DXF_1804_SET_PR REDEFINES DX-BIT33
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT34 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000400000000'.
               05 DXF_1804_TERMINAL REDEFINES DX-BIT34
                  USAGE BINARY-DOUBLE UNSIGNED.
            05 DX-BIT35 USAGE BINARY-DOUBLE UNSIGNED
               VALUE H'0000000800000000'.
               05 DXF_1804_WAIT REDEFINES DX-BIT35
                  USAGE BINARY-DOUBLE UNSIGNED.

        01 ORDMAP1 PIC X(111).

       PROCEDURE DIVISION.                                              Col73
        
       INITIALIZE DFHXBLK                                               *> ++
      *MOVE EIBTRMID TO DFHX-TERMID IN DFHXBLK                          *> ++
       MOVE H'1804' TO DFHX-COMMAND-CODE IN DFHXBLK                     *> ++
       ADD DXF_1804_CURSOR TO DFHX-FLAGS IN DFHXBLK                     *> ++
       ADD DXF_1804_ERASE TO DFHX-FLAGS IN DFHXBLK                      *> ++
       ADD DXF_1804_FROM_DA TO DFHX-FLAGS IN DFHXBLK                    *> ++
       SET DFHX-POINTER IN DFHXBLK(1) TO ADDRESS OF ORDMAP1             *> ++
       MOVE LENGTH OF ORDMAP1 TO DFHX-LENGTH IN DFHXBLK(1)              *> ++
       ADD DXF_1804_MAPSET_N TO DFHX-FLAGS IN DFHXBLK                   *> ++
       MOVE 'ORDSET1' TO DFHX-LITERAL-X IN DFHXBLK(2)                   *> ++
       ADD DXF_1804_MAP_N TO DFHX-FLAGS IN DFHXBLK                      *> ++
       MOVE 'ORDMAP1' TO DFHX-LITERAL-X IN DFHXBLK(3)                   *> ++
       
        DISPLAY "Ding""Dong" .                                          Col73
        DISPLAY "Ding''Dong" .                                          Col73
        DISPLAY 'Ding""Dong' .                                          Col73
        DISPLAY 'Ding''Dong' .                                          Col73
        DISPLAY "Ding'Dong"  .                                          Col73
        DISPLAY 'Ding''Dong' .                                          Col73
        DISPLAY "Ding'"                                                 Col73
      -         "Dong"       .                                          Col73
        DISPLAY 'Ding'''   *> Can I put a comment here?                 Col73
      -         'Dong'       .                                          Col73
        DISPLAY "Weird 1:".  DISPLAY                              'Ding'Col73
      -         ''Dong'      .  DISPLAY "That was weird.".              Col73
        DISPLAY "Weird 2:".  DISPLAY                              "Ding"Col73
      -         ""Dong"      .  DISPLAY "That was weird.".              Col73
        STOP RUN.                                                       Col73
