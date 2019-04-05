#!/bin/bash

[ $# -lt 3 ] && { echo "Usage: $0 rpm_file gpg_key_id base_image [image_version] [gnupg_dir]"; exit 1; }

RPM_FILE=$1
GPG_KEY_ID=$2
BASE_IMAGE=$3

if [ $# -lt 4 ]; then IMAGE_VERSION="latest"; else IMAGE_VERSION=$4; fi

if [ $# -lt 5 ]; then GNUPG_DIR="$HOME/.gnupg"; else GNUPG_DIR=$5; fi

RPM_ABS_PATH=$(realpath $RPM_FILE)
RPM_FILE_NAME=$(basename $RPM_FILE)

docker run -it -w /root --rm --name $BASE_IMAGE.build.$IMAGE_VERSION \
    --mount type=bind,src=$GNUPG_DIR,dst=/root/.gnupg \
    --mount type=bind,src=$RPM_ABS_PATH,dst=/root/$RPM_FILE_NAME \
    dcore.$BASE_IMAGE.build:$IMAGE_VERSION rpmsign --addsign --key-id "$GPG_KEY_ID" $RPM_FILE_NAME
