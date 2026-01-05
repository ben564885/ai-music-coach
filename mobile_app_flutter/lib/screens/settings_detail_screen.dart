import 'package:flutter/material.dart';
import 'package:supabase_flutter/supabase_flutter.dart';
import 'package:flutter_bloc/flutter_bloc.dart';
import 'package:mobile_app_flutter/utils/app_theme.dart';
import 'package:mobile_app_flutter/widgets/gradient_card.dart';
import 'package:mobile_app_flutter/blocs/auth/auth_bloc.dart';
import 'package:mobile_app_flutter/utils/app_localizations.dart';


enum SettingsType {
  notifications,
  language,
  help,
  about,
}

class SettingsDetailScreen extends StatefulWidget {
  final SettingsType type;
  final String title;

  const SettingsDetailScreen({
    super.key,
    required this.type,
    required this.title,
  });

  @override
  State<SettingsDetailScreen> createState() => _SettingsDetailScreenState();
}

class _SettingsDetailScreenState extends State<SettingsDetailScreen> {
  final _supabase = Supabase.instance.client;
  
  // Local state to manage toggles
  late Map<String, bool> _settingsState;
  late String _selectedLanguage;

  @override
  void initState() {
    super.initState();
    _loadInitialSettings();
  }

  void _loadInitialSettings() {
    final user = _supabase.auth.currentUser;
    final metadata = user?.userMetadata ?? {};
    final prefs = metadata['preferences'] as Map<String, dynamic>? ?? {};

    // Initialize with defaults if not found in preferences
    _settingsState = {
      'Push Notifications': prefs['Push Notifications'] ?? true,
      'Achievement Alerts': prefs['Achievement Alerts'] ?? true,
      'Weekly Summary': prefs['Weekly Summary'] ?? false,
      'New Exercises': prefs['New Exercises'] ?? false,
      'Public Profile': prefs['Public Profile'] ?? false,
      'Share Analytics': prefs['Share Analytics'] ?? true,
    };
    _selectedLanguage = prefs['language']?.toString() ?? 'English';
  }

