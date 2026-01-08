from __future__ import annotations

from pydantic import BaseModel, Field
from typing import Any, Dict, Optional

class DeviceStatus(BaseModel):
    serial: str
    firmware: str | None = None
    fingerprint_enrolled: bool = False

class EnvelopeHeader(BaseModel):
    """
    Demo container header (first line JSON).
    Replace with your real RSA envelope format.
    """
    version: str = "0.1"
    recipient_serial: str
    original_filename: str
    alg: str = "DEMO-PLAINTEXT-B64"
    meta: Dict[str, Any] = Field(default_factory=dict)

class AuthResult(BaseModel):
    accepted: bool
    reason: Optional[str] = None
    elapsed_ms: Optional[int] = None
