Description:
In this project, we need to implement a memory management unit in Operating System.

How to run:
1. in this directory use Makefile to make executable file: mmu
$ make

2. use mmu to produce output into /out directory
./runit.sh inputs out ./mmu

3. compare our output with the reference output
./gradeit.sh refout out

Review:

input inst frames  f  r  c  e  a  w
 1       1      8  .  .  .  .  .  .
               21  .  .  .  .  .  .
               58  .  .  .  .  .  .
              121  .  .  .  .  .  .
 2       1      8  .  .  .  .  .  .
               21  .  .  .  .  .  .
               58  .  .  .  .  .  .
              121  .  .  .  .  .  .
 3       1      8  .  .  .  .  .  .
               21  .  .  .  .  .  .
               58  .  .  .  .  .  .
              121  .  .  .  .  .  .
 4       1      8  .  .  .  .  .  .
               21  .  .  .  .  .  .
               58  .  .  .  .  .  .
              121  .  .  .  .  .  .
 5       1      8  .  .  .  .  .  .
               21  .  .  .  .  .  .
               58  .  .  .  .  .  .
              121  .  .  .  .  .  .
 6       1      8  .  .  .  .  .  .
               21  .  .  .  .  .  .
               58  .  .  .  .  .  .
              121  .  .  .  .  .  .
 7      13      8  .  .  .  .  .  .
               21  .  .  .  .  .  .
               58  .  .  .  .  .  .
              121  .  .  .  .  .  .
 8      10      8  .  .  .  .  .  .
               21  .  .  .  .  .  .
               58  .  .  .  .  .  .
              121  .  .  .  .  .  .
 9      51      8  .  .  .  .  .  .
               21  .  .  .  .  .  .
               58  .  .  .  .  .  .
              121  .  .  .  .  .  .
10     243      8  .  .  .  .  .  .
               21  .  .  .  .  .  .
               58  .  .  .  .  .  .
              121  .  .  .  .  .  .
11     243      8  .  .  .  .  .  .
               21  .  .  .  .  .  .
               58  .  .  .  .  .  .
              121  .  .  .  .  .  .
12     490      8  .  .  .  .  .  .
               21  .  .  .  .  .  .
               58  .  .  .  .  .  .
              121  .  .  .  .  .  .
13    2428      8  .  #  .  .  .  .
               21  .  #  .  .  .  .
               58  .  #  .  .  .  .
              121  .  #  .  .  .  .
14    2444      8  .  #  .  .  .  .
               21  .  #  .  .  .  .
               58  .  #  .  .  .  .
              121  .  #  .  .  .  .
15   53721      8  .  #  .  .  .  .
               21  .  #  .  .  .  .
               58  .  #  .  .  .  .
              121  .  #  .  .  .  .
SUM               60 48 60 60 60 60

Total = 348 out of 360 at 60% = 58.0000
LatePoints = D1 | Other Deductions = D2
Score: 40 - D + 58.0000 = 98.0000
1. When reading random values, you write:

for (int i = 1; i < randvals[0]; i++) 

However, it should be:

for (int i = 1; i <= randvals[0]; i++) 
