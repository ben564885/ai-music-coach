"""
Database models and schemas
"""

from datetime import datetime
from typing import Optional, List, Dict, Any
from dataclasses import dataclass, asdict
import json


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
        return {k: v for k, v in data.items() if v is not None}
    
    @classmethod
    def from_dict(cls, data: dict) -> 'Recording':
        """Create Recording from dictionary"""
        if data.get('mistakes') and isinstance(data['mistakes'], str):
            data['mistakes'] = json.loads(data['mistakes'])
        if data.get('created_at'):
            data['created_at'] = datetime.fromisoformat(data['created_at'].replace('Z', '+00:00'))
        if data.get('updated_at'):
            data['updated_at'] = datetime.fromisoformat(data['updated_at'].replace('Z', '+00:00'))
        return cls(**data)


@dataclass
class SheetMusic:
    """Sheet music model"""
    id: Optional[str] = None
    user_id: str = None
    title: str = ""
    file_url: str = ""
    reference_data: Dict[str, Any] = None
    created_at: Optional[datetime] = None
    updated_at: Optional[datetime] = None
    
    def to_dict(self) -> dict:
        """Convert to dictionary for database storage"""
        data = asdict(self)
        if self.reference_data:
            data['reference_data'] = json.dumps(self.reference_data) if isinstance(self.reference_data, dict) else self.reference_data
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
            data['created_at'] = datetime.fromisoformat(data['created_at'].replace('Z', '+00:00'))
        if data.get('updated_at'):
            data['updated_at'] = datetime.fromisoformat(data['updated_at'].replace('Z', '+00:00'))
        return cls(**data)

