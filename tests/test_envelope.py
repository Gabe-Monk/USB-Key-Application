from pathlib import Path
from usbkey.envelope import EnvelopeHeader, write_demo_envelope, read_demo_envelope

def test_roundtrip(tmp_path: Path):
    header = EnvelopeHeader(recipient_serial="ABC", original_filename="x.txt")
    payload = b"hello"
    p = tmp_path / "a.ukey"
    write_demo_envelope(p, header, payload)
    h2, pl2 = read_demo_envelope(p)
    assert h2.recipient_serial == "ABC"
    assert pl2 == payload
