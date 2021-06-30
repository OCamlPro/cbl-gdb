        >>SOURCE FREE
IDENTIFICATION DIVISION.
PROGRAM-ID. test.
DATA DIVISION.
WORKING-STORAGE SECTION.
*>
*>  NOTE: This program is tightly linked to the gdb debugging script test3.scr
*>  If any lines are added, removed, or even moved the effectiveness of the
*>  script will be destroyed, unless and until the script is changed to match.
*>
*>  You Have Been Warned
*>
01 header                          PIC X(40)                                  .
01 withpic                         PIC $ZZZZ9.99+          VALUE  1.23        .
01 def-BINARY-CHAR                 BINARY-CHAR             VALUE  123         .
01 def-BINARY-CHAR-SIGNED          BINARY-CHAR SIGNED      VALUE -2           .
01 def-BINARY-CHAR-UNSIGNED        BINARY-CHAR UNSIGNED    VALUE  3           .
01 def-BINARY-C-LONG               BINARY-C-LONG           VALUE  4           .
01 def-BINARY-C-LONG-SIGNED        BINARY-C-LONG SIGNED    VALUE -5           .
01 def-BINARY-C-LONG-UNSIGNED      BINARY-C-LONG UNSIGNED  VALUE  6           .
01 def-BINARY-DOUBLE               BINARY-DOUBLE           VALUE  7           .
01 def-BINARY-DOUBLE-SIGNED        BINARY-DOUBLE SIGNED    VALUE -8           .
01 def-BINARY-DOUBLE-UNSIGNED      BINARY-DOUBLE UNSIGNED  VALUE  9           .
01 def-BINARY-INT                  BINARY-INT              VALUE  10          .
01 def-BINARY-LONG                 BINARY-LONG             VALUE  11          .
01 def-BINARY-LONG-SIGNED          BINARY-LONG SIGNED      VALUE -12          .
01 def-BINARY-LONG-UNSIGNED        BINARY-LONG UNSIGNED    VALUE  13          .
01 def-BINARY-LONG-LONG            BINARY-LONG-LONG        VALUE  14          .
01 def-BINARY-SHORT                BINARY-SHORT            VALUE  15          .
01 def-BINARY-SHORT-SIGNED         BINARY-SHORT SIGNED     VALUE -16          .
01 def-BINARY-SHORT-UNSIGNED       BINARY-SHORT UNSIGNED   VALUE  17          .
01 def-COMP-1                      COMP-1                  VALUE  1.11        .
01 def-COMPUTATIONAL-1             COMPUTATIONAL-1         VALUE  3.33        .
01 def-COMP-2                      COMP-2                  VALUE  2.22        .
01 def-COMPUTATIONAL-2             COMPUTATIONAL-2         VALUE  4.44        .
01 def-FLOAT-DECIMAL-16            FLOAT-DECIMAL-16        VALUE  5.55        .
01 def-FLOAT-DECIMAL-34            FLOAT-DECIMAL-34        VALUE  9999999999999999999999999999999999 .  *> 6.66        .
01 def-FLOAT-LONG                  FLOAT-LONG              VALUE  7.77        .
01 def-FLOAT-SHORT                 FLOAT-SHORT             VALUE  8.88        .
01 def-INDEX                       INDEX                   VALUE  123456789          .
01 def-POINTER                     POINTER                                    .
01 def-PROCEDURE-POINTER           PROCEDURE-POINTER                          .
01 def-PROGRAM-POINTER             PROGRAM-POINTER                            .
01 def-SIGNED-INT                  SIGNED-INT              VALUE  23          .
01 def-SIGNED-LONG                 SIGNED-LONG             VALUE  24          .
01 def-SIGNED-SHORT                SIGNED-SHORT            VALUE  25          .
01 def-UNSIGNED-INT                UNSIGNED-INT            VALUE  26          .
01 def-UNSIGNED-LONG               UNSIGNED-LONG           VALUE  27          .
01 def-UNSIGNED-SHORT              UNSIGNED-SHORT          VALUE  28          .
01 def-COMP-3             COMP-3           PICTURE S9(9)V9(7) VALUE  988776654.1223345 .
01 def-COMPUTATIONAL-3    COMPUTATIONAL-3  PICTURE S9(9)V9(7) VALUE -988776654.1223345 .
01 def-PACKED-DECIMAL     PACKED-DECIMAL   PICTURE S9(9)V9(7) VALUE  988776654.1223345 .
01 def-PACKED-DECIMAL-S   PACKED-DECIMAL   PICTURE S9(8)V9(7) VALUE  -88776654.1223345 .
01 def-BINARY             BINARY           PICTURE S9(9)V9(7) VALUE  1.0                 .
01 def-BINARY-S           BINARY           PICTURE S9(9)V9(7) VALUE -988776654.1223345 .
01 def-COMP               COMP             PICTURE S9(9)V9(7) VALUE  988776654.1223345 .
01 def-COMPUTATIONAL      COMPUTATIONAL    PICTURE S9(9)V9(7) VALUE -988776654.1223345 .
01 def-COMP-4             COMP-4           PICTURE S9(9)V9(7) VALUE  988776654.1223345 .
01 def-COMPUTATIONAL-4    COMPUTATIONAL-4  PICTURE S9(9)V9(7) VALUE -988776654.1223345 .
01 def-COMP-5             COMP-5           PICTURE S9(9)V9(7) VALUE  988776654.1223345 .
01 def-COMPUTATIONAL-5    COMPUTATIONAL-5  PICTURE S9(9)V9(7) VALUE -988776654.1223345 .
01 def-COMP-6             COMP-6           PICTURE  9(8)V9(7) VALUE   88776654.1223345 .
01 def-COMP-X             COMP-X           PICTURE S9(9)V9(7) VALUE  988776654.1223345 .
01 def-COMPUTATIONAL-X    COMPUTATIONAL-X  PICTURE S9(9)V9(7) VALUE  988776654.1223345 .
01 def-COMP-XX            COMPUTATIONAL-X  PICTURE X(8)       VALUE  9887766541223345 .
01 def-DISPLAY            DISPLAY          PICTURE S9(9)V9(7) VALUE  988776654.1223345 .



