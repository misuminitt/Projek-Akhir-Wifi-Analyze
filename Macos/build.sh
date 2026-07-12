#!/bin/sh

set -e
cd "$(dirname "$0")"

clang++ -std=c++98 -Wall -Wextra -pedantic wifi_analyze_macos.cpp -o wifi_analyze_macos

echo "Build selesai: Macos/wifi_analyze_macos"
