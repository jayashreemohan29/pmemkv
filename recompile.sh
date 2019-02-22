#!/bin/bash

cd bin
echo "Compiling pmemkv"
make pmemkv
cd ..
echo "Installing pmemkv"
sudo make install
