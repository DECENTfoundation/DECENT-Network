#!/bin/bash

[ $# -lt 2 ] && { echo "Usage: $0 image_name dcore_version"; exit 1; }

docker run -it -w /root --name DCore --mount type=bind,src=$PWD/DCore.spec,dst=/root/DCore.spec,readonly --mount type=bind,src=$PWD/dcore.sh,dst=/root/dcore.sh,readonly $1 "/root/dcore.sh $2"
docker cp DCore:/root/rpmbuild/RPMS/x86_64/DCore-$DCORE_VERSION-1.fc29.x86_64.rpm .
docker cp DCore:/root/rpmbuild/RPMS/x86_64/DCore-GUI-$DCORE_VERSION-1.fc29.x86_64.rpm .
docker stop DCore
docker rm DCore
