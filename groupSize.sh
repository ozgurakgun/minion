#!/bin/bash

if [ "$#" = "0" ]; then
	 echo './graphSize <minion> <instance>'
	 exit 1
fi


$1 -Xgraph $2 > $2.size.gapin
echo 'a := OutputTextFile("'$2.size.gapout'", false);;' >> $2.size.gapin
echo 'PrintTo(a, Size(Group(generators)));' >> $2.size.gapin
gap.sh < $2.size.gapin > /dev/null
cat $2.size.gapout
rm $2.size.gapin $2.size.gapout

