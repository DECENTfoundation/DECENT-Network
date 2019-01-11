## OS build image

Naming convention for images: use `decent/` prefix, then append original image name and `/build` suffix, e.g. `decent/ubuntu/build`.

| Build argument | Default value |
| --------------- | ------------- |
| IMAGE_VERSION | latest |

Examples:

    # the latest Ubuntu image
    docker build -t decent/ubuntu/build -f Dockerfile.ubuntu.build .

    # Ubuntu 18.04 image
    docker build -t decent/ubuntu/build:18.04 -f Dockerfile.ubuntu.build --build-arg IMAGE_VERSION=18.04 .

    # Fedora 29 image
    docker build -t decent/fedora/build:29 -f Dockerfile.fedora.build --build-arg IMAGE_VERSION=29 .

## DCore build

DCore build is specific for each platform.

Fedora example:

    mkdir packages
    docker run -it -w /root --name DCore --mount type=bind,src=$PWD/DCore.spec,dst=/root/DCore.spec,readonly $1 --mount type=bind,src=$PWD/packages,dst=/root/rpmbuild/RPMS/x86_64 'rpmbuild -bb –define "dcore_version 1.4.0" -define "git_revision 1.4.0" DCore.spec'
    docker rm DCore
    ls packages
    # DCore-1.4.0-1.fc29.x86_64.rpm
    # DCore-GUI-1.4.0-1.fc29.x86_64.rpm

## DCore runtime image

Naming convention for images: `decent/` prefix, then append original image name and `/dcore` suffix, e.g. `decent/ubuntu/dcore`. You must specify DCore installation package file in `DCORE_FILE` build argument. Optionally you might have to specify the OS layer image version using the `IMAGE_VERSION` build argument.

| Build argument | Default value |
| --------------- | ------------- |
| DCORE_FILE | - |
| IMAGE_VERSION | latest |
| IPFS_VERSION | v0.4.18 |
| TARGETOS | linux |
| TARGETARCH | amd64 |

Examples:

    # the latest image
    docker build -t decent/ubuntu/dcore -f Dockerfile.ubuntu --build-arg DCORE_FILE=DCore-latest.deb .

    # specific release
    docker build -t decent/ubuntu/dcore:1.4.0 -f Dockerfile.ubuntu --build-arg DCORE_FILE=DCore-1.4.0.deb --build-arg IMAGE_VERSION=18.04 .

    # specific release
    docker build -t decent/fedora/dcore:1.4.0 -f Dockerfile.fedora --build-arg DCORE_FILE=DCore-1.4.0-1.fc29.x86_64.rpm --build-arg IMAGE_VERSION=29 .

## DCore run

DCore image exposes 3 ports: 8090 (websocket RPC to listen on), 8091 (wallet websocket RPC to listen on) and 40000 (P2P).
You need to mount an external data directory and genesis file (when using custom configuration) to the running container.

| Host | Container path |
| ---- | -------------- |
| /path/to/data | $DCORE_HOME/.decent/data |
| /path/to/genesis.json | $DCORE_HOME/.decent/genesis.json |

| Environment variable | Default value |
| -------------------- | ------------- |
| DCORE_HOME | /root |
| DCORE_USER | root |

Examples:

    # run node on mainnet
    docker run -d --name DCore --mount 'type=bind,src=/path/to/data,dst=/root/.decent/data' decent/ubuntu/dcore:latest

    # run node on custom configuration
    docker run -d --name DCore --mount 'type=bind,src=/path/to/data,dst=/root/.decent/data' --mount 'type=bind,src=/path/to/genesis.json,dst=/root/.decent/genesis.json' -e "DCORE_EXTRA_ARGS=--genesis-json /root/.decent/genesis.json" decent/ubuntu/dcore:latest

    # run wallet
    docker cp /path/to/wallet.json DCore:/root/.decent/wallet.json
    docker exec -it DCore cli_wallet
    docker cp DCore:/root/.decent/wallet.json /path/to/wallet.json
