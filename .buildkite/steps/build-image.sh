#!/bin/bash
set -euo pipefail

REVISION=$(git rev-parse HEAD)

pushd cyberway.contracts
SYSTEM_CONTRACTS_VERSION=$(git rev-parse HEAD)
popd

if [[ ${BUILDKITE_BRANCH} == "master" ]]; then
    BUILDTYPE="stable"
else
    BUILDTYPE="latest"
fi

CDT_TAG=${CDT_TAG:-$BUILDTYPE}
CW_TAG=${CW_TAG:-$BUILDTYPE}
BUILDER_TAG=${BUILDER_TAG:-$BUILDTYPE}

docker build -t cyberway/commun.contracts:${REVISION} --build-arg=cw_tag=${CW_TAG} --build-arg=cdt_tag=${CDT_TAG} --build-arg=builder_tag=${BUILDER_TAG} --build-arg=version=${REVISION} --build-arg=sys_contracts_version=${SYSTEM_CONTRACTS_VERSION} -f Docker/Dockerfile .
