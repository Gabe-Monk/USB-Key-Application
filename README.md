# USB Key Host Application

This repository contains the **host-side partner software** for the ELEC 498 USB Key project. It is a Python-based CLI application designed to communicate with a secure USB hardware token (microcontroller + fingerprint sensor) to authenticate users before granting access to encrypted files.

## Capabilities

### Core Features
* **Biometric Authentication**: Enrolls and authenticates fingerprints via the connected hardware device.
* **Secure File Access**:
    * **Encrypt**: Wraps files in a secure envelope (currently demo/stubbed crypto) targeted to a specific device serial number.
    * **Decrypt**: Authenticates the user via the device, retrieves a secret key, and decrypts the file to a secure temporary location.
* **Automatic Cleanup**: Decrypted files are stored in `tmpfs` (on Linux) or a temporary folder and are **automatically purged** when the application exits, the device is disconnected, or the process is killed.
* **Dockerized**: Fully containerized environment for consistent cross-platform execution.

### Device Communication Modes
The application supports three distinct communication backends:
1.  **Serial (UART)**: Communicates directly with physical hardware via USB serial ports (default).
2.  **ZeroMQ (ZMQ)**: Communicates over TCP/IP networks. Useful for:
    * Interfacing with C++ simulators (like `colins_stuff`).
    * Connecting to network-attached hardware or emulators.
    * Testing the "Client" logic separately from the physical hardware.
3.  **HTTP Mock**: A pure-Python mock server for development when no hardware or C++ simulator is available.

---

## ZeroMQ (ZMQ) Features

The application includes a ZMQ driver that functions as a **Request (REQ)** client. This allows the host app to send JSON commands to any **Reply (REP)** server compatible with the project protocol.

### Protocol Compatibility
To use the ZMQ backend, your device (or simulator) must:
1.  Listen on a ZMQ `REP` socket (e.g., `tcp://*:5555`).
2.  Accept JSON requests in the format: `{"req_id": "...", "cmd": "STATUS", "data": {}}`.
3.  Reply with JSON responses: `{"req_id": "...", "ok": true, "data": {...}}`.

### Testing with ZMQ
You can test this mode without hardware using the provided mock scripts:
* **Python Mock**: `python tests/zmq_mock_server.py`
* **C++ Simulator**: See `colins_stuff/` for the C++ implementation.

---

## Configuration & Environment Variables

The application is configured entirely via environment variables.

| Variable | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| **Backend Selection** | | | |
| `USBKEY_USE_ZMQ` | Bool | `False` | **New**: Set to `true` to enable ZeroMQ mode. Takes precedence over Mock/Serial. |
| `USBKEY_USE_MOCK` | Bool | `True` | Set to `true` to use the internal HTTP mock. Set to `false` for real Serial or ZMQ. |
| **ZMQ Config** | | | |
| `USBKEY_ZMQ_ADDRESS` | String | `tcp://localhost:5555` | **New**: The full ZMQ address of the target device/simulator. |
| **Serial Config** | | | |
| `USBKEY_PORT` | String | `/dev/ttyACM0` | The serial port for physical devices. |
| `USBKEY_BAUD` | Int | `115200` | Baud rate for serial communication. |
| **Mock Config** | | | |
| `USBKEY_DEVICE_URL` | String | `http://localhost:8765` | URL of the internal HTTP mock server. |
| **Application Config** | | | |
| `USBKEY_FILE_MAX_BYTES`| Int | `1048576` | Max file size allowed for encryption (default 1 MB). |
| `USBKEY_TEMP_ROOT` | String | `/tmp/usb_key` | Location for temporary decrypted files. |
| `USBKEY_LOG_LEVEL` | String | `INFO` | Logging verbosity (`DEBUG`, `INFO`, `WARNING`, `ERROR`). |

---

## Quick Start: Running with a Real ZMQ Device

Follow these steps to connect the application to a real physical device (or a C++ simulator running on another machine) via ZeroMQ.

### 1. Prepare the Device
Ensure your device or C++ application is running and listening on a reachable IP address.
* **Example**: Your Raspberry Pi/Controller is at `192.168.1.50` and listening on port `5555`.

### 2. Configure the Host App
Set the environment variables to disable the mock and enable ZMQ pointing to your device.

**Mac / Linux:**
export USBKEY_USE_ZMQ=true
export USBKEY_USE_MOCK=false
export USBKEY_ZMQ_ADDRESS="tcp://192.168.1.50:5555"

**Windows (PowerShell):**
$env:USBKEY_USE_ZMQ="true"
$env:USBKEY_USE_MOCK="false"
$env:USBKEY_ZMQ_ADDRESS="tcp://192.168.1.50:5555"

### 3. Run Commands
Now run the CLI commands as normal. They will communicate over the network to your device.

**CLI Command Reference**
**All commands are run via python -m usbkey <COMMAND>. Below is a full list of available commands and their usage.**

status - Check the connection and status of the USB key.
python -m usbkey status
Output: Shows Device Serial, Firmware Version, and Fingerprint Enrollment status.

enroll - Start the fingerprint enrollment process on the device.
python -m usbkey enroll
Usage: Follow the interactive prompts (LEDs/Sensor instructions on the device).

auth - Test fingerprint authentication without decrypting a file.
python -m usbkey auth
Output: "Accepted" or "Rejected" with timing details.

encrypt - Create a secure envelope for a specific recipient serial number.
python -m usbkey encrypt <FILE_PATH> --recipient-serial <SERIAL>
Arguments:
path: Path to the input file (max 1 MB).
--recipient-serial, -r: The serial number of the USB key that can decrypt this file.
Output: Generates a .ukey file in the same directory.

decrypt - Decrypt a secure envelope (.ukey) using the connected device.
python -m usbkey decrypt <FILE_PATH.ukey>
Usage: Prompts for fingerprint authentication. If successful, creates a decrypted copy in a secure temp folder.
Behavior: The program waits for you to press Enter. Once you press Enter (or if the program closes), the decrypted file is permanently deleted.

watch - Continuously poll the device status (useful for testing connection stability).
python -m usbkey watch --interval-s 2.0
Arguments:
--interval-s: Polling interval in seconds (default: 1.0).

**Development**
Running via Docker
The easiest way to run the full stack (App + Mock) is via Docker Compose:
docker compose up --build

To run commands inside the container:
docker compose exec app python -m usbkey status

**Manual Setup (Local)**
Install dependencies:
pip install -r requirements.txt
Run the app:
python -m usbkey --help