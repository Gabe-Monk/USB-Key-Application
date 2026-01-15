#!/bin/bash
# Navigate to the colins_stuff directory
cd "$(dirname "$0")"

# Run the container with:
# - Interactive terminal (-it)
# - Privileged mode for USB access (--privileged)
# - Mount the current directory to /workspace
# - Mount USB devices (/dev/ttyACM* and /dev/ttyUSB*)
# - Install dependencies, build, and run
docker run -it --rm \
  --privileged \
  -v "$(pwd):/workspace" \
  -w /workspace \
  498_colin \
  ./entrypoint.sh