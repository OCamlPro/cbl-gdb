       Identification Division.
       Program-Id. test.

       Data Division.
       Working-Storage Section.
       01 A Pic 9999 Value 1.
       01 B Pic 9999 Value 1.
       01 C Pic 9999 Value 1.
       01 D Pic 9999 Value 1.
       01 E Pic 9999 Value 1.
       01 F Pic 9999 Value 1.
       01 G Pic 9999 Value 1.

       01 TSTR  Pic X(10) Value 'TESTSTRING'.
       01 TSUBSTR  Pic X(10) Value 'STS'.

       01 TOCCUR.
           03 TOSTR Pic X(10) Value SPACE OCCURS 10.
       
       01 0e00 PIC 9V9.

       Procedure Division.

       TEST-START SECTION.

       DISPLAY "Start debug test".

       P-CALC SECTION.

       Perform 10 Times
           MOVE 'TESTSTRING' TO TOSTR(A)
           Add 1 to A
           Add 1 to B
           Add 1 to C
           Add 1 to D
           Add 1 to E
           Add 1 to F
           Add 1 to G
           Display G
       End-Perform.

       DISPLAY TSTR.
       
       GOBACK.
