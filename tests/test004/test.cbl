        IDENTIFICATION DIVISION.
        PROGRAM-ID. test.

        DATA DIVISION.
           WORKING-STORAGE SECTION.
           01 WS-TABLE.
              05 WS-A OCCURS 3 TIMES.
                 10 WS-B PIC A(2).
                 10 WS-C OCCURS 2 TIMES.
                    15 WS-D PIC X(3).

        01 Z-TABLE                EXTERNAL.
           05 Z-PID               PIC 9(08).
           05 Z-ADR               PIC 9(02).
           05 T-ENTRIES OCCURS 90 INDEXED BY TAB-ADR-IND.
             08 T-ADR                 PIC X(16).
             08 T-STAT                PIC X.
             08 SUBTABLE OCCURS 10 INDEXED BY SUBINDEX.
               10 SUBTEXT  PIC X(11).
               10 SUBTEXT2 PIC X(11).
             08 T-PRGM                PIC X(08).
             08 T-ID                  PIC X(02).
             08 T-ADR-SAVE            PIC S9(16) COMP-5.

        01 T_ENTRY PIC 9999 VALUE 13.
        01 T_ADDRESSES OCCURS 0 TO 1000 TIMES
           DEPENDING ON T_ENTRY
           ASCENDING KEY IS TAB-ADR-PRGM-ID
           INDEXED BY TAB-ADR-IND.
           05 TAB-ADR-ELEMENT.
             10 TAB-ADR-PRGM-ID.
               15 TAB-ADR-PRGM      PIC X(8).
               15 TAB-ADR-ID        PIC X(2).
             10 TAB-ADR-ADR-64      PIC S9(16) COMP-5.
             10 TAB-ADR-LAST-ADR-64 PIC S9(16) COMP-5.

        01 zed   PIC 9999 VALUE 0.
        01 one   PIC 9999 VALUE 1.
        01 two   PIC 9999 VALUE 2.

        01 nums1.
            05 three PIC 9999 VALUE 3.
            05 four  PIC 9999 VALUE 4.

        01 nums2.
            05 three PIC 9999 VALUE 3.
            05 four  PIC 9999 VALUE 4.

        01 svalue PIC S999999 BINARY VALUE -123456.

        01 WS-BASED PIC X(64) BASED.

        77 var-one pic 9 value 1.
        77 var-two pic 9 value 2.
        01 .
           03 var-tab pic x occurs 10.
           03 var-entry pic x.

        PROCEDURE DIVISION.

            move "1" to var-tab(1)
            move "2" to var-tab(2)
            move "3" to var-tab(3)
            move "4" to var-tab(4)
            move "5" to var-tab(5)
            move "6" to var-tab(6)
            move "7" to var-tab(7)
            move "8" to var-tab(8)
            move "9" to var-tab(9)
            move "0" to var-tab(10)
        
           MOVE 87654321 to Z-PID.
           MOVE 2 TO SUBINDEX.
           MOVE 3 TO TAB-ADR-IND in T_ADDRESSES.
           MOVE "subtext" to SUBTEXT(TAB-ADR-IND OF Z-TABLE,SUBINDEX).
           DISPLAY 'TAB-ADR-IND  : ' TAB-ADR-IND of T_ADDRESSES.
           DISPLAY 'Z-PID  : ' Z-PID.

           MOVE '12ABCDEF34GHIJKL56MNOPQR' TO WS-TABLE.
           DISPLAY 'WS-TABLE  : ' WS-TABLE.
           DISPLAY 'WS-A(1)   : ' WS-A(1).
           DISPLAY 'WS-C(1,1) : ' WS-C(1,1).
           DISPLAY 'WS-C(1,2) : ' WS-C(1,2).
           DISPLAY 'WS-A(2)   : ' WS-A(2).
           DISPLAY 'WS-C(2,1) : ' WS-C(2,1).
           DISPLAY 'WS-C(2,2) : ' WS-C(2,2).
           DISPLAY 'WS-A(3)   : ' WS-A(3).
           DISPLAY 'WS-C(3,1) : ' WS-C(3,1).
           DISPLAY 'WS-C(3,2) : ' WS-C(3,2).
           DISPLAY 'WS-D(1,1) : ' WS-D(1,1).
           DISPLAY 'WS-D(3,2) : ' WS-D(3,2).
           DISPLAY 'WS-D(3;2) : ' WS-D(3;2).
           DISPLAY 'WS-D(3 2) : ' WS-D(3 2).
           DISPLAY 'WS-D(3,,,,,2) : ' WS-D(3,,,,,2).

           DISPLAY 'ONE       : ' ONE.
           DISPLAY 'TWO       : ' TWO.
           DISPLAY 'THREE     : ' THREE in nums1.
           DISPLAY 'FOUR      : ' FOUR  in nums1.

           MOVE "Hi, bob!" to T-ADR(1).
           MOVE "Hi, mom!" to T-ADR(2).
           MOVE "Hi, marty!" to T-ADR(90).
           DISPLAY 'T-ADR(1): ' T-ADR(1).
           DISPLAY 'T-ADR(2): ' T-ADR(2).
           DISPLAY 'T-ADR(90): ' T-ADR(90).

           MOVE "First" TO TAB-ADR-PRGM(1)
           MOVE "11"    TO TAB-ADR-ID(1)
           MOVE 1111    TO TAB-ADR-ADR-64(1)
           MOVE 111111  TO TAB-ADR-LAST-ADR-64(1)

           MOVE "First---" TO TAB-ADR-PRGM(1)
           MOVE "11"    TO TAB-ADR-ID(1)
           MOVE 1111    TO TAB-ADR-ADR-64(1)
           MOVE 111111  TO TAB-ADR-LAST-ADR-64(1)

           MOVE "Second--" TO TAB-ADR-PRGM(2)
           MOVE "22"     TO TAB-ADR-ID(2)
           MOVE 2222     TO TAB-ADR-ADR-64(2)
           MOVE 222222   TO TAB-ADR-LAST-ADR-64(2)

           DISPLAY 'svalue : ' svalue
           DISPLAY 'MOVE 89 to svalue(2:2)'
           MOVE 89 to svalue(2:2)
           DISPLAY 'svalue : ' svalue
           MOVE "89" to svalue(2:2)
           DISPLAY 'svalue : ' svalue
           MOVE -123456 to svalue.

           ALLOCATE WS-BASED.
           MOVE "I am a BASED variable" to WS-BASED.
           DISPLAY WS-BASED.

           STOP RUN.

        END PROGRAM test.
