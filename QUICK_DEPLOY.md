# Quick Cloud Deployment Guide

## 🚀 Recommended: Railway (Easiest)

### Step 1: Prepare Your Code

Your code is already ready! Just make sure:
- ✅ `requirements.txt` includes `gunicorn`
- ✅ `Procfile` exists (I created it)
- ✅ Environment variables are in `.env` (don't commit this!)

### Step 2: Deploy to Railway

1. **Go to Railway**: https://railway.app
2. **Sign up** with GitHub
3. **New Project** → **Deploy from GitHub repo**
4. **Select repository**: `ai_music_coach`
5. **Set root directory**: `cloud-backend`
6. **Add environment variables**:
   ```
   SUPABASE_URL=https://zugupjngontrrcroykot.supabase.co
   SUPABASE_SERVICE_KEY=your_service_key
   SUPABASE_JWT_SECRET=your_jwt_secret
   ANTHROPIC_API_KEY=your_anthropic_key
   PORT=5000
   ```
7. **Deploy!** Railway will:
   - Detect Python
   - Install dependencies
   - Run your app
   - Give you a URL like `https://your-app.railway.app`

### Step 3: Update Mobile App

Edit `mobile-app/src/config.js`:
```javascript
export const API_BASE_URL = 'https://your-app.railway.app';
```

### Step 4: Test

```bash
curl https://your-app.railway.app/health
```

Should return: `{"status":"healthy","timestamp":"..."}`

## 🎯 That's It!

Railway handles:
- ✅ HTTPS/SSL automatically
- ✅ Auto-deploys on git push
- ✅ Environment variables
- ✅ Logs and monitoring
- ✅ Scaling (if needed)

## Alternative: Render.com

1. Go to https://render.com
2. New → Web Service
3. Connect GitHub repo
4. Settings:
   - Root Directory: `cloud-backend`
   - Build: `pip install -r requirements.txt`
   - Start: `gunicorn server:app --bind 0.0.0.0:$PORT`
5. Add environment variables
6. Deploy!

## Cost Comparison

- **Railway**: Free $5 credit/month → ~$5-10/month after
- **Render**: Free (spins down) → $7/month always-on
- **Fly.io**: Free tier → Pay-as-you-go (~$3-5/month)

## Update Mobile App Config

Once deployed, update `mobile-app/src/config.js`:

```javascript
export const API_BASE_URL = __DEV__ 
  ? 'http://10.0.0.146:5000'  // Local development
  : 'https://your-app.railway.app';  // Production
```

## Pro Tips

1. **Use Railway's free tier** for development/testing
2. **Set up auto-deploy** - pushes to main branch auto-deploy
3. **Monitor logs** in Railway dashboard
4. **Use Railway's PostgreSQL** if you want (but Supabase is fine)
5. **Scale up** only when needed (Railway auto-scales)

Your backend will be live on the internet in ~5 minutes! 🎉

