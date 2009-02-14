#!/bin/bash

echo MINION 3 > $2
echo '**VARIABLES**' >> $2
echo 'BOOL B[1000]' >> $2
echo '**CONSTRAINTS**' >> $2
perl -pe 's/^[ne] ([0-9]*) ([0-9]*)/diseq\(B[$1],B[$2]\)/'  $1 | grep diseq >> $2
echo '**EOF**' >> $2