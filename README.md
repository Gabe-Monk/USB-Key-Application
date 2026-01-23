# USB Key Project (ELEC 498)


## How to Run It

1. Install Docker engine. If you're using Ubuntu on WSL2, you can do so by following [these instructions](https://docs.docker.com/engine/install/ubuntu/)
2. Build the Docker image via `docker compose build`
3. Run the application via `docker compose run --rm app`

If you want to have a simulated device to communicate with, run the following in another terminal before running this application

```bash
cd colins_stuff
docker build -t 498_colin .
docker run -it --rm \
  -p 5555:5555 \
  --privileged \
  -v "$(pwd):/workspace" \
  -w /workspace \
  498_colin \
  bash -c "g++ gabes_zmqTest.cpp -o gabes_zmqTest -lzmq && ./gabes_zmqTest"
```

## What Each File Does
Here is a quick explanation of the files so you know where everything is:

1. `main.py`:
This is the main application file. It handles the "Handshake" and shows the menu loop (Press 1 for Status, Press 2 for Enroll, etc.).
2. `communication.py`:
This file handles all the ZMQ socket stuff. It sends JSON commands and waits for answers.
It also runs a Watchdog in the background. This is a separate thread that keeps checking if the device is plugged in. If the device disappears, it deletes the decrypted files.

3. `files.py`:
This file is supposed to handle reading, encrypting, and decrypting files. \
*Note: Right now, the encryption functions are empty (placeholders). I put comments in there to remind myself to add the real encryption logic later once Aahash's code is ready.*

4. `Dockerfile`:
Encapsulates the application.

5. `docker-compose.yml`:
Used to run the dockerfile. The network_mode: "host" so the code inside the container can talk to the device

6. `requirements.txt`:
This is just a list of libraries required, currently only installs pyzmq

## To-Do List

[ ] Add real encryption code in files.py.

[ ] Test with the real hardware.
