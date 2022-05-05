#!/bin/bash

CONFIG="servers.config"

LOCAL_SSH_HOST=""
while [[ $# -gt 0 ]]; do
  key="$1"
  case $key in
    --host)
      LOCAL_SSH_HOST="$2"
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

while IFS= read -r line
do
  tokens=( $line )
  SSH_HOST=${tokens[0]}
  HOST=${tokens[1]}
  PORT=${tokens[2]}

  if [ "${SSH_HOST}" == "${LOCAL_SSH_HOST}" ]; then
    echo "Launching server on ${SSH_HOST} at ${HOST}:${PORT} ..."
    tmux new-session -d -s server_${PORT} "go run ${SERVER} --host ${HOST} --port ${PORT}"
    echo "Launching server on ${SSH_HOST} at ${HOST}:${PORT} ... done"
  fi
done < "${CONFIG}"