#!/bin/bash

if [ "$#" = "0" ]; then
	 echo './do_test.sh <filename>'
	 exit 1
fi

echo -n $1 ' & ' > $1.ans
singlesol=`minion $1          -noprintsols -cpulimit 300 | grep "^Total Nodes\|^Solutions Found\|^Total Time" | awk '{print $3}'`
allsol=`minion $1 -findallsols -noprintsols -cpulimit 300 | grep "^Total Nodes\|^Solutions Found\|^Total Time" | awk '{print $3}'`

echo $singlesol $allsol | awk '{print $1," & ",$2," & ",$3," & ",$4," & ",$5," & ",$6}' >> $1.ans
