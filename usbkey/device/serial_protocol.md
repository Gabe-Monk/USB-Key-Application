# Serial JSON protocol (proposed)

This is a simple request/response protocol for host <-> Pico over UART/USB serial.

Each message is a single JSON object on one line (`\n` terminated).

## Request

```json
{"req_id":"<uuid>","cmd":"STATUS","data":{}}
```

## Response

```json
{"req_id":"<uuid>","ok":true,"data":{"serial":"ABC123","firmware":"0.1","fingerprint_enrolled":true}}
```

Commands used by the host app:
- `STATUS`
- `ENROLL_FINGERPRINT`
- `AUTH_FINGERPRINT` (returns accepted + elapsed_ms)
- `GET_SECRET_KEY` (returns base64 key bytes)

The real firmware can implement a different protocol; only the Python adapter would need to change.
