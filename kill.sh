#!/bin/bash

CONFIG="servers.config"

SSH_HOSTs=""
while IFS= read -r line
do
  tokens=( $line )
  SSH_HOSTs="${SSH_HOSTs}${tokens[0]},"
done < "${CONFIG}"
SSH_HOSTs=$(echo "${SSH_HOSTs%,*}" | tr ',' '\n' | sort -u)

while IFS= read -r line
do
  echo "Killing servers on ${line}..."
  ssh -n "${line}" "tmux kill-server" || echo "No server running on ${line}"
done <<< "${SSH_HOSTs}"