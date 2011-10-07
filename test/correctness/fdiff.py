#!/usr/bin/python

import sys
import os

filename0 = sys.argv[1]
filename1 = sys.argv[2]

file0 = open(filename0, "r")
file1 = open(filename1, "r")

#e = 0.00005
e = 0.05
#e=0
line = 0

errs = 0

while file0:
    line+=1
    line0 = file0.readline().rstrip()
    line1 = file1.readline().rstrip()
    
    if (line0 == line1 == ""):
        exit(0)

    if (line0 != line1):
        try:
            f0 = float(line0)
            f1 = float(line1)
            if (abs(f0-f1) > e):
                throw
        except:
            print "line", line, ":", line0, "!=", line1
            errs = errs + 1
            if (errs > 10):
                exit(1)

exit(0)
