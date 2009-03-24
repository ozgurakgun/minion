#!/bin/bash
if [ "$#" != "2" ]; then
	echo './DIMACS2Minion <instance> <number of colours>'
	exit 1
fi

echo 'MINION 3' > $1.minion
echo '**VARIABLES**' >> $1.minion
echo 'DISCRETE V[' `grep ^p $1 | awk '{print $3}'` '] {1..'$2'}' >> $1.minion
echo '**CONSTRAINTS**' >> $1.minion
grep ^e $1 | awk '{ print "diseq(V[" , $2 - 1 , "],V[" , $3 - 1, "])"}' >> $1.minion
echo '**EOF**' >> $1.minion
