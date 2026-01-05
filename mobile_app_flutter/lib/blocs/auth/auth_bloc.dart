
import 'dart:async';
import 'package:bloc/bloc.dart';
import 'package:equatable/equatable.dart';
import 'package:supabase_flutter/supabase_flutter.dart' as supabase;
import 'package:supabase_flutter/supabase_flutter.dart'; // Import for User type
import 'package:mobile_app_flutter/repositories/auth_repository.dart';

part 'auth_event.dart';
part 'auth_state.dart';

class AuthBloc extends Bloc<AuthEvent, AuthState> {
  final AuthRepository _authRepository;
  late final StreamSubscription<supabase.AuthState> _authStateSubscription;

  AuthBloc({required AuthRepository authRepository})
      : _authRepository = authRepository,
        super(const AuthState.unknown()) {
    on<AuthStarted>(_onAuthStarted);
    on<AuthSignInRequested>(_onAuthSignInRequested);
    on<AuthSignUpRequested>(_onAuthSignUpRequested);
    on<AuthGoogleSignInRequested>(_onAuthGoogleSignInRequested);
    on<AuthSignOutRequested>(_onAuthSignOutRequested);
    on<AuthUserMetadataUpdated>(_onAuthUserMetadataUpdated);

    _authStateSubscription = _authRepository.authStateChanges.listen((data) {
       add(AuthStarted());
    });
  }

  Future<void> _onAuthStarted(AuthStarted event, Emitter<AuthState> emit) async {
    final user = _authRepository.currentUser;
    if (user != null) {
      emit(AuthState.authenticated(user));
    } else {
      emit(const AuthState.unauthenticated());
    }
  }

  Future<void> _onAuthSignInRequested(
      AuthSignInRequested event, Emitter<AuthState> emit) async {
    emit(const AuthState.loading());
    try {
      await _authRepository.signInWithEmailAndPassword(
        email: event.email,
        password: event.password,
      );
    } catch (e) {
      emit(AuthState.error(e.toString()));
    }
  }

  Future<void> _onAuthSignUpRequested(
      AuthSignUpRequested event, Emitter<AuthState> emit) async {
    emit(const AuthState.loading());
    try {
      final response = await _authRepository.signUpWithEmailAndPassword(
        email: event.email,
        password: event.password,
      );
      
      // Check if user was created
      if (response.user != null) {
        // If session exists, user is immediately authenticated (email confirmation disabled)
        // If session is null, email confirmation is required
        if (response.session != null) {
          // User is immediately authenticated - auth state change listener will handle it
          // But emit authenticated state here to be safe
          emit(AuthState.authenticated(response.user!));
        } else {
          // Email confirmation required - show message
          emit(AuthState.error('Please check your email to confirm your account before signing in.'));
        }
      } else {
        emit(AuthState.error('Failed to create account. Please try again.'));
      }
    } catch (e) {
      emit(AuthState.error(e.toString()));
    }
  }

  Future<void> _onAuthGoogleSignInRequested(
      AuthGoogleSignInRequested event, Emitter<AuthState> emit) async {
    emit(const AuthState.loading());
    try {
      await _authRepository.signInWithGoogle();
    } catch (e) {
      emit(AuthState.error(e.toString()));
    }
  }

  Future<void> _onAuthSignOutRequested(
      AuthSignOutRequested event, Emitter<AuthState> emit) async {
    await _authRepository.signOut();
  }

  Future<void> _onAuthUserMetadataUpdated(
      AuthUserMetadataUpdated event, Emitter<AuthState> emit) async {
    // Force a re-fetch of the current user to get updated metadata
    final user = _authRepository.currentUser;
    if (user != null) {
      emit(AuthState.authenticated(user));
    }
  }

  @override
  Future<void> close() {
    _authStateSubscription.cancel();
    return super.close();
  }
}
