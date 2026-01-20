import zmq
import json
import time
import threading
import os

# ---------------------------------------------------------
# SETUP ZMQ
# ---------------------------------------------------------
print("Setting up connection...")
context = zmq.Context()
socket = context.socket(zmq.REQ)
# Increase timeout to 5 seconds to match the C++ simulator
socket.setsockopt(zmq.RCVTIMEO, 5000) 

print("Connecting to host computer port 5555...")
socket.connect("tcp://host.docker.internal:5555")

current_decrypted_file = None
running = True

# ---------------------------------------------------------
# FUNCTIONS
# ---------------------------------------------------------

def send_command(cmd, data=None):
    """
    Sends a command. If it fails, it resets the connection 
    so the next command doesn't crash.
    """
    # We need to use 'global' so we can replace the broken socket
    global socket, context 

    if data is None:
        data = {}
    
    msg = {
        "req_id": "student-request-1", 
        "cmd": cmd, 
        "data": data
    }
    
    json_str = json.dumps(msg)
    
    try:
        # 1. Try to send
        socket.send_string(json_str)
        
        # 2. Try to receive
        reply_str = socket.recv_string()
        return json.loads(reply_str)

    except Exception as e:
        # 3. IF ANYTHING GOES WRONG (Timeout, Error, etc.)
        print(f"\n[!] Error: {e}")
        print("[-] Resetting the connection (Hanging up and redialing)...")
        
        # Close the broken socket
        socket.close(linger=0)
        
        # Create a brand new one
        socket = context.socket(zmq.REQ)
        socket.setsockopt(zmq.RCVTIMEO, 5000) # Remember the 5s timeout!
        socket.connect("tcp://host.docker.internal:5555")
        
        return None

def start_watchdog():
    t = threading.Thread(target=_watchdog_loop)
    t.daemon = True
    t.start()

def _watchdog_loop():
    global current_decrypted_file
    
    while running:
        time.sleep(1)
        
        # Watchdog needs its own private socket
        temp_socket = context.socket(zmq.REQ)
        temp_socket.setsockopt(zmq.RCVTIMEO, 1000)
        temp_socket.connect("tcp://host.docker.internal:5555")
        
        try:
            temp_socket.send_string(json.dumps({"req_id": "wd", "cmd": "STATUS", "data": {}}))
            temp_socket.recv_string()
        except:
            # If watchdog fails, assume device is gone
            if current_decrypted_file is not None:
                print("\n\n[SECURITY ALERT] DEVICE DISCONNECTED! DELETING FILES!")
                if os.path.exists(current_decrypted_file):
                    os.remove(current_decrypted_file)
                    print(f"Deleted {current_decrypted_file}")
                    current_decrypted_file = None
        
        # Always close the temporary socket
        temp_socket.close()