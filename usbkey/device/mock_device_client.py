from __future__ import annotations

import base64
import requests

from usbkey.models import DeviceStatus, AuthResult
from usbkey.device.base import UsbKeyDevice

class MockUsbKeyDevice(UsbKeyDevice):
    """HTTP client for the docker-compose mock device."""

    def __init__(self, base_url: str):
        self.base_url = base_url.rstrip("/")

    def connect(self) -> None:
        return

    def close(self) -> None:
        return

    def status(self) -> DeviceStatus:
        r = requests.get(f"{self.base_url}/status", timeout=5)
        r.raise_for_status()
        return DeviceStatus(**r.json())

    def enroll_fingerprint(self) -> None:
        r = requests.post(f"{self.base_url}/enroll", timeout=30)
        r.raise_for_status()

    def authenticate(self, timeout_s: float = 10.0) -> AuthResult:
        r = requests.post(f"{self.base_url}/auth", json={"timeout_s": timeout_s}, timeout=timeout_s + 2)
        r.raise_for_status()
        return AuthResult(**r.json())

    def get_secret_key(self) -> bytes:
        r = requests.get(f"{self.base_url}/secret", timeout=5)
        r.raise_for_status()
        b64 = r.json()["secret_key_b64"]
        return base64.b64decode(b64.encode("utf-8"))
