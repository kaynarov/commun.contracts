# WARNING: FAKE Dockerfile, for CI test purposes only!

ARG buildtype=stable
FROM cyberway/cyberway:$buildtype as cyberway
FROM cyberway/cyberway.cdt:$buildtype as cdt

FROM cyberway/builder:$buildtype as builder
COPY --from=cdt /opt/cyberway.cdt /opt/cyberway.cdt
COPY --from=cyberway /opt/cyberway /opt/cyberway

ARG branch=master

# TODO: Can't use https api here, because ssh used for CI runner with private repos:
# ADD https://api.github.com/repos/GolosChain/commun.contracts/git/refs/heads/$branch /etc/version.json

RUN git clone -b $branch git@github.com:GolosChain/commun.contracts.git --recursive

#RUN cd commun.contracts/cyberway.contracts \
#    && echo ":$(git rev-parse HEAD)" > version \
#    && cmake -H. -B"build" \
#        -GNinja \
#        -DCMAKE_BUILD_TYPE=Release \
#        -DCMAKE_INSTALL_PREFIX=/opt/cyberway.contracts/ \
#        -Deosio.cdt_DIR=/opt/cyberway.cdt/lib/cmake/eosio.cdt \
#        -DEOSIO_ROOT=/opt/cyberway \
#    && cmake --build build --target install \
#    && cd .. \
#    && echo "$branch:$(git rev-parse HEAD)" > version \
#    && cmake -H. -B"build" \
#        -GNinja \
#        -DCMAKE_BUILD_TYPE=Release \
#        -DCMAKE_INSTALL_PREFIX=/opt/commun.contracts/ \
#        -Deosio.cdt_DIR=/opt/cyberway.cdt/lib/cmake/eosio.cdt \
#        -DEOSIO_ROOT=/opt/cyberway \
#    && cmake --build build --target install

FROM ubuntu:18.04

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get -y install openssl ca-certificates python3 python3-numpy python3-bson \
    && rm -rf /var/lib/apt/lists/*

#COPY --from=builder /commun.contracts/version /opt/commun.contracts/version
#COPY --from=builder /opt/commun.contracts/ /opt/commun.contracts/
#COPY --from=builder /commun.contracts/build/tests/unit_test /opt/commun.contracts/unit_test

#COPY --from=builder /commun.contracts/cyberway.contracts/version /opt/cyberway.contracts/version
#COPY --from=builder /opt/cyberway.contracts/ /opt/cyberway.contracts/
#COPY --from=builder /commun.contracts/cyberway.contracts/build/tests/unit_test /opt/cyberway.contracts/unit_test
#COPY --from=builder /commun.contracts/cyberway.contracts/tests/test_contracts /opt/cyberway.contracts/test_contracts

#COPY --from=builder /commun.contracts/cyberway.contracts/scripts/bios-boot-sequence /opt/cyberway.contracts/scripts/bios-boot-sequence
#COPY --from=builder /commun.contracts/scripts /opt/commun.contracts/scripts
#COPY --from=builder /commun.contracts/scripts/boot-sequence.py /opt/commun.contracts/scripts/boot-sequence.py

#COPY --from=builder /usr/local/lib/libbson-1.0.so.0.0.0 /usr/local/lib/
#COPY --from=cyberway /opt/cyberway/bin/cleos /opt/cyberway/bin/cleos
#COPY --from=cyberway /opt/cyberway/bin/keosd /opt/cyberway/bin/keosd

#RUN ldconfig && ln -s ../cyberway.contracts /opt/commun.contracts/cyberway.contracts

#ENV CYBERWAY_TEST_CONTRACTS /opt/cyberway.contracts/test_contracts/
#ENV CYBERWAY_CONTRACTS /opt/cyberway.contracts/
#ENV GOLOS_CONTRACTS /opt/commun.contracts/
