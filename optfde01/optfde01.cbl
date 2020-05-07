      *> /* #######################################################################
      *> # Copyright (c) 2019-2020 Symas Corporation
      *> # All rights reserved.
      *> #
      *> # Redistribution and use in source and binary forms, with or without
      *> # modification, are permitted provided that the following conditions are met:
      *> #
      *> #    * Redistributions of source code must retain the above copyright
      *> #      notice, this list of conditions and the following disclaimer.
      *> #    * Redistributions in binary form must reproduce the above copyright
      *> #      notice, this list of conditions and the following disclaimer in
      *> #      the documentation and/or other materials provided with the
      *> #      distribution.
      *> #    * Neither the name of the Symas Corporation nor the names of its
      *> #      contributors may be used to endorse or promote products derived
      *> #      from this software without specific prior written permission.
      *> #
      *> # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
      *> # AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
      *> # IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
      *> # DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
      *> # FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
      *> # DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
      *> # SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
      *> # CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
      *> # OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
      *> # OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
      *> #
      *> ############################################################################ *
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
      *> Gratuitous redefines for cbl-gdb demo
          05 old-dat1 REDEFINES old-date.
             10 datmo      PIC 9(2).
             10 datda      PIC 9(2).
             10 datyr      PIC 9(4).
          05 new-date      pic 9(8)   VALUE ZERO.
      *> Gratuitous redefines for cbl-gdb demo
          05 new-dat1 REDEFINES new-date.
             10 datyr      PIC 9(4).
             10 datmo      PIC 9(2).
             10 datda      PIC 9(2).
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
