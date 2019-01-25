## DCore runtime image

Because DCore build is specific for each platform, there are helper scripts to make life easier. Each of them requires
two mandatory arguments (OS image and DCore versions) and two optional arguments (git revision tag - defaults to DCore version if not specified, packages directory - defaults to packages subdirectory).

> Usage: ./ubuntu.sh image_version dcore_version [git_revision] [package_dir]

### Ubuntu (latest, 18.04, 16.04)

To create deb packages and docker image:

    # the latest OS image
    ./ubuntu.sh latest 1.3.3
    # or specific OS version
    ./ubuntu.sh 18.04 1.3.3
    ls packages
    # dcore_1.3.3-ubuntu18.04_amd64.deb
    # dcore-gui_1.3.3-ubuntu18.04_amd64.deb
    docker images
    # decent/ubuntu/dcore   1.3.3

### Debian (latest, 9.6)

To create deb packages and docker image:

    # the latest OS image
    ./debian.sh latest 1.3.3
    # or specific OS version
    ./debian.sh 9.6 1.3.3
    ls packages
    # dcore_1.3.3-debian9.6_amd64.deb
    # dcore-gui_1.3.3-debian9.6_amd64.deb
    docker images
    # decent/debian/dcore   1.3.3

### Fedora (latest, 29, 28)

To create rpm packages and docker image:

    # the latest OS image
    ./fedora.sh latest 1.3.3
    # or specific OS version
    ./fedora.sh 29 1.3.3
    ls packages
    # DCore-1.3.3-1.fc29.x86_64.rpm
    # DCore-GUI-1.3.3-1.fc29.x86_64.rpm
    docker images
    # decent/fedora/dcore   1.3.3

If you already have DCore deb or rpm packages you can just build the runtime image.

Naming convention for images: `decent/` prefix, then append original image name and `/dcore` suffix, e.g. `decent/ubuntu/dcore`. You must specify DCore installation package version in `DCORE_VERSION` build argument. Optionally you can specify the OS layer image version using the `IMAGE_VERSION` and PBC library version using the `PBC_VERSION` build arguments.

| Build argument | Default value |
| --------------- | ------------- |
| DCORE_VERSION | - |
| IMAGE_VERSION | latest |
| PBC_VERSION | 0.5.14 |

    # the latest Ubuntu OS image
    docker build -t decent/ubuntu/dcore:1.3.3 -f ubuntu/Dockerfile --build-arg DCORE_VERSION=1.3.3 packages

    # specific Ubuntu OS version
    docker build -t decent/ubuntu/dcore:1.3.3 -f ubuntu/Dockerfile --build-arg DCORE_VERSION=1.3.3 --build-arg IMAGE_VERSION=18.04 packages

    # specific Debian OS version
    docker build -t decent/debian/dcore:1.3.3 -f debian/Dockerfile --build-arg DCORE_VERSION=1.3.3 --build-arg IMAGE_VERSION=9.6 packages

    # specific Fedora OS version
    docker build -t decent/fedora/dcore:1.3.3 -f fedora/Dockerfile --build-arg DCORE_VERSION=1.3.3 --build-arg IMAGE_VERSION=29 packages

## DCore run

DCore image exposes 3 ports: 8090 (websocket RPC to listen on), 5001 (IPFS API) and 40000 (P2P).
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

    # stop node
    docker stop DCore

## OS build image

Naming convention for images: use `decent/` prefix, then append original image name and `/build` suffix, e.g. `decent/ubuntu/build`.

| Build argument | Default value |
| --------------- | ------------- |
| IMAGE_VERSION | latest |
| PBC_VERSION | 0.5.14 |
| PBC_GIT_REV | 0.5.14 |

Examples:

    # the latest Ubuntu image
    docker build -t decent/ubuntu/build -f ubuntu/Dockerfile.build ubuntu

    # Ubuntu 18.04 image
    docker build -t decent/ubuntu/build:18.04 -f ubuntu/Dockerfile.build --build-arg IMAGE_VERSION=18.04 ubuntu

    # Debian 9.6 image
    docker build -t decent/debian/build:9.6 -f debian/Dockerfile.build --build-arg IMAGE_VERSION=9.6 debian

    # Fedora 29 image
    docker build -t decent/fedora/build:29 -f fedora/Dockerfile.build --build-arg IMAGE_VERSION=29 fedora

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
