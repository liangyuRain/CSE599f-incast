#!/bin/bash

if [ $# -eq 0 ]; then
    echo "usage: deploy_client.sh [ssh hostname] [--serverCount server_count] [--serverDelay server_delay (us)] [--serverFileSize server_file_size (byte)] [--launchInterval launch_interval (us)] [--numOfExperiments num_of_experiments] [--maxWinConnect max_num_of_sending_server] [--maxWinSize max_total_size_of_sending_files]"
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
MAX_WIN_CONNECT=0
MAX_WIN_SIZE=0

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
    --maxWinConnect)
      MAX_WIN_CONNECT="$2"
      shift
      shift
      ;;
    --maxWinSize)
      MAX_WIN_SIZE="$2"
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
ssh -t "${SSH_HOST}" "bash ${RUN_SCRIPT} ${CONFIG} ${SERVER_COUNT} ${SERVER_DELAY} ${SERVER_FILE_SIZE} ${LAUNCH_INTERVAL} ${NUM_OF_EXPERIMENTS} ${MAX_WIN_CONNECT} ${MAX_WIN_SIZE}"