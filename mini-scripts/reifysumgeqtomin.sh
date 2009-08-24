#!/bin/bash

#convert reify(sumgeq([v1,...,vn],n),r) to min([v1,vn],r)

sed 's/reify(sumgeq(\[\([^]]*\)], \([0-9]*\)), \([^)]*\))/~\1~\2~\3/' $1 | while read line; do
    if [ "${line:0:1}" != "~" ]; then
	echo $line;
    else
	count=$(echo $line | cut -d'~' -f3);
	vars=$(echo $line | cut -d'~' -f2);
	vars_count=$(($(echo $vars | tr -cd '\,' | wc -m)+1));
	rar_var=$(echo $line | cut -d'~' -f4);
	if [ $count -eq $vars_count ]; then
	    echo "min([$vars],$rar_var)";
	else
	    echo $line;
	fi;
    fi;
done;
