#!/bin/bash

# AI Music Coach Setup Script
echo "🎵 Setting up AI Music Coach..."
echo ""

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check Python
echo -e "${YELLOW}Checking Python...${NC}"
if ! command -v python3 &> /dev/null; then
    echo "❌ Python 3 is not installed. Please install Python 3.9+ first."
    exit 1
fi
PYTHON_VERSION=$(python3 --version)
echo -e "${GREEN}✓${NC} $PYTHON_VERSION"

# Check Node.js
echo -e "${YELLOW}Checking Node.js...${NC}"
if ! command -v node &> /dev/null; then
    echo "❌ Node.js is not installed. Please install Node.js 18+ first."
    exit 1
fi
NODE_VERSION=$(node --version)
echo -e "${GREEN}✓${NC} $NODE_VERSION"

# Setup Cloud Backend
echo ""
echo -e "${YELLOW}Setting up Cloud Backend...${NC}"
cd cloud-backend

if [ ! -d "venv" ]; then
    echo "Creating Python virtual environment..."
    python3 -m venv venv
fi

echo "Activating virtual environment..."
source venv/bin/activate

echo "Upgrading pip..."
pip install --upgrade pip --quiet

echo "Installing Python dependencies..."
pip install -r requirements.txt --quiet

if [ ! -f ".env" ]; then
    echo "Creating .env file..."
    cat > .env << 'EOF'
# Anthropic API Key for Claude AI coaching
ANTHROPIC_API_KEY=your_api_key_here

# Server Configuration
PORT=5000
DEBUG=True

# File Upload Settings
MAX_UPLOAD_SIZE=50MB
EOF
    echo -e "${YELLOW}⚠${NC}  Please edit cloud-backend/.env and add your ANTHROPIC_API_KEY"
else
    echo -e "${GREEN}✓${NC} .env file already exists"
fi

# Create necessary directories
mkdir -p uploads recordings
echo -e "${GREEN}✓${NC} Cloud backend setup complete"

# Setup Mobile App
echo ""
echo -e "${YELLOW}Setting up Mobile App...${NC}"
cd ../mobile-app

if [ ! -d "node_modules" ]; then
    echo "Installing Node.js dependencies..."
    npm install --silent
else
    echo -e "${GREEN}✓${NC} Node modules already installed"
fi

echo -e "${GREEN}✓${NC} Mobile app setup complete"

cd ..

echo ""
echo -e "${GREEN}✅ Setup complete!${NC}"
echo ""
echo "Next steps:"
echo "1. Edit cloud-backend/.env and add your ANTHROPIC_API_KEY"
echo "2. Start the backend: cd cloud-backend && source venv/bin/activate && python server.py"
echo "3. Start the mobile app: cd mobile-app && npm start"
echo ""

