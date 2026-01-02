"""
Database repository for CRUD operations
"""

from typing import List, Optional
from datetime import datetime
from database.supabase_client import get_supabase_client
from database.models import Recording, SheetMusic


class RecordingRepository:
    """Repository for recording operations"""
    
    @staticmethod
    def create(recording: Recording) -> Recording:
        """Create a new recording"""
        supabase = get_supabase_client()
        data = recording.to_dict()
        data['created_at'] = datetime.utcnow().isoformat()
        data['updated_at'] = datetime.utcnow().isoformat()
        
        result = supabase.table('recordings').insert(data).execute()
        if result.data:
            return Recording.from_dict(result.data[0])
        raise Exception("Failed to create recording")
    
    @staticmethod
    def get_by_id(recording_id: str) -> Optional[Recording]:
        """Get recording by ID"""
        supabase = get_supabase_client()
        result = supabase.table('recordings').select('*').eq('id', recording_id).execute()
        if result.data:
            return Recording.from_dict(result.data[0])
        return None
    
    @staticmethod
    def get_by_user(user_id: str, limit: int = 50) -> List[Recording]:
        """Get all recordings for a user"""
        supabase = get_supabase_client()
        result = supabase.table('recordings')\
            .select('*')\
            .eq('user_id', user_id)\
            .order('created_at', desc=True)\
            .limit(limit)\
            .execute()
        
        return [Recording.from_dict(item) for item in result.data] if result.data else []
    
    @staticmethod
    def update(recording_id: str, updates: dict) -> Optional[Recording]:
        """Update a recording"""
        supabase = get_supabase_client()
        updates['updated_at'] = datetime.utcnow().isoformat()
        result = supabase.table('recordings').update(updates).eq('id', recording_id).execute()
        if result.data:
            return Recording.from_dict(result.data[0])
        return None
    
    @staticmethod
    def delete(recording_id: str) -> bool:
        """Delete a recording"""
        supabase = get_supabase_client()
        result = supabase.table('recordings').delete().eq('id', recording_id).execute()
        return result.data is not None


class SheetMusicRepository:
    """Repository for sheet music operations"""
    
    @staticmethod
    def create(sheet_music: SheetMusic) -> SheetMusic:
        """Create a new sheet music entry"""
        supabase = get_supabase_client()
        data = sheet_music.to_dict()
        data['created_at'] = datetime.utcnow().isoformat()
        data['updated_at'] = datetime.utcnow().isoformat()
        
        result = supabase.table('sheet_music').insert(data).execute()
        if result.data:
            return SheetMusic.from_dict(result.data[0])
        raise Exception("Failed to create sheet music")
    
    @staticmethod
    def get_by_id(sheet_music_id: str) -> Optional[SheetMusic]:
        """Get sheet music by ID"""
        supabase = get_supabase_client()
        result = supabase.table('sheet_music').select('*').eq('id', sheet_music_id).execute()
        if result.data:
            return SheetMusic.from_dict(result.data[0])
        return None
    
    @staticmethod
    def get_by_user(user_id: str, limit: int = 50) -> List[SheetMusic]:
        """Get all sheet music for a user"""
        supabase = get_supabase_client()
        result = supabase.table('sheet_music')\
            .select('*')\
            .eq('user_id', user_id)\
            .order('created_at', desc=True)\
            .limit(limit)\
            .execute()
        
        return [SheetMusic.from_dict(item) for item in result.data] if result.data else []
    
    @staticmethod
    def delete(sheet_music_id: str) -> bool:
        """Delete sheet music"""
        supabase = get_supabase_client()
        result = supabase.table('sheet_music').delete().eq('id', sheet_music_id).execute()
        return result.data is not None

