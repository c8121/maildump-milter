#!/bin/bash

BASE=$(realpath $(dirname $0))

sourceDir=$BASE/src/test
binDir=$BASE/bin/test

if [[ ! -d "$sourceDir" ]] ; then
	echo "Source directory not found: $sourceDir"
	exit
fi

if [[ ! -d "$binDir" ]] ; then
	echo "Create $binDir"
	mkdir -p "$binDir"
fi

cd $BASE
gcc -g -o "$binDir/base64" "$sourceDir/base64.c"


