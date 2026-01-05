import 'package:flutter/material.dart';
import 'package:flutter_bloc/flutter_bloc.dart';
import 'package:supabase_flutter/supabase_flutter.dart';
import '../models/exercise.dart';
import '../repositories/exercises_repository.dart';
import '../repositories/recordings_repository.dart';
import '../widgets/exercise_card.dart';
import 'package:mobile_app_flutter/utils/app_localizations.dart';
import '../utils/app_theme.dart';

class ExercisesScreen extends StatefulWidget {
  const ExercisesScreen({super.key});

  @override
  State<ExercisesScreen> createState() => _ExercisesScreenState();
}

class _ExercisesScreenState extends State<ExercisesScreen> {
  late ExercisesRepository _exercisesRepository;
  ExerciseCategory? _selectedCategory;
  bool _showOnlyFavorites = false;
  Set<String> _favoriteIds = {};
  final _supabase = Supabase.instance.client;

  @override
  void initState() {
    super.initState();
    _exercisesRepository = ExercisesRepository(
      context.read<RecordingsRepository>(),
    );
    _loadFavorites();
  }

  Future<void> _loadFavorites() async {
    final metadata = _supabase.auth.currentUser?.userMetadata;
    final prefs = metadata?['preferences'] as Map<String, dynamic>? ?? {};
    final favorites = prefs['favorite_exercises'] as List<dynamic>? ?? [];
    setState(() {
      _favoriteIds = favorites.map((e) => e.toString()).toSet();
      _showOnlyFavorites = _favoriteIds.isNotEmpty;
    });
  }

