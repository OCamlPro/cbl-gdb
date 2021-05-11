      * This program uses Report Writer to produce the
      * Century Medical Center Quarterly Payroll Register.

      * Modification History
      * Date      Programmer   Description
      * -------   ----------   ----------------------------------------
      * 02/2021   kday         This is a modified version of a program
      *                        originally written By Jay Moseley in
      *                        April, 2008.

       Identification Division.
       Program-Id. test.

       Environment Division.

       Configuration Section.

       Input-Output Section.
       File-Control.

           Select QtrlyPyrlRegData
               Assign To         "QtrlyPyrlRegData.dat"
               Organization Is   Line Sequential.

           Select QtrlyPyrlReg
               Assign To        "QtrlyPyrlReg.rpt".

       Data Division.

       File Section.

       Fd  QtrlyPyrlRegData.
       01  SortedQtrlyPyrlDataRec.
           05  QPRDRDeptNum            Pic 9(02).
           05  Filler                  Pic X(01).
           05  QPRDREmpKey.
               10  QPRDREmpNum         Pic 9(04).
               10  Filler              Pic X(01).
               10  QPRDREmpName        Pic X(20).
           05  Filler                  Pic X(01).
           05  QPRDRPayDate            Pic 9(08).
           05  Filler Redefines QPRDRPayDate.
               10  QPRDRPayDateYYYY    Pic 9(04).
               10  QPRDRPayDateMM      Pic 9(02).
               10  QPRDRPayDateDD      Pic 9(02).
           05  Filler                  Pic X(01).
           05  QPRDRGrossPay           Pic 9(04)V99.
           05  Filler                  Pic X(01).
           05  QPRDRFICA               Pic 9(03)V99.
           05  Filler                  Pic X(01).
           05  QPRDRFedWH              Pic 9(03)V99.
           05  Filler                  Pic X(01).
           05  QPRDRMiscDed            Pic 9(03)V99.
           05  Filler                  Pic X(01).
           05  QPRDRNetPay             Pic 9(04)V99.
           05  Filler                  Pic X(09).

       Fd  QtrlyPyrlReg
           Report Is QtrlyPyrlRegRpt.

       Working-Storage Section.

       77  EOFSwitch                   Pic X(01) Value 'N'.
           88  EOF                               Value 'Y'.

       01  DeptPcntsTbl.
           05  DeptPcntsTblDeptNum
               Occurs 6 Times
               Indexed By DeptPcntsTblDeptIndex.
               10  DeptPcntsTblPcntg
                   Occurs 5 Times
                   Indexed By DeptPcntsTblPcntgIndex
                                       Pic 9(03)V99.

       01  DeptTotsTblData.
           05  Filler           Pic X(17) Value '01Management     '.
           05  Filler           Pic X(50) Value Zeros.
           05  Filler           Pic X(17) Value '05Administration '.
           05  Filler           Pic X(50) Value Zeros.
           05  Filler           Pic X(17) Value '10Skilled Nursing'.
           05  Filler           Pic X(50) Value Zeros.
           05  Filler           Pic X(17) Value '15Patient Support'.
           05  Filler           Pic X(50) Value Zeros.
           05  Filler           Pic X(17) Value '20Housekeeping   '.
           05  Filler           Pic X(50) Value Zeros.
           05  Filler           Pic X(17) Value '25Maintenance    '.
           05  Filler           Pic X(50) Value Zeros.
       01  Filler Redefines DeptTotsTblData.
           05  DeptTotsTblRec          Occurs 6 Times
                                       Indexed By DeptTotsTblIndex.
               10  DeptTotsTblDeptNum      Pic 9(02).
               10  DeptTotsTblDeptName     Pic X(15).
               10  DeptTotsTblGrossPay     Pic 9(08)V99.
               10  DeptTotsTblFICA         Pic 9(08)V99.
               10  DeptTotsTblFedWH        Pic 9(08)V99.
               10  DeptTotsTblMiscDed      Pic 9(08)V99.
               10  DeptTotsTblNetPay       Pic 9(08)V99.

       01  RunDate.
           05  RunDateMMDDYYYY         Pic 9(08).
           05  RunDateMMDDYYYYFmt      Pic XX/XX/XXXX.
           05  RunDateYYYYMMDD         Pic 9(08).

       Report Section.

       Rd  QtrlyPyrlRegRpt
           Controls Are Final,
                        QPRDRDeptNum,
                        QPRDREmpKey
           Page Limit Is 66 Lines
           Heading 1
           First Detail 7
           Last Detail 60.

       01  Type Page Heading.
           05  Line 1.
               10  Column 01   Pic X(05) Value 'Date:'.
               10  Column 07   Pic X(10)
                   Source RunDateMMDDYYYYFmt.
               10  Column 39   Pic X(13) Value 'C E N T U R Y'.
               10  Column 55   Pic X(13) Value 'M E D I C A L'.
               10  Column 71   Pic X(11) Value 'C E N T E R'.
               10  Column 110  Pic X(05) Value 'Page:'.
               10  Column 116  Pic ZZ,ZZ9
                   Source Page-Counter.
           05  Line 2.
               10  Column 35   Pic X(17) Value 'Q U A R T E R L Y'.
               10  Column 55   Pic X(13) Value 'P A Y R O L L'.
               10  Column 71   Pic X(15) Value 'R E G I S T E R'.
           05  Line 4.
               10  Column 06   Pic X(28) Value
                   '--------- Employee ---------'.
               10  Column 40   Pic X(05) Value 'Gross'.
               10  Column 54   Pic X(04) Value 'FICA'.
               10  Column 66   Pic X(07) Value 'Fed W/H'.
               10  Column 80   Pic X(05) Value 'Misc.'.
               10  Column 95   Pic X(03) Value 'Net'.
           05  Line 5.
               10  Column 07   Pic X(03) Value 'Num'.
               10  Column 22   Pic X(04) Value 'Name'.
               10  Column 41   Pic X(03) Value 'Pay'.
               10  Column 55   Pic X(03) Value 'Tax'.
               10  Column 68   Pic X(03) Value 'Tax'.
               10  Column 79   Pic X(07) Value 'Deduct.'.
               10  Column 95   Pic X(03) Value 'Pay'.

       01  DeptHdr
           Type Control Heading QPRDRDeptNum
           Next Group Plus 1.
           05  Line Plus 1.
               10  Column 01   Pic X(18) Value 'Department Number:'.
               10  Column 21   Pic 9(02)
                   Source QPRDRDeptNum.
               10  Column 24   Pic X(15)
                   Source DeptTotsTblDeptName(DeptTotsTblIndex).

       01  EmpDtl
           Type Detail.
           05  Line Plus 1.
               10  Column 01   Pic X(25)
                   Source QPRDREmpKey.
               10  Column 50   Pic 9(04).99
                   Source QPRDRGrossPay.
               10  Column 60   Pic 9(03).99
                   Source QPRDRFICA.
               10  Column 70   Pic 9(03).99
                   Source QPRDRFedWH.
               10  Column 80   Pic 9(03).99
                   Source QPRDRMiscDed.
               10  Column 90   Pic 9(04).99
                   Source QPRDRNetPay.

       01  EmpFtr
           Type Control Footing QPRDREmpKey.
           05  Line Plus 1.
               10  Column 06   Pic ZZZ9
                   Source QPRDREmpNum.
               10  Column 17   Pic X(20)
                   Source QPRDREmpName.
               10  Column 38   Pic $$,$$9.99
                   Sum QPRDRGrossPay.
               10  Column 53   Pic $$$9.99
                   Sum QPRDRFICA.
               10  Column 66   Pic $$$9.99
                   Sum QPRDRFedWH.
               10  Column 79   Pic $$$9.99
                   Sum QPRDRMiscDed.
               10  Column 92   Pic $$,$$9.99
                   Sum QPRDRNetPay.

       01  DeptFtr
           Type Control Footing QPRDRDeptNum
           Next Group Plus 2.
           05  Line Plus 2.
               10  Column 14   Pic X(20) Value
                   'Department Totals'.
               10  DeptFtrGrossPay
                   Column 38   Pic $$,$$9.99
                   Sum QPRDRGrossPay.
               10  Column 48   Pic X(01) Value '*'.
               10  DeptFtrFICA
                   Column 53   Pic $$$9.99
                   Sum QPRDRFICA.
               10  Column 61   Pic X(01) Value '*'.
               10  DeptFtrFedWH
                   Column 66   Pic $$$9.99
                   Sum QPRDRFedWH.
               10  Column 74   Pic X(01) Value '*'.
               10  DeptFtrMiscDed
                   Column 79   Pic $$$9.99
                   Sum QPRDRMiscDed.
               10  Column 87   Pic X(01) Value '*'.
               10  DeptFtrNet
                   Column 92   Pic $$,$$9.99
                   Sum QPRDRNetPay.
               10  Column 102  Pic X(01) Value '*'.

       01  CmpnyFtr
           Type Control Footing Final.
           05  Line Plus 2.
               10  Column 14       Pic X(20) Value 'Company Totals'.
               10  CmpnyFtrGrossPay
                   Column 37       Pic $$$,$$9.99
                   Sum QPRDRGrossPay.
               10  Column 48       Pic X(02) Value '**'.
               10  CmpnyFtrFICA
                   Column 51       Pic $$,$$9.99
                   Sum QPRDRFICA.
               10  Column 61       Pic X(02) Value '**'.
               10  CmpnyFtrFedWH
                   Column 64       Pic $$,$$9.99
                   Sum QPRDRFedWH.
               10  Column 74       Pic X(02) Value '**'.
               10  CmpnyFtrMiscDed
                   Column 77       Pic $$,$$9.99
                   Sum QPRDRMiscDed.
               10  Column 87       Pic X(02) Value '**'.
               10  CmpnyFtrNetPay
                   Column 91       Pic $$$,$$9.99
                   Sum QPRDRNetPay.
               10  Column 102      Pic X(02) Value '**'.

       01  RptFtr
           Type Report Footing.
           05  Line 1.
               10  Column 01   Pic X(05) Value 'Date:'.
               10  Column 07   Pic X(10)
                   Source RunDateMMDDYYYYFmt.
               10  Column 39   Pic X(13) Value 'C E N T U R Y'.
               10  Column 55   Pic X(13) Value 'M E D I C A L'.
               10  Column 71   Pic X(11) Value 'C E N T E R'.
               10  Column 110  Pic X(05) Value 'Page:'.
               10  Column 116  Pic ZZzz9 Source Page-Counter.
           05  Line 2.
               10  Column 35   Pic X(17) Value 'Q U A R T E R L Y'.
               10  Column 55   Pic X(13) Value 'P A Y R O L L'.
               10  Column 71   Pic X(15) Value 'R E G I S T E R'.
           05  Line 4.
               10  Column 40   Pic X(05) Value 'Gross'.
               10  Column 58   Pic X(04) Value 'FICA'.
               10  Column 74   Pic X(07) Value 'Fed W/H'.
               10  Column 92   Pic X(05) Value 'Misc.'.
               10  Column 111  Pic X(03) Value 'Net'.
           05  Line 5.
               10  Column 41   Pic X(03) Value 'Pay'.
               10  Column 59   Pic X(03) Value 'Tax'.
               10  Column 76   Pic X(03) Value 'Tax'.
               10  Column 91   Pic X(07) Value 'Deduct.'.
               10  Column 111  Pic X(03) Value 'Pay'.

           05  Line Plus 2.
               10  Column 05   Pic X(29) Value
                   '* * * Department Totals * * *'.
           05  Line Plus 2.
               10  Column 05   Pic 9(02)
                   Source DeptTotsTblDeptNum(1).
               10  Column 08   Pic X(15)
                   Source DeptTotsTblDeptName(1).
               10  Column 38   Pic $$,$$9.99
                   Source DeptTotsTblGrossPay(1).
               10  Column 48   Pic ZZ9
                   Source DeptPcntsTblPcntg(1, 1).
               10  Column 51   Pic X(01) Value '%'.
               10  Column 57   Pic $$$9.99
                   Source DeptTotsTblFICA(1).
               10  Column 65   Pic ZZ9
                   Source DeptPcntsTblPcntg(1, 2).
               10  Column 68   Pic X(01) Value '%'.
               10  Column 74   Pic $$$9.99
                   Source DeptTotsTblFedWH(1).
               10  Column 82   Pic ZZ9
                   Source DeptPcntsTblPcntg(1, 3).
               10  Column 85   Pic X(01) Value '%'.
               10  Column 91   Pic $$$9.99
                   Source DeptTotsTblMiscDed(1).
               10  Column 99   Pic ZZ9
                   Source DeptPcntsTblPcntg(1, 4).
               10  Column 102  Pic X(01) Value '%'.
               10  Column 108  Pic $$,$$9.99
                   Source DeptTotsTblNetPay(1).
               10  Column 118  Pic ZZ9
                   Source DeptPcntsTblPcntg(1, 5).
               10  Column 121  Pic X(01) Value '%'.
           05  Line Plus 2.
               10  Column 05   Pic 9(02)
                   Source DeptTotsTblDeptNum(2).
               10  Column 08   Pic X(15)
                   Source DeptTotsTblDeptName(2).
               10  Column 38   Pic $$,$$9.99
                   Source DeptTotsTblGrossPay(2).
               10  Column 48   Pic ZZ9
                   Source DeptPcntsTblPcntg(2, 1).
               10  Column 51   Pic X(01) Value '%'.
               10  Column 57   Pic $$$9.99
                   Source DeptTotsTblFICA(2).
               10  Column 65   Pic ZZ9
                   Source DeptPcntsTblPcntg(2, 2).
               10  Column 68   Pic X(01) Value '%'.
               10  Column 74   Pic $$$9.99
                   Source DeptTotsTblFedWH(2).
               10  Column 82   Pic ZZ9
                   Source DeptPcntsTblPcntg(2, 3).
               10  Column 85   Pic X(01) Value '%'.
               10  Column 91   Pic $$$9.99
                   Source DeptTotsTblMiscDed(2).
               10  Column 99   Pic ZZ9
                   Source DeptPcntsTblPcntg(2, 4).
               10  Column 102  Pic X(01) Value '%'.
               10  Column 108  Pic $$,$$9.99
                   Source DeptTotsTblNetPay(2).
               10  Column 118  Pic ZZ9
                   Source DeptPcntsTblPcntg(2, 5).
               10  Column 121  Pic X(01) Value '%'.
           05  Line Plus 2.
               10  Column 05   Pic 9(02)
                   Source DeptTotsTblDeptNum(3).
               10  Column 08   Pic X(15)
                   Source DeptTotsTblDeptName(3).
               10  Column 38   Pic $$,$$9.99
                   Source DeptTotsTblGrossPay(3).
               10  Column 48   Pic ZZ9
                   Source DeptPcntsTblPcntg(3, 1).
               10  Column 51   Pic X(01) Value '%'.
               10  Column 57   Pic $$$9.99
                   Source DeptTotsTblFICA(3).
               10  Column 65   Pic ZZ9
                   Source DeptPcntsTblPcntg(3, 2).
               10  Column 68   Pic X(01) Value '%'.
               10  Column 74   Pic $$$9.99
                   Source DeptTotsTblFedWH(3).
               10  Column 82   Pic ZZ9
                   Source DeptPcntsTblPcntg(3, 3).
               10  Column 85   Pic X(01) Value '%'.
               10  Column 91   Pic $$$9.99
                   Source DeptTotsTblMiscDed(3).
               10  Column 99   Pic ZZ9
                   Source DeptPcntsTblPcntg(3, 4).
               10  Column 102  Pic X(01) Value '%'.
               10  Column 108  Pic $$,$$9.99
                   Source DeptTotsTblNetPay(3).
               10  Column 118  Pic ZZ9
                   Source DeptPcntsTblPcntg(3, 5).
               10  Column 121  Pic X(01) Value '%'.
           05  Line Plus 2.
               10  Column 05   Pic 9(02)
                   Source DeptTotsTblDeptNum(4).
               10  Column 08   Pic X(15)
                   Source DeptTotsTblDeptName(4).
               10  Column 38   Pic $$,$$9.99
                   Source DeptTotsTblGrossPay(4).
               10  Column 48   Pic ZZ9
                   Source DeptPcntsTblPcntg(4, 1).
               10  Column 51   Pic X(01) Value '%'.
               10  Column 57   Pic $$$9.99
                   Source DeptTotsTblFICA(4).
               10  Column 65   Pic ZZ9
                   Source DeptPcntsTblPcntg(4, 2).
               10  Column 68   Pic X(01) Value '%'.
               10  Column 74   Pic $$$9.99
                   Source DeptTotsTblFedWH(4).
               10  Column 82   Pic ZZ9
                   Source DeptPcntsTblPcntg(4, 3).
               10  Column 85   Pic X(01) Value '%'.
               10  Column 91   Pic $$$9.99
                   Source DeptTotsTblMiscDed(4).
               10  Column 99   Pic ZZ9
                   Source DeptPcntsTblPcntg(4, 4).
               10  Column 102  Pic X(01) Value '%'.
               10  Column 108  Pic $$,$$9.99
                   Source DeptTotsTblNetPay(4).
               10  Column 118  Pic ZZ9
                   Source DeptPcntsTblPcntg(4, 5).
               10  Column 121  Pic X(01) Value '%'.
           05  Line Plus 2.
               10  Column 05   Pic 9(02)
                   Source DeptTotsTblDeptNum(5).
               10  Column 08   Pic X(15)
                   Source DeptTotsTblDeptName(5).
               10  Column 38   Pic $$,$$9.99
                   Source DeptTotsTblGrossPay(5).
               10  Column 48   Pic ZZ9
                   Source DeptPcntsTblPcntg(5, 1).
               10  Column 51   Pic X(01) Value '%'.
               10  Column 57   Pic $$$9.99
                   Source DeptTotsTblFICA(5).
               10  Column 65   Pic ZZ9
                   Source DeptPcntsTblPcntg(5, 2).
               10  Column 68   Pic X(01) Value '%'.
               10  Column 74   Pic $$$9.99
                   Source DeptTotsTblFedWH(5).
               10  Column 82   Pic ZZ9
                   Source DeptPcntsTblPcntg(5, 3).
               10  Column 85   Pic X(01) Value '%'.
               10  Column 91   Pic $$$9.99
                   Source DeptTotsTblMiscDed(5).
               10  Column 99   Pic ZZ9
                   Source DeptPcntsTblPcntg(5, 4).
               10  Column 102  Pic X(01) Value '%'.
               10  Column 108  Pic $$,$$9.99
                   Source DeptTotsTblNetPay(5).
               10  Column 118  Pic ZZ9
                   Source DeptPcntsTblPcntg(5, 5).
               10  Column 121  Pic X(01) Value '%'.
           05  Line Plus 2.
               10  Column 05   Pic 9(02)
                   Source DeptTotsTblDeptNum(6).
               10  Column 08   Pic X(15)
                   Source DeptTotsTblDeptName(6).
               10  Column 38   Pic $$,$$9.99
                   Source DeptTotsTblGrossPay(6).
               10  Column 48   Pic ZZ9
                   Source DeptPcntsTblPcntg(6, 1).
               10  Column 51   Pic X(01) Value '%'.
               10  Column 57   Pic $$$9.99
                   Source DeptTotsTblFICA(6).
               10  Column 65   Pic ZZ9
                   Source DeptPcntsTblPcntg(6, 2).
               10  Column 68   Pic X(01) Value '%'.
               10  Column 74   Pic $$$9.99
                   Source DeptTotsTblFedWH(6).
               10  Column 82   Pic ZZ9
                   Source DeptPcntsTblPcntg(6, 3).
               10  Column 85   Pic X(01) Value '%'.
               10  Column 91   Pic $$$9.99
                   Source DeptTotsTblMiscDed(6).
               10  Column 99   Pic ZZ9
                   Source DeptPcntsTblPcntg(6, 4).
               10  Column 102  Pic X(01) Value '%'.
               10  Column 108  Pic $$,$$9.99
                   Source DeptTotsTblNetPay(6).
               10  Column 118  Pic ZZ9
                   Source DeptPcntsTblPcntg(6, 5).
               10  Column 121  Pic X(01) Value '%'.

           05  Line Plus 2.
               10  Column 05   Pic X(26)      Value
                   '* * * Company Totals * * *'.
           05  Line Plus 1.
               10  Column 37   Pic $$$,$$9.99
                   Source CmpnyFtrGrossPay.
               10  Column 48   Pic X(5) Value '100%'.
               10  Column 55   Pic $$,$$9.99
                   Source CmpnyFtrFICA.
               10  Column 65   Pic X(5) Value '100%'.
               10  Column 72   Pic $$,$$9.99
                   Source CmpnyFtrFedWH.
               10  Column 82   Pic X(5) Value '100%'.
               10  Column 89   Pic $$,$$9.99
                   Source CmpnyFtrMiscDed.
               10  Column 99   Pic X(5) Value '100%'.
               10  Column 107  Pic $$$,$$9.99
                   Source CmpnyFtrNetPay.
               10  Column 118  Pic X(5) Value '100%'.

       Procedure Division.

       Declaratives.

       DeptHdrUse Section.
           Use Before Reporting DeptHdr.
       DeptHdrProc.
           Set DeptTotsTblIndex        To +1.
           Search DeptTotsTblRec
               When DeptTotsTblDeptNum(DeptTotsTblIndex) =
                    QPRDRDeptNum
                   Move 0 To DeptTotsTblGrossPay(DeptTotsTblIndex),
                             DeptTotsTblFICA(DeptTotsTblIndex),
                             DeptTotsTblFedWH(DeptTotsTblIndex),
                             DeptTotsTblMiscDed(DeptTotsTblIndex),
                             DeptTotsTblNetPay(DeptTotsTblIndex).
       DeptHdrUseExit.
           Exit.

       DeptFtrUse Section.
           Use Before Reporting DeptFtr.
       DeptFtrProc.
           Move DeptFtrGrossPay    To
               DeptTotsTblGrossPay(DeptTotsTblIndex).
           Move DeptFtrFICA        To
               DeptTotsTblFICA(DeptTotsTblIndex).
           Move DeptFtrFedWH       To
               DeptTotsTblFedWH(DeptTotsTblIndex).
           Move DeptFtrMiscDed     To
               DeptTotsTblMiscDed(DeptTotsTblIndex).
           Move DeptFtrNet         To
               DeptTotsTblNetPay(DeptTotsTblIndex).
       DeptFtrUseExit.
           Exit.

       CmpnyFtrUse Section.
           Use Before Reporting CmpnyFtr.
       CmpnyFtrProc.
           Perform CmpnyFtrCalc
               Varying DeptPcntsTblDeptIndex From +1 By +1
               Until DeptPcntsTblDeptIndex > +6.
           Go To CmpnyFtrUseExit.

       CmpnyFtrCalc.
           Set DeptTotsTblIndex        To DeptPcntsTblDeptIndex.
           Set DeptPcntsTblPcntgIndex  To 1.
           Compute
               DeptPcntsTblPcntg(DeptPcntsTblDeptIndex,
                                 DeptPcntsTblPcntgIndex) Rounded =
               ((DeptTotsTblGrossPay(DeptTotsTblIndex) /
                 CmpnyFtrGrossPay) * 100) + .5.
           Set DeptPcntsTblPcntgIndex  To 2.
           Compute
               DeptPcntsTblPcntg(DeptPcntsTblDeptIndex,
                                 DeptPcntsTblPcntgIndex) Rounded =
               ((DeptTotsTblFICA(DeptTotsTblIndex) /
                 CmpnyFtrFICA) * 100) + .5.
           Set DeptPcntsTblPcntgIndex  To 3.
           Compute
               DeptPcntsTblPcntg(DeptPcntsTblDeptIndex,
                                 DeptPcntsTblPcntgIndex) Rounded =
               ((DeptTotsTblFedWH(DeptTotsTblIndex) /
                 CmpnyFtrFedWH) * 100) + .5.
           Set DeptPcntsTblPcntgIndex  To 4.
           Compute
               DeptPcntsTblPcntg(DeptPcntsTblDeptIndex,
                                 DeptPcntsTblPcntgIndex) Rounded =
               ((DeptTotsTblMiscDed(DeptTotsTblIndex) /
                 CmpnyFtrMiscDed) * 100) + .5.
           Set DeptPcntsTblPcntgIndex  To 5.
           Compute
               DeptPcntsTblPcntg(DeptPcntsTblDeptIndex,
                                 DeptPcntsTblPcntgIndex) Rounded =
               ((DeptTotsTblNetPay(DeptTotsTblIndex) /
                 CmpnyFtrNetPay) * 100) + .5.
       CmpnyFtrUseExit.
           Exit.

       End Declaratives.

       0000-Mainline.
           Perform 0100-Initialization.
           Perform 1000-ProcessPayrlData
               Until EOF.
           Perform 9999-Termination.

       0100-Initialization.
           Display '*** Program QtrlyPyrlReg '
                   'Has Started ***'.
           Move 20210510 to RunDateYYYYMMDD
           Move RunDateYYYYMMDD(5:2)   To RunDateMMDDYYYY(1:2).
           Move RunDateYYYYMMDD(7:2)   To RunDateMMDDYYYY(3:2).
           Move RunDateYYYYMMDD(1:4)   To RunDateMMDDYYYY(5:4).
           Move RunDateMMDDYYYY        To RunDateMMDDYYYYFmt.
           Open Input QtrlyPyrlRegData,
                Output QtrlyPyrlReg.
           Initiate QtrlyPyrlRegRpt.
           Read QtrlyPyrlRegData
               At End
                   Move 'Y'            To EOFSwitch.

       1000-ProcessPayrlData.
           Generate QtrlyPyrlRegRpt.
           Read QtrlyPyrlRegData
               At End
                   Move 'Y'            To EOFSwitch.

       9999-Termination.
           Terminate QtrlyPyrlRegRpt.
           Close QtrlyPyrlRegData,
                 QtrlyPyrlReg.
           Display '*** Program QtrlyPyrlReg '
                   'Has Finished ***'.
           Stop Run.
