import communication
import files
import os

# Wait until communication link with main program is established
communication.wait_until_up()

# Start the security watchdog immediately
communication.start_watchdog()

# ---------------------------------------------------------
# TODO: HANDSHAKE
# ---------------------------------------------------------


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
    
    if choice == "1":
        resp = communication.send_command("GET_STATUS")
        if resp:
            data = resp.get("data", {})
            print(f"Device Serial: {data.get('serial')}")
            print(f"Device Firmware: {data.get('firmware')}")
            print(f"Enrolled: {data.get('fingerprint_enrolled')}")
        else:
            print("Device not responding.")

    elif choice == "2":
        print("Starting enrollment... Put your finger on the sensor.")
        resp = communication.send_command("ENROLL_FINGERPRINT")
        if resp and resp.get("ok"):
            print("Success! Fingerprint saved.")
        else:
            print("Enrollment failed.")

    elif choice == "3":
        print("Please scan your finger now...")
        resp = communication.send_command("AUTH_FINGERPRINT", {"timeout_s": 10}) # TODO: Make sure this timeout_s thing actually does anything
        if resp and resp.get("ok"):
            print("FINGERPRINT ACCEPTED!")
        elif resp:
            data = resp.get("data", {})
            print(f"Fingerprint rejected. Error: {data.get('error')}") # TODO: Make sure this error thing actually does anything
        else:
            print("FINGERPRINT REJECTED.")

    elif choice == "4":
        fname = input("Enter filename to encrypt: ")
        target_sn = int(input("Enter serial number of target decryptor device:"))
        files.encrypt_file(fname, target_sn)

    elif choice == "5":
        fname = input("Enter .ukey filename to decrypt: ")

        # Get serial number of connected device
        resp = communication.send_command("GET_STATUS")
        if resp:
            device_sn = int(resp.get("data", {}).get('serial'))
        else:
            print("Device not responding.")

        
        # print("You must authenticate first...")
        # resp = communication.send_command("AUTH_FINGERPRINT")
        
        if True:#resp and resp["data"]["accepted"]:
            print("Auth OK.")
            
            secret_file = files.decrypt_file(fname, device_sn)
            
            # Only proceed if a file was actually created
            if secret_file:
                input("File is ready. Press ENTER to delete it.")
                
                # # Cleanup
                # if os.path.exists(secret_file):
                #     os.remove(secret_file)
                #     print("File deleted.")
                #     communication.current_decrypted_file = None
        else:
            print("Authentication failed!")

    elif choice == "6":
        print("Exiting...")
        communication.running = False # Stop the watchdog
        break

    else:
        print("Invalid choice.")