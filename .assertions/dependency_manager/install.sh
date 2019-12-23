#!/bin/bash
apt-package/install.sh g++
apt-package/install.sh cmake
apt-package/install.sh make
systemlib/install.sh pthread
git/install.sh https://github.com/rockerbacon/assertions-test.git v1.0
