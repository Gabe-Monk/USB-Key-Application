from __future__ import annotations

from pathlib import Path
from typing import Tuple

from usbkey.models import EnvelopeHeader

class EnvelopeError(ValueError):
    pass

def write_demo_envelope(out_path: Path, header: EnvelopeHeader, payload: bytes) -> None:
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("wb") as f:
        f.write((header.model_dump_json() + "\n").encode("utf-8"))
        f.write(payload)

def read_demo_envelope(path: Path) -> Tuple[EnvelopeHeader, bytes]:
    with path.open("rb") as f:
        first_line = f.readline()
        if not first_line:
            raise EnvelopeError("Empty file")
        try:
            header = EnvelopeHeader.model_validate_json(first_line.decode("utf-8"))
        except Exception as e:
            raise EnvelopeError(f"Invalid envelope header: {e}") from e
        payload = f.read()
    return header, payload
