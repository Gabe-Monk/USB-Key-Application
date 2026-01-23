FROM python:3.12-slim

RUN apt-get update && \
    apt-get install -y usbutils \
                       g++ \
                       libserialport-dev \
                       libzmq3-dev && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy program files into container
COPY . .

# Install Python libraries
RUN pip install --no-cache-dir -r build/requirements.txt

# run the app
ENTRYPOINT ["/app/build/entrypoint.sh"]