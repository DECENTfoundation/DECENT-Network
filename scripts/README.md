### OS Layer

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

### DCore

Naming convention for images: use OS Layer name as prefix and append `/dcore`, e.g. `decent/ubuntu/dcore`. You need to specify the OS Layer image using the `BASE_IMAGE` build argument.

| Build argument | Default value |
| --------------- | ------------- |
| BASE_IMAGE | - |
| GIT_REV | master |

Examples:

    # the latest image
    docker build -t decent/ubuntu/dcore:latest -f Dockerfile.dcore --build-arg BASE_IMAGE=decent/ubuntu:latest

    # specific release
    docker build -t decent/ubuntu/dcore:1.4.0 -f Dockerfile.dcore --build-arg BASE_IMAGE=decent/ubuntu:latest --build-arg GIT_REV=1.4.0

    # development image
    docker build -t decent/ubuntu/dcore:dev -f Dockerfile.dcore --build-arg BASE_IMAGE=decent/ubuntu:latest --build-arg GIT_REV=develop
