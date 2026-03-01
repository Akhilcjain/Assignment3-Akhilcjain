#!/bin/sh
#Write a startup script to start daemon

if [ $# -lt 1 ]; 
then
	echo "Error"
	exit 1
fi

case $1 in
	"start")
		echo "Start command received"
		if [ -f "aesdsocket" ]; then
			./aesdsocket -d
		else
			exit 1
		fi

		;;
	"stop")
		echo "stop command received"
		pkill -f "aesdsocket"
		;;
	*)
		echo "Command not recognized"
		;;
esac

