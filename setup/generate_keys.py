import subprocess
import base64
import os

def create_keys():
    """
    Generates a 512-bit RSA key pair using system OpenSSL commands.
    The private key is converted to a compact binary (DER) format and Base64 encoded
    so it can be easily copied and written to the USB key's limited EEPROM.
    """
    print("--- 1. Generating RSA-512 Keys ---")
    
    # Use system OpenSSL to generate the private key.
    # 512-bit is used to ensure the key physically fits within the tiny EEPROM storage limits.
    subprocess.run(["openssl", "genrsa", "-out", "temp_priv.pem", "512"], check=True, stderr=subprocess.DEVNULL)
    
    # Extract the Public Key from the generated private key and save it as a PEM file.
    # This public key will be used by the PC-side application to encrypt files.
    subprocess.run(["openssl", "rsa", "-in", "temp_priv.pem", "-pubout", "-out", "public_key.pem"], check=True, stderr=subprocess.DEVNULL)
    print("SUCCESS: 'public_key.pem' saved to this folder.")

    # Convert the Private Key from PEM (text) to DER (binary) format.
    # Binary format removes PEM headers/footers and base64 overhead, making it small enough for EEPROM.
    subprocess.run(["openssl", "rsa", "-in", "temp_priv.pem", "-outform", "DER", "-out", "temp_priv.der"], check=True, stderr=subprocess.DEVNULL)

    # Read the binary DER file and encode it into a Base64 string.
    # This stringifies the binary so you can easily copy-paste it into the write_key.py script.
    with open("temp_priv.der", "rb") as f:
        key_data = f.read()
        b64_key = base64.b64encode(key_data).decode('utf-8')

    # Cleanup the temporary private key files so they aren't left unsecured on the PC.
    os.remove("temp_priv.pem")
    os.remove("temp_priv.der")

    print("\n--- 2. COPY THE STRING BELOW ---")
    print("Paste this string into the 'KEY_STRING' variable in the Pico script:")
    print("-" * 60)
    print(b64_key)
    print("-" * 60)

if __name__ == "__main__":
    create_keys()