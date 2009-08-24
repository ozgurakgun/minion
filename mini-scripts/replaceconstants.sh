#!/bin/bash

#This script is intended to replace some constants by discrete variables with
#the same value. The reason why all constants are not replaced is that some
#cannot be replaced by variables, e.g. the offset in an ineq constraint.

#Only selected constants are replaced, more rules can easily be added below to
#make the script more complete.

#Won't work on files with line breaks in the middle of constraints. reification
#may present a problem.

#The first parameter is the file to be processed.

cat $1 | while read line; do
    #if a constant is found on a line, output the line in a special format to be specially processed
    #format is "~constant~conwithconstantreplaced"
    res=`echo $line | sed 's/\(.*\)eq(\([^,]*\),\s*\([0-9]\+\))\(.*\)/~\3~\1eq(\2,constant\3)\4/' \
       | sed 's/ineq(\([0-9]\+\),\(.*\)/~\1~ineq(constant\1,\2/' \
       | sed 's/\(.*\)eq(\([0-9]\+\),\s*\(.*\))\(.*\)/~\2~\1eq(constant\2,\3)\4/'`;
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