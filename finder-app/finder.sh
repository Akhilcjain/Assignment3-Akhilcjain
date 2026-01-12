#!/bin/sh

filecount=0
linecount=0

if [ "$#" != 2 ]; then
	echo "Error"
	exit 1
elif [ ! -d "$1" ]; then
	echo "File Missing"
	exit 1
else
	for file in $(find "$1" -type f)
	do
		#echo "File found $file"
		filecount=$((filecount+1))
	done
	for line in $(grep -r -o "$2" "$1")
	do
		linecount=$((linecount+1))
	done
	echo "The number of files are $filecount and the number of matching lines are $linecount"
fi

