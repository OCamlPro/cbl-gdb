## common settings for the tests, to be sourced from other
## testscripts

# no paging, always full outpout (will be dedirected to file)
set height 0
set width 0
set pagination off

# print settings with reasonable size, may be adjusted
# in the tests
set print repeats 10
set print elements 200

# allow breakpoint setting before starting the process
# without any questions
set breakpoint pending on

# run the tests as portable as possible
set disable-randomization off


## common function defintions for testing

# output everything, full context, current line, then step
define stutter_step_single
    echo
    cprint *
    cprint ?
    cprint/x 1
    step
end

# 
define stutter_step_loop
    set variable $_exitcode=1031
    while $_exitcode==1031
        stutter_step_single
    end
end

#
# note: definition of called "printem" function is
#       too be defined externally
define printloop
    set variable $_exitcode=1031
    while $_exitcode==1031
       printem
       continue
   end
end

# names used in testscripts
alias -a stutter = stutter_step_single
alias -a stutter_step = stutter_step_loop
