import 'package:flutter_dotenv/flutter_dotenv.dart';

class ApiConfig {
  /// Get the backend API base URL from environment variables
  /// REQUIRED: Set BACKEND_URL in .env file
  static String get backendUrl {
    // Try environment variable first
    final envUrl = dotenv.env['BACKEND_URL'] ?? dotenv.env['API_URL'];
    
    if (envUrl != null && envUrl.isNotEmpty) {
      return envUrl;
    }
    
    // No fallback - user must configure .env file
    throw Exception(
      'BACKEND_URL not configured!\n\n'
      'Please create a .env file in mobile_app_flutter/ with:\n'
      'BACKEND_URL=http://your-backend-ip:5001\n\n'
      'See README.md for setup instructions.'
    );
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

