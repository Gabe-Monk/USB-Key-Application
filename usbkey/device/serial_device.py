from __future__ import annotations

import base64
import json
import time
import uuid
from typing import Any, Dict

import serial  # type: ignore

from usbkey.models import DeviceStatus, AuthResult
from usbkey.device.base import UsbKeyDevice

class SerialDeviceError(RuntimeError):
    pass

class SerialUsbKeyDevice(UsbKeyDevice):
    def __init__(self, port: str, baud: int = 115200, timeout_s: float = 2.0):
        self.port = port
        self.baud = baud
        self.timeout_s = timeout_s
        self.ser: serial.Serial | None = None

    def connect(self) -> None:
        if self.ser and self.ser.is_open:
            return
        self.ser = serial.Serial(self.port, self.baud, timeout=self.timeout_s)

    def close(self) -> None:
        if self.ser:
            try:
                self.ser.close()
            except Exception:
                pass
        self.ser = None

    def _rpc(self, cmd: str, data: Dict[str, Any] | None = None, timeout_s: float = 5.0) -> Dict[str, Any]:
        if not self.ser or not self.ser.is_open:
            raise SerialDeviceError("Serial device not connected")

        req_id = str(uuid.uuid4())
        payload = {"req_id": req_id, "cmd": cmd, "data": data or {}}
        line = (json.dumps(payload) + "\n").encode("utf-8")
        self.ser.write(line)
        self.ser.flush()

        deadline = time.time() + timeout_s
        while time.time() < deadline:
            raw = self.ser.readline()
            if not raw:
                continue
            try:
                msg = json.loads(raw.decode("utf-8", errors="replace").strip())
            except json.JSONDecodeError:
                continue
            if msg.get("req_id") != req_id:
                continue
            if not msg.get("ok", False):
                raise SerialDeviceError(msg.get("error") or "Device returned ok=false")
            return msg.get("data") or {}

        raise SerialDeviceError(f"Timeout waiting for response to {cmd}")

    def status(self) -> DeviceStatus:
        data = self._rpc("STATUS")
        return DeviceStatus(**data)

    def enroll_fingerprint(self) -> None:
        self._rpc("ENROLL_FINGERPRINT", timeout_s=30.0)

    def authenticate(self, timeout_s: float = 10.0) -> AuthResult:
        data = self._rpc("AUTH_FINGERPRINT", timeout_s=timeout_s)
        return AuthResult(**data)

    def get_secret_key(self) -> bytes:
        data = self._rpc("GET_SECRET_KEY")
        b64 = data.get("secret_key_b64")
        if not isinstance(b64, str):
            raise SerialDeviceError("Malformed GET_SECRET_KEY response")
        return base64.b64decode(b64.encode("utf-8"))
