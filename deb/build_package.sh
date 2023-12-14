#!/bin/bash
dpkg-deb --build libcfg
mkdir -p packages
mv libcfg.deb packages/libcfg-v0.0.1-amd64.deb