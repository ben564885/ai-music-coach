import 'package:flutter_dotenv/flutter_dotenv.dart';

class ApiConfig {
  /// Get the backend API base URL from environment variables
  /// Falls back to localhost:5001 if not set
  static String get backendUrl {
    // Try environment variable first
    final envUrl = dotenv.env['BACKEND_URL'] ?? dotenv.env['API_URL'];
    
    if (envUrl != null && envUrl.isNotEmpty) {
      return envUrl;
    }
    
    // Fallback to default (can be changed here or via .env file)
    return 'http://192.168.34.95:5001';
  }
  
  /// Get the full URL for a specific API endpoint
  static String getApiUrl(String endpoint) {
    // Remove leading slash if present
    final cleanEndpoint = endpoint.startsWith('/') ? endpoint.substring(1) : endpoint;
    return '$backendUrl/$cleanEndpoint';
  }
  
  /// Get the upload URL for sheet music
  static String get uploadSheetMusicUrl => getApiUrl('api/upload-sheet-music');
  
  /// Get the URL for serving uploaded files
  static String getUploadFileUrl(String filePath) {
    // If it's already a full URL, return it
    if (filePath.startsWith('http://') || filePath.startsWith('https://')) {
      return filePath;
    }
    
    // Extract filename - handle both "uploads/filename.jpg" and "filename.jpg"
    String fileName = filePath;
    if (filePath.contains('/')) {
      fileName = filePath.split('/').last;
    }
    
    return '$backendUrl/uploads/$fileName';
  }
}

