#!/bin/bash

# Script to help set up Supabase configuration
# This will be run after you provide your Supabase credentials

echo "🔧 Setting up Supabase configuration..."
echo ""

# Check if .env file exists
if [ ! -f "cloud-backend/.env" ]; then
    echo "Creating cloud-backend/.env file..."
    cat > cloud-backend/.env << 'EOF'
# Anthropic API Key for Claude AI coaching
ANTHROPIC_API_KEY=your_api_key_here

# Server Configuration
PORT=5000
DEBUG=True

# Supabase Configuration
SUPABASE_URL=your_supabase_url_here
SUPABASE_SERVICE_KEY=your_service_key_here
SUPABASE_JWT_SECRET=your_jwt_secret_here

# File Upload Settings
MAX_UPLOAD_SIZE=50MB
EOF
    echo "✅ Created cloud-backend/.env"
else
    echo "✅ cloud-backend/.env already exists"
fi

echo ""
echo "📝 Next steps:"
echo "1. Edit cloud-backend/.env and add your Supabase credentials"
echo "2. Edit mobile-app/src/config.js and add your Supabase URL and anon key"
echo "3. Run the SQL schema in Supabase dashboard (see database/schema.sql)"
echo ""

