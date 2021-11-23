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

gcc -o "$binDir/maildump" "$sourceDir/maildump.c" $REQUIRE -pthread -lbsd


echo "Build mailforwarder"
gcc -o "$binDir/mailforward" "$sourceDir/mailforward.c" -lmailutils


echo "Build mailparser & mailassembler"
gcc -o "$binDir/mailparser" "$sourceDir/mailparser.c" -lmailutils
gcc -o "$binDir/mailassembler" "$sourceDir/mailassembler.c" -lmailutils

echo "Build mailindexer"
gcc -o "$binDir/mailindexer-solr" "$sourceDir/mailindexer-solr.c" -lmailutils

echo "Build archive, archivemetadb & mailarchiver"
gcc -o "$binDir/archive" "$sourceDir/archive.c" -lbsd
gcc -o "$binDir/archivemetadb" "$sourceDir/archivemetadb.c" -lmysqlclient
gcc -o "$binDir/mailarchiver" "$sourceDir/mailarchiver.c" -lbsd -lmailutils

echo "cat-* utils"
gcc -o "$binDir/cat-txt" "$sourceDir/cat-txt.c"
gcc -o "$binDir/cat-dog" "$sourceDir/cat-dog.c"
gcc -o "$binDir/cat-doc" "$sourceDir/cat-doc.c"
gcc -o "$binDir/cat-pdf" "$sourceDir/cat-pdf.c"
gcc -o "$binDir/cat-html" "$sourceDir/cat-html.c"

