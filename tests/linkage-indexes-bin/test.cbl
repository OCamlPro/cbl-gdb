        IDENTIFICATION   DIVISION.
        PROGRAM-ID.      "prog".
        DATA             DIVISION.
        WORKING-STORAGE  SECTION.
        01 SHM-AREA.
         05 SHM-AMNT                    PIC  9(008) binary value 10.
         05 SHM-ELEMENT-LEN             PIC  9(018).
         05 SHM-ELEMENTS.
          07 SHM-ELEMENT
            OCCURS           1 TO 20 TIMES
            DEPENDING ON     SHM-AMNT
            ASCENDING KEY IS SHM-VAL1
            INDEXED BY       SHM-IND.
           10 SHM-REC.
             15 SHM-VAL1                PIC  9(001).
             15 SHM-VAL2                PIC  X(001).
        01 I PIC 999.

        PROCEDURE DIVISION.
        COMPUTE SHM-VAL1(1) = 1
        COMPUTE SHM-VAL1(2) = 2
        COMPUTE SHM-VAL1(3) = 3
        COMPUTE SHM-VAL1(4) = 4
        COMPUTE SHM-VAL1(5) = 5
        COMPUTE SHM-VAL1(6) = 6
        COMPUTE SHM-VAL1(7) = 7
        COMPUTE SHM-VAL1(8) = 8
        COMPUTE SHM-VAL1(9) = 9
        PERFORM VARYING I FROM 10 BY 1 UNTIL I > 20
            MOVE ZERO  TO SHM-VAL1(I)
            MOVE SPACE TO SHM-VAL2(I)
            END-PERFORM
        CALL "prog2" using SHM-AREA
        GOBACK.
        END PROGRAM "prog".

        IDENTIFICATION   DIVISION.
        PROGRAM-ID.      "prog2".
        DATA             DIVISION.
        LINKAGE          SECTION.
        01 LS-SHM-AREA.
         05 LS-SHM-AMNT                    PIC  9(008) binary.
         05 LS-SHM-ELEMENT-LEN             PIC  9(018).
         05 LS-SHM-ELEMENTS.
          07 LS-SHM-ELEMENT
            OCCURS           UNBOUNDED
            DEPENDING ON     LS-SHM-AMNT
            ASCENDING KEY IS LS-SHM-VAL1
            INDEXED BY       LS-SHM-IND.
           10 LS-SHM-REC.
             15 LS-SHM-VAL1                PIC  9(001).
             15 LS-SHM-VAL2                PIC  X(001).
        PROCEDURE DIVISION USING LS-SHM-AREA.
        DISPLAY LS-SHM-VAL1(1)
        DISPLAY LS-SHM-VAL1(2)
        DISPLAY LS-SHM-VAL1(3)
        DISPLAY LS-SHM-VAL1(4)
        DISPLAY LS-SHM-VAL1(5)
        DISPLAY LS-SHM-VAL1(6)
        DISPLAY LS-SHM-VAL1(7)
        DISPLAY LS-SHM-VAL1(8)
        DISPLAY LS-SHM-VAL1(9)
        GOBACK.
        END PROGRAM "prog2".
