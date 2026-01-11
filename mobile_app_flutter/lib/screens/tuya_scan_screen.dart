
import 'package:flutter/material.dart';
import 'package:tuya_home_sdk_flutter/tuya_home_sdk_flutter.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:mobile_app_flutter/utils/app_theme.dart';
import 'package:mobile_app_flutter/repositories/device_repository.dart';
import 'package:google_sign_in/google_sign_in.dart' as gsi;
import 'package:flutter_dotenv/flutter_dotenv.dart';
import 'dart:async';

class TuyaScanScreen extends StatefulWidget {
  const TuyaScanScreen({super.key});

  @override
  State<TuyaScanScreen> createState() => _TuyaScanScreenState();
}

class _TuyaScanScreenState extends State<TuyaScanScreen> {
  List<dynamic> _devices = [];
  bool _isScanning = false;
  bool _isTuyaLoggedIn = false;
  bool _isCheckingLogin = true;
  String _statusMessage = "Checking Tuya login status...";
  StreamSubscription? _scanSubscription;

  final _ssidController = TextEditingController();
  final _passwordController = TextEditingController();

  @override
  void initState() {
    super.initState();
    _checkTuyaLoginAndPermissions();
  }

  @override
  void dispose() {
    _scanSubscription?.cancel();
    _ssidController.dispose();
    _passwordController.dispose();
    super.dispose();
  }

  Future<void> _checkTuyaLoginAndPermissions() async {
    setState(() {
      _isCheckingLogin = true;
      _statusMessage = "Checking Tuya login status...";
    });
    
    // Request permissions
    await [
      Permission.location,
      Permission.bluetooth,
      Permission.bluetoothScan,
      Permission.bluetoothConnect,
      Permission.bluetoothAdvertise,
    ].request();
    
    // Check if logged into Tuya
    try {
      final tuyaUser = await TuyaHomeSdkFlutter.instance.getUserInfo();
      
      if (tuyaUser != null) {
        debugPrint("Tuya: Logged in as ${tuyaUser.email}");
        setState(() => _isTuyaLoggedIn = true);
        
        // Check for homes
        final homes = await TuyaHomeSdkFlutter.instance.getHomeList();
        debugPrint("Tuya Homes: ${homes.length}");
        
        if (homes.isEmpty) {
          setState(() => _statusMessage = "⚠️ No Tuya Home found!\nTap 'Create Home' below.");
        } else {
          setState(() => _statusMessage = "✓ Ready to scan! Put your device in pairing mode first.");
        }
      } else {
        debugPrint("Tuya: Not logged in");
        setState(() {
          _isTuyaLoggedIn = false;
          _statusMessage = "⚠️ Not logged into Tuya.\nTap 'Login with Google' below to connect.";
        });
      }
    } catch (e) {
      debugPrint("Tuya check error: $e");
      setState(() {
        _isTuyaLoggedIn = false;
        _statusMessage = "⚠️ Tuya connection error.\nTap 'Login with Google' to try again.";
      });
    }
    
    setState(() => _isCheckingLogin = false);
  }

  Future<void> _loginToTuyaWithGoogle() async {
    setState(() => _statusMessage = "Opening Google login...");
    
    try {
      // Use Google Sign-In to get token, then pass to Tuya
      final googleSignIn = await _triggerGoogleSignIn();
      if (googleSignIn == null) {
        setState(() => _statusMessage = "Google login cancelled.");
        return;
      }
      
      setState(() => _statusMessage = "Logging into Tuya...");
      
      final success = await TuyaHomeSdkFlutter.instance.loginByAuth2(
        type: 'gg',  // Google
        countryCode: '1',
        accessToken: googleSignIn,
      );
      
      if (success) {
        debugPrint("Tuya: Google OAuth login successful!");
        setState(() {
          _isTuyaLoggedIn = true;
          _statusMessage = "✓ Logged in! Checking for homes...";
        });
        
        // Check/create home
        final homes = await TuyaHomeSdkFlutter.instance.getHomeList();
        if (homes.isEmpty) {
          setState(() => _statusMessage = "✓ Logged in! Now tap 'Create Home' below.");
        } else {
          setState(() => _statusMessage = "✓ Ready to scan! Put your device in pairing mode first.");
        }
      } else {
        setState(() => _statusMessage = "❌ Tuya login failed. Try again.");
      }
    } catch (e) {
      debugPrint("Tuya Google login error: $e");
      setState(() => _statusMessage = "❌ Login error: $e");
    }
  }

