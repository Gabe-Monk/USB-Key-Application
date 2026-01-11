FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y usbutils \
                       g++ \
                       libserialport-dev \
                       libzmq3-dev && \
    rm -rf /var/lib/apt/lists/*

# CMD ["lsusb"]