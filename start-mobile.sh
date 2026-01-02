#!/bin/bash

# Start Mobile App
cd "$(dirname "$0")/mobile-app"

if [ ! -d "node_modules" ]; then
    echo "❌ Node modules not found. Please run ./setup.sh first."
    exit 1
fi

echo "🚀 Starting AI Music Coach Mobile App..."
echo "📱 Scan the QR code with Expo Go app or press 'i' for iOS / 'a' for Android"
echo ""
npm start

