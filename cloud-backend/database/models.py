"""
Database models and schemas
"""

from datetime import datetime
from typing import Optional, List, Dict, Any
from dataclasses import dataclass, asdict, fields
import json
from dateutil import parser


@dataclass
class Recording:
    """Recording model"""
    id: Optional[str] = None
    user_id: str = None
    title: str = ""
    audio_url: str = ""
    sheet_music_id: Optional[str] = None
    mistakes: List[Dict[str, Any]] = None
    feedback: str = ""
    midi_raw: Optional[str] = None  # URL to MIDI file in Supabase Storage
    created_at: Optional[datetime] = None
    updated_at: Optional[datetime] = None
    
    def to_dict(self) -> dict:
        """Convert to dictionary for database storage"""
        data = asdict(self)
        if self.mistakes:
            data['mistakes'] = json.dumps(self.mistakes) if isinstance(self.mistakes, list) else self.mistakes
        if self.created_at:
            data['created_at'] = self.created_at.isoformat()
        if self.updated_at:
            data['updated_at'] = self.updated_at.isoformat()
        # midi_raw is now a URL string, no encoding needed
        return {k: v for k, v in data.items() if v is not None}
    
    @classmethod
    def from_dict(cls, data: dict) -> 'Recording':
        """Create Recording from dictionary"""
        if data.get('mistakes') and isinstance(data['mistakes'], str):
            data['mistakes'] = json.loads(data['mistakes'])
        if data.get('created_at'):
            data['created_at'] = parser.isoparse(data['created_at'])
        if data.get('updated_at'):
            data['updated_at'] = parser.isoparse(data['updated_at'])
        # midi_raw is now a URL string, no decoding needed
        # Filter out keys that turn into unexpected keyword arguments
        known_fields = {f.name for f in fields(cls)}
        filtered_data = {k: v for k, v in data.items() if k in known_fields}
        return cls(**filtered_data)


@dataclass
class SheetMusic:
    """Sheet music model"""
    id: Optional[str] = None
    user_id: str = None
    title: str = ""
    file_url: str = ""
    reference_data: Dict[str, Any] = None
    audiveris_raw_output: Optional[str] = None
    created_at: Optional[datetime] = None
    updated_at: Optional[datetime] = None
    
    def to_dict(self) -> dict:
        """Convert to dictionary for database storage"""
        data = asdict(self)
        # Keep reference_data as dict - Supabase will convert to JSONB automatically
        # Don't convert to JSON string, Supabase Python client handles dict -> JSONB conversion
        if self.created_at:
            data['created_at'] = self.created_at.isoformat()
        if self.updated_at:
            data['updated_at'] = self.updated_at.isoformat()
        return {k: v for k, v in data.items() if v is not None}
    
    @classmethod
    def from_dict(cls, data: dict) -> 'SheetMusic':
        """Create SheetMusic from dictionary"""
        if data.get('reference_data') and isinstance(data['reference_data'], str):
            data['reference_data'] = json.loads(data['reference_data'])
        if data.get('created_at'):
            data['created_at'] = parser.isoparse(data['created_at'])
        if data.get('updated_at'):
            data['updated_at'] = parser.isoparse(data['updated_at'])
        return cls(**data)


@dataclass
class Device:
    """Device model - links firmware devices to user accounts"""
    id: Optional[str] = None
    device_id: str = ""  # Tuya device UUID (e.g., "uuid2395651a4cae9262")
    user_id: str = ""    # Supabase auth.users UUID
    name: str = "PracticePod"
    created_at: Optional[datetime] = None
    last_upload_at: Optional[datetime] = None
    
    def to_dict(self) -> dict:
        """Convert to dictionary for database storage"""
        data = asdict(self)
        if self.created_at:
            data['created_at'] = self.created_at.isoformat()
        if self.last_upload_at:
            data['last_upload_at'] = self.last_upload_at.isoformat()
        return {k: v for k, v in data.items() if v is not None}
    
    @classmethod
    def from_dict(cls, data: dict) -> 'Device':
        """Create Device from dictionary"""
        if data.get('created_at'):
            data['created_at'] = parser.isoparse(data['created_at'])
        if data.get('last_upload_at'):
            data['last_upload_at'] = parser.isoparse(data['last_upload_at'])
        # Filter out keys that aren't in the dataclass
        known_fields = {f.name for f in fields(cls)}
        filtered_data = {k: v for k, v in data.items() if k in known_fields}
        return cls(**filtered_data)
