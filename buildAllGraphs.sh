#!/bin/bash

echo -n $2
for i in GeneratorsBasic FullGroup StabChainGap StabComplete OrbitComplete StabChainME; do
    echo -n ' '`do_time.sh generateGraph.sh $1 $2 $i`
done

for i in StabCompleteN OrbitCompleteN; do
    echo -n ' '`do_time.sh generateGraph.sh $1 $2 $i 1`
    echo -n ' '`do_time.sh generateGraph.sh $1 $2 $i 2`
    echo -n ' '`do_time.sh generateGraph.sh $1 $2 $i 3`
done

for i in 2 3 5 10 20 30 40 50; do
    echo -n ' '`do_time.sh generateGraph.sh $1 $2 RandomElem $i`
done
echo
