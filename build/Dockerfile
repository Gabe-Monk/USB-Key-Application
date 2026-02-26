FROM python:3.12-slim

RUN apt-get update && \
    apt-get install -y usbutils \
                       udev \
                       g++ \
                       libjsoncpp-dev \
                       libserialport-dev \
                       libzmq3-dev \
                       libssl-dev && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy program files into container
COPY . .

# Install Python libraries
RUN pip install --no-cache-dir -r build/requirements.txt

# Build hardware communication submodule
RUN g++ hardware_communication/hw_comms.cpp -o hardware_communication/hw_comms -lzmq -ljsoncpp -lserialport

# Build Aahash's Encryption/Decryption binaries
# We compile them into the build/ directory for easy access
RUN gcc temporary/aahashs_stuff/rsa_aes_enc.c temporary/aahashs_stuff/aesGcm.c -o build/rsa_aes_enc -lssl -lcrypto
RUN gcc temporary/aahashs_stuff/rsa_aes_dec.c temporary/aahashs_stuff/aesGcm.c -o build/rsa_aes_dec -lssl -lcrypto

RUN chmod +x build/entrypoint.sh