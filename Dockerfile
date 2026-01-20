FROM python:3.12-slim

WORKDIR /app

# install libraries
COPY requirements.txt .
RUN pip install -r requirements.txt

# copy all my python files
COPY main.py .
COPY communication.py .
COPY files.py .

# run the app
ENTRYPOINT ["python", "-u", "main.py"]