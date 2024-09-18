        identification  division.
        program-id.     prog31.
        data            division.
        working-storage section.
        01 variable1    pic x(16) value "I am variable31".
        procedure       division.
        display         variable1
        call            "prog32"
        goback.
        end program     prog31.

        identification  division.
        program-id.     prog32.
        data            division.
        working-storage section.
        01 variable1    pic x(16) value "I am variable32".
        procedure       division.
        display         variable1
        call            "prog33"
        goback.
        end program     prog32.

        identification  division.
        program-id.     prog33.
        data            division.
        working-storage section.
        01 variable1    pic x(16) value "I am variable33".
        procedure       division.
        display         variable1
        goback.
        end program     prog33.

