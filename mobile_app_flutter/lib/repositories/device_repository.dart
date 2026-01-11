import 'package:supabase_flutter/supabase_flutter.dart';

/// Represents a linked PracticePod device
class Device {
  final String id;
  final String deviceId;  // Tuya device UUID
  final String userId;
  final String name;
  final DateTime createdAt;
  final DateTime? lastUploadAt;

  Device({
    required this.id,
    required this.deviceId,
    required this.userId,
    required this.name,
    required this.createdAt,
    this.lastUploadAt,
  });

  factory Device.fromJson(Map<String, dynamic> json) {
    return Device(
      id: json['id'],
      deviceId: json['device_id'],
      userId: json['user_id'],
      name: json['name'] ?? 'PracticePod',
      createdAt: DateTime.parse(json['created_at']).subtract(const Duration(hours: 8)),
      lastUploadAt: json['last_upload_at'] != null 
          ? DateTime.parse(json['last_upload_at']).subtract(const Duration(hours: 8))
          : null,
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'id': id,
      'device_id': deviceId,
      'user_id': userId,
      'name': name,
      'created_at': createdAt.toIso8601String(),
      'last_upload_at': lastUploadAt?.toIso8601String(),
    };
  }
}

/// Repository for managing device-user links
/// 
/// After pairing a PracticePod device via Tuya BLE, call [linkDevice] 
/// with the device's UUID to associate it with the current user's account.
/// 
/// All recordings uploaded from that device will then be saved to this user's account.
class DeviceRepository {
  final SupabaseClient _supabase;

  DeviceRepository({SupabaseClient? supabase})
      : _supabase = supabase ?? Supabase.instance.client;

  /// Get the current user's ID
  String? get _currentUserId => _supabase.auth.currentUser?.id;

  /// Link a device to the current user's account.
  /// 
  /// [deviceId] - The Tuya device UUID (e.g., "uuid2395651a4cae9262")
  /// [name] - Optional friendly name for the device
  /// 
  /// If the device is already linked to another user, it will be transferred
  /// to the current user (the previous user will lose access to future recordings).
  Future<Device?> linkDevice(String deviceId, {String name = 'PracticePod'}) async {
    final userId = _currentUserId;
    if (userId == null) {
      print('DeviceRepository: Cannot link device - no user logged in');
      return null;
    }

    try {
      // Use upsert to either create or update the device link
      // onConflict: 'device_id' means if device_id already exists, update instead of insert
      final response = await _supabase
          .from('devices')
          .upsert({
            'device_id': deviceId,
            'user_id': userId,
            'name': name,
          }, onConflict: 'device_id')
          .select()
          .single();
      
      print('DeviceRepository: Device $deviceId linked to user $userId');
      return Device.fromJson(response);
    } catch (e) {
      print('DeviceRepository: Error linking device: $e');
      return null;
    }
  }

  /// Get all devices linked to the current user
  Future<List<Device>> getDevices() async {
    final userId = _currentUserId;
    if (userId == null) {
      print('DeviceRepository: Cannot get devices - no user logged in');
      return [];
    }

    try {
      final response = await _supabase
          .from('devices')
          .select()
          .eq('user_id', userId)
          .order('created_at', ascending: false);

      return (response as List).map((json) => Device.fromJson(json)).toList();
    } catch (e) {
      print('DeviceRepository: Error fetching devices: $e');
      return [];
    }
  }

  /// Unlink a device from the current user's account
  /// 
  /// Returns true if successful, false otherwise.
  Future<bool> unlinkDevice(String deviceId) async {
    final userId = _currentUserId;
    if (userId == null) {
      print('DeviceRepository: Cannot unlink device - no user logged in');
      return false;
    }

    try {
      await _supabase
          .from('devices')
          .delete()
          .eq('device_id', deviceId)
          .eq('user_id', userId);
      
      print('DeviceRepository: Device $deviceId unlinked from user $userId');
      return true;
    } catch (e) {
      print('DeviceRepository: Error unlinking device: $e');
      return false;
    }
  }

  /// Check if a device is linked to the current user
  Future<bool> isDeviceLinked(String deviceId) async {
    final userId = _currentUserId;
    if (userId == null) return false;

    try {
      final response = await _supabase
          .from('devices')
          .select('id')
          .eq('device_id', deviceId)
          .eq('user_id', userId)
          .maybeSingle();
      
      return response != null;
    } catch (e) {
      print('DeviceRepository: Error checking device link: $e');
      return false;
    }
  }
}

