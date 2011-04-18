#!/bin/bash

echo -n $2
for i in GeneratorsBasic FullGroup StabChainGap StabComplete OrbitComplete StabChainME StabChainReduced ArityOne; do
    echo -n ' '`do_time.sh ./generateGraph.sh $1 $2 $i 1`
done

for i in StabCompleteN OrbitCompleteN; do
    echo -n ' '`do_time.sh ./generateGraph.sh $1 $2 $i 1 1`
    echo -n ' '`do_time.sh ./generateGraph.sh $1 $2 $i 1 2`
    echo -n ' '`do_time.sh ./generateGraph.sh $1 $2 $i 1 3`
done

for seed in 1 2 3 4 5 6 7; do
	echo -n ' '`do_time.sh ./generateGraph.sh $1 $2 RandomGens $seed`
	for i in 2 5 10 20 40; do
    	echo -n ' '`do_time.sh ./generateGraph.sh $1 $2 RandomElem $seed $i`
	done
done
echo