01 len-BINARY-CHAR                 CONSTANT AS LENGTH OF def-BINARY-CHAR            .
01 len-BINARY-CHAR-SIGNED          CONSTANT AS LENGTH OF def-BINARY-CHAR-SIGNED     .
01 len-BINARY-CHAR-UNSIGNED        CONSTANT AS LENGTH OF def-BINARY-CHAR-UNSIGNED   .
01 len-BINARY-C-LONG               CONSTANT AS LENGTH OF def-BINARY-C-LONG          .
01 len-BINARY-C-LONG-SIGNED        CONSTANT AS LENGTH OF def-BINARY-C-LONG-SIGNED   .
01 len-BINARY-C-LONG-UNSIGNED      CONSTANT AS LENGTH OF def-BINARY-C-LONG-UNSIGNED .
01 len-BINARY-DOUBLE               CONSTANT AS LENGTH OF def-BINARY-DOUBLE          .
01 len-BINARY-DOUBLE-SIGNED        CONSTANT AS LENGTH OF def-BINARY-DOUBLE-SIGNED   .
01 len-BINARY-DOUBLE-UNSIGNED      CONSTANT AS LENGTH OF def-BINARY-DOUBLE-UNSIGNED .
01 len-BINARY-INT                  CONSTANT AS LENGTH OF def-BINARY-INT             .
01 len-BINARY-LONG                 CONSTANT AS LENGTH OF def-BINARY-LONG            .
01 len-BINARY-LONG-SIGNED          CONSTANT AS LENGTH OF def-BINARY-LONG-SIGNED     .
01 len-BINARY-LONG-UNSIGNED        CONSTANT AS LENGTH OF def-BINARY-LONG-UNSIGNED   .
01 len-BINARY-LONG-LONG            CONSTANT AS LENGTH OF def-BINARY-LONG-LONG       .
01 len-BINARY-SHORT                CONSTANT AS LENGTH OF def-BINARY-SHORT           .
01 len-BINARY-SHORT-SIGNED         CONSTANT AS LENGTH OF def-BINARY-SHORT-SIGNED    .
01 len-BINARY-SHORT-UNSIGNED       CONSTANT AS LENGTH OF def-BINARY-SHORT-UNSIGNED  .
01 len-COMPUTATIONAL               CONSTANT AS LENGTH OF def-COMPUTATIONAL          .
01 len-COMP-1                      CONSTANT AS LENGTH OF def-COMP-1                 .
01 len-COMP-2                      CONSTANT AS LENGTH OF def-COMP-2                 .
01 len-COMPUTATIONAL-1             CONSTANT AS LENGTH OF def-COMPUTATIONAL-1        .
01 len-COMPUTATIONAL-2             CONSTANT AS LENGTH OF def-COMPUTATIONAL-2        .
01 len-FLOAT-DECIMAL-16            CONSTANT AS LENGTH OF def-FLOAT-DECIMAL-16       .
01 len-FLOAT-DECIMAL-34            CONSTANT AS LENGTH OF def-FLOAT-DECIMAL-34       .
01 len-FLOAT-LONG                  CONSTANT AS LENGTH OF def-FLOAT-LONG             .
01 len-FLOAT-SHORT                 CONSTANT AS LENGTH OF def-FLOAT-SHORT            .
01 len-INDEX                       CONSTANT AS LENGTH OF def-INDEX                  .
01 len-POINTER                     CONSTANT AS LENGTH OF def-POINTER                .
01 len-PROCEDURE-POINTER           CONSTANT AS LENGTH OF def-PROCEDURE-POINTER      .
01 len-PROGRAM-POINTER             CONSTANT AS LENGTH OF def-PROGRAM-POINTER        .
01 len-SIGNED-INT                  CONSTANT AS LENGTH OF def-SIGNED-INT             .
01 len-SIGNED-LONG                 CONSTANT AS LENGTH OF def-SIGNED-LONG            .
01 len-SIGNED-SHORT                CONSTANT AS LENGTH OF def-SIGNED-SHORT           .
01 len-UNSIGNED-INT                CONSTANT AS LENGTH OF def-UNSIGNED-INT           .
01 len-UNSIGNED-LONG               CONSTANT AS LENGTH OF def-UNSIGNED-LONG          .
01 len-UNSIGNED-SHORT              CONSTANT AS LENGTH OF def-UNSIGNED-SHORT         .

