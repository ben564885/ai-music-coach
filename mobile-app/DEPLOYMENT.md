# Mobile App Deployment Guide

## Testing on Your Phone (Development)

### Option 1: Expo Go (Easiest - Recommended for Testing)

1. **Install Expo Go on your phone:**
   - iOS: Download from [App Store](https://apps.apple.com/app/expo-go/id982107779)
   - Android: Download from [Google Play](https://play.google.com/store/apps/details?id=host.exp.exponent)

2. **Start the development server:**
   ```bash
   cd mobile-app
   npm start
   ```

3. **Connect your phone:**
   - **Same WiFi**: Make sure your phone and computer are on the same WiFi network
   - **Scan QR code**: Open Expo Go app and scan the QR code shown in terminal
   - **Or use tunnel**: If QR code doesn't work, press `s` to switch to tunnel mode

4. **That's it!** The app will load on your phone and auto-reload when you make changes.

### Option 2: Development Build (More Native Features)

For features that require native modules (like Bluetooth), you need a development build:

```bash
# Install EAS CLI
npm install -g eas-cli

# Login to Expo
eas login

# Build development version
eas build --profile development --platform ios    # For iOS
eas build --profile development --platform android # For Android
```

Then install the `.ipa` (iOS) or `.apk` (Android) file on your device.

## Building for App Stores

### Prerequisites

1. **Expo Account**: Sign up at [expo.dev](https://expo.dev)
2. **Apple Developer Account** ($99/year) - For iOS App Store
3. **Google Play Developer Account** ($25 one-time) - For Google Play Store

### Step 1: Configure App Details

Edit `app.json`:
```json
{
  "expo": {
    "name": "AI Music Coach",
    "slug": "ai-music-coach",
    "version": "1.0.0",
    "orientation": "portrait",
    "icon": "./assets/icon.png",
    "splash": {
      "image": "./assets/splash.png",
      "resizeMode": "contain",
      "backgroundColor": "#ffffff"
    },
    "ios": {
      "bundleIdentifier": "com.yourcompany.aimusiccoach",
      "buildNumber": "1"
    },
    "android": {
      "package": "com.yourcompany.aimusiccoach",
      "versionCode": 1
    }
  }
}
```

### Step 2: Create App Icons and Splash Screens

You need:
- **Icon**: 1024x1024px PNG (no transparency)
- **Splash**: 1242x2436px PNG

Place them in `mobile-app/assets/`:
- `icon.png` - App icon
- `splash.png` - Splash screen

### Step 3: Install EAS CLI

```bash
npm install -g eas-cli
eas login
```

### Step 4: Configure EAS Build

```bash
cd mobile-app
eas build:configure
```

This creates `eas.json` with build profiles.

### Step 5: Build for iOS App Store

```bash
# Build for App Store submission
eas build --platform ios --profile production

# Or build locally (requires macOS with Xcode)
eas build --platform ios --profile production --local
```

**What happens:**
1. Expo builds your app in the cloud
2. Downloads `.ipa` file
3. You can submit directly to App Store from Expo dashboard

### Step 6: Build for Google Play Store

```bash
# Build for Play Store submission
eas build --platform android --profile production
```

**What happens:**
1. Expo builds your app
2. Downloads `.aab` (Android App Bundle) file
3. Upload to Google Play Console

### Step 7: Submit to App Stores

#### iOS App Store

**Option A: Via Expo (Easiest)**
```bash
eas submit --platform ios
```

**Option B: Manual via App Store Connect**
1. Go to [App Store Connect](https://appstoreconnect.apple.com)
2. Create new app
3. Upload `.ipa` file
4. Fill in app details, screenshots, description
5. Submit for review

#### Google Play Store

**Option A: Via Expo**
```bash
eas submit --platform android
```

**Option B: Manual via Google Play Console**
1. Go to [Google Play Console](https://play.google.com/console)
2. Create new app
3. Upload `.aab` file
4. Fill in store listing, screenshots, description
5. Submit for review

## Quick Start Commands

```bash
# Test on phone (Expo Go)
cd mobile-app
npm start
# Scan QR code with Expo Go app

# Build for iOS
eas build --platform ios --profile production

# Build for Android
eas build --platform android --profile production

# Submit to stores
eas submit --platform ios
eas submit --platform android
```

## Troubleshooting

### Expo Go won't connect
- Make sure phone and computer are on same WiFi
- Try tunnel mode: Press `s` in Expo terminal
- Check firewall settings

### Build fails
- Make sure `app.json` is properly configured
- Check that you're logged in: `eas whoami`
- Verify bundle identifier/package name is unique

### App Store rejection
- Make sure you have proper app icons and screenshots
- Privacy policy URL is required
- App description must be clear about functionality

## Cost Breakdown

- **Expo**: Free (with paid tiers for more builds)
- **Apple Developer**: $99/year
- **Google Play Developer**: $25 one-time
- **EAS Build**: Free tier includes limited builds, then pay-per-build

## Next Steps After Submission

1. **Wait for review** (usually 1-3 days for iOS, faster for Android)
2. **Respond to any feedback** from app stores
3. **Monitor analytics** in App Store Connect / Play Console
4. **Update app** as needed with `eas build` and `eas submit`

