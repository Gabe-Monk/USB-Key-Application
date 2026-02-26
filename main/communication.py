import zmq
import json
import time
import threading
import os

# ---------------------------------------------------------
# SETUP ZMQ
# ---------------------------------------------------------
# print("Setting up connection...")
context = zmq.Context()
socket = context.socket(zmq.REQ)
# Increase timeout to 5 seconds to match the C++ simulator
socket.setsockopt(zmq.RCVTIMEO, 5000) 

# print("Connecting to hardware interface on port 5555...")
socket.connect("tcp://localhost:5555")

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
        "req_id": 123, 
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
        socket.connect("tcp://localhost:5555")
        
        return None

def wait_until_up():
    """
    Pings hw_comms submodule until it gets a response. To be used during init
    """

    # We need to use 'global' so we can replace the broken socket
    global socket, context 

    # print("Waiting for hw_comms submodule to be up...")

    msg = {
        "req_id": 123, 
        "cmd": "WD_HEARTBEAT", 
        "data": None
    }

    json_str = json.dumps(msg)

    response = False
    
    while not response:
        try:
            # 1. Try to send
            socket.send_string(json_str)
            
            # 2. Try to receive
            socket.recv_string()

            response = True
        except Exception as e:
            # Close the broken socket
            socket.close(linger=0)
            
            # Create a brand new one
            socket = context.socket(zmq.REQ)
            socket.setsockopt(zmq.RCVTIMEO, 5000) # Remember the 5s timeout!
            socket.connect("tcp://localhost:5555")

            time.sleep(0.1)
            response = False
    
    # print("Done waiting for hw_comms submodule...")


def start_watchdog():
    t = threading.Thread(target=_watchdog_loop)
    t.daemon = True
    t.start()

def _watchdog_loop():
    global current_decrypted_file
    
    while running:
        time.sleep(1)
        
        try:
            resp = send_command("WD_HEARTBEAT")
            if not resp.get("device_connected"):
                raise AssertionError("Device disconnected")
        except:
            # If watchdog fails, assume device is gone
            if current_decrypted_file is not None:
                print("\n\n[SECURITY ALERT] DEVICE DISCONNECTED! DELETING FILES!")
                # if os.path.exists(current_decrypted_file):
                #     os.remove(current_decrypted_file)
                #     print(f"Deleted {current_decrypted_file}")
                #     current_decrypted_file = None