#!/bin/bash

echo \# Autogenerated by make_depend.sh > Makefile.dep

for file in `cd minion && find .`
do
  # Remove the initial ./ from the filename
  file=${file##*./}
  # Get the extension
  ext=${file##*.}
  if [[ "$ext" == "cpp" ]]; then
    # This nasty line just changes the extension...
    BINFILE=${file%.cpp}.o
  #  BINFILE=${BINFILE#./}
    g++ -MM -MT \$\(OBJDIR\)/${BINFILE} minion/$file > /dev/null
    if [ "$?" -eq "0" ]; then
      # Have to run this twice, as storing the result in a variable
      # Loses line endings.
      g++ -MM -MT \$\(OBJDIR\)/${BINFILE} minion/$file >> Makefile.dep
    fi
  fi
done