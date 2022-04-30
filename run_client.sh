#!/bin/bash

if [ $# -ne 6 ]; then
    echo "usage: run_client.sh [hostname file] [server count] [server delay (us)] [server file size (byte)] [rpc launch interval (us)] [num of experiments]"
    exit 1
fi

echo "Building client..."
make clean
make
echo "Running client..."
./client $1 $2 $3 $4 $5 $6