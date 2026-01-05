import '../models/exercise.dart';
import '../repositories/recordings_repository.dart';

class ExercisesRepository {
  final RecordingsRepository _recordingsRepository;

  ExercisesRepository(this._recordingsRepository);

  // Predefined exercise library
  static final List<Exercise> _exerciseLibrary = [
    // Scales
    Exercise(
      id: 'scale_c_major',
      title: 'C Major Scale',
      description: 'Practice the fundamental C major scale across two octaves. Focus on even tone and consistent rhythm.',
      difficulty: ExerciseDifficulty.beginner,
      durationMinutes: 5,
      category: ExerciseCategory.scales,
      focusAreas: ['Pitch Accuracy', 'Tone Quality', 'Finger Technique'],
    ),
    Exercise(
      id: 'scale_chromatic',
      title: 'Chromatic Scale',
      description: 'Work through all 12 notes chromatically. Great for developing finger dexterity and pitch accuracy.',
      difficulty: ExerciseDifficulty.intermediate,
      durationMinutes: 8,
      category: ExerciseCategory.scales,
      focusAreas: ['Pitch Accuracy', 'Finger Technique', 'Intonation'],
    ),
    Exercise(
      id: 'scale_g_major',
      title: 'G Major Scale',
      description: 'Practice G major scale with focus on smooth transitions and consistent tone.',
      difficulty: ExerciseDifficulty.beginner,
      durationMinutes: 5,
      category: ExerciseCategory.scales,
      focusAreas: ['Pitch Accuracy', 'Tone Quality'],
    ),
    
    // Arpeggios
    Exercise(
      id: 'arpeggio_c_major',
      title: 'C Major Arpeggio',
      description: 'Practice C major arpeggio (C-E-G-C) focusing on clean note transitions and intonation.',
      difficulty: ExerciseDifficulty.intermediate,
      durationMinutes: 6,
      category: ExerciseCategory.arpeggios,
      focusAreas: ['Pitch Accuracy', 'Interval Training', 'Breath Control'],
    ),
    Exercise(
      id: 'arpeggio_dominant_7th',
      title: 'Dominant 7th Arpeggios',
      description: 'Work through dominant 7th arpeggios in various keys. Develops harmonic understanding.',
      difficulty: ExerciseDifficulty.advanced,
      durationMinutes: 10,
      category: ExerciseCategory.arpeggios,
      focusAreas: ['Pitch Accuracy', 'Music Theory', 'Interval Training'],
    ),
    
    // Rhythm
    Exercise(
      id: 'rhythm_long_tones',
      title: 'Long Tone Practice',
      description: 'Hold notes for extended periods to develop breath control and tone stability.',
      difficulty: ExerciseDifficulty.beginner,
      durationMinutes: 8,
      category: ExerciseCategory.tone,
      focusAreas: ['Tone Quality', 'Breath Control', 'Pitch Stability'],
    ),
    Exercise(
      id: 'rhythm_patterns',
      title: 'Rhythmic Patterns',
      description: 'Practice various rhythmic patterns including dotted notes, triplets, and syncopation.',
      difficulty: ExerciseDifficulty.intermediate,
      durationMinutes: 10,
      category: ExerciseCategory.rhythm,
      focusAreas: ['Rhythm Accuracy', 'Timing', 'Articulation'],
    ),
    
    // Technique
    Exercise(
      id: 'technique_articulation',
      title: 'Articulation Exercises',
      description: 'Practice different articulation styles: staccato, legato, and accents.',
      difficulty: ExerciseDifficulty.intermediate,
      durationMinutes: 7,
      category: ExerciseCategory.technique,
      focusAreas: ['Articulation', 'Tongue Control', 'Breath Control'],
    ),
    Exercise(
      id: 'technique_dynamics',
      title: 'Dynamic Control',
      description: 'Work on crescendos and diminuendos to develop expressive playing.',
      difficulty: ExerciseDifficulty.advanced,
      durationMinutes: 8,
      category: ExerciseCategory.technique,
      focusAreas: ['Dynamics', 'Breath Control', 'Tone Quality'],
    ),
    
    // Sight Reading
    Exercise(
      id: 'sight_reading_simple',
      title: 'Simple Melodies',
      description: 'Practice sight-reading simple melodies in common time signatures.',
      difficulty: ExerciseDifficulty.beginner,
      durationMinutes: 10,
      category: ExerciseCategory.sightReading,
      focusAreas: ['Reading Skills', 'Rhythm Accuracy', 'Pitch Accuracy'],
    ),
    Exercise(
      id: 'sight_reading_complex',
      title: 'Complex Rhythms',
      description: 'Challenge yourself with complex rhythmic patterns and key changes.',
      difficulty: ExerciseDifficulty.advanced,
      durationMinutes: 12,
      category: ExerciseCategory.sightReading,
      focusAreas: ['Reading Skills', 'Rhythm Accuracy', 'Key Signatures'],
    ),
  ];

