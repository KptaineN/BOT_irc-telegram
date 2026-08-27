FROM python:3.12-slim

WORKDIR /app

COPY requirements.txt .

RUN pip install --no-cache-dir -r requirements.txt

COPY bridge.py .
COPY irc_bot.py .

CMD ["python", "bridge.py"]
