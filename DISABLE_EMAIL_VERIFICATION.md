# How to Disable Email Verification in Supabase

## Steps to Bypass Email Confirmation:

1. **Go to Supabase Dashboard**
   - Navigate to your project

2. **Open Authentication Settings**
   - Go to **Authentication** → **Providers** → **Email**

3. **Disable Email Confirmation**
   - Find the toggle for **"Confirm email"** or **"Enable email confirmations"**
   - Turn it **OFF**
   - Save changes

4. **That's it!**
   - Users will now be automatically authenticated after sign up
   - No email confirmation required
   - Users can sign in immediately after creating an account

## What Happens After Disabling:

- ✅ Users sign up → Immediately authenticated
- ✅ No email sent
- ✅ No confirmation link needed
- ✅ Users can sign in right away
- ✅ Works exactly like Google sign-in (instant authentication)

## Code Already Handles This:

The app code already supports both modes:
- **With email confirmation**: Shows message to check email
- **Without email confirmation**: Immediately authenticates user

No code changes needed - just disable it in Supabase dashboard!

