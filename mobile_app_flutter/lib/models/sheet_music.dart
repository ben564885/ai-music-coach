import 'dart:convert';

class SheetMusic {
  final String id;
  final String userId;
  final String title;
  final String fileUrl;
  final Map<String, dynamic> referenceData;
  final String? audiverisRawOutput;
  final DateTime createdAt;

  SheetMusic({
    required this.id,
    required this.userId,
    required this.title,
    required this.fileUrl,
    required this.referenceData,
    this.audiverisRawOutput,
    required this.createdAt,
  });

  factory SheetMusic.fromJson(Map<String, dynamic> json) {
    // Handle reference_data - it might be a Map, String (JSON), or null
    Map<String, dynamic> referenceData = {};
    if (json['reference_data'] != null) {
      if (json['reference_data'] is Map) {
        referenceData = Map<String, dynamic>.from(json['reference_data']);
      } else if (json['reference_data'] is String) {
        try {
          referenceData = Map<String, dynamic>.from(jsonDecode(json['reference_data']));
        } catch (e) {
          print('Error parsing reference_data: $e');
          referenceData = {};
        }
      }
    }
    
    return SheetMusic(
      id: json['id'].toString(),
      userId: json['user_id']?.toString() ?? '',
      title: json['title']?.toString() ?? 'Untitled',
      fileUrl: json['file_url']?.toString() ?? '',
      referenceData: referenceData,
      audiverisRawOutput: json['audiveris_raw_output']?.toString(),
      createdAt: json['created_at'] != null 
          ? DateTime.parse(json['created_at'].toString()).subtract(const Duration(hours: 8))
          : DateTime.now(),
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'id': id,
      'user_id': userId,
      'title': title,
      'file_url': fileUrl,
      'reference_data': referenceData,
      'audiveris_raw_output': audiverisRawOutput,
      'created_at': createdAt.toIso8601String(),
    };
  }
  String get keySignature => referenceData['key_signature'] ?? referenceData['key'] ?? 'C';

  String get friendlyKeySignature {
    String key = keySignature;
    // Replace minus with flat symbol 'b'
    if (key.contains('-')) {
      key = key.replaceAll('-', 'b');
    }
    // Replace plus with sharp symbol '#'
    if (key.contains('+')) {
      key = key.replaceAll('+', '#');
    }
    return key;
  }
}
