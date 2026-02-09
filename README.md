# USB Key Project (ELEC 498)

## How to Run It

### Wiring
A wiring diagram for this project is available in `Schematic.pdf`.

![wiring diagram](imgs/Schematic.png)

### Push Firmware to USB Key
1. Install necessary firmwares and libraries by following the instructions in `firmware/README.md`
1. Upload all of the `.py` files in the `firmware` directory to the USB key's root file directory
1. Disconnect the USB key

### Configure Port Forwarding (Windows & WSL2)
1. Powershell: `winget install usbipd-win`
1. Close that Powershell terminal and open a new one (as admin)
1. Do `usbipd list` in Powershell with the device disconnected
1. Plug the device into the USB port you plan to use it with and do `usbipd list` again, seeing which entry is there now that wasn't before. Take note of its BUSID (e.g., "1-2")
1. In Powershell, do `usbipd bind --force --busid <YOUR BUSID>`, then do `usbipd attach --wsl --busid <YOUR BUSID> --auto-attach` and leave that Powershell terminal running
1. Unplug the USB key again
1. When done running the program (from another terminal following later steps), kill the Powershell process and then do `usbipd unbind --busid <YOUR BUSID>` (in Powershell)

### Compile and Run Client-Side Program
1. Install Docker engine. If you're using Ubuntu on WSL2, you can do so by following [these instructions](https://docs.docker.com/engine/install/ubuntu/)
1. Build the Docker image via `docker compose build`
1. Run the application via `docker compose run --rm app`

## What Each File Does
Here is a quick explanation of the files so you know where everything is:

  1. `main/main.py`:
  This is the main application file. It handles the "Handshake" and shows the menu loop (Press 1 for Status, Press 2 for Enroll, etc.).

  1. `main/communication.py`:
  This file handles all the ZMQ socket stuff. It sends JSON commands and waits for answers.
  It also runs a Watchdog in the background. This is a separate thread that keeps checking if the device is plugged in. If the device disappears, it deletes the decrypted files.

  1. `main/files.py`:
  This file is supposed to handle reading, encrypting, and decrypting files. \
  *Note: Right now, the encryption functions are empty (placeholders). I put comments in there to remind myself to add the real encryption logic later once Aahash's code is ready.*

  1. `build/Dockerfile`:
  Encapsulates the application.

  1. `docker-compose.yml`:
  Used to run the dockerfile. The network_mode: "host" so the code inside the container can talk to the device

  1. `build/requirements.txt`:
  This is just a list of libraries required, currently only installs pyzmq

## To-Do List

[ ] Add real encryption code in files.py.

[ ] Test with the real hardware.
