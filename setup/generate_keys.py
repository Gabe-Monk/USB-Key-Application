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
    
    # Convert the Public Key from PEM (text) to DER (binary) format.
    # Binary format removes PEM headers/footers and base64 overhead, making it small enough for EEPROM.
    # This key will be used by PC side of appliation during encryption
    subprocess.run(["openssl", "rsa", "-in", "temp_priv.pem", "-pubout", "-outform", "DER", "-out", "temp_pub.der"], check=True, stderr=subprocess.DEVNULL)

    # Convert the Private Key from PEM (text) to DER (binary) format.
    # Binary format removes PEM headers/footers and base64 overhead, making it small enough for EEPROM.
    # This key will be stored privately on a USB Key device
    subprocess.run(["openssl", "rsa", "-in", "temp_priv.pem", "-outform", "DER", "-out", "temp_priv.der"], check=True, stderr=subprocess.DEVNULL)

    # Read the binary private DER file and encode it into a Base64 string.
    # This stringifies the binary so you can easily copy-paste it into the write_key.py script.
    with open("temp_priv.der", "rb") as f:
        key_data = f.read()
        b64_priv_key = base64.b64encode(key_data).decode('utf-8')

    # Read the binary public DER file and encode it into a Base64 string.
    # This stringifies the binary so you can easily copy-paste it into the keys.csv file
    with open("temp_pub.der", "rb") as f:
        key_data = f.read()
        b64_pub_key = base64.b64encode(key_data).decode('utf-8')

    # Cleanup the temporary private key files so they aren't left unsecured on the PC.
    os.remove("temp_priv.pem")
    os.remove("temp_priv.der")
    os.remove("temp_pub.der")

    print("\n--- 2. COPY THE PUBLIC KEY ---")
    print("Paste this string into as the public key for your entry into crypto/keys.csv for the appropriate serial number:")
    print("-" * 60)
    print(b64_pub_key)
    print("-" * 60)

    print("\n--- 3. COPY THE PRIVATE KEY ---")
    print("Paste this string into the 'KEY_STRING' variable in the setup/write_key.py Pico script:")
    print("-" * 60)
    print(b64_priv_key)
    print("-" * 60)

if __name__ == "__main__":
    create_keys()