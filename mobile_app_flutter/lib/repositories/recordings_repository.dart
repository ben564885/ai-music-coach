
import 'package:supabase_flutter/supabase_flutter.dart';

class Recording {
  final String id;
  final String userId;
  final String? audioUrl;
  final DateTime createdAt;
  final String title;
  final int durationSeconds;
  final int? accuracyScore;
  final dynamic feedback; // JSON object with mistake markers

  Recording({
    required this.id,
    required this.userId,
    this.audioUrl,
    required this.createdAt,
    required this.title,
    this.durationSeconds = 0,
    this.accuracyScore,
    this.feedback,
  });

  factory Recording.fromJson(Map<String, dynamic> json) {
    return Recording(
      id: json['id'],
      userId: json['user_id'],
      audioUrl: json['audio_url'],
      createdAt: DateTime.parse(json['created_at']).subtract(const Duration(hours: 8)),
      title: json['title'] ?? 'Untitled Recording',
      durationSeconds: json['duration_seconds'] ?? 0,
      accuracyScore: json['accuracy_score'],
      feedback: json['feedback'],
    );
  }
}

class RecordingsRepository {
  final SupabaseClient _supabase;

  RecordingsRepository({SupabaseClient? supabase})
      : _supabase = supabase ?? Supabase.instance.client;

  Future<List<Recording>> getRecordings() async {
    try {
      final response = await _supabase
          .from('recordings')
          .select()
          .order('created_at', ascending: false);
      
      final data = response as List<dynamic>;
      return data.map((json) => Recording.fromJson(json)).toList();
    } catch (e) {
      // Handle error or rethrow
      return [];
    }
  }

  Future<Recording?> getRecording(String id) async {
    try {
      final response = await _supabase
          .from('recordings')
          .select()
          .eq('id', id)
          .single();
      
      return Recording.fromJson(response);
    } catch (e) {
      return null;
    }
  }

  Future<Recording?> updateRecording(String id, {String? title}) async {
    try {
      final updates = <String, dynamic>{};
      if (title != null) updates['title'] = title;
      
      if (updates.isEmpty) return null;
      
      final response = await _supabase
          .from('recordings')
          .update(updates)
          .eq('id', id)
          .select()
          .single();
      
      return Recording.fromJson(response);
    } catch (e) {
      print('RecordingsRepository: Error updating recording: $e');
      return null;
    }
  }

  Future<bool> deleteRecording(String id) async {
    try {
      await _supabase
          .from('recordings')
          .delete()
          .eq('id', id);
      return true;
    } catch (e) {
      print('RecordingsRepository: Error deleting recording: $e');
      return false;
    }
  }
}
