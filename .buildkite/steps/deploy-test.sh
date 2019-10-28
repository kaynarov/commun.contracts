#/bin/bash

set -euo pipefail

IMAGETAG=${BUILDKITE_BRANCH:-master}
COMMUN_IMAGE=cyberway/commun.contracts:$IMAGETAG

if [[ "${IMAGETAG}" == "master" ]]; then
    BUILDTYPE="stable"
else
    BUILDTYPE="latest"
fi

export BUILDTYPE
export COMMUN_IMAGE

docker stop mongo nodeosd notifier || true
docker rm mongo nodeosd notifier || true
docker volume rm cyberway-mongodb-data cyberway-nodeos-data cyberway-queue || true
docker volume create --name=cyberway-mongodb-data
docker volume create --name=cyberway-nodeos-data
docker volume create --name=cyberway-queue

cd Docker

rm -fR genesis-data
mkdir genesis-data
env COMMUN_IMAGE=$COMMUN_IMAGE DEST=genesis-data ../scripts/create-genesis.sh

docker-compose -f docker-compose-deploy.yml up -d

function cleanup_docker {
    echo "Cleanup docker-compose..."
    docker-compose -f docker-compose-deploy.yml down
    echo "Cleanup docker-compose done."
}
trap cleanup_docker EXIT

docker logs -f notifier >notifier.log &

# Run unit-tests
docker run --rm --network commun-deploy_test-net -ti $COMMUN_IMAGE /bin/bash -c \
    'retry=3; until /opt/cyberway/bin/cleos --url http://nodeosd:8888 get info; do sleep 10; let retry--; [ $retry -gt 0 ] || exit 1; done; exit 0'

docker run --rm --network commun-deploy_test-net -ti $COMMUN_IMAGE /bin/bash -c \
    '/opt/commun.contracts/scripts/boot-sequence.py'

docker run --rm --network commun-deploy_test-net -v `readlink -f notifier.log`:/events.dump -ti $COMMUN_IMAGE \
    /bin/bash -c 'export PATH=/opt/cyberway/bin/:$PATH CYBERWAY_URL=http://nodeosd:8888 MONGODB=mongodb://mongo:27017; python3 -m unittest discover -v --start-directory /opt/commun.contracts/scripts/'

exit 0
