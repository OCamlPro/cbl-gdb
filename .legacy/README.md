This is the island of misfit software.

The ELF and DWARF knowledge in OBMOD, for example, was too hard-won for us to
just cast it aside.  But it's no longer needed in cbl-gdb.

So, it lives here.

SELFCHECK and PICGEN, in contrast, are actually a useful regression testing 
programs; they compare the way cprint.py displays and enters variables to the 
way that gdb and cobc do.  But they broke when I eliminated OBMOD, and they 
needs to be repaired.