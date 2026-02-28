#!/bin/bash

/app/build/bin/hw_comms &
pid=$!

python -u /app/main/main.py

kill $pid