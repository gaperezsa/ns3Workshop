#!/bin/bash
filename=$1
cp $filename.cc ../../ns-allinone-3.36.1/ns-3.36.1/scratch/$filename.cc
../../ns-allinone-3.36.1/ns-3.36.1/ns3 build
../../ns-allinone-3.36.1/ns-3.36.1/ns3 run scratch/$filename
