# Backend Deployment Guide

## Quick Answer: Cloudflare?

**Can you use Cloudflare?** 
- ❌ Cloudflare Workers: No (too limited for ML libraries)
- ✅ Cloudflare Tunnel: Yes! Expose Railway/Render through Cloudflare
- ✅ Railway alone: Also fine (already has SSL)

See `CLOUDFLARE_DEPLOYMENT.md` for Cloudflare Tunnel setup.

## Recommended Cloud Platforms

### 🥇 **Railway** (Best Overall - Recommended)

**Why Railway:**
- ✅ Easiest setup (connects to GitHub, auto-deploys)
- ✅ Free tier: $5/month credit (usually enough for development)
- ✅ Built-in PostgreSQL (can use Supabase instead)
- ✅ Automatic HTTPS/SSL
- ✅ Environment variable management
- ✅ Great for Python/Flask apps
- ✅ Handles file uploads well

**Setup Steps:**
1. Go to [railway.app](https://railway.app)
2. Sign up with GitHub
3. Click "New Project" → "Deploy from GitHub repo"
4. Select your `ai_music_coach` repository
5. Set root directory to `cloud-backend`
6. Add environment variables (see "Environment Variables" section below)
7. Railway auto-detects Python and installs dependencies
8. Done! Get your URL (e.g., `https://your-app.railway.app`)

**Cost:** Free tier ($5 credit/month), then ~$5-10/month

---

### 🥈 **Render** (Great Alternative)

**Why Render:**
- ✅ Free tier available (with limitations)
- ✅ Easy GitHub integration
- ✅ Automatic SSL
- ✅ Good for Flask apps
- ⚠️ Free tier spins down after 15min inactivity (wakes on request)

**Setup Steps:**
1. Go to [render.com](https://render.com)
2. Sign up with GitHub
3. Click "New" → "Web Service"
4. Connect your GitHub repo
5. Settings:
   - Root Directory: `cloud-backend`
   - Build Command: `pip install -r requirements.txt`
   - Start Command: `gunicorn server:app`
6. Add environment variables
7. Deploy!

**Cost:** Free tier (with spin-down), then $7/month for always-on

---

### 🥉 **Fly.io** (Best Performance)

**Why Fly.io:**
- ✅ Great performance (runs close to users)
- ✅ Generous free tier
- ✅ Good for global apps
- ⚠️ Slightly more setup required

**Setup Steps:**
1. Install Fly CLI: `curl -L https://fly.io/install.sh | sh`
2. Sign up: `fly auth signup`
3. In `cloud-backend/` directory: `fly launch`
4. Follow prompts
5. Add secrets: `fly secrets set SUPABASE_URL=...`
6. Deploy: `fly deploy`

**Cost:** Free tier (3 VMs), then pay-as-you-go

---

## Quick Comparison

| Platform | Ease | Free Tier | Cost After | Best For |
|----------|------|-----------|------------|----------|
| **Railway** | ⭐⭐⭐⭐⭐ | $5 credit | $5-10/mo | Easiest setup |
| **Render** | ⭐⭐⭐⭐ | Yes (spins down) | $7/mo | Simple deployment |
| **Fly.io** | ⭐⭐⭐ | Yes | Pay-as-go | Performance |
| **Heroku** | ⭐⭐⭐ | No | $7+/mo | Legacy option |
| **DigitalOcean** | ⭐⭐⭐ | No | $5/mo | Simple VPS |

## My Recommendation: **Railway**

For your use case, Railway is the best choice because:
1. **Easiest setup** - Just connect GitHub and it works
2. **Handles audio uploads** - No special configuration needed
3. **Good free tier** - $5 credit usually covers development
4. **Automatic HTTPS** - No SSL certificate setup
5. **Environment variables** - Easy to manage secrets
6. **Great docs** - Excellent Python/Flask support

## Environment Variables

Create a `.env` file or set these in your cloud platform:

```bash
# Required
SUPABASE_URL=https://your-project.supabase.co

# Authentication Key - Use ONE of these options:
# Option A: New Secret Key (from "Publishable and secret API keys" section)
SUPABASE_SECRET_KEY=sb_secret_your_key_here

# Option B: Legacy service_role key (if you rotated JWT secret via support)
# SUPABASE_SERVICE_KEY=eyJ...your_service_role_key_here

# Optional: JWT Secret for fallback auth (if Secret Key doesn't work for auth)
# Get from Supabase Dashboard → Settings → API → JWT Secret
# SUPABASE_JWT_SECRET=your-jwt-secret

# AI Coaching
ANTHROPIC_API_KEY=sk-ant-your-key-here

# Server
PORT=5000
```

### Which key should you use?

| Scenario | Use This |
|----------|----------|
| Fresh setup with new Secret Key | `SUPABASE_SECRET_KEY` |
| Rotated JWT secret via support | `SUPABASE_SERVICE_KEY` (new service_role) |
| Auth issues with Secret Key | Add `SUPABASE_JWT_SECRET` as fallback |

The code will automatically try `SUPABASE_SECRET_KEY` first, then fall back to `SUPABASE_SERVICE_KEY`.

---

## Setup Files Needed

I'll create the necessary deployment files for Railway (and others):

