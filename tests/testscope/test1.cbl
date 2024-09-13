        identification  division.
        program-id.     prog11.
        data            division.
        working-storage section.
        01 variable1    pic x(16) value "I am variable11".
        procedure       division.
        display         variable1
        display         variable1
        display         variable1
        call            "prog12"
        call            "prog21"
        goback.
        end program     prog11.

        identification  division.
        program-id.     prog12.
        data            division.
        working-storage section.
        01 variable1    pic x(16) value "I am variable12".
        procedure       division.
        display         variable1
        display         variable1
        display         variable1
        call            "prog13"
        goback.
        end program     prog12.

        identification  division.
        program-id.     prog13.
        data            division.
        working-storage section.
        01 variable1    pic x(16) value "I am variable13".
        procedure       division.
        display         variable1
        display         variable1
        display         variable1
        goback.
        end program     prog13.

