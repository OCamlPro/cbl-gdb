       IDENTIFICATION DIVISION. 
       PROGRAM-ID. checkreg. 
      *
        ENVIRONMENT DIVISION.
        INPUT-OUTPUT SECTION.
        FILE-CONTROL.
            SELECT trans-raw-file ASSIGN TO "TRANSACTIONS"
                ORGANIZATION IS LINE SEQUENTIAL.
            SELECT trans-work-file ASSIGN TO "TRANSWORK"
                ORGANIZATION IS LINE SEQUENTIAL.
            SELECT trans-input-file ASSIGN TO "TRANSSORT"
                ORGANIZATION IS LINE SEQUENTIAL.
            SELECT pgm-output ASSIGN TO "REPORT"
                ORGANIZATION IS LINE SEQUENTIAL.
      *
        DATA DIVISION.
        FILE SECTION.
        FD trans-raw-file.
            01 trans-raw. *> 92 characters
                05 trans-seq PIC 9999 .
                05 FILLER PIC X.
                05 trans-date.
                    10 trans-date-year PIC 9999 .
                    10 FILLER PIC X.
                    10 trans-date-month PIC 99 .
                    10 FILLER PIC X.
                    10 trans-date-day PIC 99 . 
                05 FILLER        PIC X.
                05 trans-type    PIC X(11) .
                05 FILLER        PIC X.
                05 trans-desc    PIC X(27) .
                05 FILLER        PIC X.
                05 trans-debit   PIC 999999.99 .
                05 FILLER        PIC X.
                05 trans-credit  PIC 999999.99 .
                05 FILLER        PIC X.
                05 trans-checkno PIC 9(6) .
                05 FILLER        PIC X.
                05 trans-runbal  PIC 999999.99 .
      *
        FD trans-input-file.
            01 trans-input. *> 92 characters
                05 trans-seq PIC 9999 .
                05 FILLER PIC X.
                05 trans-date.
                    10 trans-date-year PIC 9999 .
                    10 FILLER PIC X.
                    10 trans-date-month PIC 99 .
                    10 FILLER PIC X.
                    10 trans-date-day PIC 99 . 
                05 FILLER        PIC X.
                05 trans-type    PIC X(11) .
                05 FILLER        PIC X.
                05 trans-desc    PIC X(27) .
                05 FILLER        PIC X.
                05 trans-debit   PIC 999999.99 .
                05 FILLER        PIC X.
                05 trans-credit  PIC 999999.99 .
                05 FILLER        PIC X.
                05 trans-checkno PIC 9(6) .
                05 FILLER        PIC X.
                05 trans-runbal  PIC 999999.99 .
      *
        SD trans-work-file.
            01 trans-work. *> 92 characters
                05 trans-seq PIC 9999 .
                05 FILLER PIC X.
                05 trans-date.
                    10 trans-date-year PIC 9999 .
                    10 FILLER PIC X.
                    10 trans-date-month PIC 99 .
                    10 FILLER PIC X.
                    10 trans-date-day PIC 99 . 
                05 FILLER        PIC X.
                05 trans-type    PIC X(11) .
                05 FILLER        PIC X.
                05 trans-desc    PIC X(27) .
                05 FILLER        PIC X.
                05 trans-debit   PIC 999999.99 .
                05 FILLER        PIC X.
                05 trans-credit  PIC 999999.99 .
                05 FILLER        PIC X.
                05 trans-checkno PIC 9(6) .
                05 FILLER        PIC X.
                05 trans-runbal  PIC 999999.99 .
      *
        FD pgm-output.
            01 output-record PIC X(132).
      
        WORKING-STORAGE SECTION.
        01 file-status PIC x  VALUE space.
            88 END-OF-FILE VALUE high-value
              WHEN SET TO false is low-value.
        01 line-count PIC 99 VALUE 99 .
        01 print-buffer PIC X(132).
        01 print-buffer-save PIC X(132).
        01 print-header-line-1.
            05 FILLER PIC X(10) VALUE SPACES.
            05 FILLER PIC X(122) VALUE "This is fixed boilerplate text t"
      -        "hat we want at the top of each page".
      *              
        01 running-balance PIC S9(6)V9(2).
        01 running-debits  PIC S9(6)V9(2).
        01 running-credits PIC S9(6)V9(2).
        01 running-amount  PIC S9(6)V9(2).
        01 ending-balance  PIC S9(6)V9(2).
        01 print-starting-balance.
            05 FILLER PIC X(24) VALUE "The starting balance is ".
            05 balance PIC ZZZ,ZZ9.99- .
        01 print-ending-balance.
            05 FILLER PIC X(22) VALUE "The ending balance is ".
            05 balance PIC ZZZ,ZZ9.99- .
        01 print-running-balance-header.
            05 FILLER PIC X(11) VALUE "Date".
            05 FILLER PIC X(12) VALUE "Type".
            05 FILLER PIC X(28) VALUE "Description".
            05 FILLER PIC X(12) VALUE "Amount".
            05 FILLER PIC X(15) VALUE "Running balance".
        01 print-running-balance.
            05 prb-date PIC X(11).
            05 prb-type PIC X(12).
            05 prb-desc PIC X(28).
            05 prb-amount PIC ZZZ,ZZ9.99- .
            05 FILLER PIC X.
            05 prb-runbal PIC ZZZ,ZZ9.99- .
      *
        PROCEDURE DIVISION.
        main SECTION.  
        0000-main.
            PERFORM 5000-sort-input.
            OPEN input trans-input-file.
            OPEN output pgm-output.
            PERFORM 1000-read.
            PERFORM until end-of-file
                PERFORM 2000-process 
                PERFORM 1000-read
                END-PERFORM.
            MOVE 0 to RETURN-CODE.
            PERFORM 9000-close.
            GOBACK.
      *
        support SECTION.
        1000-read.
            READ trans-input-file 
                AT END SET END-OF-FILE TO TRUE.
      *
        2000-process.
            *> DISPLAY trans-input.
            EVALUATE trans-type IN trans-input
                WHEN "BALSTART" 
                    PERFORM 2100-balance-start
                WHEN "DEBIT" 
                    PERFORM 2200-debit
                WHEN "DIRECTDEBIT" 
                    PERFORM 2200-debit
                WHEN "CHECK" 
                    PERFORM 2200-debit
                WHEN "CREDIT" 
                    PERFORM 2300-credit
                WHEN "DIRECTDEP" 
                    PERFORM 2300-credit
                WHEN "DEP" 
                    PERFORM 2300-credit
                WHEN "INT" 
                    PERFORM 2300-credit
                WHEN "BALEND" 
                    PERFORM 2400-balance-end
                WHEN OTHER 
                    DISPLAY "Unknown type: " trans-type IN trans-input
                    PERFORM 9900-die-horribly.
      *
        2100-balance-start.
            MOVE trans-runbal IN trans-input TO running-balance.
            MOVE running-balance TO balance in print-starting-balance.
            MOVE print-starting-balance to print-buffer.
            PERFORM write-line 2 times.
      *
        2200-debit.
            ADD FUNCTION NUMVAL(trans-debit IN trans-input)
                TO running-debits.
            SUBTRACT FUNCTION NUMVAL(trans-debit IN trans-input)
                FROM running-balance.
            MOVE FUNCTION NUMVAL(trans-debit IN trans-input)
                 TO running-amount.
            MULTIPLY running-amount BY -1 GIVING running-amount.
            PERFORM 2350-running-balance.
      *
        2300-credit.
            ADD FUNCTION NUMVAL(trans-credit IN trans-input)
                TO running-credits.
            ADD FUNCTION NUMVAL(trans-credit IN trans-input)
                TO running-balance.
            MOVE FUNCTION NUMVAL(trans-credit IN trans-input)
                TO running-amount.
            PERFORM 2350-running-balance.
      *
        2350-running-balance.
            MOVE trans-date IN trans-input TO prb-date.
            MOVE trans-type IN trans-input TO prb-type.
            MOVE trans-desc IN trans-input TO prb-desc.
            MOVE running-amount TO prb-amount.
            MOVE running-balance TO prb-runbal.
            MOVE print-running-balance TO print-buffer.
            PERFORM write-line.
      *
        2400-balance-end.
            PERFORM write-line.
            MOVE trans-runbal IN trans-input TO ending-balance.
            MOVE ending-balance TO balance in print-ending-balance.
            MOVE print-ending-balance to print-buffer.
            PERFORM write-line.
      *
        write-line-actual.
            DISPLAY print-buffer.
            MOVE print-buffer to output-record.
            WRITE output-record AFTER ADVANCING 1 LINES.
            MOVE SPACES to print-buffer.
            ADD 1 TO line-count.
      *
        write-line.
            IF line-count > 60 THEN
               MOVE print-buffer to print-buffer-save
               MOVE ZERO TO line-count
               MOVE print-header-line-1 TO print-buffer
               PERFORM write-line-actual 3 TIMES
               MOVE print-buffer-save to print-buffer
               END-IF.
            PERFORM write-line-actual.
      *
        5000-sort-input.
            SORT trans-work-file 
                ON ASCENDING KEY trans-seq OF trans-work
                USING trans-raw-file GIVING trans-input-file.
      *
        9000-close. 
            CLOSE trans-input-file.
            CLOSE pgm-output.
      *
        9900-die-horribly.
            PERFORM 9000-close.
            DISPLAY "Aborting".
            STOP RUN.
      *
       END PROGRAM checkreg.
