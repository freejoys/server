#! /bin/sh
cd ../skynet
make macosx
cp -fr skynet lualib luaclib cservice service ../game/
