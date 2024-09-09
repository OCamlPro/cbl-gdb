        identification  division.
        program-id.     prog1.
        data            division.
        working-storage section.
        01 variable1    pic x(16) value "I am variable1".
        procedure       division.
        display         variable1
        display         variable1
        display         variable1
        call            "prog2"
        goback.
        end program     prog1.

        identification  division.
        program-id.     prog2.
        data            division.
        working-storage section.
        01 variable1    pic x(16) value "I am variable2".
        procedure       division.
        display         variable1
        display         variable1
        display         variable1
        call            "prog3"
        goback.
        end program     prog2.

        identification  division.
        program-id.     prog3.
        data            division.
        working-storage section.
        01 variable1    pic x(16) value "I am variable3".
        procedure       division.
        display         variable1
        display         variable1
        display         variable1
        goback.
        end program     prog3.

