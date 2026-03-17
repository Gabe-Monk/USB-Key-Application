import communication
import files
import os
import signal
import subprocess

def authenticateFingerprint():
    '''Returns `True` if authentication worked, else `False`'''

    print("Please scan your finger now...")
    resp = communication.send_command("AUTH_FINGERPRINT")
    if resp and resp.get("ok"):
        print("Fingerprint accepted")
        return True
    elif resp:
        data = resp.get("data", {})
        print(f"Fingerprint rejected. Error: {data.get('error')}") # TODO: Make sure this error thing actually does anything
        return False
    
    print("Fingerprint rejected. No response from USB key")
    return False

def handleTermination(sig, frame):
    print("\nReceived termination signal. Cleaning up...")

    if communication.current_decrypted_file is not None:
        if os.path.exists(communication.current_decrypted_file):
            os.remove(communication.current_decrypted_file)
            print(f"Deleted temporary, decrypted copy of file ({communication.current_decrypted_file})")
        else:
            print(f"Couldn't find file '{communication.current_decrypted_file}' to delete")
    exit(0)

# Register listener for SIGTERM/SIGINTs
signal.signal(signal.SIGTERM, handleTermination)
signal.signal(signal.SIGINT, handleTermination)

# Wait until communication link with main program is established
communication.wait_until_up()

# Start the security watchdog
communication.start_watchdog()

# ---------------------------------------------------------
# MENU LOOP
# ---------------------------------------------------------
while True:
    print("\n--- MENU ---")
    print("1. Check Status")
    print("2. Enroll Fingerprint")
    print("3. Authenticate (Login)")
    print("4. Encrypt a File")
    print("5. Decrypt a File")
    print("6. Exit")
    
    choice = input("Select an option: ")
    
    if choice == "1": # Check Status
        resp = communication.send_command("GET_STATUS")
        if resp:
            data = resp.get("data", {})
            print(f"Device Serial: {data.get('serial')}")
            print(f"Device Firmware: {data.get('firmware')}")
            print(f"Authenticated: {data.get('authenticated')}")
            continue
        else:
            print("Device not responding")
            continue

    elif choice == "2": # Enroll Fingerprint
        print("Starting enrollment... Put your finger on the sensor.")
        resp = communication.send_command("ENROLL_FINGERPRINT")
        if resp and resp.get("ok"):
            print("Success! Fingerprint saved")
            continue
        else:
            print("Enrollment failed")
            continue

    elif choice == "3": # Authenticate (Login)
        authenticateFingerprint()
        continue

    elif choice == "4": # Encrypt a File
        fname = input("Enter filename to encrypt: ")
        target_sn = int(input("Enter serial number of target decryptor device: "))
        files.encrypt_file(fname, target_sn)
        continue

    elif choice == "5": # Decrypt a File
        fname = input("Enter .ukey filename to decrypt: ")

        # Get serial number of connected device
        resp = communication.send_command("GET_STATUS")
        if resp:
            data = resp.get("data", {})
            device_sn = int(data.get('serial'))
            authenticated = str(data.get("authenticated", "false")).lower() == "true"
        else:
            print("Device not responding")
            continue

        if not authenticated:
            print("User not yet authenticated via biometrics. Please authenticate before attempting decryption")
            authenticated = authenticateFingerprint()
        
            if not authenticated:
                print("Authentication required for decryption failed. Please try again")
                continue
        
        if authenticated: # Not an `else` here because we modify this value in the above `if not` block
            print("User authenticated. Proceeding with decryption")
            
            secret_file = files.decrypt_file(fname, device_sn)
            
            # Only proceed if a file was actually created
            if secret_file:
                # Open in Nano if .txt, otherwise, just create file (not uesful to open other types like pdf in text editors)
                if secret_file.lower().endswith(".txt"):
                    print(f"Opening {secret_file} in read-only mode...")
                    try:
                        # The '-v' flag opens nano in 'view' mode (read-only)
                        subprocess.run(["nano", "-v", secret_file])
                    except Exception as e:
                        print(f"Failed to open file viewer: {e}")
                    except KeyboardInterrupt:
                        # If we get a Ctrl+C here when file still exists, make sure we delete it
                        handleTermination(None, None)
                else:
                    try:
                        input(f"File '{secret_file}' is ready. Press ENTER to delete it")
                    except KeyboardInterrupt:
                        # If we get a Ctrl+C here when file still exists, make sure we delete it
                        handleTermination(None, None)
                    
                
                # Cleanup
                if os.path.exists(secret_file):
                    os.remove(secret_file)
                    print("File deleted")
                    communication.current_decrypted_file = None
                    continue
                else:
                    print(f"Couldn't find file '{secret_file}' to delete")
                    continue
            else:
                print(f"Failed to find target file '{fname}'")
                continue

    elif choice == "6": # Exit
        print("Exiting...")
        communication.running = False # Stop the watchdog
        break

    else:
        print("Invalid choice.")