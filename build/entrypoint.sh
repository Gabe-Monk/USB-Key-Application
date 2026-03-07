#!/bin/bash

/app/build/bin/hw_comms &
pid=$!

# exec helps forward SIGTERM from a "docker stop" to the python program to help
# us clean up better
exec python -u /app/main/main.py

kill $pid