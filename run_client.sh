#!/bin/bash

if [ $# -ne 6 ]; then
    echo "usage: run_client.sh [hostname file] [server count] [server delay] [server file size] [rpc launch interval] [num of experiments]"
    exit 1
fi

echo "Running client"
make clean
make
./client $1 $2 $3 $4 $5 $6