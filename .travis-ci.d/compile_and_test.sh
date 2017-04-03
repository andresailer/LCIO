#!/bin/bash

ILCSOFT=/cvmfs/clicdp.cern.ch/iLCSoft/builds/current/CI_${COMPILER}
source $ILCSOFT/init_ilcsoft.sh


ls -l /cvmfs/sft.cern.ch/lcg/releases/LCG_88/java/8u91/x86_64-slc6-gcc62-opt/

cd /Package
mkdir build
cd build
cmake -GNinja -DUSE_CXX11=ON -DBUILD_ROOTDICT=ON -DCMAKE_CXX_FLAGS="-fdiagnostics-color=always" \
      -D LCIO_GENERATE_HEADERS=ON \
      -D JAVA_DIR=/cvmfs/sft.cern.ch/lcg/releases/LCG_88/java/8u91/x86_64-slc6-gcc62-opt/ \
      -D INSTALL_JAR=ON -D LCIO_JAVA_USE_MAVEN=OFF .. && \
ninja && \
ninja install && \
ctest --output-on-failure
