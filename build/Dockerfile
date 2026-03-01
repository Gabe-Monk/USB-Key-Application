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
RUN pip install --no-cache-dir -r /app/build/requirements.txt

RUN mkdir -p /app/build/bin

# Build hardware communication submodule
RUN g++ hardware_communication/hw_comms.cpp -o build/bin/hw_comms -lzmq -ljsoncpp -lserialport

# Build encryption/decryption binaries
# We compile them into the build/ directory for easy access
RUN gcc crypto/rsa_aes_enc.c crypto/aesGcm.c -o build/bin/rsa_aes_enc -lssl -lcrypto
RUN gcc crypto/rsa_aes_dec.c crypto/aesGcm.c -o build/bin/rsa_aes_dec -lssl -lcrypto