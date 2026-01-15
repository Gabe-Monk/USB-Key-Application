import zmq
import json
import time
import base64
import secrets

def main():
    context = zmq.Context()
    socket = context.socket(zmq.REP)
    socket.bind("tcp://*:5555")

    print("ZMQ Mock Device running on tcp://*:5555")
    print("Press Ctrl+C to stop.")

    # Device State
    state = {
        "serial": "ZMQ-MOCK-01",
        "firmware": "1.0-zmq",
        "fingerprint_enrolled": False,
        "secret_key": secrets.token_bytes(32)
    }

    try:
        while True:
            # 1. Receive Request
            msg = socket.recv_json()
            print(f"Received: {msg}")

            req_id = msg.get("req_id")
            cmd = msg.get("cmd")
            data = msg.get("data", {})

            response_data = {}
            ok = True
            error = None

            # 2. Process Command
            if cmd == "STATUS":
                response_data = {
                    "serial": state["serial"],
                    "firmware": state["firmware"],
                    "fingerprint_enrolled": state["fingerprint_enrolled"]
                }
            
            elif cmd == "ENROLL_FINGERPRINT":
                print(">>> Simulating enrollment... (sleeping 2s)")
                time.sleep(2)
                state["fingerprint_enrolled"] = True
            
            elif cmd == "AUTH_FINGERPRINT":
                print(">>> Simulating auth... (sleeping 1s)")
                time.sleep(1)
                if state["fingerprint_enrolled"]:
                    response_data = {"accepted": True, "elapsed_ms": 1000}
                else:
                    response_data = {"accepted": False, "reason": "No fingerprint enrolled", "elapsed_ms": 1000}

            elif cmd == "GET_SECRET_KEY":
                b64_key = base64.b64encode(state["secret_key"]).decode("utf-8")
                response_data = {"secret_key_b64": b64_key}

            else:
                ok = False
                error = f"Unknown command: {cmd}"

            # 3. Send Response
            reply = {
                "req_id": req_id,
                "ok": ok,
                "data": response_data,
                "error": error
            }
            socket.send_json(reply)
            print(f"Sent: {reply}\n")

    except KeyboardInterrupt:
        print("\nStopping...")
    finally:
        socket.close()
        context.term()

if __name__ == "__main__":
    main()
