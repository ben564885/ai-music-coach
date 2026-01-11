"""
Database repository for CRUD operations
"""

from typing import List, Optional
from datetime import datetime
from database.supabase_client import get_supabase_client
from database.models import Recording, SheetMusic, Device


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


class DeviceRepository:
    """Repository for device operations - links firmware devices to user accounts"""
    
    @staticmethod
    def link_device(device_id: str, user_id: str, name: str = "PracticePod") -> Device:
        """
        Link a device to a user account.
        If device already exists, updates the user_id (transfers ownership).
        """
        supabase = get_supabase_client()
        
        # Check if device already exists
        existing = supabase.table('devices').select('*').eq('device_id', device_id).execute()
        
        if existing.data:
            # Update existing device (transfer to new user)
            result = supabase.table('devices').update({
                'user_id': user_id,
                'name': name
            }).eq('device_id', device_id).execute()
        else:
            # Create new device link
            data = {
                'device_id': device_id,
                'user_id': user_id,
                'name': name,
                'created_at': datetime.utcnow().isoformat()
            }
            result = supabase.table('devices').insert(data).execute()
        
        if result.data:
            return Device.from_dict(result.data[0])
        raise Exception("Failed to link device")
    
    @staticmethod
    def get_user_by_device(device_id: str) -> Optional[str]:
        """
        Look up the user_id for a given device_id.
        Returns user_id (UUID string) if found, None otherwise.
        This is called by the firmware upload endpoint.
        """
        supabase = get_supabase_client()
        result = supabase.table('devices').select('user_id').eq('device_id', device_id).execute()
        
        if result.data and len(result.data) > 0:
            return result.data[0]['user_id']
        return None
    
    @staticmethod
    def get_devices_by_user(user_id: str) -> List[Device]:
        """Get all devices linked to a user"""
        supabase = get_supabase_client()
        result = supabase.table('devices')\
            .select('*')\
            .eq('user_id', user_id)\
            .order('created_at', desc=True)\
            .execute()
        
        return [Device.from_dict(item) for item in result.data] if result.data else []
    
    @staticmethod
    def unlink_device(device_id: str, user_id: str) -> bool:
        """
        Unlink a device from a user.
        Only allows unlinking if the device belongs to the user.
        """
        supabase = get_supabase_client()
        result = supabase.table('devices')\
            .delete()\
            .eq('device_id', device_id)\
            .eq('user_id', user_id)\
            .execute()
        return result.data is not None and len(result.data) > 0
    
    @staticmethod
    def update_last_upload(device_id: str) -> None:
        """Update the last_upload_at timestamp for a device"""
        supabase = get_supabase_client()
        supabase.table('devices').update({
            'last_upload_at': datetime.utcnow().isoformat()
        }).eq('device_id', device_id).execute()

