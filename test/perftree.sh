#!/bin/sh

LOCATION=$(realpath $(dirname $0))

if [ $# -lt 3 ] ; then
	"$LOCATION/../build/chessh-client" -a -t `expr $1 + 1` -i "$2"
else
	"$LOCATION/../build/chessh-client" -a -t `expr $1 + 1` -i "$2" -s "$3"
fi
