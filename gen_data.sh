#!/bin/bash
COUNT=0
while true; do 
    [ $COUNT -ge $1 ] && break
    TIME=`echo "scale=5; $RANDOM/25767" | bc -q` 
    echo $TIME `echo $TIME | md5sum | cut -b 1-16`
    # Increment
    COUNT=`expr $COUNT + 1`
done
