#!/bin/bash

if [ $# -ne 8 ]; then
    echo "usage: run_client.sh [hostname file] [server count] [server delay (us)] [server file size (byte)] [rpc launch interval (us)] [num of experiments] [max num of connections in a window] [max total size of connections in a window]"
    exit 1
fi

echo "Building client..."
make clean
make client
echo "Running client..."
./client $1 $2 $3 $4 $5 $6 $7 $8