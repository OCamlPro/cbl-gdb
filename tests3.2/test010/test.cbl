       IDENTIFICATION   DIVISION.
       PROGRAM-ID.      "prog".
       DATA             DIVISION.
       WORKING-STORAGE  SECTION.
       01  MYFLD        PIC X(10) VALUE "Hey   you!".
       01  LINK         PIC X(10).
       PROCEDURE        DIVISION.
      *> set a breakpoint here and cwatch LINK:
           DISPLAY MYFLD
      *> first trigger should happen here
           MOVE MYFLD TO LINK
           CALL "sub" USING LINK
      *> should display "Ho    you!"
           DISPLAY LINK
           STOP RUN.
       END PROGRAM "prog".

       IDENTIFICATION   DIVISION.
       PROGRAM-ID.      "sub".
       DATA             DIVISION.
       LINKAGE          SECTION.
       01  MYFLD2        PIC X(6).
       PROCEDURE        DIVISION using MYFLD2.
      *> should be displayed fine as "Hey   "
           DISPLAY MYFLD2
      *> second trigger should happen here
           MOVE "Ho" TO MYFLD2
           GOBACK.
       END PROGRAM "sub".
