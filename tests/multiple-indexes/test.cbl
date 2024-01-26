        IDENTIFICATION   DIVISION.
        PROGRAM-ID.      "prog".
        DATA             DIVISION.
        WORKING-STORAGE  SECTION.
        01 ATAB EXTERNAL.
           03 STAB OCCURS 0 TO 700 TIMES
             DEPENDING ON     ATAB-MAX
             ASCENDING KEY IS SA
             INDEXED BY       I-IND-S
                              I-IND
                              I-IND2
                              I-IND3.
             05 TAB-ELEMENT.
               07  SA         PIC  X(03).
               07  INIT       PIC  X(01).
        01 ATAB-MAX PIC 9(3) EXTERNAL.

        01 ATAB2-MAX PIC 9(3) .
        01 ATAB2 .
           03 STAB2 OCCURS 0 TO 700 TIMES
             DEPENDING ON     ATAB2-MAX
             ASCENDING KEY IS SA2
             INDEXED BY       I2-IND-S
                              I2-IND
                              I2-IND2
                              I2-IND3.
             05 TAB-ELEMENT.
               07  SA2         PIC  X(03).
               07  INIT2       PIC  X(01).

        PROCEDURE        DIVISION.
        MOVE SPACES TO ATAB 
        MOVE ZERO   TO ATAB-MAX
        GOBACK.
        END PROGRAM "prog".
