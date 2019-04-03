#!/bin/bash
set -euo pipefail

IMAGETAG=${BUILDKITE_BRANCH:-master}
BRANCHNAME=${BUILDKITE_BRANCH:-master}

if [[ "${IMAGETAG}" == "master" ]]; then
    BUILDTYPE="stable"
else
    BUILDTYPE="latest"
fi

docker build -t cyberway/commun.contracts:${IMAGETAG} --build-arg branch=${BRANCHNAME} --build-arg buildtype=${BUILDTYPE} -f Docker/Dockerfile .
