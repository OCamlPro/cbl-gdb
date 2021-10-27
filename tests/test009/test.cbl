        IDENTIFICATION   DIVISION.
        PROGRAM-ID.      test.
        DATA             DIVISION.
        WORKING-STORAGE  SECTION.
        01  THREE_CHARS.
            05  CHAR_1                  PIC X(01) VALUE SPACE.
            05  CHAR_2                  PIC X(01) VALUE SPACE.
            05  CHAR_3                  PIC X(01) VALUE SPACE.
        01 FILLER REDEFINES THREE_CHARS PIC X(03).
            88  JX                      VALUE "JJJ", "XXX".

        PROCEDURE DIVISION.
        MOVE "XYZ" to THREE_CHARS
        IF JX DISPLAY "True" ELSE DISPLAY "False" END-IF
        MOVE "JJJ" to THREE_CHARS
        IF JX DISPLAY "True" ELSE DISPLAY "False" END-IF
        MOVE "XXX" to THREE_CHARS
        IF JX DISPLAY "True" ELSE DISPLAY "False" END-IF
        MOVE "ABC" to THREE_CHARS
        IF JX DISPLAY "True" ELSE DISPLAY "False" END-IF
        DISPLAY "We're done"
        GOBACK.
