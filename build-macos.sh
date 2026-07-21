#!/bin/sh
# Builds KoboRoot.tgz on macOS using the NickelTC cross-toolchain image, run via
# Apple's `container` CLI (https://github.com/apple/container) instead of Docker.
#
# Requires: the `container` CLI installed.

set -e

cd "$(dirname "$0")"

IMAGE=ghcr.io/pgaskin/nickeltc:1

if ! container system status >/dev/null 2>&1; then
  echo "Starting container system service..."
  container system start
fi

container image pull "$IMAGE"

container run --rm --platform linux/amd64 \
  --volume "$PWD:$PWD" \
  --workdir "$PWD" \
  "$IMAGE" \
  sh -c 'make && make koboroot'
