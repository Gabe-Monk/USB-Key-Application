from __future__ import annotations

from abc import ABC, abstractmethod
from usbkey.models import DeviceStatus, AuthResult

class UsbKeyDevice(ABC):
    @abstractmethod
    def connect(self) -> None:
        ...

    @abstractmethod
    def close(self) -> None:
        ...

    @abstractmethod
    def status(self) -> DeviceStatus:
        ...

    @abstractmethod
    def enroll_fingerprint(self) -> None:
        ...

    @abstractmethod
    def authenticate(self, timeout_s: float = 10.0) -> AuthResult:
        ...

    @abstractmethod
    def get_secret_key(self) -> bytes:
        ...
