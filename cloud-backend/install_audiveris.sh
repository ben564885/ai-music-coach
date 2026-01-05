#!/bin/bash
# Script to download and build Audiveris from source

set -e

echo "Installing Audiveris from source..."

# Check if Java is installed
if ! command -v java &> /dev/null; then
    echo "Error: Java is not installed. Please install Java first."
    exit 1
fi

echo "Java version:"
java -version

# Create a directory for Audiveris
AUDIVERIS_DIR="$HOME/audiveris"
mkdir -p "$AUDIVERIS_DIR"
cd "$AUDIVERIS_DIR"

# Clone the repository if it doesn't exist
if [ ! -d "audiveris" ]; then
    echo "Cloning Audiveris repository..."
    git clone https://github.com/Audiveris/audiveris.git
fi

cd audiveris

# Check if Gradle wrapper exists
if [ ! -f "gradlew" ]; then
    echo "Error: Gradle wrapper not found. Repository may be incomplete."
    exit 1
fi

# Make gradlew executable
chmod +x gradlew

# Build Audiveris
echo "Building Audiveris (this may take several minutes)..."
./gradlew build -x test

# Find the JAR file
JAR_FILE=$(find build/libs -name "audiveris-*.jar" -not -name "*-sources.jar" | head -1)

if [ -z "$JAR_FILE" ]; then
    echo "Error: Could not find built JAR file."
    exit 1
fi

echo ""
echo "✅ Audiveris built successfully!"
echo ""
echo "JAR file location: $JAR_FILE"
echo ""
echo "Add this to your cloud-backend/.env file:"
echo "AUDIVERIS_JAR=$JAR_FILE"
echo ""

