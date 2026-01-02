"""
Authentication utilities for verifying Supabase JWT tokens
"""

from functools import wraps
from flask import request, jsonify
from jose import jwt, JWTError
import os
from dotenv import load_dotenv

load_dotenv()

# Supabase JWT secret (get from Supabase dashboard → Settings → API → JWT Secret)
JWT_SECRET = os.getenv('SUPABASE_JWT_SECRET')


def verify_token(token: str) -> dict:
    """
    Verify Supabase JWT token and return user info
    Returns user_id and email if valid, None otherwise
    """
    if not JWT_SECRET:
        raise ValueError("SUPABASE_JWT_SECRET must be set in .env")
    
    try:
        # Remove 'Bearer ' prefix if present
        if token.startswith('Bearer '):
            token = token[7:]
        
        # Decode and verify token
        payload = jwt.decode(token, JWT_SECRET, algorithms=['HS256'])
        
        # Extract user info from Supabase token structure
        user_id = payload.get('sub')
        email = payload.get('email')
        
        return {
            'user_id': user_id,
            'email': email,
            'payload': payload
        }
    except JWTError:
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

