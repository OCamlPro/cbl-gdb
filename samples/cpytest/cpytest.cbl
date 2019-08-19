       IDENTIFICATION DIVISION.
       PROGRAM-ID. cpytest.
       ENVIRONMENT DIVISION.
       INPUT-OUTPUT SECTION.
       DATA DIVISION.
       FILE SECTION.
       WORKING-STORAGE SECTION.
       LINKAGE SECTION.
       PROCEDURE DIVISION.
       MAIN SECTION.
       00-MAIN.
         DISPLAY " ".
         DISPLAY "This line is from the main source."
         DISPLAY " ".
         COPY include REPLACING ==$MSG$== BY "I am a robot.".
         DISPLAY " ".
         DISPLAY "This line is from the main source, again.".
         DISPLAY " ".

       END PROGRAM cpytest.