01 len-COMP-3             CONSTANT AS LENGTH OF def-COMP-3              .
01 len-COMPUTATIONAL-3    CONSTANT AS LENGTH OF def-COMPUTATIONAL-3     .
01 len-PACKED-DECIMAL     CONSTANT AS LENGTH OF def-PACKED-DECIMAL      .
01 len-PACKED-DECIMAL-S   CONSTANT AS LENGTH OF def-PACKED-DECIMAL-S    .
01 len-BINARY             CONSTANT AS LENGTH OF def-BINARY              .
01 len-BINARY-S           CONSTANT AS LENGTH OF def-BINARY-S            .
01 len-COMP               CONSTANT AS LENGTH OF def-COMP                .
*>01 len-COMPUTATIONAL      CONSTANT AS LENGTH OF def-COMPUTATIONAL       .
01 len-COMP-4             CONSTANT AS LENGTH OF def-COMP-4              .
01 len-COMPUTATIONAL-4    CONSTANT AS LENGTH OF def-COMPUTATIONAL-4     .
01 len-COMP-5             CONSTANT AS LENGTH OF def-COMP-5              .
01 len-COMPUTATIONAL-5    CONSTANT AS LENGTH OF def-COMPUTATIONAL-5     .
01 len-COMP-6             CONSTANT AS LENGTH OF def-COMP-6              .
01 len-COMP-X             CONSTANT AS LENGTH OF def-COMP-X              .
01 len-COMPUTATIONAL-X    CONSTANT AS LENGTH OF def-COMPUTATIONAL-X     .
01 len-COMP-XX            CONSTANT AS LENGTH OF def-COMP-XX             .
01 len-DISPLAY            CONSTANT AS LENGTH OF def-DISPLAY             .

