#!/bin/bash

echo 'MINION 3'
echo '**VARIABLES**'
echo 'DISCRETE V[' `grep ^p $1 | awk '{print $3}'` '] {1..'$2'}'
echo '**CONSTRAINTS**'
grep ^e $1 | awk '{ print "neq(" , $2 , "," , $3, ")"}'
echo '**EOF**'
