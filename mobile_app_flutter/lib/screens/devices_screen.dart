import 'package:flutter/material.dart';
import 'package:supabase_flutter/supabase_flutter.dart';
import 'package:flutter_bloc/flutter_bloc.dart';
import 'package:mobile_app_flutter/utils/app_theme.dart';
import 'package:mobile_app_flutter/widgets/gradient_card.dart';
import 'package:mobile_app_flutter/blocs/auth/auth_bloc.dart';
import 'package:mobile_app_flutter/screens/scan_screen.dart';
import 'package:intl/intl.dart';

class DevicesScreen extends StatefulWidget {
  const DevicesScreen({super.key});

  @override
  State<DevicesScreen> createState() => _DevicesScreenState();
}

class _DevicesScreenState extends State<DevicesScreen> {
  final _supabase = Supabase.instance.client;
  List<Map<String, dynamic>> _knownDevices = [];

  @override
  void initState() {
    super.initState();
    _loadKnownDevices();
  }

  void _loadKnownDevices() {
    final user = _supabase.auth.currentUser;
    final metadata = user?.userMetadata ?? {};
    final prefs = metadata['preferences'] as Map<String, dynamic>? ?? {};
    final devices = prefs['known_devices'] as List? ?? [];
    
    setState(() {
      _knownDevices = List<Map<String, dynamic>>.from(devices);
      // Sort by last connected date descending
      _knownDevices.sort((a, b) {
        final dateA = DateTime.parse(a['lastConnected'] ?? DateTime.now().toIso8601String());
        final dateB = DateTime.parse(b['lastConnected'] ?? DateTime.now().toIso8601String());
        return dateB.compareTo(dateA);
      });
    });
  }

  Future<void> _removeDevice(int index) async {
    final deviceToRemove = _knownDevices[index];
    setState(() {
      _knownDevices.removeAt(index);
    });

    try {
      final user = _supabase.auth.currentUser;
      if (user == null) return;

      final currentMetadata = user.userMetadata ?? {};
      final currentPrefs = Map<String, dynamic>.from(currentMetadata['preferences'] as Map? ?? {});
      currentPrefs['known_devices'] = _knownDevices;

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
          SnackBar(content: Text('Error removing device: $e')),
        );
        _loadKnownDevices(); // Reload to restore state
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
                title: const Text(
                  'My Devices',
                  style: TextStyle(
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
            SliverToBoxAdapter(
              child: Padding(
                padding: const EdgeInsets.all(AppTheme.spacingM),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.stretch,
                  children: [
                    ElevatedButton.icon(
                      onPressed: () async {
                        final result = await Navigator.push(
                          context,
                          MaterialPageRoute(builder: (context) => const ScanScreen()),
                        );
                        if (result == true) {
                          _loadKnownDevices();
                        }
                      },
                      icon: const Icon(Icons.add),
                      label: const Text('Add New Device'),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: AppTheme.primaryLight,
                        foregroundColor: Colors.white,
                        padding: const EdgeInsets.symmetric(vertical: AppTheme.spacingM),
                      ),
                    ),
                    const SizedBox(height: AppTheme.spacingL),
                    Text(
                      'Past Connected Devices',
                      style: Theme.of(context).textTheme.titleMedium?.copyWith(
                            fontWeight: FontWeight.bold,
                            color: Colors.white70,
                          ),
                    ),
                    const SizedBox(height: AppTheme.spacingS),
                  ],
                ),
              ),
            ),
            if (_knownDevices.isEmpty)
              SliverFillRemaining(
                hasScrollBody: false,
                child: Center(
                  child: Column(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      Icon(Icons.bluetooth_disabled, size: 64, color: Colors.white.withOpacity(0.2)),
                      const SizedBox(height: AppTheme.spacingM),
                      Text(
                        'No saved devices yet',
                        style: TextStyle(color: Colors.white.withOpacity(0.5)),
                      ),
                    ],
                  ),
                ),
              )
            else
              SliverPadding(
                padding: const EdgeInsets.symmetric(horizontal: AppTheme.spacingM),
                sliver: SliverList(
                  delegate: SliverChildBuilderDelegate(
                    (context, index) {
                      final device = _knownDevices[index];
                      final lastConnected = DateTime.parse(device['lastConnected'] ?? DateTime.now().toIso8601String());
                      final formattedDate = DateFormat.yMMMd().add_jm().format(lastConnected);

                      return GradientCard(
                        margin: const EdgeInsets.only(bottom: AppTheme.spacingM),
                        child: ListTile(
                          leading: Container(
                            padding: const EdgeInsets.all(8),
                            decoration: BoxDecoration(
                              color: AppTheme.primaryLight.withOpacity(0.1),
                              shape: BoxShape.circle,
                            ),
                            child: const Icon(Icons.bluetooth, color: AppTheme.primaryLight),
                          ),
                          title: Text(
                            device['name'] ?? 'Unknown Device',
                            style: const TextStyle(fontWeight: FontWeight.bold, color: Colors.white),
                          ),
                          subtitle: Text(
                            'Last connected: $formattedDate',
                            style: const TextStyle(color: AppTheme.textSecondary, fontSize: 12),
                          ),
                          trailing: IconButton(
                            icon: const Icon(Icons.delete_outline, color: AppTheme.errorColor),
                            onPressed: () => _removeDevice(index),
                          ),
                        ),
                      );
                    },
                    childCount: _knownDevices.length,
                  ),
                ),
              ),
          ],
        ),
      ),
    );
  }
}
