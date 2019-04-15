#!/bin/bash

[ $# -lt 2 ] && { echo "Usage: $0 image_name data_dir [container_user [container_home]]"; exit 1; }

if [ $# -lt 3 ]; then CONTAINER_USER=dcore; else CONTAINER_USER=$3; fi

if [ $# -lt 4 ]; then CONTAINER_HOME=/home/$CONTAINER_USER; else CONTAINER_HOME=$4; fi

if [ $# -lt 3 ]; then
    docker run -d --rm --name DCore -p 8090:8090 -p 40000:40000 \
        --mount type=bind,src=$2,dst=$CONTAINER_HOME/.decent/data $1
else
    docker run -d --rm --name DCore -p 8090:8090 -p 40000:40000 \
        --mount type=bind,src=$2,dst=$CONTAINER_HOME/.decent/data \
        -e "DCORE_USER=$CONTAINER_USER" -e "DCORE_HOME=$CONTAINER_HOME" $1
fi
