#!/bin/bash

# this only works if your build directory has been created and cmaked
cd build
make && bin/kbDemoMinVR -c ../config/YURT_beta_config.xml
