FROM python:3.12-slim

# Basic OS deps (shred for best-effort secure delete on Linux)
RUN apt-get update && apt-get install -y --no-install-recommends \
    coreutils \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

RUN pip install --no-cache-dir -U pip \
 && pip install --no-cache-dir -r /app/requirements.txt

ENTRYPOINT ["python", "-m", "usbkey"]
