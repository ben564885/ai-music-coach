# ⚠️ SECURITY ALERT: Rotate Your Secrets

GitGuardian detected secrets in your repository. **You MUST rotate these immediately:**

## 🔴 Secrets That Were Exposed

1. **Supabase Service Role Key** (CRITICAL)
   - This key has FULL database access
   - **Action Required**: Rotate immediately

2. **Supabase Anon Key** (LOW RISK)
   - This is actually safe to expose (designed for client-side)
   - But GitGuardian flagged it anyway

3. **Anthropic API Key** (CRITICAL)
   - From the `.env` file that was in git history
   - **Action Required**: Rotate immediately

## 🛡️ How to Rotate Secrets

### 1. Supabase Service Role Key

1. Go to: https://supabase.com/dashboard/project/zugupjngontrrcroykot/settings/api
2. Click **"Reset service_role key"** (or generate new)
3. Copy the new key
4. Update:
   - `cloud-backend/.env` (local)
   - Railway environment variables (production)

### 2. Supabase Anon Key (Optional)

1. Same page: https://supabase.com/dashboard/project/zugupjngontrrcroykot/settings/api
2. Click **"Reset anon key"** if you want
3. Update `mobile-app/src/config.js`

### 3. Anthropic API Key

1. Go to: https://console.anthropic.com/settings/keys
2. Revoke the old key that was exposed (check your key list - it starts with `sk-ant-api03-...`)
3. Generate a new key
4. Update:
   - `cloud-backend/.env` (local)
   - Railway environment variables (production)

## ✅ What I Fixed

- ✅ Removed `.env` from git history
- ✅ Removed hardcoded secrets from `UPDATE_ENV.sh`
- ✅ Removed hardcoded anon key from `mobile-app/src/config.js`
- ✅ All files now use placeholders
- ✅ Force-pushed cleaned history to GitHub

## 📝 Current Status

**All secrets are now removed from git history**, but you still need to:
1. Rotate the exposed keys (they're compromised)
2. Update your local `.env` files
3. Update Railway environment variables

## 🔒 Going Forward

- ✅ Never commit `.env` files
- ✅ Never hardcode secrets in scripts
- ✅ Use environment variables everywhere
- ✅ Use placeholders in example files

Your repository is now clean! 🎉
