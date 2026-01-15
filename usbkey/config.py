from __future__ import annotations

from dataclasses import dataclass
import os

def _env_int(name: str, default: int) -> int:
    v = os.getenv(name)
    if v is None:
        return default
    try:
        return int(v)
    except ValueError:
        raise ValueError(f"Env {name} must be int, got: {v!r}")

def _env_bool(name: str, default: bool) -> bool:
    v = os.getenv(name)
    if v is None:
        return default
    return v.strip().lower() in {"1", "true", "yes", "y", "on"}

@dataclass(frozen=True)
class Settings:
    use_mock: bool = _env_bool("USBKEY_USE_MOCK", True)
    # New flag to enable ZMQ
    use_zmq: bool = _env_bool("USBKEY_USE_ZMQ", False)
    # Address for ZMQ (defaults to localhost for testing)
    zmq_address: str = os.getenv("USBKEY_ZMQ_ADDRESS", "tcp://localhost:5555")
    
    device_url: str = os.getenv("USBKEY_DEVICE_URL", "http://localhost:8765")
    serial_port: str = os.getenv("USBKEY_PORT", "/dev/ttyACM0")
    serial_baud: int = _env_int("USBKEY_BAUD", 115200)
    file_max_bytes: int = _env_int("USBKEY_FILE_MAX_BYTES", 1_048_576)  # 1 MB
    temp_root: str = os.getenv("USBKEY_TEMP_ROOT", "/tmp/usb_key")
