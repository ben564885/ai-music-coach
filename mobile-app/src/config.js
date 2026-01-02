// API Configuration
export const API_BASE_URL = __DEV__ 
  ? 'http://10.0.0.146:5000'  // Development - your computer's IP for phone testing
  : 'https://your-app.railway.app';  // Production - update with your Railway/Render URL

// Supabase Configuration
// Get from Supabase Dashboard → Settings → API
export const SUPABASE_URL = process.env.EXPO_PUBLIC_SUPABASE_URL;
export const SUPABASE_ANON_KEY = process.env.EXPO_PUBLIC_SUPABASE_ANON_KEY;

// BLE Configuration
export const T5AI_DEVICE_NAME = 'T5AI-MusicCoach';
export const T5AI_SERVICE_UUID = '0000FFE0-0000-1000-8000-00805F9B34FB';
export const T5AI_CHARACTERISTIC_UUID = '0000FFE1-0000-1000-8000-00805F9B34FB';

// File Upload Settings
export const MAX_FILE_SIZE = 50 * 1024 * 1024; // 50MB
export const SUPPORTED_IMAGE_FORMATS = ['image/jpeg', 'image/png', 'image/jpg'];
export const SUPPORTED_DOCUMENT_FORMATS = ['application/pdf', 'application/xml', 'text/xml'];
