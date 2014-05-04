#!/bin/bash
while true
do
   ATIME=`stat -c %Z main.cpp`
   if [[ "$ATIME" != "$LTIME" ]]
   then
	echo ""
	echo ""
	echo "========================="
	echo ""
	make && echo "OK" || echo "FAIL"
	
	LTIME=$ATIME
   fi
   sleep 1
done

