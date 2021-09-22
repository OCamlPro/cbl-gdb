       IDENTIFICATION   DIVISION.
       PROGRAM-ID.      test.
       DATA             DIVISION.
       WORKING-STORAGE  SECTION.
        01  XTEST PIC XXXX COMP-X.
        01  FCD-MAP.
            05  FCD-FILE-STATUS.
                10  FCD-STATUS-KEY-1 PIC X.
                10  FCD-STATUS-KEY-2 PIC X.
                10  FCD-BINARY       REDEFINES FCD-STATUS-KEY-2
                                     PIC X COMP-X.
            05  FCD-LENGTH            PIC XX COMP-X.
            05  FCD-VERSION           PIC X COMP-X.
               78 FCD-VERSION-NUMBER           VALUE 0.
      
            05  FCD-ORGANIZATION     PIC 9(2) COMP-X.
              78 fcd--line-sequential-org      value 0.
              78 fcd--sequential-org           value 1.
              78 fcd--indexed-org              value 2.
              78 fcd--relative-org             value 3.
              78 fcd--determine-org            value 255. *> see opcode 0006
            05  FCD-ACCESS-MODE      PIC 9(2) COMP-X.
              78 fcd--sequential-access        value 0.
              78 fcd--dup-prime-access         value 1.
              78 fcd--random-access            value 4.
              78 fcd--dynamic-access           value 8.
              78 fcd--status-defined           value h"80".
            05  FCD-OPEN-MODE        PIC 9(2) COMP-X.
              78 fcd--open-input               value 0.
              78 fcd--open-output              value 1.
              78 fcd--open-i-o                 value 2.
              78 fcd--open-extend              value 3.
              78 fcd--open-max                 value 3.
              78 fcd--open-closed              value 128.
       PROCEDURE DIVISION.
       MOVE 258 to XTEST
       DISPLAY XTEST
       MOVE FCD--open-closed to FCD-OPEN-MODE
       DISPLAY FCD-OPEN-MODE
       GOBACK.

