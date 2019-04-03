#!/bin/bash
set -euo pipefail

IMAGETAG=${BUILDKITE_BRANCH:-master}
BRANCHNAME=${BUILDKITE_BRANCH:-master}

docker images

docker login -u=$DHUBU -p=$DHUBP
docker push cyberway/commun.contracts:${IMAGETAG}

if [[ "${IMAGETAG}" == "master" ]]; then
    docker tag cyberway/commun.contracts:${IMAGETAG} cyberway/commun.contracts:stable
    docker push cyberway/commun.contracts:stable
fi

if [[ "${IMAGETAG}" == "develop" ]]; then
    docker tag cyberway/commun.contracts:${IMAGETAG} cyberway/commun.contracts:latest
    docker push cyberway/commun.contracts:latest
fi

