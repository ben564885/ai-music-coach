import 'package:flutter/material.dart';
import 'package:flutter_bloc/flutter_bloc.dart';
import 'package:supabase_flutter/supabase_flutter.dart' hide AuthState;
import 'package:mobile_app_flutter/blocs/auth/auth_bloc.dart';
import 'package:mobile_app_flutter/utils/app_theme.dart';
import 'package:mobile_app_flutter/widgets/stat_card.dart';
import 'package:mobile_app_flutter/widgets/gradient_card.dart';
import 'package:mobile_app_flutter/repositories/stats_repository.dart';
import 'package:mobile_app_flutter/screens/edit_profile_screen.dart';
import 'package:mobile_app_flutter/screens/audio_settings_screen.dart';
import 'package:mobile_app_flutter/screens/settings_detail_screen.dart';
import 'package:mobile_app_flutter/screens/devices_screen.dart';
import 'package:mobile_app_flutter/screens/instrument_selection_screen.dart';
import 'package:mobile_app_flutter/utils/app_localizations.dart';

class ProfileScreen extends StatelessWidget {
  const ProfileScreen({super.key});

  @override
  Widget build(BuildContext context) {
    final user = context.select((AuthBloc bloc) => bloc.state.user);
    final email = user?.email ?? 'Unknown User';
    final fullName = user?.userMetadata?['full_name'] as String?;
    final username = (fullName != null && fullName.trim().isNotEmpty) 
        ? fullName 
        : email.split('@')[0];
    final initials = (fullName != null && fullName.trim().isNotEmpty)
        ? fullName.trim().split(' ').map((e) => e[0]).take(2).join().toUpperCase()
        : (email.isNotEmpty ? email[0].toUpperCase() : '?');
    final l10n = AppLocalizations.of(context);

    return BlocListener<AuthBloc, AuthState>(
      listener: (context, state) {
        // When user logs out, navigate back to login screen
        if (state.status == AuthStatus.unauthenticated) {
          Navigator.of(context).popUntil((route) => route.isFirst);
        }
      },
      child: Scaffold(
      body: Container(
        decoration: const BoxDecoration(
          gradient: AppTheme.backgroundGradient,
        ),
        child: CustomScrollView(
        slivers: [
          // Profile Header
          SliverAppBar.large(
            expandedHeight: 250,
            pinned: true,
            backgroundColor: Colors.transparent,
            flexibleSpace: FlexibleSpaceBar(
              background: Container(
                decoration: BoxDecoration(
                  gradient: LinearGradient(
                    begin: Alignment.topCenter,
                    end: Alignment.bottomCenter,
                    colors: [
                      AppTheme.primaryColor.withOpacity(0.3),
                      Colors.transparent,
                    ],
                  ),
                ),
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    const SizedBox(height: 60),
                    Container(
                      width: 100,
                      height: 100,
                      decoration: BoxDecoration(
                        gradient: AppTheme.primaryGradient,
                        shape: BoxShape.circle,
                        boxShadow: AppTheme.glowShadow,
                        border: Border.all(
                          color: Colors.white,
                          width: 2,
                        ),
                        image: (user?.userMetadata?['avatar_url'] != null)
                            ? DecorationImage(
                                image: NetworkImage(user!.userMetadata?['avatar_url']),
                                fit: BoxFit.cover,
                              )
                            : null,
                      ),
                      child: (user?.userMetadata?['avatar_url'] == null)
                          ? Center(
                              child: Text(
                                initials,
                                style: Theme.of(context).textTheme.displayMedium,
                              ),
                            )
                          : null,
                    ),
                    const SizedBox(height: AppTheme.spacingM),
                    Text(
                      username,
                      style: Theme.of(context).textTheme.headlineMedium?.copyWith(
                            fontWeight: FontWeight.bold,
                          ),
                    ),
                    Text(
                      email,
                      style: Theme.of(context).textTheme.bodyMedium,
                    ),
                  ],
                ),
              ),
            ),
          ),

          // Statistics Overview
          Sliverpadding(
            padding: const EdgeInsets.all(AppTheme.spacingM),
            sliver: SliverToBoxAdapter(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    'Overview',
                    style: Theme.of(context).textTheme.headlineSmall?.copyWith(
                          fontWeight: FontWeight.bold,
                        ),
                  ),
                  FutureBuilder<UserStats>(
                    future: context.read<StatsRepository>().getUserStats(),
                    builder: (context, snapshot) {
                      final stats = snapshot.data;
                      final isLoading = snapshot.connectionState == ConnectionState.waiting;
                      
                      return Column(
                        children: [
                          Row(
                            children: [
                              Expanded(
                                child: StatCard(
                                  icon: Icons.timer,
                                  label: 'Total Practice',
                                  value: isLoading ? '...' : (stats?.totalPracticeTimeFormatted ?? '0h'),
                                  gradient: AppTheme.accentGradient,
                                  iconColor: AppTheme.secondaryColor,
                                ),
                              ),
                              const SizedBox(width: AppTheme.spacingM),
                              Expanded(
                                child: StatCard(
                                  icon: Icons.local_fire_department,
                                  label: 'Day Streak',
                                  value: isLoading ? '...' : '${stats?.streakDays ?? 0}',
                                  iconColor: AppTheme.warningColor,
                                ),
                              ),
                            ],
                          ),
                          const SizedBox(height: AppTheme.spacingM),
                          Row(
                            children: [
                              Expanded(
                                child: StatCard(
                                  icon: Icons.music_note,
                                  label: l10n.translate('total_sessions'),
                                  value: isLoading ? '...' : '${stats?.totalSessions ?? 0}',
                                  iconColor: AppTheme.primaryLight,
                                ),
                              ),
                              const SizedBox(width: AppTheme.spacingM),
                              Expanded(
                                child: StatCard(
                                  icon: Icons.trending_up,
                                  label: l10n.translate('avg_score'),
                                  value: isLoading ? '...' : '${stats?.averageAccuracy.round() ?? 0}%',
                                  iconColor: AppTheme.successColor,
                                ),
                              ),
                            ],
                          ),
                        ],
                      );
                    },
                  ),
                ],
              ),
            ),
          ),

          // Settings Section
          SliverPadding(
            padding: const EdgeInsets.all(AppTheme.spacingM),
            sliver: SliverList(
              delegate: SliverChildListDelegate([
                _buildSettingsSection(context, l10n.translate('settings'), [
                  _buildSettingsTile(
                    context,
                    Icons.person_outline,
                    l10n.translate('edit_profile'),
                    () => Navigator.push(
                      context,
                      MaterialPageRoute(builder: (context) => const EditProfileScreen()),
                    ),
                  ),
                  _buildSettingsTile(
                    context,
                    Icons.notifications_outlined,
                    l10n.translate('notifications'),
                    () => Navigator.push(
                      context,
                      MaterialPageRoute(
                        builder: (context) => SettingsDetailScreen(
                          type: SettingsType.notifications,
                          title: l10n.translate('notifications'),
                        ),
                      ),
                    ),
                  ),
                ]),
                const SizedBox(height: AppTheme.spacingL),
                _buildSettingsSection(context, 'App', [
                  _buildSettingsTile(
                    context,
                    Icons.mic_none,
                    l10n.translate('audio_settings'),
                    () => Navigator.push(
                      context,
                      MaterialPageRoute(builder: (context) => const AudioSettingsScreen()),
                    ),
                  ),
                  _buildInstrumentTile(context, user),
                  _buildSettingsTile(
                    context,
                    Icons.bluetooth,
                    l10n.translate('devices'),
                    () => Navigator.push(
                      context,
                      MaterialPageRoute(builder: (context) => const DevicesScreen()),
                    ),
                  ),
                  _buildSettingsTile(
                    context,
                    Icons.language,
                    l10n.translate('language'),
                    () => Navigator.push(
                      context,
                      MaterialPageRoute(
                        builder: (context) => SettingsDetailScreen(
                          type: SettingsType.language,
                          title: l10n.translate('language'),
                        ),
                      ),
                    ),
                  ),
                ]),
                const SizedBox(height: AppTheme.spacingL),
                _buildSettingsSection(context, l10n.translate('help_support'), [
                  _buildSettingsTile(
                    context,
                    Icons.help_outline,
                    l10n.translate('help_support'),
                    () => Navigator.push(
                      context,
                      MaterialPageRoute(
                        builder: (context) => SettingsDetailScreen(
                          type: SettingsType.help,
                          title: l10n.translate('help_support'),
                        ),
                      ),
                    ),
                  ),
                  _buildSettingsTile(
                    context,
                    Icons.info_outline,
                    l10n.translate('about'),
                    () => Navigator.push(
                      context,
                      MaterialPageRoute(
                        builder: (context) => SettingsDetailScreen(
                          type: SettingsType.about,
                          title: l10n.translate('about'),
                        ),
                      ),
                    ),
                  ),
                ]),
                const SizedBox(height: AppTheme.spacingXL),
                // Sign Out Button
                Padding(
                  padding: const EdgeInsets.symmetric(horizontal: AppTheme.spacingM),
                  child: ElevatedButton.icon(
                    onPressed: () {
                      _showSignOutDialog(context);
                    },
                    icon: const Icon(Icons.logout),
                    label: Text(l10n.translate('sign_out')),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: AppTheme.surfaceColor,
                      foregroundColor: AppTheme.errorColor,
                      elevation: 0,
                      side: const BorderSide(color: AppTheme.errorColor),
                    ),
                  ),
                ),
                const SizedBox(height: 100), // Bottom padding
              ]),
            ),
          ),
        ],
      ),
    ),
      ),
    );
  }

  Widget _buildSettingsSection(BuildContext context, String title, List<Widget> children) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(
          title,
          style: Theme.of(context).textTheme.titleMedium?.copyWith(
                fontWeight: FontWeight.bold,
                color: Colors.white70,
              ),
        ),
        const SizedBox(height: AppTheme.spacingS),
        GradientCard(
          padding: EdgeInsets.zero,
          child: Column(
            children: children,
          ),
        ),
      ],
    );
  }

  Widget _buildSettingsTile(
    BuildContext context,
    IconData icon,
    String title,
    VoidCallback onTap,
  ) {
    return ListTile(
      leading: Icon(icon, color: Colors.white70),
      title: Text(
        title,
        style: Theme.of(context).textTheme.bodyLarge,
      ),
      trailing: const Icon(Icons.chevron_right, color: Colors.white38),
      onTap: onTap,
    );
  }

  Widget _buildInstrumentTile(BuildContext context, User? user) {
    // Get current instrument from user metadata
    String instrumentName = 'Not set';
    if (user != null) {
      final metadata = user.userMetadata ?? {};
      final prefs = metadata['preferences'] as Map<String, dynamic>? ?? {};
      instrumentName = prefs['instrument'] as String? ?? 'Not set';
    }

    // Get emoji for instrument
    String emoji = '🎵';
    switch (instrumentName.toLowerCase()) {
      case 'piano':
        emoji = '🎹';
        break;
      case 'violin':
        emoji = '🎻';
        break;
      case 'guitar':
        emoji = '🎸';
        break;
      case 'flute':
        emoji = '🎵';
        break;
      case 'clarinet':
        emoji = '🎼';
        break;
      case 'trumpet':
        emoji = '🎺';
        break;
      case 'saxophone':
        emoji = '🎷';
        break;
    }

    return ListTile(
      leading: Text(
        emoji,
        style: const TextStyle(fontSize: 24),
      ),
      title: Text(
        'Instrument',
        style: Theme.of(context).textTheme.bodyLarge,
      ),
      subtitle: Text(
        instrumentName,
        style: Theme.of(context).textTheme.bodySmall?.copyWith(
              color: Colors.white60,
            ),
      ),
      trailing: const Icon(Icons.chevron_right, color: Colors.white38),
      onTap: () => Navigator.push(
        context,
        MaterialPageRoute(
          builder: (context) => const InstrumentSelectionScreen(
            isFromSettings: true,
          ),
        ),
      ),
    );
  }

  void _showSignOutDialog(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        backgroundColor: AppTheme.surfaceColor,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(AppTheme.radiusL),
        ),
        title: Text(
          l10n.translate('sign_out'),
          style: Theme.of(context).textTheme.headlineSmall,
        ),
        content: Text(
          l10n.translate('sign_out_confirmation'),
          style: Theme.of(context).textTheme.bodyMedium,
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: Text(l10n.translate('cancel')),
          ),
          TextButton(
            onPressed: () {
              Navigator.pop(context);
              context.read<AuthBloc>().add(AuthSignOutRequested());
              // Pop all routes until we reach the root (login screen)
              Navigator.of(context).popUntil((route) => route.isFirst);
            },
            style: TextButton.styleFrom(foregroundColor: AppTheme.errorColor),
            child: Text(l10n.translate('sign_out')),
          ),
        ],
      ),
    );
  }
}

// Helper wrapper for SliverPadding to fix the typo in main code
class Sliverpadding extends StatelessWidget {
  final EdgeInsetsGeometry padding;
  final Widget sliver;

  const Sliverpadding({
    super.key,
    required this.padding,
    required this.sliver,
  });

  @override
  Widget build(BuildContext context) {
    return SliverPadding(padding: padding, sliver: sliver);
  }
}
