## OS build image

Naming convention for images: use `decent/` prefix, then append original image name and `/build` suffix, e.g. `decent/ubuntu/build`.

| Build argument | Default value |
| --------------- | ------------- |
| IMAGE_VERSION | latest |

Examples:

    # the latest Ubuntu image
    docker build -t decent/ubuntu/build -f Dockerfile.ubuntu.build .

    # Ubuntu 16.04 image
    docker build -t decent/ubuntu/build:16.04 -f Dockerfile.ubuntu.build --build-arg IMAGE_VERSION=16.04 .

    # Fedora 29 image
    docker build -t decent/fedora/build:29 -f Dockerfile.fedora.build --build-arg IMAGE_VERSION=29 .

## DCore runtime image

Naming convention for images: `decent/` prefix, then append original image name and `/dcore` suffix, e.g. `decent/ubuntu/dcore`. You must specify DCore installation package file in `DCORE_FILE` build argument. Optionally you might have to specify the OS layer image version using the `IMAGE_VERSION` build argument.

| Build argument | Default value |
| --------------- | ------------- |
| DCORE_FILE | - |
| IMAGE_VERSION | latest |
| IPFS_VERSION | v0.4.18 |
| TARGETOS | linux |
| TARGETARCH | amd64 |

Because DCore build is specific for each platform, there are helper scripts to make life easier. Each one requires
two mandatory arguments (OS image and DCore versions) and one optional argument (git revision tag - it defaults to DCore version if not specified).

Ubuntu example (creates deb packages and docker image):

    # the latest OS image
    ./ubuntu.sh latest 1.3.3
    # or specific OS version
    ./ubuntu.sh 16.04 1.3.3
    ls packages
    # dcore_1.3.3_amd64.deb
    # dcore-gui_1.3.3_amd64.deb
    docker images
    # decent/ubuntu/dcore   1.3.3

Fedora example (creates rpm packages and docker image):

    # the latest OS image
    ./fedora.sh latest 1.3.3
    # or specific OS version
    ./fedora.sh 29 1.3.3
    ls packages
    # DCore-1.3.3-1.fc29.x86_64.rpm
    # DCore-GUI-1.3.3-1.fc29.x86_64.rpm
    docker images
    # decent/fedora/dcore   1.3.3

If you already have DCore installation packages:

    # the latest OS image
    docker build -t decent/ubuntu/dcore:1.3.3 -f Dockerfile.ubuntu --build-arg DCORE_FILE=/path/to/dcore_1.3.3_amd64.deb .

    # specific OS version
    docker build -t decent/ubuntu/dcore:1.3.3 -f Dockerfile.ubuntu --build-arg DCORE_FILE=/path/to/dcore_1.3.3_amd64.deb --build-arg IMAGE_VERSION=16.04 .

    # specific OS version
    docker build -t decent/fedora/dcore:1.3.3 -f Dockerfile.fedora --build-arg DCORE_FILE=/path/to/DCore-1.3.3-1.fc29.x86_64.rpm --build-arg IMAGE_VERSION=29 .

## DCore custom build

It is also possible to build DCore on custom OS image which satisfy all required dependencies.

| Build argument | Default value |
| --------------- | ------------- |
| BASE_IMAGE | - |
| GIT_REV | master |

Examples:

    # the latest DCore version
    docker build -t decent/custom/dcore -f Dockerfile.dcore --build-arg BASE_IMAGE=custom .

    # specific DCore release
    docker build -t decent/custom/dcore:1.3.3 -f Dockerfile.dcore --build-arg BASE_IMAGE=custom --build-arg GIT_REV=1.3.3 .

## DCore run

DCore image exposes 2 ports: 8090 (websocket RPC to listen on) and 40000 (P2P).
You need to mount an external data directory and genesis file (when using custom configuration) to the running container.

| Host | Container path |
| ---- | -------------- |
| /path/to/data | $DCORE_HOME/.decent/data |
| /path/to/genesis.json | $DCORE_HOME/.decent/genesis.json |
| /path/to/wallet.json | $DCORE_HOME/.decent/wallet.json |

| Environment variable | Default value |
| -------------------- | ------------- |
| DCORE_HOME | /root |
| DCORE_USER | root |

Examples:

    # run node on mainnet
    ./dcore.sh decent/ubuntu/dcore:1.3.3 /path/to/data

    # run node on custom configuration
    ./dcore.sh decent/ubuntu/dcore:1.3.3 /path/to/data /path/to/genesis.json

    # run wallet
    ./cli_wallet.sh /path/to/wallet.json
