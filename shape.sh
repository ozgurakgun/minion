#!/bin/bash

echo 'MINION 3'
echo '**VARIABLES**'

echo 'DISCRETE m['$1','$2'] {1..'$3'}'
echo '**CONSTRAINTS**'

COUNT=0

while [ $COUNT -lt $1 ]; do
    echo 'sumgeq(m['$COUNT',_], 0)'
    let COUNT=COUNT+1
done

COUNT=0

while [ $COUNT -lt $2 ]; do
    echo 'sumgeq(m[_,'$COUNT'], -1)'
    let COUNT=COUNT+1
done

echo '**EOF**'
