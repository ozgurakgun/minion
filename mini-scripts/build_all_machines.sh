#!/bin/bash
mkdir machines
rm -rf machines/*
initial_path=`pwd`
for i in lifeImmigration lifeBriansBrain life life3d labs_three labs pegsol and_constraint labs;
do
    ./build_machine.sh $i False False
    ./build_machine.sh $i False True
    ./build_machine.sh $i True False
    ./build_machine.sh $i True True
    # This next line to make sure building fails, if it isn't there.
    rm ../minion/constraints/generated_constraint_code.h
    cp machines/$i.False.False.vm_out ../minion/constraints/generated_constraint_code.h
( cd ..; rm -rf bin-$i; mkdir bin-$i; cd bin-$i; cmake ..; make minion -j2; cp minion $initial_path/machines/minion-$i )
done