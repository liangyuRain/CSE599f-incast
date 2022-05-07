#!/bin/bash

RUN_SCRIPT="run_wondershaper.sh"

SSH_HOST=$1
LIMIT=$2 # Kbps

scp -o "StrictHostKeyChecking=no" "${RUN_SCRIPT}" "${SSH_HOST}:~/"
ssh -n -o "StrictHostKeyChecking=no" "${SSH_HOST}" "bash ${RUN_SCRIPT} ${LIMIT}"