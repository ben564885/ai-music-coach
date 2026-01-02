# Dockerfile for cloud deployment (Railway, Render, Fly.io, etc.)
FROM python:3.9-slim

WORKDIR /app

# Install system dependencies for audio processing
RUN apt-get update && apt-get install -y \
    libsndfile1 \
    ffmpeg \
    && rm -rf /var/lib/apt/lists/*

# Copy requirements first for better caching
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy application code
COPY . .

# Create directories for uploads
RUN mkdir -p uploads recordings

# Expose port
EXPOSE 5000

# Use gunicorn for production
CMD ["gunicorn", "server:app", "--bind", "0.0.0.0:5000", "--workers", "2", "--timeout", "120"]

