#!/bin/bash

hardware_communication/hw_comms &
pid=$!

python -u main/main.py

kill $pid