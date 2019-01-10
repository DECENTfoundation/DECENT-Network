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

DCore image exposes 3 ports: 8090 (websocket RPC to listen on), 8091 (wallet websocket RPC to listen on) and 40000 (P2P) and 2 volumes for external database and wallets.

| Environment variable | Default value |
| -------------------- | ------------- |
| DATA_DIR | /var/lib/decentd |
| DCORE_HOME | /root |
| DCORE_USER | root |

Examples:

    # create volume (needed only when run for the first time)
    docker volume create decent
    # start node
    docker run --name DCore --rm --env-file env.list --mount 'type=volume,src=decent,dst=/var/lib/decentd' decent/ubuntu/dcore:latest
