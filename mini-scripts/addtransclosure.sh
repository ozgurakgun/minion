#!/bin/bash

#input file in first parameter, transformed file to standard output

#warning, only going to finish running if # variables is small

v_raw=`cat $1 | sed -n 's/\(DISCRETE\|BOUND\|BOOL\)\W\+\([[:alnum:]]\+\).*/\2/p' | tr '\n' ' '`;
v=($v_raw); #array of variables
v_s=${#v[*]}; #number of variables
v_last=$(($v_s-1)); #last occupied array position

eof_pos=`grep -n "**EOF**" $1 | cut -f1 -d:`;
head -n $(($eof_pos-1)) $1;

#output transitive closure constraints, i.e, for each choice of 3 variables x, y and z
#print x=y AND y=z IMPLIES x=z, x=z AND z=y IMPLIES x=y, y=x AND x=z IMPLIES y=z
#these are equivalent to y=x AND x!=z IMPLIES y!=z and so on
echo "**CONSTRAINTS**";
for x in `seq 0 $v_last`; do
    for y in `seq $((x+1)) $v_last`; do
	for z in `seq $(($y+1)) $v_last`; do
	    echo "watched-or({diseq(${v[x]},${v[y]}),diseq(${v[y]},${v[z]}),eq(${v[x]},${v[z]})})";
	    echo "watched-or({diseq(${v[x]},${v[z]}),diseq(${v[z]},${v[y]}),eq(${v[x]},${v[y]})})";
	    echo "watched-or({diseq(${v[y]},${v[x]}),diseq(${v[x]},${v[z]}),eq(${v[y]},${v[z]})})";
	done;
    done;
done;
#NB. 3*C(n,3) constraints are printed = n(n-1)(n-2)/2

echo "**EOF**";