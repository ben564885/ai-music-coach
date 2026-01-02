# Quick Testing Guide

## Test on Your Phone Right Now (5 minutes)

### Step 1: Install Expo Go
- **iPhone**: [Download from App Store](https://apps.apple.com/app/expo-go/id982107779)
- **Android**: [Download from Google Play](https://play.google.com/store/apps/details?id=host.exp.exponent)

### Step 2: Start the App
```bash
cd mobile-app
npm start
```

You'll see a QR code in your terminal.

### Step 3: Connect Your Phone
1. **Make sure your phone and computer are on the same WiFi network**
2. Open Expo Go app on your phone
3. Tap "Scan QR Code"
4. Point camera at the QR code in terminal
5. App will load on your phone!

### Troubleshooting

**QR code doesn't work?**
- Press `s` in the terminal to switch to tunnel mode
- Or type your computer's IP address manually in Expo Go

**App won't load?**
- Check WiFi connection (both devices on same network)
- Try restarting: Press `r` in terminal to reload
- Check firewall settings on your computer

**Changes not showing?**
- Shake your phone to open Expo menu
- Tap "Reload" or press `r` in terminal

That's it! You can now test the app on your real phone. Changes you make will automatically reload.

## Next: Deploy to App Store

See `DEPLOYMENT.md` for instructions on building and submitting to App Store and Google Play.

