import 'package:flutter/material.dart';
import 'package:mobile_app_flutter/utils/app_theme.dart';
import 'package:mobile_app_flutter/repositories/device_repository.dart';

/// Simple screen for manually linking a device by entering its UUID.
/// This bypasses Tuya BLE pairing and just links the device UUID to the user's account.
/// 
/// The device UUID is displayed on the PracticePod screen or printed on the device.
class ManualDeviceLinkScreen extends StatefulWidget {
  const ManualDeviceLinkScreen({super.key});

  @override
  State<ManualDeviceLinkScreen> createState() => _ManualDeviceLinkScreenState();
}

class _ManualDeviceLinkScreenState extends State<ManualDeviceLinkScreen> {
  final _deviceIdController = TextEditingController();
  final _deviceNameController = TextEditingController(text: 'My PracticePod');
  final _deviceRepo = DeviceRepository();
  
  bool _isLinking = false;
  String? _errorMessage;

  @override
  void dispose() {
    _deviceIdController.dispose();
    _deviceNameController.dispose();
    super.dispose();
  }

  Future<void> _linkDevice() async {
    final deviceId = _deviceIdController.text.trim();
    final deviceName = _deviceNameController.text.trim();

    if (deviceId.isEmpty) {
      setState(() => _errorMessage = 'Please enter a device ID');
      return;
    }

    // Basic validation - device IDs are typically alphanumeric
    if (!RegExp(r'^[a-zA-Z0-9_-]+$').hasMatch(deviceId)) {
      setState(() => _errorMessage = 'Invalid device ID format');
      return;
    }

    setState(() {
      _isLinking = true;
      _errorMessage = null;
    });

    try {
      final device = await _deviceRepo.linkDevice(
        deviceId,
        name: deviceName.isNotEmpty ? deviceName : 'PracticePod',
      );

      if (device != null) {
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
              content: Text('Device "${device.name}" linked successfully!'),
              backgroundColor: AppTheme.successColor,
            ),
          );
          Navigator.pop(context, true); // Return success
        }
      } else {
        setState(() => _errorMessage = 'Failed to link device. Please try again.');
      }
    } catch (e) {
      setState(() => _errorMessage = 'Error: $e');
    } finally {
      if (mounted) {
        setState(() => _isLinking = false);
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Link Device'),
        leading: IconButton(
          icon: const Icon(Icons.arrow_back_ios_new),
          onPressed: () => Navigator.pop(context),
        ),
      ),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(24),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            // Instructions Card
            Container(
              padding: const EdgeInsets.all(16),
              decoration: BoxDecoration(
                color: AppTheme.primaryColor.withOpacity(0.1),
                borderRadius: BorderRadius.circular(12),
                border: Border.all(color: AppTheme.primaryColor.withOpacity(0.3)),
              ),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Row(
                    children: [
                      Icon(Icons.info_outline, color: AppTheme.primaryColor),
                      const SizedBox(width: 8),
                      Text(
                        'How to find your Device ID',
                        style: Theme.of(context).textTheme.titleMedium?.copyWith(
                          fontWeight: FontWeight.bold,
                          color: AppTheme.primaryColor,
                        ),
                      ),
                    ],
                  ),
                  const SizedBox(height: 12),
                  const Text(
                    '1. Turn on your PracticePod\n'
                    '2. Look at the device screen or check the sticker on the back\n'
                    '3. Find the Device ID (starts with "uuid...")\n'
                    '4. Enter it below',
                    style: TextStyle(color: Colors.white70, height: 1.5),
                  ),
                ],
              ),
            ),
            
            const SizedBox(height: 32),
            
            // Device ID Input
            Text(
              'Device ID',
              style: Theme.of(context).textTheme.titleSmall?.copyWith(
                color: Colors.white70,
              ),
            ),
            const SizedBox(height: 8),
            TextField(
              controller: _deviceIdController,
              decoration: InputDecoration(
                hintText: 'e.g., uuid2395651a4cae9262',
                prefixIcon: const Icon(Icons.qr_code),
                filled: true,
                fillColor: AppTheme.surfaceColor,
                border: OutlineInputBorder(
                  borderRadius: BorderRadius.circular(12),
                  borderSide: BorderSide.none,
                ),
                focusedBorder: OutlineInputBorder(
                  borderRadius: BorderRadius.circular(12),
                  borderSide: BorderSide(color: AppTheme.primaryColor, width: 2),
                ),
              ),
              style: const TextStyle(color: Colors.white, fontFamily: 'monospace'),
              autocorrect: false,
              enableSuggestions: false,
            ),
            
            const SizedBox(height: 24),
            
            // Device Name Input
            Text(
              'Device Name (optional)',
              style: Theme.of(context).textTheme.titleSmall?.copyWith(
                color: Colors.white70,
              ),
            ),
            const SizedBox(height: 8),
            TextField(
              controller: _deviceNameController,
              decoration: InputDecoration(
                hintText: 'My PracticePod',
                prefixIcon: const Icon(Icons.label_outline),
                filled: true,
                fillColor: AppTheme.surfaceColor,
                border: OutlineInputBorder(
                  borderRadius: BorderRadius.circular(12),
                  borderSide: BorderSide.none,
                ),
                focusedBorder: OutlineInputBorder(
                  borderRadius: BorderRadius.circular(12),
                  borderSide: BorderSide(color: AppTheme.primaryColor, width: 2),
                ),
              ),
              style: const TextStyle(color: Colors.white),
            ),
            
            const SizedBox(height: 16),
            
            // Error Message
            if (_errorMessage != null)
              Container(
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: AppTheme.errorColor.withOpacity(0.1),
                  borderRadius: BorderRadius.circular(8),
                  border: Border.all(color: AppTheme.errorColor.withOpacity(0.3)),
                ),
                child: Row(
                  children: [
                    Icon(Icons.error_outline, color: AppTheme.errorColor, size: 20),
                    const SizedBox(width: 8),
                    Expanded(
                      child: Text(
                        _errorMessage!,
                        style: TextStyle(color: AppTheme.errorColor),
                      ),
                    ),
                  ],
                ),
              ),
            
            const SizedBox(height: 32),
            
            // Link Button
            ElevatedButton(
              onPressed: _isLinking ? null : _linkDevice,
              style: ElevatedButton.styleFrom(
                backgroundColor: AppTheme.primaryColor,
                foregroundColor: Colors.white,
                padding: const EdgeInsets.symmetric(vertical: 16),
                shape: RoundedRectangleBorder(
                  borderRadius: BorderRadius.circular(12),
                ),
              ),
              child: _isLinking
                  ? const SizedBox(
                      height: 20,
                      width: 20,
                      child: CircularProgressIndicator(
                        strokeWidth: 2,
                        color: Colors.white,
                      ),
                    )
                  : const Text(
                      'Link Device',
                      style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold),
                    ),
            ),
            
            const SizedBox(height: 24),
            
            // Help Text
            Container(
              padding: const EdgeInsets.all(16),
              decoration: BoxDecoration(
                color: AppTheme.surfaceColor,
                borderRadius: BorderRadius.circular(12),
              ),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Row(
                    children: [
                      Icon(Icons.help_outline, color: Colors.white54, size: 20),
                      const SizedBox(width: 8),
                      Text(
                        'What happens next?',
                        style: Theme.of(context).textTheme.titleSmall?.copyWith(
                          color: Colors.white70,
                          fontWeight: FontWeight.bold,
                        ),
                      ),
                    ],
                  ),
                  const SizedBox(height: 8),
                  const Text(
                    'Once linked, any recordings made on this device will automatically appear in your account. '
                    'Make sure your PracticePod is connected to the same WiFi network.',
                    style: TextStyle(color: Colors.white54, fontSize: 13, height: 1.4),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }
}

