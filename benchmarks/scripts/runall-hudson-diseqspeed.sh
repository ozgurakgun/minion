#!/bin/sh
#python benchmarks/scripts/benchmark-hudson.py $1 bibd test_instances/diseq_speed_test/bibdline*.minion
python benchmarks/scripts/benchmark-hudson.py $1 golomb test_instances/diseq_speed_test/golomb*.minion.bz2 test_instances/diseq_speed_test/ruler*.minion.bz2
#python benchmarks/scripts/benchmark-hudson.py $1 knight test_instances/diseq_speed_test/knight5.minion
#python benchmarks/scripts/benchmark-hudson.py $1 ladder test_instances/diseq_speed_test/ladder*.minion
#python benchmarks/scripts/benchmark-hudson.py $1 langford test_instances/diseq_speed_test/langford*.minion.bz2
#python benchmarks/scripts/benchmark-hudson.py $1 nqueens test_instances/diseq_speed_test/nqueens*.minion.bz2
#python benchmarks/scripts/benchmark-hudson.py $1 peg test_instances/diseq_speed_test/peg*.minion
#python benchmarks/scripts/benchmark-hudson.py $1 quasigroup-element test_instances/diseq_speed_test/qg-element*
#python benchmarks/scripts/benchmark-hudson.py $1 quasigroup-gacelement test_instances/diseq_speed_test/qg-gacelement*
#python benchmarks/scripts/benchmark-hudson.py $1 quasigroup-watchelement test_instances/diseq_speed_test/qg-watchelement*
#python benchmarks/scripts/benchmark-hudson.py $1 sokoban test_instances/diseq_speed_test/sokoban*.minion
#python benchmarks/scripts/benchmark-hudson.py $1 solitaire test_instances/diseq_speed_test/solitaire*
