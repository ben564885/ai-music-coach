import 'package:flutter/material.dart';
import '../models/exercise.dart';
import '../utils/app_theme.dart';

class ExerciseCard extends StatelessWidget {
  final Exercise exercise;
  final VoidCallback? onTap;
  final VoidCallback? onFavoriteToggle;

  const ExerciseCard({
    super.key,
    required this.exercise,
    this.onTap,
    this.onFavoriteToggle,
  });

  Color _getDifficultyColor() {
    switch (exercise.difficulty) {
      case ExerciseDifficulty.beginner:
        return AppTheme.successColor;
      case ExerciseDifficulty.intermediate:
        return AppTheme.warningColor;
      case ExerciseDifficulty.advanced:
        return AppTheme.errorColor;
    }
  }

  IconData _getCategoryIcon() {
    switch (exercise.category) {
      case ExerciseCategory.scales:
        return Icons.stairs;
      case ExerciseCategory.arpeggios:
        return Icons.waves;
      case ExerciseCategory.sightReading:
        return Icons.menu_book;
      case ExerciseCategory.technique:
        return Icons.build;
      case ExerciseCategory.rhythm:
        return Icons.graphic_eq;
      case ExerciseCategory.tone:
        return Icons.tune;
    }
  }

  @override
  Widget build(BuildContext context) {
    final difficultyColor = _getDifficultyColor();
    
    return Container(
      margin: const EdgeInsets.only(bottom: AppTheme.spacingM),
      decoration: BoxDecoration(
        gradient: AppTheme.cardGradient,
        borderRadius: BorderRadius.circular(AppTheme.radiusL),
        border: Border.all(
          color: exercise.isRecommended
              ? AppTheme.primaryColor.withOpacity(0.5)
              : Colors.white.withOpacity(0.1),
          width: exercise.isRecommended ? 2 : 1,
        ),
        boxShadow: exercise.isRecommended
            ? [
                BoxShadow(
                  color: AppTheme.primaryColor.withOpacity(0.3),
                  blurRadius: 20,
                  offset: const Offset(0, 8),
                ),
              ]
            : AppTheme.cardShadow,
      ),
      child: Material(
        color: Colors.transparent,
        child: InkWell(
          onTap: onTap,
          borderRadius: BorderRadius.circular(AppTheme.radiusL),
          child: Padding(
            padding: const EdgeInsets.all(AppTheme.spacingM),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                // Header row
                Row(
                  children: [
                    // Category icon
                    Container(
                      padding: const EdgeInsets.all(AppTheme.spacingS),
                      decoration: BoxDecoration(
                        gradient: AppTheme.primaryGradient,
                        borderRadius: BorderRadius.circular(AppTheme.radiusS),
                      ),
                      child: Icon(
                        _getCategoryIcon(),
                        color: Colors.white,
                        size: 20,
                      ),
                    ),
                    const SizedBox(width: AppTheme.spacingS),
                    // Category label
                    Text(
                      exercise.category.displayName,
                      style: Theme.of(context).textTheme.bodySmall?.copyWith(
                            color: AppTheme.primaryColor,
                            fontWeight: FontWeight.w600,
                          ),
                    ),
                    const Spacer(),
                    // Favorite button
                    IconButton(
                      onPressed: onFavoriteToggle,
                      icon: Icon(
                        exercise.isFavorite ? Icons.star : Icons.star_border,
                        color: exercise.isFavorite ? AppTheme.warningColor : Colors.white24,
                        size: 28,
                      ),
                      padding: EdgeInsets.zero,
                      constraints: const BoxConstraints(),
                    ),
                  ],
                ),
                const SizedBox(height: AppTheme.spacingM),
                // Title
                Text(
                  exercise.title,
                  style: Theme.of(context).textTheme.headlineSmall?.copyWith(
                        fontWeight: FontWeight.bold,
                      ),
                ),
                const SizedBox(height: AppTheme.spacingS),
                // Description
                Text(
                  exercise.description,
                  style: Theme.of(context).textTheme.bodyMedium,
                  maxLines: 2,
                  overflow: TextOverflow.ellipsis,
                ),
                const SizedBox(height: AppTheme.spacingM),
                // Metadata row
                Row(
                  children: [
                    // Difficulty badge
                    Container(
                      padding: const EdgeInsets.symmetric(
                        horizontal: AppTheme.spacingS,
                        vertical: AppTheme.spacingXS,
                      ),
                      decoration: BoxDecoration(
                        color: difficultyColor.withOpacity(0.2),
                        borderRadius: BorderRadius.circular(AppTheme.radiusS),
                        border: Border.all(
                          color: difficultyColor,
                          width: 1,
                        ),
                      ),
                      child: Text(
                        exercise.difficulty.displayName,
                        style: Theme.of(context).textTheme.bodySmall?.copyWith(
                              color: difficultyColor,
                              fontWeight: FontWeight.bold,
                              fontSize: 11,
                            ),
                      ),
                    ),
                    const SizedBox(width: AppTheme.spacingS),
                    // Duration
                    Icon(
                      Icons.access_time,
                      size: 14,
                      color: Colors.white54,
                    ),
                    const SizedBox(width: 4),
                    Text(
                      '${exercise.durationMinutes} min',
                      style: Theme.of(context).textTheme.bodySmall,
                    ),
                  ],
                ),
                // Focus areas
                if (exercise.focusAreas.isNotEmpty) ...[
                  const SizedBox(height: AppTheme.spacingS),
                  Wrap(
                    spacing: AppTheme.spacingXS,
                    runSpacing: AppTheme.spacingXS,
                    children: exercise.focusAreas.take(3).map((area) {
                      return Container(
                        padding: const EdgeInsets.symmetric(
                          horizontal: AppTheme.spacingS,
                          vertical: 2,
                        ),
                        decoration: BoxDecoration(
                          color: Colors.white.withOpacity(0.1),
                          borderRadius: BorderRadius.circular(AppTheme.radiusS),
                        ),
                        child: Text(
                          area,
                          style: Theme.of(context).textTheme.bodySmall?.copyWith(
                                fontSize: 10,
                              ),
                        ),
                      );
                    }).toList(),
                  ),
                ],
              ],
            ),
          ),
        ),
      ),
    );
  }
}
