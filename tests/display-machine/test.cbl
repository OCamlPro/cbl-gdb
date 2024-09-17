        >>SOURCE FREE
        identification division.
        program-id. test.
        data division.
        working-storage section.
        01 VAR-DISPLAY                   PIC  999V99 DISPLAY                VALUE  123.45.
        01 VAR-DISPLAY-P                 PIC S999V99 DISPLAY                VALUE  123.45.
        01 VAR-DISPLAY-N                 PIC S999V99 DISPLAY                VALUE -123.45.
        01 VAR-BINARY                    PIC  999V99 BINARY                 VALUE  123.45.
        01 VAR-BINARY-P                  PIC S999V99 BINARY                 VALUE  123.45.
        01 VAR-BINARY-N                  PIC S999V99 BINARY                 VALUE -123.45.
        01 VAR-BINARY-C-LONG                         BINARY-C-LONG          VALUE  123.
        01 VAR-BINARY-C-LONG-P                       BINARY-C-LONG          VALUE  123.
        01 VAR-BINARY-C-LONG-N                       BINARY-C-LONG          VALUE -123.
        01 VAR-BINARY-C-LONG-SIGNED                  BINARY-C-LONG SIGNED   VALUE  123.
        01 VAR-BINARY-C-LONG-SIGNED-P                BINARY-C-LONG SIGNED   VALUE  123.
        01 VAR-BINARY-C-LONG-SIGNED-N                BINARY-C-LONG SIGNED   VALUE -123.
        01 VAR-BINARY-C-LONG-UNSIGNED                BINARY-C-LONG UNSIGNED VALUE  123.
        01 VAR-BINARY-CHAR                           BINARY-CHAR            VALUE  123.
        01 VAR-BINARY-CHAR-P                         BINARY-CHAR            VALUE  123.
        01 VAR-BINARY-CHAR-N                         BINARY-CHAR            VALUE -123.
        01 VAR-BINARY-CHAR-SIGNED                    BINARY-CHAR SIGNED     VALUE  123.
        01 VAR-BINARY-CHAR-SIGNED-P                  BINARY-CHAR SIGNED     VALUE  123.
        01 VAR-BINARY-CHAR-SIGNED-N                  BINARY-CHAR SIGNED     VALUE -123.
        01 VAR-BINARY-CHAR-UNSIGNED                  BINARY-CHAR UNSIGNED   VALUE  123.
        01 VAR-BINARY-DOUBLE                         BINARY-DOUBLE          VALUE  123.
        01 VAR-BINARY-DOUBLE-P                       BINARY-DOUBLE          VALUE  123.
        01 VAR-BINARY-DOUBLE-N                       BINARY-DOUBLE          VALUE -123.
        01 VAR-BINARY-DOUBLE-SIGNED                  BINARY-DOUBLE SIGNED   VALUE  123.
        01 VAR-BINARY-DOUBLE-SIGNED-P                BINARY-DOUBLE SIGNED   VALUE  123.
        01 VAR-BINARY-DOUBLE-SIGNED-N                BINARY-DOUBLE SIGNED   VALUE -123.
        01 VAR-SIGNED-LONG                           SIGNED-LONG            VALUE  123.
        01 VAR-SIGNED-LONG-P                         SIGNED-LONG            VALUE  123.
        01 VAR-SIGNED-LONG-N                         SIGNED-LONG            VALUE -123.
        01 VAR-BINARY-DOUBLE-UNSIGNED                BINARY-DOUBLE UNSIGNED VALUE  123.
        01 VAR-UNSIGNED-LONG                         UNSIGNED-LONG          VALUE  123.
        01 VAR-BINARY-INT                            BINARY-INT             VALUE  123.
        01 VAR-BINARY-INT-P                          BINARY-INT             VALUE  123.
        01 VAR-BINARY-INT-N                          BINARY-INT             VALUE -123.
        01 VAR-BINARY-LONG                           BINARY-LONG            VALUE  123.
        01 VAR-BINARY-LONG-P                         BINARY-LONG            VALUE  123.
        01 VAR-BINARY-LONG-N                         BINARY-LONG            VALUE -123.
        01 VAR-BINARY-LONG-SIGNED                    BINARY-LONG SIGNED     VALUE  123.
        01 VAR-BINARY-LONG-SIGNED-P                  BINARY-LONG SIGNED     VALUE  123.
        01 VAR-BINARY-LONG-SIGNED-N                  BINARY-LONG SIGNED     VALUE -123.
        01 VAR-SIGNED-INT                            SIGNED-INT             VALUE  123.
        01 VAR-SIGNED-INT-P                          SIGNED-INT             VALUE  123.
        01 VAR-SIGNED-INT-N                          SIGNED-INT             VALUE -123.
        01 VAR-BINARY-LONG-UNSIGNED                  BINARY-LONG UNSIGNED   VALUE  123.
        01 VAR-UNSIGNED-INT                          UNSIGNED-INT           VALUE  123.
        01 VAR-BINARY-LONG-LONG                      BINARY-LONG-LONG       VALUE  123.
        01 VAR-BINARY-LONG-LONG-P                    BINARY-LONG-LONG       VALUE  123.
        01 VAR-BINARY-LONG-LONG-N                    BINARY-LONG-LONG       VALUE -123.
        01 VAR-BINARY-SHORT                          BINARY-SHORT           VALUE  123.
        01 VAR-BINARY-SHORT-P                        BINARY-SHORT           VALUE  123.
        01 VAR-BINARY-SHORT-N                        BINARY-SHORT           VALUE -123.
        01 VAR-BINARY-SHORT-SIGNED                   BINARY-SHORT SIGNED    VALUE  123.
        01 VAR-BINARY-SHORT-SIGNED-P                 BINARY-SHORT SIGNED    VALUE  123.
        01 VAR-BINARY-SHORT-SIGNED-N                 BINARY-SHORT SIGNED    VALUE -123.
        01 VAR-SIGNED-SHORT                          SIGNED-SHORT           VALUE  123.
        01 VAR-SIGNED-SHORT-P                        SIGNED-SHORT           VALUE  123.
        01 VAR-SIGNED-SHORT-N                        SIGNED-SHORT           VALUE -123.
        01 VAR-BINARY-SHORT-UNSIGNED                 BINARY-SHORT UNSIGNED  VALUE  123.
        01 VAR-UNSIGNED-SHORT                        UNSIGNED-SHORT         VALUE  123.
        01 VAR-COMP                      PIC  999V99 COMP                   VALUE  123.45.
        01 VAR-COMP-P                    PIC S999V99 COMP                   VALUE  123.45.
        01 VAR-COMP-N                    PIC S999V99 COMP                   VALUE -123.45.
        01 VAR-COMPUTATIONAL             PIC  999V99 COMPUTATIONAL          VALUE  123.45.
        01 VAR-COMPUTATIONAL-P           PIC S999V99 COMPUTATIONAL          VALUE  123.45.
        01 VAR-COMPUTATIONAL-N           PIC S999V99 COMPUTATIONAL          VALUE -123.45.
        01 VAR-COMP-1                                COMP-1                 VALUE  123.45.
        01 VAR-COMP-1-P                              COMP-1                 VALUE  123.45.
        01 VAR-COMP-1-N                              COMP-1                 VALUE -123.45.
        01 VAR-COMPUTATIONAL-1                       COMPUTATIONAL-1        VALUE  123.45.
        01 VAR-COMPUTATIONAL-1-P                     COMPUTATIONAL-1        VALUE  123.45.
        01 VAR-COMPUTATIONAL-1-N                     COMPUTATIONAL-1        VALUE -123.45.
        01 VAR-FLOAT-SHORT                           FLOAT-SHORT            VALUE  123.45.
        01 VAR-FLOAT-SHORT-P                         FLOAT-SHORT            VALUE  123.45.
        01 VAR-FLOAT-SHORT-N                         FLOAT-SHORT            VALUE -123.45.
        01 VAR-COMP-2                                COMP-2                 VALUE  123.45.
        01 VAR-COMP-2-P                              COMP-2                 VALUE  123.45.
        01 VAR-COMP-2-N                              COMP-2                 VALUE -123.45.
        01 VAR-COMPUTATIONAL-2                       COMPUTATIONAL-2        VALUE  123.45.
        01 VAR-COMPUTATIONAL-2-P                     COMPUTATIONAL-2        VALUE  123.45.
        01 VAR-COMPUTATIONAL-2-N                     COMPUTATIONAL-2        VALUE -123.45.
        01 VAR-FLOAT-LONG                            FLOAT-LONG             VALUE  123.45.
        01 VAR-FLOAT-LONG-P                          FLOAT-LONG             VALUE  123.45.
        01 VAR-FLOAT-LONG-N                          FLOAT-LONG             VALUE -123.45.
        01 VAR-COMP-3                    PIC  999V99 COMP-3                 VALUE  123.45.
        01 VAR-COMP-3-P                  PIC S999V99 COMP-3                 VALUE  123.45.
        01 VAR-COMP-3-N                  PIC S999V99 COMP-3                 VALUE -123.45.
        01 VAR-COMPUTATIONAL-3           PIC  999V99 COMPUTATIONAL-3        VALUE  123.45.
        01 VAR-COMPUTATIONAL-3-P         PIC S999V99 COMPUTATIONAL-3        VALUE  123.45.
        01 VAR-COMPUTATIONAL-3-N         PIC S999V99 COMPUTATIONAL-3        VALUE -123.45.
        01 VAR-PACKED-DECIMAL            PIC  999V99 PACKED-DECIMAL         VALUE  123.45.
        01 VAR-PACKED-DECIMAL-P          PIC S999V99 PACKED-DECIMAL         VALUE  123.45.
        01 VAR-PACKED-DECIMAL-N          PIC S999V99 PACKED-DECIMAL         VALUE -123.45.
        01 VAR-COMP-4                    PIC  999V99 COMP-4                 VALUE  123.45.
        01 VAR-COMP-4-P                  PIC S999V99 COMP-4                 VALUE  123.45.
        01 VAR-COMP-4-N                  PIC S999V99 COMP-4                 VALUE -123.45.
        01 VAR-COMPUTATIONAL-4           PIC  999V99 COMPUTATIONAL-4        VALUE  123.45.
        01 VAR-COMPUTATIONAL-4-P         PIC S999V99 COMPUTATIONAL-4        VALUE  123.45.
        01 VAR-COMPUTATIONAL-4-N         PIC S999V99 COMPUTATIONAL-4        VALUE -123.45.
        01 VAR-COMP-5                    PIC  999V99 COMP-5                 VALUE  123.45.
        01 VAR-COMP-5-P                  PIC S999V99 COMP-5                 VALUE  123.45.
        01 VAR-COMP-5-N                  PIC S999V99 COMP-5                 VALUE -123.45.
        01 VAR-COMPUTATIONAL-5           PIC  999V99 COMPUTATIONAL-5        VALUE  123.45.
        01 VAR-COMPUTATIONAL-5-P         PIC S999V99 COMPUTATIONAL-5        VALUE  123.45.
        01 VAR-COMPUTATIONAL-5-N         PIC S999V99 COMPUTATIONAL-5        VALUE -123.45.
        01 VAR-COMP-6                    PIC  999V99 COMP-6                 VALUE  123.45.
        01 VAR-COMPUTATIONAL-6           PIC  999V99 COMPUTATIONAL-6        VALUE  123.45.
        01 VAR-COMP-X                    PIC  999V99 COMP-X                 VALUE  123.45.
        01 VAR-COMP-X-P                  PIC S999V99 COMP-X                 VALUE  123.45.
        01 VAR-COMP-X-N                  PIC S999V99 COMP-X                 VALUE -123.45.
        01 VAR-COMPUTATIONAL-X           PIC  999V99 COMPUTATIONAL-X        VALUE  123.45.
        01 VAR-COMPUTATIONAL-X-P         PIC S999V99 COMPUTATIONAL-X        VALUE  123.45.
        01 VAR-COMPUTATIONAL-X-N         PIC S999V99 COMPUTATIONAL-X        VALUE -123.45.
        01 VAR-FLOAT-DECIMAL-16                      FLOAT-DECIMAL-16       VALUE  123.45.
        01 VAR-FLOAT-DECIMAL-16-P                    FLOAT-DECIMAL-16       VALUE  123.45.
        01 VAR-FLOAT-DECIMAL-16-N                    FLOAT-DECIMAL-16       VALUE -123.45.
        01 VAR-FLOAT-DECIMAL-34                      FLOAT-DECIMAL-34       VALUE  123.45.
        01 VAR-FLOAT-DECIMAL-34-P                    FLOAT-DECIMAL-34       VALUE  123.45.
        01 VAR-FLOAT-DECIMAL-34-N                    FLOAT-DECIMAL-34       VALUE -123.45.
        01 VAR-INDEX                                 INDEX                  VALUE  123.
        01 VAR-POINTER                               POINTER.
        01 VAR-PROCEDURE-POINTER                     PROCEDURE-POINTER.
        01 VAR-PROGRAM-POINTER                       PROGRAM-POINTER.
        procedure division.
        DISPLAY VAR-DISPLAY.
        DISPLAY VAR-DISPLAY-P.
        DISPLAY VAR-DISPLAY-N.
        DISPLAY VAR-BINARY.
        DISPLAY VAR-BINARY-P.
        DISPLAY VAR-BINARY-N.
        DISPLAY VAR-BINARY-C-LONG.
        DISPLAY VAR-BINARY-C-LONG-P.
        DISPLAY VAR-BINARY-C-LONG-N.
        DISPLAY VAR-BINARY-C-LONG-SIGNED.
        DISPLAY VAR-BINARY-C-LONG-SIGNED-P.
        DISPLAY VAR-BINARY-C-LONG-SIGNED-N.
        DISPLAY VAR-BINARY-C-LONG-UNSIGNED.
        DISPLAY VAR-BINARY-CHAR.
        DISPLAY VAR-BINARY-CHAR-P.
        DISPLAY VAR-BINARY-CHAR-N.
        DISPLAY VAR-BINARY-CHAR-SIGNED.
        DISPLAY VAR-BINARY-CHAR-SIGNED-P.
        DISPLAY VAR-BINARY-CHAR-SIGNED-N.
        DISPLAY VAR-BINARY-CHAR-UNSIGNED.
        DISPLAY VAR-BINARY-DOUBLE.
        DISPLAY VAR-BINARY-DOUBLE-P.
        DISPLAY VAR-BINARY-DOUBLE-N.
        DISPLAY VAR-BINARY-DOUBLE-SIGNED.
        DISPLAY VAR-BINARY-DOUBLE-SIGNED-P.
        DISPLAY VAR-BINARY-DOUBLE-SIGNED-N.
        DISPLAY VAR-SIGNED-LONG.
        DISPLAY VAR-SIGNED-LONG-P.
        DISPLAY VAR-SIGNED-LONG-N.
        DISPLAY VAR-BINARY-DOUBLE-UNSIGNED.
        DISPLAY VAR-UNSIGNED-LONG.
        DISPLAY VAR-BINARY-INT.
        DISPLAY VAR-BINARY-INT-P.
        DISPLAY VAR-BINARY-INT-N.
        DISPLAY VAR-BINARY-LONG.
        DISPLAY VAR-BINARY-LONG-P.
        DISPLAY VAR-BINARY-LONG-N.
        DISPLAY VAR-BINARY-LONG-SIGNED.
        DISPLAY VAR-BINARY-LONG-SIGNED-P.
        DISPLAY VAR-BINARY-LONG-SIGNED-N.
        DISPLAY VAR-SIGNED-INT.
        DISPLAY VAR-SIGNED-INT-P.
        DISPLAY VAR-SIGNED-INT-N.
        DISPLAY VAR-BINARY-LONG-UNSIGNED.
        DISPLAY VAR-UNSIGNED-INT.
        DISPLAY VAR-BINARY-LONG-LONG.
        DISPLAY VAR-BINARY-LONG-LONG-P.
        DISPLAY VAR-BINARY-LONG-LONG-N.
        DISPLAY VAR-BINARY-SHORT.
        DISPLAY VAR-BINARY-SHORT-P.
        DISPLAY VAR-BINARY-SHORT-N.
        DISPLAY VAR-BINARY-SHORT-SIGNED.
        DISPLAY VAR-BINARY-SHORT-SIGNED-P.
        DISPLAY VAR-BINARY-SHORT-SIGNED-N.
        DISPLAY VAR-SIGNED-SHORT.
        DISPLAY VAR-SIGNED-SHORT-P.
        DISPLAY VAR-SIGNED-SHORT-N.
        DISPLAY VAR-BINARY-SHORT-UNSIGNED.
        DISPLAY VAR-UNSIGNED-SHORT.
        DISPLAY VAR-COMP.
        DISPLAY VAR-COMP-P.
        DISPLAY VAR-COMP-N.
        DISPLAY VAR-COMPUTATIONAL.
        DISPLAY VAR-COMPUTATIONAL-P.
        DISPLAY VAR-COMPUTATIONAL-N.
        DISPLAY VAR-COMP-1.
        DISPLAY VAR-COMP-1-P.
        DISPLAY VAR-COMP-1-N.
        DISPLAY VAR-COMPUTATIONAL-1.
        DISPLAY VAR-COMPUTATIONAL-1-P.
        DISPLAY VAR-COMPUTATIONAL-1-N.
        DISPLAY VAR-FLOAT-SHORT.
        DISPLAY VAR-FLOAT-SHORT-P.
        DISPLAY VAR-FLOAT-SHORT-N.
        DISPLAY VAR-COMP-2.
        DISPLAY VAR-COMP-2-P.
        DISPLAY VAR-COMP-2-N.
        DISPLAY VAR-COMPUTATIONAL-2.
        DISPLAY VAR-COMPUTATIONAL-2-P.
        DISPLAY VAR-COMPUTATIONAL-2-N.
        DISPLAY VAR-FLOAT-LONG.
        DISPLAY VAR-FLOAT-LONG-P.
        DISPLAY VAR-FLOAT-LONG-N.
        DISPLAY VAR-COMP-3.
        DISPLAY VAR-COMP-3-P.
        DISPLAY VAR-COMP-3-N.
        DISPLAY VAR-COMPUTATIONAL-3.
        DISPLAY VAR-COMPUTATIONAL-3-P.
        DISPLAY VAR-COMPUTATIONAL-3-N.
        DISPLAY VAR-PACKED-DECIMAL.
        DISPLAY VAR-PACKED-DECIMAL-P.
        DISPLAY VAR-PACKED-DECIMAL-N.
        DISPLAY VAR-COMP-4.
        DISPLAY VAR-COMP-4-P.
        DISPLAY VAR-COMP-4-N.
        DISPLAY VAR-COMPUTATIONAL-4.
        DISPLAY VAR-COMPUTATIONAL-4-P.
        DISPLAY VAR-COMPUTATIONAL-4-N.
        DISPLAY VAR-COMP-5.
        DISPLAY VAR-COMP-5-P.
        DISPLAY VAR-COMP-5-N.
        DISPLAY VAR-COMPUTATIONAL-5.
        DISPLAY VAR-COMPUTATIONAL-5-P.
        DISPLAY VAR-COMPUTATIONAL-5-N.
        DISPLAY VAR-COMP-6.
        DISPLAY VAR-COMPUTATIONAL-6.
        DISPLAY VAR-COMP-X.
        DISPLAY VAR-COMP-X-P.
        DISPLAY VAR-COMP-X-N.
        DISPLAY VAR-COMPUTATIONAL-X.
        DISPLAY VAR-COMPUTATIONAL-X-P.
        DISPLAY VAR-COMPUTATIONAL-X-N.
        DISPLAY VAR-FLOAT-DECIMAL-16.
        DISPLAY VAR-FLOAT-DECIMAL-16-P.
        DISPLAY VAR-FLOAT-DECIMAL-16-N.
        DISPLAY VAR-FLOAT-DECIMAL-34.
        DISPLAY VAR-FLOAT-DECIMAL-34-P.
        DISPLAY VAR-FLOAT-DECIMAL-34-N.
        DISPLAY VAR-INDEX.
        DISPLAY VAR-POINTER.
        DISPLAY VAR-PROCEDURE-POINTER.
        DISPLAY VAR-PROGRAM-POINTER.
        end program test.
