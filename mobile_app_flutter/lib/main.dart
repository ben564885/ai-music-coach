

import 'package:flutter/material.dart';
import 'package:flutter_bloc/flutter_bloc.dart';
import 'package:supabase_flutter/supabase_flutter.dart' hide AuthState;
import 'package:mobile_app_flutter/repositories/auth_repository.dart';
import 'package:mobile_app_flutter/blocs/auth/auth_bloc.dart';
import 'package:mobile_app_flutter/screens/login_screen.dart';
import 'package:mobile_app_flutter/screens/home_screen.dart';
import 'package:mobile_app_flutter/screens/instrument_selection_screen.dart';
import 'package:mobile_app_flutter/services/ble_service.dart';
import 'package:mobile_app_flutter/services/audio_service.dart';
import 'package:mobile_app_flutter/repositories/recordings_repository.dart';
import 'package:mobile_app_flutter/repositories/stats_repository.dart';
import 'package:mobile_app_flutter/repositories/sheet_music_repository.dart';
import 'package:mobile_app_flutter/utils/app_theme.dart';
import 'package:mobile_app_flutter/utils/app_localizations.dart';
import 'package:flutter_localizations/flutter_localizations.dart';

import 'package:flutter_dotenv/flutter_dotenv.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  
  await dotenv.load(fileName: ".env");

  // TODO: Replace with your actual Supabase URL and Anon Key
  // You can also pass these via --dart-define at build time:
  // flutter run --dart-define=SUPABASE_URL=... --dart-define=SUPABASE_ANON_KEY=...
  final supabaseUrl = dotenv.env['EXPO_PUBLIC_SUPABASE_URL'] ?? '';
  final supabaseAnonKey = dotenv.env['EXPO_PUBLIC_SUPABASE_ANON_KEY'] ?? '';

  if (supabaseUrl.isEmpty || supabaseAnonKey.isEmpty) {
     throw Exception('Supabase keys not found in .env file');
  }

  await Supabase.initialize(
    url: supabaseUrl,
    anonKey: supabaseAnonKey,
  );

  // Deep link handling is done in AuthWrapper

  runApp(const MyApp());
}


// ... imports

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MultiRepositoryProvider(
      providers: [
        RepositoryProvider(create: (context) => AuthRepository()),
        RepositoryProvider(create: (context) => BleService()..init()),
        RepositoryProvider(create: (context) => AudioService()),
        RepositoryProvider(create: (context) => RecordingsRepository()),
        RepositoryProvider(create: (context) => StatsRepository()),
        RepositoryProvider(create: (context) => SheetMusicRepository(Supabase.instance.client)),
      ],
      child: BlocProvider(
        create: (context) => AuthBloc(
          authRepository: context.read<AuthRepository>(),
        )..add(AuthStarted()),
        child: BlocBuilder<AuthBloc, AuthState>(
          builder: (context, state) {
            final user = state.user;
            final metadata = user?.userMetadata ?? {};
            final prefs = metadata['preferences'] as Map<String, dynamic>? ?? {};
            final languageCode = _getLanguageCode(prefs['language']?.toString() ?? 'English');

            return MaterialApp(
              debugShowCheckedModeBanner: false,
              title: 'AI Music Coach',
              theme: AppTheme.darkTheme,
              locale: Locale(languageCode),
              supportedLocales: const [
                Locale('en'),
                Locale('es'),
                Locale('fr'),
                Locale('de'),
                Locale('ja'),
              ],
              localizationsDelegates: [
                const AppLocalizationsDelegate(),
                GlobalMaterialLocalizations.delegate,
                GlobalWidgetsLocalizations.delegate,
                GlobalCupertinoLocalizations.delegate,
              ],
              home: const AuthWrapper(),
            );
          },
        ),
      ),
    );
  }

  String _getLanguageCode(String language) {
    switch (language) {
      case 'Spanish': return 'es';
      case 'French': return 'fr';
      case 'German': return 'de';
      case 'Japanese': return 'ja';
      default: return 'en';
    }
  }
}

class AuthWrapper extends StatefulWidget {
  const AuthWrapper({super.key});

  @override
  State<AuthWrapper> createState() => _AuthWrapperState();
}

class _AuthWrapperState extends State<AuthWrapper> {
  bool _hasSelectedInstrument(User? user) {
    if (user == null) return false;
    final metadata = user.userMetadata ?? {};
    final prefs = metadata['preferences'] as Map<String, dynamic>? ?? {};
    return prefs['instrument'] != null;
  }

  @override
  Widget build(BuildContext context) {
    return BlocBuilder<AuthBloc, AuthState>(
      builder: (context, state) {
        if (state.status == AuthStatus.authenticated) {
          final user = state.user;
          // Check if user has selected an instrument
          if (!_hasSelectedInstrument(user)) {
            return const InstrumentSelectionScreen();
          }
          return const HomeScreen();
        }
        return const LoginScreen();
      },
    );
  }
}
