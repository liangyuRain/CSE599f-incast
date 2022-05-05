#!/bin/bash

CONFIG="servers.config"
RUN_SCRIPT="run_server.sh"

SSH_HOSTs=""
while IFS= read -r line
do
  tokens=( $line )
  SSH_HOSTs="${SSH_HOSTs}${tokens[0]},"
done < "${CONFIG}"
SSH_HOSTs=$(echo "${SSH_HOSTs%,*}" | tr ',' '\n' | sort -u)

while IFS= read -r line
do
  echo "Sending ${RUN_SCRIPT} to ${line} ..."
  scp -o "StrictHostKeyChecking=no" "${CONFIG}" "${line}:~/"
  scp -o "StrictHostKeyChecking=no" "${RUN_SCRIPT}" "${line}:~/"
  echo "Sending ${RUN_SCRIPT} to ${line} ... done"

  echo "Deploying servers on ${line} ..."
  ssh -n -o "StrictHostKeyChecking=no" "${line}" "bash ${RUN_SCRIPT} --host ${line}"
  echo "Deploying servers on ${line} ... done"
done <<< "${SSH_HOSTs}"