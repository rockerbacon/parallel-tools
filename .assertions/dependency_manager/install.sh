#!/bin/bash
apt-package/install.sh g++
apt-package/install.sh cmake
apt-package/install.sh make
git/install.sh https://github.com/rockerbacon/assertions-test.git v2.0.4 true "src/objs" "src/objs" ""
git/install.sh https://github.com/rockerbacon/cpp-benchmark.git v1.0.5 false "src/objs" "src/objs" ""
