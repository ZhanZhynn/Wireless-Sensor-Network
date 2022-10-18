Note:

a) slave = seafloor seismic sensor
b) master = base station
c) processFunc = balloon seismic sensor
d) log.txt = base station log file
e) status.txt = SENTINEL termination check

==============================================

How to run :

1) open terminal in directory

2) commands to run :
    - make all
    - make run OR mpirun -oversubscribe -np 5 main_out 2 2
    
3) Input value in status.txt to continue (1) or terminate (-1) program

