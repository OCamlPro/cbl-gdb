       >>DEFINE CONSTANT log-level AS 2 
      *> RUN WITH:
      *>   dd_optfdein=src/data/optfraw dd_optfdeou=src/data/optfdsort /
      *>       optfde01
      *> JCL equivalent is src/jcl/optfde01.jcl (I hope)
      
       IDENTIFICATION DIVISION. 
       PROGRAM-ID. optfde01. 
      
       ENVIRONMENT DIVISION.
       INPUT-OUTPUT SECTION.
       FILE-CONTROL.
           SELECT pgm-input ASSIGN TO "dd_input"
              ORGANIZATION IS LINE SEQUENTIAL.
           SELECT pgm-output ASSIGN TO "dd_output"
              ORGANIZATION IS LINE SEQUENTIAL.
      * *| Logging uses SYSPRING |*
      *    SELECT log-output ASSIGN TO "dd_sysprint"
      *       ORGANIZATION IS LINE SEQUENTIAL.
      
       DATA DIVISION.
       FILE SECTION.
       FD pgm-input.
           01 input-record PIC X(118).
       FD pgm-output.
           01 output-record PIC X(118).
      *FD log-output.
      *    01 output-record PIC X(118).
      
       WORKING-STORAGE SECTION.
       01  file-status         PICTURE x  VALUE space.
           88 END-OF-FILE             VALUE high-value
              WHEN SET TO false is          low-value.
       01 working-data.
          05 pgm-name      pic x(9)   VALUE "optfde01".
          05 old-date      pic 9(8)   VALUE ZERO.
          05 new-date      pic 9(8)   VALUE ZERO.
          05 my-counters.
             10 FILLER       PIC X(18) VALUE "Records modified: ".
             10 custmas-ctr  PIC 9(5) VALUE ZERO.
      
       LOCAL-STORAGE SECTION.
         copy ordent.
             
      *> ***************************************************************
       PROCEDURE DIVISION.
       main section.  
       00-main.
          DISPLAY "Program name: " pgm-name.
          open input pgm-input.
          open output pgm-output.
          INITIALIZE working-data.
          PERFORM 01-read.
          PERFORM until end-of-file
             PERFORM 02-process 
             PERFORM 01-read
             END-PERFORM.
          DISPLAY my-counters.
          MOVE 0 to RETURN-CODE.
          PERFORM 59-close. 
          goback.
      
       support section.
       01-read.
          READ pgm-input 
             AT END SET END-OF-FILE TO TRUE.
      
       02-process.
          MOVE input-record TO oeit.
          PERFORM 50-fix-date.
          MOVE oeit TO output-record.
          WRITE output-record.
          ADD 1 TO custmas-ctr.
      
       50-fix-date.  *> Gratuitous sub paragraph
          MOVE oeit-orddate IN oeit to old-date.
          STRING old-date(5:4) old-date(1:4)
             INTO new-date.
          MOVE new-date TO oeit-orddate in oeit.

       59-close. 
          CLOSE pgm-input.
          CLOSE pgm-output.

       END PROGRAM optfde01.