PROCEDURE DIVISION.
000-Main.
DISPLAY "On this system, with this build of GnuCOBOL, the"
DISPLAY "PICTURE-less USAGE's have these lengths (in bytes):"
MOVE "An extravaganza of variables!" to HEADER.
DISPLAY " "
DISPLAY "BINARY-CHAR             " len-BINARY-CHAR
DISPLAY "BINARY-CHAR SIGNED      " len-BINARY-CHAR-SIGNED
DISPLAY "BINARY-CHAR UNSIGNED    " len-BINARY-CHAR-UNSIGNED
DISPLAY "BINARY-C-LONG           " len-BINARY-C-LONG
DISPLAY "BINARY-C-LONG SIGNED    " len-BINARY-C-LONG-SIGNED
DISPLAY "BINARY-C-LONG UNSIGNED  " len-BINARY-C-LONG-UNSIGNED
DISPLAY "BINARY-DOUBLE           " len-BINARY-DOUBLE
DISPLAY "BINARY-DOUBLE SIGNED    " len-BINARY-DOUBLE-SIGNED
DISPLAY "BINARY-DOUBLE UNSIGNED  " len-BINARY-DOUBLE-UNSIGNED
DISPLAY "BINARY-INT              " len-BINARY-INT
DISPLAY "BINARY-LONG             " len-BINARY-LONG
DISPLAY "BINARY-LONG SIGNED      " len-BINARY-LONG-SIGNED
DISPLAY "BINARY-LONG UNSIGNED    " len-BINARY-LONG-UNSIGNED
DISPLAY "BINARY-LONG-LONG        " len-BINARY-LONG-LONG
DISPLAY "BINARY-SHORT            " len-BINARY-SHORT
DISPLAY "BINARY-SHORT SIGNED     " len-BINARY-SHORT-SIGNED
DISPLAY "BINARY-SHORT UNSIGNED   " len-BINARY-SHORT-UNSIGNED
DISPLAY "COMPUTATIONAL           " len-COMPUTATIONAL
DISPLAY "COMP-1                  " len-COMP-1
DISPLAY "COMP-2                  " len-COMP-2
DISPLAY "COMPUTATIONAL-1         " len-COMPUTATIONAL-1
DISPLAY "COMPUTATIONAL-2         " len-COMPUTATIONAL-2
DISPLAY "FLOAT-DECIMAL-16        " len-FLOAT-DECIMAL-16
DISPLAY "FLOAT-DECIMAL-34        " len-FLOAT-DECIMAL-34
DISPLAY "FLOAT-LONG              " len-FLOAT-LONG
DISPLAY "FLOAT-SHORT             " len-FLOAT-SHORT
DISPLAY "INDEX                   " len-INDEX
DISPLAY "POINTER                 " len-POINTER
DISPLAY "PROCEDURE-POINTER       " len-PROCEDURE-POINTER
DISPLAY "PROGRAM-POINTER         " len-PROGRAM-POINTER
DISPLAY "SIGNED-INT              " len-SIGNED-INT
DISPLAY "SIGNED-LONG             " len-SIGNED-LONG
DISPLAY "SIGNED-SHORT            " len-SIGNED-SHORT
DISPLAY "UNSIGNED-INT            " len-UNSIGNED-INT
DISPLAY "UNSIGNED-LONG           " len-UNSIGNED-LONG
DISPLAY "UNSIGNED-SHORT          " len-UNSIGNED-SHORT

DISPLAY " "
DISPLAY "...and here are the values:"
DISPLAY " "

*> Per the warning at the top of the module, this is the section that
*> has to match the script

