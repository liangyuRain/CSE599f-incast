#!/bin/bash

PORT="4000"
HOST="127.0.0.1"
while [[ $# -gt 0 ]]; do
  key="$1"
  case $key in
    --port)
      PORT="$2"
      shift
      shift
      ;;
    --host)
      HOST="$2"
      shift
      shift
      ;;
    *)
      shift
      ;;
  esac
done

go version || retVal=1
if [ $retVal -eq 1 ]; then
  if [ ! -f go1.18.1.linux-amd64.tar.gz ]; then
    echo "Go has not been installed. Installing..."
    wget -q https://go.dev/dl/go1.18.1.linux-amd64.tar.gz
    sudo rm -rf /usr/local/go
    sudo tar -C /usr/local -xzf go1.18.1.linux-amd64.tar.gz
  fi
  export PATH=$PATH:/usr/local/go/bin
  go version
fi

SERVER=~/CSE599f-incast/tcp/server.go
if [ ! -f $SERVER ]; then
  echo "${SERVER} does not exist. Cloning..."
  git clone https://github.com/liangyuRain/CSE599f-incast.git
fi

echo "Running server at ${HOST}:${PORT}..."
tmux new-session -d -s server_${PORT} "go run ${SERVER} --host ${HOST} --port ${PORT}"