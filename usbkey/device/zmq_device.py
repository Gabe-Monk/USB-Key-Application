from __future__ import annotations

import base64
import json
import zmq
import uuid
from typing import Any, Dict

from usbkey.models import DeviceStatus, AuthResult
from usbkey.device.base import UsbKeyDevice

class ZmqDeviceError(RuntimeError):
    pass

class ZmqUsbKeyDevice(UsbKeyDevice):
    """
    Device implementation that communicates via ZeroMQ REQ/REP.
    Compatible with the colins_stuff/zmqTest.cpp if it implements the JSON protocol,
    or the Python mock server.
    """
    def __init__(self, address: str, timeout_ms: int = 2000):
        self.address = address
        self.timeout_ms = timeout_ms
        self.context: zmq.Context | None = None
        self.socket: zmq.Socket | None = None

    def connect(self) -> None:
        if self.socket:
            return
        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.REQ)
        # Set receive timeout to avoid hanging forever
        self.socket.setsockopt(zmq.RCVTIMEO, self.timeout_ms)
        self.socket.connect(self.address)

    def close(self) -> None:
        if self.socket:
            self.socket.close()
            self.socket = None
        if self.context:
            self.context.term()
            self.context = None

    def _rpc(self, cmd: str, data: Dict[str, Any] | None = None) -> Dict[str, Any]:
        if not self.socket:
            raise ZmqDeviceError("ZMQ device not connected")

        req_id = str(uuid.uuid4())
        payload = {"req_id": req_id, "cmd": cmd, "data": data or {}}
        
        try:
            self.socket.send_json(payload)
            response = self.socket.recv_json()
        except zmq.ZMQError as e:
            raise ZmqDeviceError(f"ZMQ communication failed: {e}")

        # Basic protocol validation
        if not isinstance(response, dict):
             raise ZmqDeviceError("Invalid response format")
             
        if response.get("req_id") != req_id:
             # In a strict REQ/REP pattern this shouldn't happen, but good to check
             raise ZmqDeviceError("Request ID mismatch")

        if not response.get("ok", False):
            raise ZmqDeviceError(response.get("error") or "Device returned ok=false")

        return response.get("data") or {}

    def status(self) -> DeviceStatus:
        data = self._rpc("STATUS")
        return DeviceStatus(**data)

    def enroll_fingerprint(self) -> None:
        # Increase timeout for enrollment if necessary
        old_timeout = self.socket.getsockopt(zmq.RCVTIMEO)
        self.socket.setsockopt(zmq.RCVTIMEO, 30000) # 30s
        try:
            self._rpc("ENROLL_FINGERPRINT")
        finally:
            self.socket.setsockopt(zmq.RCVTIMEO, old_timeout)

    def authenticate(self, timeout_s: float = 10.0) -> AuthResult:
        old_timeout = self.socket.getsockopt(zmq.RCVTIMEO)
        self.socket.setsockopt(zmq.RCVTIMEO, int(timeout_s * 1000) + 2000)
        try:
            data = self._rpc("AUTH_FINGERPRINT", data={"timeout_s": timeout_s})
            return AuthResult(**data)
        finally:
            self.socket.setsockopt(zmq.RCVTIMEO, old_timeout)

    def get_secret_key(self) -> bytes:
        data = self._rpc("GET_SECRET_KEY")
        b64 = data.get("secret_key_b64")
        if not isinstance(b64, str):
            raise ZmqDeviceError("Malformed GET_SECRET_KEY response")
        return base64.b64decode(b64.encode("utf-8"))
