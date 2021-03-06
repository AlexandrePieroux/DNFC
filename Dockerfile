# This is a test docker image to be run with the docker-compose file provided
FROM ubuntu:bionic
LABEL Name=dnfc-test Version=0.0.1

RUN apt-get update
RUN apt-get install -y software-properties-common
RUN apt-get update

RUN apt-get install wget gcc gdbserver libgtest-dev \
    build-essential python-dev autotools-dev libicu-dev \
    build-essential libbz2-dev libboost-all-dev -y
RUN apt-get clean autoclean
RUN apt-get autoremove -y
RUN rm -rf /var/lib/apt/lists/*

# get cmake 3.13 (latest)
RUN wget https://cmake.org/files/v3.13/cmake-3.13.3.tar.gz
RUN tar -xzvf cmake-3.13.3.tar.gz
WORKDIR /cmake-3.13.3/
RUN ./bootstrap
RUN make -j4
RUN make install

# get 1.69 boost (latest)
WORKDIR /
RUN wget https://dl.bintray.com/boostorg/release/1.69.0/source/boost_1_69_0.tar.gz
RUN tar xvzf boost_1_69_0.tar.gz
WORKDIR /boost_1_69_0
RUN ./bootstrap.sh
RUN ./b2 --toolset=gcc cxxstd=17 runtime-link=shared threading=multi install

# compile and install gtest (latest)
WORKDIR /usr/src/gtest
RUN cmake CMakeLists.txt
RUN make 
RUN cp *.a /usr/lib

RUN useradd -ms /bin/bash develop
RUN echo "develop   ALL=(ALL:ALL) ALL" >> /etc/sudoers

# for gdbserver
EXPOSE 2000

USER develop
VOLUME /home/develop/dnfc
WORKDIR /home/develop/dnfc
