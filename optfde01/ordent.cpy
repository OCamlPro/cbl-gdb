      *> /* #######################################################################
      *> # Copyright (c) 2019-2020 COBOLworx Corporation
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
      *> #    * Neither the name of the COBOLworx Corporation nor the names of its
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
