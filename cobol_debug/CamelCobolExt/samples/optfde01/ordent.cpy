      *> Order Entry Input Transactions (forms data)
      *> OEIT is Order Entry Input Transaction
      *> For each order there is one "header line"
       01 oeit. 
          05 oeit-custnum      pic 9(6). 
          05 oeit-orddate      pic 9(8). 
          05 oeit-ordnum       pic 99. 
          05 oeit-rectype      pic x. 
          05 oeit-lineno       pic 99.
          05 oeit-ponum        pic x(8).
          05 oeit-filler       pic x(8).
      *> For each order there is one to 20 "detail lines" (Products
      *> ordered and the quantities requested.
       01 oeitl.
          05 oeitl-custnum      pic 9(6). 
          05 oeitl-orddate      pic 9(8).
          05 oeitl-ordnum       pic 99. 
          05 oeitl-rectype      pic x. 
          05 oeitl-lineno       pic 99.
          05 oeitl-prodnum      pic 9(8). 
          05 oeitl-qty          pic 9(8).
      *> The program recognizes the record types based on the 
      *> contents of oeit-rectype.  "H" is a header, "L" is a 
       01 ordmast. 
          05 oeit-custnum      pic 9(6). 
          05 oeit-orddate      pic 9(8).
          05 oeit-ordnum       pic 99. 
          05 oeit-ponum        pic x(8).
       01 ordline.
          05 oeitl-lineno       pic 99.
          05 oeitl-prodnum      pic 9(8). 
          05 oeitl-qty          pic 9(8).
