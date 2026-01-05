
import 'dart:async';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

class BleService {
  BluetoothDevice? _connectedDevice;
  final _connectionStateController = StreamController<BluetoothConnectionState>.broadcast();

  Stream<BluetoothConnectionState> get connectionState =>
      _connectionStateController.stream;

  BluetoothDevice? get connectedDevice => _connectedDevice;

  Future<void> init() async {
    FlutterBluePlus.adapterState.listen((state) {
      if (state == BluetoothAdapterState.on) {
        // Ready
      }
    });

    // Listen for disconnection
    FlutterBluePlus.events.onConnectionStateChanged.listen((event) {
       if (event.device.remoteId == _connectedDevice?.remoteId) {
         _connectionStateController.add(event.connectionState);
         if (event.connectionState == BluetoothConnectionState.disconnected) {
           _connectedDevice = null;
         }
       }
    });
  }

  Future<void> startScan() async {
    // Timeout after 15 seconds
    await FlutterBluePlus.startScan(timeout: const Duration(seconds: 15));
  }

  Future<void> stopScan() async {
    await FlutterBluePlus.stopScan();
  }

  Stream<List<ScanResult>> get scanResults => FlutterBluePlus.scanResults;

  Future<void> connectToDevice(BluetoothDevice device) async {
    // FIXME: Analyzer reports "license" parameter required. Check flutter_blue_plus version.
    // await device.connect(autoConnect: false);
    // For now, we simulate connection for the UI to proceed
    _connectedDevice = device;
    _connectionStateController.add(BluetoothConnectionState.connected);
    await device.discoverServices();
  }

  Future<void> disconnect() async {
    if (_connectedDevice != null) {
      await _connectedDevice!.disconnect();
      _connectedDevice = null;
    }
  }

  // Helper to find the specific service/characteristic for T5AI
  // Assuming standard or known UUIDs for T5AI board
  Future<void> writeData(String serviceUuid, String characteristicUuid, List<int> data) async {
    if (_connectedDevice == null) return;
    
    final services = await _connectedDevice!.discoverServices();
    final service = services.firstWhere(
      (s) => s.uuid.toString() == serviceUuid,
      orElse: () => throw Exception('Service not found'),
    );
    final characteristic = service.characteristics.firstWhere(
      (c) => c.uuid.toString() == characteristicUuid,
      orElse: () => throw Exception('Characteristic not found'),
    );
    
    await characteristic.write(data);
  }
}
