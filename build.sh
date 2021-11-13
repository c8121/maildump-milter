#!/bin/bash

BASE=$(realpath $(dirname $0))

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

gcc -o "$binDir/maildump" "$sourceDir/maildump.c" $REQUIRE -pthread



if [[ ! -d "$BASE/lib/sntools" ]] ; then
	echo "Pull sntools"
	if [[ ! -d "$BASE/lib" ]] ; then
		mkdir "$BASE/lib"
	fi
	cd "$BASE/lib"
	git clone https://github.com/c8121/sntools.git
	cd "$BASE"
fi


echo "Build mailforwarder"
gcc -o "$binDir/mailforward" "$sourceDir/mailforward.c" 


echo "Build mailparser"
gcc -o "$binDir/mailparser" "$sourceDir/mailparser.c"

echo "Build archive"
gcc -o "$binDir/archive" "$sourceDir/archive.c"
gcc -o "$binDir/mailarchiver" "$sourceDir/mailarchiver.c"
