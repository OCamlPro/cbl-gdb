       IDENTIFICATION DIVISION.
       PROGRAM-ID. bob01.
       ENVIRONMENT DIVISION.
       CONFIGURATION SECTION.
          REPOSITORY. FUNCTION ALL INTRINSIC.
       DATA DIVISION.
       WORKING-STORAGE SECTION.
       77 num pic 999 value 832.
       77 result pic 999 value 0.
       PROCEDURE DIVISION.
          DISPLAY "in 'main'".
          call 'add1' using by reference num, result.
           display result.
          call 'sub1' using by reference num, result.
           display result.
          exit.
       end program bob01.

       identification division.
       program-id. add1.
       environment division.
       CONFIGURATION SECTION.
          REPOSITORY. FUNCTION ALL INTRINSIC.
       DATA DIVISION.
         linkage section.
       77 aaaa pic 999 value 0.
       77 bbbb pic 999 value 0.
       procedure division using aaaa, bbbb.
           display "in add1".
           display aaaa " " bbbb.
          move aaaa to bbbb.
           display aaaa " " bbbb.
          add 1 to bbbb.
           display aaaa " " bbbb.
          goback.
          end program add1.

       identification division.
       program-id. sub1.
       environment division.
       CONFIGURATION SECTION.
          REPOSITORY. FUNCTION ALL INTRINSIC.
       DATA DIVISION.
         linkage section.
       77 aaaa pic 999 value 0.
       77 bbbb pic 999 value 0.
       procedure division using aaaa, bbbb.
           display "in add1".
           display aaaa " " bbbb.
          move aaaa to bbbb.
           display aaaa " " bbbb.
          subtract 1 from bbbb.
           display aaaa " " bbbb.
          goback.
          end program sub1.
