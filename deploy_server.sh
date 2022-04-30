#!/bin/bash

CONFIG="servers.config"
RUN_SCRIPT="run_server.sh"

while IFS= read -r line
do
  tokens=( $line )
  SSH_HOST=${tokens[0]}
  HOST=${tokens[1]}
  PORT=${tokens[2]}

  echo "Deploying server to ${SSH_HOST} at ${HOST}:${PORT}..."
  scp "${RUN_SCRIPT}" "${SSH_HOST}:~/"
  ssh -n "${SSH_HOST}" "bash ${RUN_SCRIPT} --host ${HOST} --port ${PORT}"
done < "${CONFIG}"