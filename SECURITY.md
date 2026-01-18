# Security Guide

This document outlines security best practices for the AI Music Coach project.

## Credential Management

### Never Commit Sensitive Data

The following files contain sensitive credentials and are excluded from git:
- `firmware_copy/config.h` - Device credentials and backend IP
- `cloud-backend/.env` - API keys and database credentials
- `mobile_app_flutter/.env` - Backend URL and Supabase keys

### Setup Process

1. **Firmware Configuration**:
   ```bash
   cd firmware_copy
   cp config.h.example config.h
   # Edit config.h with your actual credentials
   ```

2. **Backend Configuration**:
   ```bash
   cd cloud-backend
   cp .env.example .env
   # Edit .env with your actual API keys
   ```

3. **Mobile App Configuration**:
   ```bash
   cd mobile_app_flutter
   cp .env.example .env
   # Edit .env with your backend URL and Supabase keys
   ```

## Credentials Overview

### Firmware (`config.h`)
- **PRODUCT_KEY**: Tuya Product ID (from iot.tuya.com)
- **DEVICE_UUID**: Tuya Device UUID (from iot.tuya.com)
- **AUTH_KEY**: Tuya Device Auth Key (from iot.tuya.com) - **CRITICAL: Keep secret**
- **CLOUD_BACKEND_HOST**: Backend server IP address

### Backend (`.env`)
- **SUPABASE_URL**: Supabase project URL
- **SUPABASE_SERVICE_KEY**: Supabase service role key - **CRITICAL: Keep secret**
- **GEMINI_API_KEY**: Google Gemini API key
- **SOUNDSLICE_EMAIL/PASSWORD**: SoundSlice account credentials
- **SAMPLAB_EMAIL/PASSWORD**: Samplab account credentials
- **ROBOFLOW_API_KEY**: Roboflow API key (optional)

### Mobile App (`.env`)
- **BACKEND_URL**: Backend server URL
- **EXPO_PUBLIC_SUPABASE_URL**: Supabase project URL
- **EXPO_PUBLIC_SUPABASE_ANON_KEY**: Supabase anonymous key

## If Credentials Are Exposed

If you accidentally commit credentials to git:

1. **Immediately rotate all exposed credentials**:
   - Tuya: Create new device and get new AUTH_KEY
   - Supabase: Rotate service role key
   - API Keys: Regenerate all API keys (Gemini, Roboflow, etc.)
   - SoundSlice/Samplab: Change passwords

2. **Remove from git history** (if not yet pushed):
   ```bash
   git filter-branch --force --index-filter \
     "git rm --cached --ignore-unmatch path/to/file" \
     --prune-empty --tag-name-filter cat -- --all
   ```

3. **If already pushed**: Rotate credentials immediately and consider the old ones compromised

## Development vs Production

- Use separate credentials for development and production
- Never use production credentials in development
- Use environment-specific configuration files

## Additional Security Measures

- Keep dependencies updated (`pip install -r requirements.txt --upgrade`)
- Review and audit third-party dependencies regularly
- Use HTTPS in production (configure reverse proxy with SSL)
- Implement rate limiting on API endpoints
- Monitor for suspicious activity in Supabase logs
