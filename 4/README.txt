How to run:
1. in this directory use Makefile to make executable file: iosched
$ make

2. use mmu to produce output into /out directory
./runit.sh out ./iosched

3. compare our output with the reference output
./gradeit.sh refout out
