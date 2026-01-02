#!/bin/bash

# Start Cloud Backend Server
cd "$(dirname "$0")/cloud-backend"

if [ ! -d "venv" ]; then
    echo "❌ Virtual environment not found. Please run ./setup.sh first."
    exit 1
fi

source venv/bin/activate
echo "🚀 Starting AI Music Coach Backend Server..."
echo "📍 Server will be available at http://localhost:5000"
echo ""
python server.py

