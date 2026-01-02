"""
Supabase client initialization and database utilities
"""

import os
from supabase import create_client, Client
from dotenv import load_dotenv

load_dotenv()

# Initialize Supabase client
supabase_url = os.getenv('SUPABASE_URL')

# Support both new Secret Key AND legacy service_role key
# Priority: SUPABASE_SECRET_KEY > SUPABASE_SERVICE_KEY
# - New Secret Key: From "Publishable and secret API keys" section
# - Legacy service_role: From "Project API keys" section (if you rotated JWT secret)
supabase_key = os.getenv('SUPABASE_SECRET_KEY') or os.getenv('SUPABASE_SERVICE_KEY')

if not supabase_url:
    raise ValueError("SUPABASE_URL must be set in .env")

if not supabase_key:
    raise ValueError(
        "Either SUPABASE_SECRET_KEY or SUPABASE_SERVICE_KEY must be set in .env.\n"
        "  - SUPABASE_SECRET_KEY: New key from 'Publishable and secret API keys' section\n"
        "  - SUPABASE_SERVICE_KEY: Legacy service_role key (if you contacted support to rotate)"
    )

supabase: Client = create_client(supabase_url, supabase_key)


def get_supabase_client() -> Client:
    """Get the Supabase client instance"""
    return supabase


def create_tables():
    """Create database tables if they don't exist"""
    # This will be run via SQL migrations in Supabase dashboard
    # Or we can use Supabase Python client to create tables programmatically
    pass