  Future<void> _savePreference(String key, dynamic value) async {
    setState(() {
      if (key == 'language') {
        _selectedLanguage = value as String;
      } else {
        _settingsState[key] = value as bool;
      }
    });

    try {
      final user = _supabase.auth.currentUser;
      if (user == null) return;

      final currentMetadata = user.userMetadata ?? {};
      final currentPrefs = Map<String, dynamic>.from(currentMetadata['preferences'] as Map? ?? {});
      currentPrefs[key] = value;

      await _supabase.auth.updateUser(
        UserAttributes(
          data: {
            ...currentMetadata,
            'preferences': currentPrefs,
          },
        ),
      );

      if (mounted) {
        context.read<AuthBloc>().add(const AuthUserMetadataUpdated());
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Error saving preference: $e')),
        );
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Container(
        decoration: const BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topCenter,
            end: Alignment.bottomCenter,
            colors: [
              AppTheme.backgroundColor,
              Color(0xFF0F0C29),
            ],
          ),
        ),
        child: CustomScrollView(
          slivers: [
            SliverAppBar(
              expandedHeight: 120,
              pinned: true,
              backgroundColor: Colors.transparent,
              flexibleSpace: FlexibleSpaceBar(
                title: Text(
                  widget.title,
                  style: const TextStyle(
                    fontWeight: FontWeight.bold,
                    color: Colors.white,
                  ),
                ),
                centerTitle: false,
                titlePadding: const EdgeInsets.only(left: 56, bottom: 16),
              ),
              leading: IconButton(
                icon: const Icon(Icons.arrow_back_ios_new, color: Colors.white),
                onPressed: () => Navigator.pop(context),
              ),
            ),
            SliverPadding(
              padding: const EdgeInsets.all(AppTheme.spacingM),
              sliver: SliverList(
                delegate: SliverChildListDelegate([
                  _buildContent(context),
                ]),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildContent(BuildContext context) {
    switch (widget.type) {
      case SettingsType.notifications:
        return _buildNotificationSettings();
      case SettingsType.language:
        return _buildLanguageSettings();
      case SettingsType.help:
        return _buildHelpContent();
      case SettingsType.about:
        return _buildAboutContent();
    }
  }

  Widget _buildNotificationSettings() {
    final l10n = AppLocalizations.of(context);
    return Column(
      children: [
        _buildSettingToggle('Push Notifications', l10n.translate('push_notifications'), l10n.translate('push_notifications_desc')),
        _buildSettingToggle('Achievement Alerts', l10n.translate('achievement_alerts'), l10n.translate('achievement_alerts_desc')),
        _buildSettingToggle('Weekly Summary', l10n.translate('weekly_summary'), l10n.translate('weekly_summary_desc')),
        _buildSettingToggle('New Exercises', l10n.translate('new_exercises'), l10n.translate('new_exercises_desc')),
      ],
    );
  }

  Widget _buildLanguageSettings() {
    return Column(
      children: [
        _buildLanguageTile('English'),
        _buildLanguageTile('Spanish'),
        _buildLanguageTile('French'),
        _buildLanguageTile('German'),
        _buildLanguageTile('Japanese'),
      ],
    );
  }

  Widget _buildHelpContent() {
    final l10n = AppLocalizations.of(context);
    return Column(
      children: [
        _buildActionTile(Icons.question_answer_outlined, l10n.translate('faq'), l10n.translate('faq_desc')),
        _buildActionTile(Icons.mail_outline, l10n.translate('contact_support'), l10n.translate('contact_support_desc')),
        _buildActionTile(Icons.book_outlined, l10n.translate('tutorials'), l10n.translate('tutorials_desc')),
        _buildActionTile(Icons.bug_report_outlined, l10n.translate('report_bug'), l10n.translate('report_bug_desc')),
      ],
    );
  }

  Widget _buildAboutContent() {
    final l10n = AppLocalizations.of(context);
    return Column(
      children: [
        Center(
          child: Column(
            children: [
              const SizedBox(height: AppTheme.spacingXL),
              const Icon(Icons.music_note, size: 60, color: AppTheme.primaryLight),
              const SizedBox(height: AppTheme.spacingM),
              const Text(
                'AI Music Coach',
                style: TextStyle(
                  fontSize: 24,
                  fontWeight: FontWeight.bold,
                  color: Colors.white,
                ),
              ),
              Text(
                '${l10n.translate('version')} 1.0.0',
                style: const TextStyle(
                  color: AppTheme.textSecondary,
                ),
              ),
              const SizedBox(height: AppTheme.spacingXL),
            ],
          ),
        ),
        _buildActionTile(Icons.description_outlined, l10n.translate('terms_service'), l10n.translate('terms_service_desc')),
        _buildActionTile(Icons.privacy_tip_outlined, l10n.translate('privacy_policy'), l10n.translate('privacy_policy_desc')),
        _buildActionTile(Icons.code, l10n.translate('open_source'), l10n.translate('open_source_desc')),
      ],
    );
  }

  Widget _buildSettingToggle(String stateKey, String title, String subtitle) {
    return GradientCard(
      margin: const EdgeInsets.only(bottom: AppTheme.spacingM),
      child: SwitchListTile(
        title: Text(title, style: const TextStyle(fontWeight: FontWeight.bold, color: Colors.white)),
        subtitle: Text(subtitle, style: const TextStyle(color: AppTheme.textSecondary)),
        value: _settingsState[stateKey] ?? false,
        onChanged: (val) => _savePreference(stateKey, val),
        activeColor: AppTheme.primaryLight,
      ),
    );
  }

  Widget _buildActionTile(IconData icon, String title, String subtitle, {Color? color}) {
    return GradientCard(
      margin: const EdgeInsets.only(bottom: AppTheme.spacingM),
      child: ListTile(
        leading: Icon(icon, color: color ?? AppTheme.textSecondary),
        title: Text(title, style: TextStyle(fontWeight: FontWeight.bold, color: color ?? Colors.white)),
        subtitle: Text(subtitle, style: const TextStyle(color: AppTheme.textSecondary)),
        trailing: const Icon(Icons.chevron_right, color: AppTheme.textSecondary),
        onTap: () {},
      ),
    );
  }

  Widget _buildLanguageTile(String language) {
    return GradientCard(
      margin: const EdgeInsets.only(bottom: AppTheme.spacingM),
      child: RadioListTile<String>(
        title: Text(language, style: const TextStyle(fontWeight: FontWeight.bold, color: Colors.white)),
        value: language,
        groupValue: _selectedLanguage,
        onChanged: (val) {
          if (val != null) {
            _savePreference('language', val);
          }
        },
        activeColor: AppTheme.primaryLight,
      ),
    );
  }
}
