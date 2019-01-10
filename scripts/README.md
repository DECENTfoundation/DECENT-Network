## OS Layer

Naming convention for images: use `decent/` prefix and append original image name, e.g. `decent/ubuntu`.

| Build argument | Default value |
| --------------- | ------------- |
| VERSION | latest |
| IPFS_VERSION | v0.4.18 |
| TARGETOS | linux |
| TARGETARCH | amd64 |

Examples:

    # the latest ubuntu image
    docker build -t decent/ubuntu -f Dockerfile.ubuntu .

    # Fedora 29 image
    docker build -t decent/fedora -f Dockerfile.fedora --build-arg VERSION=29 .

## DCore build

Naming convention for images: use OS Layer name as prefix and append `/dcore`, e.g. `decent/ubuntu/dcore`. You need to specify the OS Layer image using the `BASE_IMAGE` build argument.

| Build argument | Default value |
| --------------- | ------------- |
| BASE_IMAGE | - |
| GIT_REV | master |

Examples:

    # the latest image
    docker build -t decent/ubuntu/dcore:latest -f Dockerfile.dcore --build-arg BASE_IMAGE=decent/ubuntu:latest .

    # specific release
    docker build -t decent/ubuntu/dcore:1.4.0 -f Dockerfile.dcore --build-arg BASE_IMAGE=decent/ubuntu:latest --build-arg GIT_REV=1.4.0 .

    # development image
    docker build -t decent/ubuntu/dcore:dev -f Dockerfile.dcore --build-arg BASE_IMAGE=decent/ubuntu:latest --build-arg GIT_REV=develop .

## DCore run

DCore image exposes 3 ports: 8090 (websocket RPC to listen on), 8091 (wallet websocket RPC to listen on) and 40000 (P2P).
You need to mount an external data directory and genesis file (when using custom configuration) to running the container.

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
    docker run -d --name DCore --mount 'type=bind,src=/path/to/data,dst=$DCORE_HOME/.decent/data' decent/ubuntu/dcore:latest

    # run node on custom configuration
    docker run -d --name DCore --mount 'type=bind,src=/path/to/data,dst=$DCORE_HOME/.decent/data' --mount 'type=bind,src=/path/to/genesis.json,dst=$DCORE_HOME/.decent/genesis.json' -e "DCORE_EXTRA_ARGS=--genesis-json $DCORE_HOME/.decent/genesis.json" decent/ubuntu/dcore:latest

    # run wallet
    docker cp /path/to/wallet.json DCore:$DCORE_HOME/.decent/wallet.json
    docker exec -it DCore cli_wallet
    docker cp DCore:$DCORE_HOME/.decent/wallet.json /path/to/wallet.json
