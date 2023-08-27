        IDENTIFICATION   DIVISION.
        PROGRAM-ID.      "prog".
        DATA             DIVISION.
        WORKING-STORAGE  SECTION.
        01 aaa.
          03 bbb occurs 6.
            05 ccc occurs 4.
              07 ccc1 pic 9.
              07 ccc2 pic x.

        PROCEDURE DIVISION.
        DISPLAY "Trapping on a CALL line can cause confusion"
        CALL "prog2" using aaa.
        GOBACK.
        END PROGRAM "prog".

        IDENTIFICATION   DIVISION.
        PROGRAM-ID.      "prog2".
        DATA             DIVISION.
        LINKAGE SECTION.
        01 aaa.
          03 bbb occurs 6.
            05 ccc occurs 4.
              07 ccc1 pic 9.
              07 ccc2 pic x.
        PROCEDURE DIVISION USING aaa.
        DISPLAY LENGTH aaa
        DISPLAY LENGTH OF bbb
        DISPLAY LENGTH OF ccc
        DISPLAY LENGTH OF ccc1
        GOBACK.
        END PROGRAM "prog2".
