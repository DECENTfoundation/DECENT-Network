#!/bin/bash

[ $# -lt 1 ] && { echo "Usage: $0 wallet_file [image_name]"; exit 1; }

if [ $# -lt 2 ]; then IMAGE_NAME=DCore; else IMAGE_NAME=$2; fi

docker cp $1 $IMAGE_NAME:/root/.decent/wallet.json
docker exec -it -w /root $IMAGE_NAME cli_wallet
docker cp $IMAGE_NAME:/root/.decent/wallet.json $1
