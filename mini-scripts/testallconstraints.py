#!/usr/bin/python
# Generate two minion input files, run them then compare dumptree outputs to 
# detect bugs in constraint propagators.

import sys, os, getopt
from constraint_test_common import *
import random
from sendemail import *

(optargs, other)=getopt.gnu_getopt(sys.argv, "", ["minion=", "numtests="])

if len(other)>1:
    print "Usage: testallconstraints.py [--minion=<location of minion binary>] [--numtests=...]"
    sys.exit(1)

conslist=["pow", "alldiff", "reifyalldiff",  "reifyimplyalldiff"]
# This one tests all the constraints in the following list.

numtests=50
minionbin="bin/minion"
for i in optargs:
    (a1, a2)=i
    if a1=="--minion":
        minionbin=a2
    elif a1=="--numtests":
        numtests=int(a2)

for consname1 in conslist:
    random.seed(12345)   # stupid seed but at least it makes the test repeatable.
    
    reify=False
    reifyimply=False
    consname=consname1
    if consname[0:10]=="reifyimply":
        reifyimply=True
        consname=consname[10:]
    
    if consname[0:5]=="reify":
        reify=True
        consname=consname[5:]
    
    testobj=eval("test"+consname+"()")
    testobj.solver=minionbin
    
    for testnum in range(numtests):
        print "Test number %d"%(testnum)
        if not testobj.runtest(reify=reify, reifyimply=reifyimply):
            mail("Mail from testallconstraints.py.\nProblem with constraint %s. Run testconstraint.py %s on current SVN to replicate the test."%(consname1, consname1))
            break


