from __future__ import annotations

import os
import shutil
import subprocess
from pathlib import Path

def ensure_dir(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)

def write_private_file(path: Path, data: bytes) -> None:
    ensure_dir(path.parent)
    flags = os.O_WRONLY | os.O_CREAT | os.O_TRUNC
    fd = os.open(str(path), flags, 0o600)
    try:
        with os.fdopen(fd, "wb") as f:
            f.write(data)
            f.flush()
            os.fsync(f.fileno())
    finally:
        try:
            os.close(fd)
        except OSError:
            pass

def best_effort_secure_delete(path: Path) -> None:
    """
    Best-effort delete:
    - Prefer shred if available (Linux).
    - Fall back to unlink.
    Note: secure deletion is not guaranteed on SSDs / journaling FS.
    """
    if not path.exists():
        return

    shred = shutil.which("shred")
    if shred:
        try:
            subprocess.run([shred, "-u", "-z", "-n", "3", str(path)], check=False)
            return
        except Exception:
            pass

    try:
        path.unlink(missing_ok=True)
    except Exception:
        # last resort: overwrite with zeros in Python then unlink
        try:
            size = path.stat().st_size
            with open(path, "r+b") as f:
                f.write(b"\x00" * min(size, 1024 * 1024))
                f.flush()
                os.fsync(f.fileno())
        except Exception:
            pass
        try:
            path.unlink(missing_ok=True)
        except Exception:
            pass

class TempDecryptedFile:
    def __init__(self, root: Path, filename: str):
        self.root = root
        self.filename = filename
        self.path = root / "decrypted_file" / filename

    def create(self, data: bytes) -> Path:
        write_private_file(self.path, data)
        return self.path

    def cleanup(self) -> None:
        best_effort_secure_delete(self.path)
