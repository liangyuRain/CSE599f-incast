#!/bin/bash

LIMIT=$1

let "LIMIT_IN_KBps=$LIMIT*1000/8/1024"

if [ ! -d "wondershaper" ]; then
  git clone https://github.com/magnific0/wondershaper.git
  cd wondershaper
  sudo make install
fi

if [ $LIMIT -gt 0 ]; then
  echo "Limiting downlink speed to ${LIMIT}Kbps (${LIMIT_IN_KBps}KB/s) ..."
  sudo wondershaper -a eno50 -d $LIMIT_IN_KBps
  echo "Limiting downlink speed to ${LIMIT}Kbps (${LIMIT_IN_KBps}KB/s) ... done"
else
  echo "Clear downlink speed limit ..."
  sudo wondershaper -c -a eno50
  echo "Clear downlink speed limit ... done"
fi