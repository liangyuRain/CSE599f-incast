#!/bin/bash

if [ $# -eq 0 ]; then
    echo "usage: deploy_client.sh [ssh hostname]"
    exit 1
fi

SSH_HOST=$1

CONFIG="servers.config"
REPO="CSE599f-incast"
RUN_SCRIPT="run_client.sh"

SERVER_COUNT=1
SERVER_DELAY=0
SERVER_FILE_SIZE=1024
LAUNCH_INTERVAL=0
NUM_OF_EXPERIMENTS=10

while [[ $# -gt 0 ]]; do
  key="$1"
  case $key in
    --serverCount)
      SERVER_COUNT="$2"
      shift
      shift
      ;;
    --serverDelay)
      SERVER_DELAY="$2"
      shift
      shift
      ;;
    --serverFileSize)
      SERVER_FILE_SIZE="$2"
      shift
      shift
      ;;
    --launchInterval)
      LAUNCH_INTERVAL="$2"
      shift
      shift
      ;;
    --numOfExperiments)
      NUM_OF_EXPERIMENTS="$2"
      shift
      shift
      ;;
    *)
      shift
      ;;
  esac
done

echo "Deploying client to ${SSH_HOST}..."
scp -r [!.]* "${SSH_HOST}:~/"
ssh -n "${SSH_HOST}" "cd ${PWD##*/} ; bash ${RUN_SCRIPT} ${CONFIG} ${SERVER_COUNT} ${SERVER_DELAY} ${SERVER_FILE_SIZE} ${LAUNCH_INTERVAL} ${NUM_OF_EXPERIMENTS}"