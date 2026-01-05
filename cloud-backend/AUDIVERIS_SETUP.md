# Audiveris Setup Guide

This guide explains how to install and configure Audiveris for use with the AI Music Coach backend.

## What is Audiveris?

Audiveris is an open-source Optical Music Recognition (OMR) system that converts scanned sheet music images into MusicXML format. It's a Java-based application that can run in batch mode for automated processing.

## Installation Options

### Option 1: Download Pre-built Installer (Easiest)

1. **Go to Audiveris Releases**: https://github.com/Audiveris/audiveris/releases
2. **Download the latest release** for your platform:
   - **macOS**: Download the `.dmg` file
   - **Linux**: Download the `.deb` file  
   - **Windows**: Download the `.msi` file
3. **Install**:
   - **macOS**: Open the `.dmg` and drag Audiveris to Applications folder
   - **Linux**: `sudo dpkg -i audiveris-*.deb`
   - **Windows**: Run the `.msi` installer

### Option 2: Build from Source

If you need the latest features or the installer doesn't work:

1. **Clone the repository**:
   ```bash
   git clone https://github.com/Audiveris/audiveris.git
   cd audiveris
   ```

2. **Build with Gradle**:
   ```bash
   ./gradlew build
   ```

3. **Find the JAR file**: It will be in `build/libs/audiveris-*.jar`

## Finding the Audiveris Executable/JAR

After installation, you need to find where Audiveris is located:

### macOS (after .dmg installation):
```bash
# Check if it's in Applications
ls -la /Applications/Audiveris.app/Contents/MacOS/

# Or find it anywhere on your system
find /Applications -name "Audiveris" -type f 2>/dev/null
```

The executable is typically at: `/Applications/Audiveris.app/Contents/MacOS/Audiveris`

### Linux:
```bash
# Check common locations
which audiveris
# or
find /usr -name "audiveris" 2>/dev/null
```

### If you built from source:
The JAR file will be in: `audiveris/build/libs/audiveris-*.jar`

## Configuration

### Step 1: Find Your Audiveris Path

Run one of these commands to find Audiveris:

**macOS:**
```bash
# Try to find the executable
find /Applications -name "Audiveris" -type f 2>/dev/null

# Or check if it's in PATH
which audiveris
```

**Linux:**
```bash
which audiveris
# or
find /usr -name "audiveris" 2>/dev/null
```

**If you have a JAR file:**
```bash
# Find the JAR file location
find ~ -name "audiveris*.jar" 2>/dev/null
```

### Step 2: Set Environment Variable

Add one of these to your `.env` file in the `cloud-backend/` directory:

**Option A: If you found an executable (recommended)**
```bash
# For macOS
AUDIVERIS_CMD=/Applications/Audiveris.app/Contents/MacOS/Audiveris

# For Linux
AUDIVERIS_CMD=/usr/bin/audiveris

# Or if it's in your PATH, just use:
AUDIVERIS_CMD=audiveris
```

**Option B: If you have a JAR file**
```bash
# Point to the JAR file
AUDIVERIS_JAR=/path/to/audiveris/build/libs/audiveris-5.9.0.jar
```

### Step 3: Test the Configuration

Test if Audiveris is accessible:

```bash
# If using AUDIVERIS_CMD
$AUDIVERIS_CMD --version

# If using AUDIVERIS_JAR
java -jar $AUDIVERIS_JAR --version
```

## Example .env File

Add this to `cloud-backend/.env`:

```bash
# Audiveris Configuration
# Use one of these options:

# Option 1: Executable path (if installed via installer)
AUDIVERIS_CMD=/Applications/Audiveris.app/Contents/MacOS/Audiveris

# Option 2: JAR file path (if built from source)
# AUDIVERIS_JAR=/Users/yourname/audiveris/build/libs/audiveris-5.9.0.jar

# Option 3: If audiveris is in your PATH
# AUDIVERIS_CMD=audiveris
```

## Troubleshooting

### "Audiveris not found" error

1. **Check if Audiveris is installed**:
   ```bash
   # macOS
   ls -la /Applications/Audiveris.app/
   
   # Linux
   which audiveris
   ```

2. **Verify the path in .env**:
   ```bash
   # Make sure the path exists
   ls -la "$AUDIVERIS_CMD"
   # or
   ls -la "$AUDIVERIS_JAR"
   ```

3. **Check file permissions**:
   ```bash
   chmod +x "$AUDIVERIS_CMD"
   ```

### "Command not found" error

- Make sure Java is installed: `java -version`
- If using JAR, ensure the path is correct and the file exists
- Try using the full absolute path instead of a relative path

### Audiveris doesn't support batch mode

Some versions of Audiveris may not support command-line batch processing. In that case:

1. **Check Audiveris version**: Make sure you have version 5.5 or later
2. **Try building from source**: The latest development version may have better CLI support
3. **Alternative**: Use the GUI version and export MusicXML manually, then use the `/api/process-musicxml` endpoint

## Alternative: Using Audiveris GUI

If command-line batch mode doesn't work, you can:

1. Use Audiveris GUI to process images
2. Export as MusicXML
3. Use the `/api/process-musicxml` endpoint to process the MusicXML file

## Need Help?

- **Audiveris Documentation**: https://audiveris.github.io/audiveris/
- **GitHub Issues**: https://github.com/Audiveris/audiveris/issues
- **User Handbook**: https://audiveris.github.io/audiveris/_pages/handbook/

