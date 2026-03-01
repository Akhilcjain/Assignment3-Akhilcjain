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
	#	if [ -f "/usr/bin/aesdsocket" ]; then
		start-stop-daemon -S -n aesdsocket -a /usr/bin/aesdsocket -- -d
	#	else
		exit 0
	#	fi

		;;
	"stop")
		echo "stop command received"
		start-stop-daemon -K -n "aesdsocket"
		;;
	*)
		echo "Command not recognized"
		;;
esac

