#!/bin/bash

#This script is intended to replace some constants by discrete variables with
#the same value. The reason why all constants are not replaced is that some
#cannot be replaced by variables, e.g. the offset in an ineq constraint.

#Only selected constants are replaced, more rules can easily be added below to
#make the script more complete.

#The first parameter is the file to be processed. The second parameter should be
#the output file. They can be the same file.

tempfile=`mktemp /tmp/tfile.XXXXXXXXXX`; #make a named pipe where the output can be put
#first, scan the file to work out what all the numbers are:
#do this by replacing non-digits with spaces and then squeezing the spaces together
nums=`cat $1 | tr --complement '[:digit:]' ' ' | tr --squeeze-repeats ' ' | sed 's/^ //' | tr ' ' '\n' | sort -n | uniq`;
#NB these are an overestimate of the constants that are needed, some may never be used
#sed 's/[\,\ \(\)]\+\([0-9]\+\)/ n\1 /gp' -n - ABORTED attempted improvement

header_pos=`grep 'MINION' $1 -n | cut -f1 -d:`;

head -n $header_pos $1; #replicate the file up to and including the header

#now output the constant variables
echo "**VARIABLES**";
for i in $nums; do
    echo "DISCRETE constant$i{$i..$i}";
done;

#now replicate from after the header except replacing certain constants by constant variables
cat $1 | tail -n +$(($header_pos+1)) \
       | sed 's/eq(\([^,]*\),\s*\([0-9]\+\))/eq(\1,constant\2)/' \
       | sed 's/ineq(\([0-9]\+\),\(.*\)/ineq(constant\1,\2/';
