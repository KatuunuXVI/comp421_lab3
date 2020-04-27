#!/bin/sh
make clean
make
/clear/courses/comp421/pub/bin/yalnix -lu 20 -s yfs $1
