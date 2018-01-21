#!/bin/bash


# Jemin Hwangbo May 30, 2017
#
# Works on Ubuntu 14.04 and 16.04
# for Ubuntu 14.04, you need to add ffmpeg manually
# Register to bitbucket first, add ssh key then continue

### General configuration of bash and flags for apt-get and add-apt-repository
set -e
set -o xtrace

APT_GET_FLAGS=-qq
ADD_APT_REPOSITORY_FLAGS=-y

### Check Ubuntu version 
version=$(lsb_release -r | awk '{ print $2 }')
yrelease=$( echo "$version" | cut -d. -f1 )
mrelease=$( echo "$version" | cut -d. -f2 )

### General paths
sed -i "/\b\RAI_ROOT\\b/d" $HOME/.bashrc
printf 'export RAI_ROOT='$PWD'\n' >> $HOME/.bashrc
RAI_ROOT="$PWD"

cd "$RAI_ROOT"
mkdir -p dependencies

### adding ppa's
# compilers
sudo add-apt-repository $ADD_APT_REPOSITORY_FLAGS ppa:ubuntu-toolchain-r/test
sudo add-apt-repository ppa:jonathonf/gcc-7.1

## update
sudo apt-get update

## gcc-7 & 6
sudo apt-get install gcc-7 g++-7 g++-6
# basics
sudo apt-get install $APT_GET_FLAGS libtinyxml-dev autoconf automake libtool make unzip

### Build essentials
sudo apt-get install $APT_GET_FLAGS build-essential

### logging
sudo apt-get install $APT_GET_FLAGS libgflags-dev libgoogle-glog-dev

### Boost
sudo apt-get install $APT_GET_FLAGS libboost-all-dev

printf 'export PATH="$PATH:$HOME/bin"\n' >> $HOME/.bashrc

## Cmake 3
wget https://cmake.org/files/v3.6/cmake-3.6.1-Linux-x86_64.sh
chmod +x cmake-3.6.1-Linux-x86_64.sh
sudo ./cmake-3.6.1-Linux-x86_64.sh --skip-license --prefix=/usr
rm cmake-3.6.1-Linux-x86_64.sh

# RAI_Common
cd $RAI_ROOT
sudo rm -rf raicommon
git clone https://bitbucket.org/jhwangbo/raicommon.git
cd raicommon
mkdir build
cd build
cmake .. && sudo make install -j

# RAI_Graphics
cd $RAI_ROOT
sudo rm -rf raigraphics_opengl
git clone https://bitbucket.org/jhwangbo/raigraphics_opengl.git
cd raigraphics_opengl
sudo ./install.sh
mkdir build
cd build
cmake .. && sudo make install -j

cd $RAI_ROOT
exit
