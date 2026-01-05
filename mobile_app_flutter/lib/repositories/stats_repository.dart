import 'package:supabase_flutter/supabase_flutter.dart';

class UserStats {
  final int totalPracticeSecondsWeek;
  final int totalPracticeSecondsAllTime;
  final int totalSessions;
  final double averageAccuracy;
  final int streakDays;

  UserStats({
    required this.totalPracticeSecondsWeek,
    required this.totalPracticeSecondsAllTime,
    required this.totalSessions,
    required this.averageAccuracy,
    required this.streakDays,
  });

  factory UserStats.fromJson(Map<String, dynamic> json) {
    return UserStats(
      totalPracticeSecondsWeek: json['total_practice_seconds_week'] ?? 0,
      totalPracticeSecondsAllTime: json['total_practice_seconds_all_time'] ?? 0,
      totalSessions: json['total_sessions'] ?? 0,
      averageAccuracy: (json['average_accuracy'] as num?)?.toDouble() ?? 0.0,
      streakDays: json['streak_days'] ?? 0,
    );
  }

  String get practiceTimeFormatted => _formatSeconds(totalPracticeSecondsWeek);
  String get totalPracticeTimeFormatted => _formatSeconds(totalPracticeSecondsAllTime);

  String _formatSeconds(int seconds) {
    if (seconds < 60) return '${seconds}s';
    if (seconds < 3600) return '${(seconds / 60).round()}m';
    return '${(seconds / 3600).toStringAsFixed(1)}h';
  }
}

class StatsRepository {
  final SupabaseClient _supabase;

  StatsRepository({SupabaseClient? supabase})
      : _supabase = supabase ?? Supabase.instance.client;

  Future<UserStats> getUserStats() async {
    try {
      final userId = _supabase.auth.currentUser?.id;
      if (userId == null) {
        return UserStats(
          totalPracticeSecondsWeek: 0,
          totalPracticeSecondsAllTime: 0,
          totalSessions: 0,
          averageAccuracy: 0,
          streakDays: 0,
        );
      }

      final response = await _supabase
          .from('user_practice_stats')
          .select()
          .eq('user_id', userId)
          .maybeSingle();
      
      if (response == null) {
        return UserStats(
          totalPracticeSecondsWeek: 0,
          totalPracticeSecondsAllTime: 0,
          totalSessions: 0,
          averageAccuracy: 0,
          streakDays: 0,
        );
      }
      
      return UserStats.fromJson(response);
    } catch (e) {
      return UserStats(
        totalPracticeSecondsWeek: 0,
        totalPracticeSecondsAllTime: 0,
        totalSessions: 0,
        averageAccuracy: 0,
        streakDays: 0,
      );
    }
  }
}
