#!/bin/bash

CONFIG="servers.config"

HOSTs=""

while IFS= read -r line
do
  tokens=( $line )
  HOSTs="${HOSTs}${tokens[0]} ${tokens[1]},"
done < "${CONFIG}"
HOSTs=$(echo "${HOSTs%,*}" | tr ',' '\n' | sort -u)

IPERF_SERVER_SSH=""
IPERF_SERVER_IP=""
while IFS= read -r line
do
  tokens=( $line )
  SSH_HOST="${tokens[0]}"
  IP_HOST="${tokens[1]}"
  if [ "${IPERF_SERVER_SSH}" == "" ]; then
    IPERF_SERVER_SSH="${SSH_HOST}"
    IPERF_SERVER_IP="${IP_HOST}"
    echo "Setting up ${IPERF_SERVER_SSH} (${IPERF_SERVER_IP}) as iperf server..."
    ssh -n -o "StrictHostKeyChecking=no" "${IPERF_SERVER_SSH}" "tmux new-session -d -s iperf_server 'iperf -s'"
  else
    echo -e "\nTesting bw from ${SSH_HOST} (${IP_HOST}) to ${IPERF_SERVER_SSH} (${IPERF_SERVER_IP})..."
    ssh -n -o "StrictHostKeyChecking=no" "${SSH_HOST}" "iperf -c ${IPERF_SERVER_IP}"
  fi
done <<< "${HOSTs}"