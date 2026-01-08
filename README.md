# USB Key Host Application (Docker-based)

This repository contains a Dockerized **host-side** application for the ELEC 498 USB Key project.
It implements the **partner software** that runs on the user's computer and communicates with a USB key
(microcontroller + fingerprint sensor ).

> Note: Cryptography here is intentionally **stubbed / demo-only**. Replace `usbkey/crypto.py`
> with your real RSA-based implementation.

## Features

- CLI application intended to be cross-platform via Docker (Linux container)
- Detect and connect to a USB key (serial JSON protocol)
- Enroll a fingerprint (command forwarded to device firmware)
- Authenticate fingerprint before allowing access to a file
- Validate input file format + enforce **max file size = 1 MB**
- Create a decrypted working copy under `/tmp/usb_key/...` and **purge on exit / signals / device removal** (best-effort)
- Optional **mock device** service for development without hardware

## Quick start (mock device)

```bash
docker compose up --build
```

In another terminal:

```bash
docker compose exec app python -m python -m usbkey status
docker compose exec app python -m python -m usbkey enroll
```

## CLI

```bash
python -m usbkey --help
python -m usbkey status
python -m usbkey enroll
python -m usbkey auth
python -m usbkey encrypt <path> --recipient-serial <SERIAL>
python -m usbkey decrypt <path.ukey>
```

## File format (demo container)

`*.ukey` files are **JSON header + payload**:

- First line: JSON metadata (recipient serial, original filename, algorithm label)
- Remaining bytes: demo payload (base64 of plaintext)

Replace this with your real RSA envelope format.

## Project layout

- `usbkey/` — application package
- `usbkey/device/` — device communication layer (serial + mock)
- `usbkey/workflows.py` — high-level flows (encrypt/decrypt/auth + cleanup)
- `usbkey/secure_tmp.py` — temp file handling + deletion
- `mock_device/` — mock USB key server used by docker-compose
- `tests/` — pytest unit tests
