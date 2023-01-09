## CBL-GDB regression testing

The command `make known-good` loops through the various subdirectories and creates, in each directory, a `test.known-good` file that becomes part of the repository.

The command `make test` loops through the various subdirectories and creates, in each directory, a `test.under-test` file, which, we fondly hope, is functionally the same as the `test.known-good` file.

Different versions of GDB on different systems produce, as a matter of course, somewhat different output.  Two runs on the same system will produce different output as well; when a run completes, GDB announces things like `[Inferior 1 (process 32542) exited normally]`. So, the Python program `strip-diff` applies a bunch of heuristics to remove elements that are expected to be different.

The results of the `strip-diff` program are the files `known-good` and `under-test`.  These are compared to see if there are actual significant differences between the known-good results and the current under-test results.

