IDENTIFICATION DIVISION.
PROGRAM-ID. Error-Generator.
DATA DIVISION.
WORKING-STORAGE SECTION.
*>01 N COMP PIC 99.   *> Compiles
01 N COMP.   *>generates signal SIGSEGV

01 def-COMPUTATIONAL      COMPUTATIONAL    PICTURE S9(9)V9(7) VALUE -988776654.1223345 .
01 len-COMPUTATIONAL      CONSTANT AS LENGTH OF def-COMPUTATIONAL          .
01 len-COMPUTATIONAL      CONSTANT AS LENGTH OF def-COMPUTATIONAL       .   *> Also generates an error


*> 01 def-COMP-XX            COMPUTATIONAL-X  PICTURE X(9) VALUE       1 .
*> 
*> Causes the following field and attributes to be generated:
*> 
*> def-COMP-XX
*> field_size:              15
*> field_data_pointer:      b_58
*> field_attribute_pointer: a_25
*> attr_type:               17
*> attr_digits:             36
*> attr_scale:              0
*> attr_flags:              0x20
*> pic_pointer:
*> picture:
*> picture2:
*> pic_data: (HEX)          [ 00 00 00 ff 7f 24 30 00 00 00 00 00 00 00 01 ]
*> pic_data: (ASCII)        "....$0........"


*> In the .cbl.lst file, it would be nice if everybody had a PICTURE indicator

*> 00008 NUMERIC        01   def-COMPUTATIONAL-2            S9(17)V9(17) COMP-2
*> 00008 NUMERIC        01   def-FLOAT-DECIMAL-16
*> 00016 NUMERIC        01   def-FLOAT-DECIMAL-34
*> 00008 NUMERIC        01   def-FLOAT-LONG                 S9(17)V9(17) COMP-2
*> 00004 NUMERIC        01   def-FLOAT-SHORT                S9(7)V9(8) COMP-1
*> 00004 NUMERIC        01   def-INDEX
*> 00008 POINTER        01   def-POINTER
*> 00008 POINTER        01   def-PROCEDURE-POINTER
*> 00008 POINTER        01   def-PROGRAM-POINTER
*> 00004 NUMERIC        01   def-SIGNED-INT                 BINARY-LONG COMP-5
*> 
*> 
*> When a module has but one PROGRAM-ID, the -T listing doesn't have the
*> PROGRAM information included.
*> 
*> 
*> 