DISPLAY header
DISPLAY withpic
DISPLAY def-BINARY-CHAR            "   def-BINARY-CHAR             "
DISPLAY def-BINARY-CHAR-SIGNED     "   def-BINARY-CHAR-SIGNED      "
DISPLAY def-BINARY-CHAR-UNSIGNED   "   def-BINARY-CHAR-UNSIGNED    "
DISPLAY def-BINARY-C-LONG          "   def-BINARY-C-LONG           "
DISPLAY def-BINARY-C-LONG-SIGNED   "   def-BINARY-C-LONG-SIGNED    "
DISPLAY def-BINARY-C-LONG-UNSIGNED "   def-BINARY-C-LONG-UNSIGNED  "
DISPLAY def-BINARY-DOUBLE          "   def-BINARY-DOUBLE           "
DISPLAY def-BINARY-DOUBLE-SIGNED   "   def-BINARY-DOUBLE-SIGNED    "
DISPLAY def-BINARY-DOUBLE-UNSIGNED "   def-BINARY-DOUBLE-UNSIGNED  "
DISPLAY def-BINARY-INT             "   def-BINARY-INT              "
DISPLAY def-BINARY-LONG            "   def-BINARY-LONG             "
DISPLAY def-BINARY-LONG-SIGNED     "   def-BINARY-LONG-SIGNED      "
DISPLAY def-BINARY-LONG-UNSIGNED   "   def-BINARY-LONG-UNSIGNED    "
DISPLAY def-BINARY-LONG-LONG       "   def-BINARY-LONG-LONG        "
DISPLAY def-BINARY-SHORT           "   def-BINARY-SHORT            "
DISPLAY def-BINARY-SHORT-SIGNED    "   def-BINARY-SHORT-SIGNED     "
DISPLAY def-BINARY-SHORT-UNSIGNED  "   def-BINARY-SHORT-UNSIGNED   "
DISPLAY def-COMPUTATIONAL          "   def-COMPUTATIONAL           "
DISPLAY def-COMP-1                 "   def-COMP-1                  "
DISPLAY def-COMP-2                 "   def-COMP-2                  "
DISPLAY def-COMPUTATIONAL-1        "   def-COMPUTATIONAL-1         "
DISPLAY def-COMPUTATIONAL-2        "   def-COMPUTATIONAL-2         "
DISPLAY def-FLOAT-DECIMAL-16       "   def-FLOAT-DECIMAL-16        "
DISPLAY def-FLOAT-DECIMAL-34       "   def-FLOAT-DECIMAL-34        "
DISPLAY def-FLOAT-LONG             "   def-FLOAT-LONG              "
DISPLAY def-FLOAT-SHORT            "   def-FLOAT-SHORT             "
DISPLAY def-INDEX                  "   def-INDEX                   "
DISPLAY def-SIGNED-INT             "   def-SIGNED-INT              "
DISPLAY def-SIGNED-LONG            "   def-SIGNED-LONG             "
DISPLAY def-SIGNED-SHORT           "   def-SIGNED-SHORT            "
DISPLAY def-UNSIGNED-INT           "   def-UNSIGNED-INT            "
DISPLAY def-UNSIGNED-LONG          "   def-UNSIGNED-LONG           "
DISPLAY def-UNSIGNED-SHORT         "   def-UNSIGNED-SHORT          "
DISPLAY def-COMP-3                 "   def-COMP-3                  "
DISPLAY def-COMPUTATIONAL-3        "   def-COMPUTATIONAL-3         "
DISPLAY def-PACKED-DECIMAL         "   def-PACKED-DECIMAL          "
DISPLAY def-PACKED-DECIMAL-S       "   def-PACKED-DECIMAL-S        "
DISPLAY def-BINARY                 "   def-BINARY                  "
DISPLAY def-BINARY-S               "   def-BINARY-S                "
DISPLAY def-COMP                   "   def-COMP                    "
DISPLAY def-COMPUTATIONAL          "   def-COMPUTATIONAL           "
DISPLAY def-COMP-4                 "   def-COMP-4                  "
DISPLAY def-COMPUTATIONAL-4        "   def-COMPUTATIONAL-4         "
DISPLAY def-COMP-5                 "   def-COMP-5                  "
DISPLAY def-COMPUTATIONAL-5        "   def-COMPUTATIONAL-5         "
DISPLAY def-COMP-6                 "   def-COMP-6                  "
DISPLAY def-COMP-X                 "   def-COMP-X                  "
DISPLAY def-COMPUTATIONAL-X        "   def-COMPUTATIONAL-X         "
DISPLAY def-COMP-XX                "   def-COMP-XX                 "
DISPLAY def-DISPLAY                "   def-DISPLAY                 "
*>GOBACK
move 1 to def-BINARY-LONG ; display def-BINARY-LONG






GOBACK.


