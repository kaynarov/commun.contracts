#/bin/bash

set -euo pipefail

docker stop mongo || true
docker rm mongo || true
docker volume rm cyberway-mongodb-data || true
docker volume create --name=cyberway-mongodb-data

cd Docker

IMAGETAG=${BUILDKITE_BRANCH:-master}

docker-compose up -d

# Run unit-tests
sleep 10s
docker run --network commun-tests_contracts-net -ti cyberway/commun.contracts:$IMAGETAG  /bin/bash -c 'export MONGO_URL=mongodb://mongo:27017; /opt/commun.contracts/unit_test -l message -r detailed'
result=$?

docker-compose down

exit $result
