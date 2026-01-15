FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y usbutils \
                       g++ \
                       libserialport-dev \
                       libzmq3-dev \
                       python3 \
                       python3-pip && \
    rm -rf /var/lib/apt/lists/*

RUN pip3 install --no-cache-dir typer>=0.12.3 \
                                rich>=13.7.1 \
                                pydantic>=2.7.1 \
                                requests>=2.32.3 \
                                pyserial>=3.5 \
                                pyzmq>=26.0.0

# CMD ["lsusb"]