"""
Database repository for CRUD operations
"""

from typing import List, Optional
from datetime import datetime
from database.supabase_client import get_supabase_client
from database.models import Recording, SheetMusic, Device, Analysis, WiFiNetwork


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


class AnalysisRepository:
    """Repository for analysis operations - stores AI feedback results"""
    
    @staticmethod
    def create(analysis: Analysis) -> Analysis:
        """Create a new analysis entry"""
        supabase = get_supabase_client()
        data = analysis.to_dict()
        data['created_at'] = datetime.utcnow().isoformat()
        
        result = supabase.table('analyses').insert(data).execute()
        if result.data:
            return Analysis.from_dict(result.data[0])
        raise Exception("Failed to create analysis")
    
    @staticmethod
    def get_by_id(analysis_id: str) -> Optional[Analysis]:
        """Get analysis by ID"""
        supabase = get_supabase_client()
        result = supabase.table('analyses').select('*').eq('id', analysis_id).execute()
        if result.data:
            return Analysis.from_dict(result.data[0])
        return None
    
    @staticmethod
    def get_by_user(user_id: str, limit: int = 50) -> List[Analysis]:
        """Get all analyses for a user, ordered by most recent first"""
        supabase = get_supabase_client()
        result = supabase.table('analyses')\
            .select('*')\
            .eq('user_id', user_id)\
            .order('created_at', desc=True)\
            .limit(limit)\
            .execute()
        
        return [Analysis.from_dict(item) for item in result.data] if result.data else []
    
    @staticmethod
    def get_by_recording(recording_id: str) -> List[Analysis]:
        """Get all analyses for a specific recording"""
        supabase = get_supabase_client()
        result = supabase.table('analyses')\
            .select('*')\
            .eq('recording_id', recording_id)\
            .order('created_at', desc=True)\
            .execute()
        
        return [Analysis.from_dict(item) for item in result.data] if result.data else []
    
    @staticmethod
    def delete(analysis_id: str) -> bool:
        """Delete an analysis"""
        supabase = get_supabase_client()
        result = supabase.table('analyses').delete().eq('id', analysis_id).execute()
        return result.data is not None
    
    @staticmethod
    def update_status(analysis_id: str, status: str, score: int = 0, 
                      strength: str = "", improvement: str = "",
                      feedback_points: list = None, full_feedback: str = "") -> bool:
        """Update an analysis with results"""
        supabase = get_supabase_client()
        update_data = {
            'status': status,
            'score': score,
            'strength': strength,
            'improvement': improvement,
            'full_feedback': full_feedback
        }
        if feedback_points is not None:
            update_data['feedback_points'] = feedback_points
        
        result = supabase.table('analyses')\
            .update(update_data)\
            .eq('id', analysis_id)\
            .execute()
        return result.data is not None


class WiFiNetworkRepository:
    """Repository for WiFi network operations"""
    
    @staticmethod
    def get_active_network(device_id: str) -> Optional[WiFiNetwork]:
        """Get the active WiFi network for a device"""
        supabase = get_supabase_client()
        result = supabase.table('wifi_networks')\
            .select('*')\
            .eq('device_id', device_id)\
            .eq('is_active', True)\
            .limit(1)\
            .execute()
        
        if result.data and len(result.data) > 0:
            return WiFiNetwork.from_dict(result.data[0])
        return None
    
    @staticmethod
    def get_all_networks(device_id: str) -> List[WiFiNetwork]:
        """Get all WiFi networks for a device"""
        supabase = get_supabase_client()
        result = supabase.table('wifi_networks')\
            .select('*')\
            .eq('device_id', device_id)\
            .order('created_at', desc=True)\
            .execute()
        
        return [WiFiNetwork.from_dict(item) for item in result.data] if result.data else []
    
    @staticmethod
    def save_network(device_id: str, ssid: str, password: str) -> WiFiNetwork:
        """Save or update a WiFi network for a device"""
        print(f"[WiFiNetworkRepository] save_network called: device_id='{device_id}', ssid='{ssid}'")
        supabase = get_supabase_client()
        
        # First, deactivate all other networks for this device
        print(f"[WiFiNetworkRepository] Deactivating other networks for device '{device_id}'")
        supabase.table('wifi_networks')\
            .update({'is_active': False})\
            .eq('device_id', device_id)\
            .execute()
        
        # Check if this network already exists
        print(f"[WiFiNetworkRepository] Checking for existing network...")
        existing = supabase.table('wifi_networks')\
            .select('*')\
            .eq('device_id', device_id)\
            .eq('ssid', ssid)\
            .limit(1)\
            .execute()
        
        if existing.data and len(existing.data) > 0:
            # Update existing network
            print(f"[WiFiNetworkRepository] Updating existing network (id={existing.data[0]['id']})")
            result = supabase.table('wifi_networks')\
                .update({
                    'password': password,
                    'is_active': True,
                    'updated_at': 'now()'
                })\
                .eq('id', existing.data[0]['id'])\
                .execute()
            
            print(f"[WiFiNetworkRepository] Update result: {result.data}")
            if result.data and len(result.data) > 0:
                return WiFiNetwork.from_dict(result.data[0])
        else:
            # Create new network
            print(f"[WiFiNetworkRepository] Creating new network...")
            wifi_network = WiFiNetwork(
                device_id=device_id,
                ssid=ssid,
                password=password,
                is_active=True
            )
            
            network_dict = wifi_network.to_dict()
            print(f"[WiFiNetworkRepository] Inserting: {network_dict}")
            
            result = supabase.table('wifi_networks')\
                .insert(network_dict)\
                .execute()
            
            print(f"[WiFiNetworkRepository] Insert result: {result.data}")
            if result.data and len(result.data) > 0:
                return WiFiNetwork.from_dict(result.data[0])
        
        raise Exception(f"Failed to save WiFi network: device_id={device_id}, ssid={ssid}")
    
    @staticmethod
    def delete_network(device_id: str, ssid: str) -> bool:
        """Delete a WiFi network for a device"""
        supabase = get_supabase_client()
        result = supabase.table('wifi_networks')\
            .delete()\
            .eq('device_id', device_id)\
            .eq('ssid', ssid)\
            .execute()
        return result.data is not None

