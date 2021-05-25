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

       Procedure Division.

       TEST-START SECTION.

       DISPLAY "Start debug test".

       P-CALC SECTION.

       Perform 10 Times
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
