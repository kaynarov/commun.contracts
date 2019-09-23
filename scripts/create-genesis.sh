#!/bin/bash
set -e

: ${DEST:="genesis-data-temp"}
: ${COMMUN_IMAGE:="cyberway/commun.contracts:latest"}
: ${INITIAL_TIMESTAMP:=$(date +"%FT%T.%3N" --utc)}

err() {
    echo "ERROR: $*" >&2
    exit 1
}


echo "Create directory $DEST"
[ -d $DEST ] || mkdir $DEST

echo "Info about images $COMMUN_IMAGE:"
docker image ls $COMMUN_IMAGE | tail -n +2

LS_CMD="ls -l / /opt/cyberway/bin/ /opt/golos.contracts /opt/golos.contracts/genesis"
CREATE_GENESIS_CMD="/opt/cyberway/bin/create-genesis -g genesis-info.json -o /genesis-data/genesis.dat"
EXTRA_MAPPED_FILES=

if [ -f "$GENESIS_JSON_TMPL" ]; then
    EXTRA_MAPPED_FILES+=" -v `readlink -f $GENESIS_JSON_TMPL`:/opt/golos.contracts/genesis/genesis.json.tmpl"
fi

if [ -f "$GENESIS_INFO_TMPL" ]; then
    EXTRA_MAPPED_FILES+=" -v `readlink -f $GENESIS_INFO_TMPL`:/opt/golos.contracts/genesis/genesis-info.json.tmpl"
fi

rm -f create-genesis.log && touch create-genesis.log

docker run --rm \
    -v `readlink -f create-genesis.log`:/create-genesis.log \
    $EXTRA_MAPPED_FILES \
    -v `readlink -f $DEST`:/genesis-data \
    -e LS_CMD="$LS_CMD" \
    -e CREATE_GENESIS_CMD="$CREATE_GENESIS_CMD" \
    $COMMUN_IMAGE bash -c \
    '$LS_CMD &&
     sed "s|\${INITIAL_TIMESTAMP}|'$INITIAL_TIMESTAMP$'|; /^#/d" /opt/golos.contracts/genesis/genesis.json.tmpl | tee genesis.json /genesis-data/genesis.json&& \
     sed "s|\$CYBERWAY_CONTRACTS|$CYBERWAY_CONTRACTS|;s|\$COMMUN_CONTRACTS|$COMMUN_CONTRACTS|; /^#/d" /opt/golos.contracts/genesis/genesis-info.json.tmpl | tee genesis-info.json && \
     $CREATE_GENESIS_CMD 2>&1 | tee create-genesis.log'

gpo=$(grep -oP "Dynamic global property: \K.*" <create-genesis.log || err "Failed to extract dynamic global property")
head_block_id=$(echo $gpo | grep -oP '"head_block_id":"\K[0-9a-f]{40}(?=")' || err "Failed to extract head_block_num")
initial_timestamp=$(grep -oP "Set initial_timestamp to \K.*$" <create-genesis.log || true)
[ "$initial_timestamp" ] && set_initial_timestamp="s|\(\"initial_timestamp\"\s*:\s*\)\".*\"|\1\"$initial_timestamp\"|; " || true

GENESIS_DATA_HASH=$(sha256sum $DEST/genesis.dat | cut -f1 -d" ")
sed -i.bak "s|\${GENESIS_DATA_HASH}|$GENESIS_DATA_HASH|; s|\(\"initial_chain_id\":\s*\"0\+\)0\{40\}\"|\1$head_block_id\"|; $set_initial_timestamp" $DEST/genesis.json
cat $DEST/genesis.json
