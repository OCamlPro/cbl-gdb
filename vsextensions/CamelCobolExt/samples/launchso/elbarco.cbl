       ID DIVISION. 
       PROGRAM-ID. elbarco.
       ENVIRONMENT DIVISION.
       DATA DIVISION.
       LINKAGE SECTION.
       01 Y-OR-N PIC X(1).
       PROCEDURE DIVISION USING Y-OR-N.
       DISPLAY "   Somos el barco"
       DISPLAY "   Somos el mar"
       DISPLAY "   Yo navego en ti"
       DISPLAY "   Tu navegas en me"
       DISPLAY " "
       IF Y-OR-N = 'Y' THEN
            DISPLAY "Setting internal pause()..."
            CALL "ShowOurPid"
            CALL "pause"
            END-IF
       CALL "esquife"
       GOBACK.
       END PROGRAM elbarco.
       
       ID DIVISION. 
       PROGRAM-ID. esquife.
       PROCEDURE DIVISION.
       DISPLAY "   We are the boat"
       DISPLAY "   We are the sea"
       DISPLAY "   I sail in you"
       DISPLAY "   You sail in me"
       DISPLAY " "
       GOBACK.
       END PROGRAM esquife.
       
