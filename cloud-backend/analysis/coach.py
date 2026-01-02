"""
AI Coach Module
Generates personalized coaching feedback using Claude API
"""

import json
from anthropic import Anthropic


class AICoach:
    """Generates encouraging, actionable music coaching feedback"""
    
    def __init__(self, api_key=None):
        self.client = Anthropic(api_key=api_key) if api_key else None
    
    def generate_feedback(self, mistakes, reference_data, metadata):
        """
        Generate coaching feedback based on detected mistakes
        
        Args:
            mistakes: List of detected mistakes
            reference_data: Reference music data
            metadata: Performance metadata
        
        Returns:
            Dictionary with feedback text and structured suggestions
        """
        if not self.client:
            return self._generate_fallback_feedback(mistakes)
        
        # Prepare context for AI
        context = self._prepare_context(mistakes, reference_data, metadata)
        
        try:
            message = self.client.messages.create(
                model="claude-3-5-sonnet-20241022",
                max_tokens=1000,
                messages=[{
                    "role": "user",
                    "content": context
                }]
            )
            
            feedback_text = message.content[0].text
            
            return {
                'text': feedback_text,
                'mistakes_count': len(mistakes),
                'suggestions': self._extract_suggestions(mistakes)
            }
        
        except Exception as e:
            print(f"Error calling Claude API: {e}")
            return self._generate_fallback_feedback(mistakes)
    
    def _prepare_context(self, mistakes, reference_data, metadata):
        """Prepare context prompt for AI"""
        mistakes_summary = self._summarize_mistakes(mistakes)
        
        context = f"""You are an encouraging and supportive music teacher. A student just played a musical piece, and here's what I detected:

MISTAKES DETECTED:
{json.dumps(mistakes_summary, indent=2)}

REFERENCE INFORMATION:
- Tempo: {metadata.get('tempo', 'Unknown')} BPM
- Key Signature: {reference_data.get('key_signature', 'Unknown')}
- Total Notes: {len(reference_data.get('notes', []))}

Generate spoken coaching feedback that:
1. Starts with encouragement
2. Is specific and actionable (references exact measure numbers when available)
3. Suggests marking the sheet music
4. Groups related mistakes together
5. Is direct but supportive
6. Ends with a positive note

Keep the feedback conversational and natural, as if you're speaking directly to the student. Maximum 300 words."""

        return context
    
    def _summarize_mistakes(self, mistakes):
        """Summarize mistakes by type for AI context"""
        summary = {
            'note_accuracy': [],
            'timing': [],
            'dynamics': []
        }
        
        for mistake in mistakes:
            mistake_type = mistake.get('type', 'unknown')
            
            if mistake_type == 'note_accuracy':
                summary['note_accuracy'].append({
                    'measure': mistake.get('measure', '?'),
                    'expected': mistake.get('expected'),
                    'played': mistake.get('played'),
                    'timestamp': mistake.get('timestamp')
                })
            elif mistake_type in ['hesitation', 'rushing', 'tempo_deviation']:
                summary['timing'].append({
                    'type': mistake_type,
                    'timestamp': mistake.get('timestamp'),
                    'details': {k: v for k, v in mistake.items() if k not in ['type', 'timestamp']}
                })
            elif mistake_type == 'dynamics':
                summary['dynamics'].append({
                    'measure': mistake.get('measure', '?'),
                    'marking': mistake.get('marking'),
                    'timestamp': mistake.get('timestamp')
                })
        
        return summary
    
    def _extract_suggestions(self, mistakes):
        """Extract actionable suggestions from mistakes"""
        suggestions = []
        
        for mistake in mistakes:
            if mistake['type'] == 'note_accuracy':
                suggestions.append({
                    'action': 'mark_note',
                    'measure': mistake.get('measure'),
                    'message': f"Circle the {mistake.get('expected')} in measure {mistake.get('measure')}"
                })
            elif mistake['type'] == 'hesitation':
                suggestions.append({
                    'action': 'mark_timing',
                    'timestamp': mistake.get('timestamp'),
                    'message': f"Mark 'prep early' at {self._format_time(mistake.get('timestamp'))}"
                })
            elif mistake['type'] == 'dynamics':
                suggestions.append({
                    'action': 'mark_dynamic',
                    'measure': mistake.get('measure'),
                    'message': f"Emphasize the {mistake.get('marking')} marking in measure {mistake.get('measure')}"
                })
        
        return suggestions
    
    def _format_time(self, seconds):
        """Format seconds to MM:SS"""
        mins = int(seconds // 60)
        secs = int(seconds % 60)
        return f"{mins}:{secs:02d}"
    
    def _generate_fallback_feedback(self, mistakes):
        """Generate feedback without AI API (fallback)"""
        if not mistakes:
            return {
                'text': "Excellent performance! You played everything correctly. Keep up the great work!",
                'mistakes_count': 0,
                'suggestions': []
            }
        
        feedback_parts = []
        
        note_mistakes = [m for m in mistakes if m['type'] == 'note_accuracy']
        timing_mistakes = [m for m in mistakes if m['type'] in ['hesitation', 'rushing']]
        dynamics_mistakes = [m for m in mistakes if m['type'] == 'dynamics']
        
        if note_mistakes:
            feedback_parts.append(f"Found {len(note_mistakes)} note accuracy issues. Review the marked measures.")
        
        if timing_mistakes:
            feedback_parts.append(f"Detected {len(timing_mistakes)} timing issues. Practice with a metronome.")
        
        if dynamics_mistakes:
            feedback_parts.append(f"Pay attention to {len(dynamics_mistakes)} dynamic markings.")
        
        return {
            'text': " ".join(feedback_parts),
            'mistakes_count': len(mistakes),
            'suggestions': self._extract_suggestions(mistakes)
        }

