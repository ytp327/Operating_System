Description:
In this project, we need to implement a CPU scheduler in Operating System.

How to run:
1. in this directory use Makefile to make executable file: sched
$ make

2. use sched to produce output into /out directory
./runit.sh out ./sched

3. compare our output with the reference output
./gradeit.sh refout out

Review:
in    F    L    S   R2   R5   R10  P1:6 P2   P3   P4   E2:5 E4   E8:3
00    3    3    3    3    3    3    3    3    3    3    3    3    3
01    3    3    3    3    3    3    3    3    3    3    3    3    3
02    3    3    3    3    3    3    3    3    3    3    3    3    3
03    3    3    3    3    3    3    3    3    3    3    3    3    3
04    3    3    3    3    3    3    3    3    3    3    3    3    3
05    3    3    3    3    3    3    3    3    3    3    3    3    3
06    3    3    2    3    3    3    3    3    3    3    1    0    0
07    3    3    0    3    3    3    3    3    3    3    0    0    0
08    3    3    3    3    3    3    3    3    3    3    0    0    0
09    3    3    3    3    3    3    3    3    3    3    3    3    3
10    1    1    0    1    1    1    1    1    1    1    1    1    1
11    3    3    3    3    3    3    3    3    3    3    3    3    2
12    3    3    3    3    3    3    3    3    3    3    1    0    2
13    3    3    1    3    3    3    3    3    3    3    0    0    0
14    3    3    3    3    3    3    3    3    3    3    2    2    3
SUM 43  43  36  43  43  43  43  43  43  43  29  27  29 
508 out of 585 correct

LABRESULTS = 40 + 52.0980 - DEDUCTION = 92.0980

1. Your random number parts have bugs. When you read it from file, it should be for(int a = 0; a <= randvals[0]; a++) 

When you get val, when ofs > size, it should be :

if (ofs>randvals[0]){

ofs = 1;

res = (randvals[ofs] % burst) + 1;

ofs ++;

}

2. I don't find the difference of your PRIO and PREPRIO.

3. You shouldn't use priority_queue. When two processes have the same remaining time, you don't know which one comes first. 
