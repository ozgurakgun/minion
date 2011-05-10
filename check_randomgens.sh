#!/bin/bash
gencount=0
rancount=0
gentot=0
rantot=0
for i in `seq 1000`; do
./generateGraph.sh minion $1 RandomGens $i
cons=`grep lexleq $1.RandomGens.${i}x | wc | awk '{print $1'}`
./generateGraph.sh minion $1 RandomElem $(($i+1)) $cons

gennodes=`minion -findallsols -noprintsols $1.RandomGens.${i}x | grep 'Total Nodes' | awk '{print $3}'`
rannodes=`minion -findallsols -noprintsols $1.RandomElem.$(($i+1))x$cons | grep 'Total Nodes' | awk '{print $3}'`

gentot=$(($gentot + $gennodes))
rantot=$(($rantot + $rannodes))
if [ $gennodes -lt $rannodes ]; then
	gencount=$((gencount + 1))
fi

if [ $rannodes -lt $gennodes ]; then
	rancount=$((rancount + 1))
fi
echo $cons ' ' $gennodes ' ' $rannodes
done

# echo $gencount ' ' $rancount ' ' $gentot ' ' $rantot

