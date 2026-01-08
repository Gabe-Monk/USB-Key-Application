from __future__ import annotations

import base64
import os
import time
import secrets
from flask import Flask, jsonify, request

app = Flask(__name__)

SERIAL = os.getenv("MOCK_SERIAL", "MOCK-0001")
FIRMWARE = os.getenv("MOCK_FW", "0.1-mock")
fingerprint_enrolled = False
secret_key = secrets.token_bytes(32)

@app.get("/status")
def status():
    return jsonify({
        "serial": SERIAL,
        "firmware": FIRMWARE,
        "fingerprint_enrolled": fingerprint_enrolled,
    })

@app.post("/enroll")
def enroll():
    global fingerprint_enrolled
    time.sleep(0.5)
    fingerprint_enrolled = True
    return jsonify({"ok": True})

@app.post("/auth")
def auth():
    global fingerprint_enrolled
    start = time.time()
    time.sleep(0.35)

    if not fingerprint_enrolled:
        return jsonify({"accepted": False, "reason": "no fingerprint enrolled", "elapsed_ms": int((time.time()-start)*1000)})

    return jsonify({"accepted": True, "reason": None, "elapsed_ms": int((time.time()-start)*1000)})

@app.get("/secret")
def secret():
    return jsonify({"secret_key_b64": base64.b64encode(secret_key).decode("utf-8")})

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8765)
