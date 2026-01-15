#!/bin/bash
g++ zmqTest.cpp -o zmqTest -lzmq

./zmqTest &
pid=$!

python3 zmqTest.py

kill $pid