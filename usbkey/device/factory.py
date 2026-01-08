from __future__ import annotations

import os
from usbkey.config import Settings
from usbkey.device.base import UsbKeyDevice
from usbkey.device.discovery import guess_serial_ports

def make_device(settings: Settings) -> UsbKeyDevice:
    if settings.use_mock:
        from usbkey.device.mock_device_client import MockUsbKeyDevice
        return MockUsbKeyDevice(settings.device_url)

    from usbkey.device.serial_device import SerialUsbKeyDevice

    port = settings.serial_port
    if not os.path.exists(port) and not port.upper().startswith("COM"):
        guesses = guess_serial_ports()
        if guesses:
            port = guesses[0]
    return SerialUsbKeyDevice(port, settings.serial_baud)
