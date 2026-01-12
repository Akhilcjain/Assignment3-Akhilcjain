#!/bin/sh

#DIR_PATH=$(dirname "$1")

mkdir -p "$(dirname "$1")"

if [ $# != 2 ]; then
	echo "Argument missing"
	exit 1
else
	echo $2 > $1
fi

