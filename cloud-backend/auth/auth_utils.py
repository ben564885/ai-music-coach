"""
Authentication utilities for verifying Supabase JWT tokens
"""

from functools import wraps
from flask import request, jsonify
from database.supabase_client import get_supabase_client
import os
from dotenv import load_dotenv

load_dotenv()


def verify_token(token: str) -> dict:
    """
    Verify Supabase JWT token using Supabase client
    Returns user_id and email if valid, None otherwise
    
    This approach is more secure than manually verifying JWTs
    because Supabase handles the verification server-side.
    """
    try:
        # Remove 'Bearer ' prefix if present
        if token.startswith('Bearer '):
            token = token[7:]
        
        # Use Supabase client to verify token
        supabase = get_supabase_client()
        user_response = supabase.auth.get_user(token)
        
        if user_response and user_response.user:
            return {
                'user_id': user_response.user.id,
                'email': user_response.user.email,
            }
        return None
    except Exception as e:
        # Log the error for debugging
        print(f"Token verification error: {str(e)}")
        return None


def require_auth(f):
    """Decorator to require authentication for Flask routes"""
    @wraps(f)
    def decorated_function(*args, **kwargs):
        auth_header = request.headers.get('Authorization')
        
        if not auth_header:
            return jsonify({'error': 'Authorization header required'}), 401
        
        user_info = verify_token(auth_header)
        
        if not user_info:
            return jsonify({'error': 'Invalid or expired token'}), 401
        
        # Add user info to request context
        request.user_id = user_info['user_id']
        request.user_email = user_info['email']
        
        return f(*args, **kwargs)
    
    return decorated_function

