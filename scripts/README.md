Building Decent in Docker
-------------------------

Official images can be found in [Docker Hub](https://hub.docker.com/u/decentnetwork) repository. If you like to build your own image see instructions below.

## DCore run

There are separate DCore images for each of the supported platforms.

| OS name | Image name |
| ------- | ---------- |
| Ubuntu | dcore.ubuntu |
| Debian | dcore.debian |
| Fedora | dcore.fedora |

DCore image exposes 2 ports: 8090 (websocket RPC to listen on) and 40000 (P2P node).
You can mount an external data directory (to persist the blockchain) and genesis file (when using custom configuration) to the running container.

The default mapping of local paths to container paths:

| Host | Container path |
| ---- | -------------- |
| /path/to/data | $DCORE_HOME/.decent/data |
| /path/to/genesis.json | $DCORE_HOME/.decent/genesis.json |
| /path/to/wallet.json | $DCORE_HOME/.decent/wallet.json |

To run the node as non root user set the container environment variables:

| Environment variable | Default value |
| -------------------- | ------------- |
| DCORE_HOME | /root |
| DCORE_USER | root |

You can use helper scripts to run the node or wallet:
* `dcore.sh` - run the mainnet node
> Usage: ./dcore.sh image_name data_dir [container_path]
* `dcore_custom_net.sh` - run node on custom net
> Usage: ./dcore_custom_net.sh image_name data_dir genesis_json [container_path]
* `cli_wallet.sh` - start CLI wallet and attach to running node
> Usage: ./cli_wallet.sh wallet_file [container_name]

Examples:

    # run node on mainnet
    ./dcore.sh dcore.ubuntu:1.3.3 /path/to/data

    # run node on custom configuration
    ./dcore_custom_net.sh dcore.ubuntu:1.3.3 /path/to/data /path/to/genesis.json

    # run wallet
    ./cli_wallet.sh /path/to/wallet.json

    # stop node
    docker stop DCore

## DCore build

Because build is specific for each platform, there is a helper script to make life easier. It requires three mandatory arguments (base OS image name and version, DCore version) and two optional arguments (git revision tag - defaults to DCore version if not specified, packages directory - defaults to packages subdirectory).

> Usage: ./build.sh base_image image_version dcore_version [git_revision] [packages_dir]

### Ubuntu (latest, 19.04, 18.04, 16.04)

To create deb packages and OS build image:

    # the latest OS image
    ./build.sh ubuntu latest 1.3.3
    # or specific OS version
    ./build.sh ubuntu 18.04 1.3.3
    ls packages
    # dcore_1.3.3-ubuntu18.04_amd64.deb
    # dcore-gui_1.3.3-ubuntu18.04_amd64.deb

### Debian (latest, 9)

To create deb packages and OS build image:

    # the latest OS image
    ./build.sh debian latest 1.3.3
    # or specific OS version
    ./build.sh debian 9 1.3.3
    ls packages
    # dcore_1.3.3-debian9_amd64.deb
    # dcore-gui_1.3.3-debian9_amd64.deb

### Fedora (latest, 30, 29)

To create rpm packages and OS build image:

    # the latest OS image
    ./build.sh fedora latest 1.3.3
    # or specific OS version
    ./build.sh fedora 29 1.3.3
    ls packages
    # DCore-1.3.3-1.fc29.x86_64.rpm
    # DCore-GUI-1.3.3-1.fc29.x86_64.rpm

Naming convention for images: use `dcore.` prefix, then append base image name and `.build` suffix, e.g. `dcore.ubuntu.build`.

| Build argument | Default value |
| --------------- | ------------- |
| IMAGE_VERSION | latest |
| PBC_VERSION | 0.5.14 |
| PBC_GIT_REV | 0.5.14 |

Examples:

    # the latest Ubuntu image
    docker build -t dcore.ubuntu.build -f ubuntu/Dockerfile.build ubuntu

    # Ubuntu 18.04 image
    docker build -t dcore.ubuntu.build:18.04 -f ubuntu/Dockerfile.build --build-arg IMAGE_VERSION=18.04 ubuntu

    # Debian 9 image
    docker build -t dcore.debian.build:9 -f debian/Dockerfile.build --build-arg IMAGE_VERSION=9 debian

    # Fedora 29 image
    docker build -t dcore.fedora.build:29 -f fedora/Dockerfile.build --build-arg IMAGE_VERSION=29 fedora

## DCore runtime image

If you have DCore deb or rpm packages ready you can build the runtime image. There is a helper script which requires three mandatory arguments (base OS image name and version, DCore version) and one optional argument (packages directory - defaults to packages subdirectory).

> Usage: ./build_runtime.sh base_image image_version dcore_version [packages_dir]

Naming convention for images: `dcore.` prefix and append base image name, e.g. `dcore.ubuntu`. You must specify DCore installation package version in `DCORE_VERSION` build argument. Optionally you can specify the OS layer image version using the `IMAGE_VERSION` and PBC library version using the `PBC_VERSION` build arguments.

| Build argument | Default value |
| --------------- | ------------- |
| DCORE_VERSION | - |
| IMAGE_VERSION | latest |
| PBC_VERSION | 0.5.14 |

    # the latest Ubuntu OS image
    docker build -t dcore.ubuntu:1.3.3 -f ubuntu/Dockerfile --build-arg DCORE_VERSION=1.3.3 packages

    # specific Ubuntu OS version
    docker build -t dcore.ubuntu:1.3.3 -f ubuntu/Dockerfile --build-arg DCORE_VERSION=1.3.3 --build-arg IMAGE_VERSION=18.04 packages

    # specific Debian OS version
    docker build -t dcore.debian:1.3.3 -f debian/Dockerfile --build-arg DCORE_VERSION=1.3.3 --build-arg IMAGE_VERSION=9 packages

    # specific Fedora OS version
    docker build -t dcore.fedora:1.3.3 -f fedora/Dockerfile --build-arg DCORE_VERSION=1.3.3 --build-arg IMAGE_VERSION=29 packages

## DCore custom build

It is also possible to build DCore on custom OS image which satisfy all required dependencies.

| Build argument | Default value |
| --------------- | ------------- |
| BASE_IMAGE | - |
| GIT_REV | master |

Examples:

    # the latest DCore version
    docker build -t dcore.custom -f Dockerfile.dcore --build-arg BASE_IMAGE=custom .

    # specific DCore release
    docker build -t dcore.custom:1.3.3 -f Dockerfile.dcore --build-arg BASE_IMAGE=custom --build-arg GIT_REV=1.3.3 .
