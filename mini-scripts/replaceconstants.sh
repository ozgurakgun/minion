#!/bin/bash

#This script is intended to replace some constants by discrete variables with
#the same value. The reason why all constants are not replaced is that some
#cannot be replaced by variables, e.g. the offset in an ineq constraint.

#Only selected constants are replaced, more rules can easily be added below to
#make the script more complete.

#The first parameter is the file to be processed. The second parameter should be
#the output file. They can be the same file.

cat $1 | while read line; do
    #if a constant is found on a line, output the line in a special format to be specially processed
    #format is "~constant~conwithconstantreplaced"
    res=`echo $line | sed 's/\(.*\)eq(\([^,]*\),\s*\([0-9]\+\))\(.*\)/~\3~\1eq(\2,constant\3)\4/' \
       | sed 's/ineq(\([0-9]\+\),\(.*\)/~\1~ineq(constant\1,\2/'`;
    if [ "${res:0:1}" == "~" ]; then
	constant=`echo $res | cut -f2 -d'~' | sed 's/0*\([0-9]\+\)/\1/'`;
	if [ "${used[constant]}" == "" ]; then
	    echo "**VARIABLES**";
	    echo "DISCRETE constant$constant{$constant..$constant}";
	    echo "**CONSTRAINTS**";
	    used[constant]=1;
	fi;
	echo $res | cut -f3 -d'~';
    else
	echo $res;
    fi;
done;