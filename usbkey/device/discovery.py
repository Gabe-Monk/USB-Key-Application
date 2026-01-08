from __future__ import annotations

def guess_serial_ports() -> list[str]:
    """Return candidate serial ports that look like USB ACM/USB serial devices."""
    try:
        from serial.tools import list_ports  # type: ignore
    except Exception:
        return []

    ports: list[str] = []
    for p in list_ports.comports():
        dev = p.device
        if "ttyACM" in dev or "ttyUSB" in dev or dev.upper().startswith("COM"):
            ports.append(dev)
    return ports
