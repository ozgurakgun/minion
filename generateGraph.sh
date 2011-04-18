#!/bin/bash

if [ "$#" = "0" ]; then
	 echo './generateGraph <minion> <instance> <method> <randomseed> <method parameter (optional)>'
	 exit 1
fi

SEED=0
PRINTSEED=
if [ "X$4" != "X" ]; then
	SEED=$4
	PRINTSEED=.$4x
fi

case $3 in
GeneratorsBasic|FullGroup|StabChainGap|StabComplete|OrbitComplete|StabChainME|StabChainReduced|RandomGens|ArityOne)
  GAPCOMMAND="$3(generators)";;

StabCompleteN|OrbitCompleteN|RandomElem)
  GAPCOMMAND="$3($5,generators)";;
*)
  echo 'Invalid symmetry braeking method'
  exit;;
esac
$1 -Xgraph $2 > $2.gapin
cat `dirname $0`/rule1.g >> $2.gapin
cat `dirname $0`/StabFiles.g >> $2.gapin
echo 'Reset(GlobalMersenneTwister, '$SEED');;' >> $2.gapin
echo 'a := OutputTextFile("'$2.gapout'", false);;' >> $2.gapin
echo 'H :='$GAPCOMMAND';;' >> $2.gapin
echo 'gen_constraints(H, varnames, a);' >> $2.gapin
echo 'quit;' >> $2.gapin
gap.sh < $2.gapin > /dev/null

grep -v "**EOF**" $2 > $2.$3$PRINTSEED$5
cat $2.gapout >> $2.$3$PRINTSEED$5
echo '**EOF**' >> $2.$3$PRINTSEED$5
# cleanup
#rm $2.gapin $2.gapout
