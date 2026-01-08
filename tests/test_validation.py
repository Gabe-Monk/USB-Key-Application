from pathlib import Path
import pytest
from usbkey.workflows import validate_input_file, ValidationError

def test_validate_size(tmp_path: Path):
    p = tmp_path / "big.bin"
    p.write_bytes(b"0" * 10)
    validate_input_file(p, max_bytes=10)
    with pytest.raises(ValidationError):
        validate_input_file(p, max_bytes=9)
