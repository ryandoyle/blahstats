#!/bin/bash
while true; do 
    TIME=`echo "scale=5; $RANDOM/25767" | bc -q` 
    echo $TIME `echo $TIME | md5sum | cut -b 1-16`
    # Increment
    COUNT=`expr $COUNT + 1`
    sleep $1
done
