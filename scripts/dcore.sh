#!/bin/bash

[ $# -lt 2 ] && { echo "Usage: $0 image_name data_dir [genesis_json]"; exit 1; }

if [ $# -lt 3 ]; then
    docker run -d --rm --name DCore -p 5001:5001 -p 8090:8090 -p 40000:40000 \
        --mount type=bind,src=$2,dst=/root/.decent/data $1
else
    docker run -d --rm --name DCore -p 5001:5001 -p 8090:8090 -p 40000:40000 \
        --mount type=bind,src=$2,dst=/root/.decent/data \
        --mount type=bind,src=$3,dst=/root/.decent/genesis.json,readonly \
        -e "DCORE_EXTRA_ARGS=--genesis-json /root/.decent/genesis.json" $1
fi
