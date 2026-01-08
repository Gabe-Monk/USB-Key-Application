FROM python:3.12-slim
WORKDIR /srv
RUN pip install --no-cache-dir flask==3.0.3
COPY mock_device/server.py /srv/server.py
EXPOSE 8765
CMD ["python", "server.py"]
