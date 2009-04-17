#!/bin/bash

#Given a reference solver (1st param) and a learning solver (2nd param), read
#out constraints from the output of the learning solver and check that each one
#is a correct implied constraint. The instance is given as the 3rd parameter.

#An optional fourth parameter specifies the constraint number to start at,
#this would be useful to skip those already tested.

#The constraints printed out should be a negative of the learned constraint, in
#minion input format. The lines should be ADDEDNEG=X where X is the negative of
#the learned constraint.

TIMEOUT=100; #timeout in seconds
REFSOLVER=$1;
LEARNSOLVER=$2;
INSTANCE=$3;
TMPFILE=`mktemp`;
LEARNOUTPUT=`mktemp`;
LEARNCONS=`mktemp`;
INSTRUN=`mktemp`;
if [ $# -eq 4 ]; then #work out where to start
    FIRST=$4
else
    FIRST=1
fi

echo "Sanity checking the instance:";
if $REFSOLVER $INSTANCE | grep "Problem solvable?: no" > /dev/null; then
    echo "This instance has no solutions, so it's not suitable for testing.";
    exit 1;
else
    echo "The instance has at least one solution according to the reference solver so proceeding to test.";
fi
echo;

eofline=`grep -n "**EOF**" $INSTANCE | cut -f1 -d:`;

echo "Running the instance using the learning solver";
$LEARNSOLVER $INSTANCE > $LEARNOUTPUT;
cat $LEARNOUTPUT | grep "ADDEDNEG=" > $LEARNCONS;
numlearned=`wc -l $LEARNCONS | cut -f1 -d' '`;
if cat $LEARNOUTPUT | grep "Solve Time" > /dev/null; then
    echo "Completed successfully";
else
    echo "Did not finish search, testing anyway.";
fi
echo "Found $numlearned constraints to test.";
echo; 

cat $LEARNCONS | while read line; do
    count=$(($count+1));
    if [ $count -ge $FIRST ]; then
	echo "Progress: $count of $numlearned";
	con=`echo $line | cut -c10-`; #get the con alone
	head -n $(($eofline-1)) $INSTANCE > $TMPFILE;
	echo $con >> $TMPFILE;
	echo "**EOF**" >> $TMPFILE;
	$REFSOLVER -timelimit $TIMEOUT $TMPFILE > $INSTRUN;
	if cat $INSTRUN | grep "Problem solvable?: yes" > /dev/null; then
	    echo "WARNING! The negative of the following constraint is not implied";
	    echo;
	    echo $con;
	    echo;
	elif cat $INSTRUN | grep "Time out." > /dev/null; then
	    echo "Unknown - timed out at $TIMEOUT seconds";
	else
	    echo "Success!";
	fi
    fi
done
    
