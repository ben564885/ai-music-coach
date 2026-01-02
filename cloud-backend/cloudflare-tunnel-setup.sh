#!/bin/bash

# Cloudflare Tunnel Setup Script
# Run this after deploying your backend to Railway/Render

echo "🌐 Setting up Cloudflare Tunnel..."
echo ""

# Check if cloudflared is installed
if ! command -v cloudflared &> /dev/null; then
    echo "Installing cloudflared..."
    if [[ "$OSTYPE" == "darwin"* ]]; then
        brew install cloudflare/cloudflare/cloudflared
    elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
        wget https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-amd64.deb
        sudo dpkg -i cloudflared-linux-amd64.deb
    else
        echo "Please install cloudflared manually: https://developers.cloudflare.com/cloudflare-one/connections/connect-apps/install-and-setup/installation/"
        exit 1
    fi
fi

echo "✅ cloudflared installed"
echo ""
echo "Next steps:"
echo "1. Login to Cloudflare: cloudflared tunnel login"
echo "2. Create tunnel: cloudflared tunnel create ai-music-coach"
echo "3. Configure DNS in Cloudflare dashboard"
echo "4. Run tunnel: cloudflared tunnel run ai-music-coach"
echo ""
echo "See CLOUDFLARE_DEPLOYMENT.md for detailed instructions"

