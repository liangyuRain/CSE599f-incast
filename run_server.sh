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

echo "Building server ..."
make clean
make server
echo "Building server ... done"

while IFS= read -r line
do
  tokens=( $line )
  SSH_HOST=${tokens[0]}
  HOST=${tokens[1]}
  PORT=${tokens[2]}

  if [ "${SSH_HOST}" == "${LOCAL_SSH_HOST}" ]; then
    echo "Launching server on ${SSH_HOST} at ${HOST}:${PORT} ..."
    tmux new-session -d -s server_${PORT} "bash -c 'stdbuf --output=L ./server ${HOST} ${PORT} 2>&1 | tee server_${PORT}.log'"
    echo "Launching server on ${SSH_HOST} at ${HOST}:${PORT} ... done"
  fi
done < "${CONFIG}"