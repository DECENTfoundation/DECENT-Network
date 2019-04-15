#!/bin/bash

[ $# -lt 3 ] && { echo "Usage: $0 image_name data_dir genesis_json [container_user [container_home]]"; exit 1; }

if [ $# -lt 4 ]; then CONTAINER_USER=dcore; else CONTAINER_USER=$4; fi

if [ $# -lt 5 ]; then CONTAINER_HOME=/home/$CONTAINER_USER; else CONTAINER_HOME=$5; fi

GENESIS_JSON_PATH=$CONTAINER_HOME/.decent/genesis.json

if [ $# -lt 4 ]; then
    docker run -d --rm --name DCore -p 8090:8090 -p 40000:40000 \
        --mount type=bind,src=$2,dst=$CONTAINER_HOME/.decent/data \
        --mount type=bind,src=$3,dst=$GENESIS_JSON_PATH,readonly \
        -e "DCORE_EXTRA_ARGS=--genesis-json $GENESIS_JSON_PATH" $1
else
    docker run -d --rm --name DCore -p 8090:8090 -p 40000:40000 \
        --mount type=bind,src=$2,dst=$CONTAINER_HOME/.decent/data \
        --mount type=bind,src=$3,dst=$GENESIS_JSON_PATH,readonly \
        -e "DCORE_EXTRA_ARGS=--genesis-json $GENESIS_JSON_PATH" \
        -e "DCORE_USER=$CONTAINER_USER" -e "DCORE_HOME=$CONTAINER_HOME" $1
fi
