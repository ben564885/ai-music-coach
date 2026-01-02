"""
Authentication utilities for verifying Supabase JWT tokens
"""

from functools import wraps
from flask import request, jsonify
from database.supabase_client import get_supabase_client
import os
from dotenv import load_dotenv

load_dotenv()

# Optional: JWT secret for fallback manual verification
# Only needed if supabase.auth.get_user() doesn't work with your key
JWT_SECRET = os.getenv('SUPABASE_JWT_SECRET')


def verify_token(token: str) -> dict:
    """
    Verify Supabase JWT token.
    
    Tries multiple methods:
    1. Supabase client's auth.get_user() - works with service_role key
    2. Manual JWT decode - works if you have JWT_SECRET set
    
    Returns user_id and email if valid, None otherwise.
    """
    # Remove 'Bearer ' prefix if present
    if token.startswith('Bearer '):
        token = token[7:]
    
    # Method 1: Try Supabase client's auth API
    try:
        supabase = get_supabase_client()
        user_response = supabase.auth.get_user(token)
        
        if user_response and user_response.user:
            return {
                'user_id': user_response.user.id,
                'email': user_response.user.email,
            }
    except Exception as e:
        print(f"Supabase auth.get_user() failed: {str(e)}")
        # Fall through to try manual JWT verification
    
    # Method 2: Fallback to manual JWT decode (if JWT_SECRET is set)
    if JWT_SECRET:
        try:
            from jose import jwt, JWTError
            payload = jwt.decode(token, JWT_SECRET, algorithms=['HS256'])
            
            user_id = payload.get('sub')
            email = payload.get('email')
            
            if user_id:
                return {
                    'user_id': user_id,
                    'email': email,
                }
        except ImportError:
            print("python-jose not installed. Run: pip install python-jose")
        except JWTError as e:
            print(f"JWT decode failed: {str(e)}")
        except Exception as e:
            print(f"JWT verification error: {str(e)}")
    
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

