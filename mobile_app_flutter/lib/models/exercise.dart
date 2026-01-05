enum ExerciseDifficulty {
  beginner,
  intermediate,
  advanced;

  String get displayName {
    switch (this) {
      case ExerciseDifficulty.beginner:
        return 'Beginner';
      case ExerciseDifficulty.intermediate:
        return 'Intermediate';
      case ExerciseDifficulty.advanced:
        return 'Advanced';
    }
  }
}

enum ExerciseCategory {
  scales,
  arpeggios,
  sightReading,
  technique,
  rhythm,
  tone;

  String get displayName {
    switch (this) {
      case ExerciseCategory.scales:
        return 'Scales';
      case ExerciseCategory.arpeggios:
        return 'Arpeggios';
      case ExerciseCategory.sightReading:
        return 'Sight Reading';
      case ExerciseCategory.technique:
        return 'Technique';
      case ExerciseCategory.rhythm:
        return 'Rhythm';
      case ExerciseCategory.tone:
        return 'Tone';
    }
  }
}

class Exercise {
  final String id;
  final String title;
  final String description;
  final ExerciseDifficulty difficulty;
  final int durationMinutes;
  final ExerciseCategory category;
  final List<String> focusAreas;
  final int completionCount;
  final DateTime? lastAttempted;
  final bool isRecommended;
  final bool isFavorite;

  Exercise({
    required this.id,
    required this.title,
    required this.description,
    required this.difficulty,
    required this.durationMinutes,
    required this.category,
    required this.focusAreas,
    this.completionCount = 0,
    this.lastAttempted,
    this.isRecommended = false,
    this.isFavorite = false,
  });

  Exercise copyWith({
    String? id,
    String? title,
    String? description,
    ExerciseDifficulty? difficulty,
    int? durationMinutes,
    ExerciseCategory? category,
    List<String>? focusAreas,
    int? completionCount,
    DateTime? lastAttempted,
    bool? isRecommended,
    bool? isFavorite,
  }) {
    return Exercise(
      id: id ?? this.id,
      title: title ?? this.title,
      description: description ?? this.description,
      difficulty: difficulty ?? this.difficulty,
      durationMinutes: durationMinutes ?? this.durationMinutes,
      category: category ?? this.category,
      focusAreas: focusAreas ?? this.focusAreas,
      completionCount: completionCount ?? this.completionCount,
      lastAttempted: lastAttempted ?? this.lastAttempted,
      isRecommended: isRecommended ?? this.isRecommended,
      isFavorite: isFavorite ?? this.isFavorite,
    );
  }
}