  Future<String?> _triggerGoogleSignIn() async {
    try {
      // Get web client ID from environment
      final webClientId = dotenv.env['GOOGLE_WEB_CLIENT_ID'];
      if (webClientId == null || webClientId.isEmpty) {
        debugPrint("Google Web Client ID not found");
        return null;
      }
      
      // Initialize Google Sign In (same pattern as auth_repository)
      await gsi.GoogleSignIn.instance.initialize(
        serverClientId: webClientId,
      );
      
      // Authenticate with Google
      final googleUser = await gsi.GoogleSignIn.instance.authenticate();
      final googleAuth = googleUser.authentication;
      
      // Google Sign-In 7.x only provides idToken
      // Tuya should accept this as the OAuth token
      return googleAuth.idToken;
    } catch (e) {
      debugPrint("Google sign-in error: $e");
      return null;
    }
  }

  Future<void> _createTuyaHome() async {
    // Show dialog to get home name
    final nameController = TextEditingController(text: "My Home");
    
    final homeName = await showDialog<String>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text("Create Tuya Home"),
        content: TextField(
          controller: nameController,
          decoration: const InputDecoration(labelText: "Home Name"),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text("Cancel"),
          ),
          ElevatedButton(
            onPressed: () => Navigator.pop(context, nameController.text),
            child: const Text("Create"),
          ),
        ],
      ),
    );
    
    if (homeName == null || homeName.isEmpty) return;
    
    setState(() => _statusMessage = "Creating home '$homeName'...");
    
    try {
      // Note: createHome might not exist - check Tuya SDK
      // For now, show instructions
      setState(() => _statusMessage = "⚠️ Please create a home in Smart Life app first,\nthen come back here.");
      
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text("Open Smart Life app → Create a Home → Return here"),
          duration: Duration(seconds: 5),
        ),
      );
    } catch (e) {
      debugPrint("Create home error: $e");
      setState(() => _statusMessage = "Error creating home: $e");
    }
  }

  void _showPermissionDialog() {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Permissions Required'),
        content: const Text(
          'Bluetooth and Location permissions are required to scan for devices.\n\n'
          'Please go to Settings and enable them for this app.',
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () {
              Navigator.pop(context);
              openAppSettings();
            },
            child: const Text('Open Settings'),
          ),
        ],
      ),
    );
  }

  Future<void> _startScan() async {
    setState(() {
      _devices = [];
      _isScanning = true;
      _statusMessage = "Scanning for devices in pairing mode...";
    });

    debugPrint("Starting Tuya device scan...");

    try {
      final stream = TuyaHomeSdkFlutter.instance.discoverDevices();
      debugPrint("Got discovery stream: $stream");
      
      _scanSubscription = stream.listen(
        (device) {
          debugPrint("Device found: ${device.name} - ${device.uuid}");
          setState(() {
            _statusMessage = "Found device: ${device.name}";
            if (!_devices.any((d) => d.uuid == device.uuid)) {
              _devices.add(device);
            }
          });
        },
        onError: (error) {
          debugPrint("Scan stream error: $error");
          setState(() {
            _statusMessage = "Scan error: $error";
            _isScanning = false;
          });
        },
        onDone: () {
          debugPrint("Scan stream completed");
          setState(() {
            if (_devices.isEmpty) {
              _statusMessage = "Scan complete. No devices found.";
            }
            _isScanning = false;
          });
        },
      );
      
      // Auto-stop after 30 seconds
      Future.delayed(const Duration(seconds: 30), () {
        if (_isScanning) {
          _stopScan();
          if (_devices.isEmpty) {
            setState(() => _statusMessage = "No devices found. Make sure device is in pairing mode.");
          }
        }
      });
      
    } catch (e) {
      debugPrint("Scan start error: $e");
      setState(() {
        _statusMessage = "Error starting scan: $e";
        _isScanning = false;
      });
    }
  }

  void _stopScan() {
    debugPrint("Stopping scan...");
    _scanSubscription?.cancel();
    _scanSubscription = null;
    setState(() {
      _isScanning = false;
      if (_devices.isEmpty) {
        _statusMessage = "Scan stopped. No devices found.";
      } else {
        _statusMessage = "Found ${_devices.length} device(s). Tap to pair.";
      }
    });
  }

  void _showWifiDialog(dynamic device) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Enter WiFi Credentials'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            TextField(
              controller: _ssidController,
              decoration: const InputDecoration(labelText: 'WiFi SSID'),
            ),
            TextField(
              controller: _passwordController,
              decoration: const InputDecoration(labelText: 'Password'),
              obscureText: true,
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () {
              Navigator.pop(context);
              _startActivation(device);
            },
            child: const Text('Pair'),
          ),
        ],
      ),
    );
  }

  Future<void> _startActivation(dynamic device) async {
    _stopScan();

    final ssid = _ssidController.text;
    final password = _passwordController.text;
    
    showDialog(
      context: context,
      barrierDismissible: false,
      builder: (ctx) => const Center(child: CircularProgressIndicator()),
    );

    try {
        final homes = await TuyaHomeSdkFlutter.instance.getHomeList();
        num homeId = 0;
        if (homes.isNotEmpty) {
           homeId = homes.first.homeId;
        } else {
           debugPrint("No Home Found!");
           Navigator.pop(context);
           ScaffoldMessenger.of(context).showSnackBar(
             const SnackBar(content: Text("No Tuya Home found!")),
           );
           return;
        }

        if (!mounted) return;

        debugPrint("Starting activation: homeId=$homeId, uuid=${device.uuid}, productId=${device.productId}");
        
        await TuyaHomeSdkFlutter.instance.startConfigBLEWifiDevice(
          ssid: ssid, 
          password: password, 
          homeId: homeId, 
          deviceUuid: device.uuid, 
          deviceProductId: device.productId ?? "", 
          timeout: 100
        ).then((dev) async {
            if (!mounted) return;
            Navigator.pop(context);
            if(dev != null) {
                // =========================================================
                // LINK DEVICE TO USER ACCOUNT
                // This connects the Tuya device to the logged-in user so
                // recordings from this device are saved to their account
                // =========================================================
                final deviceRepo = DeviceRepository();
                final linkedDevice = await deviceRepo.linkDevice(
                  device.uuid,
                  name: device.name ?? 'PracticePod',
                );
                
                if (linkedDevice != null) {
                  debugPrint("Device ${device.uuid} linked to user account!");
                  if (!mounted) return;
                  ScaffoldMessenger.of(context).showSnackBar(
                    const SnackBar(content: Text("Device paired and linked to your account!")),
                  );
                } else {
                  debugPrint("Warning: Device paired but failed to link to account");
                  if (!mounted) return;
                  ScaffoldMessenger.of(context).showSnackBar(
                    const SnackBar(content: Text("Device paired! (Note: account link may have failed)")),
                  );
                }
                
                Navigator.pop(context, true);
            }
        });
        
    } catch(e) {
        debugPrint("Activation error: $e");
        if (!mounted) return;
        Navigator.pop(context);
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text("Activation Error: $e")),
        );
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Device Pairing')),
      body: Column(
        children: [
          // Status Card
          Container(
            margin: const EdgeInsets.all(16),
            padding: const EdgeInsets.all(16),
            decoration: BoxDecoration(
              color: _isTuyaLoggedIn 
                  ? AppTheme.successColor.withOpacity(0.1)
                  : AppTheme.warningColor.withOpacity(0.1),
              borderRadius: BorderRadius.circular(12),
              border: Border.all(
                color: _isTuyaLoggedIn 
                    ? AppTheme.successColor.withOpacity(0.3)
                    : AppTheme.warningColor.withOpacity(0.3),
              ),
            ),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Row(
                  children: [
                    Icon(
                      _isTuyaLoggedIn ? Icons.check_circle : Icons.warning,
                      color: _isTuyaLoggedIn ? AppTheme.successColor : AppTheme.warningColor,
                    ),
                    const SizedBox(width: 8),
                    Text(
                      _isTuyaLoggedIn ? "Tuya Connected" : "Tuya Login Required",
                      style: Theme.of(context).textTheme.titleMedium?.copyWith(
                        fontWeight: FontWeight.bold,
                        color: _isTuyaLoggedIn ? AppTheme.successColor : AppTheme.warningColor,
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 8),
                Text(
                  _statusMessage,
                  style: const TextStyle(color: Colors.white70),
                ),
              ],
            ),
          ),
          
          // Login / Create Home Buttons (when not ready)
          if (!_isTuyaLoggedIn && !_isCheckingLogin) ...[
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 16),
              child: SizedBox(
                width: double.infinity,
                child: ElevatedButton.icon(
                  onPressed: _loginToTuyaWithGoogle,
                  icon: const Icon(Icons.login),
                  label: const Text("Login with Google"),
                  style: ElevatedButton.styleFrom(
                    backgroundColor: AppTheme.primaryColor,
                    foregroundColor: Colors.white,
                    padding: const EdgeInsets.all(16),
                  ),
                ),
              ),
            ),
            const SizedBox(height: 8),
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 16),
              child: Text(
                "You must login to Tuya to pair devices.\nThis is separate from your app account.",
                style: Theme.of(context).textTheme.bodySmall?.copyWith(
                  color: Colors.white54,
                ),
                textAlign: TextAlign.center,
              ),
            ),
          ],
          
          // Instructions (when logged in)
          if (_isTuyaLoggedIn) ...[
            Container(
              margin: const EdgeInsets.symmetric(horizontal: 16),
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: AppTheme.surfaceColor,
                borderRadius: BorderRadius.circular(8),
              ),
              child: const Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text("Before scanning:", style: TextStyle(fontWeight: FontWeight.bold, color: Colors.white)),
                  SizedBox(height: 4),
                  Text("1. Put your PracticePod in pairing mode", style: TextStyle(color: Colors.white70, fontSize: 13)),
                  Text("2. Hold the reset button for 5+ seconds", style: TextStyle(color: Colors.white70, fontSize: 13)),
                  Text("3. Device should show pairing indicator", style: TextStyle(color: Colors.white70, fontSize: 13)),
                ],
              ),
            ),
            const SizedBox(height: 16),
            
            // Scan Button
            Padding(
              padding: const EdgeInsets.symmetric(horizontal: 16),
              child: SizedBox(
                width: double.infinity,
                child: ElevatedButton.icon(
                  onPressed: _isScanning ? _stopScan : _startScan,
                  icon: _isScanning 
                      ? const SizedBox(
                          width: 20,
                          height: 20,
                          child: CircularProgressIndicator(strokeWidth: 2, color: Colors.white),
                        )
                      : const Icon(Icons.bluetooth_searching),
                  label: Text(_isScanning ? "Stop Scan" : "Start Scan"),
                  style: ElevatedButton.styleFrom(
                    backgroundColor: _isScanning ? AppTheme.errorColor : AppTheme.primaryColor,
                    foregroundColor: Colors.white,
                    padding: const EdgeInsets.all(16),
                  ),
                ),
              ),
            ),
          ],
          
          const SizedBox(height: 16),
          
          // Device List
          Expanded(
            child: _isCheckingLogin
                ? const Center(child: CircularProgressIndicator())
                : _devices.isEmpty
                    ? Center(
                        child: Text(
                          _isScanning ? "Scanning..." : (_isTuyaLoggedIn ? "No devices found" : ""),
                          style: Theme.of(context).textTheme.bodyLarge?.copyWith(
                            color: Colors.white54,
                          ),
                        ),
                      )
                    : ListView.builder(
                        itemCount: _devices.length,
                        itemBuilder: (ctx, i) {
                          final d = _devices[i];
                          return Card(
                            margin: const EdgeInsets.symmetric(horizontal: 16, vertical: 4),
                            child: ListTile(
                              leading: const Icon(Icons.devices, color: AppTheme.primaryColor),
                              title: Text(d.name ?? "Unknown Device"),
                              subtitle: Text("UUID: ${d.uuid ?? 'N/A'}"),
                              trailing: const Icon(Icons.arrow_forward_ios, size: 16),
                              onTap: () => _showWifiDialog(d),
                            ),
                          );
                        },
                      ),
          ),
        ],
      ),
    );
  }
}
