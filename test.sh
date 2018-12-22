#!/bin/sh

cd `dirname $0`

#CONFIG=Debug
CONFIG=Release

sudo kextunload -b org.darkvoid.driver.IOElectrify

xcodebuild -configuration ${CONFIG} &&
sudo cp -R build/${CONFIG}/IOElectrify.kext /tmp &&
sudo chown -R root:wheel /tmp/IOElectrify.kext &&
sudo kextutil /tmp/IOElectrify.kext;

