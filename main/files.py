import os
import subprocess
import base64
import shutil
import communication
import csv
from Crypto.PublicKey import RSA

# Configuration constants for the hardcoded paths expected by the C encryption/decryption binaries
ENC_BIN = "/app/build/bin/rsa_aes_enc"
DEC_BIN = "/app/build/bin/rsa_aes_dec"
C_INPUT_FILE = "input.pdf"
C_ENC_OUTPUT_FILE = "output.bin"
C_PRIV_KEY = "usb_priv.pem"
C_DEC_OUT = "input_decrypted.pdf"
CSV_FILE = "/app/crypto/keys.csv"

def get_pub_key(target_sn):
    with open(CSV_FILE, newline="") as f:
        reader = csv.DictReader(f)

        for row in reader:
            if int(row["Serial Number"]) == target_sn:
                return row["Public RSA Key"]

    return None

def simple_encrypt_file(target_file, target_sn):
    """
    Encrypts any file by preparing a workspace for the rsa_aes_enc C binary.
    The C binary expects specific hardcoded filenames, so we temporarily copy the target 
    file into the expected location, run the binary, and move the output back with a .ukey extension.
    """
    if not os.path.exists(target_file):
        print(f"Error: {target_file} not found.")
        return

    # Use absolute paths so files can be safely moved regardless of the terminal's working directory
    target_abs = os.path.abspath(target_file)
    output_ukey = target_abs + ".ukey"

    try:
        # 1. Setup workspace: Copy the target file to 'input.pdf' (the name the C binary expects)
        if target_abs != os.path.abspath(C_INPUT_FILE):
            shutil.copy(target_abs, C_INPUT_FILE)

        # 2. Get public key to pass to encryption program (still base64 encoded)
        pubkey = get_pub_key(target_sn)
        if pubkey == None:
            print(f"Failed to find public key for device with serial number {target_sn} in {CSV_FILE}")
            return

        # 3. Execute the C encryption binary
        subprocess.run([ENC_BIN, pubkey], check=True)
        
        if os.path.exists(C_ENC_OUTPUT_FILE):
            # 4: Append the target device's serial number (assuming fixed, 32-bit length)
            #    to end of encrypted file

            # Read encrypted data
            with open(C_ENC_OUTPUT_FILE, "rb") as f:
                encrypted_data = f.read()
            
            # Append unsigned, 32-bit serial number to end of encrypted data in big-endian manner
            encrypted_data += target_sn.to_bytes(4, byteorder="big", signed=False)

            # 5. Handle output: Move the resulting encrypted_data into the .ukey file
            if os.path.exists(output_ukey):
                os.remove(output_ukey) # Overwrite if it already exists
                
            with open(output_ukey, "wb") as f:
                f.write(encrypted_data)

            print(f"Success! Encrypted file created at: {output_ukey}")
            
    except Exception as e:
        print(f"Encryption failed: {e}")
    finally:
        # 6. Clean up temporary workspace files to avoid clutter
        for tmp in [C_INPUT_FILE, C_ENC_OUTPUT_FILE]:
            if os.path.exists(tmp) and os.path.abspath(tmp) != target_abs:
                os.remove(tmp)

def simple_decrypt_file(target_ukey, device_sn):
    """
    Retrieves the private key from the hardware, formats it, and uses the rsa_aes_dec binary.
    Outputs the decrypted file as [name]-decrypted.[ext] in the original directory.
    """
    if not os.path.exists(target_ukey):
        print(f"Error: {target_ukey} not found.")
        return None

    ukey_abs = os.path.abspath(target_ukey)
    
    # Calculate the final output name: e.g., 'document.pdf.ukey' -> 'document-decrypted.pdf'
    original_base = ukey_abs.replace(".ukey", "")
    base_name, extension = os.path.splitext(original_base)
    output_decrypted = f"{base_name}-decrypted{extension}"

    # Extract serial number that file targetted its encryption towards
    try:
        with open(ukey_abs, "rb") as f:
            encrypted_data_incl_sn = f.read()
        
        if len(encrypted_data_incl_sn) < 4:
            print("Error: UKEY file too small to contain target serial number.")
            return None
        
        encrypted_data = encrypted_data_incl_sn[:-4]
        target_sn = int.from_bytes(encrypted_data_incl_sn[-4:], byteorder="big", signed=False)

        if target_sn != device_sn:
            print(f"Error: Trying to decrypt file encrypted for device {target_sn} with device {device_sn}")
            return None
    except Exception as e:
        print(f"Failed to extract serial number from .ukey file: {e}")
        return None

    # 1. Hardware Key Retrieval via ZMQ request to the C++ hardware communication wrapper
    resp = communication.send_command("GET_PRIVATE_KEY")
    if not resp or not resp.get("ok"):
        print("Error: Could not retrieve key from hardware.")
        return None
    
    try:
        # 2. Key Preparation for C binary
        # The hardware returns the key in Base64 (from its DER binary format)
        der_bytes = base64.b64decode(resp["data"]["secret_key"])
        
        # Import the DER key and export it as PEM (text format) to 'usb_priv.pem'
        with open(C_PRIV_KEY, "wb") as f:
            f.write(RSA.import_key(der_bytes).export_key(format='PEM'))

        # 3. Setup workspace: The decryption binary expects 'output.bin' as its input file
        if os.path.exists(C_ENC_OUTPUT_FILE):
            os.remove(C_ENC_OUTPUT_FILE)
        
        # Write stripped encrypted data (without sn) for C binary
        with open(C_ENC_OUTPUT_FILE, "wb") as f:
            f.write(encrypted_data)

        # 4. Execute Decryption binary
        subprocess.run([DEC_BIN], check=True)
        
        # 5. Handle output: The binary hardcodes its output to 'input_decrypted.pdf'
        if os.path.exists(C_DEC_OUT):
            if os.path.exists(output_decrypted):
                os.remove(output_decrypted) # Overwrite if it already exists
                
            # Move and rename the output back to the original folder
            shutil.move(C_DEC_OUT, output_decrypted)
            print(f"Success! Decrypted file created at: {output_decrypted}")
            
            # Register this file with the watchdog thread so it gets securely deleted if the USB disconnects
            communication.current_decrypted_file = output_decrypted
            return output_decrypted
        else:
            print("Error: Decryption binary failed to produce output.")

    except Exception as e:
        print(f"Decryption failed: {e}")
        return None
    finally:
        # 6. Cleanup sensitive temporary files (Crucial to ensure the private key isn't left on the PC)
        for tmp in [C_ENC_OUTPUT_FILE, C_PRIV_KEY]:
            if os.path.exists(tmp):
                os.remove(tmp)