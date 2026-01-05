# Simple Audiveris Setup (Recommended)

Since building from source requires Java 25, the easiest approach is to use the **pre-built installer** which comes with Java bundled.

## Quick Setup Steps

### 1. Download Audiveris Installer

**Option A: Download via Browser**
1. Go to: https://github.com/Audiveris/audiveris/releases/latest
2. Download: `Audiveris-5.9.0-macosx-arm64.dmg` (for Apple Silicon Macs)
   - Or `Audiveris-5.9.0-macosx-x86_64.dmg` (for Intel Macs)

**Option B: Download via Command Line**
```bash
cd ~/Downloads
curl -L -o Audiveris.dmg https://github.com/Audiveris/audiveris/releases/download/5.9.0/Audiveris-5.9.0-macosx-arm64.dmg
```

### 2. Install Audiveris

1. Open the `.dmg` file
2. Drag `Audiveris.app` to your `Applications` folder

### 3. Find the Executable Path

The Audiveris executable is located at:
```
/Applications/Audiveris.app/Contents/MacOS/Audiveris
```

### 4. Add to Your .env File

Add this line to `cloud-backend/.env`:

```bash
AUDIVERIS_CMD=/Applications/Audiveris.app/Contents/MacOS/Audiveris
```

### 5. Test It

```bash
/Applications/Audiveris.app/Contents/MacOS/Audiveris --version
```

## Alternative: If Command-Line Doesn't Work

If the pre-built installer doesn't support command-line batch mode, you have two options:

### Option A: Use the GUI and Manual Export
1. Open Audiveris from Applications
2. Process your sheet music images
3. Export as MusicXML
4. Use the `/api/process-musicxml` endpoint to process the MusicXML

### Option B: Install Java 25 and Build from Source

If you want to build from source (requires Java 25):

1. **Install Java 25** (requires password):
   ```bash
   brew install microsoft-openjdk@25
   # You'll need to enter your password
   ```

2. **Add Java 25 to PATH**:
   ```bash
   echo 'export PATH="/opt/homebrew/opt/microsoft-openjdk@25/bin:$PATH"' >> ~/.zshrc
   source ~/.zshrc
   ```

3. **Build Audiveris**:
   ```bash
   cd ~
   git clone https://github.com/Audiveris/audiveris.git
   cd audiveris
   chmod +x gradlew
   ./gradlew build -x test
   ```

4. **Find the JAR and add to .env**:
   ```bash
   find ~/audiveris/build/libs -name "audiveris-*.jar" -not -name "*-sources.jar"
   # Then add to .env: AUDIVERIS_JAR=/path/to/audiveris.jar
   ```

## Recommended: Use the Installer

For most users, **using the pre-built installer is the simplest approach**. Just download, install, and add the path to your `.env` file!

