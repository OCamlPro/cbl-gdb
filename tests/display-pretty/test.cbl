        >>SOURCE FREE
        identification division.
        program-id. test.
        data division.
        working-storage section.
        01    VAR-01.
         02   VAR-02A PIC 999v99    VALUE 123.45.
         02   VAR-02B PIC S999v99   VALUE 123.45.
         02   VAR-02C PIC S999v99   VALUE -123.45.
         02   VAR-02.
          03  VAR-03A PIC X(20)     VALUE "Robert".
          03  VAR-03B PIC X(20)     VALUE "Joseph".
          03  VAR-03C PIC X(20)     VALUE "Dubner".
          03  VAR-03.
           04 VAR-03A PIC 9999      VALUE 1953.
           04 VAR-03B PIC 99        VALUE 02.
           04 VAR-03C PIC 99        VALUE 27.
        procedure division.
        display VAR-01.
        end program test.