  /// Get recommended exercises based on recent practice feedback
  Future<List<Exercise>> getRecommendedExercises() async {
    try {
      // Fetch recent recordings to analyze
      final recordings = await _recordingsRepository.getRecordings();
      
      // Analyze feedback to identify weak areas
      final weakAreas = _analyzeWeakAreas(recordings);
      
      // Get exercises that target those weak areas
      final recommended = _getExercisesForWeakAreas(weakAreas);
      
      return recommended;
    } catch (e) {
      // If analysis fails, return beginner-friendly exercises
      return _exerciseLibrary
          .where((e) => e.difficulty == ExerciseDifficulty.beginner)
          .take(3)
          .toList();
    }
  }

  /// Get all exercises, optionally filtered by category or difficulty
  List<Exercise> getAllExercises({
    ExerciseCategory? category,
    ExerciseDifficulty? difficulty,
  }) {
    var exercises = List<Exercise>.from(_exerciseLibrary);
    
    if (category != null) {
      exercises = exercises.where((e) => e.category == category).toList();
    }
    
    if (difficulty != null) {
      exercises = exercises.where((e) => e.difficulty == difficulty).toList();
    }
    
    return exercises;
  }

  /// Analyze recordings to identify weak areas
  Set<String> _analyzeWeakAreas(List<Recording> recordings) {
    final weakAreas = <String>{};
    
    // Take the 5 most recent recordings
    final recentRecordings = recordings.take(5);
    
    for (final recording in recentRecordings) {
      if (recording.feedback != null) {
        // In a real implementation, parse the feedback JSON
        // For now, we'll use heuristics based on the presence of feedback
        
        // If feedback exists, suggest fundamental exercises
        weakAreas.addAll([
          'Pitch Accuracy',
          'Tone Quality',
          'Rhythm Accuracy',
        ]);
      }
    }
    
    // If no specific weak areas identified, suggest general practice
    if (weakAreas.isEmpty) {
      weakAreas.addAll(['Pitch Accuracy', 'Tone Quality']);
    }
    
    return weakAreas;
  }

  /// Get exercises that target specific weak areas
  List<Exercise> _getExercisesForWeakAreas(Set<String> weakAreas) {
    final recommended = <Exercise>[];
    
    // Find exercises that match the weak areas
    for (final exercise in _exerciseLibrary) {
      final matchingAreas = exercise.focusAreas
          .where((area) => weakAreas.contains(area))
          .length;
      
      if (matchingAreas > 0) {
        recommended.add(exercise.copyWith(isRecommended: true));
      }
    }
    
    // Sort by relevance (number of matching focus areas) and difficulty
    recommended.sort((a, b) {
      final aMatches = a.focusAreas.where((area) => weakAreas.contains(area)).length;
      final bMatches = b.focusAreas.where((area) => weakAreas.contains(area)).length;
      
      if (aMatches != bMatches) {
        return bMatches.compareTo(aMatches); // More matches first
      }
      
      // If equal matches, prefer easier exercises
      return a.difficulty.index.compareTo(b.difficulty.index);
    });
    
    // Return top 6 recommendations
    return recommended.take(6).toList();
  }
}
