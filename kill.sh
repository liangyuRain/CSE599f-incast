#!/bin/bash

CONFIG="servers.config"

while IFS= read -r line
do
  tokens=( $line )
  SSH_HOST=${tokens[0]}
  HOST=${tokens[1]}
  PORT=${tokens[2]}

  echo "Killing server on ${SSH_HOST} at ${HOST}:${PORT}..."
  ssh -n "${SSH_HOST}" "tmux kill-server"
done < "${CONFIG}"