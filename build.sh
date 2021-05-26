#!/bin/bash

BASE=$(dirname $0)

sourceDir=$BASE/src
binDir=$BASE/bin

if [[ ! -d "$sourceDir" ]] ; then
	echo "Source directory not found: $sourceDir"
	exit
fi

if [[ ! -d "$binDir" ]] ; then
	echo "Create $binDir"
	mkdir -p "$binDir"
fi



echo "Build maildump"
REQUIRE="/usr/lib/x86_64-linux-gnu/libmilter.a"
for r in $REQUIRE ; do
	if [[ ! -f "$r" ]] ; then
		echo "Missing required: $r"
		exit
	fi
done

gcc -o "$binDir/maildump" "$sourceDir/maildump.cpp" $REQUIRE -lstdc++ -pthread



echo "Build mailforwarder"
gcc -o "$binDir/mailforward" "$sourceDir/mailforward.cpp" -lstdc++ 
