#!/bin/bash

if [ "$#" = "0" ]; then
	 echo './do_test.sh <minion> <instance> <method> <method parameter (optional)>'
	 exit 1
fi

./generateGraph.sh $*
filename=$2.$3$4
echo $filename
singlesol=`minion-svn $filename           | grep "^Total Nodes\|^Solutions Found\|^Total Time" | awk '{print $3}'`
allsol=`minion-svn $filename -findallsols | grep "^Total Nodes\|^Solutions Found\|^Total Time" | awk '{print $3}'`

echo $singlesol $allsol | awk '{print $1," & ",$2," & ",$3," & ",$4," & ",$5," & ",$6}'
