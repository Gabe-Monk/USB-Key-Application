from __future__ import annotations

import base64

class CryptoNotImplementedError(NotImplementedError):
    pass

def demo_encrypt_bytes(plaintext: bytes) -> bytes:
    """Demo-only encryption (NOT secure)."""
    return base64.b64encode(plaintext)

def demo_decrypt_bytes(ciphertext: bytes) -> bytes:
    """Demo-only decryption (NOT secure)."""
    return base64.b64decode(ciphertext)

# --- Real crypto hooks (TODO) ---

def encrypt_for_recipient(*args, **kwargs) -> bytes:
    """Replace with RSA envelope encryption."""
    raise CryptoNotImplementedError(
        "Real encryption not implemented. Replace usbkey/crypto.py with your RSA implementation."
    )

def decrypt_with_secret(*args, **kwargs) -> bytes:
    """Replace with RSA envelope decryption."""
    raise CryptoNotImplementedError(
        "Real decryption not implemented. Replace usbkey/crypto.py with your RSA implementation."
    )
