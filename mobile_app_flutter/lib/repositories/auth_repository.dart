import 'dart:convert';
import 'dart:math';
import 'package:crypto/crypto.dart';
import 'package:supabase_flutter/supabase_flutter.dart';
import 'package:google_sign_in/google_sign_in.dart' as gsi;
import 'package:flutter_dotenv/flutter_dotenv.dart';

class AuthRepository {
  final SupabaseClient _supabase;

  AuthRepository({SupabaseClient? supabase})
      : _supabase = supabase ?? Supabase.instance.client;

  Stream<AuthState> get authStateChanges => _supabase.auth.onAuthStateChange;

  User? get currentUser => _supabase.auth.currentUser;

  Future<AuthResponse> signInWithEmailAndPassword({
    required String email,
    required String password,
  }) async {
    return _supabase.auth.signInWithPassword(
      email: email,
      password: password,
    );
  }

  Future<AuthResponse> signUpWithEmailAndPassword({
    required String email,
    required String password,
  }) async {
    // For now, use default redirect URL
    // Once you add 'aimusiccoach://email-confirmation' to Supabase dashboard
    // (Authentication → URL Configuration → Redirect URLs), you can uncomment:
    // const redirectTo = 'aimusiccoach://email-confirmation';
    
    return _supabase.auth.signUp(
      email: email,
      password: password,
      // emailRedirectTo: redirectTo, // Uncomment after adding to Supabase dashboard
    );
  }

  /// Generates a random nonce string
  String _generateNonce([int length = 32]) {
    const charset = '0123456789ABCDEFGHIJKLMNOPQRSTUVXYZabcdefghijklmnopqrstuvwxyz-._';
    final random = Random.secure();
    return List.generate(length, (_) => charset[random.nextInt(charset.length)]).join();
  }

  /// Returns the SHA256 hash of the input string
  String _sha256ofString(String input) {
    final bytes = utf8.encode(input);
    final digest = sha256.convert(bytes);
    return digest.toString();
  }

  Future<AuthResponse> signInWithGoogle() async {
    // Web Client ID from Google Cloud Console - required for Supabase authentication
    final webClientId = dotenv.env['GOOGLE_WEB_CLIENT_ID'];
    
    if (webClientId == null || webClientId.isEmpty) {
      throw 'Google Web Client ID not found in environment.';
    }

    // Generate a nonce - raw nonce goes to Supabase, hashed nonce goes to Google
    final rawNonce = _generateNonce();
    final hashedNonce = _sha256ofString(rawNonce);

    // Initialize GoogleSignIn instance with web client ID and hashed nonce
    // The iOS client ID should be in Info.plist (GIDClientID) for native iOS auth
    await gsi.GoogleSignIn.instance.initialize(
      serverClientId: webClientId,
      nonce: hashedNonce,
    );

    // Authenticate with Google - this triggers the sign-in flow
    final googleUser = await gsi.GoogleSignIn.instance.authenticate();
    
    final googleAuth = googleUser.authentication;
    final idToken = googleAuth.idToken;

    if (idToken == null) {
      throw 'No ID Token found.';
    }

    // Pass the raw (unhashed) nonce to Supabase for verification
    return _supabase.auth.signInWithIdToken(
      provider: OAuthProvider.google,
      idToken: idToken,
      nonce: rawNonce,
    );
  }

  Future<void> signOut() async {
    await gsi.GoogleSignIn.instance.signOut();
    await _supabase.auth.signOut();
  }
}
