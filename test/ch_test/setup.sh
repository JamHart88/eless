#!/usr/bin/sh

ln -sf ../../*.hpp .
ln -sf ../../ch.cpp
rm ifile.hpp
ln -sf ch_mocks.hpp ifile.hpp 