  Future<void> _toggleFavorite(String exerciseId) async {
    final newFavorites = Set<String>.from(_favoriteIds);
    if (newFavorites.contains(exerciseId)) {
      newFavorites.remove(exerciseId);
    } else {
      newFavorites.add(exerciseId);
    }

    setState(() {
      _favoriteIds = newFavorites;
    });

    try {
      final metadata = _supabase.auth.currentUser?.userMetadata ?? {};
      final prefs = Map<String, dynamic>.from(metadata['preferences'] ?? {});
      prefs['favorite_exercises'] = newFavorites.toList();
      
      await _supabase.auth.updateUser(
        UserAttributes(data: {'preferences': prefs}),
      );
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('Failed to update favorites')),
        );
      }
    }
  }

  List<Exercise> _getFilteredExercises() {
    var exercises = _exercisesRepository.getAllExercises(category: _selectedCategory);
    
    // Apply favorite filter
    if (_showOnlyFavorites) {
      exercises = exercises.where((e) => _favoriteIds.contains(e.id)).toList();
    }

    return exercises.map((e) {
      return e.copyWith(isFavorite: _favoriteIds.contains(e.id));
    }).toList();
  }

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);
    return Container(
      decoration: const BoxDecoration(
        gradient: AppTheme.backgroundGradient,
      ),
      child: SafeArea(
        bottom: false,
        child: CustomScrollView(
          slivers: [
            // Category filter chips at the top
            SliverToBoxAdapter(
              child: Padding(
                padding: const EdgeInsets.all(AppTheme.spacingM),
                child: SingleChildScrollView(
                  scrollDirection: Axis.horizontal,
                  child: Row(
                    children: [
                      // Favorites Chip
                      Padding(
                        padding: const EdgeInsets.only(right: AppTheme.spacingS),
                        child: FilterChip(
                          avatar: Icon(
                            _showOnlyFavorites ? Icons.star : Icons.star_border,
                            size: 16,
                            color: _showOnlyFavorites ? Colors.white : Colors.white70,
                          ),
                          label: Text(l10n.translate('favorites')),
                          selected: _showOnlyFavorites,
                          onSelected: (selected) {
                            setState(() {
                              _showOnlyFavorites = selected;
                              if (selected) _selectedCategory = null;
                            });
                          },
                          backgroundColor: AppTheme.surfaceColor,
                          selectedColor: AppTheme.warningColor,
                          checkmarkColor: Colors.white,
                          showCheckmark: false,
                          pressElevation: 0,
                          labelStyle: Theme.of(context).textTheme.bodyMedium?.copyWith(
                                color: _showOnlyFavorites ? Colors.white : Colors.white70,
                                fontWeight: _showOnlyFavorites ? FontWeight.bold : FontWeight.normal,
                              ),
                          shape: RoundedRectangleBorder(
                            borderRadius: BorderRadius.circular(AppTheme.radiusM),
                          ),
                          side: BorderSide(
                            color: _showOnlyFavorites 
                                ? AppTheme.warningColor 
                                : Colors.white.withOpacity(0.2),
                          ),
                        ),
                      ),
                      _buildCategoryChip(null, l10n.translate('category_all')),
                      ...ExerciseCategory.values.map(
                        (category) => _buildCategoryChip(
                          category,
                          _getLocalizedCategory(category, l10n),
                        ),
                      ),
                    ],
                  ),
                ),
              ),
            ),

            // All Exercises List
            SliverPadding(
              padding: const EdgeInsets.fromLTRB(
                AppTheme.spacingM,
                0,
                AppTheme.spacingM,
                AppTheme.spacingXL,
              ),
              sliver: SliverList(
                delegate: SliverChildListDelegate(
                  _getFilteredExercises().map((exercise) {
                    return ExerciseCard(
                      exercise: exercise,
                      onTap: () {
                        _showExerciseDetail(context, exercise);
                      },
                      onFavoriteToggle: () => _toggleFavorite(exercise.id),
                    );
                  }).toList(),
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildCategoryChip(ExerciseCategory? category, String label) {
    final isSelected = !_showOnlyFavorites && _selectedCategory == category;
    
    return Padding(
      padding: const EdgeInsets.only(right: AppTheme.spacingS),
      child: FilterChip(
        label: Text(label),
        selected: isSelected,
        onSelected: (selected) {
          setState(() {
            _selectedCategory = selected ? category : null;
            if (selected) _showOnlyFavorites = false;
          });
        },
        backgroundColor: AppTheme.surfaceColor,
        selectedColor: AppTheme.primaryColor,
        checkmarkColor: Colors.white,
        labelStyle: Theme.of(context).textTheme.bodyMedium?.copyWith(
              color: isSelected ? Colors.white : Colors.white70,
              fontWeight: isSelected ? FontWeight.w600 : FontWeight.normal,
            ),
        side: BorderSide(
          color: isSelected
              ? AppTheme.primaryColor
              : Colors.white.withOpacity(0.2),
          width: 1,
        ),
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(AppTheme.radiusM),
        ),
      ),
    );
  }

  void _showExerciseDetail(BuildContext context, Exercise exercise) {
    showModalBottomSheet(
      context: context,
      backgroundColor: Colors.transparent,
      isScrollControlled: true,
      builder: (context) {
        final l10n = AppLocalizations.of(context);
        return Container(
          height: MediaQuery.of(context).size.height * 0.7,
          decoration: BoxDecoration(
            gradient: AppTheme.cardGradient,
            borderRadius: const BorderRadius.vertical(
              top: Radius.circular(AppTheme.radiusXL),
            ),
            border: Border.all(
              color: Colors.white.withOpacity(0.1),
              width: 1,
            ),
          ),
          child: Column(
            children: [
              // Handle bar
              Container(
                margin: const EdgeInsets.only(top: AppTheme.spacingM),
                width: 40,
                height: 4,
                decoration: BoxDecoration(
                  color: Colors.white.withOpacity(0.3),
                  borderRadius: BorderRadius.circular(2),
                ),
              ),
              Expanded(
                child: SingleChildScrollView(
                  padding: const EdgeInsets.all(AppTheme.spacingL),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      // Category and difficulty
                      Row(
                        children: [
                          Container(
                            padding: const EdgeInsets.all(AppTheme.spacingS),
                            decoration: BoxDecoration(
                              gradient: AppTheme.primaryGradient,
                              borderRadius: BorderRadius.circular(AppTheme.radiusS),
                            ),
                            child: Icon(
                              _getCategoryIcon(exercise.category),
                              color: Colors.white,
                              size: 24,
                            ),
                          ),
                          const SizedBox(width: AppTheme.spacingS),
                          Text(
                            _getLocalizedCategory(exercise.category, l10n),
                            style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                                  color: AppTheme.primaryColor,
                                  fontWeight: FontWeight.w600,
                                ),
                          ),
                        ],
                      ),
                      const SizedBox(height: AppTheme.spacingL),
                      // Title
                      Text(
                        exercise.title,
                        style: Theme.of(context).textTheme.displaySmall?.copyWith(
                              fontWeight: FontWeight.bold,
                            ),
                      ),
                      const SizedBox(height: AppTheme.spacingM),
                      // Description
                      Text(
                        exercise.description,
                        style: Theme.of(context).textTheme.bodyLarge,
                      ),
                      const SizedBox(height: AppTheme.spacingL),
                      // Metadata
                      _buildDetailRow(
                        Icons.signal_cellular_alt,
                        l10n.translate('difficulty'),
                        _getLocalizedDifficulty(exercise.difficulty, l10n),
                      ),
                      const SizedBox(height: AppTheme.spacingS),
                      _buildDetailRow(
                        Icons.access_time,
                        l10n.translate('duration'),
                        '${exercise.durationMinutes} ${l10n.translate('minutes')}',
                      ),
                      const SizedBox(height: AppTheme.spacingL),
                      // Focus areas
                      Text(
                        l10n.translate('focus_areas'),
                        style: Theme.of(context).textTheme.headlineSmall?.copyWith(
                              fontWeight: FontWeight.bold,
                            ),
                      ),
                      const SizedBox(height: AppTheme.spacingS),
                      Wrap(
                        spacing: AppTheme.spacingS,
                        runSpacing: AppTheme.spacingS,
                        children: exercise.focusAreas.map((area) {
                          return Container(
                            padding: const EdgeInsets.symmetric(
                              horizontal: AppTheme.spacingM,
                              vertical: AppTheme.spacingS,
                            ),
                            decoration: BoxDecoration(
                              color: AppTheme.primaryColor.withOpacity(0.2),
                              borderRadius: BorderRadius.circular(AppTheme.radiusM),
                              border: Border.all(
                                color: AppTheme.primaryColor.withOpacity(0.5),
                                width: 1,
                              ),
                            ),
                            child: Text(
                              area,
                              style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                                    color: AppTheme.primaryLight,
                                    fontWeight: FontWeight.w600,
                                  ),
                            ),
                          );
                        }).toList(),
                      ),
                      const SizedBox(height: AppTheme.spacingXL),
                      // Start button
                      SizedBox(
                        width: double.infinity,
                        child: ElevatedButton.icon(
                          onPressed: () {
                            Navigator.pop(context);
                            // TODO: Navigate to practice session
                            ScaffoldMessenger.of(context).showSnackBar(
                              SnackBar(
                                content: Text('${l10n.translate('starting')} ${exercise.title}...'),
                                backgroundColor: AppTheme.primaryColor,
                              ),
                            );
                          },
                          icon: const Icon(Icons.play_arrow),
                          label: Text(l10n.translate('start_exercise')),
                          style: ElevatedButton.styleFrom(
                            padding: const EdgeInsets.symmetric(
                              vertical: AppTheme.spacingM,
                            ),
                          ),
                        ),
                      ),
                    ],
                  ),
                ),
              ),
            ],
          ),
        );
      },
    );
  }

  Widget _buildDetailRow(IconData icon, String label, String value) {
    return Row(
      children: [
        Icon(icon, size: 20, color: Colors.white54),
        const SizedBox(width: AppTheme.spacingS),
        Text(
          '$label: ',
          style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                color: Colors.white54,
              ),
        ),
        Text(
          value,
          style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                fontWeight: FontWeight.w600,
              ),
        ),
      ],
    );
  }

  IconData _getCategoryIcon(ExerciseCategory category) {
    switch (category) {
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

  String _getLocalizedCategory(ExerciseCategory category, AppLocalizations l10n) {
    switch (category) {
      case ExerciseCategory.scales: return l10n.translate('category_scales');
      case ExerciseCategory.arpeggios: return l10n.translate('category_arpeggios');
      case ExerciseCategory.sightReading: return l10n.translate('category_sight_reading');
      case ExerciseCategory.technique: return l10n.translate('category_technique');
      case ExerciseCategory.rhythm: return l10n.translate('category_rhythm');
      case ExerciseCategory.tone: return l10n.translate('category_tone');
    }
  }

  String _getLocalizedDifficulty(ExerciseDifficulty difficulty, AppLocalizations l10n) {
    switch (difficulty) {
      case ExerciseDifficulty.beginner: return l10n.translate('difficulty_beginner');
      case ExerciseDifficulty.intermediate: return l10n.translate('difficulty_intermediate');
      case ExerciseDifficulty.advanced: return l10n.translate('difficulty_advanced');
    }
  }
}
