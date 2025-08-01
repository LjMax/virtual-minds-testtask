FROM python:3.9-slim
WORKDIR /app
RUN pip install flask requests
RUN apt update
RUN apt install dnsutils curl -y
COPY daemon.py .
CMD ["python", "daemon.py"]
