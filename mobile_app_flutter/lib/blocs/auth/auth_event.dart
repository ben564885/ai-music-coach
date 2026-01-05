part of 'auth_bloc.dart';

abstract class AuthEvent extends Equatable {
  const AuthEvent();

  @override
  List<Object> get props => [];
}

class AuthStarted extends AuthEvent {}

class AuthSignInRequested extends AuthEvent {
  final String email;
  final String password;

  const AuthSignInRequested(this.email, this.password);

  @override
  List<Object> get props => [email, password];
}

class AuthSignUpRequested extends AuthEvent {
  final String email;
  final String password;

  const AuthSignUpRequested(this.email, this.password);

  @override
  List<Object> get props => [email, password];
}

class AuthSignOutRequested extends AuthEvent {}

class AuthGoogleSignInRequested extends AuthEvent {}

class AuthUserMetadataUpdated extends AuthEvent {
  const AuthUserMetadataUpdated();
}
