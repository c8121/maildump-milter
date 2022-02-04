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

gcc -Wall -o "$binDir/maildump" "$sourceDir/maildump.c" $REQUIRE -pthread -lbsd


echo "Build mailforwarder"
gcc -Wall -o "$binDir/mailforward" "$sourceDir/mailforward.c" -lmailutils


echo "Build mailparser & mailassembler"
gcc -Wall -o "$binDir/mailparser" "$sourceDir/mailparser.c" -lmailutils
gcc -Wall -o "$binDir/mailassembler" "$sourceDir/mailassembler.c" -lmailutils

echo "Build mailindexer"
gcc -Wall -o "$binDir/mailindexer-solr" "$sourceDir/mailindexer-solr.c" -lbsd -lmailutils

echo "Build archive, archivemetadb & mailarchiver"
gcc -Wall -o "$binDir/archive" "$sourceDir/archive.c" -lbsd
gcc -g -Wall -o "$binDir/archivemetadb" "$sourceDir/archivemetadb.c" -lmysqlclient
gcc -Wall -o "$binDir/mailarchiver" "$sourceDir/mailarchiver.c" -lbsd -lmailutils

echo "cat-* utils"
gcc -Wall -o "$binDir/cat-txt" "$sourceDir/cat-txt.c"
gcc -Wall -o "$binDir/cat-dog" "$sourceDir/cat-dog.c"
gcc -Wall -o "$binDir/cat-doc" "$sourceDir/cat-doc.c"
gcc -Wall -o "$binDir/cat-pdf" "$sourceDir/cat-pdf.c"
gcc -Wall -o "$binDir/cat-html" "$sourceDir/cat-html.c"

