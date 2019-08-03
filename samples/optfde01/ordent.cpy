      *> Order Entry Input Transactions (forms data)
      *> OEIT is Order Entry Input Transaction
      *> For each order there is one "header line"
       01 oeit. 
          05 oeit-custnum pic 9(6). 
          05 oeit-orddate pic 9(8).  *> Incoming is MMDDYYYY -> YYYYMMDD. 
          05 oeit-ordnum       pic 99. 
          05 oeit-rectype      pic x. 
          05 oeit-lineno       pic 99.
          05 oeit-hbody.
             10 oeith-ponum     pic x(8).
             10 oeith-filler    pic x(8).
      *> For each order there is one to 20 "detail lines" (Products
      *> ordered and the quantities requested.
       05 oeit-lbody redefines oeit-hbody.
          10 oeitl-prodnum      pic 9(8). 
          10 oeitl-qty          pic 9(8).
      *> The program recognizes the record types based on the 
      *> contents of oeit-rectype.  "H" is a header, "L" is a 
      *> detail line.
 
