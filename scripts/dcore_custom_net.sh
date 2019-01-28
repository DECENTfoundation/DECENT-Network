#!/bin/bash

[ $# -lt 3 ] && { echo "Usage: $0 image_name data_dir genesis_json [container_path]"; exit 1; }

if [ $# -lt 4 ]; then
    docker run -d --rm --name DCore -p 5001:5001 -p 8090:8090 -p 40000:40000 \
        --mount type=bind,src=$2,dst=/root/.decent/data \
        --mount type=bind,src=$3,dst=/root/.decent/genesis.json,readonly \
        -e "DCORE_EXTRA_ARGS=--genesis-json /root/.decent/genesis.json" $1
else
    GENESIS_JSON_PATH=$4"/../genesis.json"
    echo $GENESIS_JSON_PATH
    docker run -d --rm --name DCore -p 5001:5001 -p 8090:8090 -p 40000:40000 \
        --mount type=bind,src=$2,dst=$4 \
        --mount type=bind,src=$3,dst=$GENESIS_JSON_PATH,readonly \
        -e "DCORE_EXTRA_ARGS=--genesis-json "$GENESIS_JSON_PATH -e DCORE_DATA_DIR=$4 $1
fi
