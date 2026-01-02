"""
Supabase client initialization and database utilities
"""

import os
from supabase import create_client, Client
from dotenv import load_dotenv

load_dotenv()

# Initialize Supabase client
supabase_url = os.getenv('SUPABASE_URL')
supabase_key = os.getenv('SUPABASE_SERVICE_KEY')  # Use service key for backend operations

if not supabase_url or not supabase_key:
    raise ValueError("SUPABASE_URL and SUPABASE_SERVICE_KEY must be set in .env")

supabase: Client = create_client(supabase_url, supabase_key)


def get_supabase_client() -> Client:
    """Get the Supabase client instance"""
    return supabase


def create_tables():
    """Create database tables if they don't exist"""
    # This will be run via SQL migrations in Supabase dashboard
    # Or we can use Supabase Python client to create tables programmatically
    pass

