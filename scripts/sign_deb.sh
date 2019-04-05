#!/bin/bash

[ $# -lt 3 ] && { echo "Usage: $0 deb_file gpg_key_id base_image [image_version] [gnupg_dir]"; exit 1; }

DEB_FILE=$1
GPG_KEY_ID=$2
BASE_IMAGE=$3

if [ $# -lt 4 ]; then IMAGE_VERSION="latest"; else IMAGE_VERSION=$4; fi

if [ $# -lt 5 ]; then GNUPG_DIR="$HOME/.gnupg"; else GNUPG_DIR=$5; fi

DEB_ABS_PATH=$(realpath $DEB_FILE)
DEB_FILE_NAME=$(basename $DEB_FILE)

docker run -it -w /root --rm --name $BASE_IMAGE.build.$IMAGE_VERSION \
    --mount type=bind,src=$GNUPG_DIR,dst=/root/.gnupg \
    --mount type=bind,src=$DEB_ABS_PATH,dst=/root/$DEB_FILE_NAME \
    dcore.$BASE_IMAGE.build:$IMAGE_VERSION dpkg-sig -s builder -k "$GPG_KEY_ID" $DEB_FILE_NAME
