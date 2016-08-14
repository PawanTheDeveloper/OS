Documentation for Warmup Assignment 2
=====================================

+-------+
| BUILD |
+-------+
Succesfully Builds without any warnings and Errors.

Commands:
make clean
make
./warmup2 

+---------+
| GRADING |
+---------+

Basic running of the code : 100 out of 100 pts

Missing required section(s) in README file : Providing Readme File.
Cannot compile : Successfull Compilation
Compiler warnings : 0
"make clean" : Working Correctly
Segmentation faults : 0
Separate compilation : Nope
Using busy-wait : CPU and Memory usage was very less.
Handling of commandline arguments: Handles all CommandLine Arguments properly.
    1) -n : Yes
    2) -lambda : Yes
    3) -mu : Yes
    4) -r : Yes
    5) -B : Yes
    6) -P : Yes
Trace output :
    1) regular packets: Yes
    2) dropped packets: Yes
    3) removed packets: Yes
    4) token arrival (dropped or not dropped): Yes
Statistics output :
    1) inter-arrival time : Yes
    2) service time : Yes
    3) number of customers in Q1 : Yes
    4) number of customers in Q2 : Yes
    5) number of customers at a server : Yes
    6) time in system : Yes
    7) standard deviation for time in system : Yes
    8) drop probability : Yes
Output bad format : 0
Output wrong precision for statistics (should be 6-8 significant digits) : 0
Large service time test : 0
Large inter-arrival time test : 0
Tiny inter-arrival time test : 0
Tiny service time test : 0
Large total number of customers test : 0
Large total number of customers with high arrival rate test : 0
Dropped tokens test : Done
Cannot handle <Cntrl+C> at all (ignored or no statistics) : Handles <Cntrl+C>
Can handle <Cntrl+C> but statistics way off : All stats showed
Not using condition variables and do some kind of busy-wait : 
Synchronization check : Done
Deadlocks : None

+------+
| BUGS |
+------+
None
+------------------+
| OTHER (Optional) |
+------------------+
Comments on design decisions: 
1) Learned the use of Opt_long_Only method for handling command Line arguments effectively.
2) Successfully understood the simplicity of Mutex variable , use and power of Mutex.
