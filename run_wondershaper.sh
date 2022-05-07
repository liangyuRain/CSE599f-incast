#!/bin/bash

LIMIT=$1 #Kbps

if [ ! -d "wondershaper" ]; then
  git clone https://github.com/magnific0/wondershaper.git
  cd wondershaper
  sudo make install
fi

if [ $LIMIT -gt 0 ]; then
  echo "Limiting downlink speed to ${LIMIT}Kbps ..."
  sudo wondershaper -a eno50 -d $LIMIT
  echo "Limiting downlink speed to ${LIMIT}Kbps ... done"
else
  echo "Clear downlink speed limit ..."
  sudo wondershaper -c -a eno50
  echo "Clear downlink speed limit ... done"
fi