from __future__ import annotations

import logging
import signal
import threading
import time
from pathlib import Path

from usbkey.config import Settings
from usbkey.crypto import demo_encrypt_bytes, demo_decrypt_bytes
from usbkey.device.base import UsbKeyDevice
from usbkey.envelope import EnvelopeHeader, write_demo_envelope, read_demo_envelope
from usbkey.secure_tmp import TempDecryptedFile
from usbkey.models import AuthResult

log = logging.getLogger("usbkey.workflows")

class ValidationError(ValueError):
    pass

def validate_input_file(path: Path, max_bytes: int) -> None:
    if not path.exists():
        raise ValidationError(f"File does not exist: {path}")
    size = path.stat().st_size
    if size > max_bytes:
        raise ValidationError(f"File too large ({size} bytes). Max allowed is {max_bytes} bytes.")

def encrypt_flow(settings: Settings, input_path: Path, recipient_serial: str) -> Path:
    validate_input_file(input_path, settings.file_max_bytes)
    plaintext = input_path.read_bytes()

    header = EnvelopeHeader(
        recipient_serial=recipient_serial,
        original_filename=input_path.name,
        alg="DEMO-PLAINTEXT-B64",
        meta={"note": "Demo-only. Replace with RSA envelope."},
    )
    payload = demo_encrypt_bytes(plaintext)
    out_path = input_path.with_suffix(input_path.suffix + ".ukey")
    write_demo_envelope(out_path, header, payload)
    return out_path

def _install_cleanup_handlers(cleanup_fn):
    def handler(signum, _frame):
        log.warning("Received signal %s, cleaning up...", signum)
        cleanup_fn()

    for sig in (signal.SIGINT, signal.SIGTERM):
        try:
            signal.signal(sig, handler)
        except Exception:
            pass

def decrypt_flow(settings: Settings, device: UsbKeyDevice, envelope_path: Path):
    validate_input_file(envelope_path, settings.file_max_bytes)
    if envelope_path.suffix != ".ukey":
        raise ValidationError("Invalid file format: expected a .ukey envelope (demo format).")

    header, payload = read_demo_envelope(envelope_path)

    device.connect()
    st = device.status()
    if st.serial != header.recipient_serial:
        device.close()
        raise ValidationError(
            f"Recipient mismatch: file is for serial {header.recipient_serial}, but device is {st.serial}."
        )

    auth: AuthResult = device.authenticate(timeout_s=15.0)
    if not auth.accepted:
        device.close()
        raise ValidationError(f"Fingerprint rejected: {auth.reason or 'unknown reason'}")

    # Secret key would normally be used to decrypt. We still request it to test the interface.
    _ = device.get_secret_key()

    plaintext = demo_decrypt_bytes(payload)

    temp_root = Path(settings.temp_root)
    tmp = TempDecryptedFile(temp_root, header.original_filename)
    out_path = tmp.create(plaintext)

    cleaned = {"done": False}

    def cleanup():
        if cleaned["done"]:
            return
        cleaned["done"] = True
        log.info("Purging decrypted file and closing device...")
        try:
            tmp.cleanup()
        finally:
            try:
                device.close()
            except Exception:
                pass

    _install_cleanup_handlers(cleanup)

    def watchdog():
        # Best-effort "device removal" detection by polling status.
        while not cleaned["done"]:
            try:
                device.status()
            except Exception:
                log.warning("Device no longer reachable; purging decrypted file.")
                cleanup()
                return
            time.sleep(0.75)

    t = threading.Thread(target=watchdog, daemon=True)
    t.start()

    return out_path, cleanup
