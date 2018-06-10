#!/bin/bash

cmake-build-debug/ssas -s 0 -o stdlib stdlib.txt && cmake-build-debug/ssas -s 1024 -o obj1 testfile1.txt && cmake-build-debug/ssas -s 2048 -o obj2 testfile2.txt && cmake-build-debug/ssemu obj1 obj2 stdlib
