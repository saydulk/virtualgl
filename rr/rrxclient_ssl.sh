#!/bin/sh
# description: Linux Client Daemon

. /etc/rc.d/init.d/functions

[ -x /usr/bin/vglclient ] || exit 0

RETVAL=0
LOGPATH=/var/log/

start() {
	echo -n $"Starting Secure Client Daemon: "
	daemon /usr/bin/vglclient --service -s -l$LOGPATH/vglclient_ssl.log && success || failure
	echo
	if [ ! $? -eq 0 ]; then RETVAL=$?; fi
}

stop() {
	echo -n $"Shutting down Secure Client Daemon: "
	killproc vglclient
	echo
}

reload() {
	stop
	start
}

ID=`id -u`
if [ ! $ID -eq 0 ]; then
	LOGPATH=$HOME/
fi

case "$1" in
	start)
		start
		;;
	stop)
		stop
		;;
	restart)
		stop
		start
		;;
	*)
		echo $"Usage: $prog {start|stop|restart}"
		exit 1
esac
exit $RETVAL
