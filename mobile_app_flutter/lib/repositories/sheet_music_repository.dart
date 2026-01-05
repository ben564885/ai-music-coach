import 'package:supabase_flutter/supabase_flutter.dart';
import '../models/sheet_music.dart';

class SheetMusicRepository {
  final SupabaseClient _supabase;

  SheetMusicRepository(this._supabase);

  Future<List<SheetMusic>> getSheetMusic() async {
    final userId = _supabase.auth.currentUser?.id;
    if (userId == null) {
      print('SheetMusicRepository: No user ID');
      return [];
    }

    try {
      final response = await _supabase
          .from('sheet_music')
          .select()
          .eq('user_id', userId)
          .order('created_at', ascending: false);

      print('SheetMusicRepository: Fetched ${response.length} items');
      
      if (response.isEmpty) {
        print('SheetMusicRepository: No sheet music found for user $userId');
        return [];
      }

      return (response as List).map((json) {
        try {
          return SheetMusic.fromJson(json);
        } catch (e) {
          print('SheetMusicRepository: Error parsing sheet music: $e');
          print('SheetMusicRepository: JSON: $json');
          return null;
        }
      }).whereType<SheetMusic>().toList();
    } catch (e) {
      print('SheetMusicRepository: Error fetching sheet music: $e');
      return [];
    }
  }
}
