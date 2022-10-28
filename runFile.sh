#!/bin/bash
filename=$1
cp $filename.cc ../../libs/ns-3-dev/scratch/$filename.cc
../../libs/ns-3-dev/ns3 build
../../libs/ns-3-dev/ns3 run scratch/$filename
