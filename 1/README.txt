Description:
In this project, we need to implement a linker in Operating System.

How to run:
1. in this directory use Makefile to make executable file: sched
$ make

2. use sched to produce output into /out directory
./runit.sh out ./linker

3. compare our output with the reference output
./gradeit.sh refout out

Review:

RES 30 of 34:  1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 1 1 1 1 1 1 1 1 0 1 1 1 1 0

LABRESULTS = 40 + 52.9380 - DEDUCTION = 92.9380
 1. Fail to detect "Error: Relative address exceeds module size; zero used"

 2. "Warning: Module 3: X31 too big 10 (max=0) assume zero relative". The max is module size.

 3. When a module is not empty, the code count is needed.
