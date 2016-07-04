#!/bin/sh

sys=`uname -s`
if [ "$sys" = "Darwin" ]; then
	unzip ios.zip -d ../
fi

if [ "$sys" = "Linux" ]; then
	unzip linux.zip -d ../
fi

echo "install complete"

