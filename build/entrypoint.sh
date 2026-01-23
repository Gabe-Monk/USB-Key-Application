#!/bin/bash
# g++ zmqTest.cpp -o zmqTest -lzmq

hardware_communication/hw_comms &
pid=$!

python -u main/main.py

kill $pid