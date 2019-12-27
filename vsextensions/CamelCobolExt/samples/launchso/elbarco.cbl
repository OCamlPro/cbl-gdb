       ID DIVISION. 
       PROGRAM-ID. elbarco. 
       PROCEDURE DIVISION. 
       DISPLAY "   Somos el barco"
       DISPLAY "   Somos el mar"
       DISPLAY "   Yo navego en ti"
       DISPLAY "   Tu navegas en me"
       DISPLAY " "
       CALL "esquife"
       GOBACK.
       END PROGRAM elbarco.
       
      *> This is entertaining.  When this el-barco.so is executed by cobcrun,
      *> it behaves as expected: we see the first verse in Spanish, and the 
      *> second verse in English.
      *>
      *> But when el-barco is invoked from a C program via dlopen(), we get
      *> back an error message: 
      *>
      *>     libcob: elbarco.cbl:9: error: module 'esquife' not found
      *>
      *> You heard it here first: RJD 2019-12-14
       
       ID DIVISION. 
       PROGRAM-ID. esquife. 
       PROCEDURE DIVISION. 
       DISPLAY "   We are the boat"
       DISPLAY "   We are the sea"
       DISPLAY "   I sail in you"
       DISPLAY "   You sail in me"
       DISPLAY " "
       GOBACK.
       END PROGRAM esquife.
       
