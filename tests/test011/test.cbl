        >>SOURCE FREE
        identification division.
        program-id. test.
        data division.
        working-storage section.
        01 A PIC X VALUE "A".
           88  AFLAG   VALUE 'A' 'B' 'z' thru 'a' 'C' "I" through "N" 'Z'
               WHEN SET TO FALSE IS 'Y'.
        01 B PIC 999.
           88  BFLAG   VALUE 1, 3, 5, 10 THROUGH 20, 24
               WHEN SET TO FALSE IS 0.
        procedure division.
        move 'D' to A
        if aflag 
            display "true"
        else
            display "false".
        move 'A' to A
        if aflag 
            display "true"
        else
            display "false".
        set aflag to false
        if aflag 
            display "true"
        else
            display "false".

        move 2 to b
        if bflag 
            display "true"
        else
            display "false".
        move 15 to b
        if bflag 
            display "true"
        else
            display "false".
        set bflag to false
        if bflag 
            display "true"
        else
            display "false".

        end program test.
