# Supabase Email Confirmation Setup

## Configuration Required

To enable email confirmation with deep links, you need to configure the redirect URL in your Supabase dashboard:

### Steps:

1. Go to your Supabase project dashboard
2. Navigate to **Authentication** → **URL Configuration**
3. Add the following to **Redirect URLs**:
   ```
   aimusiccoach://email-confirmation
   ```
4. Also add it to **Site URL** (if needed):
   ```
   aimusiccoach://email-confirmation
   ```

### How It Works:

1. User signs up with email/password
2. Supabase sends confirmation email with link containing `code` parameter
3. Link redirects to `aimusiccoach://email-confirmation?code=...`
4. App opens via deep link
5. Supabase SDK automatically handles the code and confirms the email
6. User sees success screen: "Account Created! Your email has been confirmed."
7. User can then sign in

### Testing:

1. Sign up with a new email
2. Check your email for the confirmation link
3. Click the link (on your phone or computer)
4. If on phone: App should open automatically
5. If on computer: You'll need to manually open the app (deep links work best on mobile)
6. You should see the "Account Created!" success screen

### Note:

- The deep link scheme `aimusiccoach://` is configured in:
  - iOS: `ios/Runner/Info.plist`
  - Android: `android/app/src/main/AndroidManifest.xml`
- The redirect URL `aimusiccoach://email-confirmation` is set in `auth_repository.dart`

