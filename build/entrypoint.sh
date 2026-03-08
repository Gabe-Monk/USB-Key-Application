#!/bin/bash

# Run hw_comms in a loop so it restarts if it crashes
(while true; do /app/build/bin/hw_comms; sleep 1; done) &
pid=$!

# exec helps forward SIGTERM from a "docker stop" to the python program to help
# us clean up better
exec python -u /app/main/main.py

kill $pid