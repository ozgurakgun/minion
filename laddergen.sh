#!/bin/bash
echo MINION 3;
echo;
echo **VARIABLES**;
echo DISCRETE x[$1] {1..$1};
echo DISCRETE y[$1] {1..$1};
echo BOOL aux[$(($1-1))];
echo;
echo **CONSTRAINTS**;
echo "eq(x[0],y[0])";
for i in `seq 0 $(($1-2))`; do
    echo "reify(eq(x[$i],y[$i]),aux[$i])";
    echo "reify(eq(x[$(($i+1))],y[$(($i+1))]),aux[$i])";
done;
echo "diseq(x[$(($1-1))],y[$(($1-1))])";
echo **EOF**;
