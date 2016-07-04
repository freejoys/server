#! /bin/sh
cd ../skynet
make linux
cp -fr skynet lualib luaclib cservice service ../game/
