#!/usr/bin/env bash

set -eu

img=afl-souffle
docker build -t "${img}" .
docker run \
       --rm \
       --mount type=bind,src=$(realpath $PWD/../../),target=/work \
       --mount type=tmpfs,target=/temp \
       --workdir /work \
       --interactive \
       --tty \
       --memory=24g \
       --memory-swap=25g \
       --network=none \
       "${img}"
