        IDENTIFICATION   DIVISION.
        PROGRAM-ID.      "prog".
        DATA             DIVISION.
        WORKING-STORAGE  SECTION.
        01 TAB01.
          05 TABD          PIC  s9(2) comp-5 value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB01.
      
        01 TAB02.
          05 TABD          PIC  s9(4) comp-5 value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB02.
      
        01 TAB03.
          05 TABD          PIC  s9(8) comp-5 value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB03.
      
        01 TAB04.
          05 TABD          PIC  s9(16) comp-5 value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB04.
      
        01 TAB05.
          05 TABD          PIC  9(2) comp-5 value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB05.
      
        01 TAB06.
          05 TABD          PIC  9(4) comp-5 value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB06.
      
        01 TAB07.
          05 TABD          PIC  9(8) comp-5 value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB07.
      
        01 TAB08.
          05 TABD          PIC  9(16) comp-5 value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB08.
      
        01 TAB11.
          05 TABD          PIC  s9(2) binary value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB11.
      
        01 TAB12.
          05 TABD          PIC  s9(4) binary value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB12.
      
        01 TAB13.
          05 TABD          PIC  s9(8) binary value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB13.
      
        01 TAB14.
          05 TABD          PIC  s9(16) binary value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB14.
      
        01 TAB15.
          05 TABD          PIC  9(2) binary value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB15.
      
        01 TAB16.
          05 TABD          PIC  9(4) binary value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB16.
      
        01 TAB17.
          05 TABD          PIC  9(8) binary value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB17.
      
        01 TAB18.
          05 TABD          PIC  9(16) binary value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB18.
      
        01 TAB31.
          05 TABD          PIC  s9(2) display value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB31.
      
        01 TAB32.
          05 TABD          PIC  s9(4) display value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB32.
      
        01 TAB33.
          05 TABD          PIC  s9(8) display value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB33.
      
        01 TAB34.
          05 TABD          PIC  s9(16) display value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB34.
      
        01 TAB35.
          05 TABD          PIC  9(2) display value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB35.
      
        01 TAB36.
          05 TABD          PIC  9(4) display value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB36.
      
        01 TAB37.
          05 TABD          PIC  9(8) display value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB37.
      
        01 TAB38.
          05 TABD          PIC  9(16) display value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB38.
      
        01 TAB41.
          05 TABD          PIC  s9(2) COMP-3 value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB41.
      
        01 TAB42.
          05 TABD          PIC  s9(4) COMP-3 value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB42.
      
        01 TAB43.
          05 TABD          PIC  s9(8) COMP-3 value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB43.
      
        01 TAB44.
          05 TABD          PIC  s9(16) COMP-3 value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB44.
      
        01 TAB45.
          05 TABD          PIC  9(2) COMP-3 value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB45.
      
        01 TAB46.
          05 TABD          PIC  9(4) COMP-3 value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB46.
      
        01 TAB47.
          05 TABD          PIC  9(8) COMP-3 value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB47.
      
        01 TAB48.
          05 TABD          PIC  9(16) COMP-6 value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB48.
      
        01 TAB51.
          05 TABD          PIC  9(2) COMP-6 value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB51.
      
        01 TAB52.
          05 TABD          PIC  9(4) COMP-6 value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB52.
      
        01 TAB53.
          05 TABD          PIC  9(8) COMP-6 value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB53.
      
        01 TAB54.
          05 TABD          PIC  9(16) COMP-6 value 15.
          05 TABELEMENT
             PIC 99
             OCCURS           1 TO 20 TIMES
             DEPENDING ON     TABD OF TAB54.

      *>  As of 2024-01-29, CBL-GDB doesn't try to handle floating-point ODO
      *>  01 TAB61.
      *>    05 TABD           COMP-1 value 15.
      *>    05 TABELEMENT
      *>       PIC 99
      *>       OCCURS           1 TO 20 TIMES
      *>       DEPENDING ON     TABD OF TAB61.
      *>
      *>  01 TAB62.
      *>    05 TABD           COMP-2 value 15.
      *>    05 TABELEMENT
      *>       PIC 99
      *>       OCCURS           1 TO 20 TIMES
      *>       DEPENDING ON     TABD OF TAB62.

        01 I PIC 99.
        PROCEDURE DIVISION.
        PERFORM VARYING I FROM 1 BY 1 UNTIL I>20
            MOVE I TO TABELEMENT OF TAB01(I)
            MOVE I TO TABELEMENT OF TAB02(I)
            MOVE I TO TABELEMENT OF TAB03(I)
            MOVE I TO TABELEMENT OF TAB04(I)
            MOVE I TO TABELEMENT OF TAB05(I)
            MOVE I TO TABELEMENT OF TAB06(I)
            MOVE I TO TABELEMENT OF TAB07(I)
            MOVE I TO TABELEMENT OF TAB08(I)
            MOVE I TO TABELEMENT OF TAB11(I)
            MOVE I TO TABELEMENT OF TAB12(I)
            MOVE I TO TABELEMENT OF TAB13(I)
            MOVE I TO TABELEMENT OF TAB14(I)
            MOVE I TO TABELEMENT OF TAB15(I)
            MOVE I TO TABELEMENT OF TAB16(I)
            MOVE I TO TABELEMENT OF TAB17(I)
            MOVE I TO TABELEMENT OF TAB18(I)
            MOVE I TO TABELEMENT OF TAB31(I)
            MOVE I TO TABELEMENT OF TAB32(I)
            MOVE I TO TABELEMENT OF TAB33(I)
            MOVE I TO TABELEMENT OF TAB34(I)
            MOVE I TO TABELEMENT OF TAB35(I)
            MOVE I TO TABELEMENT OF TAB36(I)
            MOVE I TO TABELEMENT OF TAB37(I)
            MOVE I TO TABELEMENT OF TAB38(I)
            MOVE I TO TABELEMENT OF TAB41(I)
            MOVE I TO TABELEMENT OF TAB42(I)
            MOVE I TO TABELEMENT OF TAB43(I)
            MOVE I TO TABELEMENT OF TAB44(I)
            MOVE I TO TABELEMENT OF TAB45(I)
            MOVE I TO TABELEMENT OF TAB46(I)
            MOVE I TO TABELEMENT OF TAB47(I)
            MOVE I TO TABELEMENT OF TAB48(I)
            MOVE I TO TABELEMENT OF TAB51(I)
            MOVE I TO TABELEMENT OF TAB52(I)
            MOVE I TO TABELEMENT OF TAB53(I)
            MOVE I TO TABELEMENT OF TAB54(I)
      *>      MOVE I TO TABELEMENT OF TAB61(I)
      *>      MOVE I TO TABELEMENT OF TAB62(I)
            END-PERFORM

        GOBACK.
        END PROGRAM "prog".

