        identification  division.
        program-id.     prog21.
        data            division.
        working-storage section.
        01 variable1    pic x(16) value "I am variable21".
        procedure       division.
        display         variable1
        display         variable1
        display         variable1
        call            "prog22"
        call            "prog31"
        goback.
        end program     prog21.

        identification  division.
        program-id.     prog22.
        data            division.
        working-storage section.
        01 variable1    pic x(16) value "I am variable22".
        procedure       division.
        display         variable1
        display         variable1
        display         variable1
        call            "prog23"
        goback.
        end program     prog22.

        identification  division.
        program-id.     prog23.
        data            division.
        working-storage section.
        01 variable1    pic x(16) value "I am variable23".
        procedure       division.
        display         variable1
        display         variable1
        display         variable1
        goback.
        end program     prog23.

