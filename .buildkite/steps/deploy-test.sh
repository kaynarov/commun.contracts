#/bin/bash
set -euo pipefail

IMAGETAG=stable
ALL=
INTERACTIVE=
LOCAL=

while getopts t:ailh option; do
case "${option}" in
    t) IMAGETAG=${OPTARG};;
    a) ALL=1;;
    i) INTERACTIVE=1;;
    l) LOCAL=1;;
    *) echo "Usage: $0 [OPTIONS]. Where OPTIONS can be:"
       echo "    -t <IMAGETAG>  tag for cyberway/commun.contracts docker-image"
       echo "    -a             run all tests"
       echo "    -i             interactive mode"
       echo "    -l             use local 'scripts' directory"
       exit 1;;
esac
done

COMMUN_IMAGE=cyberway/commun.contracts:$IMAGETAG
echo "Use ${COMMUN_IMAGE} image"

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

EXTRA_ARGS=
if [[ -n "$LOCAL" ]]; then
    EXTRA_ARGS+=" -v `readlink -f ../cyberway.contracts/build`:/opt/cyberway.contracts/"
    EXTRA_ARGS+=" -v `readlink -f ../build`:/opt/commun.contracts/"
    EXTRA_ARGS+=" -v `readlink -f ../scripts`:/opt/commun.contracts/scripts/"
fi
if [[ -n "$ALL" ]]; then
    cmd='python3 -m unittest discover -v --start-directory /opt/commun.contracts/scripts/'
else
    cmd="export INTERACTIVE=${INTERACTIVE} EVENTS_FILE=/events.dump; alias cleos='cleos -u http://nodeosd:8888'; cd /opt/commun.contracts/scripts; echo; echo 'For run tests use: ./testDeploy.py <class>/<method>'; /bin/bash"
fi
docker run --rm --network commun-deploy_test-net -v `readlink -f notifier.log`:/events.dump $EXTRA_ARGS -ti $COMMUN_IMAGE \
    /bin/bash -c 'export PATH=/opt/cyberway/bin/:$PATH CYBERWAY_URL=http://nodeosd:8888 MONGODB=mongodb://mongo:27017; '"$cmd"

exit 0
