"""
Supabase client initialization and database utilities
"""

import os
from supabase import create_client, Client
from dotenv import load_dotenv

load_dotenv()

# Initialize Supabase client
supabase_url = os.getenv('SUPABASE_URL')
supabase_service_key = os.getenv('SUPABASE_SERVICE_KEY')

if not supabase_url:
    raise ValueError("SUPABASE_URL must be set in .env")

if not supabase_service_key:
    raise ValueError("SUPABASE_SERVICE_KEY must be set in .env")

supabase: Client = create_client(supabase_url, supabase_service_key)


def get_supabase_client() -> Client:
    """Get the Supabase client instance"""
    return supabase


def upload_to_storage(bucket_name: str, file_path: str, file_data: bytes, content_type: str = 'audio/wav') -> str:
    """
    Upload a file to Supabase storage and return the public URL.
    
    Args:
        bucket_name: Name of the storage bucket (e.g., 'recordings')
        file_path: Path/name for the file in the bucket
        file_data: Binary file data
        content_type: MIME type of the file
        
    Returns:
        Public URL of the uploaded file
    """
    try:
        # Upload to Supabase storage
        result = supabase.storage.from_(bucket_name).upload(
            path=file_path,
            file=file_data,
            file_options={"content-type": content_type}
        )
        
        # Get public URL
        public_url = supabase.storage.from_(bucket_name).get_public_url(file_path)
        return public_url
        
    except Exception as e:
        print(f"Supabase storage upload error: {e}")
        raise


def create_tables():
    """Create database tables if they don't exist"""
    # This will be run via SQL migrations in Supabase dashboard
    # Or we can use Supabase Python client to create tables programmatically
    pass

