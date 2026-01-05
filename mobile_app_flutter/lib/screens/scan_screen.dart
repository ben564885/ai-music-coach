
import 'package:flutter/material.dart';
import 'package:flutter_bloc/flutter_bloc.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:supabase_flutter/supabase_flutter.dart';
import 'package:mobile_app_flutter/blocs/auth/auth_bloc.dart';
import '../services/ble_service.dart';

class ScanScreen extends StatefulWidget {
  const ScanScreen({super.key});

  @override
  State<ScanScreen> createState() => _ScanScreenState();
}

class _ScanScreenState extends State<ScanScreen> {
  final _supabase = Supabase.instance.client;

  Future<void> _saveKnownDevice(BluetoothDevice device) async {
    try {
      final user = _supabase.auth.currentUser;
      if (user == null) return;

      final currentMetadata = user.userMetadata ?? {};
      final currentPrefs = Map<String, dynamic>.from(currentMetadata['preferences'] as Map? ?? {});
      final knownDevices = List<Map<String, dynamic>>.from(currentPrefs['known_devices'] as List? ?? []);

      // Check if device already exists
      final deviceId = device.remoteId.toString();
      final existingIndex = knownDevices.indexWhere((d) => d['id'] == deviceId);

      final deviceData = {
        'id': deviceId,
        'name': device.platformName.isNotEmpty ? device.platformName : 'Unknown Device',
        'lastConnected': DateTime.now().toIso8601String(),
      };

      if (existingIndex != -1) {
        knownDevices[existingIndex] = deviceData;
      } else {
        knownDevices.add(deviceData);
      }

      currentPrefs['known_devices'] = knownDevices;

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
      debugPrint('Error saving known device: $e');
    }
  }

  @override
  void initState() {
    super.initState();
    context.read<BleService>().startScan();
  }

  @override
  void dispose() {
    context.read<BleService>().stopScan();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Connect to Device')),
      body: StreamBuilder<List<ScanResult>>(
        stream: context.read<BleService>().scanResults,
        initialData: const [],
        builder: (context, snapshot) {
          final results = snapshot.data ?? [];
          return ListView.builder(
            itemCount: results.length,
            itemBuilder: (context, index) {
              final result = results[index];
              return ListTile(
                title: Text(result.device.platformName.isNotEmpty
                    ? result.device.platformName
                    : 'Unknown Device'),
                subtitle: Text(result.device.remoteId.toString()),
                trailing: ElevatedButton(
                  onPressed: () async {
                    try {
                      await context.read<BleService>().connectToDevice(result.device);
                      await _saveKnownDevice(result.device);
                      if (context.mounted) {
                        Navigator.pop(context, true);
                      }
                    } catch (e) {
                      if (context.mounted) {
                        ScaffoldMessenger.of(context).showSnackBar(
                          SnackBar(content: Text('Connection failed: $e')),
                        );
                      }
                    }
                  },
                  child: const Text('Connect'),
                ),
              );
            },
          );
        },
      ),
    );
  }
}
